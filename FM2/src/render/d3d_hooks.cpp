#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rex/cvar.h>
#include <rex/hash.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xevent.h>
#include <rex/system/util/object_table.h>

#include "native_renderer/fm2_direct_draw_decode.h"
#include "native_renderer/fm2_native_renderer.h"
#include "render/guest_device.h"
#include "render/guest_heap.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_state.h"
#include "render/fm2_device.h"
#include "render/video.h"

namespace rr = fm2::render;
namespace ghp = fm2::ghp;
namespace nr = fm2::native_renderer;

// Win32 thread-id without pulling in <windows.h> (avoids min/max macro clashes).
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);

namespace fm2::render {
GuestBuffer *CreateVertexBuffer(uint32_t length);
GuestBuffer *CreateIndexBuffer(uint32_t length, uint32_t format);
void RegisterBufferAlias(uint32_t guestAddr, GuestBuffer *buf);
GuestBuffer *LookupBufferAlias(uint32_t guestAddr);
GuestTexture *CreateTexture(uint32_t width, uint32_t height, uint32_t depth,
                            uint32_t levels, uint32_t usage, uint32_t format,
                            uint32_t pool, uint32_t type);
GuestSurface *CreateSurface(uint32_t width, uint32_t height, uint32_t format,
                            uint32_t multiSample);
GuestVertexDeclaration *
CreateVertexDeclaration(GuestVertexElement *guestElements);
void RegisterVertexDeclarationAlias(uint32_t guestAddress,
                                    GuestVertexDeclaration *declaration);
GuestShader *CreateVertexShader(const uint32_t *function);
GuestShader *CreatePixelShader(const uint32_t *function);
void RegisterShaderAlias(uint32_t guestAddress, GuestShader *shader);
GuestShader *LookupShaderAlias(uint32_t guestAddress);
GuestTexture *LoadTextureFromMemory(const uint8_t *data, uint32_t size);
GuestTexture *TranslateGuestTextureFetch(const void *guestFetch,
                                         bool uploadGuestData);
GuestTexture *LookupGuestTextureByBase(uint32_t baseAddress);
GuestTexture *TranslateGuestTexture(void *guestHeader, bool uploadGuestData);
GuestBaseTexture *TranslateGuestSurface(void *guestHeader);
uint32_t LockVertexBuffer(GuestBuffer *buffer, uint32_t flags);
void UnlockVertexBuffer(GuestBuffer *buffer);
uint32_t LockIndexBuffer(GuestBuffer *buffer, uint32_t flags);
void UnlockIndexBuffer(GuestBuffer *buffer);
void LockTextureRect(GuestTexture *texture, uint32_t *outPitch,
                     uint32_t *outBits);
void UnlockTextureRect(GuestTexture *texture);
} // namespace fm2::render

REX_IMPORT(__imp__FM2_RenderContext_SetPixelShaderState,
           g_origFm2SetPixelShaderState, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetVertexShaderState,
           g_origFm2SetVertexShaderState, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_BindVertexStream,
           g_origFm2BindVertexStream,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t));
