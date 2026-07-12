// render/render_state.h
//
// Phase 3 (render state + pipeline/shaders): state setters, shader load/PSO
// cache, texture/sampler binding. Still no draws (Phase 4).
//
// Trimmed from the reference repo's render_state.h, which also declared a
// large VRAM-viewer/present-diagnostics API (SetTestGameTexture,
// RecordVramViewTexture, GetSceneResolveSource, SetScenePresentRT, ...) and
// PM4-record/replay-specific plumbing (SnapshotSurfaceForResolve,
// SetStreamSourceGuestData/HostWindow, StretchRect) that belongs to the
// abandoned diagnostic subsystem or to the Phase 4 constant-transport
// rebuild, not here.

#pragma once

#include <cstdint>

#include "render/guest_device.h"
#include "render/guest_resources.h"

namespace fm2::render {

void BeginRenderStateFrame();
// Monotonic frame counter (incremented each BeginRenderStateFrame); used to
// dedupe per-frame guest-memory uploads.
uint64_t CurrentFrameIndex();

void SetRenderState(GuestDevice* device, uint32_t state, uint32_t value);
void SetViewportEnable(GuestDevice* device, uint32_t value);
void UpdateClipPlaneConstants(GuestDevice* device);
void SetDepthState(uint32_t zEnable, uint32_t zWriteEnable, uint32_t cmpFunc);

struct GuestStencilState {
  bool enable;
  bool twoSided;
  uint32_t frontFunc, frontFail, frontDepthFail, frontPass;
  uint32_t backFunc, backFail, backDepthFail, backPass;
  uint32_t readMask, writeMask, ref;
};
void SetStencilState(const GuestStencilState& s);

void SetTexture(GuestDevice* device, uint32_t index, GuestTexture* texture);

void SetVertexShader(GuestDevice* device, GuestShader* shader);
void SetPixelShader(GuestDevice* device, GuestShader* shader);
void SetVertexDeclaration(GuestDevice* device, GuestVertexDeclaration* declaration);

void SetStreamSource(GuestDevice* device, uint32_t index, GuestBuffer* buffer, uint32_t offset,
                     uint32_t stride);
void SetIndices(GuestDevice* device, GuestBuffer* buffer);

void SetViewport(GuestDevice* device, GuestViewport* viewport);
void SetScissorRect(GuestDevice* device, GuestRect* rect);

void SetRenderTarget(GuestDevice* device, uint32_t index, GuestBaseTexture* renderTarget);
void SetImplicitRenderTarget(GuestBaseTexture* renderTarget);
GuestBaseTexture* GetCurrentColorRenderTarget();
void SetDepthStencilSurface(GuestDevice* device, GuestSurface* depthStencil);

void Clear(GuestDevice* device, uint32_t flags, const float* color, float z);

}  // namespace fm2::render
