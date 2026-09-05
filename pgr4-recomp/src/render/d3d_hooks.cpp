// render/d3d_hooks.cpp
//
// Phase 1 (device + present bring-up): hooks D3DDevice_ClearF and
// D3DDevice_Swap as full plume replacements, and lets
// PGR4_D3D_TryPresentAndUpdateStatus's original guest body keep running
// (status/bookkeeping, not a GPU call) with Video::Present() layered on top.
//
// Video::Init/Shutdown are driven from Pgr4App::OnPreLaunchModule/OnShutdown
// (see pgr4_app.h), not from a guest-function hook: PGR4_D3D_InitGlobalDeviceSingleton
// does not need to run under the native renderer, and the app-lifecycle hook is
// the same window/device-creation order the guest function would have driven.
//
// Phase 2 (resource hooks): buffer/texture/surface/vertex-declaration
// creation and lock/unlock. Every guest-facing D3D9 wrapper function that
// calls one of these primitives (e.g. PGR4_D3D_CreateTextureWrapper calling
// D3DDevice_CreateTexture) is a thin, unhooked call-through -- hooking the
// primitive here is enough; the linker's strong-symbol replacement covers
// every caller automatically, no separate wrapper hooks needed. Same reason
// PGR4_Image_ParseDDSFromMemory and the D3DX texture-from-memory pipeline
// need no hooks at all: that whole (fairly involved) original algorithm
// keeps running unmodified and transparently calls through these hooked
// primitives for the actual create/lock/unlock work.

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/types.h>

#include "render/guest_device.h"
#include "render/guest_gpu.h"
#include "render/guest_heap.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_queue.h"
#include "render/render_state.h"
#include "render/video.h"

namespace {
// PGR4: stand-in for rex::ppc::ImportFunction for guest imports that are
// FM2-engine specific or not yet located in this IDB. Callable with any
// arguments (returns 0) and forwardable via .fn(ctx, base) (no-op), so the
// ported hook bodies compile unchanged and simply do nothing on those paths.
struct Pgr4NoopGuestFn {
  void fn(PPCContext& /*ctx*/, uint8_t* /*base*/) const {}
  template <class... A>
  uint32_t operator()(A&&...) const { return 0; }
};
}  // namespace

using pgr4::render::GuestBuffer;
using pgr4::render::GuestDevice;
using pgr4::render::GuestLockedRect;
using pgr4::render::GuestResource;
using pgr4::render::GuestSurface;
using pgr4::render::GuestSurfaceDesc;
using pgr4::render::GuestVertexElement;

namespace pgr4::render {
GuestBuffer* CreateVertexBuffer(uint32_t length);
GuestBuffer* CreateIndexBuffer(uint32_t length, uint32_t format);
uint32_t LockVertexBuffer(GuestBuffer* buffer, uint32_t flags);
void UnlockVertexBuffer(GuestBuffer* buffer);
uint32_t LockIndexBuffer(GuestBuffer* buffer, uint32_t flags);
void UnlockIndexBuffer(GuestBuffer* buffer);
GuestTexture* CreateTexture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels,
                            uint32_t usage, uint32_t format, uint32_t pool, uint32_t type);
GuestSurface* CreateSurface(uint32_t width, uint32_t height, uint32_t format, uint32_t multiSample,
                            uint32_t edramBase = kNoEdramBase);
void LockRect(GuestBaseTexture* texture, uint32_t level, uint32_t* outPitch, uint32_t* outBits);
void UnlockGuestResource(GuestResource* resource);
GuestVertexDeclaration* CreateVertexDeclaration(const GuestVertexElement* guestElements);
void GetSurfaceDesc(const GuestSurface* surface, GuestSurfaceDesc* desc);
GuestTexture* LoadTextureFromMemory(const uint8_t* data, uint32_t size);
GuestShader* CreateVertexShader(const uint32_t* function);
GuestShader* CreatePixelShader(const uint32_t* function);
GuestShader* LookupShaderAlias(uint32_t guestAddress);
void RegisterShaderAlias(uint32_t guestAddress, GuestShader* shader);
}  // namespace pgr4::render

namespace {

// Bring-up frame trace: every state event during swaps [598, 600] is logged so
// one frontend frame's render-target / resolve sequence can be read back.
std::atomic<uint64_t> g_pgr4SwapIndex{0};
bool InTraceWindow() {
  const uint64_t n = g_pgr4SwapIndex.load(std::memory_order_relaxed);
  return n >= 598 && n <= 600;
}

uint32_t ReadGuestU32At(uint32_t guestAddress) {
  if (guestAddress == 0)
    return 0;
  auto* p = pgr4::ghp::ToHost<const rex::be<uint32_t>>(guestAddress);
  return p != nullptr ? p->get() : 0;
}

float ReadGuestF32At(uint32_t guestAddress) {
  return std::bit_cast<float>(ReadGuestU32At(guestAddress));
}

std::array<float, 3> ReadGuestVec3At(uint32_t guestAddress) {
  return {ReadGuestF32At(guestAddress), ReadGuestF32At(guestAddress + 4),
          ReadGuestF32At(guestAddress + 8)};
}

struct ClothPositionSample {
  uint32_t index = 0;
  std::array<float, 3> position{};
  float magnitude = 0;
  bool nonfinite = false;

  bool Suspect() const { return nonfinite || magnitude > 10000.0f; }
};

// Diagnostic only: inspect every node, including unpinned nodes and flags
// other than the first pre-simulated object. Never alter simulation data.
ClothPositionSample SampleClothPositions(uint32_t nodes, uint32_t count) {
  ClothPositionSample result;
  if (nodes == 0 || count == 0 || count > 4096)
    return result;
  for (uint32_t i = 0; i < count; ++i) {
    const auto position = ReadGuestVec3At(nodes + i * 112u);
    for (float component : position) {
      if (!std::isfinite(component))
        return {i, position, 0, true};
      if (std::abs(component) > result.magnitude)
        result = {i, position, std::abs(component), false};
    }
  }
  return result;
}

// Bounded palette diagnostics retain exact guest bytes for standalone replay.
std::string GuestBytesHex(uint32_t address, uint32_t size) {
  if (address == 0)
    return {};
  const auto* bytes = pgr4::ghp::ToHost<const uint8_t>(address);
  constexpr char digits[] = "0123456789abcdef";
  std::string hex(size * 2u, '0');
  for (uint32_t i = 0; i < size; ++i) {
    hex[i * 2u] = digits[bytes[i] >> 4];
    hex[i * 2u + 1u] = digits[bytes[i] & 15];
  }
  return hex;
}

}  // namespace