REX_IMPORT(__imp__FM2_RenderContext_BindIndexBuffer,
           g_origFm2BindIndexBuffer, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetBoundSurface,
           g_origFm2SetBoundSurface, void(uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_TryPresentAndUpdateStatus,
           g_origFm2TryPresentAndUpdateStatus, void(uint32_t));
REX_IMPORT(__imp__FM2_GpuCommandBuffer_BuildAndSubmit,
           g_origFm2GpuCommandBufferBuildAndSubmit,
           void(uint32_t, uint32_t, uint32_t));
// Guest render-worker per-frame dispatch (sub_82288948), called in an infinite
// loop by sub_82289640 on a guest XThread. In plume_native the present triggers
// (FMOD pump bit / sub_825ADE20) never fire, so we drive Video::Present() from
// here — same thread that records draws into g_commandList (no cross-thread race).
REX_IMPORT(__imp__sub_82288948, g_origRenderWorkerFrame, void(uint32_t));
// FM2_RenderContext_SetActivePassId (sub_8236E228): stores the active vertex
// declaration handle at renderContext+11540, read by FM2_D3D_EmitDrawListStatePackets
// at draw time. Forza binds the per-draw vertex declaration here (a D3DVERTEXELEMENT9
// declaration created via D3DDevice_CreateVertexDeclaration), NOT via the standard
// D3DDevice_SetVertexDeclaration that writes the device field -- so device+0x2E24 is
// always 0 in plume_native and every draw fails with missing_decl. Hook it to feed
// device->vertexDeclaration so the existing Sync/alias path resolves the layout.
REX_IMPORT(__imp__sub_8236E228, g_origSetActivePassId, void(uint32_t, uint32_t));
// FM2_RenderContext_BindSurfaceInternal (sub_823716F8): binds a COLOR render-target
// surface to slot a2 (0-3) at renderContext+12160+4*slot -- this is Forza's color RT
// bind (proven: FM2_D3D_CreatePresentBackbufferResources calls it with slot 0 for the
// backbuffer color surface, while depth goes through SetBoundSurface at +12176). The
// native renderer never hooked it, so g_renderTarget stayed null and forward draws had
// no color attachment -> rendered depth-only -> blue screen. Hook it to bind the color
// surface natively via SetRenderTargetNative, mirroring the depth path.
REX_IMPORT(__imp__sub_823716F8, g_origBindSurfaceInternal,
           void(uint32_t, uint32_t, uint32_t));
// Frame-sync gate: the guest game-loop thread (sub_822172E8) blocks each frame on
// FM2_Win32_WaitForSingleObject(dword_829C24C0,-1) when in vsync frame-sync mode.
// FM2_SignalGate pulses that event to advance the loop, but its normal trigger is
// dead in plume_native (no Xenia GPU/vblank), so the game never advances/renders.
// We pulse it ourselves (rate-limited) from the per-frame present hook.
REX_IMPORT(__imp__FM2_SignalGate_8220A4E8, g_origSignalGate, void(uint32_t));
REX_IMPORT(__imp__FM2_FmodIrqSubmit_8236C688,
           g_origFm2FmodIrqSubmit8236C688,
           void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_ProducerProgressGuard_82369340,
           g_origFm2ProducerProgressGuard, uint32_t(uint32_t));
REX_IMPORT(__imp__FM2_Render_LoadPixelShaderResourceById,
           g_origFm2LoadPixelShaderResourceById, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_Render_LoadVertexShaderResourceById,
           g_origFm2LoadVertexShaderResourceById, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_Render_AllocGpuPassMemoryBlock,
           g_origFm2AllocGpuPassMemoryBlock, uint32_t(uint32_t));
REX_IMPORT(__imp__FM2_D3D_CreateGpuMemoryBlock,
           g_origFm2CreateGpuMemoryBlock, uint32_t(uint32_t));

REX_IMPORT(__imp__FM2_RenderContext_SetDepthStencilEnableState,
           g_origFm2SetDepthStencilEnableState, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetAlphaBlendEnableBits,
           g_origFm2SetAlphaBlendEnableBits, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetAlphaTestState,
           g_origFm2SetAlphaTestState, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetDepthCompareBits,
           g_origFm2SetDepthCompareBits, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetColorWriteMaskBits,
           g_origFm2SetColorWriteMaskBits, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane0Enable,
           g_origFm2SetClipPlane0Enable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane1Enable,
           g_origFm2SetClipPlane1Enable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane2Enable,
           g_origFm2SetClipPlane2Enable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane3Enable,
           g_origFm2SetClipPlane3Enable, void(uint32_t, uint32_t));

REX_IMPORT(__imp__FM2_D3DDevice_CreateVertexBuffer, g_origCreateVertexBuffer,
           uint32_t(uint32_t));
REX_IMPORT(__imp__FM2_D3DDevice_CreateIndexBuffer, g_origCreateIndexBuffer,
           uint32_t(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3DVertexBuffer_Lock, g_origVertexBufferLock,
           uint32_t(void *, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_LockGpuBufferRaw, g_origIndexBufferLock,
           uint32_t(void *, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3DSurface_GetDesc, g_origSurfaceGetDesc,
           void(void *, void *));
REX_IMPORT(__imp__FM2_D3DSurface_LockRect, g_origSurfaceLockRect,
           void(rr::GuestTexture *, void *, void *, uint32_t));
REX_IMPORT(__imp__FM2_D3DResource_UnlockResource, g_origUnlockResource,
           void(rr::GuestResource *, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_EmitIndexedDrawPm4Packets,
           g_origFm2EmitIndexedDrawPm4Packets,
           void(uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset,
           g_origFm2EmitIndexedDrawPm4PacketsWithGpuOffset,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_EmitIndexedDrawPm4WithVertexFormatSetup,
           g_origFm2EmitIndexedDrawPm4WithVertexFormatSetup,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
// Forward/opaque object pass draw emitter (a1=CDevice/context, a2=drawListNode,
// a3=flags). The depth prepass uses EmitIndexedDrawPm4* (hooked above); the
// forward COLOR pass goes through here and is NOT translated to native draws ->
// the color RT never gets written -> black. Diagnostic hook to decode the draw
// list before translating.
REX_IMPORT(__imp__FM2_D3D_EmitDirtyStateAndDrawList,
           g_origFm2EmitDirtyStateAndDrawList,
           void(uint32_t, uint32_t, uint32_t));
// Forza's PM4 draw-list interpreter (walks command nodes, dispatches opcodes).
// Hooked only to count whether the 3D-scene draw path runs in plume_native.
REX_IMPORT(__imp__FM2_Render_WalkAndDispatchPm4DrawList,
           g_origWalkAndDispatchPm4DrawList, void(uint32_t));
// EDRAM resolve emitter: copies the rendered surface to guest memory that the
// game then samples as a texture. Untranslated in plume_native -> sampled
// memory is empty/black. Hooked to register the resolved surface so samples hit
// the real rendered plume texture.
REX_IMPORT(__imp__FM2_D3D_EmitSurfaceResolvePackets, g_origEmitSurfaceResolve,
           uint32_t(uint32_t, uint32_t, uint32_t));

namespace {

// Writes directly to the FM2 clean log so traces appear alongside LogLine output.
template <typename... Args>
static void LogReplayDbg(const char* fmt, Args&&... args) {
  FILE* f = std::fopen("C:\\temp\\fm2-clean.log", "a");
  if (!f) return;
  std::fprintf(f, fmt, std::forward<Args>(args)...);
  std::fputc('\n', f);
  std::fflush(f);
  std::fclose(f);
}

// Self-contained RenderDoc in-app capture trigger (no header/include-path dep).
// TriggerCapture is the 16th fn ptr in RENDERDOC_API_1_0_0 (renderdoc_app.h).
// When a frame draws real geometry (>threshold draws) we auto-trigger a capture
// so a *render* frame is captured without the user having to time the key.
// Layout matches RENDERDOC_API_1_0_1 fn-ptr order (renderdoc_app.h).
struct MiniRdocApi {
  void *fn[15];                        // GetAPIVersion .. GetCapture (0..14)
  void(__cdecl *TriggerCapture)(void); // 15
  void *fn2[3];                        // IsRemoteAccessConnected,LaunchReplayUI,SetActiveWindow (16..18)
  void(__cdecl *StartFrameCapture)(void *dev, void *wnd);       // 19
  void *IsFrameCapturing;                                       // 20
  uint32_t(__cdecl *EndFrameCapture)(void *dev, void *wnd);     // 21
};
std::atomic<uint32_t> g_drawsSinceLastPresent{0};
MiniRdocApi *RdocApi() {
  static MiniRdocApi *s_rdoc = nullptr;
  static bool s_tried = false;
  if (!s_tried) {
    s_tried = true;
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
      using GetApiFn = int(__cdecl *)(int, void **);
      if (auto get = reinterpret_cast<GetApiFn>(
              GetProcAddress(mod, "RENDERDOC_GetAPI"))) {
        get(10000 /*eRENDERDOC_API_Version_1_0_0*/,
            reinterpret_cast<void **>(&s_rdoc));
      }
    }
    LogReplayDbg("FM2_RDOC_INIT renderdoc.dll=%s api=%s",
                 GetModuleHandleA("renderdoc.dll") ? "loaded" : "absent",
                 s_rdoc ? "ok" : "null");
  }
  return s_rdoc;
}
// Scene draws land on isolated/alternating frames, so TriggerCapture (next frame)
// keeps missing them. Instead wrap the WHOLE render-worker call (Start at top,
// before the draws execute, End after present) for a short window of frames once
// drawing is detected -- a real geometry frame is captured in full.
std::atomic<int> g_rdocCaptureRemaining{0};
std::atomic<bool> g_rdocArmed{false};
bool RenderDocFrameBegin() {
  MiniRdocApi *api = RdocApi();
  if (api == nullptr || g_rdocCaptureRemaining.load(std::memory_order_relaxed) <= 0)
    return false;
  api->StartFrameCapture(nullptr, nullptr);
  return true;
}
void RenderDocFrameEnd(bool capturing, uint32_t drawsThisFrame) {
  MiniRdocApi *api = RdocApi();
  if (capturing && api != nullptr) {
    api->EndFrameCapture(nullptr, nullptr);
    g_rdocCaptureRemaining.fetch_sub(1, std::memory_order_relaxed);
  }
  if (api != nullptr && drawsThisFrame > 5u &&
      !g_rdocArmed.exchange(true, std::memory_order_relaxed)) {
    g_rdocCaptureRemaining.store(8, std::memory_order_relaxed);
    LogReplayDbg("FM2_RDOC_ARM draws=%u capturing next 8 frames", drawsThisFrame);
  }
}

using rr::GuestBaseTexture;
using rr::GuestBuffer;
using rr::GuestDevice;
using rr::GuestShader;
using rr::GuestSurface;
using rr::GuestTexture;
using rr::GuestVertexDeclaration;

REXCVAR_DEFINE_UINT32(
    fm2_plume_gate_pulse_hz, 1000, "FM2",
    "plume_native: rate (Hz) to pulse the guest frame-sync gate so the game loop "
    "advances in every state. Default 1000 (far above any real framerate, so "
    "effectively uncapped) without pinning a core; 0 = truly unlimited spin.");
REXCVAR_DEFINE_BOOL(
    fm2_shader_resource_dump, false, "FM2",
    "Dump FM2 shader resource payloads loaded through title shader IDs.");
REXCVAR_DEFINE_UINT32(
    fm2_shader_resource_dump_limit, 128, "FM2",
    "Maximum unique FM2 shader resource payloads to dump in one run.");
REXCVAR_DEFINE_BOOL(
    fm2_plume_render_hook_trace, false, "FM2",
    "Emit bounded diagnostics when FM2 native render hooks fire.");
REXCVAR_DEFINE_UINT32(
    fm2_plume_render_hook_trace_limit, 128, "FM2",
    "Maximum FM2 native render hook trace lines to emit per hook in one run; 0 is unlimited.");
REXCVAR_DEFINE_BOOL(
    fm2_plume_vdswap_present, true, "FM2",
    "Mirror FM2's VdSwap frontbuffer into the active Plume swapchain.");
REXCVAR_DEFINE_BOOL(
    fm2_plume_bypass_prod_guard_wait, true, "FM2",
    "Make FM2 producer-progress guard report ready in Plume mode (required in "
    "plume_native/plume_clear: no Xenia GPU backend to advance completion fences).");

struct GuestRasterizerState {
  uint8_t pad0[8];
  rex::be<uint32_t> cullMode;
};
static_assert(offsetof(GuestRasterizerState, cullMode) == 8);

// Guest D3DLOCKED_RECT { DWORD Pitch; void* pBits; } (big-endian).
struct GuestLockedRect {
  rex::be<uint32_t> pitch;
  rex::be<uint32_t> bits;
};

template <typename T> T *AsFm2(T *p) {
  return rr::IsFm2Resource(p) ? p : nullptr;
}

uint32_t NextRenderHookTraceIndex(uint32_t &counter) {
  if (!REXCVAR_GET(fm2_plume_render_hook_trace)) {
    return 0;
  }
  const uint32_t index = ++counter;
  const uint32_t limit = REXCVAR_GET(fm2_plume_render_hook_trace_limit);
  if (limit != 0 && index > limit) {
    return 0;
  }
  return index;
}

bool ShouldMirrorPlumeRenderState() {
  return !nr::WantsReXGraphics();
}

// Per-second present diagnostics: prove which hook drives present, on which
// thread, and whether it has a valid present source. One line per second.
struct PresentDiagSlot {
  std::atomic<uint64_t> calls{0};
  std::atomic<uint64_t> withSource{0};
  std::atomic<uint32_t> lastTid{0};
  std::atomic<uint64_t> lastSec{0};
};

void CountPresentDiag(const char *label, PresentDiagSlot &slot,
                      const rr::GuestBaseTexture *presentSource) {
  slot.calls.fetch_add(1, std::memory_order_relaxed);
  if (presentSource != nullptr && presentSource->texture != nullptr) {
    slot.withSource.fetch_add(1, std::memory_order_relaxed);
  }
  slot.lastTid.store(static_cast<uint32_t>(::GetCurrentThreadId()),
                     std::memory_order_relaxed);
  using clock = std::chrono::steady_clock;
  const uint64_t nowSec = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(clock::now().time_since_epoch())
          .count());
  uint64_t last = slot.lastSec.load(std::memory_order_relaxed);
  if (last == 0) {
    slot.lastSec.store(nowSec, std::memory_order_relaxed);
    return;
  }
  if (nowSec == last ||
      !slot.lastSec.compare_exchange_strong(last, nowSec, std::memory_order_relaxed)) {
    return;
  }
  const uint64_t calls = slot.calls.exchange(0, std::memory_order_relaxed);
  const uint64_t withSrc = slot.withSource.exchange(0, std::memory_order_relaxed);
  LogReplayDbg("FM2_PRESENT_DIAG sec=%llu hook=%s calls=%llu with_source=%llu tid=%u src=%p",
               (unsigned long long)nowSec, label, (unsigned long long)calls,
               (unsigned long long)withSrc,
               slot.lastTid.load(std::memory_order_relaxed),
               (const void *)presentSource);
}

// Per-second tally of native draw device availability. The PM4 draw emitters
// rely on the global active guest device (set by the Set* render-context hooks);
// if it is null the draw silently bails before opening a frame. This proves
// whether that is happening in steady state.
struct DrawDeviceSlot {
  std::atomic<uint64_t> ok{0};
  std::atomic<uint64_t> nullDev{0};
  std::atomic<uint64_t> lastSec{0};
};
void CountDrawDevice(const char *label, DrawDeviceSlot &slot, bool deviceOk) {
  (deviceOk ? slot.ok : slot.nullDev).fetch_add(1, std::memory_order_relaxed);
  using clock = std::chrono::steady_clock;
  const uint64_t nowSec = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          clock::now().time_since_epoch())
          .count());
  uint64_t last = slot.lastSec.load(std::memory_order_relaxed);
  if (last == 0) {
    slot.lastSec.store(nowSec, std::memory_order_relaxed);
    return;
  }
  if (nowSec == last ||
      !slot.lastSec.compare_exchange_strong(last, nowSec,
                                            std::memory_order_relaxed)) {
    return;
  }
  const uint64_t ok = slot.ok.exchange(0, std::memory_order_relaxed);
  const uint64_t nul = slot.nullDev.exchange(0, std::memory_order_relaxed);
  LogReplayDbg("FM2_DRAW_DEVICE sec=%llu hook=%s ok=%llu null=%llu",
               (unsigned long long)nowSec, label, (unsigned long long)ok,
               (unsigned long long)nul);
}

// Host-side pulse of the guest frame-sync event (handle stored at guest
// dword_829C24C0). Looks the handle up in the kernel object table and pulses the
// event directly. SAFE from any (native) thread: touches only host kernel
// objects, never recompiled guest code (calling guest code from a native thread
// crashes — no guest thread context). The event ref is cached to avoid object-
// table lock contention when pulsing at a high/unlimited rate.
void HostPulseFrameSyncGate() {
  auto *handlePtr = ghp::ToHost<rex::be<uint32_t>>(0x829C24C0u);
  if (handlePtr == nullptr)
    return;
  const uint32_t handle = handlePtr->get();
  if (handle == 0)
    return;
  static uint32_t s_cachedHandle = 0;
  static rex::system::object_ref<rex::system::XEvent> s_ev;
  if (handle != s_cachedHandle || !s_ev) {
    auto *ks = rex::system::kernel_state();
    if (ks == nullptr)
      return;
    s_ev = ks->object_table()->LookupObject<rex::system::XEvent>(handle);
    s_cachedHandle = handle;
  }
  if (s_ev)
    s_ev->Pulse(0, false);
}

// Persistent frame-sync gate driver: a dedicated thread pulses the gate so the
// guest game loop advances in EVERY game state (not just while the loading-screen
// frame loop runs). Safe because it pulses host-side (HostPulseFrameSyncGate).
// Rate via fm2_plume_gate_pulse_hz (0 = unlimited / uncapped framerate).
void StartGatePulseThreadOnce() {
  static std::once_flag s_once;
  std::call_once(s_once, [] {
    std::thread([] {
      static PresentDiagSlot s_gateThreadSlot;
      for (;;) {
        if (ShouldMirrorPlumeRenderState()) {
          HostPulseFrameSyncGate();
          CountPresentDiag("GateThread", s_gateThreadSlot, nullptr);
        }
        const uint32_t hz = REXCVAR_GET(fm2_plume_gate_pulse_hz);
        if (hz == 0) {
          std::this_thread::yield();
        } else {
          std::this_thread::sleep_for(std::chrono::microseconds(1000000u / hz));
        }
      }
    }).detach();
  });
}

uint32_t ReadGuestU32At(uint32_t guestAddress);

GuestDevice *DeviceForRenderContext(uint32_t renderContext) {
  if (renderContext != 0) {
    auto *device = ghp::ToHost<GuestDevice>(renderContext);
    rr::SetActiveGuestDevice(device);
    return device;
  }
  return rr::GetActiveGuestDevice();
}

void SetStreamSourceNative(GuestDevice *device, uint32_t stream,
                           GuestBuffer *buffer, uint32_t offset,
                           uint32_t stride, uint64_t mask);
void SetIndicesNative(GuestDevice *device, GuestBuffer *buffer);
void SetRenderTargetNative(GuestDevice *device, uint32_t index,
                           GuestSurface *surface);
void SetVertexShaderNative(GuestDevice *device, GuestShader *shader);
void SetPixelShaderNative(GuestDevice *device, GuestShader *shader);
void DrawIndexedVertices(GuestDevice *device, uint32_t primType,
                         int32_t baseVertexIndex, uint32_t startIndex,
                         uint32_t indexCount);
void DrawVerticesUP(GuestDevice *device, uint32_t primType,
                    uint32_t vertexCount, void *data, uint32_t stride);

struct PendingImmediateDraw {
  GuestDevice *device = nullptr;
  uint32_t primType = 0;
  uint32_t vertexCount = 0;
  uint32_t stride = 0;
  uint32_t stagingAddr = 0;
  uint32_t stagingSize = 0;
};
PendingImmediateDraw g_pendingImmediateDraw;

void FlushImmediateVertices() {
  PendingImmediateDraw &p = g_pendingImmediateDraw;
  if (p.device == nullptr)
    return;
  GuestDevice *device = p.device;
  p.device = nullptr; 
  rr::DrawPrimitiveUP(device, p.primType, p.vertexCount,
                      ghp::ToHost<void>(p.stagingAddr), p.stride);
}

bool IsDepthFormat(plume::RenderFormat format) {
  return format == plume::RenderFormat::D16_UNORM ||
         format == plume::RenderFormat::D32_FLOAT ||
         format == plume::RenderFormat::D32_FLOAT_S8_UINT;
}

uint32_t BeginVertices(GuestDevice *device, uint32_t primType,
                       uint32_t vertexCount, uint32_t stride) {
  FlushImmediateVertices();
  const uint32_t size = vertexCount * stride;
  if (size == 0)
    return 0; // mirrors the guest's BeginRingAlloc-failure path
  PendingImmediateDraw &p = g_pendingImmediateDraw;
  if (size > p.stagingSize) {
    p.stagingAddr = ghp::GuestAllocRaw(size, 0x10); // grow-only staging
    p.stagingSize = size;
  }
  p.device = device;
  p.primType = primType;
  p.vertexCount = vertexCount;
  p.stride = stride;
  return p.stagingAddr;
}
constexpr uint32_t kScratchRingSize = 0x100000; // 1 MiB
constexpr uint32_t kScratchRingSlack =
    0x40000; // burst headroom past the threshold

uint32_t KickOff(GuestDevice *device) {
  static uint32_t s_scratchRing = 0;
  if (s_scratchRing == 0)
    s_scratchRing = ghp::GuestAllocRaw(kScratchRingSize, 0x100);
  auto *fields = reinterpret_cast<rex::be<uint32_t> *>(device);
  fields[48 / 4] = s_scratchRing; // write cursor
  fields[56 / 4] =
      s_scratchRing + kScratchRingSize - kScratchRingSlack; // kick threshold
  fields[13428 / 4] =
      s_scratchRing; // segment start (EndVertices cursor restore)
  return s_scratchRing;
}

void BlockOnSecondaryPosition(GuestDevice * /*device*/, uint32_t /*position*/,
                              uint32_t /*flags*/) {}

void Fm2Present(uint32_t presentChain) {
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=Fm2Present n={} presentChain=0x{:08X}",
                n, presentChain);
  }
  g_origFm2TryPresentAndUpdateStatus(presentChain);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  FlushImmediateVertices();
  // Diagnostic only: confirmed FM2_D3D_TryPresentAndUpdateStatus does NOT fire in
  // plume_native, so present is driven from Fm2GpuCommandBufferBuildAndSubmit
  // instead. Leave this so we notice if that ever changes.
  static PresentDiagSlot s_fm2PresentSlot;
  CountPresentDiag("Fm2Present", s_fm2PresentSlot, nullptr);
  Video::Present();
}

void Fm2GpuCommandBufferBuildAndSubmit(uint32_t commandBuffer, uint32_t arg4,
                                       uint32_t arg5) {
  static uint32_t s_traceCount = 0;
  const uint32_t n = NextRenderHookTraceIndex(s_traceCount);
  const uint32_t cursorBefore = ReadGuestU32At(commandBuffer + 48u);
  const uint32_t kickThreshold = ReadGuestU32At(commandBuffer + 56u);
  const uint32_t ringBase = ReadGuestU32At(commandBuffer + 10768u);
  const uint32_t skipVdSwapFlag = ReadGuestU32At(commandBuffer + 21156u);
  const uint32_t pendingStart = ReadGuestU32At(commandBuffer + 21240u);
  const uint32_t pendingEnd = ReadGuestU32At(commandBuffer + 21244u);
  const uint32_t pendingEnabled = ReadGuestU32At(commandBuffer + 21248u);
  if (n != 0) {
    REXGPU_INFO(
        "FM2_PLUME_RENDER_HOOK hook=GpuBuildSubmit n={} stage=entry "
        "cmd=0x{:08X} r4=0x{:08X} r5=0x{:08X} cursor=0x{:08X} "
        "kick=0x{:08X} ring=0x{:08X} skipVdSwapFlag=0x{:08X} "
        "pending={}->{} enabled=0x{:08X}",
        n, commandBuffer, arg4, arg5, cursorBefore, kickThreshold, ringBase,
        skipVdSwapFlag, pendingStart, pendingEnd, pendingEnabled);
  }

  if (ShouldMirrorPlumeRenderState()) {
    // plume_native / plume_clear: the Xenia CP thread is disabled, so the
    // original body would crash kicking the GPU ring buffer. Skip it, but
    // replicate the midasm FM2PlumeTraceVdSwap that normally runs at the end of
    // the original (0x8236CD78): set the present source from the last-bound
    // color render target and present. This is the game/render thread that
    // records draws into g_commandList, so present is correctly same-threaded.
    rr::GuestBaseTexture *presentSource = rr::GetLastDrawnColorRenderTarget();
    if (presentSource != nullptr && presentSource->texture != nullptr) {
      rr::SetPresentSource(presentSource);
    }
    static PresentDiagSlot s_gpuCmdBufSlot;
    CountPresentDiag("GpuCmdBuf", s_gpuCmdBufSlot, presentSource);
    Video::Present();
    return;
  }

  g_origFm2GpuCommandBufferBuildAndSubmit(commandBuffer, arg4, arg5);

  if (n != 0) {
    const uint32_t cursorAfter = ReadGuestU32At(commandBuffer + 48u);
    const uint32_t lastStatus = ReadGuestU32At(commandBuffer + 21288u);
    const uint32_t flags = ReadGuestU32At(commandBuffer + 21896u);
    REXGPU_INFO(
        "FM2_PLUME_RENDER_HOOK hook=GpuBuildSubmit n={} stage=exit "
        "cmd=0x{:08X} cursor=0x{:08X}->0x{:08X} lastStatus=0x{:08X} "
        "flags=0x{:08X}",
        n, commandBuffer, cursorBefore, cursorAfter, lastStatus, flags);
  }
}


void Fm2FmodIrqSubmitSafe(uint32_t device, uint32_t flags) {
  // In plume_native / plume_clear mode the Xenia GPU IRQ infrastructure is not
  // running, so the original would crash. Skip it entirely.
  if (ShouldMirrorPlumeRenderState()) {
    return;
  }
  g_origFm2FmodIrqSubmit8236C688(device, flags);
}

uint32_t Fm2ProducerProgressGuard(uint32_t state) {
  if (!ShouldMirrorPlumeRenderState() ||
      !REXCVAR_GET(fm2_plume_bypass_prod_guard_wait)) {
    return g_origFm2ProducerProgressGuard(state);
  }

  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO(
        "FM2_PLUME_RENDER_HOOK hook=ProducerProgressGuard n={} "
        "state=0x{:08X} bypass_return=0",
        n, state);
  }
  return 0;
}

void Fm2SetPixelShaderState(uint32_t renderContext, uint32_t shader) {
  g_origFm2SetPixelShaderState(renderContext, shader);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=SetPixelShaderState n={} "
                "renderContext=0x{:08X} shader=0x{:08X} alias=0x{:08X}",
                n, renderContext, shader,
                ghp::ToGuest(rr::LookupShaderAlias(shader)));
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr || shader == 0) {
    return;
  }
  SetPixelShaderNative(device, ghp::ToHost<GuestShader>(shader));
}

void Fm2SetVertexShaderState(uint32_t renderContext, uint32_t shader) {
  g_origFm2SetVertexShaderState(renderContext, shader);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=SetVertexShaderState n={} "
                "renderContext=0x{:08X} shader=0x{:08X} alias=0x{:08X}",
                n, renderContext, shader,
                ghp::ToGuest(rr::LookupShaderAlias(shader)));
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr || shader == 0) {
    return;
  }
  SetVertexShaderNative(device, ghp::ToHost<GuestShader>(shader));
}

