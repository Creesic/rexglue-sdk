// Guest hook bindings for the ported FM2P renderer. This file is the only one
// in src/render that names guest functions; everything it calls lives in
// render_state.cpp / d3d_resources.cpp / video.cpp.
//
// Ported from ReXFM2P/FM2/src/render/d3d_hooks.cpp with FM4 addresses. Every
// hook falls through to the original guest body when the native path is off,
// so the xenos plugin is unaffected. The hooks are raw (REX_FUNC) rather than
// marshalled (REX_HOOK) for exactly that reason: under xenos the hook is a
// single forward to __imp__ with the guest's own register state untouched.
#include "generated/default/fm4_init.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <plume_render_interface.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/types.h>

#include "gpu/d3d_guest.h"
#include "gpu/native_video.h"
#include "render/guest_device.h"
#include "render/guest_heap.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_queue.h"
#include "render/render_state.h"
#include "render/video.h"

namespace rr = fm4::render;
namespace ghp = fm4::ghp;

using fm4::render::GuestBaseTexture;
using fm4::render::GuestBuffer;
using fm4::render::GuestDevice;
using fm4::render::GuestLockedRect;
using fm4::render::GuestResource;
using fm4::render::GuestShader;
using fm4::render::GuestSurface;
using fm4::render::GuestSurfaceDesc;
using fm4::render::GuestTexture;
using fm4::render::GuestVertexDeclaration;
using fm4::render::GuestVertexElement;

// d3d_resources.cpp has no header of its own; these are its resource entry
// points, declared here the same way FM2P declares them.
namespace fm4::render {
GuestBuffer* CreateVertexBuffer(uint32_t length);
GuestBuffer* CreateIndexBuffer(uint32_t length, uint32_t format);
GuestTexture* CreateTexture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels,
                            uint32_t usage, uint32_t format, uint32_t pool, uint32_t type);
GuestSurface* CreateSurface(uint32_t width, uint32_t height, uint32_t format, uint32_t multiSample);
GuestVertexDeclaration* CreateVertexDeclaration(const GuestVertexElement* guestElements);
GuestShader* CreateVertexShader(const uint32_t* function);
GuestShader* CreatePixelShader(const uint32_t* function);
void RegisterShaderAlias(uint32_t guestAddress, GuestShader* shader);
uint32_t LockVertexBuffer(GuestBuffer* buffer, uint32_t flags);
uint32_t LockIndexBuffer(GuestBuffer* buffer, uint32_t flags);
void LockRect(GuestBaseTexture* texture, uint32_t level, uint32_t* outPitch, uint32_t* outBits);
void UnlockGuestResource(GuestResource* resource);
void GetSurfaceDesc(const GuestSurface* surface, GuestSurfaceDesc* desc);
// render_state.cpp: draws that reached FlushRenderState but found no pipeline.
uint32_t TakePipelineMissCount();
}  // namespace fm4::render

namespace {

bool Native() { return fm4::gpu::NativeRequested(); }

// Per-300-frame resource counters. Cheap enough to leave unconditional; the
// log line is the only evidence this task produces.
enum Counter : size_t {
  kCreateVB,
  kCreateIB,
  kCreateTex,
  kCreateSurf,
  kCreateDecl,
  kCreateVS,
  kCreatePS,
  kLock,
  kUnlock,
  kPassCalls,
  kDrawIdx,
  kDrawNonIdx,
  kDrawUP,
  kDrawIdxUP,
  kDrawOtherDevice,
  kSwap,
  kResolve,
  kPresentSrc,
  kRunCB,
  kRunCBMiss,
  kBeginTiling,
  kQueryGet,
  kCounterCount
};
constexpr const char* kCounterNames[kCounterCount] = {
    "vb",         "ib",        "tex",        "surf",      "decl",     "vs",
    "ps",         "lock",      "unlock",     "passcalls", "drawIdx",  "drawNonIdx",
    "drawUP",     "drawIdxUP", "drawOtherDev", "swap",     "resolve",  "presentSrc",
    "runCB",      "runCBMiss", "beginTiling", "queryGet"};
std::atomic<uint32_t> g_counters[kCounterCount]{};
void Bump(Counter c) { g_counters[c].fetch_add(1, std::memory_order_relaxed); }

// Resource identity for the conditional hooks below: the guest also builds
// texture-shaped D3DResources through the raw XG* header API (XGSetTextureHeader
// + XGOffsetResourceAddress), bypassing D3DDevice_CreateTexture entirely. Those
// carry no kFm4ResourceMagic, so their original bodies must still run.
GuestResource* OurResource(uint32_t guestAddress) {
  auto* host = ghp::ToHost<GuestResource>(guestAddress);
  return rr::IsFm4Resource(host) ? host : nullptr;
}

void StoreLockedRect(uint32_t lockedRectVa, uint32_t pitch, uint32_t bits) {
  if (auto* out = ghp::ToHost<GuestLockedRect>(lockedRectVa)) {
    out->pitch = static_cast<int32_t>(pitch);
    out->bits = bits;
  }
}

uint32_t ReadGuestU32(uint32_t guestAddress) {
  auto* p = ghp::ToHost<const rex::be<uint32_t>>(guestAddress);
  return p != nullptr ? p->get() : 0;
}

void ReadGuestVec4(uint32_t guestAddress, float out[4]) {
  auto* p = ghp::ToHost<const rex::be<float>>(guestAddress);
  if (p == nullptr) {
    out[0] = out[1] = out[2] = 0.0f;
    out[3] = 1.0f;
    return;
  }
  out[0] = p[0].get();
  out[1] = p[1].get();
  out[2] = p[2].get();
  out[3] = p[3].get();
}

void LogMissingPortHooks() {
  static std::atomic<bool> once{false};
  if (once.exchange(true, std::memory_order_relaxed)) {
    return;
  }
  REXLOG_WARN(
      "fm4render: D3DDevice_EndCommandBuffer not located; recordings close at CreateClone instead");
  REXLOG_WARN(
      "fm4render: D3DCommandBuffer_CreateShaderConstantFFixup not located; recorded command "
      "buffers replay with their record-time bindings");
  REXLOG_WARN(
      "fm4render: D3DCommandBuffer_CreateTextureFixup not located; recorded command buffers "
      "replay with their record-time bindings");
  REXLOG_WARN(
      "fm4render: D3DDevice_EndTiling not located; tiling passes close at the next BeginTiling "
      "or Swap");
}

}  // namespace

namespace fm4::render {

void OnResourceTraceFrame() {
  static std::atomic<uint32_t> frames{0};
  if ((frames.fetch_add(1, std::memory_order_relaxed) % 300) != 299) {
    return;
  }
  char line[512];
  int n = std::snprintf(line, sizeof(line), "[fm4render] frames=300");
  for (size_t i = 0; i < kCounterCount; ++i) {
    n += std::snprintf(line + n, sizeof(line) - n, " %s=%u", kCounterNames[i],
                       g_counters[i].exchange(0, std::memory_order_relaxed));
    // snprintf returns the length it WOULD have written; clamp or sizeof-n wraps.
    n = std::min<int>(n, int(sizeof(line)) - 1);
  }
  std::snprintf(line + n, sizeof(line) - n, " psoMiss=%u", TakePipelineMissCount());
  REXLOG_INFO("{}", line);
}

}  // namespace fm4::render

// ---------------------------------------------------------------------------
// Device
// ---------------------------------------------------------------------------

// Direct3D_CreateDevice(Adapter, DeviceType, hUnusedFocusWindow, BehaviorFlags,
// pPresentationParameters, D3DDevice** ppReturnedDeviceInterface) @0x826E8AA0:
// ppDevice is r8, returns 0 on success. The library still builds the real
// device (0x6080 bytes via D3D::MemAllocAligned); the renderer only latches it.
extern "C" REX_FUNC(Direct3D_CreateDevice) {
  const uint32_t out_device_va = ctx.r8.u32;  // read before the call clobbers r8
  const uint32_t device_type = ctx.r4.u32;
  __imp__Direct3D_CreateDevice(ctx, base);
  // D3DDEVTYPE_COMMAND_BUFFER == 2 (the `cmpwi r29, 2` at 0x826E8B04). FM4
  // creates two of those after the real device; latching one would point the
  // renderer at a device with no ring.
  if (!Native() || ctx.r3.u32 != 0 || device_type == 2) {
    return;
  }
  const uint32_t device_va =
      __builtin_bswap32(*reinterpret_cast<volatile uint32_t*>(base + out_device_va));
  auto* device = ghp::ToHost<GuestDevice>(device_va);
  rr::SetActiveGuestDevice(device);
  REXLOG_INFO("fm4render: guest D3DDevice at 0x{:08X}", device_va);
  rr::LogGuestDeviceLayout(device);
  LogMissingPortHooks();
}