namespace {

// D3DDevice_ClearF(device, flags, rect, D3DVECTOR4* color, z, d3dColor).
// color is 4 big-endian floats in guest memory. Now that Phase 3/4 render
// state (render target/viewport/scissor tracking) exists, this does a real
// render-target clear via rr::Clear() instead of only remembering a fallback
// present-time color -- the fallback stays as a safety net for whatever
// Video::Present() blits before any real render target is ever bound.
void ClearF(GuestDevice* device, uint32_t flags, void* /*rect*/, const uint32_t* color, float z,
            uint32_t /*d3dColor*/) {
  if (InTraceWindow())
    REXGPU_INFO("TRACE clear flags=0x{:X}", flags);
  float rgba[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  if (color != nullptr) {
    for (int i = 0; i < 4; ++i) {
      rgba[i] = std::bit_cast<float>(std::byteswap(color[i]));
    }
  }
  Video::SetFallbackClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
  pgr4::render::Clear(device, flags, rgba, z);
}

// D3DDevice_Swap: the real per-frame present trigger for FM2 (confirmed
// against this same guest binary by the reference repo's own investigation;
// PGR4_D3D_TryPresentAndUpdateStatus never actually fires on the live present
// path). The original guest body kicks the real Xenos GPU ring buffer, which
// does not exist under the native renderer, so it must not run here; this
// hook fully replaces it.
//
// arg4 holds the VdSwap-style present descriptor: frontbuffer D3D9 fetch is
// at arg4+28; dword1 & page mask is the frontbuffer base. Prefer a resolve
// destination registered at that base (composited display) over sticky RTs.
void Swap(uint32_t device, uint32_t arg4, uint32_t /*arg5*/, uint32_t caller) {
  static std::atomic<uint64_t> swapCallCount{0};
  const uint64_t swapIndex = swapCallCount.fetch_add(1, std::memory_order_relaxed) + 1;
  g_pgr4SwapIndex.store(swapIndex, std::memory_order_relaxed);
  if (InTraceWindow())
    REXGPU_INFO("TRACE swap n={} descriptor=0x{:08X}", swapIndex, arg4);
  const char* callerKind = caller == 0x82381DDCu   ? "xps-timeout"
                           : caller == 0x825ADE54u ? "tile-buffer"
                                                   : "other";

  pgr4::render::GuestBaseTexture* presentSource = nullptr;
  const char* kind = "none";
  uint32_t fbBase = 0;
  if (arg4 != 0) {
    fbBase = pgr4::ghp::HeaderBaseToPhysical(ReadGuestU32At(arg4 + 28u + 4u)) & ~0xFFFu;
    presentSource = pgr4::render::LookupResolveSurfaceAperture(fbBase);
    if (presentSource != nullptr && presentSource->texture != nullptr) {
      kind = "aperture";
    } else {
      presentSource = nullptr;
    }
  }
  // ponytail: PGR4's boot frames resolve into the video player's own texture
  // and never into the swapped frontbuffer (the composite that would happen
  // through EndTiling isn't running yet), so fall back to the most recent
  // resolve destination. Revisit once the frontend's own passes are seen.
  if (presentSource == nullptr) {
    presentSource = pgr4::render::LookupLastResolveDestination();
    if (presentSource != nullptr)
      kind = "last-resolve";
  }

  const bool accepted = Video::Present(presentSource);
  if (swapIndex <= 64 || swapIndex % 300 == 1) {
    REXGPU_INFO(
        "D3DDevice_Swap: n={} caller={} lr=0x{:08X} device=0x{:08X} "
        "descriptor=0x{:08X} defaultDescriptor={} fbBase=0x{:08X} source={:p} kind={} "
        "{}x{} accepted={} pm4draws={}",
        swapIndex, callerKind, caller, device, arg4, arg4 == device + 0x3884u, fbBase,
        static_cast<const void*>(presentSource), kind,
        presentSource != nullptr ? presentSource->width : 0,
        presentSource != nullptr ? presentSource->height : 0, accepted,
        pgr4::render::g_pm4DrawPackets.exchange(0, std::memory_order_relaxed));
  }
  // Frame bookkeeping is covered by Present's BeginCommandList /
  // OnRecordingFrameReady. An extra sync BeginRenderStateFrame Run here
  // contended with TranslateGuestTexture and worsened post-Swap freezes.
}

}  // namespace

// Capture both sides of the animation palette before it reaches flag physics.
REX_IMPORT(__imp__sub_823DFAA0, g_origWriteBonePalette, void());
REX_HOOK_RAW(sub_823DFAA0) {
  const uint32_t workspace = ctx.r3.u32;
  const uint32_t output = ctx.r4.u32;
  const uint32_t skeleton = ReadGuestU32At(workspace);
  const uint32_t count = ReadGuestU32At(skeleton);
  const uint32_t propagated = ReadGuestU32At(workspace + 40);
  g_origWriteBonePalette.fn(ctx, base);

  static std::atomic<uint32_t> samples{0};
  static std::atomic<uint32_t> anomalies{0};
  if (output == 0 || count == 0 || count > 60 ||
      anomalies.load(std::memory_order_relaxed) >= 2)
    return;
  uint32_t saturatedLane = count * 12u;
  const auto* packed = pgr4::ghp::ToHost<const rex::be<uint16_t>>(output);
  for (uint32_t lane = 0; lane < count * 12u; ++lane) {
    if ((packed[lane].get() & 0x7FFFu) == 0x7FFFu) {
      saturatedLane = lane;
      break;
    }
  }
  const bool suspect = saturatedLane != count * 12u;
  if ((suspect && anomalies.fetch_add(1, std::memory_order_relaxed) < 2) ||
      (!suspect && samples.fetch_add(1, std::memory_order_relaxed) < 2)) {
    const uint32_t local = ReadGuestU32At(workspace + 4);
    const uint32_t global = ReadGuestU32At(workspace + 16);
    REXGPU_INFO("BONEPALETTE workspace=0x{:08X} skeleton=0x{:08X} output=0x{:08X} "
                "bones={} propagated={} saturatedLane={} suspect={} "
                "workspaceHex={} skeletonHex={} localHex={} globalHex={} packedHex={}",
                workspace, skeleton, output, count, propagated, saturatedLane, suspect,
                GuestBytesHex(workspace, 184), GuestBytesHex(skeleton, 1744 + count * 28u),
                GuestBytesHex(local, count * 28u), GuestBytesHex(global, count * 28u),
                GuestBytesHex(output, count * 24u));
  }
}

REX_IMPORT(__imp__sub_823D2B20, g_origReadBoneAttachment, void());
REX_HOOK_RAW(sub_823D2B20) {
  const uint32_t manager = ctx.r3.u32;
  const uint32_t skeleton = ctx.r4.u32;
  const uint32_t paletteIndex = ctx.r5.u32;
  const uint32_t bone = ctx.r6.u32;
  const uint32_t rotationOut = ctx.r7.u32;
  const uint32_t positionOut = ctx.r8.u32;
  const uint32_t palette = ReadGuestU32At(manager + 12);
  const uint32_t matrix = palette + 24u * (paletteIndex + bone);
  g_origReadBoneAttachment.fn(ctx, base);

  static std::atomic<uint32_t> samples{0};
  static std::atomic<uint32_t> anomalies{0};
  if (anomalies.load(std::memory_order_relaxed) >= 8)
    return;
  const auto position = ReadGuestVec3At(positionOut);
  const bool suspect = std::any_of(position.begin(), position.end(), [](float v) {
    return !std::isfinite(v) || std::abs(v) > 10000.0f;
  });
  if ((suspect && anomalies.fetch_add(1, std::memory_order_relaxed) < 8) ||
      (!suspect && samples.fetch_add(1, std::memory_order_relaxed) < 12)) {
    REXGPU_INFO("BONEATTACH manager=0x{:08X} palette=0x{:08X} skeleton=0x{:08X} "
                "paletteIndex={} bone={} allocated={} p=({},{},{}) suspect={} "
                "matrixHex={} bindHex={} rotationHex={}",
                manager, palette, skeleton, paletteIndex, bone, ReadGuestU32At(manager + 16),
                position[0], position[1], position[2], suspect, GuestBytesHex(matrix, 24),
                GuestBytesHex(skeleton + 1744 + bone * 28u, 28), GuestBytesHex(rotationOut, 16));
  }
}

// Follow flag data through initialization, simulation and vertex generation.
// These hooks preserve guest execution and bound logging independently at
// each boundary; a late anomaly cannot be hidden by initialization spam.
REX_IMPORT(__imp__sub_82821BD8, g_origClothInit, void());
REX_HOOK_RAW(sub_82821BD8) {
  const uint32_t simulation = ctx.r3.u32;
  const uint32_t descriptor = ctx.r4.u32;
  const auto row0 = ReadGuestVec3At(descriptor + 32);
  const auto row1 = ReadGuestVec3At(descriptor + 48);
  const auto row2 = ReadGuestVec3At(descriptor + 64);
  const std::array<float, 3> translation = {ReadGuestF32At(descriptor + 44),
      ReadGuestF32At(descriptor + 60), ReadGuestF32At(descriptor + 76)};
  g_origClothInit.fn(ctx, base);
  static std::atomic<uint32_t> reports{0};
  if (reports.fetch_add(1, std::memory_order_relaxed) < 32) {
    const auto sample = SampleClothPositions(ReadGuestU32At(simulation + 8),
                                            ReadGuestU32At(simulation + 16));
    REXGPU_INFO("CLOTHINIT sim=0x{:08X} descriptor=0x{:08X} grid={}x{} "
                "row0=({},{},{}) row1=({},{},{}) row2=({},{},{}) t=({},{},{}) "
                "node={} p=({},{},{}) suspect={}", simulation, descriptor,
                ReadGuestU32At(simulation), ReadGuestU32At(simulation + 4),
                row0[0], row0[1], row0[2], row1[0], row1[1], row1[2],
                row2[0], row2[1], row2[2], translation[0], translation[1], translation[2],
                sample.index, sample.position[0], sample.position[1], sample.position[2],
                sample.Suspect());
  }
}

REX_IMPORT(__imp__sub_828219D8, g_origClothStep, void());
REX_HOOK_RAW(sub_828219D8) {
  static std::atomic<uint32_t> anomalies{0};
  thread_local std::unordered_map<uint32_t, uint32_t> steps;
  const uint32_t simulation = ctx.r3.u32;
  const uint32_t nodes = ReadGuestU32At(simulation + 8);
  const uint32_t count = ReadGuestU32At(simulation + 16);
  const uint32_t step = ++steps[simulation];
  const bool inspect = nodes != 0 && count > 0 && count <= 4096 &&
                       anomalies.load(std::memory_order_relaxed) < 16;
  const auto before = inspect ? SampleClothPositions(nodes, count) : ClothPositionSample{};
  const float delta = inspect ? ReadGuestF32At(ctx.r4.u32) : 0;
  g_origClothStep.fn(ctx, base);
  if (inspect) {
    const auto after = SampleClothPositions(nodes, count);
    const bool suspect = before.Suspect() || after.Suspect();
    if ((suspect && anomalies.fetch_add(1, std::memory_order_relaxed) < 16) ||
        (!suspect && (step == 1 || step == 100 || step == 101))) {
      const auto wind = ReadGuestVec3At(simulation + 224);
      REXGPU_INFO("CLOTHSTEP sim=0x{:08X} step={} nodes=0x{:08X}/{} dt={} "
                  "beforeNode={} p=({},{},{}) afterNode={} p=({},{},{}) "
                  "wind=({},{},{}) suspect={}>{}", simulation, step, nodes, count, delta,
                  before.index, before.position[0], before.position[1], before.position[2],
                  after.index, after.position[0], after.position[1], after.position[2],
                  wind[0], wind[1], wind[2], before.Suspect(), after.Suspect());
    }
  }
}

REX_IMPORT(__imp__sub_8282E0A8, g_origClothVertexFinish, void());
REX_HOOK_RAW(sub_8282E0A8) {
  // The only caller is sub_82359238, which keeps the flag object in r28.
  // The allocator's +76 is the start, and r4 is the end of the written range.
  static std::atomic<uint32_t> reports{0};
  static std::atomic<uint32_t> anomalies{0};
  const uint32_t vertices = ReadGuestU32At(ctx.r3.u32 + 76);
  const uint32_t end = ctx.r4.u32;
  const uint32_t simulation = ctx.r28.u32 + 16;
  const uint32_t nodes = ReadGuestU32At(simulation + 8);
  const uint32_t count = ReadGuestU32At(simulation + 16);
  if (ctx.lr == 0x82359448u && nodes != 0 && count > 0 && count <= 4096 &&
      vertices != 0 && uint64_t(vertices) + uint64_t(count) * 36u == end &&
      anomalies.load(std::memory_order_relaxed) < 16) {
    const auto sample = SampleClothPositions(nodes, count);
    uint32_t mismatch = count;
    for (uint32_t i = 0; i < count && mismatch == count; ++i) {
      for (uint32_t component = 0; component < 3; ++component) {
        if (ReadGuestU32At(nodes + i * 112u + component * 4u) !=
            ReadGuestU32At(vertices + i * 36u + component * 4u)) {
          mismatch = i;
          break;
        }
      }
    }
    const bool suspect = sample.Suspect() || mismatch != count;
    if ((suspect && anomalies.fetch_add(1, std::memory_order_relaxed) < 16) ||
        (!suspect && reports.fetch_add(1, std::memory_order_relaxed) < 24)) {
      const uint32_t index = mismatch != count ? mismatch : sample.index;
      const auto source = ReadGuestVec3At(nodes + index * 112u);
      const auto output = ReadGuestVec3At(vertices + index * 36u);
      REXGPU_INFO("CLOTHVB sim=0x{:08X} nodes=0x{:08X} vertices=0x{:08X}/{} "
                  "node={} source=({},{},{}) output=({},{},{}) equal={} suspect={}",
                  simulation, nodes, vertices, count, index,
                  source[0], source[1], source[2], output[0], output[1], output[2],
                  mismatch == count, suspect);
    }
  }
  g_origClothVertexFinish.fn(ctx, base);
}
// PGR4: PGR4_D3D_TryPresentAndUpdateStatus is FM2-engine or not yet located in this IDB; the guest import is
// stubbed so the ported code links. Its hook (if any) is dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origTryPresentAndUpdateStatus;

namespace {

// Kept as a passthrough: unlike D3DDevice_Swap this is guest status
// bookkeeping, not a GPU-ring call, so the original body still needs to run.
// Do NOT call Video::Present here — Swap is the live present trigger; a second
// Present races g_presentBusy and intermittently drops the real frame (boot
// hang / black after first Swap).
void PresentAndUpdateStatus(uint32_t presentChain) {
  g_origTryPresentAndUpdateStatus(presentChain);
}

}  // namespace

REX_HOOK(D3DDevice_ClearF, ClearF);
// PGR4: unlike FM2, the XDK swap state machine has to keep running -- the
// guest GPU layer (guest_gpu.cpp) consumes the ring the original writes, and
// the game's frame manager paces itself on the submitted/retired frame counters
// and swap fences the original maintains. Present first, then let the original
// run with the untouched register state.
REX_IMPORT(__imp__D3DDevice_Swap, g_origD3DSwap, void(uint32_t, uint32_t, uint32_t));
REX_HOOK_RAW(D3DDevice_Swap) {
  Swap(ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.lr);
  g_origD3DSwap.fn(ctx, base);
}
REX_HOOK(PGR4_D3D_TryPresentAndUpdateStatus, PresentAndUpdateStatus);

// ---------------------------------------------------------------------------
// Phase 2: resource creation / lock / unlock. Pure replacements -- the
// returned guest address is the native object's own address (GuestNew places
// it inside guest memory), no XDK shadow object, no alias table.
// ---------------------------------------------------------------------------

namespace {

namespace rr = pgr4::render;
namespace ghp = pgr4::ghp;

uint32_t CreateVertexBufferHook(uint32_t length, uint32_t /*usage*/, uint32_t /*pool*/) {
  return ghp::ToGuest(rr::CreateVertexBuffer(length));
}

uint32_t CreateIndexBufferHook(uint32_t length, uint32_t /*usage*/, uint32_t format,
                               uint32_t /*pool*/) {
  return ghp::ToGuest(rr::CreateIndexBuffer(length, format));
}

uint32_t CreateTextureHook(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels,
                           uint32_t usage, uint32_t format, uint32_t pool, uint32_t type) {
  return ghp::ToGuest(rr::CreateTexture(width, height, depth, levels, usage, format, pool, type));
}

uint32_t CreateSurfaceHook(uint32_t width, uint32_t height, uint32_t format, uint32_t multiSample,
                           const rex::be<uint32_t>* parameters) {
  // D3DSURFACE_PARAMETERS { Base, HierarchicalZBase, ColorExpBias }: EDRAM
  // tile base the game chose (NULL = XDK allocates). Logged while the EDRAM
  // aliasing model is being built.
  // NULL parameters: the XDK's first-fit EDRAM allocator (D3D::AllocateEdram-
  // Memory) only sees NULL-parameter surfaces, and PGR4 keeps a single one
  // alive (the 720p X8R8G8B8 target), so it lands at tile 0.
  // ponytail: model the allocator if a second NULL-parameter surface shows up.
  const uint32_t edramBase = parameters != nullptr ? (parameters[0].get() & 0xFFFu) : 0u;
  return ghp::ToGuest(rr::CreateSurface(width, height, format, multiSample, edramBase));
}

uint32_t CreateVertexDeclarationHook(const GuestVertexElement* elements) {
  return ghp::ToGuest(rr::CreateVertexDeclaration(elements));
}

// D3DDevice_CreateVertexShader/CreatePixelShader take the raw ShaderContainer
// microcode pointer directly -- these two addresses were mislabeled in the
// manifest as unrelated GPU-memory-block allocators (PGR4_Render_AllocGpuPassMemoryBlock
// / PGR4_D3D_CreateGpuMemoryBlock) despite already being correctly renamed in
// IDA; fixed in pgr4_manifest.toml. Without this fix no vertex/pixel shader
// object is ever created, so SetVertexShaderState/SetPixelShaderState's
// ResolveShader() would never find a real GuestShader to bind.
uint32_t CreateVertexShaderHook(const uint32_t* function) {
  return ghp::ToGuest(rr::CreateVertexShader(function));
}

uint32_t CreatePixelShaderHook(const uint32_t* function) {
  return ghp::ToGuest(rr::CreatePixelShader(function));
}

}  // namespace

// Not everything that reaches Lock/Unlock/GetDesc was created through our
// hooked Create* functions -- FM2 also builds texture-shaped D3DResources
// manually via the low-level XG* XDK API (XGSetTextureHeader +
// XGOffsetResourceAddress), bypassing D3DDevice_CreateTexture entirely
// (confirmed via decompile of one such caller). Those raw XDK objects don't
// have our GuestResource magic header, so IsFm2Resource() distinguishes them
// and the original guest body still runs for anything we didn't create.
REX_IMPORT(__imp__D3DVertexBuffer_Lock, g_origVertexBufferLock,
           uint32_t(uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__D3DIndexBuffer_Lock, g_origIndexBufferLock,
           uint32_t(uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__D3DSurface_LockRect, g_origSurfaceLockRect,
           void(uint32_t, uint32_t, uint32_t, uint32_t));
// D3DTexture_LockRect @ 0x82693348 takes (texture, level, lockedRect, rect);
// the trailing flags slot is kept for the shared hook signature.
REX_IMPORT(__imp__D3DTexture_LockRect, g_origTextureLockRect,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
// D3D::UnlockResource @ 0x82699F80 (resource, baseAddress, mipAddress) --
// every D3D*_Unlock wrapper funnels through it.
REX_IMPORT(__imp__D3D_UnlockResource, g_origUnlockResource, void(uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__D3DSurface_GetDesc, g_origSurfaceGetDesc, void(uint32_t, uint32_t));
// Guest D3DResource_AddRef @ 0x82369D90 / Release @ 0x82369E08 -- BE atomics
// on ReferenceCount (+4). FM2 GuestResource stores host-LE refCount there.
REX_IMPORT(__imp__D3DResource_AddRef, g_origD3DResourceAddRef, uint32_t(uint32_t));
REX_IMPORT(__imp__D3DResource_Release, g_origD3DResourceRelease, uint32_t(uint32_t));

namespace {

uint32_t VertexBufferLockHook(uint32_t bufferAddr, uint32_t offsetToLock, uint32_t sizeToLock,
                              uint32_t flags) {
  auto* buffer = ghp::ToHost<GuestBuffer>(bufferAddr);
  if (!rr::IsFm2Resource(buffer))
    return g_origVertexBufferLock(bufferAddr, offsetToLock, sizeToLock, flags);
  return rr::LockVertexBuffer(buffer, flags);
}

uint32_t IndexBufferLockHook(uint32_t bufferAddr, uint32_t offsetToLock, uint32_t sizeToLock,
                             uint32_t flags) {
  auto* buffer = ghp::ToHost<GuestBuffer>(bufferAddr);
  if (!rr::IsFm2Resource(buffer))
    return g_origIndexBufferLock(bufferAddr, offsetToLock, sizeToLock, flags);
  return rr::LockIndexBuffer(buffer, flags);
}

void SurfaceLockRectHook(uint32_t textureAddr, uint32_t lockedRectAddr, uint32_t rectAddr,
                         uint32_t flags) {
  auto* texture = ghp::ToHost<rr::GuestBaseTexture>(textureAddr);
  if (!rr::IsFm2Resource(texture)) {
    g_origSurfaceLockRect(textureAddr, lockedRectAddr, rectAddr, flags);
    return;
  }
  uint32_t pitch = 0, bits = 0;
  rr::LockRect(texture, 0, &pitch, &bits);
  if (auto* lockedRect = ghp::ToHost<GuestLockedRect>(lockedRectAddr)) {
    lockedRect->pitch = static_cast<int32_t>(pitch);
    lockedRect->bits = bits;
  }
}

// D3D9 textures lock through a *different* entry point than surfaces
// (IDirect3DTexture9::LockRect takes a mip Level; IDirect3DSurface9 doesn't).
// This guest function was previously misnamed PGR4_AudioRender_SubmitFrontBufferPath
// in the manifest/IDA -- confirmed via decompile to be the real texture-lock
// leaf (renamed to PGR4_D3DTexture_LockRect). Missing this hook left the
// original body running against our GuestTexture's non-XDK layout, producing
// a garbage lock pointer that crashed later in TileSurface/PGR4_MemcpyAligned.
// The Level param routes to LockRect so Unlock uploads into the right mip --
// dropping it uploaded every level over mip 0 and left mips 1+ uninitialized
// (minified sampling aliased, e.g. the striped menu backgrounds).
void TextureLockRectHook(uint32_t textureAddr, uint32_t level, uint32_t lockedRectAddr,
                         uint32_t rectAddr, uint32_t flags) {
  auto* texture = ghp::ToHost<rr::GuestBaseTexture>(textureAddr);
  if (!rr::IsFm2Resource(texture)) {
    g_origTextureLockRect(textureAddr, level, lockedRectAddr, rectAddr, flags);
    return;
  }
  uint32_t pitch = 0, bits = 0;
  rr::LockRect(texture, level, &pitch, &bits);
  if (auto* lockedRect = ghp::ToHost<GuestLockedRect>(lockedRectAddr)) {
    lockedRect->pitch = static_cast<int32_t>(pitch);
    lockedRect->bits = bits;
  }
}

void UnlockResourceHook(uint32_t resourceAddr, uint32_t base, uint32_t mip) {
  auto* resource = ghp::ToHost<GuestResource>(resourceAddr);
  if (!rr::IsFm2Resource(resource)) {
    g_origUnlockResource(resourceAddr, base, mip);
    rr::InvalidateGuestTexture(resource);
    return;
  }
  rr::UnlockGuestResource(resource);
}

void SurfaceGetDescHook(uint32_t surfaceAddr, uint32_t descAddr) {
  auto* surface = ghp::ToHost<GuestSurface>(surfaceAddr);
  if (!rr::IsFm2Resource(surface)) {
    g_origSurfaceGetDesc(surfaceAddr, descAddr);
    return;
  }
  rr::GetSurfaceDesc(surface, ghp::ToHost<GuestSurfaceDesc>(descAddr));
}

uint32_t D3DResourceAddRefHook(uint32_t resourceAddr) {
  auto* resource = ghp::ToHost<GuestResource>(resourceAddr);
  if (!rr::IsFm2Resource(resource))
    return g_origD3DResourceAddRef(resourceAddr);
  return resource->refCount.fetch_add(1, std::memory_order_acq_rel) + 1;
}

uint32_t D3DResourceReleaseHook(uint32_t resourceAddr) {
  auto* resource = ghp::ToHost<GuestResource>(resourceAddr);
  if (!rr::IsFm2Resource(resource))
    return g_origD3DResourceRelease(resourceAddr);
  const uint32_t remaining = resource->refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
  if (remaining == 0) {
    // Do not call guest free (sub_82369868): host Plume objects + guest
    // allocation are retired after the recording-frame fence.
    rr::ScheduleResourceDestruction(resource);
  }
  return remaining;
}

// PGR4_D3D_CreateTextureFromMemoryBuffer(textureHolder, data, size): the
// original guest body hands off to the D3DX texture-from-memory pipeline,
// whose internal tiling step (TileSurface) reads a raw GPU-memory-address
// field out of the D3D9 texture object it creates -- our GuestTexture has no
// such field, so letting that pipeline run corrupts memory (confirmed via a
// crash in PGR4_MemcpyAligned during boot). This hook bypasses it completely:
// parse+upload the DDS blob ourselves, then populate textureHolder's fields
// directly (offsets match what the guest's own post-load step,
// sub_825A2538, would have written: +4 texture handle, +12 loaded flag,
// +24 width, +28 height, +32 mip count).
void CreateTextureFromMemoryBufferHook(uint32_t textureHolder, const uint8_t* data, uint32_t size) {
  rr::GuestTexture* texture = rr::LoadTextureFromMemory(data, size);

  auto writeU32 = [&](uint32_t offset, uint32_t value) {
    if (auto* p = ghp::ToHost<rex::be<uint32_t>>(textureHolder + offset))
      p->set(value);
  };
  auto writeU8 = [&](uint32_t offset, uint8_t value) {
    if (auto* p = ghp::ToHost<uint8_t>(textureHolder + offset))
      *p = value;
  };

  writeU32(4, ghp::ToGuest(texture));
  writeU8(12, 1);
  if (texture != nullptr) {
    writeU32(24, texture->width);
    writeU32(28, texture->height);
    writeU32(32, texture->levels);
  }
}

}  // namespace

REX_HOOK(D3DDevice_CreateVertexBuffer, CreateVertexBufferHook);
REX_HOOK(D3DDevice_CreateIndexBuffer, CreateIndexBufferHook);
REX_HOOK(D3DDevice_CreateTexture, CreateTextureHook);
REX_HOOK(D3DDevice_CreateSurface, CreateSurfaceHook);
REX_HOOK(D3DDevice_CreateVertexDeclaration, CreateVertexDeclarationHook);
REX_HOOK(D3DDevice_CreateVertexShader, CreateVertexShaderHook);
REX_HOOK(D3DDevice_CreatePixelShader, CreatePixelShaderHook);
REX_HOOK(D3DVertexBuffer_Lock, VertexBufferLockHook);
REX_HOOK(D3DIndexBuffer_Lock, IndexBufferLockHook);
REX_HOOK(D3DSurface_LockRect, SurfaceLockRectHook);
REX_HOOK(D3DTexture_LockRect, TextureLockRectHook);
REX_HOOK(D3D_UnlockResource, UnlockResourceHook);
REX_HOOK(D3DSurface_GetDesc, SurfaceGetDescHook);
REX_HOOK(PGR4_D3D_CreateTextureFromMemoryBuffer, CreateTextureFromMemoryBufferHook);
REX_HOOK(D3DResource_AddRef, D3DResourceAddRefHook);    // @ 0x82369D90
REX_HOOK(D3DResource_Release, D3DResourceReleaseHook);  // @ 0x82369E08

// PGR4's raw texture allocators reuse headers and assign backing memory here.
// Invalidate after the original has written the new fetch addresses; defer the
// upload until first use, since the caller may still be filling the payload.
REX_EXTERN(__imp__XGOffsetResourceAddress);
REX_HOOK_RAW(XGOffsetResourceAddress) {
  const uint32_t resource = ctx.r3.u32;
  __imp__XGOffsetResourceAddress(ctx, base);
  rr::InvalidateGuestTexture(ghp::ToHost<void>(resource));
}

// ---------------------------------------------------------------------------
// Phase 3: render state, clip planes, bool constants, vertex/index/surface
// binding, shader state. All full replacements EXCEPT these state setters
// still call their original guest body first: unlike D3DDevice_Swap (which
// crashes if its original runs, since it kicks a nonexistent GPU ring),
// these setters are ordinary struct bit-packing with no GPU/OS calls, so
// calling the original keeps any *other*, still-unhooked guest code that
// queries render state (GetRenderState-style reads) internally consistent.
// ---------------------------------------------------------------------------

namespace {

GuestDevice* DeviceForRenderContext(uint32_t renderContext) {
  if (renderContext != 0) {
    auto* device = ghp::ToHost<GuestDevice>(renderContext);
    rr::SetActiveGuestDevice(device);
    return device;
  }
  return rr::GetActiveGuestDevice();
}

}  // namespace

#define PGR4_RS_IMPORT(guestName, callableName) \
  REX_IMPORT(__imp__##guestName, callableName, void(uint32_t, uint32_t))

PGR4_RS_IMPORT(D3DDevice_SetRenderState_AlphaBlendEnable, g_origRsAlphaBlendEnable);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_AlphaTestEnable, g_origRsAlphaTestEnable);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_AlphaRef, g_origRsAlphaRef);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_ZEnable, g_origRsZEnable);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_ZWriteEnable, g_origRsZWriteEnable);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_ZFunc, g_origRsZFunc);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_ColorWriteEnable, g_origRsColorWriteEnable);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_BlendOp, g_origRsBlendOp);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_BlendOpAlpha, g_origRsBlendOpAlpha);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_SeparateAlphaBlendEnable, g_origRsSeparateAlphaBlendEnable);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_SrcBlend, g_origRsSrcBlend);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_DestBlend, g_origRsDestBlend);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_SrcBlendAlpha, g_origRsSrcBlendAlpha);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_DestBlendAlpha, g_origRsDestBlendAlpha);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_CullMode, g_origRsCullMode);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_StencilEnable, g_origRsStencilEnable);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_TwoSidedStencilMode, g_origRsTwoSidedStencilMode);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_StencilFunc, g_origRsStencilFunc);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_StencilFail, g_origRsStencilFail);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_StencilZFail, g_origRsStencilZFail);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_StencilPass, g_origRsStencilPass);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_StencilRef, g_origRsStencilRef);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_StencilMask, g_origRsStencilMask);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_StencilWriteMask, g_origRsStencilWriteMask);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_CCWStencilFunc, g_origRsCcwStencilFunc);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_CCWStencilFail, g_origRsCcwStencilFail);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_CCWStencilZFail, g_origRsCcwStencilZFail);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_CCWStencilPass, g_origRsCcwStencilPass);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_CCWStencilRef, g_origRsCcwStencilRef);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_CCWStencilMask, g_origRsCcwStencilMask);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_CCWStencilWriteMask, g_origRsCcwStencilWriteMask);
// PGR4: ScissorTestEnable not located in this IDB yet; stubbed (hook dormant).
[[maybe_unused]] static Pgr4NoopGuestFn g_origRsScissorTestEnable;
PGR4_RS_IMPORT(D3DDevice_SetRenderState_SlopeScaleDepthBias, g_origRsSlopeScaleDepthBias);
PGR4_RS_IMPORT(D3DDevice_SetRenderState_DepthBias, g_origRsDepthBias);