void Fm2BindVertexStream(uint32_t renderContext, uint32_t slot,
                         uint32_t resource, uint32_t byte_offset,
                         uint32_t stride_bytes, uint64_t dirty_mask) {
  g_origFm2BindVertexStream(renderContext, slot, resource, byte_offset,
                            stride_bytes, dirty_mask);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  // Prefer the native alias (uploaded Plume buffer); fall back to the raw XDK
  // header path in SetStreamSourceNative which reads the guest data pointer.
  GuestBuffer *buf = ghp::ToHost<GuestBuffer>(resource);
  if (!rr::IsFm2Resource(buf))
    if (GuestBuffer *alias = rr::LookupBufferAlias(resource))
      buf = alias;
  SetStreamSourceNative(device, slot, buf, byte_offset, stride_bytes, dirty_mask);
}

void Fm2BindIndexBuffer(uint32_t renderContext, uint32_t resource) {
  g_origFm2BindIndexBuffer(renderContext, resource);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  GuestBuffer *buf = ghp::ToHost<GuestBuffer>(resource);
  if (!rr::IsFm2Resource(buf))
    if (GuestBuffer *alias = rr::LookupBufferAlias(resource))
      buf = alias;
  SetIndicesNative(device, buf);
}

void Fm2SetBoundSurface(uint32_t renderContext, uint32_t surface,
                        uint32_t surfaceArg) {
  g_origFm2SetBoundSurface(renderContext, surface, surfaceArg);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr || surface == 0) {
    return;
  }
  // FM2_RenderContext_SetBoundSurface is the guest equivalent of BOTH
  // SetRenderTarget and SetDepthStencilSurface (per IDA). Classify the resource
  // and route depth surfaces to the depth slot instead of always binding color
  // index 0 -- otherwise depth is never bound and depth-tested draws sanitize to
  // depthStencilFormat=UNKNOWN.
  GuestSurface *gs = ghp::ToHost<GuestSurface>(surface);
  rr::GuestBaseTexture *tex = AsFm2(gs);
  if (tex == nullptr && gs != nullptr)
    tex = rr::TranslateGuestSurface(gs);
  if (tex != nullptr && tex->type == rr::ResourceType::DepthStencil) {
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 8) {
      LogReplayDbg("FM2_BOUND_DEPTH surface=0x%08X tex=%p", surface,
                   (void *)tex);
    }
    // tex is already resolved + confirmed DepthStencil; bind it directly via the
    // native setter (the local d3d_hooks wrapper would just re-translate).
    rr::SetDepthStencilSurface(device, static_cast<GuestSurface *>(tex));
  } else {
    SetRenderTargetNative(device, 0, gs);
  }
}

void Fm2RememberGuestDevice(GuestDevice *device) {
  if (device != nullptr) {
    rr::SetActiveGuestDevice(device);
  }
}

// Forza binds its COLOR render target(s) via BindSurfaceInternal(ctx, slot, surface)
// (slot 0 = primary color RT), not the standard D3DDevice_SetRenderTarget. Without
// this, g_renderTarget is null, forward draws have no color attachment, and the
// screen stays at its clear color. Mirror the color surface into the native render
// target like the depth path mirrors SetBoundSurface.
void RegisterSurfaceApertures(uint32_t descriptorAddr,
                              rr::GuestBaseTexture *host);

void Fm2BindSurface(uint32_t renderContext, uint32_t slot, uint32_t surface) {
  g_origBindSurfaceInternal(renderContext, slot, surface);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr || surface == 0) {
    return;
  }
  static std::atomic<uint32_t> s_n{0};
  if (s_n.fetch_add(1, std::memory_order_relaxed) < 16) {
    LogReplayDbg("FM2_BIND_SURFACE renderContext=0x%08X slot=%u surface=0x%08X",
                 renderContext, slot, surface);
  }
  GuestSurface *gs = ghp::ToHost<GuestSurface>(surface);
  rr::GuestBaseTexture *host = AsFm2(gs);
  if (host == nullptr && gs != nullptr)
    host = rr::TranslateGuestSurface(gs);
  RegisterSurfaceApertures(surface, host);
  SetRenderTargetNative(device, slot, gs);
}

// Forza binds the per-draw vertex declaration via SetActivePassId(ctx, declHandle)
// (the "pass id" is a D3DVERTEXELEMENT9 declaration created with
// D3DDevice_CreateVertexDeclaration). The standard D3D device declaration field
// (GuestDevice +0x2E24) is never written, so SyncVertexDeclarationFromDevice sees 0
// and every PM4 draw fails missing_decl. Mirror the handle into device->vertexDeclaration
// so the existing Sync + LookupVertexDeclarationAlias auto-create path resolves the
// input layout. declHandle==0 or non-declaration values are harmless: Sync early-returns
// on 0, and the alias fallback validates element count (1..64) before building.
void Fm2SetActivePassId(uint32_t renderContext, uint32_t passId) {
  g_origSetActivePassId(renderContext, passId);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr) {
    return;
  }
  static std::atomic<uint32_t> s_n{0};
  if (s_n.fetch_add(1, std::memory_order_relaxed) < 16) {
    LogReplayDbg("FM2_SET_PASSID renderContext=0x%08X passId=0x%08X", renderContext,
                 passId);
  }
  device->vertexDeclaration = passId;
}

void Fm2DrawIndexedVertices(GuestDevice *device, uint32_t primType,
                            int32_t baseVertexIndex, uint32_t startIndex,
                            uint32_t indexCount) {
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=Fm2DrawIndexedVertices n={} "
                "device={} prim={} base={} start={} indices={}",
                n, static_cast<const void *>(device), primType, baseVertexIndex,
                startIndex, indexCount);
  }
  Fm2RememberGuestDevice(device);
  DrawIndexedVertices(device, primType, baseVertexIndex, startIndex, indexCount);
}

void Fm2DrawVerticesUP(GuestDevice *device, uint32_t primType,
                       uint32_t vertexCount, void *data, uint32_t stride) {
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=Fm2DrawVerticesUP n={} device={} "
                "prim={} vertices={} data={} stride={}",
                n, static_cast<const void *>(device), primType, vertexCount,
                data, stride);
  }
  Fm2RememberGuestDevice(device);
  DrawVerticesUP(device, primType, vertexCount, data, stride);
}

void Swap(GuestDevice *device, rr::GuestBaseTexture *frontBuffer,
          void * /*params*/) {
  static uint32_t s_traceCount = 0;
  const bool isFm2 = rr::IsFm2Resource(frontBuffer);
  const bool hasTexture = frontBuffer != nullptr && frontBuffer->texture != nullptr;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=Swap n={} device={} front={} "
                "isFm2={} hasTexture={} size={}x{} format={}",
                n, static_cast<const void *>(device),
                static_cast<const void *>(frontBuffer), isFm2, hasTexture,
                frontBuffer ? frontBuffer->width : 0,
                frontBuffer ? frontBuffer->height : 0,
                frontBuffer ? int(frontBuffer->format) : 0);
  }
  FlushImmediateVertices();
  static bool s_ringSeeded = false;
  if (!s_ringSeeded && device != nullptr) {
    KickOff(device);
    s_ringSeeded = true;
  }
  if (rr::IsFm2Resource(frontBuffer) && frontBuffer->texture != nullptr) {
    rr::SetImplicitRenderTarget(frontBuffer);
    rr::SetPresentSource(frontBuffer);
  }
  Video::Present();
}

// Neutralize these two wait primitives
void BlockOnFence() {}
void BlockUntilIdle() {}

void SynchronizeToPresentationInterval(GuestDevice * /*device*/,
                                       uint32_t /*interval*/) {}

void SetPredication(GuestDevice * /*device*/, uint32_t /*predicationMask*/) {}

void SetShaderGPRAllocation(GuestDevice * /*device*/, uint32_t /*flags*/,
                            uint32_t /*vsGprs*/, uint32_t /*psGprs*/) {}

GuestBuffer *CreateVertexBuffer(uint32_t length, uint32_t /*usage*/,
                                uint32_t /*pool*/) {
  return rr::CreateVertexBuffer(length);
}
GuestBuffer *CreateIndexBuffer(uint32_t length, uint32_t /*usage*/,
                               uint32_t format) {
  return rr::CreateIndexBuffer(length, format);
}
GuestTexture *CreateTexture(uint32_t width, uint32_t height, uint32_t depth,
                            uint32_t levels, uint32_t usage, uint32_t format,
                            uint32_t pool, uint32_t type) {
  return rr::CreateTexture(width, height, depth, levels, usage, format, pool,
                           type);
}
GuestSurface *CreateSurface(uint32_t width, uint32_t height, uint32_t format,
                            uint32_t multiSample) {
  return rr::CreateSurface(width, height, format, multiSample);
}
GuestVertexDeclaration *
CreateVertexDeclaration(rr::GuestVertexElement *elements) {
  return rr::CreateVertexDeclaration(elements);
}
GuestShader *CreateVertexShader(const uint32_t *function) {
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=CreateVertexShader n={} function={}",
                n, static_cast<const void *>(function));
  }
  return rr::CreateVertexShader(function);
}
GuestShader *CreatePixelShader(const uint32_t *function) {
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=CreatePixelShader n={} function={}",
                n, static_cast<const void *>(function));
  }
  return rr::CreatePixelShader(function);
}


uint32_t D3DXCreateTextureFromFileInMemoryEx(
    void * /*device*/, const uint8_t *srcData, uint32_t srcSize, uint32_t /*w*/,
    uint32_t /*h*/, uint32_t /*mips*/, uint32_t /*usage*/, uint32_t /*format*/,
    uint32_t /*pool*/, uint32_t /*filter*/, uint32_t /*mipFilter*/,
    uint32_t /*colorKey*/, void * /*srcInfo*/, void * /*palette*/,
    rex::be<uint32_t> *ppTexture) {
  GuestTexture *texture = rr::LoadTextureFromMemory(srcData, srcSize);
  if (ppTexture)
    *ppTexture = ghp::ToGuest(texture);
  return 0; // S_OK
}

uint32_t D3DXCreateTextureFromFileInMemory(void * /*device*/,
                                           const uint8_t *srcData,
                                           uint32_t srcSize,
                                           rex::be<uint32_t> *ppTexture) {
  GuestTexture *texture = rr::LoadTextureFromMemory(srcData, srcSize);
  if (ppTexture)
    *ppTexture = ghp::ToGuest(texture);
  return 0; // S_OK
}

uint32_t VertexBufferLock(GuestBuffer *buffer, uint32_t offset, uint32_t size,
                          uint32_t flags);
uint32_t IndexBufferLock(GuestBuffer *buffer, uint32_t offset, uint32_t size,
                         uint32_t flags);

void SurfaceLockRect(GuestTexture *texture, GuestLockedRect *lockedRect,
                     void *rect, uint32_t flags) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origSurfaceLockRect(texture, lockedRect, rect, flags);
    return;
  }
  if (!rr::IsFm2Resource(texture))
    return; // genuine guest surface: ignore
  uint32_t pitch = 0, bits = 0;
  rr::LockTextureRect(texture, &pitch, &bits);
  if (lockedRect) {
    lockedRect->pitch = pitch;
    lockedRect->bits = bits;
  }
}

// Guest D3DLOCKED_TAIL { INT RowPitch; INT SlicePitch; void* pBits; } (BE).
struct GuestLockedTail {
  rex::be<int32_t> rowPitch;
  rex::be<int32_t> slicePitch;
  rex::be<uint32_t> bits;
};


uint32_t VertexBufferLock(GuestBuffer *buffer, uint32_t offset, uint32_t size,
                          uint32_t flags) {
  if (!ShouldMirrorPlumeRenderState()) {
    return g_origVertexBufferLock(buffer, offset, size, flags);
  }
  if (rr::IsFm2Resource(buffer))
    return rr::LockVertexBuffer(buffer, flags);
  if (GuestBuffer *native = rr::LookupBufferAlias(ghp::ToGuest(buffer)))
    return rr::LockVertexBuffer(native, flags);
  return g_origVertexBufferLock(buffer, offset, size, flags);
}
uint32_t IndexBufferLock(GuestBuffer *buffer, uint32_t offset, uint32_t size,
                         uint32_t flags) {
  if (!ShouldMirrorPlumeRenderState()) {
    return g_origIndexBufferLock(buffer, offset, size, flags);
  }
  if (rr::IsFm2Resource(buffer))
    return rr::LockIndexBuffer(buffer, flags);
  if (GuestBuffer *native = rr::LookupBufferAlias(ghp::ToGuest(buffer)))
    return rr::LockIndexBuffer(native, flags);
  return g_origIndexBufferLock(buffer, offset, size, flags);
}

void XGSetVertexDeclaration(rr::GuestVertexElement *elements,
                            void *guestDeclaration) {
  rr::RegisterVertexDeclarationAlias(ghp::ToGuest(guestDeclaration),
                                     rr::CreateVertexDeclaration(elements));
}


struct GuestSurfaceDesc {
  rex::be<uint32_t> format;
  rex::be<uint32_t> type;
  rex::be<uint32_t> usage;
  rex::be<uint32_t> pool;
  rex::be<uint32_t> multiSampleType;
  rex::be<uint32_t> multiSampleQuality;
  rex::be<uint32_t> width;
  rex::be<uint32_t> height;
};

void BaseTextureLockTail(GuestTexture *texture, uint32_t arrayIndex,
                         GuestLockedTail *locked, uint32_t flags) {
  if (!rr::IsFm2Resource(texture)) {
    return;
  }
  if (locked == nullptr)
    return;
  uint32_t pitch = 0, bits = 0;
  rr::LockTextureRect(texture, &pitch, &bits);
  locked->rowPitch = int32_t(pitch);
  locked->slicePitch = int32_t(pitch * texture->height);
  locked->bits = bits;
}

void LockSurface(GuestTexture *texture, uint32_t arrayIndex, uint32_t level,
                 uint32_t flags, rex::be<uint32_t> *ppData,
                 rex::be<uint32_t> *pRowPitch, rex::be<uint32_t> *pSlicePitch,
                 rex::be<uint32_t> *pTailOffset) {
  if (!rr::IsFm2Resource(texture)) {
    return;
  }
  uint32_t pitch = 0, bits = 0;
  rr::LockTextureRect(texture, &pitch, &bits);
  if (ppData)
    *ppData = bits;
  if (pRowPitch)
    *pRowPitch = pitch;
  if (pSlicePitch)
    *pSlicePitch = pitch * texture->height;
  if (pTailOffset)
    *pTailOffset = 0;
}

void UnlockResourceHook(rr::GuestResource *resource, uint32_t base,
                        uint32_t mip) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origUnlockResource(resource, base, mip);
    return;
  }
  if (rr::IsFm2Resource(resource)) {
    switch (resource->type) {
    case rr::ResourceType::VertexBuffer:
      rr::UnlockVertexBuffer(static_cast<GuestBuffer *>(resource)); break;
    case rr::ResourceType::IndexBuffer:
      rr::UnlockIndexBuffer(static_cast<GuestBuffer *>(resource)); break;
    case rr::ResourceType::Texture:
    case rr::ResourceType::VolumeTexture:
      rr::UnlockTextureRect(static_cast<GuestTexture *>(resource)); break;
    default: break;
    }
    return;
  }
  // XDK resource aliased by a creation hook: upload the staging data.
  if (GuestBuffer *native = rr::LookupBufferAlias(ghp::ToGuest(resource))) {
    if (native->type == rr::ResourceType::VertexBuffer)
      rr::UnlockVertexBuffer(native);
    else
      rr::UnlockIndexBuffer(native);
  }
}

