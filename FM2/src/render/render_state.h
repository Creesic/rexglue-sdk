// render/render_state.h


#pragma once

#include <cstdint>

#include "render/guest_device.h"
#include "render/guest_resources.h"

namespace fm2::render {

void BeginRenderStateFrame();
// Monotonic frame counter (incremented each BeginRenderStateFrame); used to
// dedupe per-frame guest-memory uploads.
uint64_t CurrentFrameIndex();

void SetRenderState(GuestDevice *device, uint32_t state, uint32_t value);
void SetViewportEnable(GuestDevice *device, uint32_t value);
void UpdateClipPlaneConstants(GuestDevice *device);
void SetDepthState(uint32_t zEnable, uint32_t zWriteEnable, uint32_t cmpFunc);
struct GuestStencilState {
  bool enable;
  bool twoSided;
  uint32_t frontFunc, frontFail, frontDepthFail, frontPass;
  uint32_t backFunc, backFail, backDepthFail, backPass;
  uint32_t readMask, writeMask, ref;
};
void SetStencilState(const GuestStencilState &s);
void SetTexture(GuestDevice *device, uint32_t index, GuestTexture *texture);
// Bind a render-surface (or any base texture) directly into a sampler slot.
// Used by the surface-aperture registry to satisfy fetch-constant texture
// samples that point at a resolved surface's backing memory.
void SetTextureBase(GuestDevice *device, uint32_t index,
                    GuestBaseTexture *texture);
// Freeze a copy of a resolved surface (keyed by resolve-dest guest address) so
// later samples see a snapshot, not the live (overwritten) render target.
GuestBaseTexture *SnapshotSurfaceForResolve(GuestBaseTexture *source,
                                            uint32_t destBase);
void SetVertexShader(GuestDevice *device, GuestShader *shader);
void SetPixelShader(GuestDevice *device, GuestShader *shader);
void SetVertexDeclaration(GuestDevice *device,
                          GuestVertexDeclaration *declaration);
void SetStreamSource(GuestDevice *device, uint32_t index, GuestBuffer *buffer,
                     uint32_t offset, uint32_t stride);
void SetIndices(GuestDevice *device, GuestBuffer *buffer);
void SetStreamSourceGuestData(GuestDevice *device, uint32_t index,
                              const void *data, uint32_t size, uint32_t stride);
void SetIndicesGuestData(GuestDevice *device, const void *data, uint32_t size,
                         uint32_t indexStride);
void SetViewport(GuestDevice *device, GuestViewport *viewport);
void SetScissorRect(GuestDevice *device, GuestRect *rect);
void SetRenderTarget(GuestDevice *device, uint32_t index,
                     GuestBaseTexture *renderTarget);
void SetImplicitRenderTarget(GuestBaseTexture *renderTarget);
GuestBaseTexture *GetCurrentColorRenderTarget();
GuestBaseTexture *GetLastDrawnColorRenderTarget();
// Diagnostic: last game texture translated from a fetch constant (test grid).
void SetTestGameTexture(GuestBaseTexture *t);
GuestBaseTexture *GetTestGameTexture();
// Recent distinct color render targets (index 0 = most recent), for finding
// which surface holds rendered content (present-source diagnostics).
GuestBaseTexture *GetRecentColorRenderTarget(uint32_t index);
// VRAM viewer: stable set of distinct GUEST textures the draws actually sample
// (resolve-dests + loaded textures = the guest-RAM contents). The present grid
// blits these so the user can see if RAM holds black / garbage / good data.
void RecordVramViewTexture(uint32_t base, GuestBaseTexture *tex);
GuestBaseTexture *GetVramViewTexture(uint32_t index, uint32_t *outBase);
uint32_t VramViewCount();
void SetDepthStencilSurface(GuestDevice *device, GuestSurface *depthStencil);

void Clear(GuestDevice *device, uint32_t flags, const float *color, float z);

// D3DDevice_Resolve: copy/resolve the current source surface into a texture.
void StretchRect(GuestDevice *device, uint32_t flags, const GuestRect *source,
                 GuestBaseTexture *destination, const GuestPoint *destPoint);

// Draws.
void DrawPrimitive(GuestDevice *device, uint32_t primitiveType,
                   uint32_t startVertex, uint32_t primitiveCount);
void DrawIndexedPrimitive(GuestDevice *device, uint32_t primitiveType,
                          int32_t baseVertexIndex, uint32_t startIndex,
                          uint32_t primitiveCount);
void DrawPrimitiveUP(GuestDevice *device, uint32_t primitiveType,
                     uint32_t primitiveCount, void *vertexStreamZeroData,
                     uint32_t vertexStreamZeroStride);
void DrawIndexedPrimitiveUP(GuestDevice *device, uint32_t primitiveType,
                            uint32_t minVertexIndex, uint32_t numVertices,
                            uint32_t numPrimitives, const void *indexData,
                            uint32_t indexStride, const void *vertexData,
                            uint32_t vertexStride);

} // namespace fm2::render
