// render/render_commands.h
//
// Unleashed-style POD render commands. Guest threads enqueue these; the
// dedicated render thread dispatches Proc* handlers.

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
  ExecuteCommandList,
  BeginCommandList,
  WaitForGpu,
  BeginRenderStateFrame,
  CreateTextureHost,
  CreateSurfaceHost,
  UnlockTextureRect,
  UnlockBuffer16,
  UnlockBuffer32,
  CopyBufferFromUpload,
  CopyTextureFromUpload,
  CreateTranslatedTextureHost,
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

    // ExecuteCommandList / BeginCommandList / WaitForGpu / BeginRenderStateFrame:
    // no payload.

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

    struct {
      GuestBaseTexture* texture;
    } unlockTextureRect;

    struct {
      GuestBuffer* buffer;
    } unlockBuffer;

    // void* = plume::RenderBuffer* (opaque here to avoid plume include).
    struct {
      void* dst;
      void* src;
      uint64_t size;
    } copyBufferFromUpload;

    // dst = plume::RenderTexture*, src = plume::RenderBuffer*.
    struct {
      void* dst;
      void* src;
      uint32_t format;  // plume::RenderFormat
      uint32_t width;
      uint32_t height;
      uint32_t rowTexels;
      uint32_t mip;
      uint64_t srcOffset;
    } copyTextureFromUpload;

    struct {
      GuestTexture* texture;
      uint32_t width;
      uint32_t height;
      uint32_t format;  // plume::RenderFormat
      uint32_t baseAddress;
      bool* createdOut;
    } createTranslatedTextureHost;
  };
};

void DispatchRenderCommand(const RenderCommand& cmd);

void ProcExecuteCommandList();
void ProcBeginCommandList();
void ProcWaitForGpu();
void ProcCreateTextureHost(GuestTexture* texture, uint32_t width, uint32_t height, uint32_t depth,
                           uint32_t levels, uint32_t usage, uint32_t format, bool volume);
void ProcCreateSurfaceHost(GuestSurface* surface, uint32_t width, uint32_t height, uint32_t format,
                           uint32_t sampleCount, bool depth);
void ProcUnlockTextureRect(GuestBaseTexture* texture);
void ProcUnlockBuffer16(GuestBuffer* buffer);
void ProcUnlockBuffer32(GuestBuffer* buffer);
void ProcCopyBufferFromUpload(void* dst, void* src, uint64_t size);
void ProcCopyTextureFromUpload(void* dst, void* src, uint32_t format, uint32_t width, uint32_t height,
                               uint32_t rowTexels, uint32_t mip, uint64_t srcOffset);
void ProcCreateTranslatedTextureHost(GuestTexture* texture, uint32_t width, uint32_t height,
                                     uint32_t format, uint32_t baseAddress, bool* createdOut);

}  // namespace fm2::render
