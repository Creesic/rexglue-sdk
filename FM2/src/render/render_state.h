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

// ---------------------------------------------------------------------------
// Phase 4: draw dispatch + constant transport.
// ---------------------------------------------------------------------------

// Builds/looks up the PSO for the currently tracked state, uploads the
// guest's own shader constant registers (untouched since Phase 3 deliberately
// left the constant-setter functions unhooked) and the shared-constants
// buffer, and binds vertex/index buffers + viewport/scissor/framebuffer.
// Must be called immediately before every draw. Skips the draw-affecting
// binds (but still flushes framebuffer/viewport) if no valid pipeline could
// be built -- check HasBoundPipeline() before issuing the actual draw call.
void FlushRenderState(GuestDevice* device, uint32_t primitiveType);
bool HasBoundPipeline();

void DrawInstanced(uint32_t vertexCount, uint32_t startVertex);
void DrawIndexedInstanced(uint32_t indexCount, uint32_t startIndex, int32_t baseVertexIndex);

// D3DDevice_DrawVerticesUP: inline (non-buffer-backed) vertex data supplied
// directly by the guest for this one draw. Uploads it to a scratch buffer
// and binds it at stream 0 for just this call -- callers must not rely on
// stream 0's tracked GuestBuffer binding surviving a call to this function.
void DrawUserPointerVertices(GuestDevice* device, uint32_t primitiveType, uint32_t vertexCount,
                             const void* data, uint32_t stride);

// True while inside a FM2_Render_ScopedBatchBegin/Finalize bracket -- FM2's
// recorded-command-buffer object-pass path (car/showroom geometry). Draws
// issued in this state only ever execute once, at record time (there is no
// real PM4 ring for a later "replay" to execute against under this native
// renderer), and the guest's shared constant register file is not reliably
// this object's own by the time replay would have happened on real hardware.
// FlushRenderState keeps normal vertex-shader/geometry handling but binds an
// always-resident flat placeholder pixel shader instead of resolving the
// real one, so these draws render as a correctly-positioned (best-effort)
// but visibly-unshaded placeholder rather than silently vanishing or reusing
// stale constants from an unrelated draw.
void SetInsideRecordedBatch(bool inside);
bool IsInsideRecordedBatch();

}  // namespace fm2::render