#undef PGR4_RS_IMPORT

namespace {

void MirrorRenderState(uint32_t renderContext, uint32_t d3drs, uint32_t value) {
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  rr::SetRenderState(device, d3drs, value);
}

#define PGR4_RS_HOOK(hookName, origCallable, d3drs) \
  void hookName(uint32_t device, uint32_t value) { \
    origCallable(device, value);                   \
    MirrorRenderState(device, d3drs, value);       \
  }

PGR4_RS_HOOK(Pgr4RsAlphaBlendEnable, g_origRsAlphaBlendEnable, rr::D3DRS_ALPHABLENDENABLE)
PGR4_RS_HOOK(Pgr4RsAlphaTestEnable, g_origRsAlphaTestEnable, rr::D3DRS_ALPHATESTENABLE)
PGR4_RS_HOOK(Pgr4RsAlphaRef, g_origRsAlphaRef, rr::D3DRS_ALPHAREF)
PGR4_RS_HOOK(Pgr4RsZEnable, g_origRsZEnable, rr::D3DRS_ZENABLE)
PGR4_RS_HOOK(Pgr4RsZWriteEnable, g_origRsZWriteEnable, rr::D3DRS_ZWRITEENABLE)
PGR4_RS_HOOK(Pgr4RsZFunc, g_origRsZFunc, rr::D3DRS_ZFUNC)
PGR4_RS_HOOK(Pgr4RsColorWriteEnable, g_origRsColorWriteEnable, rr::D3DRS_COLORWRITEENABLE)
PGR4_RS_HOOK(Pgr4RsBlendOp, g_origRsBlendOp, rr::D3DRS_BLENDOP)
PGR4_RS_HOOK(Pgr4RsBlendOpAlpha, g_origRsBlendOpAlpha, rr::D3DRS_BLENDOPALPHA)
PGR4_RS_HOOK(Pgr4RsSeparateAlphaBlendEnable, g_origRsSeparateAlphaBlendEnable,
            rr::D3DRS_SEPARATEALPHABLENDENABLE)
PGR4_RS_HOOK(Pgr4RsSrcBlend, g_origRsSrcBlend, rr::D3DRS_SRCBLEND)
PGR4_RS_HOOK(Pgr4RsDestBlend, g_origRsDestBlend, rr::D3DRS_DESTBLEND)
PGR4_RS_HOOK(Pgr4RsSrcBlendAlpha, g_origRsSrcBlendAlpha, rr::D3DRS_SRCBLENDALPHA)
PGR4_RS_HOOK(Pgr4RsDestBlendAlpha, g_origRsDestBlendAlpha, rr::D3DRS_DESTBLENDALPHA)
PGR4_RS_HOOK(Pgr4RsCullMode, g_origRsCullMode, rr::D3DRS_CULLMODE)
PGR4_RS_HOOK(Pgr4RsStencilEnable, g_origRsStencilEnable, rr::D3DRS_STENCILENABLE)
PGR4_RS_HOOK(Pgr4RsTwoSidedStencilMode, g_origRsTwoSidedStencilMode, rr::D3DRS_TWOSIDEDSTENCILMODE)
PGR4_RS_HOOK(Pgr4RsStencilFunc, g_origRsStencilFunc, rr::D3DRS_STENCILFUNC)
PGR4_RS_HOOK(Pgr4RsStencilFail, g_origRsStencilFail, rr::D3DRS_STENCILFAIL)
PGR4_RS_HOOK(Pgr4RsStencilZFail, g_origRsStencilZFail, rr::D3DRS_STENCILZFAIL)
PGR4_RS_HOOK(Pgr4RsStencilPass, g_origRsStencilPass, rr::D3DRS_STENCILPASS)
PGR4_RS_HOOK(Pgr4RsStencilRef, g_origRsStencilRef, rr::D3DRS_STENCILREF)
PGR4_RS_HOOK(Pgr4RsStencilMask, g_origRsStencilMask, rr::D3DRS_STENCILMASK)
PGR4_RS_HOOK(Pgr4RsStencilWriteMask, g_origRsStencilWriteMask, rr::D3DRS_STENCILWRITEMASK)
PGR4_RS_HOOK(Pgr4RsCcwStencilFunc, g_origRsCcwStencilFunc, rr::D3DRS_CCWSTENCILFUNC)
PGR4_RS_HOOK(Pgr4RsCcwStencilFail, g_origRsCcwStencilFail, rr::D3DRS_CCWSTENCILFAIL)
PGR4_RS_HOOK(Pgr4RsCcwStencilZFail, g_origRsCcwStencilZFail, rr::D3DRS_CCWSTENCILZFAIL)
PGR4_RS_HOOK(Pgr4RsCcwStencilPass, g_origRsCcwStencilPass, rr::D3DRS_CCWSTENCILPASS)
PGR4_RS_HOOK(Pgr4RsCcwStencilRef, g_origRsCcwStencilRef, rr::D3DRS_CCWSTENCILREF)
PGR4_RS_HOOK(Pgr4RsCcwStencilMask, g_origRsCcwStencilMask, rr::D3DRS_CCWSTENCILMASK)
PGR4_RS_HOOK(Pgr4RsCcwStencilWriteMask, g_origRsCcwStencilWriteMask, rr::D3DRS_CCWSTENCILWRITEMASK)
PGR4_RS_HOOK(Pgr4RsScissorTestEnable, g_origRsScissorTestEnable, rr::D3DRS_SCISSORTESTENABLE)
PGR4_RS_HOOK(Pgr4RsSlopeScaleDepthBias, g_origRsSlopeScaleDepthBias, rr::D3DRS_SLOPESCALEDEPTHBIAS)
PGR4_RS_HOOK(Pgr4RsDepthBias, g_origRsDepthBias, rr::D3DRS_DEPTHBIAS)

#undef PGR4_RS_HOOK

}  // namespace

