// render/render_commands.h
//
// Unleashed-style POD render commands. Guest threads enqueue these; the
// dedicated render thread dispatches Proc* handlers. std::function Enqueue/
// Run remain for Present/draws/creates until those are converted.

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
  };
};

// Called only on the render thread (or inline when queue is down / nested).
void DispatchRenderCommand(const RenderCommand& cmd);

}  // namespace fm2::render
