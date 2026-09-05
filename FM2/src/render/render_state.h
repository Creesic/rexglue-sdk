// render/render_state.h
//
// Native D3D state, shader/PSO, resource binding, and draw translation. The
// historical VRAM viewer and object-pass replay diagnostics are intentionally
// outside this interface.

#pragma once

#include <algorithm>
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

void SetTexture(GuestDevice* device, uint32_t index, GuestTexture* texture, uint32_t guestAddress,
                int32_t exponentAdjust);
// Bind a non-GuestTexture GuestBaseTexture (render-target / depth surface used
// as a shader resource). Clears any stale GuestTexture alias at this slot.
void SetTextureBase(GuestDevice* device, uint32_t index, GuestBaseTexture* texture,
                    uint32_t guestAddress, int32_t exponentAdjust);

// Translates a raw XG-header guest texture object (created via the low-level
// XGSetTextureHeader XDK API rather than D3DDevice_CreateTexture, so it has
// no kFm2ResourceMagic tag) into a native GuestTexture by parsing its Xenos
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

void SetRenderTarget(GuestDevice* device, uint32_t index, GuestBaseTexture* renderTarget,
                     uint32_t caller = 0);
void SetImplicitRenderTarget(GuestBaseTexture* renderTarget);
GuestBaseTexture* GetCurrentColorRenderTarget();

void SetDepthStencilSurface(GuestDevice* device, GuestSurface* depthStencil, uint32_t caller = 0);

// Drain pending StretchRect / Resolve copies (and MSAA resolves) into their
// destination textures. Must run on the render thread with RecordingMutex held
// (or nested under RenderQueue::Run). Called before Present and before draws.
void FlushPendingStretchRectCommands();

// Unleashed DestructResource: queue destruction of an FM2 GuestResource until
// the current recording frame's GPU fence retires (via OnRecordingFrameReady).
void ScheduleResourceDestruction(GuestResource* resource);

void Clear(GuestDevice* device, uint32_t flags, const float* color, float z);

inline constexpr uint32_t kResolveDepthStencil = 0x4;

inline GuestBaseTexture* SelectResolveSource(uint32_t flags, GuestBaseTexture* color,
                                           GuestSurface* depth, GuestBaseTexture* lastColor) {
  // A missing depth bind must never resolve a color/presentation fallback.
  return (flags & kResolveDepthStencil) != 0 ? depth : (color != nullptr ? color : lastColor);
}

inline plume::RenderViewport ClampViewportToSurface(plume::RenderViewport viewport,
                                                    const GuestBaseTexture* surface) {
  // D3D::SetViewport (82371348) clips the requested extent to the bound
  // surface. InitSurfaceBindDefaults deliberately requests 65535 squared.
  if (surface != nullptr) {
    viewport.width = std::min(viewport.width, float(surface->width) - viewport.x);
    viewport.height = std::min(viewport.height, float(surface->height) - viewport.y);
    if (viewport.width < 0.0f || viewport.height < 0.0f)
      viewport.width = viewport.height = 0.0f;
  }
  return viewport;
}

inline bool CanCopyDepthSurface(const GuestSurface& source, const GuestBaseTexture& dest) {
  // Plume CopyTexture copies both depth and stencil planes. Require identical
  // whole 2D resources; hardware color resolves/partial depth copies are invalid.
  if (!plume::RenderFormatIsDepth(source.format) || source.format != dest.format ||
      source.sampleCount != plume::RenderSampleCount::COUNT_1 ||
      source.width != dest.width || source.height != dest.height ||
      source.levels != 1 || dest.levels != 1)
    return false;
  if (dest.type == ResourceType::DepthStencil)
    return static_cast<const GuestSurface&>(dest).sampleCount == plume::RenderSampleCount::COUNT_1;
  return dest.type == ResourceType::Texture &&
         static_cast<const GuestTexture&>(dest).viewDimension ==
             plume::RenderTextureViewDimension::TEXTURE_2D &&
         static_cast<const GuestTexture&>(dest).depth == 1;
}

inline bool IsFullResolveRegion(uint32_t sourceWidth, uint32_t sourceHeight,
                                 uint32_t destWidth, uint32_t destHeight,
                                 uint32_t destX, uint32_t destY,
                                 bool hasSrc, const plume::RenderRect& rect) {
  return sourceWidth != 0 && sourceHeight != 0 && sourceWidth == destWidth &&
      sourceHeight == destHeight && destX == 0 && destY == 0 &&
      (!hasSrc || (rect.left == 0 && rect.top == 0 &&
                   rect.right == int32_t(sourceWidth) && rect.bottom == int32_t(sourceHeight)));
}

// Resolve subresource bounds, shared by copy clipping and post-copy clear logic.
inline bool ResolveDestinationExtent(const GuestBaseTexture& texture, uint32_t level,
                                     uint32_t slice, uint32_t& width, uint32_t& height) {
  const bool cube = texture.type == ResourceType::Texture &&
      static_cast<const GuestTexture&>(texture).viewDimension ==
          plume::RenderTextureViewDimension::TEXTURE_CUBE;
  if (level >= texture.levels || level >= 32 || slice >= (cube ? 6u : 1u) ||
      texture.width == 0 || texture.height == 0)
    return false;
  width = std::max(1u, texture.width >> level);
  height = std::max(1u, texture.height >> level);
  return true;
}

// D3DDevice_Resolve copies color or depth (flags & 4) into destTexture
// (Xbox 360's EDRAM-to-linear-texture
// resolve). destPoint/sourceRect may be null (full-texture copy at 0,0),
// matching the guest API's own optional-pointer semantics.
void ResolveToTexture(GuestBaseTexture* destTexture, const GuestPoint* destPoint,
                      const GuestRect* sourceRect, uint32_t flags, uint32_t postClearFlags,
                      const float* postClearColor, float postClearZ,
                      uint32_t destLevel, uint32_t destSlice);

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
                  uint32_t vertexCount, uint64_t recordedVsDirtyFlags = 0,
                  uint64_t recordedPsDirtyFlags = 0);

// Indexed draw (FlushRenderState + drawIndexedInstanced in one render-queue job).
void DrawIndexedVertices(GuestDevice* device, uint32_t primitiveType, int32_t baseVertexIndex,
                         uint32_t startIndex, uint32_t indexCount,
                         uint64_t recordedVsDirtyFlags = 0,
                         uint64_t recordedPsDirtyFlags = 0);

// D3DDevice_DrawVerticesUP: inline (non-buffer-backed) vertex data supplied
// directly by the guest for this one draw. Uploads it to a scratch buffer
// and binds it at stream 0 for just this call -- callers must not rely on
// stream 0's tracked GuestBuffer binding surviving a call to this function.
// EndVertices requests a full live snapshot because its original Begin already
// consumed the guest dirty flags; recorded constants still use the saved masks.
void DrawUserPointerVertices(GuestDevice* device, uint32_t primitiveType, uint32_t vertexCount,
                             const void* data, uint32_t stride,
                             uint64_t recordedVsDirtyFlags = 0,
                             uint64_t recordedPsDirtyFlags = 0,
                             bool guestConsumedDirtyFlags = false);

}  // namespace fm2::render