// ---------------------------------------------------------------------------
// Resource creation: pure replacements under the native path. The originals
// allocate real XDK resource headers the host renderer has no way to back.
// Signatures confirmed against ida40; all seven return the object in r3 (not
// an HRESULT + out-parameter) and take no device argument.
// ---------------------------------------------------------------------------

// D3DDevice_CreateVertexBuffer(Length, Usage, UnusedPool) @0x826E7348.
extern "C" REX_FUNC(D3DDevice_CreateVertexBuffer) {
  if (!Native()) {
    __imp__D3DDevice_CreateVertexBuffer(ctx, base);
    return;
  }
  const uint32_t length = ctx.r3.u32;
  Bump(kCreateVB);
  ctx.r3.u64 = ghp::ToGuest(rr::CreateVertexBuffer(length));
}

// D3DDevice_CreateIndexBuffer(Length, Usage, Format, UnusedPool) @0x826E7410.
extern "C" REX_FUNC(D3DDevice_CreateIndexBuffer) {
  if (!Native()) {
    __imp__D3DDevice_CreateIndexBuffer(ctx, base);
    return;
  }
  const uint32_t length = ctx.r3.u32;
  const uint32_t format = ctx.r5.u32;
  Bump(kCreateIB);
  ctx.r3.u64 = ghp::ToGuest(rr::CreateIndexBuffer(length, format));
}

// D3DDevice_CreateTexture(Width, Height, Depth, Levels, Usage, Format,
// UnusedPool, D3DType) @0x826DF128 -- eight arguments, r3..r10. D3DType is the
// Xbox extension that distinguishes D3DRTYPE_VOLUMETEXTURE (17).
extern "C" REX_FUNC(D3DDevice_CreateTexture) {
  fm4::gpu::TraceOnCreateTexture();
  if (!Native()) {
    __imp__D3DDevice_CreateTexture(ctx, base);
    return;
  }
  const uint32_t width = ctx.r3.u32;
  const uint32_t height = ctx.r4.u32;
  const uint32_t depth = ctx.r5.u32;
  const uint32_t levels = ctx.r6.u32;
  const uint32_t usage = ctx.r7.u32;
  const uint32_t format = ctx.r8.u32;
  const uint32_t pool = ctx.r9.u32;
  const uint32_t type = ctx.r10.u32;
  Bump(kCreateTex);
  ctx.r3.u64 =
      ghp::ToGuest(rr::CreateTexture(width, height, depth, levels, usage, format, pool, type));
}