REX_HOOK(D3DDevice_SetRenderState_AlphaBlendEnable, Pgr4RsAlphaBlendEnable);
REX_HOOK(D3DDevice_SetRenderState_AlphaTestEnable, Pgr4RsAlphaTestEnable);
REX_HOOK(D3DDevice_SetRenderState_AlphaRef, Pgr4RsAlphaRef);
REX_HOOK(D3DDevice_SetRenderState_ZEnable, Pgr4RsZEnable);
REX_HOOK(D3DDevice_SetRenderState_ZWriteEnable, Pgr4RsZWriteEnable);
REX_HOOK(D3DDevice_SetRenderState_ZFunc, Pgr4RsZFunc);
REX_HOOK(D3DDevice_SetRenderState_ColorWriteEnable, Pgr4RsColorWriteEnable);
REX_HOOK(D3DDevice_SetRenderState_BlendOp, Pgr4RsBlendOp);
REX_HOOK(D3DDevice_SetRenderState_BlendOpAlpha, Pgr4RsBlendOpAlpha);
REX_HOOK(D3DDevice_SetRenderState_SeparateAlphaBlendEnable, Pgr4RsSeparateAlphaBlendEnable);
REX_HOOK(D3DDevice_SetRenderState_SrcBlend, Pgr4RsSrcBlend);
REX_HOOK(D3DDevice_SetRenderState_DestBlend, Pgr4RsDestBlend);
REX_HOOK(D3DDevice_SetRenderState_SrcBlendAlpha, Pgr4RsSrcBlendAlpha);
REX_HOOK(D3DDevice_SetRenderState_DestBlendAlpha, Pgr4RsDestBlendAlpha);
REX_HOOK(D3DDevice_SetRenderState_CullMode, Pgr4RsCullMode);
REX_HOOK(D3DDevice_SetRenderState_StencilEnable, Pgr4RsStencilEnable);
REX_HOOK(D3DDevice_SetRenderState_TwoSidedStencilMode, Pgr4RsTwoSidedStencilMode);
REX_HOOK(D3DDevice_SetRenderState_StencilFunc, Pgr4RsStencilFunc);
REX_HOOK(D3DDevice_SetRenderState_StencilFail, Pgr4RsStencilFail);
REX_HOOK(D3DDevice_SetRenderState_StencilZFail, Pgr4RsStencilZFail);
REX_HOOK(D3DDevice_SetRenderState_StencilPass, Pgr4RsStencilPass);
REX_HOOK(D3DDevice_SetRenderState_StencilRef, Pgr4RsStencilRef);
REX_HOOK(D3DDevice_SetRenderState_StencilMask, Pgr4RsStencilMask);
REX_HOOK(D3DDevice_SetRenderState_StencilWriteMask, Pgr4RsStencilWriteMask);
REX_HOOK(D3DDevice_SetRenderState_CCWStencilFunc, Pgr4RsCcwStencilFunc);
REX_HOOK(D3DDevice_SetRenderState_CCWStencilFail, Pgr4RsCcwStencilFail);
REX_HOOK(D3DDevice_SetRenderState_CCWStencilZFail, Pgr4RsCcwStencilZFail);
REX_HOOK(D3DDevice_SetRenderState_CCWStencilPass, Pgr4RsCcwStencilPass);
REX_HOOK(D3DDevice_SetRenderState_CCWStencilRef, Pgr4RsCcwStencilRef);
REX_HOOK(D3DDevice_SetRenderState_CCWStencilMask, Pgr4RsCcwStencilMask);
REX_HOOK(D3DDevice_SetRenderState_CCWStencilWriteMask, Pgr4RsCcwStencilWriteMask);
REX_HOOK(D3DDevice_SetRenderState_ScissorTestEnable, Pgr4RsScissorTestEnable);
REX_HOOK(D3DDevice_SetRenderState_SlopeScaleDepthBias, Pgr4RsSlopeScaleDepthBias);
REX_HOOK(D3DDevice_SetRenderState_DepthBias, Pgr4RsDepthBias);

// ---------------------------------------------------------------------------
// Clip planes.
// ---------------------------------------------------------------------------

REX_IMPORT(__imp__D3DDevice_SetRenderState_ClipPlaneEnable, g_origRsClipPlaneEnable,
           void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_ViewportEnable, g_origRsViewportEnable,
           void(uint32_t, uint32_t));

namespace {

void Pgr4RsClipPlaneEnable(uint32_t renderContext, uint32_t value) {
  g_origRsClipPlaneEnable(renderContext, value);
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device != nullptr)
    rr::SetClipPlaneState(device, value);
}

void Pgr4RsViewportEnable(uint32_t renderContext, uint32_t value) {
  g_origRsViewportEnable(renderContext, value);
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device != nullptr)
    rr::SetViewportEnable(device, value);
}

}  // namespace

REX_HOOK(D3DDevice_SetRenderState_ClipPlaneEnable, Pgr4RsClipPlaneEnable);
REX_HOOK(D3DDevice_SetRenderState_ViewportEnable, Pgr4RsViewportEnable);

// PGR4: PGR4_RenderContext_SetVertexDeclaration is FM2-engine or not yet located in this IDB; the guest import is
// stubbed so the ported code links. Its hook (if any) is dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origSetVertexDeclaration;

namespace {

void Pgr4SetVertexDeclaration(uint32_t renderContext, uint32_t declarationAddress) {
  g_origSetVertexDeclaration(renderContext, declarationAddress);
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  rr::SetVertexDeclaration(device, declarationAddress != 0
                                       ? ghp::ToHost<rr::GuestVertexDeclaration>(declarationAddress)
                                       : nullptr);
}

}  // namespace

REX_HOOK(PGR4_RenderContext_SetVertexDeclaration, Pgr4SetVertexDeclaration);

// PGR4 binds declarations through the XDK entry point itself
// (D3DDevice_SetVertexDeclaration @ 0x82694E38 stores device+0x2E24 and sets
// the m_Pending.m_Mask[2] shader-patch bit). Let the original keep the device
// state coherent, then mirror the binding into the native pipeline.
REX_IMPORT(__imp__D3DDevice_SetVertexDeclaration, g_origD3DSetVertexDeclaration,
           void(uint32_t, uint32_t));

namespace {

void D3DSetVertexDeclarationHook(GuestDevice* device, uint32_t declarationAddress) {
  g_origD3DSetVertexDeclaration(ghp::ToGuest(device), declarationAddress);
  rr::SetVertexDeclaration(device, declarationAddress != 0
                                       ? ghp::ToHost<rr::GuestVertexDeclaration>(declarationAddress)
                                       : nullptr);
}

}  // namespace

REX_HOOK(D3DDevice_SetVertexDeclaration, D3DSetVertexDeclarationHook);

// ---------------------------------------------------------------------------
// Vertex/index stream + surface binding.
// ---------------------------------------------------------------------------

// PGR4: PGR4_RenderContext_BindIndexBuffer is FM2-engine or not yet located in this IDB; the guest import is
// stubbed so the ported code links. Its hook (if any) is dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origBindIndexBuffer;
// PGR4: PGR4_RenderContext_SetBoundSurface is FM2-engine or not yet located in this IDB; the guest import is
// stubbed so the ported code links. Its hook (if any) is dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origSetBoundSurface;
// PGR4: sub_823716F8 is not exported by this manifest (FM2-engine, or not yet located);
// stubbed so the ported code links. Its hook stays dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origBindSurfaceInternal;

