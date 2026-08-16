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
// Render-thread: start-of-slot frame bookkeeping (call from ProcBeginCommandList).
void NotifyRenderFrameBegin();

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
// Bind a non-GuestTexture GuestBaseTexture (render-target / depth surface used
// as a shader resource). Clears any stale GuestTexture alias at this slot.
void SetTextureBase(GuestDevice* device, uint32_t index, GuestBaseTexture* texture);

// Translates a raw XG-header guest texture object (created via the low-level
// XGSetTextureHeader XDK API rather than D3DDevice_CreateTexture, so it has
// no kFm2ResourceMagic tag) into a native GuestTexture by parsing its Xenos
// fetch constant and, if requested, detiling/uploading its guest texture
// data. Results are cached by guest base address. Returns nullptr if the
// header's fetch constant can't be parsed (unsupported format, zero base,
// etc.) or the texture could not be created.
GuestTexture* TranslateGuestTexture(void* guestHeader, bool uploadGuestData);
// Same translation, but starting directly from a GPUTEXTURE_FETCH_CONSTANT
// (as bound by the PM4 command stream) rather than a full XG texture header.
GuestTexture* TranslateGuestTextureFetch(const void* guestFetch, bool uploadGuestData);

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

// Marks the currently-bound color render target as this frame's present
// source. Call right before Video::Present() (from whichever hook is the
// live present trigger) -- without this, Present() never has a front buffer
// to blit and always falls back to a flat clear color.
void PrepareFramePresent();
void SetDepthStencilSurface(GuestDevice* device, GuestSurface* depthStencil);

// Drain pending StretchRect / Resolve copies (and MSAA resolves) into their
// destination textures. Must run on the render thread with RecordingMutex held
// (or nested under RenderQueue::Run). Called before Present and before draws.
void FlushPendingStretchRectCommands();

// Unleashed DestructResource: queue destruction of an FM2 GuestResource until
// the current recording frame's GPU fence retires (via OnRecordingFrameReady).
void ScheduleResourceDestruction(GuestResource* resource);

void Clear(GuestDevice* device, uint32_t flags, const float* color, float z);

// D3DDevice_Resolve's texture-copy half: copies the currently-bound color
// render target into destTexture (Xbox 360's EDRAM-to-linear-texture
// resolve). destPoint/sourceRect may be null (full-texture copy at 0,0),
// matching the guest API's own optional-pointer semantics.
void ResolveToTexture(GuestBaseTexture* destTexture, const GuestPoint* destPoint,
                      const GuestRect* sourceRect);

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

// Non-indexed draw with QUADLIST/TRIANGLEFAN → indexed triangle-list conversion.
void DrawVertices(GuestDevice* device, uint32_t primitiveType, uint32_t startVertex,
                  uint32_t vertexCount);

// Indexed draw (FlushRenderState + drawIndexedInstanced in one render-queue job).
void DrawIndexedVertices(GuestDevice* device, uint32_t primitiveType, int32_t baseVertexIndex,
                         uint32_t startIndex, uint32_t indexCount);

// D3DDevice_DrawVerticesUP: inline (non-buffer-backed) vertex data supplied
// directly by the guest for this one draw. Uploads it to a scratch buffer
// and binds it at stream 0 for just this call -- callers must not rely on
// stream 0's tracked GuestBuffer binding surviving a call to this function.
void DrawUserPointerVertices(GuestDevice* device, uint32_t primitiveType, uint32_t vertexCount,
                             const void* data, uint32_t stride);

// True while inside a FM2_Render_ScopedBatchBegin/Finalize bracket. The lower
// command-buffer batch hooks capture these object-pass draws and replay them
// from their persistent clone with record-time state plus live traversal
// constants; FlushRenderState therefore always resolves the real pixel shader.
void SetInsideRecordedBatch(bool inside);
bool IsInsideRecordedBatch();

}  // namespace fm2::render
