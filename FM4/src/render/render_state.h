// render/render_state.h
//
// Native D3D state, shader/PSO, resource binding, and draw translation. The
// historical VRAM viewer and object-pass replay diagnostics are intentionally
// outside this interface.

#pragma once

#include <cstdint>

#include "render/guest_device.h"
#include "render/guest_resources.h"

namespace fm4::render {

// One-shot diagnostic: dumps the live values of every derived guest-device
// offset. Called from the Direct3D_CreateDevice hook.
void LogGuestDeviceLayout(const GuestDevice* device);

void BeginRenderStateFrame();
// Monotonic frame counter (incremented each BeginRenderStateFrame); used to
// dedupe per-frame guest-memory uploads.
uint64_t CurrentFrameIndex();
// Render-thread: start-of-slot frame bookkeeping (call from ProcBeginCommandList).
void NotifyRenderFrameBegin();

void SetRenderState(GuestDevice* device, uint32_t state, uint32_t value);

// Reads every render state the renderer tracks straight out of the guest
// device's Xenos register shadows. FM4 leaves 11 of the 36 D3DDevice_
// SetRenderState_* setters with no out-of-line body (hook map section 4.2), so
// none of them is hooked; this runs once per draw at the top of FlushRenderState
// instead.
void SampleGuestRenderStates(const GuestDevice* device);
void SetViewportEnable(GuestDevice* device, uint32_t value);
void SetClipPlaneState(GuestDevice* device, uint32_t enabledMask);
void SetDepthState(uint32_t zEnable, uint32_t zWriteEnable, uint32_t cmpFunc);

struct GuestStencilState {
  bool enable;
  bool twoSided;
  uint32_t frontFunc, frontFail, frontDepthFail, frontPass;
  uint32_t backFunc, backFail, backDepthFail, backPass;
  uint32_t readMask, writeMask, ref;
};
void SetStencilState(const GuestStencilState& s);

void SetTexture(GuestDevice* device, uint32_t index, GuestTexture* texture, uint32_t guestAddress);
// Bind a non-GuestTexture GuestBaseTexture (render-target / depth surface used
// as a shader resource). Clears any stale GuestTexture alias at this slot.
void SetTextureBase(GuestDevice* device, uint32_t index, GuestBaseTexture* texture,
                    uint32_t guestAddress);

// Translates a raw XG-header guest texture object (created via the low-level
// XGSetTextureHeader XDK API rather than D3DDevice_CreateTexture, so it has
// no kFm4ResourceMagic tag) into a native GuestTexture by parsing its Xenos
// fetch constant and, if requested, detiling/uploading its guest texture
// data. Results are cached by guest base address. Returns nullptr if the
// header's fetch constant can't be parsed (unsupported format, zero base,
// etc.) or the texture could not be created.
GuestTexture* TranslateGuestTexture(void* guestHeader, bool uploadGuestData);
// Same translation, but starting directly from a raw
// GPUTEXTURE_FETCH_CONSTANT rather than a full XG texture header.
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

void SetDepthStencilSurface(GuestDevice* device, GuestSurface* depthStencil);

// Drain pending StretchRect / Resolve copies (and MSAA resolves) into their
// destination textures. Must run on the render thread with RecordingMutex held
// (or nested under RenderQueue::Run). Called before Present and before draws.
void FlushPendingStretchRectCommands();

// Unleashed DestructResource: queue destruction of an FM2 GuestResource until
// the current recording frame's GPU fence retires (via OnRecordingFrameReady).
void ScheduleResourceDestruction(GuestResource* resource);

void Clear(GuestDevice* device, uint32_t flags, const float* color, float z);

// Predicated tiling: the host renders the union of the tile rects once. FM4
// has no XDK D3DDevice_EndTiling; the pass is closed by the next BeginTiling
// or by Swap.
void BeginTilingPass(const uint32_t* rects, uint32_t count, uint32_t flags, const float* clearColor,
                     float clearZ, uint32_t clearStencil);
void EndTilingPass();

// D3DDevice_Resolve's texture-copy half: copies the currently-bound color
// render target into destTexture (Xbox 360's EDRAM-to-linear-texture
// resolve). destPoint/sourceRect may be null (full-texture copy at 0,0),
// matching the guest API's own optional-pointer semantics.
void ResolveToTexture(GuestBaseTexture* destTexture, const GuestPoint* destPoint,
                      const GuestRect* sourceRect, uint32_t postClearFlags,
                      const float* postClearColor, float postClearZ);

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

// Guest thread: stage big-endian float4 registers emitted into a deferred
// command-buffer payload. QueueDrawStateSnapshots overlays and consumes them
// atomically with the next draw.
void StageDrawShaderConstants(bool vertex, uint32_t startRegister, const void* beDwords,
                              uint32_t registerCount);

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

}  // namespace fm4::render