// ---------------------------------------------------------------------------
// State setters.
// ---------------------------------------------------------------------------

void SetTexture(GuestDevice *device, uint32_t sampler, GuestTexture *texture,
                uint64_t /*mask*/) {
  FlushImmediateVertices();
  GuestTexture *reo = AsFm2(texture);
  if (reo == nullptr && texture != nullptr)
    reo = rr::TranslateGuestTexture(texture,
                                    true); 
  if (reo == nullptr && texture != nullptr) {
    // Binding an untranslatable texture samples as zeros (black) -- warn so
    // dropped bindings are attributable instead of silent.
    static std::unordered_set<const void *> s_warned;
    if (s_warned.insert(texture).second) {
      REXGPU_WARN("SetTexture: slot {} texture {} untranslatable -- bound null",
                  sampler, static_cast<const void *>(texture));
    }
  }
  rr::SetTexture(device, sampler, reo);
}
GuestShader *ResolveShader(GuestShader *shader) {
  if (rr::IsFm2Resource(shader))
    return shader;
  return rr::LookupShaderAlias(ghp::ToGuest(shader));
}

GuestShader *ResolveFXeShader(void *fxeShader) {
  if (fxeShader == nullptr)
    return nullptr;
  if (auto *reoShader = AsFm2(static_cast<GuestShader *>(fxeShader)))
    return reoShader;
  if (auto *shader = rr::LookupShaderAlias(ghp::ToGuest(fxeShader)))
    return shader;
  auto *d3dShader = reinterpret_cast<rex::be<uint32_t> *>(
      static_cast<uint8_t *>(fxeShader) + 8);
  return rr::LookupShaderAlias(d3dShader->get());
}

GuestVertexDeclaration *ResolveVertexDeclaration(void *declaration) {
  auto *reoDecl = AsFm2(static_cast<GuestVertexDeclaration *>(declaration));
  if (reoDecl != nullptr)
    return reoDecl;
  return rr::LookupVertexDeclarationAlias(ghp::ToGuest(declaration));
}

struct BoundShaderStateInfo {
  GuestVertexDeclaration *declaration = nullptr;
  GuestShader *vertexShader = nullptr;
  GuestShader *pixelShader = nullptr;
};

struct BoundShaderStateKey {
  uint32_t vertexDeclaration = 0;
  uint32_t vertexShader = 0;
  uint32_t pixelShader = 0;
  std::array<uint8_t, 16> streamStrides{};

  bool operator==(const BoundShaderStateKey &) const = default;
};

struct BoundShaderStateKeyHash {
  size_t operator()(const BoundShaderStateKey &key) const {
    uint64_t hash = 1469598103934665603ull;
    auto mix = [&](uint32_t value) {
      hash ^= value;
      hash *= 1099511628211ull;
    };
    mix(key.vertexDeclaration);
    mix(key.vertexShader);
    mix(key.pixelShader);
    for (uint8_t stride : key.streamStrides)
      mix(stride);
    return static_cast<size_t>(hash);
  }
};

struct GuestBoundShaderState {
  rex::be<uint32_t> reserved = 0;
  rex::be<uint32_t> refCount = 1;
  rex::be<uint32_t> vertexShader = 0;
  rex::be<uint32_t> vertexDeclaration = 0;
};

std::unordered_map<uint32_t, BoundShaderStateInfo> g_boundShaderStates;
std::unordered_map<BoundShaderStateKey, uint32_t, BoundShaderStateKeyHash>
    g_boundShaderStateCache;

uint32_t ReadGuestU32(const void *p) {
  return p ? reinterpret_cast<const rex::be<uint32_t> *>(p)->get() : 0;
}

bool IsReadableGuestRange(uint32_t guestAddress, uint32_t byteCount) {
  if (guestAddress == 0 || byteCount == 0)
    return false;

  const uint32_t lastAddress = guestAddress + byteCount - 1u;
  if (lastAddress < guestAddress)
    return false;

  auto *memory = ghp::GuestMemory();
  auto *heap = memory ? memory->LookupHeap(guestAddress) : nullptr;
  if (heap == nullptr)
    return false;

  const rex::memory::PageAccess access =
      heap->QueryRangeAccess(guestAddress, lastAddress);
  return access != rex::memory::PageAccess::kNoAccess;
}

uint32_t ReadGuestU32At(uint32_t guestAddress) {
  if (!IsReadableGuestRange(guestAddress, sizeof(uint32_t)))
    return 0;
  return ReadGuestU32(ghp::ToHost<const void>(guestAddress));
}

uint32_t ReadShaderContainerByteCount(uint32_t shaderContainer) {
  if (!IsReadableGuestRange(shaderContainer, 12u))
    return 0;

  const uint32_t headerBytes = ReadGuestU32At(shaderContainer + 4u);
  const uint32_t payloadBytes = ReadGuestU32At(shaderContainer + 8u);
  if (headerBytes < 12u || payloadBytes == 0)
    return 0;

  const uint32_t totalBytes = headerBytes + payloadBytes;
  if (totalBytes < headerBytes || totalBytes > 0x40000u)
    return 0;
  if (!IsReadableGuestRange(shaderContainer, totalBytes))
    return 0;
  return totalBytes;
}

GuestShader *RegisterShaderAliasFromContainer(const char *hook,
                                              uint32_t guestShaderObject,
                                              uint32_t shaderContainer,
                                              bool vertexShader) {
  const uint32_t byteCount = ReadShaderContainerByteCount(shaderContainer);
  static uint32_t s_traceCount = 0;
  if (byteCount == 0) {
    if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
      REXGPU_WARN("FM2_PLUME_RENDER_HOOK hook={} n={} shader=0x{:08X} "
                  "container=0x{:08X} invalid shader container",
                  hook, n, guestShaderObject, shaderContainer);
    }
    return nullptr;
  }

  const auto *function = ghp::ToHost<const uint32_t>(shaderContainer);
  GuestShader *shader = vertexShader ? rr::CreateVertexShader(function)
                                     : rr::CreatePixelShader(function);
  rr::RegisterShaderAlias(guestShaderObject, shader);

  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    const uint64_t hash = XXH3_64bits(function, byteCount);
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook={} n={} shader=0x{:08X} "
                "container=0x{:08X} bytes=0x{:X} type={} hash=0x{:016X} "
                "alias=0x{:08X}",
                hook, n, guestShaderObject, shaderContainer, byteCount,
                vertexShader ? "vertex" : "pixel", hash, ghp::ToGuest(shader));
  }
  return shader;
}

bool IsPlausibleShaderPayloadByteCount(uint32_t byteCount) {
  return byteCount >= nr::kXenosUcodeInstructionByteStride &&
         byteCount <= 0x10000u && (byteCount & 3u) == 0;
}

uint32_t ChooseShaderPayloadByteCount(uint32_t guestEndianSizeField) {
  if (IsPlausibleShaderPayloadByteCount(guestEndianSizeField))
    return guestEndianSizeField;

  const uint32_t littleEndianCandidate =
      nr::DirectDrawLittleEndianValueFromGuestDword(guestEndianSizeField);
  if (IsPlausibleShaderPayloadByteCount(littleEndianCandidate))
    return littleEndianCandidate;

  return 0;
}

bool WriteBinaryFile(const std::filesystem::path &path, const void *data,
                     size_t size) {
  try {
    std::filesystem::create_directories(path.parent_path());
    FILE *f = nullptr;
#if defined(_MSC_VER)
    if (fopen_s(&f, path.string().c_str(), "wb") != 0)
      f = nullptr;
#else
    f = std::fopen(path.string().c_str(), "wb");
#endif
    if (f != nullptr) {
      const bool ok = std::fwrite(data, 1, size, f) == size;
      std::fclose(f);
      return ok;
    }
  } catch (const std::filesystem::filesystem_error &e) {
    REXGPU_WARN("FM2 shader resource dump path error: {}", e.what());
  }
  return false;
}

void DumpLoadedShaderResource(const char *kind, uint32_t outRef,
                              uint32_t shaderId, uint32_t payloadBaseOffset,
                              uint32_t ucodeOffset) {
  if (!REXCVAR_GET(fm2_shader_resource_dump))
    return;

  const uint32_t shaderResource = ReadGuestU32At(outRef);
  if (shaderResource == 0) {
    REXGPU_WARN(
        "FM2 {} shader resource id=0x{:08X}: null/unreadable output ref "
        "0x{:08X}",
        kind, shaderId, outRef);
    return;
  }

  if (!IsReadableGuestRange(shaderResource, 0x4Cu)) {
    REXGPU_WARN(
        "FM2 {} shader resource id=0x{:08X}: unreadable resource 0x{:08X}",
        kind, shaderId, shaderResource);
    return;
  }

  uint32_t shaderObject = shaderResource;
  uint32_t payloadBase = ReadGuestU32At(shaderObject + payloadBaseOffset);
  uint32_t resolvedObject = 0;
  if (payloadBase == 0) {
    resolvedObject =
        ReadGuestU32At(shaderResource + nr::kDirectDrawStateHandleResolvedObjectOffset);
    if (resolvedObject != 0 && IsReadableGuestRange(resolvedObject, 0x40u)) {
      const uint32_t resolvedPayloadBase =
          ReadGuestU32At(resolvedObject + payloadBaseOffset);
      if (resolvedPayloadBase != 0) {
        shaderObject = resolvedObject;
        payloadBase = resolvedPayloadBase;
      }
    }
  }

  if (payloadBase == 0) {
    REXGPU_WARN(
        "FM2 {} shader resource id=0x{:08X}: null payload field +0x{:02X} "
        "resource=0x{:08X} resolved=0x{:08X}",
        kind, shaderId, payloadBaseOffset, shaderResource, resolvedObject);
    return;
  }

  const uint32_t sizeField =
      ReadGuestU32At(shaderObject + nr::kDirectDrawPixelShaderPayloadByteCountOffset);
  const uint32_t knownPayloadBytes = ChooseShaderPayloadByteCount(sizeField);
  uint32_t payloadDumpBytes = nr::BoundedShaderPayloadDumpByteCount(
      nr::kDirectDrawShaderByteDumpMax, knownPayloadBytes);
  if (!IsReadableGuestRange(payloadBase, payloadDumpBytes)) {
    const uint32_t smallerPayloadDumpBytes =
        nr::BoundedShaderPayloadDumpByteCount(256u, knownPayloadBytes);
    if (smallerPayloadDumpBytes != payloadDumpBytes &&
        IsReadableGuestRange(payloadBase, smallerPayloadDumpBytes)) {
      payloadDumpBytes = smallerPayloadDumpBytes;
    }
  }

  if (payloadDumpBytes == 0 || !IsReadableGuestRange(payloadBase, payloadDumpBytes)) {
    REXGPU_WARN(
        "FM2 {} shader resource id=0x{:08X}: invalid payload 0x{:08X} "
        "object=0x{:08X} knownBytes=0x{:X} sizeField=0x{:08X}",
        kind, shaderId, payloadBase, shaderObject, knownPayloadBytes, sizeField);
    return;
  }

  const auto *payloadHost = ghp::ToHost<const uint8_t>(payloadBase);
  const uint64_t payloadHash = XXH3_64bits(payloadHost, payloadDumpBytes);
  static std::mutex s_dumpMutex;
  static std::unordered_set<uint64_t> s_dumpedPayloads;
  {
    std::lock_guard lock(s_dumpMutex);
    if (s_dumpedPayloads.contains(payloadHash))
      return;
    const uint32_t limit = REXCVAR_GET(fm2_shader_resource_dump_limit);
    if (limit != 0 && s_dumpedPayloads.size() >= limit)
      return;
    s_dumpedPayloads.insert(payloadHash);
  }

  const uint32_t ucodeDumpBytes = nr::BoundedShaderUcodeDumpByteCount(
      payloadDumpBytes, knownPayloadBytes, ucodeOffset);
  nr::DirectDrawShaderUcodeCandidate candidate;
  const uint32_t ucodeBase = payloadBase + ucodeOffset;
  const bool canScanUcode =
      ucodeBase >= payloadBase &&
      IsReadableGuestRange(ucodeBase, ucodeDumpBytes) &&
      ucodeDumpBytes >=
          nr::kXenosUcodeControlFlowPairDwordCount * sizeof(uint32_t);
  if (canScanUcode) {
    const auto *ucodeHost = ghp::ToHost<const uint8_t>(ucodeBase);
    candidate = nr::FindXenosUcodeCandidate(
        reinterpret_cast<const uint32_t *>(ucodeHost), ucodeDumpBytes / 4u);
  }

  const std::filesystem::path dumpDir =
      std::filesystem::path("missed_shaders") / "fm2_shader_resources";
  char payloadName[128];
  std::snprintf(payloadName, sizeof(payloadName),
                "%s_%08X_%016llX_payload.bin", kind, shaderId,
                static_cast<unsigned long long>(payloadHash));
  const std::filesystem::path payloadPath = dumpDir / payloadName;
  const bool wrotePayload =
      WriteBinaryFile(payloadPath, payloadHost, payloadDumpBytes);

  bool wroteUcode = false;
  uint64_t ucodeHash = 0;
  std::filesystem::path ucodePath;
  if (candidate.valid) {
    const uint32_t candidateByteOffset = ucodeOffset + candidate.byte_offset;
    const auto *candidateHost = payloadHost + candidateByteOffset;
    ucodeHash = XXH3_64bits(candidateHost, candidate.bounds.total_used_bytes);
    char ucodeName[128];
    std::snprintf(ucodeName, sizeof(ucodeName),
                  "%s_%08X_%016llX_ucode_o%04X.bin", kind, shaderId,
                  static_cast<unsigned long long>(ucodeHash),
                  candidateByteOffset);
    ucodePath = dumpDir / ucodeName;
    wroteUcode = WriteBinaryFile(ucodePath, candidateHost,
                                 candidate.bounds.total_used_bytes);
  }

  REXGPU_WARN(
      "FM2 {} shader resource captured id=0x{:08X} resource=0x{:08X} "
      "object=0x{:08X} payload=0x{:08X} sizeField=0x{:08X} knownBytes=0x{:X} "
      "dumpBytes=0x{:X} payloadHash=0x{:016X} candidate={} "
      "candidateOffset=0x{:X} candidateBytes=0x{:X} ucodeHash=0x{:016X} "
      "payloadFile={} ucodeFile={}",
      kind, shaderId, shaderResource, shaderObject, payloadBase, sizeField,
      knownPayloadBytes, payloadDumpBytes, payloadHash, candidate.valid,
      candidate.byte_offset, candidate.valid ? candidate.bounds.total_used_bytes : 0u,
      ucodeHash, wrotePayload ? payloadPath.string() : "<write-failed>",
      wroteUcode ? ucodePath.string() : "<none>");
}

uint32_t Fm2AllocGpuPassMemoryBlock(uint32_t shaderContainer) {
  const uint32_t shaderObject =
      g_origFm2AllocGpuPassMemoryBlock(shaderContainer);
  if (!ShouldMirrorPlumeRenderState()) {
    return shaderObject;
  }
  if (shaderObject != 0) {
    RegisterShaderAliasFromContainer("AllocGpuPassMemoryBlock", shaderObject,
                                     shaderContainer, false);
  }
  return shaderObject;
}