namespace {

// PGR4 builds its main backbuffer / depth surfaces itself (XGSetSurfaceHeader
// on game memory, e.g. dword_82A60EDC) and binds them through
// D3DDevice_SetRenderTarget, so a raw XDK D3DSurface header has to be
// materialized as a native surface for its current layout. Layout (IDA D3DSurface,
// D3DSurface_GetDesc @ 0x82693730): +0x18 GPU_SURFACEINFO (msaa samples
// bits 16-17), +0x24 packed size (width-1 << 18 | height-1 << 3), +0x28
// D3DFORMAT. Common bit 30 marks a texture-level surface, whose parent
// texture lives at +0x18 instead -- not handled yet.
rr::GuestBaseTexture* TranslateRawSurface(uint32_t surfaceAddr) {
  struct CachedSurface {
    std::array<uint32_t, 5> layout;
    rr::GuestBaseTexture* surface;
  };
  static std::mutex s_mutex;
  static std::unordered_map<uint32_t, CachedSurface> s_translated;
  std::lock_guard lock(s_mutex);
  const uint32_t common = ReadGuestU32At(surfaceAddr);
  if ((common & 0x40000000u) != 0) {
    REXGPU_WARN("TranslateRawSurface: 0x{:08X} is a texture-level surface -- not bound",
                surfaceAddr);
    return nullptr;
  }

  const uint32_t surfaceInfo = ReadGuestU32At(surfaceAddr + 0x18u);
  const uint32_t colorInfo = ReadGuestU32At(surfaceAddr + 0x1Cu);
  const uint32_t size = ReadGuestU32At(surfaceAddr + 0x24u);
  const uint32_t format = ReadGuestU32At(surfaceAddr + 0x28u);
  const uint32_t width = (size >> 18) + 1u;
  const uint32_t height = ((size >> 3) & 0x7FFFu) + 1u;
  const uint32_t samples = 1u << ((surfaceInfo >> 16) & 3u);
  const uint32_t multiSample = samples > 1u ? samples : 0u;
  const uint32_t edramBase = colorInfo & 0xFFFu;
  const std::array<uint32_t, 5> layout{width, height, format, multiSample, edramBase};
  const auto previous = s_translated.find(surfaceAddr);
  if (previous != s_translated.end() && previous->second.layout == layout)
    return previous->second.surface;

  // PGR4's colour and depth allocators reuse headers from the same free list.
  // Rebind changed layouts through CreateSurface's EDRAM registry; mutating
  // the old native object would also change surfaces referenced by queued draws.
  auto* surface = rr::CreateSurface(width, height, format, multiSample, edramBase);
  if (surface == nullptr)
    return nullptr;
  if (previous != s_translated.end()) {
    static uint32_t replacements = 0;
    if (++replacements <= 16) {
      REXGPU_INFO("TranslateRawSurface: recycled 0x{:08X} format=0x{:08X}->0x{:08X} "
                  "{}x{} msaa={} edram=0x{:03X}",
                  surfaceAddr, previous->second.layout[2], format, width, height,
                  multiSample, edramBase);
    }
    // Drop the raw cache's reference. The EDRAM registry retains the old object.
    D3DResourceReleaseHook(ghp::ToGuest(previous->second.surface));
  }
  s_translated.insert_or_assign(surfaceAddr, CachedSurface{layout, surface});
  return surface;
}

rr::GuestBaseTexture* TranslateSurfaceForBind(uint32_t surfaceAddr) {
  auto* gs = ghp::ToHost<GuestSurface>(surfaceAddr);
  if (gs == nullptr)
    return nullptr;
  if (!rr::IsFm2Resource(gs))
    return TranslateRawSurface(surfaceAddr);
  return gs;
}

void SetRenderTargetNative(GuestDevice* device, uint32_t index, uint32_t surfaceAddr) {
  rr::SetRenderTarget(device, index, TranslateSurfaceForBind(surfaceAddr));
}

}  // namespace

// PGR4: PGR4_RenderContext_BindVertexStream is FM2-engine or not yet located in this IDB; the guest import is
// stubbed so the ported code links. Its hook (if any) is dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origFm2BindVertexStream;

// Carries a uint64_t dirty_mask split across r8:r9 in the 32-bit PPC ABI;
// the standard auto-marshaling would only read the low half, so this hook
// stays RAW to forward the full context, matching the reference's own
// documented reasoning for the same guest function.
REX_HOOK_RAW(PGR4_RenderContext_BindVertexStream) {
  const uint32_t renderContext = ctx.r3.u32;
  const uint32_t slot = ctx.r4.u32;
  const uint32_t resourceAddr = ctx.r5.u32;
  const uint32_t byteOffset = ctx.r6.u32;
  const uint32_t strideBytes = ctx.r7.u32;
  const uint64_t dirtyMask = (uint64_t(ctx.r8.u32) << 32) | ctx.r9.u32;
  g_origFm2BindVertexStream(ctx, base, renderContext, slot, resourceAddr, byteOffset, strideBytes,
                            dirtyMask);
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  auto* buffer = ghp::ToHost<GuestBuffer>(resourceAddr);
  rr::SetStreamSource(device, slot, buffer, byteOffset, strideBytes);
}

void Pgr4BindIndexBuffer(uint32_t renderContext, uint32_t resourceAddr) {
  g_origBindIndexBuffer(renderContext, resourceAddr);
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  rr::SetIndices(device, ghp::ToHost<GuestBuffer>(resourceAddr));
}

// PGR4_RenderContext_SetBoundSurface is the guest equivalent of BOTH
// SetRenderTarget and SetDepthStencilSurface: classify by resource type and
// route depth surfaces to the depth slot instead of always binding color
// index 0 (otherwise depth is never bound and every depth-tested draw
// sanitizes to an unknown depth-stencil format).
void Pgr4SetBoundSurface(uint32_t renderContext, uint32_t surfaceAddr, uint32_t surfaceArg) {
  g_origSetBoundSurface(renderContext, surfaceAddr, surfaceArg);
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr || surfaceAddr == 0)
    return;
  auto* gs = ghp::ToHost<GuestSurface>(surfaceAddr);
  if (gs != nullptr && gs->type == rr::ResourceType::DepthStencil) {
    rr::SetDepthStencilSurface(device, gs);
  } else {
    SetRenderTargetNative(device, 0, surfaceAddr);
  }
}

// Forza binds its COLOR render target(s) via this internal path (slot 0 =
// primary color RT), not the standard D3DDevice_SetRenderTarget. Without
// this, the native render target is never bound and the screen stays at
// its clear color.
void Pgr4BindSurface(uint32_t renderContext, uint32_t slot, uint32_t surfaceAddr) {
  g_origBindSurfaceInternal(renderContext, slot, surfaceAddr);
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr || surfaceAddr == 0)
    return;
  SetRenderTargetNative(device, slot, surfaceAddr);
}

REX_HOOK(PGR4_RenderContext_BindIndexBuffer, Pgr4BindIndexBuffer);
REX_HOOK(PGR4_RenderContext_SetBoundSurface, Pgr4SetBoundSurface);
REX_HOOK(sub_823716F8, Pgr4BindSurface);

// PGR4 has no engine-level render context: surfaces are bound through the XDK
// entry points (D3DDevice_SetRenderTarget @ 0x82690FA0 (device, index,
// surface), D3DDevice_SetDepthStencilSurface @ 0x82691308 (device, surface)).
// Run the originals so the guest's surface-info / pending state stays
// coherent, then mirror the binding natively.
REX_IMPORT(__imp__D3DDevice_SetRenderTarget, g_origD3DSetRenderTarget,
           void(uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetDepthStencilSurface, g_origD3DSetDepthStencilSurface,
           void(uint32_t, uint32_t));

namespace {

void D3DSetRenderTargetHook(GuestDevice* device, uint32_t index, uint32_t surfaceAddr) {
  g_origD3DSetRenderTarget(ghp::ToGuest(device), index, surfaceAddr);
  static std::atomic<uint32_t> s_logged{0};
  if (s_logged.fetch_add(1, std::memory_order_relaxed) < 16 || InTraceWindow()) {
    auto* fm2 = surfaceAddr != 0 && rr::IsFm2Resource(ghp::ToHost<void>(surfaceAddr))
                    ? ghp::ToHost<rr::GuestSurface>(surfaceAddr)
                    : nullptr;
    REXGPU_INFO("D3DDevice_SetRenderTarget: index={} surface=0x{:08X} pgr4={} {}x{} fmt=0x{:08X} host={}",
                index, surfaceAddr, fm2 != nullptr, fm2 ? fm2->width : 0u, fm2 ? fm2->height : 0u,
                fm2 ? fm2->guestFormat : 0u, static_cast<const void*>(fm2));
  }
  rr::SetRenderTarget(device, index,
                      surfaceAddr != 0 ? TranslateSurfaceForBind(surfaceAddr) : nullptr);
}

void D3DSetDepthStencilSurfaceHook(GuestDevice* device, uint32_t surfaceAddr) {
  g_origD3DSetDepthStencilSurface(ghp::ToGuest(device), surfaceAddr);
  static std::atomic<uint32_t> s_logged{0};
  if (s_logged.fetch_add(1, std::memory_order_relaxed) < 16 || InTraceWindow()) {
    REXGPU_INFO("D3DDevice_SetDepthStencilSurface: surface=0x{:08X} pgr4={}", surfaceAddr,
                surfaceAddr != 0 && rr::IsFm2Resource(ghp::ToHost<void>(surfaceAddr)));
  }
  rr::SetDepthStencilSurface(
      device, surfaceAddr != 0 ? static_cast<GuestSurface*>(TranslateSurfaceForBind(surfaceAddr))
                               : nullptr);
}

}  // namespace

REX_HOOK(D3DDevice_SetRenderTarget, D3DSetRenderTargetHook);
REX_HOOK(D3DDevice_SetDepthStencilSurface, D3DSetDepthStencilSurfaceHook);

// ---------------------------------------------------------------------------
// Shader state.
// ---------------------------------------------------------------------------

namespace {

// Every shader we create is pure-replace (own guest address is the handle),
// so resolving a guest shader reference is either "it already is one of
// ours" or "look it up by alias" (registered when the container that owns
// it gets loaded -- Phase 4 wires that registration to the actual draw-time
// shader-load path).
rr::GuestShader* ResolveShader(uint32_t shaderAddr) {
  auto* shader = ghp::ToHost<rr::GuestShader>(shaderAddr);
  if (rr::IsFm2Resource(shader))
    return shader;
  return rr::LookupShaderAlias(shaderAddr);
}

// Pure replacement, no passthrough to the original guest function: the
// original body reads its shader argument as a real 24-byte D3DPixelShader
// (Common/ReferenceCount/Fence/ReadFence/Identifier/BaseFlush), using
// ReadFence (offset 0xC) as a relative offset to a "compiled state table"
// appended after the struct. Our GuestShader is a real C++ object (mutex,
// unique_ptr, unordered_map, ...), not that 24-byte layout, so calling
// through reads garbage from inside our std::mutex as if it were state-table
// metadata. That table's merge loop decrements a uint16_t count by 2 until
// it hits zero -- mathematically impossible to terminate if the (garbage)
// count is odd, which intermittently hung the process for minutes.
void Pgr4SetPixelShaderState(uint32_t renderContext, uint32_t shaderAddr) {
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr || shaderAddr == 0)
    return;
  rr::SetPixelShader(device, ResolveShader(shaderAddr));
}

void Pgr4SetVertexShaderState(uint32_t renderContext, uint32_t shaderAddr) {
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr || shaderAddr == 0)
    return;
  rr::SetVertexShader(device, ResolveShader(shaderAddr));
}

}  // namespace

REX_HOOK(PGR4_RenderContext_SetPixelShaderState, Pgr4SetPixelShaderState);
REX_HOOK(PGR4_RenderContext_SetVertexShaderState, Pgr4SetVertexShaderState);

// PGR4 binds shaders through the XDK entry points directly. Same pure
// replacement as above (the originals parse the handle as a real
// D3DVertexShader/D3DPixelShader and would read our GuestShader's C++
// members as a state table); only the device bookkeeping the originals
// perform is mirrored: the handle slot and the m_Pending.m_Mask[2]
// shader-patch bit (0x80000).
namespace {

void MarkShaderPending(GuestDevice* device) {
  device->dirtyFlags[2] = device->dirtyFlags[2].get() | 0x80000u;
}

void D3DSetVertexShaderHook(GuestDevice* device, uint32_t shaderAddr) {
  device->vertexShader = shaderAddr;
  MarkShaderPending(device);
  rr::SetVertexShader(device, shaderAddr != 0 ? ResolveShader(shaderAddr) : nullptr);
}

void D3DSetPixelShaderHook(GuestDevice* device, uint32_t shaderAddr) {
  device->pixelShader = shaderAddr;
  MarkShaderPending(device);
  rr::SetPixelShader(device, shaderAddr != 0 ? ResolveShader(shaderAddr) : nullptr);
}

}  // namespace

REX_HOOK(D3DDevice_SetVertexShader, D3DSetVertexShaderHook);
REX_HOOK(D3DDevice_SetPixelShader, D3DSetPixelShaderHook);

// PGR4 registers its precompiled shader packs straight through
// XGRegisterVertexShader / XGRegisterPixelShader (D3DDevice_Create*Shader is
// never reached), so ResolveShader would find nothing for those handles. Let
// the original build the raw XDK object, then reassemble the ShaderContainer
// the parser expects ([cached part][physical part], contiguous -- exactly what
// D3DDevice_CreateVertexShader receives) and alias the raw handle to it.
//   VS object: 0x368-byte runtime struct, cached part at +872, ucode ptr at +32
//   PS object: 0x28-byte runtime struct,  cached part at +40,  ucode ptr at +24
// (XGGetMicrocodeShaderParts @ 0x82694898, XGRegister* @ 0x82694B28/0x826948E8)
REX_IMPORT(__imp__XGRegisterVertexShader, g_origXGRegisterVertexShader, void(uint32_t, uint32_t));
REX_IMPORT(__imp__XGRegisterPixelShader, g_origXGRegisterPixelShader, void(uint32_t, uint32_t));

