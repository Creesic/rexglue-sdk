// render/d3d_hooks.cpp
//
// Phase 1 (device + present bring-up): hooks D3DDevice_ClearF and
// D3DDevice_Swap as full plume replacements, and lets
// FM2_D3D_TryPresentAndUpdateStatus's original guest body keep running
// (status/bookkeeping, not a GPU call) with Video::Present() layered on top.
//
// Video::Init/Shutdown are driven from Fm2App::OnPreLaunchModule/OnShutdown
// (see fm2_app.h), not from a guest-function hook: FM2_D3D_InitGlobalDeviceSingleton
// does not need to run under the native renderer, and the app-lifecycle hook is
// the same window/device-creation order the guest function would have driven.
//
// Phase 2 (resource hooks): buffer/texture/surface/vertex-declaration
// creation and lock/unlock. Every guest-facing D3D9 wrapper function that
// calls one of these primitives (e.g. FM2_D3D_CreateTextureWrapper calling
// D3DDevice_CreateTexture) is a thin, unhooked call-through -- hooking the
// primitive here is enough; the linker's strong-symbol replacement covers
// every caller automatically, no separate wrapper hooks needed. Same reason
// FM2_Image_ParseDDSFromMemory and the D3DX texture-from-memory pipeline
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
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/types.h>

#include "render/guest_device.h"
#include "render/guest_heap.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_queue.h"
#include "render/render_state.h"
#include "render/video.h"

using fm2::render::GuestBuffer;
using fm2::render::GuestDevice;
using fm2::render::GuestLockedRect;
using fm2::render::GuestResource;
using fm2::render::GuestSurface;
using fm2::render::GuestSurfaceDesc;
using fm2::render::GuestVertexElement;

namespace fm2::render {
GuestBuffer* CreateVertexBuffer(uint32_t length);
GuestBuffer* CreateIndexBuffer(uint32_t length, uint32_t format);
uint32_t LockVertexBuffer(GuestBuffer* buffer, uint32_t flags);
void UnlockVertexBuffer(GuestBuffer* buffer);
uint32_t LockIndexBuffer(GuestBuffer* buffer, uint32_t flags);
void UnlockIndexBuffer(GuestBuffer* buffer);
GuestTexture* CreateTexture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels,
                            uint32_t usage, uint32_t format, uint32_t pool, uint32_t type);
GuestSurface* CreateSurface(uint32_t width, uint32_t height, uint32_t format, uint32_t multiSample);
void LockRect(GuestBaseTexture* texture, uint32_t* outPitch, uint32_t* outBits);
void UnlockGuestResource(GuestResource* resource);
GuestVertexDeclaration* CreateVertexDeclaration(const GuestVertexElement* guestElements);
void GetSurfaceDesc(const GuestSurface* surface, GuestSurfaceDesc* desc);
GuestTexture* LoadTextureFromMemory(const uint8_t* data, uint32_t size);
GuestShader* CreateVertexShader(const uint32_t* function);
GuestShader* CreatePixelShader(const uint32_t* function);
GuestShader* LookupShaderAlias(uint32_t guestAddress);
}  // namespace fm2::render