uint32_t Fm2CreateGpuMemoryBlock(uint32_t shaderContainer) {
  const uint32_t shaderObject = g_origFm2CreateGpuMemoryBlock(shaderContainer);
  if (!ShouldMirrorPlumeRenderState()) {
    return shaderObject;
  }
  if (shaderObject != 0) {
    RegisterShaderAliasFromContainer("CreateGpuMemoryBlock", shaderObject,
                                     shaderContainer, true);
  }
  return shaderObject;
}

void WriteGuestU32(void *p, uint32_t value) {
  if (p)
    *reinterpret_cast<rex::be<uint32_t> *>(p) = value;
}

BoundShaderStateKey MakeBoundShaderStateKey(void *vertexDeclaration,
                                            void *streamStrides,
                                            void *vertexShader,
                                            void *pixelShader) {
  BoundShaderStateKey key;
  key.vertexDeclaration = ghp::ToGuest(vertexDeclaration);
  key.vertexShader = ghp::ToGuest(vertexShader);
  key.pixelShader = ghp::ToGuest(pixelShader);
  auto *strides = static_cast<const rex::be<uint32_t> *>(streamStrides);
  if (strides != nullptr) {
    for (size_t i = 0; i < key.streamStrides.size(); ++i)
      key.streamStrides[i] = static_cast<uint8_t>(strides[i].get());
  }
  return key;
}

GuestBoundShaderState *CreateBoundShaderStateResource(void * /*cache*/,
                                                      void *vertexDeclaration,
                                                      void *streamStrides,
                                                      void *vertexShader,
                                                      void *pixelShader) {
  const BoundShaderStateKey key = MakeBoundShaderStateKey(
      vertexDeclaration, streamStrides, vertexShader, pixelShader);
  auto cached = g_boundShaderStateCache.find(key);
  if (cached != g_boundShaderStateCache.end())
    return ghp::ToHost<GuestBoundShaderState>(cached->second);

  uint32_t guestAddress =
      ghp::GuestAllocRaw(sizeof(GuestBoundShaderState), 0x10);
  if (guestAddress == 0)
    return nullptr;

  auto *state = ghp::ToHost<GuestBoundShaderState>(guestAddress);
  new (state) GuestBoundShaderState();
  state->vertexShader = ghp::ToGuest(vertexShader);
  state->vertexDeclaration = ghp::ToGuest(vertexDeclaration);

  g_boundShaderStates[guestAddress] = {
      ResolveVertexDeclaration(vertexDeclaration),
      ResolveFXeShader(vertexShader),
      ResolveFXeShader(pixelShader),
  };
  g_boundShaderStateCache[key] = guestAddress;
  return state;
}

void RHICreateBoundShaderState(void *outRef, void *vertexDeclaration,
                               void *streamStrides, void *vertexShader,
                               void *pixelShader) {
  GuestBoundShaderState *state = CreateBoundShaderStateResource(
      nullptr, vertexDeclaration, streamStrides, vertexShader, pixelShader);
  WriteGuestU32(outRef, ghp::ToGuest(state));
  WriteGuestU32(static_cast<uint8_t *>(outRef) + 4, 0);
}

void *AssignBoundShaderStateRef(void *dst, const void *src) {
  WriteGuestU32(dst, ReadGuestU32(src));
  WriteGuestU32(static_cast<uint8_t *>(dst) + 4, 0);
  return dst;
}

void CopyConstructBoundShaderStateRef(void *dst, const void *src) {
  AssignBoundShaderStateRef(dst, src);
}

void ReleaseBoundShaderStateRef(void *ref) {
  WriteGuestU32(ref, 0);
  WriteGuestU32(static_cast<uint8_t *>(ref) + 4, 0);
}

void SetBoundShaderState(GuestDevice *device, void *boundStateRef) {
  FlushImmediateVertices();
  const uint32_t boundState = ReadGuestU32(boundStateRef);
  if (boundState == 0) {
    rr::SetVertexDeclaration(device, nullptr);
    rr::SetVertexShader(device, nullptr);
    rr::SetPixelShader(device, nullptr);
    return;
  }
  auto it = g_boundShaderStates.find(boundState);
  if (it == g_boundShaderStates.end()) {
    REXGPU_WARN("SetBoundShaderState: unknown bound state 0x{:08X}",
                boundState);
    rr::SetVertexDeclaration(device, nullptr);
    rr::SetVertexShader(device, nullptr);
    rr::SetPixelShader(device, nullptr);
    return;
  }

  const BoundShaderStateInfo &info = it->second;
  rr::SetVertexDeclaration(device, info.declaration);
  rr::SetVertexShader(device, info.vertexShader);
  rr::SetPixelShader(device, info.pixelShader);
}

void SetVertexShaderNative(GuestDevice *device, GuestShader *shader) {
  FlushImmediateVertices();
  rr::SetVertexShader(device, ResolveShader(shader));
}

void SetPixelShaderNative(GuestDevice *device, GuestShader *shader) {
  FlushImmediateVertices();
  rr::SetPixelShader(device, ResolveShader(shader));
}

void Fm2LoadPixelShaderResourceById(uint32_t outRef, uint32_t shaderId) {
  g_origFm2LoadPixelShaderResourceById(outRef, shaderId);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=LoadPixelShaderResource n={} "
                "outRef=0x{:08X} shaderId=0x{:08X} resource=0x{:08X}",
                n, outRef, shaderId, ReadGuestU32At(outRef));
  }
  DumpLoadedShaderResource(
      "pixel", outRef, shaderId,
      nr::kDirectDrawPixelShaderPayloadGpuBaseOffset,
      nr::kDirectDrawPixelShaderPayloadUcodeOffset);
}

void Fm2LoadVertexShaderResourceById(uint32_t outRef, uint32_t shaderId) {
  g_origFm2LoadVertexShaderResourceById(outRef, shaderId);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=LoadVertexShaderResource n={} "
                "outRef=0x{:08X} shaderId=0x{:08X} resource=0x{:08X}",
                n, outRef, shaderId, ReadGuestU32At(outRef));
  }
  DumpLoadedShaderResource(
      "vertex", outRef, shaderId,
      nr::kDirectDrawVertexShaderPayloadGpuBaseOffset,
      nr::kDirectDrawVertexShaderPayloadUcodeOffset);
}

void SetVertexShaderConstantFN(GuestDevice *device, uint32_t startRegister,
                               const uint32_t *data, uint32_t vector4fCount) {
  FlushImmediateVertices();
  if (device == nullptr || data == nullptr || startRegister >= 0x100)
    return;
  const uint32_t count = std::min(vector4fCount, 0x100u - startRegister);
  std::memcpy(device->vertexShaderFloatConstants + startRegister * 4, data,
              count * 16);
}

void SetPixelShaderConstantFN(GuestDevice *device, uint32_t startRegister,
                              const uint32_t *data, uint32_t vector4fCount) {
  FlushImmediateVertices();
  if (device == nullptr || data == nullptr || startRegister >= 0x100)
    return;
  const uint32_t count = std::min(vector4fCount, 0x100u - startRegister);
  std::memcpy(device->pixelShaderFloatConstants + startRegister * 4, data,
              count * 16);
}

void SetShaderConstantB(rex::be<uint32_t> *constants, uint32_t constantCount,
                        uint32_t startRegister, const uint32_t *data,
                        uint32_t boolCount) {
  if (constants == nullptr || data == nullptr ||
      startRegister >= constantCount * 32)
    return;
  const uint32_t count =
      std::min(boolCount, constantCount * 32 - startRegister);
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t bit = startRegister + i;
    uint32_t value = constants[bit / 32].get();
    const uint32_t mask = 1u << (bit & 31);
    if ((std::byteswap(data[i]) & 1u) != 0)
      value |= mask;
    else
      value &= ~mask;
    constants[bit / 32] = value;
  }
}

void SetVertexShaderConstantB(GuestDevice *device, uint32_t startRegister,
                              const uint32_t *data, uint32_t boolCount) {
  FlushImmediateVertices();
  if (device == nullptr)
    return;
  SetShaderConstantB(device->vertexShaderBoolConstants,
                     uint32_t(std::size(device->vertexShaderBoolConstants)),
                     startRegister, data, boolCount);
}

void SetPixelShaderConstantB(GuestDevice *device, uint32_t startRegister,
                             const uint32_t *data, uint32_t boolCount) {
  FlushImmediateVertices();
  if (device == nullptr)
    return;
  SetShaderConstantB(device->pixelShaderBoolConstants,
                     uint32_t(std::size(device->pixelShaderBoolConstants)),
                     startRegister, data, boolCount);
}

void SetShaderConstantI(rex::be<uint32_t> *constants, uint32_t constantCount,
                        uint32_t startRegister, const uint32_t *data,
                        uint32_t intCount) {
  if (constants == nullptr || data == nullptr || startRegister >= constantCount)
    return;
  const uint32_t count = std::min(intCount, constantCount - startRegister);
  for (uint32_t i = 0; i < count; ++i) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data + i * 4);
    constants[startRegister + i] = (uint32_t(bytes[11]) << 16) |
                                   (uint32_t(bytes[7]) << 8) |
                                   uint32_t(bytes[3]);
  }
}

void SetVertexShaderConstantI(GuestDevice *device, uint32_t startRegister,
                              const uint32_t *data, uint32_t intCount) {
  FlushImmediateVertices();
  if (device == nullptr)
    return;
  SetShaderConstantI(device->vertexShaderIntConstants,
                     uint32_t(std::size(device->vertexShaderIntConstants)),
                     startRegister, data, intCount);
}

void SetPixelShaderConstantI(GuestDevice *device, uint32_t startRegister,
                             const uint32_t *data, uint32_t intCount) {
  FlushImmediateVertices();
  if (device == nullptr)
    return;
  SetShaderConstantI(device->pixelShaderIntConstants,
                     uint32_t(std::size(device->pixelShaderIntConstants)),
                     startRegister, data, intCount);
}

struct GuestD3DVertexBufferHeader {
  rex::be<uint32_t> common; // type 1 in the low nibble
  rex::be<uint32_t> refCount;
  rex::be<uint32_t> fence;
  rex::be<uint32_t> readFence;
  rex::be<uint32_t> identifier;
  rex::be<uint32_t> baseFlush;
  rex::be<uint32_t> format0; // byteAddress | 3 (vertex fetch constant)
  rex::be<uint32_t> format1; // length in bits 2..25, endian bits low
};

struct GuestD3DIndexBufferHeader {
  rex::be<uint32_t> common; // type 2 in the low nibble, D3DFORMAT << 29
  rex::be<uint32_t> refCount;
  rex::be<uint32_t> fence;
  rex::be<uint32_t> readFence;
  rex::be<uint32_t> identifier;
  rex::be<uint32_t> baseFlush;
  rex::be<uint32_t> address; // guest byte address of the index data
  rex::be<uint32_t> size;    // byte size
};

void SetStreamSourceNative(GuestDevice *device, uint32_t stream,
                           GuestBuffer *buffer, uint32_t offset,
                           uint32_t stride, uint64_t /*mask*/) {
  FlushImmediateVertices();
  GuestBuffer *reo = AsFm2(buffer);
  if (reo == nullptr && buffer != nullptr) {
    const auto *header =
        reinterpret_cast<const GuestD3DVertexBufferHeader *>(buffer);
    if ((header->common.get() & 0xF) == 1) {
      const uint32_t address = header->format0.get() & ~3u;
      const uint32_t size = header->format1.get() & 0x3FFFFFCu;
      if (address != 0 && size != 0 && offset < size) {
        rr::SetStreamSourceGuestData(device, stream,
                                     ghp::ToHost<void>(address + offset),
                                     size - offset, stride);
        return;
      }
    }
  }
  rr::SetStreamSource(device, stream, reo, offset, stride);
}
void SetIndicesNative(GuestDevice *device, GuestBuffer *buffer) {
  FlushImmediateVertices();
  GuestBuffer *reo = AsFm2(buffer);
  if (reo == nullptr && buffer != nullptr) {
    const auto *header =
        reinterpret_cast<const GuestD3DIndexBufferHeader *>(buffer);
    if ((header->common.get() & 0xF) == 2) {
      const uint32_t address = header->address.get();
      const uint32_t size = header->size.get();
      // D3DFORMAT in the top 3 bits: 1 = INDEX16, 6 = INDEX32.
      const uint32_t indexStride = (header->common.get() >> 29) == 6 ? 4 : 2;
      if (address != 0 && size != 0) {
        rr::SetIndicesGuestData(device, ghp::ToHost<void>(address), size,
                                indexStride);
        return;
      }
    }
  }
  rr::SetIndices(device, reo);
}
void SetViewport(GuestDevice *device, rr::GuestViewport *viewport) {
  FlushImmediateVertices();
  rr::SetViewport(device, viewport);
}
void SetScissorRect(GuestDevice *device, rr::GuestRect *rect) {
  FlushImmediateVertices();
  rr::SetScissorRect(device, rect);
}
void SetRenderTargetNative(GuestDevice *device, uint32_t index,
                           GuestSurface *surface) {
  FlushImmediateVertices();
  rr::GuestBaseTexture *reo = AsFm2(surface);
  if (reo == nullptr && surface != nullptr)
    reo = rr::TranslateGuestSurface(
        surface); // UE3 RHI-built (XGSetSurfaceHeader) surface
  rr::SetRenderTarget(device, index, reo);
}
void SetDepthStencilSurface(GuestDevice *device, GuestSurface *surface) {
  FlushImmediateVertices();
  GuestSurface *reo = AsFm2(surface);
  if (reo == nullptr && surface != nullptr) {
    rr::GuestBaseTexture *translated = rr::TranslateGuestSurface(surface);
    if (translated != nullptr &&
        translated->type == rr::ResourceType::DepthStencil) {
      reo = static_cast<GuestSurface *>(translated);
    } else if (translated != nullptr) {
      static std::unordered_set<const void *> s_warned;
      if (s_warned.insert(surface).second) {
        REXGPU_WARN("SetDepthStencilSurface: {} translated to non-depth "
                    "resource (type {}) -- bound null",
                    (const void *)surface, int(translated->type));
      }
    }
  }
  rr::SetDepthStencilSurface(device, reo);
}

// ?RHISetDepthState@@YAXPAVFD3DDepthState@@@Z

void RHISetDepthState(GuestDevice *device, void *depthStateGuest) {
  FlushImmediateVertices();

  const auto *ds = reinterpret_cast<const rex::be<uint32_t> *>(depthStateGuest);
  if (ds == nullptr)
    return;
  const uint32_t zEnable = ds[1].get();
  const uint32_t zWrite = ds[2].get();
  const uint32_t cmpFunc = ds[3].get();

  rr::SetDepthState(zEnable, zWrite, cmpFunc);
}

void RHISetStencilState(GuestDevice *device, void *stencilStateGuest) {
  FlushImmediateVertices();

  const auto *ss =
      reinterpret_cast<const rex::be<uint32_t> *>(stencilStateGuest);
  if (ss == nullptr)
    return;

  rr::GuestStencilState s;
  s.enable = ss[1].get() != 0;
  s.twoSided = ss[2].get() != 0;
  s.frontFunc = ss[3].get();
  s.frontFail = ss[4].get();
  s.frontDepthFail = ss[5].get();
  s.frontPass = ss[6].get();
  s.backFunc = ss[7].get();
  s.backFail = ss[8].get();
  s.backDepthFail = ss[9].get();
  s.backPass = ss[10].get();
  s.readMask = ss[11].get();
  s.writeMask = ss[12].get();
  s.ref = ss[13].get();
  rr::SetStencilState(s);
}