// D3DTexture_GetSurfaceLevel(pThis, Level) @0x826DEEC0. The original calls
// FindSurfaceWithinTexture (walks the XDK fetch constant) then mallocs a
// 48-byte D3DSurface whose SurfaceInfo is the parent texture. Running that on
// a GuestTexture reads host members as GPU state. Return the texture itself:
// it is already a GuestBaseTexture, SetRenderTarget can bind it, and the extra
// AddRef is dropped when the caller Releases the "surface".
extern "C" REX_FUNC(D3DTexture_GetSurfaceLevel) {
  if (!Native()) {
    __imp__D3DTexture_GetSurfaceLevel(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    __imp__D3DTexture_GetSurfaceLevel(ctx, base);
    return;
  }
  host->refCount.fetch_add(1, std::memory_order_acq_rel);
  ctx.r3.u64 = ghp::ToGuest(host);
}

// D3DDevice_CreateSurface(Width, Height, Format, MultiSample, pParameters)
// @0x826DF248. pParameters (r7) selects an explicit EDRAM base; the host
// renderer has no EDRAM, so it is ignored exactly as FM2P ignores it.
extern "C" REX_FUNC(D3DDevice_CreateSurface) {
  if (!Native()) {
    __imp__D3DDevice_CreateSurface(ctx, base);
    return;
  }
  const uint32_t width = ctx.r3.u32;
  const uint32_t height = ctx.r4.u32;
  const uint32_t format = ctx.r5.u32;
  const uint32_t multiSample = ctx.r6.u32;
  Bump(kCreateSurf);
  ctx.r3.u64 = ghp::ToGuest(rr::CreateSurface(width, height, format, multiSample));
}

// D3DDevice_CreateVertexDeclaration(pVertexElements) @0x826E6B48.
extern "C" REX_FUNC(D3DDevice_CreateVertexDeclaration) {
  if (!Native()) {
    __imp__D3DDevice_CreateVertexDeclaration(ctx, base);
    return;
  }
  const uint32_t elements = ctx.r3.u32;
  Bump(kCreateDecl);
  ctx.r3.u64 =
      ghp::ToGuest(rr::CreateVertexDeclaration(ghp::ToHost<const GuestVertexElement>(elements)));
}

// D3DDevice_CreateVertexShader(pFunction) @0x826E7088: pFunction is the raw
// ShaderContainer microcode pointer. RegisterShaderAlias keys the container
// address so Task 6's SetVertexShader can resolve a shader the guest binds by
// address without going back through creation.
extern "C" REX_FUNC(D3DDevice_CreateVertexShader) {
  const uint32_t function = ctx.r3.u32;
  fm4::gpu::TraceOnCreateShader(function, /*pixel=*/false);
  if (!Native()) {
    __imp__D3DDevice_CreateVertexShader(ctx, base);
    return;
  }
  Bump(kCreateVS);
  GuestShader* shader = rr::CreateVertexShader(ghp::ToHost<const uint32_t>(function));
  rr::RegisterShaderAlias(function, shader);
  ctx.r3.u64 = ghp::ToGuest(shader);
}

// D3DDevice_CreatePixelShader(pFunction) @0x826E6EA0.
extern "C" REX_FUNC(D3DDevice_CreatePixelShader) {
  const uint32_t function = ctx.r3.u32;
  fm4::gpu::TraceOnCreateShader(function, /*pixel=*/true);
  if (!Native()) {
    __imp__D3DDevice_CreatePixelShader(ctx, base);
    return;
  }
  Bump(kCreatePS);
  GuestShader* shader = rr::CreatePixelShader(ghp::ToHost<const uint32_t>(function));
  rr::RegisterShaderAlias(function, shader);
  ctx.r3.u64 = ghp::ToGuest(shader);
}

// ---------------------------------------------------------------------------
// Lock / unlock / describe / refcount: conditional replacements. Objects the
// guest built through the XG* header API are not ours -- IsFm4Resource is
// false and the original body runs (counted as "passcalls").
// ---------------------------------------------------------------------------

// D3DVertexBuffer_Lock(pThis, OffsetToLock, SizeToLock, Flags) @0x82386BF8.
// OffsetToLock and SizeToLock are dead in the guest body too (r4/r5 are
// overwritten with 0/0xA before the call to the shared lock helper), so the
// returned pointer is the buffer base under both paths.
extern "C" REX_FUNC(D3DVertexBuffer_Lock) {
  if (!Native()) {
    __imp__D3DVertexBuffer_Lock(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassCalls);
    __imp__D3DVertexBuffer_Lock(ctx, base);
    return;
  }
  const uint32_t flags = ctx.r6.u32;
  Bump(kLock);
  ctx.r3.u64 = rr::LockVertexBuffer(static_cast<GuestBuffer*>(host), flags);
}

// D3DIndexBuffer_Lock(pThis, OffsetToLock, SizeToLock, Flags) @0x826E8218.
extern "C" REX_FUNC(D3DIndexBuffer_Lock) {
  if (!Native()) {
    __imp__D3DIndexBuffer_Lock(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassCalls);
    __imp__D3DIndexBuffer_Lock(ctx, base);
    return;
  }
  const uint32_t flags = ctx.r6.u32;
  Bump(kLock);
  ctx.r3.u64 = rr::LockIndexBuffer(static_cast<GuestBuffer*>(host), flags);
}

// D3DSurface_LockRect(pThis, pLockedRect, pRect, Flags) @0x826DF418: no mip
// level (IDirect3DSurface9 has none).
extern "C" REX_FUNC(D3DSurface_LockRect) {
  if (!Native()) {
    __imp__D3DSurface_LockRect(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassCalls);
    __imp__D3DSurface_LockRect(ctx, base);
    return;
  }
  const uint32_t lockedRectVa = ctx.r4.u32;
  Bump(kLock);
  uint32_t pitch = 0, bits = 0;
  rr::LockRect(static_cast<GuestBaseTexture*>(host), 0, &pitch, &bits);
  StoreLockedRect(lockedRectVa, pitch, bits);
}

// D3DTexture_LockRect(pThis, Level, pLockedRect, pRect, Flags) @0x826DEF78:
// the mip level is r4. Missing this hook leaves the original body running
// against our non-XDK layout and corrupts every mip.
extern "C" REX_FUNC(D3DTexture_LockRect) {
  if (!Native()) {
    __imp__D3DTexture_LockRect(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassCalls);
    __imp__D3DTexture_LockRect(ctx, base);
    return;
  }
  const uint32_t level = ctx.r4.u32;
  const uint32_t lockedRectVa = ctx.r5.u32;
  Bump(kLock);
  uint32_t pitch = 0, bits = 0;
  rr::LockRect(static_cast<GuestBaseTexture*>(host), level, &pitch, &bits);
  StoreLockedRect(lockedRectVa, pitch, bits);
}

// D3DCubeTexture_LockRect(pThis, FaceType, Level, pLockedRect, pRect, Flags)
// @0x826DF050. Face is ignored: our lock path has no array-slice model.
extern "C" REX_FUNC(D3DCubeTexture_LockRect) {
  if (!Native()) {
    __imp__D3DCubeTexture_LockRect(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassCalls);
    __imp__D3DCubeTexture_LockRect(ctx, base);
    return;
  }
  const uint32_t level = ctx.r5.u32;
  const uint32_t lockedRectVa = ctx.r6.u32;
  Bump(kLock);
  uint32_t pitch = 0, bits = 0;
  rr::LockRect(static_cast<GuestBaseTexture*>(host), level, &pitch, &bits);
  StoreLockedRect(lockedRectVa, pitch, bits);
}

// D3D::UnlockResource(pResource, pBase, pMip) @0x822AFE48 -- the single unlock
// leaf, reached from D3DTexture_Unlock (0x826DE3D8) for textures/surfaces and
// D3DBuffer_Unlock (0x822B9620) for vertex/index buffers. Its tail is an
// lwarx/stwcx subtract of 0x100 from the resource's Common word, which would
// silently corrupt kFm4ResourceMagic if it ever ran on one of our objects.
extern "C" REX_FUNC(D3D_UnlockResource) {
  if (!Native()) {
    __imp__D3D_UnlockResource(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassCalls);
    __imp__D3D_UnlockResource(ctx, base);
    return;
  }
  Bump(kUnlock);
  rr::UnlockGuestResource(host);
}

// D3DSurface_GetDesc(pThis, pDesc) @0x826DF380.
extern "C" REX_FUNC(D3DSurface_GetDesc) {
  if (!Native()) {
    __imp__D3DSurface_GetDesc(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassCalls);
    __imp__D3DSurface_GetDesc(ctx, base);
    return;
  }
  rr::GetSurfaceDesc(static_cast<GuestSurface*>(host), ghp::ToHost<GuestSurfaceDesc>(ctx.r4.u32));
}

// D3DResource_GetType(pThis) @0x822E5CB0 returns pThis->Common & 0xF (with a
// texture-family refinement). Our objects carry kFm4ResourceMagic there, whose
// low nibble is 2 (D3DRTYPE_INDEXBUFFER) for everything -- which sent
// XGGetTextureDesc down a branch that leaves _XGTEXTURE_DESC uninitialized and
// made XGRAPHICS::TileSurface (UtilityTexture::Create @0x8285C4A8) scribble
// past the lock staging until it hit an uncommitted guest page. Values 1/2/3/4
// and 17 are confirmed from FM4's own creation functions and GetType's body;
// 5/6/7 are the XDK enum's.
extern "C" REX_FUNC(D3DResource_GetType) {
  if (!Native()) {
    __imp__D3DResource_GetType(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassCalls);
    __imp__D3DResource_GetType(ctx, base);
    return;
  }
  uint32_t type = 0;  // D3DRTYPE_NONE
  switch (host->type) {
    case fm4::render::ResourceType::VertexBuffer: type = 1; break;
    case fm4::render::ResourceType::IndexBuffer: type = 2; break;
    case fm4::render::ResourceType::Texture: type = 3; break;
    case fm4::render::ResourceType::RenderTarget:
    case fm4::render::ResourceType::DepthStencil: type = 4; break;
    case fm4::render::ResourceType::VertexDeclaration: type = 5; break;
    case fm4::render::ResourceType::VertexShader: type = 6; break;
    case fm4::render::ResourceType::PixelShader: type = 7; break;
    // 17, not 14: this XDK shifts the texture-family tail (0x822E5CE0 is
    // `li r11,0x11` for the volume branch), and d3d_resources.cpp already
    // treats D3DType 17 as D3DRTYPE_VOLUMETEXTURE.
    case fm4::render::ResourceType::VolumeTexture: type = 17; break;
  }
  ctx.r3.u64 = type;
}

// The guest AddRef/Release use big-endian lwarx/stwcx on resource+4. Our
// GuestResource::refCount is a host little-endian atomic aliasing that same
// offset, so BOTH must be hooked for our objects or refcounts corrupt and
// resources are freed while bound.
extern "C" REX_FUNC(D3DResource_AddRef) {
  if (!Native()) {
    __imp__D3DResource_AddRef(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassCalls);
    __imp__D3DResource_AddRef(ctx, base);
    return;
  }
  ctx.r3.u64 = host->refCount.fetch_add(1, std::memory_order_acq_rel) + 1;
}

extern "C" REX_FUNC(D3DResource_Release) {
  if (!Native()) {
    __imp__D3DResource_Release(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassCalls);
    __imp__D3DResource_Release(ctx, base);
    return;
  }
  const uint32_t remaining = host->refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
  if (remaining == 0) {
    // Never the guest's own D3D::DestroyResource: host Plume objects and the
    // guest allocation are retired after the recording-frame fence.
    rr::ScheduleResourceDestruction(host);
  }
  ctx.r3.u64 = remaining;
}

// ---------------------------------------------------------------------------
// State binds. No D3DDevice_SetRenderState_* is hooked anywhere in this tree:
// FM4 leaves 11 of the 36 with no out-of-line body, and all 36 are sampled from
// the guest's register shadows at draw time instead (render_state.cpp
// SampleGuestRenderStates). The binds below cannot be recovered that way --
// the shadow holds a Xenos fetch constant, not a host object pointer.
//
// D3DDevice_SetVertexShaderConstantB / SetPixelShaderConstantB are deliberately
// NOT hooked either: the bool files live at device+0x2780 / +0x2790 and
// QueueDrawStateSnapshots already reads them straight out of the device with
// kBooleanDirtyMask. Nor are the nine D3DDevice_SetSamplerState_*:
// ProcSetSamplerState decodes sampler state from the raw fetch-constant dwords
// those setters write. They are named in the TOML only so a stack is readable.
// ---------------------------------------------------------------------------

namespace {

// The library keeps the bound surfaces at device+0x31F8 (colour 0..3) and
// device+0x3208 (depth). Those slots are not just bookkeeping: they gate the
// render-state setters the sampler then reads back --
// D3DDevice_SetRenderState_ZEnable (0x8234FB48) and _StencilEnable (0x82384230)
// force their bit to 0 while 0x3208 is null, and _ColorWriteEnable (0x822BC9E0)
// zeroes the write mask while 0x31F8 is null. The originals that maintain them
// also walk the surface header (D3DDevice_SetRenderTarget does `lwz r7,0x1C(r5)`
// at 0x823563F0), which is host-shaped memory for our GuestSurface, so we store
// the slot ourselves and skip the walk.
rex::be<uint32_t>* DeviceWord(uint32_t device, uint32_t offset) {
  return device != 0 ? ghp::ToHost<rex::be<uint32_t>>(device + offset) : nullptr;
}

void StoreBoundSurfaceSlot(uint32_t device, uint32_t offset, uint32_t surface) {
  if (auto* slot = DeviceWord(device, offset)) {
    *slot = surface;
  }
}
constexpr uint32_t kBoundColorSurfaceSlot0 = 0x31F8;
constexpr uint32_t kBoundDepthSurfaceSlot = 0x3208;
// m_pRing, and the slot D3DDevice_BeginVertices parks the post-vertex ring
// pointer in (see the BeginVertices hook).
constexpr uint32_t kGuestRingPointerOffset = 0x30;
constexpr uint32_t kGuestBeginVerticesRingSaveOffset = 0x3604;
constexpr uint32_t kDepthControlOffset = 0x2934;
constexpr uint32_t kColorMaskOffset = 0x28DC;
constexpr uint32_t kRawZEnableOffset = 0x2FFC;
constexpr uint32_t kRawStencilEnableOffset = 0x3000;
constexpr uint32_t kRawColorWriteEnableSlot0 = 0x2FEC;

// Both skipped library bodies end by *re-applying* a render state they had
// previously forced to zero, now that a surface is bound again. Storing the
// gate slot is not enough on its own -- the shadow only reaches the register
// here -- which is why colorWriteEnable sampled as 0 for the whole run.
//
// D3DDevice_SetDepthStencilSurface tail @0x822D9BAC:
//   r10 = (*(dev+0x3208) != 0) ? 0xFFFFFFFF : 0        (subfic/subfe idiom)
//   DepthControl bit 1 = (r10 & *(dev+0x2FFC)) & 1     insrwi r11,r10,1,30
//   DepthControl bit 0 = (r10 & *(dev+0x3000)) & 1     rlwimi r11,r10,0,0,30
void ReapplyDepthStencilGatedStates(uint32_t device) {
  auto* depthControl = DeviceWord(device, kDepthControlOffset);
  auto* slot = DeviceWord(device, kBoundDepthSurfaceSlot);
  auto* rawZ = DeviceWord(device, kRawZEnableOffset);
  auto* rawStencil = DeviceWord(device, kRawStencilEnableOffset);
  if (depthControl == nullptr || slot == nullptr || rawZ == nullptr || rawStencil == nullptr) {
    return;
  }
  const bool bound = slot->get() != 0;
  const uint32_t zEnable = bound ? (rawZ->get() & 1u) : 0u;
  const uint32_t stencilEnable = bound ? (rawStencil->get() & 1u) : 0u;
  *depthControl = (depthControl->get() & ~0x3u) | (zEnable << 1) | stencilEnable;
}

// D3DDevice_SetRenderTarget tail @0x82356674/0x823566A8/0x823566D4/0x82356700,
// one arm per slot and identical apart from the offsets:
//   r11 = (*(dev+0x31F8+4*N) != 0) ? 0xFFFFFFFF : 0
//   ColorMask nibble N = (r11 & *(dev+0x2FEC+4*N)) & 0xF
//     insrwi 4,28 / 4,24 / 4,20 / 4,16  ->  shift 0 / 4 / 8 / 12
void ReapplyRenderTargetColorMask(uint32_t device, uint32_t slotIndex) {
  auto* colorMask = DeviceWord(device, kColorMaskOffset);
  auto* slot = DeviceWord(device, kBoundColorSurfaceSlot0 + 4u * slotIndex);
  auto* raw = DeviceWord(device, kRawColorWriteEnableSlot0 + 4u * slotIndex);
  if (colorMask == nullptr || slot == nullptr || raw == nullptr) {
    return;
  }
  const uint32_t nibble = slot->get() != 0 ? (raw->get() & 0xFu) : 0u;
  const uint32_t shift = 4u * slotIndex;
  *colorMask = (colorMask->get() & ~(0xFu << shift)) | (nibble << shift);
}

// Ported from FM2P: our own textures bind directly, render-target/depth
// surfaces sampled as shader resources go through SetTextureBase (which forces
// a 2D view), and a texture the guest built through the raw XG* header API
// carries no kFm4ResourceMagic and is translated from its Xenos fetch constant
// rather than binding null.
struct ResolvedTextureBinding {
  GuestBaseTexture* texture = nullptr;
  bool baseTexture = false;
};

ResolvedTextureBinding ResolveTextureBinding(GuestBaseTexture* texture) {
  if (texture == nullptr) {
    return {};
  }
  if (!rr::IsFm4Resource(texture)) {
    GuestTexture* translated = rr::TranslateGuestTexture(texture, true);
    if (translated == nullptr) {
      // Once per distinct header, as in FM2P: a raw XG texture whose fetch
      // constant we cannot parse binds null, and a silently null sampler is
      // indistinguishable from a shader bug.
      static std::mutex warnMutex;
      static std::unordered_set<const void*> warned;
      std::lock_guard lock(warnMutex);
      if (warned.insert(texture).second) {
        REXLOG_WARN("D3DDevice_SetTexture: texture {} untranslatable (XG header?) -- bound null",
                    static_cast<const void*>(texture));
      }
    }
    return {translated, false};
  }
  switch (texture->type) {
    case fm4::render::ResourceType::Texture:
    case fm4::render::ResourceType::VolumeTexture:
      return {texture, false};
    case fm4::render::ResourceType::RenderTarget:
    case fm4::render::ResourceType::DepthStencil:
      return {texture, true};
    default:
      return {};
  }
}

// CreateDevice's default RTs are 48-byte XDK D3DSurface objects (Common nibble
// 4). GetDesc @0x826DF380 reads Format at +0x28 and packed width/height at +0x24.
GuestSurface* HostSurfaceForXdkHeader(uint32_t surface) {
  static std::mutex mutex;
  static std::unordered_map<uint32_t, GuestSurface*> cache;
  std::lock_guard lock(mutex);
  if (auto it = cache.find(surface); it != cache.end()) {
    return it->second;
  }
  const uint32_t packed = ReadGuestU32(surface + 0x24);
  const uint32_t format = ReadGuestU32(surface + 0x28);
  const uint32_t width = (packed >> 18) + 1u;
  const uint32_t height = ((packed >> 3) & 0x7FFFu) + 1u;
  const uint32_t msaa = (ReadGuestU32(surface + 0x18) >> 16) & 3u;
  if (width == 0 || height == 0 || width > 4096u || height > 4096u) {
    return nullptr;
  }
  GuestSurface* created = rr::CreateSurface(width, height, format, msaa);
  if (created != nullptr) {
    cache[surface] = created;
    REXLOG_INFO("fm4render: XDK surface 0x{:08X} -> host {}x{} fmt=0x{:08X} msaa={}", surface,
                width, height, format, msaa);
  }
  return created;
}

// Our CreateSurface/CreateTexture objects bind directly. XDK D3DSurface headers
// (nibble 4) get a host RT via GetDesc's packed size. Textures go through
// TranslateGuestTexture. GetSurfaceLevel of our textures already returns the
// GuestTexture itself (Common bit 0x40000000 children of XDK textures still
// use Parent at +0x18).
GuestBaseTexture* ResolveBoundColorTarget(uint32_t surface) {
  if (surface == 0) {
    return nullptr;
  }
  auto* host = ghp::ToHost<GuestResource>(surface);
  if (rr::IsFm4Resource(host)) {
    return static_cast<GuestBaseTexture*>(host);
  }
  const uint32_t common = ReadGuestU32(surface);
  if ((common & 0xF) == 4) {
    if ((common & 0x40000000u) != 0) {
      const uint32_t parent = ReadGuestU32(surface + rr::kGuestSurfaceParentOffset);
      if (parent >= 0x10000u && (parent & 3u) == 0) {
        auto* parentHost = ghp::ToHost<GuestResource>(parent);
        if (rr::IsFm4Resource(parentHost)) {
          return static_cast<GuestBaseTexture*>(parentHost);
        }
      }
    }
    return HostSurfaceForXdkHeader(surface);
  }
  return rr::TranslateGuestTexture(host, false);
}

GuestSurface* ResolveBoundDepthTarget(uint32_t surface) {
  GuestBaseTexture* bound = ResolveBoundColorTarget(surface);
  if (bound == nullptr) {
    return nullptr;
  }
  switch (bound->type) {
    case fm4::render::ResourceType::DepthStencil:
    case fm4::render::ResourceType::RenderTarget:
      return static_cast<GuestSurface*>(bound);
    default:
      return nullptr;
  }
}

}  // namespace

// D3DDevice_SetTexture(pDevice, Sampler, pTexture, PendingMask3) @0x8233A9A8.
// Pure replacement: the original reads pTexture->Format.dword[0..5] (the Xenos
// fetch constant at +0x1C of a real D3DBaseTexture) and merges it into the
// device's fetch-constant file. It deliberately preserves the sampler-state
// bits ProcSetSamplerState decodes, so skipping it loses nothing there.
extern "C" REX_FUNC(D3DDevice_SetTexture) {
  if (!Native()) {
    __imp__D3DDevice_SetTexture(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t sampler = ctx.r4.u32;
  const uint32_t texture = ctx.r5.u32;
  if (sampler >= 16u) {
    return;  // Xenos exposes 16 unified sampler slots
  }
  auto* dev = ghp::ToHost<GuestDevice>(device);
  auto* host = texture != 0 ? ghp::ToHost<GuestBaseTexture>(texture) : nullptr;
  const ResolvedTextureBinding binding = ResolveTextureBinding(host);
  if (binding.baseTexture) {
    rr::SetTextureBase(dev, sampler, binding.texture, texture);
  } else {
    rr::SetTexture(dev, sampler, static_cast<GuestTexture*>(binding.texture), texture);
  }
}

// D3DDevice_SetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes,
// Stride, PendingMask3) @0x823445A0. The original still runs: it maintains
// device->boundVertexStreams (0x320C) and device->streamStrideDwords (0x3250),
// which QueueDrawStateSnapshots reads for every draw, and for a non-GuestBuffer
// stream it also writes the vertex fetch constant the raw-buffer snapshot path
// decodes. Everything it touches on one of our buffers is a read of the
// guest-owned header bytes plus a write to +8, which GuestResource reserves for
// exactly that.
extern "C" REX_FUNC(D3DDevice_SetStreamSource) {
  if (!Native()) {
    __imp__D3DDevice_SetStreamSource(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t index = ctx.r4.u32;
  const uint32_t buffer = ctx.r5.u32;
  const uint32_t offset = ctx.r6.u32;
  const uint32_t stride = ctx.r7.u32;
  __imp__D3DDevice_SetStreamSource(ctx, base);
  auto* host = buffer != 0 ? ghp::ToHost<GuestResource>(buffer) : nullptr;
  rr::SetStreamSource(ghp::ToHost<GuestDevice>(device), index,
                      rr::IsFm4Resource(host) ? static_cast<GuestBuffer*>(host) : nullptr, offset,
                      stride);
}

// D3DDevice_SetIndices(pDevice, pIndexData) @0x8236E3B8: keeps
// device->boundIndexBuffer (0x31F4) live for QueueDrawStateSnapshots.
extern "C" REX_FUNC(D3DDevice_SetIndices) {
  if (!Native()) {
    __imp__D3DDevice_SetIndices(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t buffer = ctx.r4.u32;
  __imp__D3DDevice_SetIndices(ctx, base);
  auto* host = buffer != 0 ? ghp::ToHost<GuestResource>(buffer) : nullptr;
  rr::SetIndices(ghp::ToHost<GuestDevice>(device),
                 rr::IsFm4Resource(host) ? static_cast<GuestBuffer*>(host) : nullptr);
}

// D3DDevice_SetVertexDeclaration(pDevice, pDecl) @0x822F8728: two stores, no
// dereference of pDecl at all -- keeps device->vertexDeclaration (0x2FB8) live.
extern "C" REX_FUNC(D3DDevice_SetVertexDeclaration) {
  if (!Native()) {
    __imp__D3DDevice_SetVertexDeclaration(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t declaration = ctx.r4.u32;
  __imp__D3DDevice_SetVertexDeclaration(ctx, base);
  auto* host = declaration != 0 ? ghp::ToHost<GuestResource>(declaration) : nullptr;
  rr::SetVertexDeclaration(
      ghp::ToHost<GuestDevice>(device),
      rr::IsFm4Resource(host) ? static_cast<GuestVertexDeclaration*>(host) : nullptr);
}

// D3DDevice_SetRenderTarget(pDevice, RenderTargetIndex, pSurface) @0x823563B8.
// The milestone-1 trace measured rtIdxNon0 = 2700 per 300 frames, so FM4 really
// does use MRT and the index is passed through -- render_state.cpp logs the
// slots it cannot bind yet rather than dropping them silently.
extern "C" REX_FUNC(D3DDevice_SetRenderTarget) {
  const uint32_t index = ctx.r4.u32;
  fm4::gpu::TraceOnSetRenderTarget(index);
  if (!Native()) {
    __imp__D3DDevice_SetRenderTarget(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t surface = ctx.r5.u32;
  if (index < 4u) {
    StoreBoundSurfaceSlot(device, kBoundColorSurfaceSlot0 + 4u * index, surface);
    ReapplyRenderTargetColorMask(device, index);
  }
  GuestBaseTexture* bound = ResolveBoundColorTarget(surface);
  static std::atomic<bool> loggedFirst{false};
  if (!loggedFirst.exchange(true, std::memory_order_relaxed)) {
    REXLOG_INFO("fm4render: first SetRenderTarget index={} surface=0x{:08X} common=0x{:08X} bound={}",
                index, surface, surface != 0 ? ReadGuestU32(surface) : 0, bound != nullptr);
  }
  rr::SetRenderTarget(ghp::ToHost<GuestDevice>(device), index, bound);
}

// D3DDevice_SetDepthStencilSurface(pDevice, pSurface) @0x822D9968: its own first
// instruction is `stw r4,0x3208(r3)`, which is all of it we still want.
extern "C" REX_FUNC(D3DDevice_SetDepthStencilSurface) {
  if (!Native()) {
    __imp__D3DDevice_SetDepthStencilSurface(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t surface = ctx.r4.u32;
  StoreBoundSurfaceSlot(device, kBoundDepthSurfaceSlot, surface);
  ReapplyDepthStencilGatedStates(device);
  rr::SetDepthStencilSurface(ghp::ToHost<GuestDevice>(device), ResolveBoundDepthTarget(surface));
}

// ---------------------------------------------------------------------------
// Shader state: PURE replacements, no passthrough. The original bodies read the
// shader argument as a real 24-byte D3DVertexShader/D3DPixelShader and use its
// ReadFence field (+0xC) as an offset into a compiled-state table. Our
// GuestShader is a C++ object (mutex, unique_ptr, unordered_map), so that read
// returns garbage, and the merge loop -- which decrements a uint16_t by 2 until
// it hits zero -- never terminates on an odd garbage count. In FM2P this hung
// the process for minutes at a time before the hooks were made pure.
// ---------------------------------------------------------------------------

extern "C" REX_FUNC(D3DDevice_SetVertexShader) {
  if (!Native()) {
    __imp__D3DDevice_SetVertexShader(ctx, base);
    return;
  }
  auto* host = ctx.r4.u32 != 0 ? ghp::ToHost<GuestResource>(ctx.r4.u32) : nullptr;
  rr::SetVertexShader(ghp::ToHost<GuestDevice>(ctx.r3.u32),
                      rr::IsFm4Resource(host) ? static_cast<GuestShader*>(host) : nullptr);
}

extern "C" REX_FUNC(D3DDevice_SetPixelShader) {
  if (!Native()) {
    __imp__D3DDevice_SetPixelShader(ctx, base);
    return;
  }
  auto* host = ctx.r4.u32 != 0 ? ghp::ToHost<GuestResource>(ctx.r4.u32) : nullptr;
  rr::SetPixelShader(ghp::ToHost<GuestDevice>(ctx.r3.u32),
                     rr::IsFm4Resource(host) ? static_cast<GuestShader*>(host) : nullptr);
}

// ---------------------------------------------------------------------------
// Viewport / scissor. Full replacements: the originals pack hardware clip state
// for a GPU that is not there, and both reach it by dereferencing the bound
// surface. D3D_SetViewport reads device+0x31F8 / +0x3208 and then
// `*(surface + 0x24)` to recover the target's pitch and height, then tail-calls
// D3DDevice_SetScissorRect, which does the same again through
// D3D::SetSurfaceClip -- the observed native fault at ~frame 900 (write of guest
// 0x00000004 in sub_82310530 <- D3DDevice_SetScissorRect <- D3D_SetViewport).
// ---------------------------------------------------------------------------

// D3D_SetViewport(pDevice, X, Y, Width, Height, MinZ, MaxZ) @0x822FEB28: six
// floats, FM4 having dropped FM2's trailing DWORD Flags. Both
// D3DDevice_SetViewport (0x822FEAA8) and D3DDevice_SetViewportF (0x823925C0)
// tail-call it, so hooking it alone covers both.
extern "C" REX_FUNC(D3D_SetViewport) {
  if (!Native()) {
    __imp__D3D_SetViewport(ctx, base);
    return;
  }
  rr::GuestViewport viewport{};
  viewport.x = uint32_t(int32_t(ctx.f1.f64));
  viewport.y = uint32_t(int32_t(ctx.f2.f64));
  viewport.width = uint32_t(int32_t(ctx.f3.f64));
  viewport.height = uint32_t(int32_t(ctx.f4.f64));
  viewport.minZ = float(ctx.f5.f64);
  viewport.maxZ = float(ctx.f6.f64);
  rr::SetViewport(ghp::ToHost<GuestDevice>(ctx.r3.u32), &viewport);
}

// D3DDevice_SetScissorRect(pDevice, pRect) @0x822FF050. pRect is a guest RECT
// shaped exactly like GuestRect (left/top/right/bottom).
extern "C" REX_FUNC(D3DDevice_SetScissorRect) {
  if (!Native()) {
    __imp__D3DDevice_SetScissorRect(ctx, base);
    return;
  }
  auto* rect = ctx.r4.u32 != 0 ? ghp::ToHost<rr::GuestRect>(ctx.r4.u32) : nullptr;
  if (rect == nullptr) {
    // A null RECT means "scissor = the whole render target". rr::SetScissorRect
    // dereferences its rect, and the tracked rect is only consulted when
    // scissor testing is enabled, so dropping the call leaves the last rect in
    // place -- which is what the disabled-scissor path already ignores.
    return;
  }
  rr::SetScissorRect(ghp::ToHost<GuestDevice>(ctx.r3.u32), rect);
}

// ---------------------------------------------------------------------------
// Draw dispatch. Full replacements: the guest submitters walk the bound
// D3DVertexDeclaration and vertex/index buffer headers to build fetch constants
// and flush pending state (D3D::SetPending_Shaders / SetPending_FetchConstants),
// and those objects are ours now, with host members from +0x40 on -- the walk
// reads garbage and faults (observed: read AV of guest 0x38D83030 inside
// D3DDevice_DrawVertices). The original still runs while a command buffer is
// being recorded, so the guest's own compiled clone stays structurally valid for
// Task 8's replay path; nothing calls RenderQueue::BeginRecording yet.
//
// The complete set of guest draw emitters is the nine callers of
// D3D::SetPending_FetchConstants (0x8230FA40): D3DDevice_DrawIndexedVertices,
// D3DDevice_DrawVertices, D3DDevice_BeginVertices, sub_8234DAE8 (the indexed
// BeginVertices that DrawIndexedVerticesUP uses), D3DDevice_RunCommandBuffer
// (Task 8), three command-buffer state flushers that take no primitive at all
// (sub_823712F8 / sub_826E35F8 / sub_826E4050), and SetPendingState_NoInline.
// There is no FM4 counterpart to FM2's DrawIndexedVertices_WithVertexFormatSetup.
//
// Draws submitted on a device other than the latched DeviceType=1 one belong to
// the command-buffer devices; they are counted and skipped (Task 8 owns replay).
//
// There is deliberately no FM2P-style DrainDeferredDrawShaderConstants call in
// front of these draws. FM2P needs one because FM2 links
// D3DDevice_GpuBeginShaderConstantF4 (sub_82803358), which hands the caller a
// ring pointer it fills *after* the call, so those values never enter the
// device's constant files. FM4 does not link that function: it has no symbol,
// none of the fifteen callers of D3D::CDevice::BeginRingBig (0x823913D0) has its
// shape (BeginRingBig(dev, 4*count+5), three 0x80000000 stores, a 0xC0002D00
// packet word), and 0x2D00 does not occur as an immediate anywhere in the image.
// Every FM4 shader-constant write therefore lands in the device's own float
// files at +0x780 (VS) / +0x1780 (PS) -- exactly what the draw bodies above hand
// to SetPending_AluConstants -- and QueueDrawStateSnapshots already reads them
// every draw. Nothing to drain, and nothing that can go stale.
// ---------------------------------------------------------------------------

namespace {

// Null unless `device` is the latched main device.
GuestDevice* DrawDevice(uint32_t device) {
  auto* host = ghp::ToHost<GuestDevice>(device);
  if (host != nullptr && host != rr::GetActiveGuestDevice()) {
    Bump(kDrawOtherDevice);
    return nullptr;
  }
  return host;
}

}  // namespace

// D3DDevice_DrawVertices(pDevice, PrimitiveType, StartVertex, VertexCount)
// @0x8233AC60. FM4 has no separate public wrapper -- game code calls this body
// directly (26 xrefs, e.g. `li r4,8; li r5,0; mulli r6,r11,3` at 0x828749D0 for
// a RECTLIST). Confirmed non-indexed: it emits VGT_DRAW_INITIATOR with source
// select AUTO_INDEX (`| 0x80` at 0x8233B01C) and never reads an index buffer.
extern "C" REX_FUNC(D3DDevice_DrawVertices) {
  if (!Native()) {
    __imp__D3DDevice_DrawVertices(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t primitiveType = ctx.r4.u32;
  const uint32_t startVertex = ctx.r5.u32;
  const uint32_t vertexCount = ctx.r6.u32;
  if (rr::RenderQueue::IsRecording()) {
    __imp__D3DDevice_DrawVertices(ctx, base);
  }
  ctx.r3.u64 = 0;
  if (auto* dev = DrawDevice(device)) {
    Bump(kDrawNonIdx);
    rr::DrawVertices(dev, primitiveType, startVertex, vertexCount);
  }
}

// D3DDevice_DrawIndexedVertices(pDevice, PrimitiveType, BaseVertexIndex,
// StartIndex, IndexCount) @0x82311080 -- the XDK signature, typed in ida40.
extern "C" REX_FUNC(D3DDevice_DrawIndexedVertices) {
  fm4::gpu::TraceOnDrawIndexed(/*up=*/false);
  if (!Native()) {
    __imp__D3DDevice_DrawIndexedVertices(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t primitiveType = ctx.r4.u32;
  const int32_t baseVertexIndex = int32_t(ctx.r5.u32);
  const uint32_t startIndex = ctx.r6.u32;
  const uint32_t indexCount = ctx.r7.u32;
  if (rr::RenderQueue::IsRecording()) {
    __imp__D3DDevice_DrawIndexedVertices(ctx, base);
  }
  ctx.r3.u64 = 0;
  if (auto* dev = DrawDevice(device)) {
    Bump(kDrawIdx);
    rr::DrawIndexedVertices(dev, primitiveType, baseVertexIndex, startIndex, indexCount);
  }
}

// D3DDevice_DrawVerticesUP(pDevice, PrimitiveType, VertexCount,
// pVertexStreamZeroData, VertexStreamZeroStride) @0x822B95D8. The original is
//     void* p = D3DDevice_BeginVertices(dev, prim, count, stride);
//     if (p) { CopyToWriteCombinedMemory(p, src, count*stride);
//              dev->m_pRing = *(dev + 0x3604); }
// so replacing it outright skips BeginVertices entirely -- both the ring
// reservation and the +0x3604 restore -- and the vertex data is read straight
// from the guest pointer instead. DrawUserPointerVertices copies on this thread.
extern "C" REX_FUNC(D3DDevice_DrawVerticesUP) {
  if (!Native()) {
    __imp__D3DDevice_DrawVerticesUP(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t primitiveType = ctx.r4.u32;
  const uint32_t vertexCount = ctx.r5.u32;
  const uint32_t vertexData = ctx.r6.u32;
  const uint32_t stride = ctx.r7.u32;
  if (rr::RenderQueue::IsRecording()) {
    __imp__D3DDevice_DrawVerticesUP(ctx, base);
  }
  if (auto* dev = DrawDevice(device)) {
    Bump(kDrawUP);
    rr::DrawUserPointerVertices(dev, primitiveType, vertexCount,
                                ghp::ToHost<const uint8_t>(vertexData), stride);
  }
}

// D3DDevice_DrawIndexedVerticesUP(pDevice, PrimitiveType, MinVertexIndex,
// NumVertices, IndexCount, pIndexData, IndexDataFormat, pVertexStreamZeroData,
// VertexStreamZeroStride) @0x822D42A8 -- nine arguments, the ninth on the
// caller's stack at r1+0x54 (`lwz r30, 0xB0+arg_54(r1)` at 0x822D42B4). It has
// exactly one code caller in the image (sub_822C4E80) and the renderer has no
// indexed user-pointer path, so it stays a no-op under the native path and the
// counter says whether that ever costs geometry.
extern "C" REX_FUNC(D3DDevice_DrawIndexedVerticesUP) {
  fm4::gpu::TraceOnDrawIndexed(/*up=*/true);
  if (!Native()) {
    __imp__D3DDevice_DrawIndexedVerticesUP(ctx, base);
    return;
  }
  Bump(kDrawIdxUP);
  static std::atomic<bool> warned{false};
  if (!warned.exchange(true, std::memory_order_relaxed)) {
    REXLOG_WARN(
        "fm4render: D3DDevice_DrawIndexedVerticesUP reached; indexed user-pointer draws are "
        "dropped on the native path");
  }
  ctx.r3.u64 = 0;
}

// D3DDevice_BeginVertices(pDevice, PrimitiveType, VertexCount, VertexSize)
// @0x8234D278 returns the ring pointer the guest then fills with vertices, so
// it cannot be a plain no-op -- but its body flushes pending state through the
// same SetPending_* table that faults on our resources (observed: read AV of
// guest 0x38E22030, D3DDevice_BeginVertices -> SetPending_Shaders ->
// SetPending_HiZEnable). Hand back a scratch block instead.
//
// D3DDevice_DrawVerticesUP no longer reaches this hook (it is a full
// replacement above), but thirteen other call sites do -- library helpers and
// XGRAPHICS utilities that fill the returned block and rely on BeginVertices
// having already emitted the draw. Those produce no geometry on the native
// path; beginVerts in the [d3dtrace] line counts them, and it was 300 per 300
// frames before this task purely because of the DrawVerticesUP wait animation,
// so the expectation is that it now reads ~0.
// ponytail: one shared grow-only block, no per-call lifetime -- correct while
// the block's contents are never consumed.
extern "C" REX_FUNC(D3DDevice_BeginVertices) {
  fm4::gpu::TraceOnBeginVertices();
  if (!Native()) {
    __imp__D3DDevice_BeginVertices(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t bytes = std::max(ctx.r5.u32 * ctx.r6.u32, 0x1000u);
  static std::mutex scratchMutex;
  static uint32_t scratchAddr = 0;
  static uint32_t scratchSize = 0;
  {
    // r3 must be read under the same lock: another thread growing the block
    // frees the address this one is about to hand back to the guest.
    std::lock_guard lock(scratchMutex);
    if (bytes > scratchSize) {
      ghp::GuestFreeRaw(scratchAddr);
      scratchAddr = ghp::GuestAllocRaw(bytes, 0x10);
      scratchSize = scratchAddr != 0 ? bytes : 0;
    }
    ctx.r3.u64 = scratchAddr;
  }

  // The real BeginVertices reserves ring space and parks the post-vertex ring
  // pointer at device+0x3604. Its callers restore m_pRing from that field once
  // they have copied their vertices -- D3DDevice_DrawVerticesUP (0x822B95D8) is
  //     lwz r11, 0x3604(r31)   ; 0x822B9610
  //     stw r11, 0x30(r31)     ; 0x822B9614
  // and sub_8234DAE8 / D3DDevice_DrawIndexedVerticesUP do the same. Handing back
  // a scratch block instead never writes 0x3604, so it stays 0 and
  // that restore stores 0 into m_pRing. After that EVERY ring emitter faults
  // writing guest 0x00000004, because they all guard with
  // `if (m_pRing > m_pRingLimit) RingMakeSpace()` and 0 is never greater --
  // this is the D3D::SetSurfaceClip / D3DDevice_SetShaderGPRAllocation crash.
  // We do not move the ring, so publish the unchanged m_pRing and the caller's
  // restore becomes a no-op.
  if (auto* ring = ghp::ToHost<rex::be<uint32_t>>(device + kGuestRingPointerOffset)) {
    if (auto* saved = ghp::ToHost<rex::be<uint32_t>>(device + kGuestBeginVerticesRingSaveOffset)) {
      *saved = ring->get();
    }
  }
}

// D3DDevice_Resolve @0x822E2120 is a 0x4C-byte wrapper that shuffles args into
// sub_82348DC8. Hook the public wrapper; dest is r6 (ida40 prototype).
//
// void D3DDevice_Resolve(pDevice, Flags, pSourceRect, pDestTexture, pDestPoint,
// DestLevel, DestSliceOrFace, pClearColor, ClearZ, ClearStencil, pParameters)
extern "C" REX_FUNC(D3DDevice_Resolve) {
  fm4::gpu::TraceOnResolve(ctx.r4.u32);
  if (!Native()) {
    __imp__D3DDevice_Resolve(ctx, base);
    return;
  }
  Bump(kResolve);
  const uint32_t device = ctx.r3.u32;
  const uint32_t flags = ctx.r4.u32;
  const uint32_t srcRectVa = ctx.r5.u32;
  const uint32_t destTexture = ctx.r6.u32;
  const uint32_t destPointVa = ctx.r7.u32;
  const uint32_t pClearColor = ctx.r10.u32;
  const float clearZ = float(ctx.f1.f64);

  constexpr uint32_t kResolveClearColor = 0x100;
  constexpr uint32_t kResolveClearDepthStencil = 0x200;
  uint32_t postClearFlags = 0;
  float postClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  if ((flags & kResolveClearColor) != 0) {
    postClearFlags |= rr::D3DCLEAR_TARGET;
    if (pClearColor != 0) {
      ReadGuestVec4(pClearColor, postClearColor);
    }
  }
  if ((flags & kResolveClearDepthStencil) != 0) {
    postClearFlags |= rr::D3DCLEAR_ZBUFFER | rr::D3DCLEAR_STENCIL;
  }

  if (destTexture == 0) {
    if (postClearFlags != 0) {
      rr::Clear(nullptr, postClearFlags, postClearColor, clearZ);
    }
    ctx.r3.u64 = device;
    return;
  }

  auto* destHost = ghp::ToHost<GuestResource>(destTexture);
  rr::GuestBaseTexture* hostDest = nullptr;
  uint32_t dataBase = 0;
  if (rr::IsFm4Resource(destHost)) {
    hostDest = static_cast<rr::GuestBaseTexture*>(destHost);
  } else {
    hostDest = ResolveBoundColorTarget(destTexture);
    dataBase = ReadGuestU32(destTexture + rr::kGuestTextureFetchConstantOffset + 4u) &
               0x1FFFFFFFu & ~0xFFFu;
  }

  if (hostDest == nullptr || hostDest->texture == nullptr) {
    if (postClearFlags != 0) {
      rr::Clear(nullptr, postClearFlags, postClearColor, clearZ);
    }
    ctx.r3.u64 = device;
    return;
  }

  const bool destIsBlockCompressed = hostDest->format >= plume::RenderFormat::BC1_TYPELESS &&
                                     hostDest->format <= plume::RenderFormat::BC7_UNORM_SRGB;
  if (destIsBlockCompressed) {
    static std::atomic<uint32_t> bcSkip{0};
    if (bcSkip.fetch_add(1, std::memory_order_relaxed) < 24) {
      REXLOG_WARN("fm4render: Resolve skipping BC dest 0x{:08X} fmt={}", destTexture,
                  int(hostDest->format));
    }
    if (postClearFlags != 0) {
      rr::Clear(nullptr, postClearFlags, postClearColor, clearZ);
    }
    ctx.r3.u64 = device;
    return;
  }

  if (dataBase != 0) {
    rr::RegisterResolveSurfaceAperture(dataBase, hostDest);
  }

  rr::ResolveToTexture(hostDest,
                       destPointVa != 0 ? ghp::ToHost<rr::GuestPoint>(destPointVa) : nullptr,
                       srcRectVa != 0 ? ghp::ToHost<rr::GuestRect>(srcRectVa) : nullptr,
                       postClearFlags, postClearColor, clearZ);
  ctx.r3.u64 = device;
}

// ---------------------------------------------------------------------------
// Command buffers. FM4 has no XDK D3DDevice_EndCommandBuffer: live callers
// (sub_82946C00, sub_8237CC08, ...) flush pending state through sub_826E4050
// and then D3DCommandBuffer_CreateClone. Recordings therefore close at clone.
// ---------------------------------------------------------------------------

extern "C" REX_FUNC(D3DDevice_BeginCommandBuffer) {
  fm4::gpu::TraceOnBeginCommandBuffer();
  if (Native()) {
    if (rr::RenderQueue::IsRecording()) {
      rr::RenderQueue::EndRecording();
      static std::atomic<uint32_t> discarded{0};
      if (discarded.fetch_add(1, std::memory_order_relaxed) == 0) {
        REXLOG_WARN("fm4render: BeginCommandBuffer discarded an uncloned recording");
      }
    }
    rr::RenderQueue::BeginRecording();
  }
  __imp__D3DDevice_BeginCommandBuffer(ctx, base);
}

extern "C" REX_FUNC(D3DCommandBuffer_CreateClone) {
  __imp__D3DCommandBuffer_CreateClone(ctx, base);
  if (Native()) {
    rr::RenderQueue::EndRecording();
    rr::RenderQueue::BindPendingRecording(ctx.r3.u32);
  }
}

extern "C" REX_FUNC(D3DDevice_RunCommandBuffer) {
  fm4::gpu::TraceOnRunCommandBuffer();
  if (!Native()) {
    __imp__D3DDevice_RunCommandBuffer(ctx, base);
    return;
  }
  const uint32_t cloneAddress = ctx.r4.u32;
  Bump(kRunCB);
  if (!rr::RenderQueue::ReplayRecording(cloneAddress, nullptr, 0)) {
    Bump(kRunCBMiss);
  }
}

// ---------------------------------------------------------------------------
// Predicated tiling. D3DDevice_BeginTiling (0x82374660) takes
// (pDevice, Flags, Count, pTileRects, const D3DVECTOR4* pClearColor, float
// ClearZ, DWORD ClearStencil). ClearZ occupies f1 and reserves r8, so
// ClearStencil is r9. There is no XDK EndTiling; Swap (and a nested
// BeginTiling) close the host pass.
// ---------------------------------------------------------------------------

extern "C" REX_FUNC(D3DDevice_BeginTiling) {
  fm4::gpu::TraceOnBeginTiling();
  if (!Native()) {
    __imp__D3DDevice_BeginTiling(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t flags = ctx.r4.u32;
  const uint32_t count = ctx.r5.u32;
  const uint32_t rects = ctx.r6.u32;
  const uint32_t clearColorVa = ctx.r7.u32;
  const float clearZ = float(ctx.f1.f64);
  const uint32_t clearStencil = ctx.r9.u32;
  rr::EndTilingPass();
  Bump(kBeginTiling);
  float rgba[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  if (clearColorVa != 0) {
    ReadGuestVec4(clearColorVa, rgba);
  }
  rr::BeginTilingPass(ghp::ToHost<const uint32_t>(rects), count, flags,
                      clearColorVa != 0 ? rgba : nullptr, clearZ, clearStencil);
  // Original writes CDevice tiling flags / pending masks. SetSurfaceClip is a
  // native no-op so it cannot store through a null ring.
  __imp__D3DDevice_BeginTiling(ctx, base);
  ctx.r3.u64 = device;
}

// D3DQuery_GetData(pThis, pData, Size, Flags) @0x8239B2C0 -> HRESULT.
// Type 8 (event) writes *pData=1; type 9 (occlusion) wants a pixel count.
// A never-ready query stalls the object pass; a zero count culls everything.
// Report complete and fully visible.
extern "C" REX_FUNC(D3DQuery_GetData) {
  if (!Native()) {
    __imp__D3DQuery_GetData(ctx, base);
    fm4::gpu::TraceOnQueryGetData(ctx.r3.u32);
    return;
  }
  Bump(kQueryGet);
  const uint32_t pData = ctx.r4.u32;
  const uint32_t size = ctx.r5.u32;
  if (pData != 0 && size >= 4) {
    if (auto* out = ghp::ToHost<rex::be<uint32_t>>(pData)) {
      *out = 0x0000FFFFu;
    }
  }
  ctx.r3.u64 = 0;  // S_OK
  fm4::gpu::TraceOnQueryGetData(0);
}

// ---------------------------------------------------------------------------
// Present and clear.
//
// D3DDevice_Swap(pDevice, pFrontBuffer, pParameters) @0x8237FA28. r4 is a
// D3DBaseTexture*, not FM2's VdSwap descriptor: the body copies
// pFrontBuffer->Format (18 bytes) and uses Format.dword[1] & 0xFFFFF000 as the
// front-buffer page.
// ---------------------------------------------------------------------------

extern "C" REX_FUNC(D3DDevice_Swap) {
  fm4::gpu::TraceOnSwap();
  if (!Native()) {
    __imp__D3DDevice_Swap(ctx, base);
    return;
  }
  Bump(kSwap);
  const uint32_t device = ctx.r3.u32;
  rr::EndTilingPass();
  rr::GuestBaseTexture* source = nullptr;
  const uint32_t frontBuffer = ctx.r4.u32;
  if (frontBuffer != 0) {
    auto* host = ghp::ToHost<GuestResource>(frontBuffer);
    if (rr::IsFm4Resource(host)) {
      source = static_cast<rr::GuestBaseTexture*>(host);
    } else {
      const uint32_t page =
          ReadGuestU32(frontBuffer + rr::kGuestTextureFetchConstantOffset + 4u) & 0x1FFFFFFFu &
          ~0xFFFu;
      source = rr::LookupResolveSurfaceAperture(page);
      if (source == nullptr) {
        source = rr::TranslateGuestTexture(host, false);
      }
    }
  }
  if (source != nullptr && source->texture != nullptr) {
    Bump(kPresentSrc);
    rr::SetFrontbufferPresentSource(source);
  } else {
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true, std::memory_order_relaxed)) {
      REXLOG_WARN(
          "fm4render: Swap front buffer 0x{:08X} had no host texture; presenting the last "
          "bound render target instead",
          frontBuffer);
    }
  }
  fm4::render::OnResourceTraceFrame();
  fm4::gpu::Video::Present();
  // Original Swap is void and leaves a leftover in r3 (usually a ring pointer).
  // Zeroing it made later `lbz …, 0x2B3D(r3)` fault as a read of guest 0x2B3D.
  ctx.r3.u64 = device;
}

// D3DDevice_Clear(pDevice, Count, pRects, Flags, Color, Z, Stencil, EDRAMClear)
// @0x8236D840. Flags=r6, Color=r7 (D3DCOLOR), Z in f1, Stencil=r9.
extern "C" REX_FUNC(D3DDevice_Clear) {
  if (!Native()) {
    __imp__D3DDevice_Clear(ctx, base);
    return;
  }
  const uint32_t flags = ctx.r6.u32;
  const uint32_t color = ctx.r7.u32;
  const float z = float(ctx.f1.f64);
  float rgba[4] = {((color >> 16) & 0xFF) / 255.0f, ((color >> 8) & 0xFF) / 255.0f,
                   (color & 0xFF) / 255.0f, ((color >> 24) & 0xFF) / 255.0f};
  if ((flags & rr::D3DCLEAR_TARGET) != 0) {
    ::Video::SetFallbackClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true, std::memory_order_relaxed)) {
      REXLOG_INFO("native gpu: first guest clear colour 0x{:08X}", color);
    }
  }
  rr::Clear(ghp::ToHost<GuestDevice>(ctx.r3.u32), flags, rgba, z);
  ctx.r3.u64 = 0;
}

// XDK entry points with no surviving host state. Fall through when native is
// off so the xenos plugin is unaffected.
#define FM4_D3D_GPU_NOOP(name)                \
  extern "C" REX_FUNC(name) {                 \
    if (!Native()) __imp__##name(ctx, base); \
  }

FM4_D3D_GPU_NOOP(D3DDevice_SetShaderGPRAllocation);
// D3D::SetSurfaceClip @0x82310530 writes PM4 through m_pRing. Native BeginTiling
// / EndTiling call it; a null ring was the write of guest 0x00000004.
FM4_D3D_GPU_NOOP(sub_82310530);

// Original FlushHiZStencil writes a PM4 packet and then
// `BYTE1(pDevice[1].m_ReferenceCount) &= ~0x04` -- that's CDevice+0x2B3D.
// Skipping the whole body left that flags byte stale; after BeginTiling the
// next guest load of it with a clobbered r3 was the 0x00002B3D AV. Keep the
// flag update, skip the ring write (nothing consumes PM4 on this path).
extern "C" REX_FUNC(D3DDevice_FlushHiZStencil) {
  if (!Native()) {
    __imp__D3DDevice_FlushHiZStencil(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  if (device != 0) {
    if (auto* flags = ghp::ToHost<uint8_t>(device + 0x2B3D)) {
      *flags = static_cast<uint8_t>(*flags & ~0x04u);
    }
  }
}

FM4_D3D_GPU_NOOP(D3DDevice_SetScreenExtentQueryMode);
FM4_D3D_GPU_NOOP(D3DDevice_NuiMetaData);
FM4_D3D_GPU_NOOP(D3D_ExecuteLowPriCommandBuffer);
FM4_D3D_GPU_NOOP(D3D_InitializeAsyncCommandBuffers);

#undef FM4_D3D_GPU_NOOP