namespace {

void RegisterRawShader(uint32_t shaderAddr, uint32_t physicalPart, uint32_t cachedOffset,
                       bool pixel) {
  const auto* cached = ghp::ToHost<const rex::be<uint32_t>>(shaderAddr + cachedOffset);
  const auto* physical = ghp::ToHost<const uint8_t>(physicalPart);
  if (cached == nullptr || physical == nullptr)
    return;
  const uint32_t cachedSize = cached[1].get();
  const uint32_t physicalSize = cached[2].get();
  if (cachedSize < 12u || cachedSize > 0x10000u || physicalSize == 0u || physicalSize > 0x40000u) {
    REXGPU_WARN("XGRegister{}Shader: 0x{:08X} implausible container sizes {}/{}",
                pixel ? "Pixel" : "Vertex", shaderAddr, cachedSize, physicalSize);
    return;
  }
  // Guest-resident so the ucode index can key it by guest address.
  auto* container = ghp::ToHost<uint8_t>(ghp::GuestAllocRaw(cachedSize + physicalSize, 0x10));
  if (container == nullptr)
    return;
  std::memcpy(container, cached, cachedSize);
  std::memcpy(container + cachedSize, physical, physicalSize);
  const auto* function = reinterpret_cast<const uint32_t*>(container);
  rr::RegisterShaderAlias(shaderAddr,
                          pixel ? rr::CreatePixelShader(function) : rr::CreateVertexShader(function));
}

void XGRegisterVertexShaderHook(uint32_t shaderAddr, uint32_t physicalPart) {
  g_origXGRegisterVertexShader(shaderAddr, physicalPart);
  RegisterRawShader(shaderAddr, physicalPart, 872u, false);
}

void XGRegisterPixelShaderHook(uint32_t shaderAddr, uint32_t physicalPart) {
  g_origXGRegisterPixelShader(shaderAddr, physicalPart);
  RegisterRawShader(shaderAddr, physicalPart, 40u, true);
}

}  // namespace

REX_HOOK(XGRegisterVertexShader, XGRegisterVertexShaderHook);
REX_HOOK(XGRegisterPixelShader, XGRegisterPixelShaderHook);

// Stream / index binding also goes through the XDK entry points in PGR4
// (D3DDevice_SetStreamSource @ 0x82690618 (device, stream, buffer, offset,
// stride, pendingMask), D3DDevice_SetIndices @ 0x826907C0 (device, buffer)).
// The originals keep the device slots QueueDrawStateSnapshots reads; the
// native binding adds the byte offset the slots do not carry.
REX_IMPORT(__imp__D3DDevice_SetStreamSource, g_origD3DSetStreamSource,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetIndices, g_origD3DSetIndices, void(uint32_t, uint32_t));

namespace {

void D3DSetStreamSourceHook(GuestDevice* device, uint32_t stream, uint32_t bufferAddr,
                            uint32_t offset, uint32_t stride, uint32_t pendingMask) {
  g_origD3DSetStreamSource(ghp::ToGuest(device), stream, bufferAddr, offset, stride, pendingMask);
  rr::SetStreamSource(device, stream, ghp::ToHost<GuestBuffer>(bufferAddr), offset, stride);
}

void D3DSetIndicesHook(GuestDevice* device, uint32_t bufferAddr) {
  g_origD3DSetIndices(ghp::ToGuest(device), bufferAddr);
  rr::SetIndices(device, ghp::ToHost<GuestBuffer>(bufferAddr));
}

}  // namespace

REX_HOOK(D3DDevice_SetStreamSource, D3DSetStreamSourceHook);
REX_HOOK(D3DDevice_SetIndices, D3DSetIndicesHook);

// ---------------------------------------------------------------------------
// Viewport / scissor. Full replacements: the originals pack hardware-specific
// clip/viewport state and call D3D::SetSurfaceClip, a PM4-emitting function
// for a real Xenos GPU that does not exist under this renderer.
// The state-200 scissor-enable setter is mirrored above; this section owns the
// rectangle and viewport payloads themselves.
// ---------------------------------------------------------------------------

namespace {

// D3DDevice_SetScissorRect(D3DDevice* device, const RECT* rect) -- RECT here
// is guest memory shaped exactly like GuestRect (left/top/right/bottom).
void SetScissorRectHook(GuestDevice* device, rr::GuestRect* rect) {
  rr::SetScissorRect(device, rect);
}

// D3D::SetViewport(D3DDevice* device, float X, float Y, float Width, float
// Height, float MinZ, float MaxZ, uint32_t flags) -- confirmed via IDA
// decompile; the 6 float args match GuestViewport's field order exactly. The
// trailing flags arg is accepted for correct PPC arg marshaling but unused --
// the original body only uses it for a dirty-flag bit this renderer already
// tracks itself via SetDirtyValue.
void SetViewportHook(GuestDevice* device, float x, float y, float width, float height, float minZ,
                     float maxZ, uint32_t /*flags*/) {
  rr::GuestViewport viewport;
  viewport.x = uint32_t(x);
  viewport.y = uint32_t(y);
  viewport.width = uint32_t(width);
  viewport.height = uint32_t(height);
  viewport.minZ = minZ;
  viewport.maxZ = maxZ;
  rr::SetViewport(device, &viewport);
}

}  // namespace

REX_HOOK(D3DDevice_SetScissorRect, SetScissorRectHook);
REX_HOOK(D3D_SetViewport, SetViewportHook);

// ---------------------------------------------------------------------------
// D3DDevice_SetTexture. Full replacement (Unleashed pattern): IDA confirms
// FM2 binds samplers through this entry point heavily (material setup,
// object-pass draws, PM4 draw dispatch -- 65 code xrefs at 0x8236C208). The
// original body only updates Xenos fetch/pending state for a real GPU ring;
// under the native renderer we must populate g_sharedConstants.texture*Indices
// ourselves or every shader samples the null descriptor.
//
// PendingMask3 (4th guest arg) is XDK dirty-bit bookkeeping for that ring --
// unused here. Pure-replace GuestTexture / GuestSurface objects are bound
// directly; raw XG-header textures (no kPgr4ResourceMagic, created via
// XGSetTextureHeader instead of D3DDevice_CreateTexture) go through
// TranslateGuestTexture to parse their Xenos fetch constant and materialize
// a native texture (Lock/GetDesc on these still bind null -- that gap is
// unrelated to sampling and not covered by this port).
// ---------------------------------------------------------------------------

namespace {

struct ResolvedTextureBinding {
  rr::GuestBaseTexture* texture = nullptr;
  bool baseTexture = false;
};

ResolvedTextureBinding ResolveTextureBinding(rr::GuestBaseTexture* texture) {
  if (texture == nullptr)
    return {};
  if (!rr::IsFm2Resource(texture))
    return {rr::TranslateGuestTexture(texture, true), false};
  if (texture->type == rr::ResourceType::Texture ||
      texture->type == rr::ResourceType::VolumeTexture) {
    return {texture, false};
  }
  if (texture->type == rr::ResourceType::RenderTarget ||
      texture->type == rr::ResourceType::DepthStencil) {
    return {texture, true};
  }
  return {};
}

void SetTextureHook(GuestDevice* device, uint32_t sampler, rr::GuestBaseTexture* texture) {
  // Samplers 16..19 are D3DVERTEXTEXTURESAMPLER0..3 (bone palettes, instance
  // data); they bind into the vertex texture slots of SharedConstants.
  if (sampler >= 20u)
    return;

  const uint32_t guestAddress = ghp::ToGuest(texture);
  const bool rawTexture = texture != nullptr && !rr::IsFm2Resource(texture);
  const ResolvedTextureBinding binding = ResolveTextureBinding(texture);
  if (rawTexture && binding.texture == nullptr) {
    static std::unordered_set<const void*> s_warned;
    if (s_warned.insert(texture).second) {
      REXGPU_WARN(
          "D3DDevice_SetTexture: slot {} texture {} untranslatable "
          "(XG header?) -- bound null",
          sampler, static_cast<const void*>(texture));
    }
  }

  if (binding.baseTexture) {
    rr::SetTextureBase(device, sampler, binding.texture, guestAddress);
  } else {
    rr::SetTexture(device, sampler, static_cast<rr::GuestTexture*>(binding.texture), guestAddress);
  }
}

}  // namespace

REX_HOOK(D3DDevice_SetTexture, SetTextureHook);

// ---------------------------------------------------------------------------
// D3DDevice_Resolve. Full replacement: the original body is a raw
// PM4-packet EDRAM-resolve builder for a real Xenos GPU ring buffer that
// does not exist here. Per IDA (rename comment on the function itself), FM2
// also reuses this same guest function for GPU-side audio-mixing
// bookkeeping unrelated to any real texture -- so beyond the ring buffer
// issue, letting the original run against one of our pure-replace
// GuestBaseTexture objects would corrupt it via real-D3DBaseTexture-shaped
// pointer arithmetic (same bug class fixed in SetPixelShaderState above).
// ---------------------------------------------------------------------------

namespace {

// Flag bits confirmed directly from the decompiled body (not guessed):
// 0x100 triggers the original's color-clear path, 0x200 its depth/stencil
// clear path; both are independent of the multisample-derived bits (0x70)
// the original also ORs into its working copy of Flags.
constexpr uint32_t kResolveClearColor = 0x100;
constexpr uint32_t kResolveClearDepthStencil = 0x200;
thread_local uint32_t g_resolveCaller = 0;
thread_local uint32_t g_resolveParentCaller = 0;
thread_local uint32_t g_resolveOwner = 0;

// D3DDevice_Resolve(pDevice, Flags, pSourceRect, pDestTexture, pDestPoint,
// DestLevel, DestSliceOrFace, pClearColor, ClearZ, ClearStencil,
// pParameters). DestLevel/DestSliceOrFace/pParameters don't apply to our
// single-level, single-slice-per-object resource model; accepted only so
// PPC arg marshaling stays aligned for the params after them.
void ResolveHook(GuestDevice* /*device*/, uint32_t flags, rr::GuestRect* sourceRect,
                 uint32_t destTextureAddr, rr::GuestPoint* destPoint, uint32_t /*destLevel*/,
                 uint32_t /*destSliceOrFace*/, const uint32_t* clearColor, float clearZ,
                 uint32_t /*clearStencil*/, const void* /*parameters*/) {
  if (InTraceWindow()) {
    REXGPU_INFO("TRACE resolve flags=0x{:X} dest=0x{:08X} base=0x{:08X} lr=0x{:08X}", flags,
                destTextureAddr,
                destTextureAddr != 0 ? ReadGuestU32At(destTextureAddr + 32u) : 0u, g_resolveCaller);
  }
  uint32_t postClearFlags = 0;
  float postClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  if ((flags & kResolveClearColor) != 0) {
    postClearFlags |= rr::D3DCLEAR_TARGET;
    if (clearColor != nullptr) {
      for (int i = 0; i < 4; ++i)
        postClearColor[i] = std::bit_cast<float>(std::byteswap(clearColor[i]));
    }
  }
  if ((flags & kResolveClearDepthStencil) != 0) {
    postClearFlags |= rr::D3DCLEAR_ZBUFFER | rr::D3DCLEAR_STENCIL;
  }

  // Resolve-with-clear copies EDRAM first, then clears it for the next pass.
  // Invalid/overloaded destinations still retain the clear half of that API.
  const auto queueClearOnly = [&] {
    if (postClearFlags != 0)
      rr::Clear(nullptr, postClearFlags, postClearColor, clearZ);
  };

  if (destTextureAddr == 0) {
    static std::atomic<uint32_t> s_nullDest{0};
    if (s_nullDest.fetch_add(1, std::memory_order_relaxed) < 8) {
      REXGPU_INFO("Resolve: null destination (flags=0x{:X} lr=0x{:08X}) -- clear only", flags,
                  g_resolveCaller);
    }
    queueClearOnly();
    return;
  }

  void* destHost = ghp::ToHost<void>(destTextureAddr);
  rr::GuestBaseTexture* reo = nullptr;
  uint32_t dataBase = 0;

  if (rr::IsFm2Resource(destHost)) {
    reo = static_cast<rr::GuestBaseTexture*>(destHost);
  } else {
    // Raw XDK D3DBaseTexture: translate to a host texture and key aperture by
    // the header's resolve copy-dest base (dword at +32) so Swap's frontbuffer
    // fetch can find the composited frame.
    reo = rr::TranslateGuestTexture(destHost, /*uploadGuestData=*/false);
    dataBase = ghp::HeaderBaseToPhysical(ReadGuestU32At(destTextureAddr + 32u)) & ~0xFFFu;
  }

  if (reo == nullptr || reo->texture == nullptr) {
    queueClearOnly();
    return;
  }

  reo->gpuResolved = true;
  const bool destIsBlockCompressed = reo->format >= plume::RenderFormat::BC1_TYPELESS &&
                                     reo->format <= plume::RenderFormat::BC7_UNORM_SRGB;
  if (destIsBlockCompressed) {
    static uint64_t bcSkip = 0;
    if (++bcSkip <= 24) {
      REXGPU_WARN("Resolve: skipping BC dest 0x{:08X} fmt={} (misidentified)", destTextureAddr,
                  int(reo->format));
    }
    queueClearOnly();
    return;
  }

  if (dataBase != 0) {
    rr::RegisterResolveSurfaceAperture(dataBase, reo);
  }
  // Do not SetFrontbufferPresentSource here — Swap owns that from the
  // aperture lookup so intermediate FM2 resolves cannot steal the composite.

  static uint64_t resolveApertureCount = 0;
  ++resolveApertureCount;
  // Band resolves (explicit srcRect/destPt) are the predicated-tiling composite:
  // log the first 60 unconditionally — consecutive destPt/srcRect values are the
  // evidence for the half-screen composite bug.
  static uint64_t bandResolveCount = 0;
  const bool isBandResolve = sourceRect != nullptr || destPoint != nullptr;
  if (isBandResolve)
    ++bandResolveCount;
  if ((isBandResolve && bandResolveCount <= 60) || resolveApertureCount <= 150 ||
      resolveApertureCount % 300 == 1) {
    REXGPU_INFO(
        "Resolve: n={} lr=0x{:08X} parentLr=0x{:08X} owner=0x{:08X} flags=0x{:X} "
        "dest=0x{:08X} dataBase=0x{:08X} {}x{} fmt={} pgr4={} destPt=({},{}) "
        "srcRect=({},{},{},{})",
        resolveApertureCount, g_resolveCaller, g_resolveParentCaller, g_resolveOwner, flags,
        destTextureAddr, dataBase, reo->width, reo->height, int(reo->format),
        rr::IsFm2Resource(destHost), destPoint != nullptr ? destPoint->x.get() : -1,
        destPoint != nullptr ? destPoint->y.get() : -1,
        sourceRect != nullptr ? sourceRect->left.get() : -1,
        sourceRect != nullptr ? sourceRect->top.get() : -1,
        sourceRect != nullptr ? sourceRect->right.get() : -1,
        sourceRect != nullptr ? sourceRect->bottom.get() : -1);
  }

  rr::ResolveToTexture(reo, destPoint, sourceRect, postClearFlags, postClearColor, clearZ);
}

}  // namespace