uint32_t RHIGetOcclusionQueryResult(void * /*query*/, void *outNumPixels,
                                    uint32_t /*bWait*/) {
  if (outNumPixels != nullptr)
    *reinterpret_cast<rex::be<uint32_t> *>(outNumPixels) = 0x00100000u;
  return 1; // S_OK: result available
}

// D3DDevice_ClearF(device, flags, rect, D3DVECTOR4* color, Z, d3dColor). Color
// is 4 big-endian floats in guest memory.
void ClearF(GuestDevice *device, uint32_t flags, void * /*rect*/,
            const uint32_t *color, float z, uint32_t /*d3dColor*/) {
  FlushImmediateVertices();
  float rgba[4] = {0, 0, 0, 0};
  if (color) {
    for (int i = 0; i < 4; ++i) {
      uint32_t bits = std::byteswap(color[i]);
      rgba[i] = std::bit_cast<float>(bits);
    }
  }
  rr::Clear(device, flags, rgba, z);
}

void Resolve(GuestDevice *device, uint32_t flags, const rr::GuestRect *source,
             GuestBaseTexture *destination, const rr::GuestPoint *destPoint,
             uint32_t /*destLevel*/, uint32_t /*destSliceOrFace*/,
             const void * /*clearColor*/, float /*clearZ*/,
             uint32_t /*clearStencil*/, const void * /*parameters*/) {
  FlushImmediateVertices();

  GuestBaseTexture *reo = AsFm2(destination);
  if (reo == nullptr && destination != nullptr) {
    reo = rr::TranslateGuestTexture(destination, false);
  }
  if (reo == nullptr && destination != nullptr) {
    static std::unordered_set<const void *> s_warned;
    if (s_warned.insert(destination).second) {
      REXGPU_WARN("Resolve: destination {} untranslatable -- resolve DROPPED",
                  static_cast<const void *>(destination));
    }
  }
  rr::StretchRect(device, flags, source, reo, destPoint);
}

#define RENDER_STATE_HOOK(fn, d3drs)                                           \
  void fn(GuestDevice *device, uint32_t value) {                               \
    FlushImmediateVertices();                                                  \
    rr::SetRenderState(device, rr::d3drs, value);                              \
  }
RENDER_STATE_HOOK(RsAlphaBlendEnable, D3DRS_ALPHABLENDENABLE)
RENDER_STATE_HOOK(RsAlphaTestEnable, D3DRS_ALPHATESTENABLE)
RENDER_STATE_HOOK(RsBlendOp, D3DRS_BLENDOP)
RENDER_STATE_HOOK(RsBlendOpAlpha, D3DRS_BLENDOPALPHA)
RENDER_STATE_HOOK(RsColorWriteEnable, D3DRS_COLORWRITEENABLE)
RENDER_STATE_HOOK(RsDepthBias, D3DRS_DEPTHBIAS)
RENDER_STATE_HOOK(RsDestBlend, D3DRS_DESTBLEND)
RENDER_STATE_HOOK(RsDestBlendAlpha, D3DRS_DESTBLENDALPHA)
RENDER_STATE_HOOK(RsSlopeScaleDepthBias, D3DRS_SLOPESCALEDEPTHBIAS)
RENDER_STATE_HOOK(RsSrcBlend, D3DRS_SRCBLEND)
RENDER_STATE_HOOK(RsSrcBlendAlpha, D3DRS_SRCBLENDALPHA)
RENDER_STATE_HOOK(RsZEnable, D3DRS_ZENABLE)
#undef RENDER_STATE_HOOK

void MirrorFm2RenderState(uint32_t renderContext, uint32_t state,
                          uint32_t value) {
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  FlushImmediateVertices();
  rr::SetRenderState(device, state, value);
}

void MirrorFm2ClipPlanes(uint32_t renderContext) {
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  FlushImmediateVertices();
  rr::UpdateClipPlaneConstants(device);
}

void Fm2SetDepthStencilEnableState(uint32_t renderContext, uint32_t value) {
  g_origFm2SetDepthStencilEnableState(renderContext, value);
  MirrorFm2RenderState(renderContext, rr::D3DRS_ZENABLE, value);
}

void Fm2SetAlphaBlendEnableBits(uint32_t renderContext, uint32_t value) {
  g_origFm2SetAlphaBlendEnableBits(renderContext, value);
  MirrorFm2RenderState(renderContext, rr::D3DRS_ALPHABLENDENABLE, value);
}

void Fm2SetAlphaTestState(uint32_t renderContext, uint32_t value) {
  g_origFm2SetAlphaTestState(renderContext, value);
  MirrorFm2RenderState(renderContext, rr::D3DRS_ALPHATESTENABLE, value);
}

void Fm2SetDepthCompareBits(uint32_t renderContext, uint32_t value) {
  g_origFm2SetDepthCompareBits(renderContext, value);
  MirrorFm2RenderState(renderContext, rr::D3DRS_ZFUNC, value);
}

void Fm2SetColorWriteMaskBits(uint32_t renderContext, uint32_t value) {
  g_origFm2SetColorWriteMaskBits(renderContext, value);
  MirrorFm2RenderState(renderContext, rr::D3DRS_COLORWRITEENABLE, value);
}

void Fm2SetClipPlane0Enable(uint32_t renderContext, uint32_t value) {
  g_origFm2SetClipPlane0Enable(renderContext, value);
  MirrorFm2ClipPlanes(renderContext);
}

void Fm2SetClipPlane1Enable(uint32_t renderContext, uint32_t value) {
  g_origFm2SetClipPlane1Enable(renderContext, value);
  MirrorFm2ClipPlanes(renderContext);
}

void Fm2SetClipPlane2Enable(uint32_t renderContext, uint32_t value) {
  g_origFm2SetClipPlane2Enable(renderContext, value);
  MirrorFm2ClipPlanes(renderContext);
}

void Fm2SetClipPlane3Enable(uint32_t renderContext, uint32_t value) {
  g_origFm2SetClipPlane3Enable(renderContext, value);
  MirrorFm2ClipPlanes(renderContext);
}

void ApplyRasterizerState(GuestDevice *device, void *rasterizerStateGuest) {
  FlushImmediateVertices();

  if (rasterizerStateGuest == nullptr)
    return;
  const auto *rs =
      reinterpret_cast<const GuestRasterizerState *>(rasterizerStateGuest);
  rr::SetRenderState(device, rr::D3DRS_CULLMODE, rs->cullMode.get());
}

void SetColorWriteEnable(GuestDevice *device, uint32_t value) {
  FlushImmediateVertices();
  rr::SetRenderState(device, rr::D3DRS_COLORWRITEENABLE,
                     value != 0 ? 0xFu : 0u);
}

void SetZWriteEnable(GuestDevice *device, uint32_t value) {
  FlushImmediateVertices();
  rr::SetRenderState(device, rr::D3DRS_ZWRITEENABLE, value);
}

void SetCullMode(GuestDevice *device, uint32_t value) {
  FlushImmediateVertices();
  rr::SetRenderState(device, rr::D3DRS_CULLMODE, value);
}

void RsClipPlaneEnable(GuestDevice *device, uint32_t value) {
  FlushImmediateVertices();
  rr::UpdateClipPlaneConstants(device);
}

void RsViewportEnable(GuestDevice *device, uint32_t value) {
  FlushImmediateVertices();
  rr::SetViewportEnable(device, value);
}

void SetPendingClipPlanes(GuestDevice *device, uint64_t dirtyMask) {
  FlushImmediateVertices();
  rr::UpdateClipPlaneConstants(device);
}

void SurfaceGetDesc(GuestSurface *surface, GuestSurfaceDesc *desc) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origSurfaceGetDesc(surface, desc);
    return;
  }
  if (!rr::IsFm2Resource(surface)) {
    g_origSurfaceGetDesc(surface, desc);
    return;
  }
  if (desc == nullptr)
    return;
  desc->format = surface->guestFormat;
  desc->type = 1; // D3DRTYPE_SURFACE
  desc->usage = 0;
  desc->pool = 0;
  desc->multiSampleType = 0;
  desc->multiSampleQuality = 0;
  desc->width = surface->width;
  desc->height = surface->height;
}

void SetVertexDeclarationBind(GuestDevice *device,
                              GuestVertexDeclaration *decl) {
  FlushImmediateVertices();
  GuestVertexDeclaration *reoDecl = AsFm2(decl);
  if (reoDecl == nullptr)
    reoDecl = rr::LookupVertexDeclarationAlias(ghp::ToGuest(decl));
  device->vertexDeclaration = reoDecl ? ghp::ToGuest(reoDecl) : 0;
  device->dirtyFlags[2] = device->dirtyFlags[2].get() | 0x00080000;
  if (reoDecl != nullptr)
    rr::SetVertexDeclaration(device, reoDecl);
}
void *FXeVertexShaderInit(uint8_t *self, uint32_t *blob) {
  uint32_t d3dShader = *reinterpret_cast<rex::be<uint32_t> *>(self + 8);
  rr::RegisterShaderAlias(d3dShader, rr::CreateVertexShader(blob));
  return self;
}

void *FXePixelShaderInit(uint8_t *self, uint32_t *blob) {
  uint32_t d3dShader = *reinterpret_cast<rex::be<uint32_t> *>(self + 8);
  rr::RegisterShaderAlias(d3dShader, rr::CreatePixelShader(blob));
  return self;
}

void DrawVertices(GuestDevice *device, uint32_t primType, uint32_t startVertex,
                  uint32_t vertexCount) {
  FlushImmediateVertices();
  rr::DrawPrimitive(device, primType, startVertex, vertexCount);
}
void DrawIndexedVertices(GuestDevice *device, uint32_t primType,
                         int32_t baseVertexIndex, uint32_t startIndex,
                         uint32_t indexCount) {
  FlushImmediateVertices();
  g_drawsSinceLastPresent.fetch_add(1, std::memory_order_relaxed);
  rr::DrawIndexedPrimitive(device, primType, baseVertexIndex, startIndex,
                           indexCount);
}
void DrawVerticesUP(GuestDevice *device, uint32_t primType,
                    uint32_t vertexCount, void *data, uint32_t stride) {
  FlushImmediateVertices();
  rr::DrawPrimitiveUP(device, primType, vertexCount, data, stride);
}

void RHIDrawIndexedPrimitiveUP(GuestDevice *device, uint32_t primType,
                               uint32_t minVertexIndex, uint32_t numVertices,
                               uint32_t numPrimitives, const void *indexData,
                               uint32_t indexStride, const void *vertexData,
                               uint32_t vertexStride) {
  FlushImmediateVertices();
  if (device == nullptr || indexData == nullptr || vertexData == nullptr)
    return;
  uint32_t d3dPrimType = rr::D3DPT_TRIANGLELIST;
  switch (primType) {
  case 1:
    d3dPrimType = rr::D3DPT_TRIANGLEFAN;
    break;
  case 2:
    d3dPrimType = rr::D3DPT_TRIANGLESTRIP;
    break;
  case 3:
    d3dPrimType = rr::D3DPT_LINELIST;
    break;
  case 4:
    d3dPrimType = rr::D3DPT_QUADLIST;
    break;
  default:
    break;
  }
  rr::DrawIndexedPrimitiveUP(device, d3dPrimType, minVertexIndex, numVertices,
                             numPrimitives, indexData, indexStride, vertexData,
                             vertexStride);
}

// FM2 keeps the live color-write mask in the render context (== D3D CDevice) at
// offset 10420, bits 14-16 -- guest FM2_RenderContext_SetColorWriteMaskBits packs
// (value << 14) & 0x1C000. The PM4 draw emitters bypass the RenderContext Set*
// hook path, so the native renderer's mirrored colorWriteEnable goes stale
// (observed stuck at 0 -> every draw sanitized to no color attachment ->
// rejected NoAttachments). Read the mask live per draw and push it into the
// native mirror so the bound color RT is actually used.
void ApplyLiveColorWriteFromContext(GuestDevice *device, uint32_t context) {
  if (context == 0)
    return;
  const uint32_t rs = ReadGuestU32At(context + 10420u);
  const uint32_t cw = (rs >> 14) & 0x7u; // 3-bit RGB write-enable field
  static std::atomic<uint32_t> s_n{0};
  if (s_n.fetch_add(1, std::memory_order_relaxed) < 24) {
    LogReplayDbg("FM2_LIVE_COLORWRITE ctx=0x%08X rs=0x%08X cw=0x%X", context, rs,
                 cw);
  }
  // Any nonzero live mask means the game wants color output for this draw; map
  // to a full RGBA write so the color attachment is kept and written.
  rr::SetRenderState(device, rr::D3DRS_COLORWRITEENABLE, cw ? 0xFu : 0u);
}

// Forza keeps the per-sampler bound texture pointers at context+12264+4*slot
// (read by sub_823708D8 as *(ctx + 4*(slot+3066))). The PM4 emit path bypasses
// the standard D3D SetTexture, so g_textures stays empty and the pixel shaders
// sample nothing -> the geometry renders black (confirmed via RenderDoc). Mirror
// ---------------------------------------------------------------------------
// Surface-aperture registry. Forza references textures by raw GPU address; we
// render those surfaces into host plume textures. This maps each surface's
// backing guest address (page-aligned) -> its host texture, so a fetch-constant
// sample of that address binds the real rendered surface instead of reading
// black guest memory. (The native equivalent of UnleashedRecomp's per-object
// host-resource map, keyed by address instead of object pointer.)
// ---------------------------------------------------------------------------
std::mutex g_surfaceApertureMutex;
std::unordered_map<uint32_t, rr::GuestBaseTexture *> g_surfaceAperture;

void RegisterSurfaceAperture(uint32_t guestAddr, rr::GuestBaseTexture *host) {
  if (host == nullptr || host->texture == nullptr || guestAddr < 0x08000000u ||
      guestAddr >= 0x1A000000u)
    return;
  std::lock_guard<std::mutex> lk(g_surfaceApertureMutex);
  g_surfaceAperture[guestAddr & ~0xFFFu] = host;
}

rr::GuestBaseTexture *LookupSurfaceAperture(uint32_t guestAddr) {
  std::lock_guard<std::mutex> lk(g_surfaceApertureMutex);
  auto it = g_surfaceAperture.find(guestAddr & ~0xFFFu);
  return it != g_surfaceAperture.end() ? it->second : nullptr;
}

// Scan a guest surface descriptor for texture-range backing addresses and map
// each to the host surface. We register every candidate (the descriptor holds
// the address in several forms/offsets) -- a later sample of any of them binds
// the rendered surface.
void RegisterSurfaceApertures(uint32_t descriptorAddr,
                              rr::GuestBaseTexture *host) {
  if (host == nullptr || host->texture == nullptr || descriptorAddr == 0)
    return;
  for (uint32_t off = 0; off < 512u; off += 4u) {
    const uint32_t v = ReadGuestU32At(descriptorAddr + off);
    if (v >= 0x08000000u && v < 0x1A000000u)
      RegisterSurfaceAperture(v, host); // raw page-aligned == fetch-form base
  }
}

