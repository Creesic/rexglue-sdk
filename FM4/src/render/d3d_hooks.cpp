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

#include <atomic>
#include <cstdint>
#include <cstdio>

#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/types.h>

#include "gpu/d3d_guest.h"
#include "gpu/native_video.h"
#include "render/guest_device.h"
#include "render/guest_heap.h"
#include "render/guest_resources.h"
#include "render/render_state.h"

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
  kPassthrough,
  kCounterCount
};
constexpr const char* kCounterNames[kCounterCount] = {"vb",  "ib", "tex",  "surf",   "decl",
                                                      "vs",  "ps", "lock", "unlock", "passthru"};
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

}  // namespace

namespace fm4::render {

void OnResourceTraceFrame() {
  static std::atomic<uint32_t> frames{0};
  if ((frames.fetch_add(1, std::memory_order_relaxed) % 300) != 299) {
    return;
  }
  char line[256];
  int n = std::snprintf(line, sizeof(line), "[fm4render] frames=300");
  for (size_t i = 0; i < kCounterCount; ++i) {
    n += std::snprintf(line + n, sizeof(line) - n, " %s=%u", kCounterNames[i],
                       g_counters[i].exchange(0, std::memory_order_relaxed));
  }
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
// false and the original body runs (counted as "passthru").
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
    Bump(kPassthrough);
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
    Bump(kPassthrough);
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
    Bump(kPassthrough);
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
    Bump(kPassthrough);
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
    Bump(kPassthrough);
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
    Bump(kPassthrough);
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
// and 14 are confirmed from FM4's own creation functions and GetType's body;
// 5/6/7 are the XDK enum's.
extern "C" REX_FUNC(D3DResource_GetType) {
  if (!Native()) {
    __imp__D3DResource_GetType(ctx, base);
    return;
  }
  auto* host = OurResource(ctx.r3.u32);
  if (host == nullptr) {
    Bump(kPassthrough);
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
    case fm4::render::ResourceType::VolumeTexture: type = 14; break;
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
    Bump(kPassthrough);
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
    Bump(kPassthrough);
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