REX_HOOK_RAW(D3DDevice_Resolve) {
  g_resolveCaller = ctx.lr;
  if (ctx.lr >= 0x825ADB18u && ctx.lr < 0x825ADE10u) {
    // sub_825ADB18 saves its caller LR at old r1-8, then allocates 240 bytes.
    g_resolveParentCaller = ReadGuestU32At(ctx.r1.u32 + 232u);
    g_resolveOwner = ctx.r31.u32;
  } else {
    g_resolveParentCaller = 0;
    g_resolveOwner = 0;
  }
  rex::ppc::HostToGuestFunction<ResolveHook>(ctx, base);
}

// PGR4 renders every frame through XDK predicated tiling (2+ tiles at 720p,
// tile count in dword_82A60FB0). The per-tile clear and the final EDRAM ->
// frontbuffer copy live inside D3DDevice_BeginTiling / D3DDevice_EndTiling
// (@ 0x8269C6D0 / 0x8269CB68), not in ClearF / Resolve, so both are pure
// replacements built from the same native primitives: one clear at
// BeginTiling, one resolve-with-clear at EndTiling. The originals only build
// PM4 for a ring that is not executed here.
namespace {

// D3DDevice_BeginTiling(pDevice, Flags, Count, pTilingRects, pClearColor,
// ClearZ (f1), ClearStencil)
void BeginTilingHook(GuestDevice* device, uint32_t /*flags*/, uint32_t /*count*/,
                     uint32_t /*rects*/, const uint32_t* clearColor, uint32_t /*clearStencil*/,
                     float clearZ) {
  float rgba[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  uint32_t flags = rr::D3DCLEAR_ZBUFFER | rr::D3DCLEAR_STENCIL;
  if (clearColor != nullptr) {
    flags |= rr::D3DCLEAR_TARGET;
    for (int i = 0; i < 4; ++i)
      rgba[i] = std::bit_cast<float>(std::byteswap(clearColor[i]));
  }
  rr::Clear(device, flags, rgba, clearZ);
}

// D3DDevice_EndTiling(pDevice, ResolveFlags, pResolveRects, pDestTexture,
// pClearColor, ClearZ (f1), ClearStencil, pParameters) -- same flag semantics
// as D3DDevice_Resolve (the game passes the identical flags to both paths).
void EndTilingHook(GuestDevice* device, uint32_t flags, uint32_t /*rects*/,
                   uint32_t destTextureAddr, const uint32_t* clearColor, uint32_t clearStencil,
                   uint32_t /*params*/, float clearZ) {
  static std::atomic<uint32_t> s_logged{0};
  if (s_logged.fetch_add(1, std::memory_order_relaxed) < 12 || InTraceWindow()) {
    REXGPU_INFO("D3DDevice_EndTiling: flags=0x{:X} dest=0x{:08X} clearColor={}", flags,
                destTextureAddr, clearColor != nullptr);
  }
  ResolveHook(device, flags, nullptr, destTextureAddr, nullptr, 0, 0, clearColor, clearZ,
              clearStencil, nullptr);
}

}  // namespace

REX_HOOK(D3DDevice_BeginTiling, BeginTilingHook);
REX_HOOK(D3DDevice_EndTiling, EndTilingHook);

// ---------------------------------------------------------------------------
// Phase 4: draw dispatch. Direct draws are full replacements because the
// original bodies only emit PM4 for a Xenos GPU that does not exist here.
// While FM2 compiles a reusable guest command buffer, the originals also run
// so the guest clone remains structurally valid; the native POD stream is
// recorded rather than submitted until that clone is played.
// ---------------------------------------------------------------------------

// Returns a guest-physical payload pointer that the caller fills after the
// function returns. The completed values therefore never enter GuestDevice's
// CPU constant files; record the payload here and consume it at the next draw.
// PGR4: sub_82803358 is not exported by this manifest (FM2-engine, or not yet located);
// stubbed so the ported code links. Its hook stays dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origGpuBeginShaderConstantF4;
// PGR4: sub_823767B8 is not exported by this manifest (FM2-engine, or not yet located);
// stubbed so the ported code links. Its hook stays dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origCbSetShaderConstantF;
// PGR4: sub_823766E0 is not exported by this manifest (FM2-engine, or not yet located);
// stubbed so the ported code links. Its hook stays dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origCbCreateShaderConstantFFixup;
REX_IMPORT(__imp__D3DDevice_DrawVertices, g_origDrawVertices,
           void(GuestDevice*, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_DrawIndexedVertices, g_origDrawIndexedVertices,
           void(GuestDevice*, uint32_t, int32_t, uint32_t, uint32_t));
// PGR4: D3DDevice_DrawIndexedVertices_WithVertexFormatSetup is not exported by this manifest (FM2-engine, or not yet located);
// stubbed so the ported code links. Its hook stays dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origDrawIndexedVerticesWithVertexFormat;
REX_IMPORT(__imp__D3DDevice_DrawVerticesUP, g_origDrawVerticesUP,
           void(GuestDevice*, uint32_t, uint32_t, const void*, uint32_t));
// PGR4: PGR4_D3D_BeginCommandBufferBatch is FM2-engine or not yet located in this IDB; the guest import is
// stubbed so the ported code links. Its hook (if any) is dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origBeginCommandBufferBatch;
// PGR4: PGR4_D3D_FinalizeCommandBufferBatch is FM2-engine or not yet located in this IDB; the guest import is
// stubbed so the ported code links. Its hook (if any) is dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origFinalizeCommandBufferBatch;
// PGR4: PGR4_D3D_CreateCommandBufferClone is FM2-engine or not yet located in this IDB; the guest import is
// stubbed so the ported code links. Its hook (if any) is dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origCreateCommandBufferClone;
// PGR4: PGR4_D3D_EmitDirtyStateAndDrawList is FM2-engine or not yet located in this IDB; the guest import is
// stubbed so the ported code links. Its hook (if any) is dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origEmitDirtyStateAndDrawList;
// PGR4: D3DCommandBuffer_CreateTextureFixup is not exported by this manifest (FM2-engine, or not yet located);
// stubbed so the ported code links. Its hook stays dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origCreateTextureFixup;
// PGR4: D3DCommandBuffer_SetTexture is not exported by this manifest (FM2-engine, or not yet located);
// stubbed so the ported code links. Its hook stays dormant until mapped.
[[maybe_unused]] static Pgr4NoopGuestFn g_origSetCommandBufferTexture;

namespace {

struct DeferredShaderConstants {
  uint32_t payloadGuest;
  uint32_t startRegister;
  uint32_t registerCount;
  bool pixelShader;
};

std::mutex g_deferredShaderConstantsMutex;
std::vector<DeferredShaderConstants> g_deferredShaderConstants;

struct CommandBufferFixupRange {
  uint16_t startRegister;
  uint16_t registerCount;
};

struct DeferredCommandBufferConstants {
  uint32_t startRegister;
  uint32_t registerCount;
  std::array<uint32_t, 64 * 4> values;
};

std::mutex g_commandBufferFixupMutex;
std::unordered_map<uint32_t, CommandBufferFixupRange> g_commandBufferFixupRanges;
std::vector<DeferredCommandBufferConstants> g_deferredCommandBufferConstants;

void DrainDeferredShaderConstants() {
  std::vector<DeferredShaderConstants> pending;
  {
    std::lock_guard lock(g_deferredShaderConstantsMutex);
    if (g_deferredShaderConstants.empty())
      return;
    pending.swap(g_deferredShaderConstants);
  }

  auto* memory = pgr4::ghp::GuestMemory();
  if (memory == nullptr)
    return;

  for (const DeferredShaderConstants& constants : pending) {
    const uint32_t physicalAddress = constants.payloadGuest & 0x1FFFFFFFu;
    const uint64_t byteCount = uint64_t(constants.registerCount) * 16u;
    if (byteCount == 0 || byteCount > 0x20000000u ||
        uint64_t(physicalAddress) + byteCount > 0x20000000u) {
      continue;
    }
    const uint32_t lastAddress = physicalAddress + uint32_t(byteCount) - 1u;
    if (memory->GetPhysicalHeap()->QueryRangeAccess(physicalAddress, lastAddress) ==
        rex::memory::PageAccess::kNoAccess) {
      continue;
    }
    const auto* source = memory->TranslatePhysical<const uint32_t*>(physicalAddress);
    if (source == nullptr)
      continue;
    pgr4::render::StageDrawShaderConstants(!constants.pixelShader, constants.startRegister, source,
                                          constants.registerCount);
  }
}

void DrainDeferredCommandBufferConstants() {
  std::vector<DeferredCommandBufferConstants> pending;
  {
    std::lock_guard lock(g_commandBufferFixupMutex);
    if (g_deferredCommandBufferConstants.empty())
      return;
    pending.swap(g_deferredCommandBufferConstants);
  }

  for (const DeferredCommandBufferConstants& constants : pending) {
    pgr4::render::StageDrawShaderConstants(true, constants.startRegister, constants.values.data(),
                                          constants.registerCount);
  }
}

void DrainDeferredDrawShaderConstants() {
  DrainDeferredShaderConstants();
  DrainDeferredCommandBufferConstants();
}

void DrawVerticesHook(GuestDevice* device, uint32_t primitiveType, uint32_t startVertex,
                      uint32_t vertexCount) {
  DrainDeferredDrawShaderConstants();
  if (pgr4::render::RenderQueue::IsRecording())
    g_origDrawVertices(device, primitiveType, startVertex, vertexCount);
  rr::DrawVertices(device, primitiveType, startVertex, vertexCount);
}

void DrawIndexedVerticesHook(GuestDevice* device, uint32_t primitiveType, int32_t baseVertexIndex,
                             uint32_t startIndex, uint32_t indexCount) {
  DrainDeferredDrawShaderConstants();
  if (pgr4::render::RenderQueue::IsRecording())
    g_origDrawIndexedVertices(device, primitiveType, baseVertexIndex, startIndex, indexCount);
  rr::DrawIndexedVertices(device, primitiveType, baseVertexIndex, startIndex, indexCount);
}

void DrawIndexedVerticesWithVertexFormatHook(GuestDevice* device, uint32_t primitiveType,
                                             int32_t baseVertexIndex, uint32_t startIndex,
                                             uint32_t indexCount) {
  DrainDeferredDrawShaderConstants();
  if (pgr4::render::RenderQueue::IsRecording())
    g_origDrawIndexedVerticesWithVertexFormat(device, primitiveType, baseVertexIndex, startIndex,
                                              indexCount);
  rr::DrawIndexedVertices(device, primitiveType, baseVertexIndex, startIndex, indexCount);
}

void DrawVerticesUPHook(GuestDevice* device, uint32_t primitiveType, uint32_t vertexCount,
                        const void* vertexStreamZeroData, uint32_t vertexStreamZeroStride) {
  DrainDeferredDrawShaderConstants();
  if (pgr4::render::RenderQueue::IsRecording()) {
    g_origDrawVerticesUP(device, primitiveType, vertexCount, vertexStreamZeroData,
                         vertexStreamZeroStride);
  }
  rr::DrawUserPointerVertices(device, primitiveType, vertexCount, vertexStreamZeroData,
                              vertexStreamZeroStride);
}

// PGR4's own immediate-mode layer (sub_8229CF38 / sub_8229CFE0 / sub_8229D040,
// and the screen manager via sub_82837F70) bypasses DrawVerticesUP: it calls
// D3DDevice_BeginVertices, which emits the draw packet into the ring and hands
// back the write-combined slot for the vertices, fills that slot, then
// D3DDevice_EndVertices (sub_8269A998) merely commits the ring write pointer.
// Neither reaches the renderer, so the draw is emulated here: Begin hands out
// a guest scratch buffer and End issues it as a user-pointer draw (which
// copies the vertices on the guest thread, so one buffer suffices).
uint32_t g_immVertices = 0;  // guest scratch buffer for the vertex data
uint32_t g_immCapacity = 0;
uint32_t g_immPrimitiveType = 0;
uint32_t g_immVertexCount = 0;
uint32_t g_immStride = 0;

uint32_t BeginVerticesHook(GuestDevice* /*device*/, uint32_t primitiveType, uint32_t vertexCount,
                           uint32_t vertexStreamZeroStride) {
  const uint32_t bytes = vertexCount * vertexStreamZeroStride;
  if (bytes > g_immCapacity) {
    if (g_immVertices != 0) ghp::GuestFreeRaw(g_immVertices);
    g_immCapacity = std::max(bytes, 256u * 1024u);
    g_immVertices = ghp::GuestAllocRaw(g_immCapacity, 0x100);
  }
  g_immPrimitiveType = primitiveType;
  g_immVertexCount = vertexCount;
  g_immStride = vertexStreamZeroStride;
  return g_immVertices;
}

void EndVerticesHook(GuestDevice* device) {
  if (g_immVertexCount == 0 || g_immVertices == 0) return;
  DrawVerticesUPHook(device, g_immPrimitiveType, g_immVertexCount,
                     ghp::ToHost<const void>(g_immVertices), g_immStride);
  g_immVertexCount = 0;
}

}  // namespace

REX_HOOK_RAW(sub_82803358) {
  const bool pixelShader = ctx.r4.u32 != 0;
  const uint32_t startRegister = ctx.r5.u32;
  const uint32_t registerCount = ctx.r6.u32;
  g_origGpuBeginShaderConstantF4.fn(ctx, base);

  const uint32_t payloadGuest = ctx.r3.u32;
  if (payloadGuest == 0 || registerCount == 0 || registerCount > 256 || startRegister >= 256)
    return;

  std::lock_guard lock(g_deferredShaderConstantsMutex);
  if (g_deferredShaderConstants.size() >= 2048) {
    static std::atomic<bool> loggedOverflow{false};
    if (!loggedOverflow.exchange(true, std::memory_order_relaxed)) {
      REXGPU_WARN("GpuBegin shader-constant queue overflow; dropping later payloads");
    }
    return;
  }
  g_deferredShaderConstants.push_back({payloadGuest, startRegister, registerCount, pixelShader});
  static std::atomic<bool> loggedActive{false};
  if (!loggedActive.exchange(true, std::memory_order_relaxed)) {
    REXGPU_INFO("Per-draw GpuBegin constants active: stage={} start={} count={}",
                pixelShader ? "PS" : "VS", startRegister, registerCount);
  }
}

// D3DCommandBuffer_CreateShaderConstantFFixup returns a handle describing the
// VS register range that a later SetShaderConstantF call will populate. FM2
// uses this path for per-object matrices that never enter GuestDevice's CPU
// constant file.
REX_HOOK_RAW(sub_823766E0) {
  const uint32_t startRegister = ctx.r5.u32;
  const uint32_t registerCount = ctx.r6.u32;
  g_origCbCreateShaderConstantFFixup.fn(ctx, base);

  if (registerCount == 0 || registerCount > 64 || startRegister >= 256)
    return;

  const uint32_t handle = ctx.r3.u32;
  std::lock_guard lock(g_commandBufferFixupMutex);
  g_commandBufferFixupRanges[handle] = {static_cast<uint16_t>(startRegister),
                                        static_cast<uint16_t>(registerCount)};
}

// Copy the values while the guest source is valid, but consume them only at
// the next draw so each draw receives the matrix/material values associated
// with its own command-buffer fixup.
REX_HOOK_RAW(sub_823767B8) {
  const uint32_t handle = ctx.r4.u32;
  const uint32_t sourceGuest = ctx.r5.u32;
  g_origCbSetShaderConstantF.fn(ctx, base);

  if (sourceGuest == 0)
    return;

  std::lock_guard lock(g_commandBufferFixupMutex);
  const auto rangeIt = g_commandBufferFixupRanges.find(handle);
  if (rangeIt == g_commandBufferFixupRanges.end())
    return;

  const CommandBufferFixupRange range = rangeIt->second;
  const auto* source = ghp::ToHost<const uint32_t>(sourceGuest);
  if (source == nullptr || range.registerCount == 0 || range.registerCount > 64)
    return;

  if (g_deferredCommandBufferConstants.size() >= 2048) {
    static std::atomic<bool> loggedOverflow{false};
    if (!loggedOverflow.exchange(true, std::memory_order_relaxed)) {
      REXGPU_WARN("Command-buffer shader-constant queue overflow; dropping later payloads");
    }
    return;
  }

  DeferredCommandBufferConstants constants{};
  constants.startRegister = range.startRegister;
  constants.registerCount = range.registerCount;
  std::memcpy(constants.values.data(), source, size_t(range.registerCount) * 16u);
  g_deferredCommandBufferConstants.push_back(std::move(constants));
  static std::atomic<bool> loggedActive{false};
  if (!loggedActive.exchange(true, std::memory_order_relaxed)) {
    REXGPU_INFO("Per-draw command-buffer constants active: start={} count={}", range.startRegister,
                range.registerCount);
  }
}

REX_HOOK(D3DDevice_DrawVertices, DrawVerticesHook);
REX_HOOK(D3DDevice_DrawIndexedVertices, DrawIndexedVerticesHook);
REX_HOOK(D3DDevice_DrawIndexedVertices_WithVertexFormatSetup,
         DrawIndexedVerticesWithVertexFormatHook);
REX_HOOK(D3DDevice_DrawVerticesUP, DrawVerticesUPHook);
REX_HOOK(D3DDevice_BeginVertices, BeginVerticesHook);
REX_HOOK(D3DDevice_EndVertices, EndVerticesHook);

REX_HOOK_RAW(PGR4_D3D_BeginCommandBufferBatch) {
  pgr4::render::RenderQueue::BeginRecording();
  g_origBeginCommandBufferBatch.fn(ctx, base);
}

REX_HOOK_RAW(PGR4_D3D_FinalizeCommandBufferBatch) {
  g_origFinalizeCommandBufferBatch.fn(ctx, base);
  pgr4::render::RenderQueue::EndRecording();
}

REX_HOOK_RAW(PGR4_D3D_CreateCommandBufferClone) {
  g_origCreateCommandBufferClone.fn(ctx, base);
  pgr4::render::RenderQueue::BindPendingRecording(ctx.r3.u32);
}

REX_HOOK_RAW(D3DCommandBuffer_CreateTextureFixup) {
  const uint32_t textureAddress = ctx.r5.u32;
  g_origCreateTextureFixup.fn(ctx, base);
  const uint32_t handle = ctx.r3.u32;
  const size_t matches =
      pgr4::render::RenderQueue::AssociatePendingTextureFixup(handle, textureAddress);
  static std::atomic<uint32_t> fixupCount{0};
  const uint32_t count = fixupCount.fetch_add(1, std::memory_order_relaxed) + 1;
  if (count <= 16) {
    REXGPU_INFO(
        "Deferred texture fixup recorded: n={} handle=0x{:08X} texture=0x{:08X} "
        "matches={}",
        count, handle, textureAddress, matches);
  }
}

REX_HOOK_RAW(D3DCommandBuffer_SetTexture) {
  const uint32_t cloneAddress = ctx.r3.u32;
  const uint32_t handle = ctx.r4.u32;
  const uint32_t textureAddress = ctx.r5.u32;
  g_origSetCommandBufferTexture.fn(ctx, base);

  rr::GuestBaseTexture* texture =
      textureAddress != 0 ? ghp::ToHost<rr::GuestBaseTexture>(textureAddress) : nullptr;
  const ResolvedTextureBinding binding = ResolveTextureBinding(texture);
  pgr4::render::RenderCommand replacement{};
  if (binding.baseTexture) {
    replacement.type = pgr4::render::RenderCommandType::SetTextureBase;
    replacement.setTextureBase.texture = binding.texture;
    replacement.setTextureBase.guestAddress = textureAddress;
  } else {
    replacement.type = pgr4::render::RenderCommandType::SetTexture;
    replacement.setTexture.texture = static_cast<rr::GuestTexture*>(binding.texture);
    replacement.setTexture.guestAddress = textureAddress;
  }
  const bool applied =
      pgr4::render::RenderQueue::SetRecordingTextureFixup(cloneAddress, handle, replacement);
  static std::atomic<uint32_t> setFixupCount{0};
  const uint32_t count = setFixupCount.fetch_add(1, std::memory_order_relaxed) + 1;
  if (count <= 16) {
    REXGPU_INFO(
        "Deferred texture fixup updated: n={} clone=0x{:08X} handle=0x{:08X} "
        "texture=0x{:08X} applied={}",
        count, cloneAddress, handle, textureAddress, applied);
  }
}

REX_HOOK_RAW(PGR4_D3D_EmitDirtyStateAndDrawList) {
  const uint32_t contextAddress = ctx.r3.u32;
  const uint32_t cloneAddress = ctx.r4.u32;
  pgr4::render::DeferredExecutionSnapshot executionSnapshot{};
  const uint8_t* context = nullptr;
  if (contextAddress != 0 &&
      contextAddress <= UINT32_MAX - pgr4::render::DeferredExecutionSnapshot::kContextBytes) {
    context = ghp::ToHost<const uint8_t>(contextAddress);
  }
  const bool captured = pgr4::render::CaptureDeferredExecutionSnapshot(executionSnapshot, context);
  g_origEmitDirtyStateAndDrawList.fn(ctx, base);

  if (pgr4::render::RenderQueue::ReplayRecording(cloneAddress,
                                                captured ? &executionSnapshot : nullptr)) {
    static std::atomic<uint32_t> replayCount{0};
    const uint32_t count = replayCount.fetch_add(1, std::memory_order_relaxed) + 1;
    if (count <= 16 || count % 300 == 0) {
      REXGPU_INFO("Deferred D3D command buffer replay: n={} clone=0x{:08X} liveMaterial={}", count,
                  cloneAddress, captured);
    }
  }
}

// ---------------------------------------------------------------------------
// GPU-hang watchdog defusal. D3D_CommandWaitForCompletion (a still-running,
// unhooked original function -- called both by the PM4 draw emitters this
// renderer bypasses AND by independent guest worker threads waiting for the
// GPU ring to "catch up") spins on D3D::CBlocker::Check waiting for a ring-
// position counter this renderer never advances (that bookkeeping lived
// entirely inside the original PM4 emission code the draw hooks replace).
// Confirmed via a live thread-stack dump: a guest worker thread spins here
// indefinitely, its internal ~5-unit timeout never resolving because the
// progress counter it polls never moves, until D3D_GpuHangHandler's
// last-resort path decides the GPU is unrecoverably hung and executes a
// literal __trap() -- silently killing the whole process with no visible
// crash dialog.
//
// This renderer never actually needs the wait: Video::Present() already
// fully drains the GPU (waitForCommandFence) every frame, so by the time any
// guest code could ask "has the GPU caught up", it always already has.
// Hooking Check() to unconditionally report "not blocked" short-circuits
// the spin after its first iteration without ever reaching the hang
// watchdog, while leaving D3D_CommandWaitForCompletion's own (still
// relevant to other unhooked PM4 code) submit-triggering logic untouched.
// ---------------------------------------------------------------------------

namespace {
uint32_t CBlockerCheckHook(uint32_t /*blockerThis*/) {
  return 0;
}
}  // namespace

REX_HOOK(D3D_CBlocker_Check, CBlockerCheckHook);

// XDK entry points that Unleashed also stubs and that have no surviving host
// state. Leave guest registers untouched, avoid per-call logging, and keep
// tiling/predication out of FM2's native D3D path.
#define PGR4_D3D_GPU_NOOP(name) \
  REX_HOOK_RAW(name) {}

PGR4_D3D_GPU_NOOP(D3DDevice_SetGammaRamp);
PGR4_D3D_GPU_NOOP(D3DDevice_SetShaderGPRAllocation);
PGR4_D3D_GPU_NOOP(D3DDevice_SetPredication);
PGR4_D3D_GPU_NOOP(D3DDevice_Release);

#undef PGR4_D3D_GPU_NOOP