// the live table into the native sampler bindings so textures actually show.
void ApplyLiveTexturesFromContext(GuestDevice *device, uint32_t context) {
  static std::atomic<uint32_t> s_n{0};
  const bool trace = s_n.fetch_add(1, std::memory_order_relaxed) < 24;

  // Forza binds textures via FM2_D3D_ApplyGpuMemoryPatches, which writes the
  // canonical 6-dword GPU texture fetch constant into the engine GPU block at
  // block + 1024 + slot*24 (and the descriptor pointer into block+12264+slot*4,
  // which we found empty for these draws). The block base is the value passed
  // to the draw emitter (== *dword_82A41BEC). The native draw bypasses the PM4
  // ring, so we decode the live fetch constants here and mirror them into the
  // native sampler bindings. Each slot is a standard xe_gpu_texture_fetch_t.
  const uint32_t block = context != 0 ? context : ReadGuestU32At(0x82A41BECu);
  if (block == 0)
    return;

  // Bind textures the engine resolved into the fetch-constant array. The upload
  // path is now bounds-checked (skips unmapped guest memory instead of
  // faulting), so we can translate+upload directly. TranslateGuestTextureFetch
  // checks the alias cache first, so repeated draws of the same texture reuse
  // the created resource.
  for (uint32_t slot = 0; slot < 16u; ++slot) {
    const uint32_t fcAddr = block + 1024u + slot * 24u;
    const uint32_t fc0 = ReadGuestU32At(fcAddr);
    const uint32_t fc1 = ReadGuestU32At(fcAddr + 4u);
    if ((fc0 & 0x3u) != 2u && fc1 == 0u)
      continue;
    const uint32_t base = ((fc1 >> 12) & 0xFFFFFu) << 12;
    // Prefer a registered host surface (the rendered/resolved plume texture)
    // over uploading from black guest memory.
    rr::GuestBaseTexture *surf = LookupSurfaceAperture(base);
    if (surf != nullptr) {
      rr::SetTextureBase(device, slot, surf);
      rr::SetTestGameTexture(surf);
      if (trace)
        LogReplayDbg("FM2_LIVE_TEX slot=%u base=0x%08X -> APERTURE host=%p",
                     slot, base, static_cast<void *>(surf));
      continue;
    }
    rr::GuestTexture *tex =
        rr::TranslateGuestTextureFetch(ghp::ToHost<void>(fcAddr), true);
    if (trace) {
      LogReplayDbg("FM2_LIVE_TEX slot=%u fc0=%08X fc1=%08X base=0x%08X tex=%p",
                   slot, fc0, fc1, base, static_cast<void *>(tex));
    }
    if (tex != nullptr) {
      rr::SetTexture(device, slot, tex);
      rr::SetTestGameTexture(tex); // diagnostic: expose to the present test grid
    }
  }
}

// PM4 indexed draw emitters → replaced with native Plume draw calls.
// FM2_D3D_EmitIndexedDrawPm4Packets:              (context, primType, indexBufferBase, indexCount)
// FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset: (context, primType, gpuOffset, startIndex, indexCount)
// FM2_D3D_EmitIndexedDrawPm4WithVertexFormatSetup:(context, primType, gpuOffset, startIndex, indexCount)
void Fm2EmitIndexedDrawPm4Base(uint32_t context, uint32_t primType,
                               uint32_t indexBufferBase,
                               uint32_t indexCount) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origFm2EmitIndexedDrawPm4Packets(context, primType, indexBufferBase,
                                       indexCount);
    return;
  }
  static PresentDiagSlot s_drawEmitSlot;
  CountPresentDiag("DrawEmit", s_drawEmitSlot, nullptr);
  GuestDevice *device = rr::GetActiveGuestDevice();
  static DrawDeviceSlot s_drawEmitDevSlot;
  CountDrawDevice("DrawEmit", s_drawEmitDevSlot, device != nullptr);
  if (device == nullptr) return;
  ApplyLiveColorWriteFromContext(device, context);
  ApplyLiveTexturesFromContext(device, context);
  DrawIndexedVertices(device, primType, 0, 0, indexCount);
}

// Aliased creation hooks: call the original XDK function (FM2 reads the result
// as a genuine D3D resource), then create a shadow GuestBuffer backed by Plume
// and register it in the alias map so Lock/Bind can use the native resource.
uint32_t CreateVertexBufferAliased(uint32_t length) {
  uint32_t xdkHandle = g_origCreateVertexBuffer(length);
  if (!ShouldMirrorPlumeRenderState()) {
    return xdkHandle;
  }
  if (!xdkHandle) return 0;
  GuestBuffer *native = rr::CreateVertexBuffer(length);
  if (native)
    rr::RegisterBufferAlias(xdkHandle, native);
  return xdkHandle;
}

void SubmitNativeIndexedDrawPm4(uint32_t context, uint32_t primType,
                                uint32_t startIndex, uint32_t indexCount) {
  static PresentDiagSlot s_drawSubmitSlot;
  CountPresentDiag("DrawSubmit", s_drawSubmitSlot, nullptr);
  GuestDevice *device = rr::GetActiveGuestDevice();
  static DrawDeviceSlot s_drawSubmitDevSlot;
  CountDrawDevice("DrawSubmit", s_drawSubmitDevSlot, device != nullptr);
  if (device == nullptr) return;
  ApplyLiveColorWriteFromContext(device, context);
  ApplyLiveTexturesFromContext(device, context);
  DrawIndexedVertices(device, primType, 0, startIndex, indexCount);
}

// Decode the current native state snapshot + PM4 draw args and submit a debug
// replay draw to the side-by-side replay window (shadow mode only).
// Mirrors the UnleashedRecomp/ReOdyssey pattern: hook the draw call and read
// prior stream/index state from the recorder populated by midasm hooks.
static void TryBuildAndSubmitDebugReplayForPm4Draw(uint32_t context,
                                                   uint32_t primType,
                                                   uint32_t gpuOffset,
                                                   uint32_t startIndex,
                                                   uint32_t indexCount) {
  (void)context;
  static std::atomic<uint32_t> s_dbg_calls{0};
  const uint32_t call_n = s_dbg_calls.fetch_add(1, std::memory_order_relaxed) + 1;

  if (!nr::WantsDirectDebugReplay() || indexCount == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP wants=%d idxcnt=%u",
                   call_n, (int)nr::WantsDirectDebugReplay(), indexCount);
    }
    return;
  }

  const nr::NativeStateSnapshot snapshot = nr::SnapshotLastNativeState();
  if (!snapshot.valid) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP snapshot_invalid", call_n);
    }
    return;
  }

  // Locate slot 0 and slot 1 from the snapshot (populated by midasm hooks).
  const nr::NativeStateVertexStreamBinding *s0 = nullptr;
  const nr::NativeStateVertexStreamBinding *s1 = nullptr;
  for (const auto &s : snapshot.streams) {
    if (!s.valid) continue;
    if (s.slot == 0u && !s0) s0 = &s;
    else if (s.slot == 1u && !s1) s1 = &s;
  }
  if (!s0 || s0->resource == 0 || s0->stride_bytes == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP no_s0 s0=%p res=%08X stride=%u",
                   call_n, (void*)s0,
                   s0 ? s0->resource : 0u, s0 ? s0->stride_bytes : 0u);
    }
    return;
  }
  if (!snapshot.index_buffer.valid || snapshot.index_buffer.resource == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP no_ib valid=%d res=%08X",
                   call_n, (int)snapshot.index_buffer.valid,
                   snapshot.index_buffer.resource);
    }
    return;
  }

  // Decode stream-0 vertex buffer header (XDK vertex fetch constant layout).
  const auto *s0hdr =
      ghp::ToHost<const GuestD3DVertexBufferHeader>(s0->resource);
  if (!s0hdr || (s0hdr->common.get() & 0xFu) != 1u) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP s0hdr_type hdr=%p common=%08X",
                   call_n, (void*)s0hdr,
                   s0hdr ? s0hdr->common.get() : 0u);
    }
    return;
  }
  const uint32_t s0_gpu_base = s0hdr->format0.get() & ~3u;
  const uint32_t s0_raw_size = s0hdr->format1.get() & 0x3FFFFFCu;
  const uint32_t s0_readable = s0_raw_size & nr::kD3DResourceByteSizeMask;
  if (s0_gpu_base == 0 || s0_readable == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP s0_empty base=%08X readable=%u",
                   call_n, s0_gpu_base, s0_readable);
    }
    return;
  }
  const uint32_t s0_stride = s0->stride_bytes;
  const uint32_t s0_upload_base = nr::DirectDrawReplayUploadGuestBase(s0_gpu_base);
  const uint32_t s0_vcount = s0_readable / s0_stride;
  const uint32_t s0_view_bytes = s0_vcount * s0_stride;
  const uint32_t s0_hash_bytes =
      (s0_view_bytes != 0 && s0_view_bytes < s0_readable) ? s0_view_bytes
                                                           : s0_readable;

  // Decode stream-1 (optional — plan.ready will be false if absent).
  uint32_t s1_gpu_base = 0, s1_raw_size = 0, s1_stride = 0;
  uint32_t s1_upload_base = 0, s1_vcount = 0, s1_view_bytes = 0, s1_hash_bytes = 0;
  bool s1_valid = false;
  if (s1 && s1->resource != 0 && s1->stride_bytes != 0) {
    const auto *s1hdr =
        ghp::ToHost<const GuestD3DVertexBufferHeader>(s1->resource);
    if (s1hdr && (s1hdr->common.get() & 0xFu) == 1u) {
      const uint32_t base = s1hdr->format0.get() & ~3u;
      const uint32_t raw  = s1hdr->format1.get() & 0x3FFFFFCu;
      const uint32_t readable = raw & nr::kD3DResourceByteSizeMask;
      if (base != 0 && readable != 0) {
        s1_gpu_base    = base;
        s1_raw_size    = raw;
        s1_stride      = s1->stride_bytes;
        s1_upload_base = nr::DirectDrawReplayUploadGuestBase(s1_gpu_base);
        s1_vcount      = readable / s1_stride;
        s1_view_bytes  = s1_vcount * s1_stride;
        s1_hash_bytes  = (s1_view_bytes != 0 && s1_view_bytes < readable)
                             ? s1_view_bytes : readable;
        s1_valid = true;
      }
    }
  }

  // When stream1 is absent, synthesize a stride-12 position-only view from
  // stream0 (first 12 bytes = float3 position of each vertex). Both debug
  // replay pipeline layouts (kDebugRaw32Side12, kNativePosition28Side12)
  // require stream1 stride=12.
  std::vector<uint8_t> s1_synth_bytes;
  if (!s1_valid && s0_stride >= 12u && s0_vcount > 0) {
    const uint8_t* s0_ptr = ghp::ToHost<const uint8_t>(s0_upload_base);
    if (s0_ptr) {
      s1_synth_bytes.resize(s0_vcount * 12u);
      for (uint32_t vi = 0; vi < s0_vcount; ++vi) {
        std::memcpy(&s1_synth_bytes[vi * 12u], s0_ptr + vi * s0_stride, 12u);
      }
      s1_gpu_base    = s0_gpu_base;
      s1_raw_size    = s0_raw_size;
      s1_stride      = 12u;
      s1_upload_base = s0_upload_base;
      s1_vcount      = s0_vcount;
      s1_view_bytes  = s0_vcount * 12u;
      s1_hash_bytes  = s1_view_bytes;
      s1_valid       = true;
    }
  }

  // Decode index buffer header.
  const auto *ihdr =
      ghp::ToHost<const GuestD3DIndexBufferHeader>(snapshot.index_buffer.resource);
  if (!ihdr || (ihdr->common.get() & 0xFu) != 2u) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP ihdr_type hdr=%p common=%08X",
                   call_n, (void*)ihdr,
                   ihdr ? ihdr->common.get() : 0u);
    }
    return;
  }
  // D3DFORMAT in top 3 bits of common: 1=INDEX16, 6=INDEX32.
  const uint32_t d3d_idx_fmt = (ihdr->common.get() >> 29) & 0x7u;
  const uint32_t idx_desc_fmt = (d3d_idx_fmt == 6u) ? 2u : 1u;
  const uint32_t idx_elem_bytes = nr::DirectDrawIndexElementByteCount(idx_desc_fmt);
  if (idx_elem_bytes == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP idx_elem_zero d3dfmt=%u",
                   call_n, d3d_idx_fmt);
    }
    return;
  }
  const uint32_t idx_gpu_base  = ihdr->address.get();
  const uint32_t idx_raw_size  = ihdr->size.get();
  const uint32_t idx_readable  = idx_raw_size & nr::kD3DResourceByteSizeMask;
  if (idx_gpu_base == 0 || idx_readable == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP idx_empty base=%08X readable=%u",
                   call_n, idx_gpu_base, idx_readable);
    }
    return;
  }
  const uint32_t idx_upload_base = nr::DirectDrawReplayUploadGuestBase(idx_gpu_base);
  const uint32_t idx_view_bytes  = indexCount * idx_elem_bytes;
  const uint32_t idx_hash_bytes  =
      (idx_view_bytes != 0 && idx_view_bytes < idx_readable) ? idx_view_bytes
                                                              : idx_readable;

  // Hash each buffer range (guest memory; BigEndian bytes, swapped on upload).
  const auto hash_range = [](uint32_t upload_base, uint32_t byte_count,
                              uint64_t &out_hash) -> bool {
    if (upload_base == 0 || byte_count == 0) return false;
    if (!IsReadableGuestRange(upload_base, byte_count)) return false;
    const auto *ptr = ghp::ToHost<const uint8_t>(upload_base);
    if (!ptr) return false;
    out_hash = XXH3_64bits(ptr, byte_count);
    return true;
  };

  uint64_t s0_hash = 0, s1_hash = 0, idx_hash = 0;
  const bool s0_hash_ok = hash_range(s0_upload_base, s0_hash_bytes, s0_hash);
  bool s1_hash_ok = false;
  if (!s1_synth_bytes.empty()) {
    s1_hash    = XXH3_64bits(s1_synth_bytes.data(), s1_synth_bytes.size());
    s1_hash_ok = true;
  } else {
    s1_hash_ok = s1_valid && hash_range(s1_upload_base, s1_hash_bytes, s1_hash);
  }
  const bool idx_hash_ok = hash_range(idx_upload_base, idx_hash_bytes, idx_hash);

  // Build per-buffer view summaries.
  const nr::DirectDrawBufferViewSummary sv0 = nr::BuildDirectDrawBufferViewSummary(
      s0_gpu_base, s0_raw_size, s0_vcount, s0_stride, false, s0_hash_ok, s0_hash);
  const nr::DirectDrawBufferViewSummary sv1 =
      s1_valid ? nr::BuildDirectDrawBufferViewSummary(s1_gpu_base, s1_raw_size,
                                                      s1_vcount, s1_stride, false,
                                                      s1_hash_ok, s1_hash)
               : nr::BuildDirectDrawBufferViewSummary(0u, 0u, 0u, 0u, false, false, 0u);
  const nr::DirectDrawBufferViewSummary svi = nr::BuildDirectDrawBufferViewSummary(
      idx_gpu_base, idx_raw_size, indexCount, idx_desc_fmt, true,
      idx_hash_ok, idx_hash);

  const nr::DirectDrawReplayTopology topology =
      nr::DirectDrawReplayTopologyFromDirectIfacePrimitiveType(primType);
  const nr::DirectDrawShaderKeySummary empty_shader{};
  const nr::DirectDrawIndexedPacketSummary packet =
      nr::BuildDirectDrawIndexedPacketSummary(0u, topology, startIndex,
                                              indexCount, sv0, sv1, svi,
                                              empty_shader, empty_shader);

  nr::DirectDrawDebugReplayPlan plan =
      nr::BuildDirectDrawDebugReplayPlan(packet, snapshot);
  if (!plan.ready) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP plan_not_ready prim=%u topo=%d "
                   "idx=%u s0base=%08X s0stride=%u s1valid=%d s1base=%08X "
                   "s1stride=%u idxbase=%08X s0hash_ok=%d idxhash_ok=%d",
                   call_n, primType, (int)topology,
                   indexCount, s0_gpu_base, s0_stride,
                   (int)s1_valid, s1_gpu_base, s1_stride,
                   idx_gpu_base, (int)s0_hash_ok, (int)idx_hash_ok);
    }
    return;
  }
  if (call_n <= 4) {
    LogReplayDbg("FM2_REPLAY_DBG n=%u SUBMIT prim=%u idx=%u s0base=%08X "
                 "idxbase=%08X s0hash_ok=%d idxhash_ok=%d",
                 call_n, primType, indexCount,
                 s0_gpu_base, idx_gpu_base,
                 (int)s0_hash_ok, (int)idx_hash_ok);
  }

  plan.transform = nr::BuildLastVSDebugReplayTransform();

  const nr::DirectDrawReplaySourceBytes sources{
      .stream0 = ghp::ToHost<const uint8_t>(s0_upload_base),
      .stream1 = !s1_synth_bytes.empty()
                     ? s1_synth_bytes.data()
                     : (s1_valid ? ghp::ToHost<const uint8_t>(s1_upload_base) : nullptr),
      .index   = ghp::ToHost<const uint8_t>(idx_upload_base),
  };
  nr::QueueDirectDebugReplay(plan, sources);
}

