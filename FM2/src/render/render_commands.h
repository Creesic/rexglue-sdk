// render/render_commands.h
//
// Unleashed-style POD render commands. Guest threads enqueue these; the
// dedicated render thread dispatches Proc* handlers. std::function Run/Enqueue
// remain only for ExecuteUpload (arbitrary copy-queue lambdas) until Unlock*
// paths are specialized.

#pragma once

#include <cstdint>

namespace fm2::render {

struct GuestBaseTexture;
struct GuestBuffer;
struct GuestResource;
struct GuestShader;
struct GuestSurface;
struct GuestTexture;
struct GuestVertexDeclaration;
struct GuestDevice;

enum class RenderCommandType : uint32_t {
  DestructResource,
  SetViewport,
  SetScissorRect,
  SetRenderTarget,
  SetImplicitRenderTarget,
  SetDepthStencilSurface,
  SetRenderState,
  SetTexture,
  SetTextureBase,
  SetVertexShader,
  SetPixelShader,
  SetVertexDeclaration,
  SetStreamSource,
  SetIndices,
  Clear,
  ResolveToTexture,
  DrawPrimitive,
  DrawIndexedPrimitive,
  DrawPrimitiveUP,
  ExecutePresent,
  WaitForGpu,
  BeginRenderStateFrame,
  CreateTextureHost,
  CreateSurfaceHost,
};

struct RenderCommand {
  RenderCommandType type{};
  union {
    struct {
      GuestResource* resource;
    } destructResource;

    struct {
      float x, y, width, height, minDepth, maxDepth;
    } setViewport;

    struct {
      int32_t left, top, right, bottom;
      bool scissorEnable;
    } setScissorRect;

    struct {
      GuestBaseTexture* renderTarget;  // null => g_implicitRenderTarget
    } setRenderTarget;

    struct {
      GuestBaseTexture* renderTarget;
    } setImplicitRenderTarget;

    struct {
      GuestSurface* depthStencil;
    } setDepthStencilSurface;

    struct {
      uint32_t state;
      uint32_t value;
    } setRenderState;

    struct {
      uint32_t index;
      GuestTexture* texture;
    } setTexture;

    struct {
      uint32_t index;
      GuestBaseTexture* texture;
    } setTextureBase;

    struct {
      GuestShader* shader;
    } setVertexShader;

    struct {
      GuestShader* shader;
    } setPixelShader;

    struct {
      GuestVertexDeclaration* declaration;
    } setVertexDeclaration;

    struct {
      uint32_t index;
      GuestBuffer* buffer;
      uint32_t offset;
      uint32_t stride;
    } setStreamSource;

    struct {
      GuestBuffer* buffer;
    } setIndices;

    struct {
      uint32_t flags;
      float color[4];
      float z;
    } clear;

    struct {
      GuestBaseTexture* destTexture;
      uint32_t destX;
      uint32_t destY;
      bool hasSrc;
      int32_t srcLeft, srcTop, srcRight, srcBottom;
    } resolveToTexture;

    struct {
      GuestDevice* device;
      uint32_t primitiveType;
      uint32_t startVertex;
      uint32_t vertexCount;
    } drawPrimitive;

    struct {
      GuestDevice* device;
      uint32_t primitiveType;
      int32_t baseVertexIndex;
      uint32_t startIndex;
      uint32_t indexCount;
    } drawIndexedPrimitive;

    struct {
      GuestDevice* device;
      uint32_t primitiveType;
      uint32_t vertexCount;
      uint8_t* vertexData;
      uint32_t stride;
      uint32_t bytes;
    } drawPrimitiveUP;

    // ExecutePresent / WaitForGpu / BeginRenderStateFrame: no payload.

    struct {
      GuestTexture* texture;
      uint32_t width;
      uint32_t height;
      uint32_t depth;
      uint32_t levels;
      uint32_t usage;
      uint32_t format;
      bool volume;
    } createTextureHost;

    struct {
      GuestSurface* surface;
      uint32_t width;
      uint32_t height;
      uint32_t format;
      uint32_t sampleCount;  // plume::RenderSampleCounts
      bool depth;
    } createSurfaceHost;
  };
};

// Called only on the render thread (or inline when queue is down / nested).
void DispatchRenderCommand(const RenderCommand& cmd);

// Implemented in video.cpp / d3d_resource_hooks.cpp; invoked from Dispatch.
void ProcExecutePresent();
void ProcWaitForGpu();
void ProcCreateTextureHost(GuestTexture* texture, uint32_t width, uint32_t height, uint32_t depth,
                           uint32_t levels, uint32_t usage, uint32_t format, bool volume);
void ProcCreateSurfaceHost(GuestSurface* surface, uint32_t width, uint32_t height, uint32_t format,
                           uint32_t sampleCount, bool depth);

}  // namespace fm2::render