namespace {

uint32_t ReadGuestU32At(uint32_t guestAddress) {
  if (guestAddress == 0)
    return 0;
  auto* p = fm2::ghp::ToHost<const rex::be<uint32_t>>(guestAddress);
  return p != nullptr ? p->get() : 0;
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
  float rgba[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  if (color != nullptr) {
    for (int i = 0; i < 4; ++i) {
      rgba[i] = std::bit_cast<float>(std::byteswap(color[i]));
    }
  }
  Video::SetFallbackClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
  fm2::render::Clear(device, flags, rgba, z);
}

// D3DDevice_Swap: the real per-frame present trigger for FM2 (confirmed
// against this same guest binary by the reference repo's own investigation;
// FM2_D3D_TryPresentAndUpdateStatus never actually fires on the live present
// path). The original guest body kicks the real Xenos GPU ring buffer, which
// does not exist under the native renderer, so it must not run here; this
// hook fully replaces it.
//
// arg4 holds the VdSwap-style present descriptor: frontbuffer D3D9 fetch is
// at arg4+28; dword1 & page mask is the frontbuffer base. Prefer a resolve
// destination registered at that base (composited display) over sticky RTs.
void Swap(uint32_t /*commandBuffer*/, uint32_t arg4, uint32_t /*arg5*/) {
  static uint64_t swapCallCount = 0;
  ++swapCallCount;
  if (swapCallCount % 300 == 1) {
    REXGPU_INFO("D3DDevice_Swap hook: called {} time(s) so far", swapCallCount);
  }

  fm2::render::GuestBaseTexture* presentSource = nullptr;
  const char* kind = "none";
  if (arg4 != 0) {
    const uint32_t fbBase = ReadGuestU32At(arg4 + 28u + 4u) & 0x1FFFFFFFu & ~0xFFFu;
    presentSource = fm2::render::LookupResolveSurfaceAperture(fbBase);
    if (presentSource != nullptr && presentSource->texture != nullptr) {
      kind = "aperture";
      fm2::render::SetFrontbufferPresentSource(presentSource);
    } else {
      presentSource = nullptr;
    }
    if (swapCallCount % 300 == 1) {
      REXGPU_INFO("D3DDevice_Swap: fbBase=0x{:08X} presentKind={} src={}x{}", fbBase, kind,
                  presentSource != nullptr ? presentSource->width : 0,
                  presentSource != nullptr ? presentSource->height : 0);
    }
  }

  fm2::render::PrepareFramePresent();
  Video::Present();
  // Frame bookkeeping is covered by Present's BeginCommandList /
  // OnRecordingFrameReady. An extra sync BeginRenderStateFrame Run here
  // contended with TranslateGuestTexture and worsened post-Swap freezes.
}

}  // namespace

REX_IMPORT(__imp__FM2_D3D_TryPresentAndUpdateStatus, g_origTryPresentAndUpdateStatus,
           void(uint32_t));

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
REX_HOOK(D3DDevice_Swap, Swap);
REX_HOOK(FM2_D3D_TryPresentAndUpdateStatus, PresentAndUpdateStatus);

// ---------------------------------------------------------------------------
// Phase 2: resource creation / lock / unlock. Pure replacements -- the
// returned guest address is the native object's own address (GuestNew places
// it inside guest memory), no XDK shadow object, no alias table.
// ---------------------------------------------------------------------------

namespace {

namespace rr = fm2::render;
namespace ghp = fm2::ghp;

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
                           const void* /*parameters*/) {
  return ghp::ToGuest(rr::CreateSurface(width, height, format, multiSample));
}

uint32_t CreateVertexDeclarationHook(const GuestVertexElement* elements) {
  return ghp::ToGuest(rr::CreateVertexDeclaration(elements));
}

// D3DDevice_CreateVertexShader/CreatePixelShader take the raw ShaderContainer
// microcode pointer directly -- these two addresses were mislabeled in the
// manifest as unrelated GPU-memory-block allocators (FM2_Render_AllocGpuPassMemoryBlock
// / FM2_D3D_CreateGpuMemoryBlock) despite already being correctly renamed in
// IDA; fixed in fm2_manifest.toml. Without this fix no vertex/pixel shader
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
REX_IMPORT(__imp__FM2_D3DVertexBuffer_Lock, g_origVertexBufferLock,
           uint32_t(uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_LockGpuBufferRaw, g_origIndexBufferLock,
           uint32_t(uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3DSurface_LockRect, g_origSurfaceLockRect,
           void(uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3DTexture_LockRect, g_origTextureLockRect,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3DResource_UnlockResource, g_origUnlockResource,
           void(uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3DSurface_GetDesc, g_origSurfaceGetDesc, void(uint32_t, uint32_t));
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
  rr::LockRect(texture, &pitch, &bits);
  if (auto* lockedRect = ghp::ToHost<GuestLockedRect>(lockedRectAddr)) {
    lockedRect->pitch = static_cast<int32_t>(pitch);
    lockedRect->bits = bits;
  }
}

// D3D9 textures lock through a *different* entry point than surfaces
// (IDirect3DTexture9::LockRect takes a mip Level; IDirect3DSurface9 doesn't).
// This guest function was previously misnamed FM2_AudioRender_SubmitFrontBufferPath
// in the manifest/IDA -- confirmed via decompile to be the real texture-lock
// leaf (renamed to FM2_D3DTexture_LockRect). Missing this hook left the
// original body running against our GuestTexture's non-XDK layout, producing
// a garbage lock pointer that crashed later in TileSurface/FM2_MemcpyAligned.
void TextureLockRectHook(uint32_t textureAddr, uint32_t level, uint32_t lockedRectAddr,
                         uint32_t rectAddr, uint32_t flags) {
  auto* texture = ghp::ToHost<rr::GuestBaseTexture>(textureAddr);
  if (!rr::IsFm2Resource(texture)) {
    g_origTextureLockRect(textureAddr, level, lockedRectAddr, rectAddr, flags);
    return;
  }
  uint32_t pitch = 0, bits = 0;
  rr::LockRect(texture, &pitch, &bits);
  if (auto* lockedRect = ghp::ToHost<GuestLockedRect>(lockedRectAddr)) {
    lockedRect->pitch = static_cast<int32_t>(pitch);
    lockedRect->bits = bits;
  }
}

void UnlockResourceHook(uint32_t resourceAddr, uint32_t base, uint32_t mip) {
  auto* resource = ghp::ToHost<GuestResource>(resourceAddr);
  if (!rr::IsFm2Resource(resource)) {
    g_origUnlockResource(resourceAddr, base, mip);
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

// FM2_D3D_CreateTextureFromMemoryBuffer(textureHolder, data, size): the
// original guest body hands off to the D3DX texture-from-memory pipeline,
// whose internal tiling step (TileSurface) reads a raw GPU-memory-address
// field out of the D3D9 texture object it creates -- our GuestTexture has no
// such field, so letting that pipeline run corrupts memory (confirmed via a
// crash in FM2_MemcpyAligned during boot). This hook bypasses it completely:
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

REX_HOOK(FM2_D3DDevice_CreateVertexBuffer, CreateVertexBufferHook);
REX_HOOK(FM2_D3DDevice_CreateIndexBuffer, CreateIndexBufferHook);
REX_HOOK(FM2_D3DDevice_CreateTexture, CreateTextureHook);
REX_HOOK(FM2_D3DDevice_CreateSurface, CreateSurfaceHook);
REX_HOOK(FM2_D3DDevice_CreateVertexDeclaration, CreateVertexDeclarationHook);
REX_HOOK(D3DDevice_CreateVertexShader, CreateVertexShaderHook);
REX_HOOK(D3DDevice_CreatePixelShader, CreatePixelShaderHook);
REX_HOOK(FM2_D3DVertexBuffer_Lock, VertexBufferLockHook);
REX_HOOK(FM2_D3D_LockGpuBufferRaw, IndexBufferLockHook);
REX_HOOK(FM2_D3DSurface_LockRect, SurfaceLockRectHook);
REX_HOOK(FM2_D3DTexture_LockRect, TextureLockRectHook);
REX_HOOK(FM2_D3DResource_UnlockResource, UnlockResourceHook);
REX_HOOK(FM2_D3DSurface_GetDesc, SurfaceGetDescHook);
REX_HOOK(FM2_D3D_CreateTextureFromMemoryBuffer, CreateTextureFromMemoryBufferHook);
REX_HOOK(D3DResource_AddRef, D3DResourceAddRefHook);    // @ 0x82369D90
REX_HOOK(D3DResource_Release, D3DResourceReleaseHook);  // @ 0x82369E08

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

#define FM2_RS_IMPORT(guestName, callableName) \
  REX_IMPORT(__imp__##guestName, callableName, void(uint32_t, uint32_t))

FM2_RS_IMPORT(D3DDevice_SetRenderState_AlphaBlendEnable, g_origRsAlphaBlendEnable);
FM2_RS_IMPORT(D3DDevice_SetRenderState_AlphaTestEnable, g_origRsAlphaTestEnable);
FM2_RS_IMPORT(D3DDevice_SetRenderState_AlphaRef, g_origRsAlphaRef);
FM2_RS_IMPORT(D3DDevice_SetRenderState_ZEnable, g_origRsZEnable);
FM2_RS_IMPORT(D3DDevice_SetRenderState_ZWriteEnable, g_origRsZWriteEnable);
FM2_RS_IMPORT(D3DDevice_SetRenderState_ZFunc, g_origRsZFunc);
FM2_RS_IMPORT(D3DDevice_SetRenderState_ColorWriteEnable, g_origRsColorWriteEnable);
FM2_RS_IMPORT(D3DDevice_SetRenderState_BlendOp, g_origRsBlendOp);
FM2_RS_IMPORT(D3DDevice_SetRenderState_SrcBlend, g_origRsSrcBlend);
FM2_RS_IMPORT(D3DDevice_SetRenderState_DestBlend, g_origRsDestBlend);
FM2_RS_IMPORT(D3DDevice_SetRenderState_SrcBlendAlpha, g_origRsSrcBlendAlpha);
FM2_RS_IMPORT(D3DDevice_SetRenderState_DestBlendAlpha, g_origRsDestBlendAlpha);

#undef FM2_RS_IMPORT

namespace {

void MirrorRenderState(uint32_t renderContext, uint32_t d3drs, uint32_t value) {
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  rr::SetRenderState(device, d3drs, value);
}

#define FM2_RS_HOOK(hookName, origCallable, d3drs) \
  void hookName(uint32_t device, uint32_t value) { \
    origCallable(device, value);                   \
    MirrorRenderState(device, d3drs, value);       \
  }

FM2_RS_HOOK(Fm2RsAlphaBlendEnable, g_origRsAlphaBlendEnable, rr::D3DRS_ALPHABLENDENABLE)
FM2_RS_HOOK(Fm2RsAlphaTestEnable, g_origRsAlphaTestEnable, rr::D3DRS_ALPHATESTENABLE)
FM2_RS_HOOK(Fm2RsAlphaRef, g_origRsAlphaRef, rr::D3DRS_ALPHAREF)
FM2_RS_HOOK(Fm2RsZEnable, g_origRsZEnable, rr::D3DRS_ZENABLE)
FM2_RS_HOOK(Fm2RsZWriteEnable, g_origRsZWriteEnable, rr::D3DRS_ZWRITEENABLE)
FM2_RS_HOOK(Fm2RsZFunc, g_origRsZFunc, rr::D3DRS_ZFUNC)
FM2_RS_HOOK(Fm2RsColorWriteEnable, g_origRsColorWriteEnable, rr::D3DRS_COLORWRITEENABLE)
FM2_RS_HOOK(Fm2RsBlendOp, g_origRsBlendOp, rr::D3DRS_BLENDOP)
FM2_RS_HOOK(Fm2RsSrcBlend, g_origRsSrcBlend, rr::D3DRS_SRCBLEND)
FM2_RS_HOOK(Fm2RsDestBlend, g_origRsDestBlend, rr::D3DRS_DESTBLEND)
FM2_RS_HOOK(Fm2RsSrcBlendAlpha, g_origRsSrcBlendAlpha, rr::D3DRS_SRCBLENDALPHA)
FM2_RS_HOOK(Fm2RsDestBlendAlpha, g_origRsDestBlendAlpha, rr::D3DRS_DESTBLENDALPHA)

#undef FM2_RS_HOOK

}  // namespace

REX_HOOK(D3DDevice_SetRenderState_AlphaBlendEnable, Fm2RsAlphaBlendEnable);
REX_HOOK(D3DDevice_SetRenderState_AlphaTestEnable, Fm2RsAlphaTestEnable);
REX_HOOK(D3DDevice_SetRenderState_AlphaRef, Fm2RsAlphaRef);
REX_HOOK(D3DDevice_SetRenderState_ZEnable, Fm2RsZEnable);
REX_HOOK(D3DDevice_SetRenderState_ZWriteEnable, Fm2RsZWriteEnable);
REX_HOOK(D3DDevice_SetRenderState_ZFunc, Fm2RsZFunc);
REX_HOOK(D3DDevice_SetRenderState_ColorWriteEnable, Fm2RsColorWriteEnable);
REX_HOOK(D3DDevice_SetRenderState_BlendOp, Fm2RsBlendOp);
REX_HOOK(D3DDevice_SetRenderState_SrcBlend, Fm2RsSrcBlend);
REX_HOOK(D3DDevice_SetRenderState_DestBlend, Fm2RsDestBlend);
REX_HOOK(D3DDevice_SetRenderState_SrcBlendAlpha, Fm2RsSrcBlendAlpha);
REX_HOOK(D3DDevice_SetRenderState_DestBlendAlpha, Fm2RsDestBlendAlpha);

// ---------------------------------------------------------------------------
// Clip planes.
// ---------------------------------------------------------------------------

REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane0Enable, g_origSetClipPlane0Enable,
           void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane1Enable, g_origSetClipPlane1Enable,
           void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane2Enable, g_origSetClipPlane2Enable,
           void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane3Enable, g_origSetClipPlane3Enable,
           void(uint32_t, uint32_t));

namespace {

void MirrorClipPlanes(uint32_t renderContext) {
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  rr::UpdateClipPlaneConstants(device);
}

void Fm2SetClipPlane0Enable(uint32_t renderContext, uint32_t value) {
  g_origSetClipPlane0Enable(renderContext, value);
  MirrorClipPlanes(renderContext);
}
void Fm2SetClipPlane1Enable(uint32_t renderContext, uint32_t value) {
  g_origSetClipPlane1Enable(renderContext, value);
  MirrorClipPlanes(renderContext);
}
void Fm2SetClipPlane2Enable(uint32_t renderContext, uint32_t value) {
  g_origSetClipPlane2Enable(renderContext, value);
  MirrorClipPlanes(renderContext);
}
void Fm2SetClipPlane3Enable(uint32_t renderContext, uint32_t value) {
  g_origSetClipPlane3Enable(renderContext, value);
  MirrorClipPlanes(renderContext);
}

}  // namespace

REX_HOOK(FM2_RenderContext_SetClipPlane0Enable, Fm2SetClipPlane0Enable);
REX_HOOK(FM2_RenderContext_SetClipPlane1Enable, Fm2SetClipPlane1Enable);
REX_HOOK(FM2_RenderContext_SetClipPlane2Enable, Fm2SetClipPlane2Enable);
REX_HOOK(FM2_RenderContext_SetClipPlane3Enable, Fm2SetClipPlane3Enable);

// ---------------------------------------------------------------------------
// Vertex/pixel shader bool constants (device->vertexShaderBoolConstants /
// pixelShaderBoolConstants -- packed 32 bools/dword, guest data is
// big-endian, 1 bool per dword, LSB tested). Distinct from the (deliberately
// unhooked) float-constant setters.
// ---------------------------------------------------------------------------

REX_IMPORT(__imp__sub_8236DBC8, g_origSetPsBoolConst, void(uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__sub_8236DB68, g_origSetVsBoolConst, void(uint32_t, uint32_t, uint32_t, uint32_t));

namespace {

void SetShaderConstantB(rex::be<uint32_t>* constants, uint32_t constantCount,
                        uint32_t startRegister, const uint32_t* data, uint32_t boolCount) {
  if (constants == nullptr || data == nullptr || startRegister >= constantCount * 32)
    return;
  const uint32_t count = std::min(boolCount, constantCount * 32 - startRegister);
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t bit = startRegister + i;
    uint32_t value = constants[bit / 32].get();
    const uint32_t mask = 1u << (bit & 31);
    if ((std::byteswap(data[i]) & 1u) != 0) {
      value |= mask;
    } else {
      value &= ~mask;
    }
    constants[bit / 32] = value;
  }
}

void Fm2SetPixelShaderConstantB(uint32_t device, uint32_t startRegister, uint32_t constantData,
                                uint32_t boolCount) {
  g_origSetPsBoolConst(device, startRegister, constantData, boolCount);
  auto* dev = ghp::ToHost<GuestDevice>(device);
  if (dev == nullptr || constantData == 0 || boolCount == 0)
    return;
  SetShaderConstantB(dev->pixelShaderBoolConstants,
                     uint32_t(std::size(dev->pixelShaderBoolConstants)), startRegister,
                     ghp::ToHost<const uint32_t>(constantData), boolCount);
}

void Fm2SetVertexShaderConstantB(uint32_t device, uint32_t startRegister, uint32_t constantData,
                                 uint32_t boolCount) {
  g_origSetVsBoolConst(device, startRegister, constantData, boolCount);
  auto* dev = ghp::ToHost<GuestDevice>(device);
  if (dev == nullptr || constantData == 0 || boolCount == 0)
    return;
  SetShaderConstantB(dev->vertexShaderBoolConstants,
                     uint32_t(std::size(dev->vertexShaderBoolConstants)), startRegister,
                     ghp::ToHost<const uint32_t>(constantData), boolCount);
}

}  // namespace

REX_HOOK(sub_8236DBC8, Fm2SetPixelShaderConstantB);
REX_HOOK(sub_8236DB68, Fm2SetVertexShaderConstantB);

// ---------------------------------------------------------------------------
// SetActivePassId: NOT a vertex declaration despite writing into the
// device's vertexDeclaration field -- confirmed via decompile to be a
// texture/shader-state pass token. Mirroring it there anyway is a deliberate
// band-aid the reference repo found empirically load-bearing: it's the only
// thing that gives the (Phase 4) declaration-matching path a non-zero handle
// to resolve; dropping it made every draw fail to match a declaration and
// skip, producing an all-black frame. Keep until a proper per-draw
// declaration source replaces it.
// ---------------------------------------------------------------------------

REX_IMPORT(__imp__sub_8236E228, g_origSetActivePassId, void(uint32_t, uint32_t));

namespace {

void Fm2SetActivePassId(uint32_t renderContext, uint32_t passId) {
  g_origSetActivePassId(renderContext, passId);
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  device->vertexDeclaration = passId;
}

}  // namespace

REX_HOOK(sub_8236E228, Fm2SetActivePassId);

// ---------------------------------------------------------------------------
// Vertex/index stream + surface binding.
// ---------------------------------------------------------------------------

REX_IMPORT(__imp__FM2_RenderContext_BindIndexBuffer, g_origBindIndexBuffer,
           void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetBoundSurface, g_origSetBoundSurface,
           void(uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__sub_823716F8, g_origBindSurfaceInternal, void(uint32_t, uint32_t, uint32_t));

namespace {

rr::GuestBaseTexture* TranslateSurfaceForBind(uint32_t surfaceAddr) {
  auto* gs = ghp::ToHost<GuestSurface>(surfaceAddr);
  // Unlike Lock/GetDesc/Unlock (which fall back to the original guest
  // function for a non-FM2 address), there's no "original" render-target-bind
  // path to fall back to here -- this is the only caller. Reject a garbage
  // address instead of handing back a wild pointer that SetRenderTargetInternal
  // would later store into g_renderTarget and dereference at present time.
  if (!rr::IsFm2Resource(gs))
    return nullptr;
  return gs;
}

void SetRenderTargetNative(GuestDevice* device, uint32_t index, uint32_t surfaceAddr) {
  rr::SetRenderTarget(device, index, TranslateSurfaceForBind(surfaceAddr));
}

}  // namespace

REX_IMPORT(__imp__FM2_RenderContext_BindVertexStream, g_origFm2BindVertexStream,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t));

// Carries a uint64_t dirty_mask split across r8:r9 in the 32-bit PPC ABI;
// the standard auto-marshaling would only read the low half, so this hook
// stays RAW to forward the full context, matching the reference's own
// documented reasoning for the same guest function.
REX_HOOK_RAW(FM2_RenderContext_BindVertexStream) {
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

void Fm2BindIndexBuffer(uint32_t renderContext, uint32_t resourceAddr) {
  g_origBindIndexBuffer(renderContext, resourceAddr);
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  rr::SetIndices(device, ghp::ToHost<GuestBuffer>(resourceAddr));
}

// FM2_RenderContext_SetBoundSurface is the guest equivalent of BOTH
// SetRenderTarget and SetDepthStencilSurface: classify by resource type and
// route depth surfaces to the depth slot instead of always binding color
// index 0 (otherwise depth is never bound and every depth-tested draw
// sanitizes to an unknown depth-stencil format).
void Fm2SetBoundSurface(uint32_t renderContext, uint32_t surfaceAddr, uint32_t surfaceArg) {
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
void Fm2BindSurface(uint32_t renderContext, uint32_t slot, uint32_t surfaceAddr) {
  g_origBindSurfaceInternal(renderContext, slot, surfaceAddr);
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr || surfaceAddr == 0)
    return;
  SetRenderTargetNative(device, slot, surfaceAddr);
}

REX_HOOK(FM2_RenderContext_BindIndexBuffer, Fm2BindIndexBuffer);
REX_HOOK(FM2_RenderContext_SetBoundSurface, Fm2SetBoundSurface);
REX_HOOK(sub_823716F8, Fm2BindSurface);

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
void Fm2SetPixelShaderState(uint32_t renderContext, uint32_t shaderAddr) {
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr || shaderAddr == 0)
    return;
  rr::SetPixelShader(device, ResolveShader(shaderAddr));
}

void Fm2SetVertexShaderState(uint32_t renderContext, uint32_t shaderAddr) {
  GuestDevice* device = DeviceForRenderContext(renderContext);
  if (device == nullptr || shaderAddr == 0)
    return;
  rr::SetVertexShader(device, ResolveShader(shaderAddr));
}

}  // namespace

REX_HOOK(FM2_RenderContext_SetPixelShaderState, Fm2SetPixelShaderState);
REX_HOOK(FM2_RenderContext_SetVertexShaderState, Fm2SetVertexShaderState);

// ---------------------------------------------------------------------------
// Viewport / scissor. Full replacements: the originals pack hardware-specific
// clip/viewport state and call D3D::SetSurfaceClip, a PM4-emitting function
// for a real Xenos GPU that does not exist under this renderer.
// FM2_RenderContext_SetViewportModeAndApply needs no hook of its own -- its
// body only stores a mode value and calls through to D3DDevice_SetScissorRect
// (confirmed via IDA decompile), which is hooked here.
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
// directly; raw XG-header textures (no kFm2ResourceMagic, created via
// XGSetTextureHeader instead of D3DDevice_CreateTexture) go through
// TranslateGuestTexture to parse their Xenos fetch constant and materialize
// a native texture (Lock/GetDesc on these still bind null -- that gap is
// unrelated to sampling and not covered by this port).
// ---------------------------------------------------------------------------

namespace {

void SetTextureHook(GuestDevice* device, uint32_t sampler, rr::GuestBaseTexture* texture) {
  if (sampler >= 16u)
    return;

  if (texture == nullptr) {
    rr::SetTexture(device, sampler, nullptr);
    return;
  }

  if (!rr::IsFm2Resource(texture)) {
    // Raw XG-header texture (created via XGSetTextureHeader rather than
    // D3DDevice_CreateTexture) -- parse its Xenos fetch constant and
    // materialize a native texture instead of binding null.
    rr::GuestTexture* translated = rr::TranslateGuestTexture(texture, true);
    if (translated == nullptr) {
      static std::unordered_set<const void*> s_warned;
      if (s_warned.insert(texture).second) {
        REXGPU_WARN(
            "D3DDevice_SetTexture: slot {} texture {} untranslatable "
            "(XG header?) -- bound null",
            sampler, static_cast<const void*>(texture));
      }
    }
    rr::SetTexture(device, sampler, translated);
    return;
  }

  if (texture->type == rr::ResourceType::Texture ||
      texture->type == rr::ResourceType::VolumeTexture) {
    rr::SetTexture(device, sampler, static_cast<rr::GuestTexture*>(texture));
    return;
  }

  // Render targets / depth surfaces rebound as shader resources (post /
  // resolve paths). Same GuestBaseTexture layout, different ResourceType.
  if (texture->type == rr::ResourceType::RenderTarget ||
      texture->type == rr::ResourceType::DepthStencil) {
    rr::SetTextureBase(device, sampler, texture);
    return;
  }

  rr::SetTexture(device, sampler, nullptr);
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

// D3DDevice_Resolve(pDevice, Flags, pSourceRect, pDestTexture, pDestPoint,
// DestLevel, DestSliceOrFace, pClearColor, ClearZ, ClearStencil,
// pParameters). DestLevel/DestSliceOrFace/pParameters don't apply to our
// single-level, single-slice-per-object resource model; accepted only so
// PPC arg marshaling stays aligned for the params after them.
void ResolveHook(GuestDevice* /*device*/, uint32_t flags, rr::GuestRect* sourceRect,
                 uint32_t destTextureAddr, rr::GuestPoint* destPoint, uint32_t /*destLevel*/,
                 uint32_t /*destSliceOrFace*/, const uint32_t* clearColor, float clearZ,
                 uint32_t /*clearStencil*/, const void* /*parameters*/) {
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
    dataBase = ReadGuestU32At(destTextureAddr + 32u) & 0x1FFFFFFFu & ~0xFFFu;
  }

  if (reo == nullptr || reo->texture == nullptr) {
    queueClearOnly();
    return;
  }

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
  if (isBandResolve) ++bandResolveCount;
  if ((isBandResolve && bandResolveCount <= 60) || resolveApertureCount <= 40 ||
      resolveApertureCount % 300 == 1) {
    REXGPU_INFO(
        "Resolve: flags=0x{:X} dest=0x{:08X} dataBase=0x{:08X} {}x{} fmt={} fm2={} destPt=({},{}) "
        "srcRect=({},{},{},{}) (n={})",
        flags, destTextureAddr, dataBase, reo->width, reo->height, int(reo->format),
        rr::IsFm2Resource(destHost), destPoint != nullptr ? destPoint->x.get() : -1,
        destPoint != nullptr ? destPoint->y.get() : -1,
        sourceRect != nullptr ? sourceRect->left.get() : -1,
        sourceRect != nullptr ? sourceRect->top.get() : -1,
        sourceRect != nullptr ? sourceRect->right.get() : -1,
        sourceRect != nullptr ? sourceRect->bottom.get() : -1, resolveApertureCount);
  }

  rr::ResolveToTexture(reo, destPoint, sourceRect, postClearFlags, postClearColor, clearZ);
}

}  // namespace

REX_HOOK(D3DDevice_Resolve, ResolveHook);

// ---------------------------------------------------------------------------
// Phase 4: draw dispatch. Full replacements -- the original bodies emit raw
// PM4 packets into a ring buffer for a real Xenos GPU that does not exist
// under this renderer (confirmed via decompile: both D3DDevice_DrawVertices
// and D3DDevice_DrawIndexedVertices are PM4-packet emitters, not simple
// state setters), so letting them run would be actively harmful, not just
// redundant.
// ---------------------------------------------------------------------------

REX_IMPORT(__imp__sub_8236D958, g_origUploadMatrixConstants,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
// Returns a guest-physical payload pointer that the caller fills after the
// function returns. The completed values therefore never enter GuestDevice's
// CPU constant files; record the payload here and consume it at the next draw.
REX_IMPORT(__imp__sub_82803358, g_origGpuBeginShaderConstantF4,
           uint32_t(uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__sub_823767B8, g_origCbSetShaderConstantF, void(uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__sub_823766E0, g_origCbCreateShaderConstantFFixup,
           uint32_t(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__sub_82382CC8, g_origSetPendingAluConstants,
           void(uint32_t, uint64_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_BeginCommandBufferBatch, g_origCbBatchBegin,
           uint32_t(uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_FinalizeCommandBufferBatch, g_origCbBatchFinalize, uint32_t(uint32_t));
REX_IMPORT(__imp__sub_823759A8, g_origCbCreateClone, uint32_t(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_EmitDirtyStateAndDrawList, g_origEmitDirtyStateAndDrawList,
           void(uint32_t, uint32_t, uint32_t));

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

  auto* memory = fm2::ghp::GuestMemory();
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
    fm2::render::StageDrawShaderConstants(!constants.pixelShader, constants.startRegister, source,
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
    fm2::render::StageDrawShaderConstants(true, constants.startRegister, constants.values.data(),
                                          constants.registerCount);
  }
}

void DrainDeferredDrawShaderConstants() {
  DrainDeferredShaderConstants();
  DrainDeferredCommandBufferConstants();
}

bool CopyGuestRange(uint32_t guestAddress, uint8_t* destination, size_t length) {
  const auto* source = ghp::ToHost<const uint8_t>(guestAddress);
  if (source == nullptr)
    return false;
  std::memcpy(destination, source, length);
  return true;
}

void RestoreGuestRange(uint32_t guestAddress, const uint8_t* source, size_t length) {
  auto* destination = ghp::ToHost<uint8_t>(guestAddress);
  if (destination != nullptr)
    std::memcpy(destination, source, length);
}

struct CapturedObjPassDraw {
  uint32_t ctx = 0;
  uint32_t prim = 0;
  int32_t baseVertexIndex = 0;
  uint32_t startIndex = 0;
  uint32_t indexCount = 0;
  rr::GuestShader* vs = nullptr;
  rr::GuestShader* ps = nullptr;
  rr::GuestVertexDeclaration* decl = nullptr;
  uint32_t declField = 0;
  rr::ObjReplayRenderState rstate;
  std::array<uint8_t, 0x1000> vsConsts{};
  std::array<uint8_t, 0x1000> psConsts{};
  std::array<uint8_t, 4> ibPtr{};
  std::array<uint8_t, 32> vbPtrs{};
  std::array<uint8_t, 8> vbStrides{};
  std::array<uint8_t, 16> shaderPtrs{};
  std::array<uint8_t, 4> colorWrite{};
  std::array<uint8_t, 128> texTable{};
  std::array<uint8_t, 16 * 24> texFetchConsts{};
  std::array<uint8_t, 0x1000> mainVsConsts{};
  std::array<uint8_t, 0x1000> mainPsConsts{};
  bool mainValid = false;
  uint32_t flushSeq = 0;
};

thread_local bool t_recordingBatch = false;
thread_local std::vector<CapturedObjPassDraw> t_pendingBatchDraws;
thread_local bool t_inObjReplay = false;
thread_local uint32_t t_objTraversalCtx = 0;
thread_local uint64_t t_travDirtyVs[4]{};
thread_local std::array<uint8_t, 0x1000> t_flushedVsConsts{};
thread_local std::array<uint8_t, 0x1000> t_flushedPsConsts{};
thread_local bool t_flushedVsValid = false;
thread_local bool t_flushedPsValid = false;
thread_local uint32_t t_flushedSeq = 0;

std::mutex g_objReplayMutex;
std::unordered_map<uint32_t, std::vector<CapturedObjPassDraw>> g_objReplayCache;

void CaptureBatchDrawPending(uint32_t context, uint32_t primitiveType, int32_t baseVertexIndex,
                             uint32_t startIndex, uint32_t indexCount) {
  if (context == 0 || t_pendingBatchDraws.size() >= 512u)
    return;

  CapturedObjPassDraw draw;
  draw.ctx = context;
  draw.prim = primitiveType;
  draw.baseVertexIndex = baseVertexIndex;
  draw.startIndex = startIndex;
  draw.indexCount = indexCount;
  draw.vs = rr::GetPipelineVertexShader();
  draw.ps = rr::GetPipelinePixelShader();
  draw.decl = rr::GetPipelineVertexDeclaration();
  if (GuestDevice* device = rr::GetActiveGuestDevice()) {
    draw.declField = device->vertexDeclaration.get();
  }
  rr::CaptureObjReplayRenderState(draw.rstate);

  if (!CopyGuestRange(context + 0x700u, draw.vsConsts.data(), draw.vsConsts.size()) ||
      !CopyGuestRange(context + 0x1700u, draw.psConsts.data(), draw.psConsts.size()) ||
      !CopyGuestRange(context + 0x2F7Cu, draw.ibPtr.data(), draw.ibPtr.size()) ||
      !CopyGuestRange(context + 0x2F94u, draw.vbPtrs.data(), draw.vbPtrs.size()) ||
      !CopyGuestRange(context + 0x2FD8u, draw.vbStrides.data(), draw.vbStrides.size()) ||
      !CopyGuestRange(context + 0x3070u, draw.shaderPtrs.data(), draw.shaderPtrs.size()) ||
      !CopyGuestRange(context + 10420u, draw.colorWrite.data(), draw.colorWrite.size()) ||
      !CopyGuestRange(context + 12264u, draw.texTable.data(), draw.texTable.size()) ||
      !CopyGuestRange(context + 1024u, draw.texFetchConsts.data(), draw.texFetchConsts.size())) {
    return;
  }

  if (t_flushedVsValid && t_flushedPsValid) {
    draw.mainVsConsts = t_flushedVsConsts;
    draw.mainPsConsts = t_flushedPsConsts;
    draw.mainValid = true;
    draw.flushSeq = t_flushedSeq;
  }
  t_pendingBatchDraws.push_back(std::move(draw));
}

void StartBatchDrawRecording() {
  t_recordingBatch = true;
  t_pendingBatchDraws.clear();
}

void StopBatchDrawRecording() {
  t_recordingBatch = false;
}

void BindPendingBatchDrawsToClone(uint32_t clone) {
  if (clone == 0 || t_pendingBatchDraws.empty()) {
    t_pendingBatchDraws.clear();
    return;
  }
  std::lock_guard lock(g_objReplayMutex);
  if (g_objReplayCache.size() > 4096u)
    g_objReplayCache.clear();
  g_objReplayCache[clone] = std::move(t_pendingBatchDraws);
  t_pendingBatchDraws.clear();
}

struct CtxReplayScribbleGuard {
  uint32_t ctx = 0;
  bool valid = false;
  std::array<uint8_t, 0x1000> vsConsts{};
  std::array<uint8_t, 0x1000> psConsts{};
  std::array<uint8_t, 4> ibPtr{};
  std::array<uint8_t, 32> vbPtrs{};
  std::array<uint8_t, 8> vbStrides{};
  std::array<uint8_t, 16> shaderPtrs{};
  std::array<uint8_t, 4> colorWrite{};
  std::array<uint8_t, 128> texTable{};
  std::array<uint8_t, 16 * 24> texFetchConsts{};

  explicit CtxReplayScribbleGuard(uint32_t context) : ctx(context) {
    valid = CopyGuestRange(ctx + 0x700u, vsConsts.data(), vsConsts.size()) &&
            CopyGuestRange(ctx + 0x1700u, psConsts.data(), psConsts.size()) &&
            CopyGuestRange(ctx + 0x2F7Cu, ibPtr.data(), ibPtr.size()) &&
            CopyGuestRange(ctx + 0x2F94u, vbPtrs.data(), vbPtrs.size()) &&
            CopyGuestRange(ctx + 0x2FD8u, vbStrides.data(), vbStrides.size()) &&
            CopyGuestRange(ctx + 0x3070u, shaderPtrs.data(), shaderPtrs.size()) &&
            CopyGuestRange(ctx + 10420u, colorWrite.data(), colorWrite.size()) &&
            CopyGuestRange(ctx + 12264u, texTable.data(), texTable.size()) &&
            CopyGuestRange(ctx + 1024u, texFetchConsts.data(), texFetchConsts.size());
  }

  ~CtxReplayScribbleGuard() {
    if (!valid)
      return;
    RestoreGuestRange(ctx + 0x700u, vsConsts.data(), vsConsts.size());
    RestoreGuestRange(ctx + 0x1700u, psConsts.data(), psConsts.size());
    RestoreGuestRange(ctx + 0x2F7Cu, ibPtr.data(), ibPtr.size());
    RestoreGuestRange(ctx + 0x2F94u, vbPtrs.data(), vbPtrs.size());
    RestoreGuestRange(ctx + 0x2FD8u, vbStrides.data(), vbStrides.size());
    RestoreGuestRange(ctx + 0x3070u, shaderPtrs.data(), shaderPtrs.size());
    RestoreGuestRange(ctx + 10420u, colorWrite.data(), colorWrite.size());
    RestoreGuestRange(ctx + 12264u, texTable.data(), texTable.size());
    RestoreGuestRange(ctx + 1024u, texFetchConsts.data(), texFetchConsts.size());
  }
};

void ReplayObjPassDraw(const CapturedObjPassDraw& draw) {
  RestoreGuestRange(draw.ctx + 0x700u, draw.vsConsts.data(), draw.vsConsts.size());
  const uint32_t traversalContext = t_objTraversalCtx;
  if (traversalContext != 0 && traversalContext != draw.ctx) {
    const auto* traversalVs = ghp::ToHost<const uint8_t>(traversalContext + 0x700u);
    auto* replayVs = ghp::ToHost<uint8_t>(draw.ctx + 0x700u);
    if (traversalVs != nullptr && replayVs != nullptr) {
      std::memcpy(replayVs, traversalVs, 12u * 16u);
      for (uint32_t word = 0; word < 4u; ++word) {
        uint64_t bits = t_travDirtyVs[word];
        if (word == 0)
          bits &= ~0xFFFull;
        while (bits != 0) {
          const uint32_t reg = word * 64u + uint32_t(std::countr_zero(bits));
          bits &= bits - 1;
          std::memcpy(replayVs + reg * 16u, traversalVs + reg * 16u, 16u);
        }
      }
    }
  }

  RestoreGuestRange(draw.ctx + 0x1700u, draw.psConsts.data(), draw.psConsts.size());
  if (draw.mainValid) {
    if (auto* replayVs = ghp::ToHost<uint8_t>(draw.ctx + 0x700u)) {
      std::memcpy(replayVs + 12u * 16u, draw.mainVsConsts.data() + 12u * 16u, (184u - 12u) * 16u);
      std::memset(replayVs + 58u * 16u, 0, 16u);
    }
    if (auto* replayPs = ghp::ToHost<uint8_t>(draw.ctx + 0x1700u)) {
      std::memcpy(replayPs, draw.mainPsConsts.data(), 180u * 16u);
    }
  }

  RestoreGuestRange(draw.ctx + 0x2F7Cu, draw.ibPtr.data(), draw.ibPtr.size());
  RestoreGuestRange(draw.ctx + 0x2F94u, draw.vbPtrs.data(), draw.vbPtrs.size());
  RestoreGuestRange(draw.ctx + 0x2FD8u, draw.vbStrides.data(), draw.vbStrides.size());
  RestoreGuestRange(draw.ctx + 0x3070u, draw.shaderPtrs.data(), draw.shaderPtrs.size());
  RestoreGuestRange(draw.ctx + 10420u, draw.colorWrite.data(), draw.colorWrite.size());
  RestoreGuestRange(draw.ctx + 12264u, draw.texTable.data(), draw.texTable.size());
  RestoreGuestRange(draw.ctx + 1024u, draw.texFetchConsts.data(), draw.texFetchConsts.size());

  rr::RestoreObjReplayRenderState(draw.rstate);
  GuestDevice* device = rr::GetActiveGuestDevice();
  if (device == nullptr)
    return;
  device->vertexDeclaration = draw.declField;
  rr::SetVertexDeclaration(device, draw.decl);
  rr::SetVertexShader(device, draw.vs);
  rr::SetPixelShader(device, draw.ps);

  // Guest VB/stride restore alone does not update host input slots. Without
  // this, ResolveVertexDeclaration can keep a stale 32B stride while the
  // restored guest ctx says 8 (fm2mmgrok10).
  if (const auto* ibBe = ghp::ToHost<const rex::be<uint32_t>>(draw.ctx + 0x2F7Cu)) {
    const uint32_t ibAddr = ibBe->get();
    rr::SetIndices(device, ibAddr != 0 ? ghp::ToHost<GuestBuffer>(ibAddr) : nullptr);
  }
  for (uint32_t s = 0; s < 8u; ++s) {
    const auto* vbBe = ghp::ToHost<const rex::be<uint32_t>>(draw.ctx + 0x2F94u + s * 4u);
    const uint8_t* strideBytes = ghp::ToHost<const uint8_t>(draw.ctx + 0x2FD8u);
    const uint32_t vbAddr = vbBe != nullptr ? vbBe->get() : 0;
    const uint8_t strideDwords = strideBytes != nullptr ? strideBytes[s] : 0;
    if (vbAddr == 0 || strideDwords == 0) {
      rr::SetStreamSource(device, s, nullptr, 0, 0);
      continue;
    }
    rr::SetStreamSource(device, s, ghp::ToHost<GuestBuffer>(vbAddr), 0,
                        uint32_t(strideDwords) * 4u);
  }

  rr::SetLiveFloatConstantFiles(ghp::ToHost<const void>(draw.ctx + 0x700u),
                                ghp::ToHost<const void>(draw.ctx + 0x1700u));
  rr::DrawIndexedVertices(device, draw.prim, draw.baseVertexIndex, draw.startIndex,
                          draw.indexCount);
  rr::SetLiveFloatConstantFiles(nullptr, nullptr);
}

// Rough measurement of what fraction of real draws go through this direct
// dispatch path vs. only ever executing from inside a recorded command-buffer
// batch (see IsInsideRecordedBatch) -- logged periodically so a real play
// session (menu -> garage/showroom -> track) gives a concrete answer instead
// of a guess.
uint64_t g_directDrawCount = 0;
uint64_t g_recordedBatchDrawCount = 0;
uint64_t g_lastLoggedFrame = ~0ull;

void TrackDrawForInstrumentation() {
  if (rr::IsInsideRecordedBatch()) {
    ++g_recordedBatchDrawCount;
  } else {
    ++g_directDrawCount;
  }
  const uint64_t frame = rr::CurrentFrameIndex();
  if (frame != g_lastLoggedFrame && frame % 300 == 0) {
    g_lastLoggedFrame = frame;
    const uint64_t total = g_directDrawCount + g_recordedBatchDrawCount;
    REXGPU_INFO("FM2 draw-path mix: direct={} recorded-batch={} ({:.1f}% recorded)",
                g_directDrawCount, g_recordedBatchDrawCount,
                100.0 * double(g_recordedBatchDrawCount) / double(std::max<uint64_t>(1, total)));
  }
}

void DrawVerticesHook(GuestDevice* device, uint32_t primitiveType, uint32_t startVertex,
                      uint32_t vertexCount) {
  DrainDeferredDrawShaderConstants();
  TrackDrawForInstrumentation();
  rr::DrawVertices(device, primitiveType, startVertex, vertexCount);
}

void DrawIndexedVerticesHook(GuestDevice* device, uint32_t primitiveType, int32_t baseVertexIndex,
                             uint32_t startIndex, uint32_t indexCount) {
  DrainDeferredDrawShaderConstants();
  TrackDrawForInstrumentation();
  // Object-pass record: snapshot guest context + pipeline before the native
  // submit so EmitDirty can replay with real PS/state later.
  if (t_recordingBatch && !t_inObjReplay && device != nullptr) {
    const uint32_t context = ghp::ToGuest(device);
    if (context != 0)
      CaptureBatchDrawPending(context, primitiveType, baseVertexIndex, startIndex, indexCount);
  }
  rr::DrawIndexedVertices(device, primitiveType, baseVertexIndex, startIndex, indexCount);
}

void DrawVerticesUPHook(GuestDevice* device, uint32_t primitiveType, uint32_t vertexCount,
                        const void* vertexStreamZeroData, uint32_t vertexStreamZeroStride) {
  DrainDeferredDrawShaderConstants();
  TrackDrawForInstrumentation();
  rr::DrawUserPointerVertices(device, primitiveType, vertexCount, vertexStreamZeroData,
                              vertexStreamZeroStride);
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

// Capture draw-site LR so FlushRenderState can gate the object-pass WVP overlay
// (g_lastDrawCallerLr == 0x82566A34). Still dispatch through HostToGuest for
// the typed draw hooks.
REX_HOOK_RAW(D3DDevice_DrawVertices) {
  rr::SetLastDrawCallerLr(static_cast<uint32_t>(ctx.lr));
  rex::ppc::HostToGuestFunction<DrawVerticesHook>(ctx, base);
}

REX_HOOK_RAW(D3DDevice_DrawIndexedVertices) {
  rr::SetLastDrawCallerLr(static_cast<uint32_t>(ctx.lr));
  rex::ppc::HostToGuestFunction<DrawIndexedVerticesHook>(ctx, base);
}

REX_HOOK_RAW(D3DDevice_DrawIndexedVertices_WithVertexFormatSetup) {
  rr::SetLastDrawCallerLr(static_cast<uint32_t>(ctx.lr));
  rex::ppc::HostToGuestFunction<DrawIndexedVerticesHook>(ctx, base);
}

REX_HOOK_RAW(D3DDevice_DrawVerticesUP) {
  rr::SetLastDrawCallerLr(static_cast<uint32_t>(ctx.lr));
  rex::ppc::HostToGuestFunction<DrawVerticesUPHook>(ctx, base);
}

// Tier A step 5–7: REX_HOOK_RAW feeders + object-pass record/replay.
// Never use HostToGuest marshalling on EmitDirty.
REX_HOOK_RAW(sub_8236D958) {
  // Capture before original — it clobbers r3–r6.
  const uint32_t devCtx = ctx.r3.u32;
  const uint32_t destReg = ctx.r4.u32;
  const uint32_t srcAddr = ctx.r5.u32;
  const uint32_t count = ctx.r6.u32;
  const uint32_t lr = static_cast<uint32_t>(ctx.lr);
  g_origUploadMatrixConstants.fn(ctx, base);

  // Object-pass traversal staging context + dirty VS regs for replay overlays.
  if (lr >= 0x8255BC00u && lr < 0x8255E800u) {
    t_objTraversalCtx = devCtx;
    const uint32_t lo = destReg < 256u ? destReg : 256u;
    const uint32_t hi = std::min(destReg + count, 256u);
    for (uint32_t r = lo; r < hi; ++r)
      t_travDirtyVs[r >> 6] |= uint64_t{1} << (r & 63u);
  }

  const void* src = ghp::ToHost<const void>(srcAddr);
  if (src == nullptr || count == 0)
    return;

  // Cross-context VS shadow for FlushRenderState when no live file is bound.
  rr::MirrorPassVsConstants(destReg, src, count);

  // Object-pass shared view-projection (c0–c11): arm on a large |c0.x| write at
  // reg0 from the object-pass setup region; complete with reg4/reg8.
  static std::atomic<bool> s_objWvpArmed{false};
  if (lr >= 0x8255BC00u && lr < 0x8255E800u && count >= 4u) {
    if (destReg == 0u) {
      const float c0x = std::bit_cast<float>(static_cast<const rex::be<uint32_t>*>(src)[0].get());
      const float ac = std::fabs(c0x);
      if (ac > 55.f && ac < 5000.f) {
        s_objWvpArmed.store(true, std::memory_order_relaxed);
        rr::CaptureObjPassWvp(0u, src, count);
      }
    } else if ((destReg == 4u || destReg == 8u) && s_objWvpArmed.load(std::memory_order_relaxed)) {
      rr::CaptureObjPassWvp(destReg, src, count);
      if (destReg == 8u)
        s_objWvpArmed.store(false, std::memory_order_relaxed);
    }
  }
}

REX_HOOK_RAW(sub_82382CC8) {
  // r5=constBase (0x4000 VS / 0x4400 PS), r6=registerFile guest ptr.
  const uint32_t constBase = ctx.r5.u32;
  const uint32_t regFile = ctx.r6.u32;
  g_origSetPendingAluConstants.fn(ctx, base);

  // Flush-time VS/PS snapshots for object-pass replay shared material bands.
  if (regFile != 0 && (constBase == 0x4000u || constBase == 0x4400u)) {
    if (const auto* rf = ghp::ToHost<const uint8_t>(regFile)) {
      if (constBase == 0x4000u) {
        std::memcpy(t_flushedVsConsts.data(), rf, t_flushedVsConsts.size());
        t_flushedVsValid = true;
      } else {
        std::memcpy(t_flushedPsConsts.data(), rf, t_flushedPsConsts.size());
        t_flushedPsValid = true;
      }
      ++t_flushedSeq;
    }
  }

  // VS ALU flush → scene3d WVP overlay (regs 15..18).
  if (constBase != 0x4000u || regFile == 0)
    return;
  const auto* wvp = ghp::ToHost<const uint32_t>(regFile + 15u * 16u);
  if (wvp == nullptr)
    return;

  uint32_t significant = 0;
  for (uint32_t i = 0; i < 16u; ++i) {
    const float value = std::bit_cast<float>(std::byteswap(wvp[i]));
    const float magnitude = std::fabs(value);
    if (std::isfinite(value) && magnitude >= 0.25f && magnitude <= 1.0e6f)
      ++significant;
  }
  if (significant >= 3u)
    rr::SetScene3dVsOverlayFromDevice(wvp, 15u, 4u);
}

REX_HOOK_RAW(FM2_D3D_BeginCommandBufferBatch) {
  if (rr::kObjPassRecordReplay)
    StartBatchDrawRecording();
  g_origCbBatchBegin.fn(ctx, base);
}

REX_HOOK_RAW(FM2_D3D_FinalizeCommandBufferBatch) {
  g_origCbBatchFinalize.fn(ctx, base);
  if (rr::kObjPassRecordReplay)
    StopBatchDrawRecording();
}

REX_HOOK_RAW(sub_823759A8) {
  g_origCbCreateClone.fn(ctx, base);
  // Return value in r3 is the drawNode EmitDirty will pass later.
  if (rr::kObjPassRecordReplay)
    BindPendingBatchDrawsToClone(ctx.r3.u32);
}

REX_HOOK_RAW(FM2_D3D_EmitDirtyStateAndDrawList) {
  // Snapshot args before the original — it clobbers guest registers.
  const uint32_t context = ctx.r3.u32;
  const uint32_t drawNode = ctx.r4.u32;
  g_origEmitDirtyStateAndDrawList.fn(ctx, base);

  // Scene3d WVP overlay (regs 15..18) — safe even without a matching clone.
  const auto* wvp = ghp::ToHost<const uint32_t>(context + 0x700u + 15u * 16u);
  if (wvp != nullptr) {
    uint32_t significant = 0;
    for (uint32_t i = 0; i < 16u; ++i) {
      const float value = std::bit_cast<float>(std::byteswap(wvp[i]));
      const float magnitude = std::fabs(value);
      if (std::isfinite(value) && magnitude >= 0.25f && magnitude <= 1.0e6f)
        ++significant;
    }
    if (significant >= 3u)
      rr::SetScene3dVsOverlayFromDevice(wvp, 15u, 4u);
  }

  if (!rr::kObjPassRecordReplay || t_inObjReplay)
    return;

  std::unique_lock lock(g_objReplayMutex);
  const auto it = g_objReplayCache.find(drawNode);
  if (it == g_objReplayCache.end() || it->second.empty())
    return;

  const std::vector<CapturedObjPassDraw>& draws = it->second;
  const uint32_t replayContext = draws.front().ctx;
  CtxReplayScribbleGuard guard(replayContext);
  if (!guard.valid)
    return;

  GuestDevice* device = rr::GetActiveGuestDevice();
  rr::GuestShader* previousVs = rr::GetPipelineVertexShader();
  rr::GuestShader* previousPs = rr::GetPipelinePixelShader();
  rr::GuestVertexDeclaration* previousDecl = rr::GetPipelineVertexDeclaration();
  const uint32_t previousDeclField = device != nullptr ? device->vertexDeclaration.get() : 0;
  rr::ObjReplayRenderState previousRenderState;
  rr::CaptureObjReplayRenderState(previousRenderState);

  t_inObjReplay = true;
  for (const CapturedObjPassDraw& draw : draws) {
    if (draw.ctx == replayContext)
      ReplayObjPassDraw(draw);
  }
  t_inObjReplay = false;

  if (device != nullptr) {
    device->vertexDeclaration = previousDeclField;
    rr::SetVertexDeclaration(device, previousDecl);
    rr::SetVertexShader(device, previousVs);
    rr::SetPixelShader(device, previousPs);
  }
  rr::RestoreObjReplayRenderState(previousRenderState);
  std::fill(std::begin(t_travDirtyVs), std::end(t_travDirtyVs), 0);
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

// ---------------------------------------------------------------------------
// Recorded command-buffer object-pass batch boundary (car/showroom
// geometry). FM2_Render_ScopedBatchBegin/Finalize wrap a
// FM2_D3D_BeginCommandBufferBatch/FinalizeCommandBufferBatch bracket that
// switches the game onto a secondary device/context. Tracked as a flag for
// placeholder PS substitution while object-pass replay is disabled.
// ---------------------------------------------------------------------------

REX_IMPORT(__imp__FM2_Render_ScopedBatchBegin, g_origScopedBatchBegin, void());
REX_IMPORT(__imp__FM2_Render_ScopedBatchFinalize, g_origScopedBatchFinalize, void());

REX_HOOK_RAW(FM2_Render_ScopedBatchBegin) {
  g_origScopedBatchBegin.fn(ctx, base);
  rr::SetInsideRecordedBatch(true);
}

REX_HOOK_RAW(FM2_Render_ScopedBatchFinalize) {
  rr::SetInsideRecordedBatch(false);
  g_origScopedBatchFinalize.fn(ctx, base);
}