void Fm2EmitIndexedDrawPm4WithGpuOffset(uint32_t context, uint32_t primType,
                                        uint32_t gpuOffset,
                                        uint32_t startIndex,
                                        uint32_t indexCount) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origFm2EmitIndexedDrawPm4PacketsWithGpuOffset(
        context, primType, gpuOffset, startIndex, indexCount);
    TryBuildAndSubmitDebugReplayForPm4Draw(context, primType, gpuOffset,
                                          startIndex, indexCount);
    return;
  }
  SubmitNativeIndexedDrawPm4(context, primType, startIndex, indexCount);
}

void Fm2EmitIndexedDrawPm4WithVertexFormatSetup(uint32_t context,
                                                uint32_t primType,
                                                uint32_t gpuOffset,
                                                uint32_t startIndex,
                                                uint32_t indexCount) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origFm2EmitIndexedDrawPm4WithVertexFormatSetup(
        context, primType, gpuOffset, startIndex, indexCount);
    TryBuildAndSubmitDebugReplayForPm4Draw(context, primType, gpuOffset,
                                          startIndex, indexCount);
    return;
  }
  SubmitNativeIndexedDrawPm4(context, primType, startIndex, indexCount);
}

// Forward/opaque object-pass draw emitter. DIAGNOSTIC: call the original (keeps
// the game's state machine consistent -- it already runs safely in plume_native)
// then dump the draw-list structure at drawNode+116 so we can decode the per-draw
// (primType, startIndex, indexCount) and translate to native Plume draws.
void Fm2EmitDirtyStateAndDrawList(uint32_t context, uint32_t drawNode,
                                  uint32_t flags) {
  g_origFm2EmitDirtyStateAndDrawList(context, drawNode, flags);
  if (!ShouldMirrorPlumeRenderState())
    return;
  static PresentDiagSlot s_dirtyDrawSlot;
  CountPresentDiag("DirtyDraw", s_dirtyDrawSlot, nullptr);
  static std::atomic<uint32_t> s_n{0};
  if (s_n.fetch_add(1, std::memory_order_relaxed) < 16) {
    const uint32_t nodeFlags = ReadGuestU32At(drawNode + 108u);
    const uint32_t listHead = ReadGuestU32At(drawNode + 116u);
    const uint32_t count = listHead ? ReadGuestU32At(listHead + 4u) : 0;
    LogReplayDbg(
        "FM2_DIRTY_DRAW ctx=0x%08X node=0x%08X nodeFlags=0x%08X list=0x%08X "
        "count=%u e=[0x%08X 0x%08X | 0x%08X 0x%08X | 0x%08X 0x%08X]",
        context, drawNode, nodeFlags, listHead, count,
        listHead ? ReadGuestU32At(listHead + 8u) : 0,
        listHead ? ReadGuestU32At(listHead + 12u) : 0,
        listHead ? ReadGuestU32At(listHead + 16u) : 0,
        listHead ? ReadGuestU32At(listHead + 20u) : 0,
        listHead ? ReadGuestU32At(listHead + 24u) : 0,
        listHead ? ReadGuestU32At(listHead + 28u) : 0);
  }
}

} // namespace

void FM2PlumeTraceVdSwap(PPCRegister &r3, PPCRegister &r4, PPCRegister &r8,
                         PPCRegister &r9, PPCRegister &r10,
                         PPCRegister &r1) {
  nr::FlushDebugReplayOnPresent();

  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }

  auto readGuestU32 = [](uint32_t guestAddress) -> uint32_t {
    auto *value = ghp::ToHost<rex::be<uint32_t>>(guestAddress);
    return value != nullptr ? value->get() : 0;
  };

  const uint32_t fetchAddress = r4.u32;
  const uint32_t frontbufferAddress = readGuestU32(r8.u32);
  const uint32_t textureFormat = readGuestU32(r9.u32);
  const uint32_t colorSpace = readGuestU32(r10.u32);
  const uint32_t widthPtr = readGuestU32(r1.u32 + 84);
  const uint32_t heightPtr = readGuestU32(r1.u32 + 92);
  const uint32_t width = readGuestU32(widthPtr);
  const uint32_t height = readGuestU32(heightPtr);

  // In plume_native mode, draws go to GPU-side GuestSurface textures, so
  // TranslateGuestTextureFetch (which reads stale guest CPU memory) produces
  // a blank texture. Use the last-bound color render target instead, since
  // all rendering is complete by the time VdSwap fires.
  rr::GuestBaseTexture *presentSource = rr::GetLastDrawnColorRenderTarget();
  if (presentSource == nullptr || presentSource->texture == nullptr) {
    presentSource =
        rr::TranslateGuestTextureFetch(ghp::ToHost<void>(fetchAddress), true);
  }
  const bool hasTexture =
      presentSource != nullptr && presentSource->texture != nullptr;

  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=VdSwap n={} buffer=0x{:08X} "
                "fetch=0x{:08X} frontbuffer=0x{:08X} textureFormat=0x{:08X} "
                "colorSpace=0x{:08X} size={}x{} source={} hasTexture={}",
                n, r3.u32, fetchAddress, frontbufferAddress, textureFormat,
                colorSpace, width, height, static_cast<const void *>(presentSource),
                hasTexture);
  }

  if (!REXCVAR_GET(fm2_plume_vdswap_present)) {
    return;
  }

  if (hasTexture) {
    rr::SetPresentSource(presentSource);
  }
  // FM2PlumeTraceVdSwap fires inside FM2_GpuCommandBuffer_BuildAndSubmit
  // (0x8236CB28), NOT inside FM2_D3D_TryPresentAndUpdateStatus (0x824F83D8).
  // Fm2Present hooks TryPresentAndUpdateStatus but that function is NOT on the
  // FMOD pump path that drives VdSwap, so Video::Present() would never be
  // called without doing it here.
  Video::Present();
}

// ===========================================================================
// FM2 native renderer hooks: call generated FM2 bodies, then mirror state.
// ===========================================================================
REX_HOOK(FM2_RenderContext_SetPixelShaderState, Fm2SetPixelShaderState);
REX_HOOK(FM2_RenderContext_SetVertexShaderState, Fm2SetVertexShaderState);
// REX_HOOK_RAW: forward full PPCContext to original in passthrough mode.
// uint64_t dirty_mask (arg 5) spans r8:r9 in 32-bit PPC ABI; the standard
// REX_HOOK arg-translation reads only ctx.r8.u32 and zeroes r9 in the
// auto-isolating call, silently discarding the low 32 bits of dirty_mask.
REX_HOOK_RAW(FM2_RenderContext_BindVertexStream) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origFm2BindVertexStream.fn(ctx, base);
    return;
  }
  rex::ppc::HostToGuestFunction<Fm2BindVertexStream>(ctx, base);
}
REX_HOOK(FM2_RenderContext_BindIndexBuffer, Fm2BindIndexBuffer);
REX_HOOK(FM2_RenderContext_SetBoundSurface, Fm2SetBoundSurface);
REX_HOOK(sub_8236E228, Fm2SetActivePassId);
REX_HOOK(sub_823716F8, Fm2BindSurface);
REX_HOOK(FM2_Render_LoadPixelShaderResourceById,
         Fm2LoadPixelShaderResourceById);
REX_HOOK(FM2_Render_LoadVertexShaderResourceById,
         Fm2LoadVertexShaderResourceById);

REX_HOOK(FM2_RenderContext_SetDepthStencilEnableState,
         Fm2SetDepthStencilEnableState);
REX_HOOK(FM2_RenderContext_SetAlphaBlendEnableBits,
         Fm2SetAlphaBlendEnableBits);
REX_HOOK(FM2_RenderContext_SetAlphaTestState, Fm2SetAlphaTestState);
REX_HOOK(FM2_RenderContext_SetDepthCompareBits, Fm2SetDepthCompareBits);
REX_HOOK(FM2_RenderContext_SetColorWriteMaskBits, Fm2SetColorWriteMaskBits);
REX_HOOK(FM2_RenderContext_SetClipPlane0Enable, Fm2SetClipPlane0Enable);
REX_HOOK(FM2_RenderContext_SetClipPlane1Enable, Fm2SetClipPlane1Enable);
REX_HOOK(FM2_RenderContext_SetClipPlane2Enable, Fm2SetClipPlane2Enable);
REX_HOOK(FM2_RenderContext_SetClipPlane3Enable, Fm2SetClipPlane3Enable);

REX_HOOK(FM2_D3D_TryPresentAndUpdateStatus, Fm2Present);
REX_HOOK(FM2_GpuCommandBuffer_BuildAndSubmit,
         Fm2GpuCommandBufferBuildAndSubmit);
// Render-worker per-frame dispatch (guest sub_82288948), called in an infinite
// loop by sub_82289640. RAW hook: the original needs its full PPC context
// (r13 thread base, etc.); auto-marshaling would isolate it and trap. After the
// original builds the frame, present on this same thread (the one recording into
// g_commandList) in plume_native/plume_clear, where no other present trigger fires.
REX_HOOK_RAW(sub_82288948) {
  // Start the RenderDoc capture BEFORE the original runs the scene draws (their
  // GPU submit happens inside it), so a real geometry frame is captured in full.
  const bool rdocCapturing =
      ShouldMirrorPlumeRenderState() ? RenderDocFrameBegin() : false;
  g_origRenderWorkerFrame.fn(ctx, base);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  rr::GuestBaseTexture *presentSource = rr::GetLastDrawnColorRenderTarget();
  static PresentDiagSlot s_renderWorkerSlot;
  CountPresentDiag("RenderWorker", s_renderWorkerSlot, presentSource);
  if (presentSource != nullptr && presentSource->texture != nullptr) {
    rr::SetPresentSource(presentSource);
    Video::Present();
  }
  RenderDocFrameEnd(rdocCapturing,
                    g_drawsSinceLastPresent.exchange(0, std::memory_order_relaxed));
  // Ensure the persistent host-side gate driver is running (starts once; keeps
  // pulsing in every game state even after this loading-screen loop ends).
  StartGatePulseThreadOnce();
}
REX_HOOK(FM2_FmodIrqSubmit_8236C688, Fm2FmodIrqSubmitSafe);
REX_HOOK(FM2_ProducerProgressGuard_82369340, Fm2ProducerProgressGuard);

REX_HOOK(FM2_D3DDevice_CreateVertexBuffer, CreateVertexBufferAliased);
REX_HOOK_RAW(FM2_D3DDevice_CreateIndexBuffer) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origCreateIndexBuffer.fn(ctx, base);
    return;
  }
  uint32_t length = ctx.r3.u32;
  uint32_t format = ctx.r4.u32;
  uint32_t xdkHandle = g_origCreateIndexBuffer(length, format);
  ctx.r3.u32 = xdkHandle;
  if (!xdkHandle) return;
  GuestBuffer *native = rr::CreateIndexBuffer(length, format);
  if (native)
    rr::RegisterBufferAlias(xdkHandle, native);
}

REX_HOOK(FM2_Render_AllocGpuPassMemoryBlock, Fm2AllocGpuPassMemoryBlock);
REX_HOOK(FM2_D3D_CreateGpuMemoryBlock, Fm2CreateGpuMemoryBlock);

// Lock / unlock
REX_HOOK(FM2_D3DVertexBuffer_Lock, VertexBufferLock);
REX_HOOK(FM2_D3D_LockGpuBufferRaw, IndexBufferLock);
REX_HOOK(FM2_D3DSurface_LockRect, SurfaceLockRect);
REX_HOOK(FM2_D3DResource_UnlockResource, UnlockResourceHook);
REX_HOOK(FM2_D3DSurface_GetDesc, SurfaceGetDesc);

REX_HOOK(FM2_D3D_EmitIndexedDrawPm4Packets, Fm2EmitIndexedDrawPm4Base);
REX_HOOK(FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset,
         Fm2EmitIndexedDrawPm4WithGpuOffset);
REX_HOOK(FM2_D3D_EmitIndexedDrawPm4WithVertexFormatSetup,
         Fm2EmitIndexedDrawPm4WithVertexFormatSetup);
REX_HOOK(FM2_D3D_EmitDirtyStateAndDrawList, Fm2EmitDirtyStateAndDrawList);

// Diagnostic: count how often Forza's PM4 draw-list interpreter runs. Confirms
// whether the 3D-scene render path executes in plume_native (vs only the
// loading/wait-screen frame loop). RAW hook: preserve full context for original.
REX_HOOK_RAW(FM2_Render_WalkAndDispatchPm4DrawList) {
  static PresentDiagSlot s_walkSlot;
  CountPresentDiag("WalkDispatch", s_walkSlot, nullptr);
  g_origWalkAndDispatchPm4DrawList.fn(ctx, base);
}

// EDRAM resolve. DIAGNOSTIC PASS: scan the context block for the destination
// base address (a texture-range guest address) so we learn which register
// holds it, then we can register the resolved surface there.
uint32_t Fm2EmitSurfaceResolve(uint32_t context, uint32_t flags, uint32_t a3) {
  if (ShouldMirrorPlumeRenderState() && context != 0) {
    const uint32_t colorSurf = ReadGuestU32At(context + 12160u);
    if (colorSurf != 0) {
      // The surface being resolved is one we rendered into a host texture.
      // Register its backing addresses so later samples bind the rendered
      // surface. Get the host texture from the surface descriptor.
      GuestSurface *gs = ghp::ToHost<GuestSurface>(colorSurf);
      rr::GuestBaseTexture *host = AsFm2(gs);
      if (host == nullptr && gs != nullptr)
        host = rr::TranslateGuestSurface(gs);
      if (host == nullptr)
        host = rr::GetLastDrawnColorRenderTarget();
      RegisterSurfaceApertures(colorSurf, host);
    }
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 12) {
      LogReplayDbg("FM2_RESOLVE ctx=0x%08X flags=0x%08X colorSurf=0x%08X",
                   context, flags, colorSurf);
    }
  }
  return g_origEmitSurfaceResolve(context, flags, a3);
}
REX_HOOK(FM2_D3D_EmitSurfaceResolvePackets, Fm2EmitSurfaceResolve);
