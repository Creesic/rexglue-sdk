#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <rex/cvar.h>
#include <rex/hash.h>
#include <rex/hook.h>
#include <rex/logging.h>

#include "native_renderer/fm2_direct_draw_decode.h"
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

namespace fm2::render {
GuestBuffer *CreateVertexBuffer(uint32_t length);
GuestBuffer *CreateIndexBuffer(uint32_t length, uint32_t format);
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
REX_IMPORT(__imp__FM2_Render_LoadPixelShaderResourceById,
           g_origFm2LoadPixelShaderResourceById, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_Render_LoadVertexShaderResourceById,
           g_origFm2LoadVertexShaderResourceById, void(uint32_t, uint32_t));

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

REX_IMPORT(__imp__FM2_D3DVertexBuffer_Lock, g_origVertexBufferLock,
           uint32_t(void *, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_LockGpuBufferRaw, g_origIndexBufferLock,
           uint32_t(void *, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3DSurface_GetDesc, g_origSurfaceGetDesc,
           void(void *, void *));

namespace {

using rr::GuestBaseTexture;
using rr::GuestBuffer;
using rr::GuestDevice;
using rr::GuestShader;
using rr::GuestSurface;
using rr::GuestTexture;
using rr::GuestVertexDeclaration;

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
  FlushImmediateVertices();
  Video::Present();
}

void Fm2SetPixelShaderState(uint32_t renderContext, uint32_t shader) {
  g_origFm2SetPixelShaderState(renderContext, shader);
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr || shader == 0) {
    return;
  }
  SetPixelShaderNative(device, ghp::ToHost<GuestShader>(shader));
}

void Fm2SetVertexShaderState(uint32_t renderContext, uint32_t shader) {
  g_origFm2SetVertexShaderState(renderContext, shader);
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
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr) {
    return;
  }
  SetStreamSourceNative(device, slot, ghp::ToHost<GuestBuffer>(resource),
                        byte_offset, stride_bytes, dirty_mask);
}

void Fm2BindIndexBuffer(uint32_t renderContext, uint32_t resource) {
  g_origFm2BindIndexBuffer(renderContext, resource);
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr) {
    return;
  }
  SetIndicesNative(device, ghp::ToHost<GuestBuffer>(resource));
}

void Fm2SetBoundSurface(uint32_t renderContext, uint32_t surface,
                        uint32_t surfaceArg) {
  g_origFm2SetBoundSurface(renderContext, surface, surfaceArg);
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr || surface == 0) {
    return;
  }
  SetRenderTargetNative(device, 0, ghp::ToHost<GuestSurface>(surface));
}

void Fm2RememberGuestDevice(GuestDevice *device) {
  if (device != nullptr) {
    rr::SetActiveGuestDevice(device);
  }
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
                     void * /*rect*/, uint32_t /*flags*/) {
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
  if (!rr::IsFm2Resource(buffer))
    return g_origVertexBufferLock(buffer, offset, size, flags);
  return rr::LockVertexBuffer(buffer, flags);
}
uint32_t IndexBufferLock(GuestBuffer *buffer, uint32_t offset, uint32_t size,
                         uint32_t flags) {
  if (!rr::IsFm2Resource(buffer))
    return g_origIndexBufferLock(buffer, offset, size, flags);
  return rr::LockIndexBuffer(buffer, flags);
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

void UnlockResourceHook(rr::GuestResource *resource, uint32_t /*base*/,
                        uint32_t /*mip*/) {
  if (!rr::IsFm2Resource(resource))
    return; // genuine guest D3D resource: ignore
  switch (resource->type) {
  case rr::ResourceType::VertexBuffer:
    rr::UnlockVertexBuffer(static_cast<GuestBuffer *>(resource));
    break;
  case rr::ResourceType::IndexBuffer:
    rr::UnlockIndexBuffer(static_cast<GuestBuffer *>(resource));
    break;
  case rr::ResourceType::Texture:
  case rr::ResourceType::VolumeTexture:
    rr::UnlockTextureRect(static_cast<GuestTexture *>(resource));
    break;
  default:
    break;
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
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  FlushImmediateVertices();
  rr::SetRenderState(device, state, value);
}

void MirrorFm2ClipPlanes(uint32_t renderContext) {
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

} // namespace

void FM2PlumeTraceVdSwap(PPCRegister &r3, PPCRegister &r4, PPCRegister &r8,
                         PPCRegister &r9, PPCRegister &r10,
                         PPCRegister &r1) {
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

  rr::GuestTexture *frontBuffer =
      rr::TranslateGuestTextureFetch(ghp::ToHost<void>(fetchAddress), true);
  const bool hasTexture =
      frontBuffer != nullptr && frontBuffer->texture != nullptr;

  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=VdSwap n={} buffer=0x{:08X} "
                "fetch=0x{:08X} frontbuffer=0x{:08X} textureFormat=0x{:08X} "
                "colorSpace=0x{:08X} size={}x{} source={} hasTexture={}",
                n, r3.u32, fetchAddress, frontbufferAddress, textureFormat,
                colorSpace, width, height, static_cast<const void *>(frontBuffer),
                hasTexture);
  }

  if (!REXCVAR_GET(fm2_plume_vdswap_present)) {
    return;
  }

  if (hasTexture) {
    rr::SetPresentSource(frontBuffer);
  }
  Video::Present();
}

// ===========================================================================
// FM2 native renderer hooks: call generated FM2 bodies, then mirror state.
// ===========================================================================

REX_HOOK(FM2_RenderContext_SetPixelShaderState, Fm2SetPixelShaderState);
REX_HOOK(FM2_RenderContext_SetVertexShaderState, Fm2SetVertexShaderState);
REX_HOOK(FM2_RenderContext_BindVertexStream, Fm2BindVertexStream);
REX_HOOK(FM2_RenderContext_BindIndexBuffer, Fm2BindIndexBuffer);
REX_HOOK(FM2_RenderContext_SetBoundSurface, Fm2SetBoundSurface);
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
