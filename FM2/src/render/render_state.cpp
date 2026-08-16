// render/render_state.cpp
//
// Phase 3 (render state + pipeline/shaders): state setters + dirty-state
// tracking. Rebuilt fresh rather than ported verbatim from the reference --
// that file also carried a large, admittedly-broken constant-transport/PM4
// command-buffer-replay diagnostic subsystem (an interactive shader-probe
// tool, EDRAM-tile-detection heuristics, dozens of hardcoded C:\temp log
// writes) that a prior investigation already concluded needs a fresh design,
// not a port. This file keeps only the solid, non-diagnostic state-tracking
// logic; FlushRenderState (reading this dirty state + shader constants and
// actually binding a PSO/buffers at draw time) is Phase 4 work.
//
// Deliberately deferred to Phase 4 (needs an actual draw/PSO-build path to
// make sense of): vertex-declaration <-> shader-header matching, the
// guest-data-ranged-snapshot draw path, StretchRect/resolve-snapshot
// tracking, and full two-sided stencil-state hooking (SetStencilState is
// implemented and ready below, matching D3D9 semantics exactly, but nothing
// calls it yet -- FM2's stencil render-state setters are lower-value/higher-
// risk to wire without a draw path to observe the effect against).
// SetVertexShader/SetPixelShader/SetVertexDeclaration below only track
// *which* shader/declaration is bound; resolving that into a real input
// layout happens when a PSO is actually built.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <plume_render_interface.h>

#include <rex/hash.h>  // XXH3_64bits
#include <rex/logging.h>

#include "render/guest_device.h"
#include "render/guest_heap.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_queue.h"
#include "render/render_state.h"
#include "render/shaders/placeholder_ps.hlsl.dxil.h"
#include "render/video.h"

// Spec-constant bits (XenosRecomp shared ABI -- must match the offline
// shader-translation tool's bit layout exactly).
#define SPEC_CONSTANT_R11G11B10_NORMAL (1 << 0)
#define SPEC_CONSTANT_ALPHA_TEST (1 << 1)
#define SPEC_CONSTANT_ALPHA_TO_COVERAGE (1 << 3)
#define SPEC_CONSTANT_REVERSE_Z (1 << 4)
#define SPEC_CONSTANT_UNPACK_UBYTE4_BASIS (1 << 6)
#define SPEC_CONSTANT_POSITION_F16 (1 << 7)

using namespace plume;

namespace fm2::render {

void ClearResolveSurfaceAperture(GuestBaseTexture* host);

namespace {

// ---------------------------------------------------------------------------
// Tracked pipeline-affecting state (Phase 4's PSO cache will key off this).
// ---------------------------------------------------------------------------

struct PipelineState {
  GuestShader* vertexShader = nullptr;
  GuestShader* pixelShader = nullptr;
  GuestVertexDeclaration* vertexDeclaration = nullptr;
  RenderPrimitiveTopology primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
  bool zEnable = true;
  bool zWriteEnable = true;
  RenderBlend srcBlend = RenderBlend::ONE;
  RenderBlend destBlend = RenderBlend::ZERO;
  RenderCullMode cullMode = RenderCullMode::NONE;
  RenderComparisonFunction zFunc = RenderComparisonFunction::LESS;
  bool alphaBlendEnable = false;
  RenderBlendOperation blendOp = RenderBlendOperation::ADD;
  float slopeScaledDepthBias = 0.0f;
  int32_t depthBias = 0;
  RenderBlend srcBlendAlpha = RenderBlend::ONE;
  RenderBlend destBlendAlpha = RenderBlend::ZERO;
  RenderBlendOperation blendOpAlpha = RenderBlendOperation::ADD;
  uint32_t colorWriteEnable = 0xFu;
  uint8_t vertexStrides[16]{};
  RenderFormat renderTargetFormat = RenderFormat::UNKNOWN;
  RenderFormat depthStencilFormat = RenderFormat::UNKNOWN;
  RenderSampleCounts sampleCount = RenderSampleCount::COUNT_1;
  bool depthClipEnabled = true;
  uint32_t specConstants = 0;
  bool stencilEnable = false;
  uint8_t stencilReadMask = 0xFF, stencilWriteMask = 0xFF, stencilRef = 0;
  RenderComparisonFunction stencilFrontFunc = RenderComparisonFunction::ALWAYS;
  RenderStencilOp stencilFrontFail = RenderStencilOp::KEEP;
  RenderStencilOp stencilFrontDepthFail = RenderStencilOp::KEEP;
  RenderStencilOp stencilFrontPass = RenderStencilOp::KEEP;
  RenderComparisonFunction stencilBackFunc = RenderComparisonFunction::ALWAYS;
  RenderStencilOp stencilBackFail = RenderStencilOp::KEEP;
  RenderStencilOp stencilBackDepthFail = RenderStencilOp::KEEP;
  RenderStencilOp stencilBackPass = RenderStencilOp::KEEP;
};

// Per-draw "SharedConstants" (root CBV b2) -- the fixed ABI contract shared
// with XenosRecomp's shader translation (field order/sizes must match the
// offline compiler's DEFINE_SHARED_CONSTANTS() layout exactly, not be
// invented). Several fields (swappedNormals/Binormals/Tangents/BlendWeights,
// conditionalSurveyIndex/conditionalRenderingIndex) are part of the ABI but
// have no live producer in this renderer yet -- normal/tangent packing is
// instead handled via the hasR11G11B10Normal/hasUByte4TangentBasis spec
// constants below, and occlusion-query/predication is simply unimplemented;
// they stay at 0. vteFlags is hardcoded to FM2's one observed
// PA_CL_VTE_CNTL value rather than decoded from a PM4 register.
struct SharedConstants {
  uint32_t texture2DIndices[16]{};
  uint32_t texture3DIndices[16]{};
  uint32_t textureCubeIndices[16]{};
  uint32_t samplerIndices[16]{};  // always 0 (the single default sampler) -- see FlushSamplerStates note.
  uint32_t booleans = 0;          // bits 0-15 = VS bool constants, bits 16-31 = PS bool constants.
  uint32_t swappedTexcoords = 0;
  uint32_t swappedNormals = 0;
  uint32_t swappedBinormals = 0;
  uint32_t swappedTangents = 0;
  uint32_t swappedBlendWeights = 0;
  float halfPixelOffsetX = 0.0f;
  float halfPixelOffsetY = 0.0f;
  float clipPlane[4]{};
  uint32_t clipPlaneEnabled = 0;
  float alphaThreshold = 0.0f;
  uint32_t conditionalSurveyIndex = 0;
  uint32_t conditionalRenderingIndex = 0;
  uint32_t vteFlags = 8;
};
static_assert(sizeof(SharedConstants) == 324);
static_assert(offsetof(SharedConstants, booleans) == 256);
static_assert(offsetof(SharedConstants, halfPixelOffsetX) == 280);
static_assert(offsetof(SharedConstants, clipPlane) == 288);
static_assert(offsetof(SharedConstants, vteFlags) == 320);

struct DirtyStates {
  bool renderTargetAndDepthStencil;
  bool viewport;
  bool pipelineState;
  bool scissorRect;
  uint8_t vertexStreamFirst;
  uint8_t vertexStreamLast;
  bool indices;
  explicit DirtyStates(bool value)
      : renderTargetAndDepthStencil(value),
        viewport(value),
        pipelineState(value),
        scissorRect(value),
        vertexStreamFirst(value ? 0 : 15),
        vertexStreamLast(value ? 15 : 0),
        indices(value) {}
};

template <typename T>
void SetDirtyValue(bool& dirty, T& dest, const T& src) {
  if (dest != src) {
    dest = src;
    dirty = true;
  }
}

GuestBaseTexture* g_renderTarget = nullptr;
GuestBaseTexture* g_implicitRenderTarget = nullptr;
// Last live color RT bound for drawing. Present snapshots this when the guest
// has already unbound g_renderTarget (common at Swap time).
GuestBaseTexture* g_lastPresentableRenderTarget = nullptr;
// StretchRect format-skip present override (survives Swap re-setting aperture).
std::atomic<GuestBaseTexture*> g_stretchRectPresentOverride{nullptr};
GuestSurface* g_depthStencil = nullptr;
GuestSurface* g_implicitDepthStencil = nullptr;
RenderFramebuffer* g_framebuffer = nullptr;
std::unordered_map<uint64_t, std::unique_ptr<RenderFramebuffer>> g_framebufferCache;
RenderViewport g_viewport{0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f};
PipelineState g_pipelineState;
SharedConstants g_sharedConstants;
bool g_sharedConstantsInitialized = false;
GuestTexture* g_textures[16]{};
bool g_scissorTestEnable = false;
RenderRect g_scissorRect;
RenderVertexBufferView g_vertexBufferViews[16];
RenderInputSlot g_inputSlots[16];
RenderIndexBufferView g_indexBufferView{RenderBufferReference{}, 0, RenderFormat::R16_UINT};
DirtyStates g_dirtyStates(true);
uint64_t g_frameIndex = 0;

// Unleashed-style deferred StretchRect / Resolve: surfaces accumulate
// destination textures, drained by FlushPendingStretchRectCommands before
// Present and before draws that need a fresh sample of the resolved content.
std::unordered_set<GuestSurface*> g_pendingSurfaceCopies;
std::unordered_set<GuestSurface*> g_pendingMsaaResolves;

// ---------------------------------------------------------------------------
// Constant/vertex upload allocator (Phase 4). One buffer per GPU frame slot:
// reset only after that slot's fence retires (OnRecordingFrameReady), so
// 2-frame pipelining cannot overwrite in-flight CBVs/UP vertices.
// ---------------------------------------------------------------------------

class UploadAllocator {
 public:
  void Reset() { offset_ = 0; }

  // Copies `size` bytes from `src` into the next aligned region of the
  // frame's upload buffer, returning a reference usable as a root CBV or a
  // vertex/index buffer view. If `byteSwap`, treats src/size as an array of
  // big-endian uint32_t (guest register file) and swaps into the buffer;
  // otherwise does a plain copy (host-native structs, guest UP vertex data).
  // Returns a null reference if the frame's upload budget is exhausted.
  RenderBufferReference Upload(const void* src, uint64_t size, bool byteSwap) {
    EnsureCreated();
    offset_ = (offset_ + kAlignment - 1) & ~(kAlignment - 1);
    if (offset_ + size > kBufferSize)
      return RenderBufferReference{};

    uint8_t* dst = mapped_ + offset_;
    if (byteSwap) {
      const uint32_t* s = reinterpret_cast<const uint32_t*>(src);
      uint32_t* d = reinterpret_cast<uint32_t*>(dst);
      for (uint64_t i = 0; i < size / sizeof(uint32_t); ++i)
        d[i] = std::byteswap(s[i]);
    } else {
      std::memcpy(dst, src, size);
    }
    RenderBufferReference ref = buffer_->at(offset_);
    offset_ += size;
    return ref;
  }

  void UploadAndBindRootDescriptor(const void* src, uint64_t size, uint32_t rootIndex,
                                   bool byteSwap) {
    RenderBufferReference ref = Upload(src, size, byteSwap);
    if (ref.ref == nullptr)
      return;
    CommandList()->setGraphicsRootDescriptor(ref, rootIndex);
  }

 private:
  static constexpr uint64_t kBufferSize = 4 * 1024 * 1024;
  static constexpr uint64_t kAlignment = 256;

  void EnsureCreated() {
    if (buffer_ != nullptr)
      return;
    // Used both as a root CBV source (FlushRenderState) and as a scratch
    // vertex buffer (DrawUserPointerVertices) -- flag for both usages.
    buffer_ = Device()->createBuffer(RenderBufferDesc::UploadBuffer(
        kBufferSize,
        RenderBufferFlag::CONSTANT | RenderBufferFlag::VERTEX | RenderBufferFlag::INDEX));
    mapped_ = reinterpret_cast<uint8_t*>(buffer_->map());
  }

  std::unique_ptr<RenderBuffer> buffer_;
  uint8_t* mapped_ = nullptr;
  uint64_t offset_ = 0;
};
std::array<UploadAllocator, kNumFrames> g_uploadAllocators;
std::array<std::vector<std::unique_ptr<RenderBuffer>>, kNumFrames> g_tempUploadBuffers;

UploadAllocator& CurrentUploadAllocator() {
  return g_uploadAllocators[CurrentRecordingFrame() % kNumFrames];
}

// CPU-side staging for guest→render-thread handoff (Unleashed
// IntermediaryUploadAllocator). Guest threads copy DrawUP / similar payloads
// here before Enqueue; render thread reads them while processing the job.
// Reset only after the render queue has drained those jobs (Present / WaitForGPU).
class IntermediaryUploadAllocator {
 public:
  uint8_t* Allocate(uint32_t size) {
    std::lock_guard lock(mutex_);
    constexpr uint32_t kChunk = 16 * 1024 * 1024;
    if (size > kChunk)
      return nullptr;
    if (offset_ + size > kChunk) {
      ++index_;
      offset_ = 0;
    }
    if (buffers_.size() <= index_)
      buffers_.resize(index_ + 1);
    if (buffers_[index_] == nullptr) {
      buffers_[index_] = std::make_unique<uint8_t[]>(kChunk);
    }
    uint8_t* result = buffers_[index_].get() + offset_;
    offset_ += (size + 0xFu) & ~0xFu;
    return result;
  }

  uint8_t* AllocateCopy(const void* src, uint32_t size) {
    uint8_t* dst = Allocate(size);
    if (dst != nullptr && src != nullptr && size != 0)
      std::memcpy(dst, src, size);
    return dst;
  }

  void Reset() {
    std::lock_guard lock(mutex_);
    index_ = 0;
    offset_ = 0;
  }

 private:
  std::mutex mutex_;
  std::vector<std::unique_ptr<uint8_t[]>> buffers_;
  uint32_t index_ = 0;
  uint32_t offset_ = 0;
};
IntermediaryUploadAllocator g_intermediaryUploadAllocator;

std::array<std::vector<GuestResource*>, kNumFrames> g_tempResources;

void DestructTempResources(uint32_t frame) {
  auto& resources = g_tempResources[frame % kNumFrames];
  for (GuestResource* resource : resources) {
    if (resource == nullptr || !IsFm2Resource(resource))
      continue;
    switch (resource->type) {
      case ResourceType::Texture:
      case ResourceType::VolumeTexture: {
        auto* texture = static_cast<GuestTexture*>(resource);
        if (g_renderTarget == texture)
          g_renderTarget = nullptr;
        if (g_lastPresentableRenderTarget == texture)
          g_lastPresentableRenderTarget = nullptr;
        if (g_implicitRenderTarget == texture)
          g_implicitRenderTarget = nullptr;
        ClearResolveSurfaceAperture(texture);
        if (texture->sourceSurface != nullptr) {
          texture->sourceSurface->destinationTextures.erase(texture);
          texture->sourceSurface = nullptr;
        }
        for (uint32_t i = 0; i < std::size(g_textures); ++i) {
          if (g_textures[i] == texture)
            g_textures[i] = nullptr;
        }
        if (texture->mappedMemory != nullptr) {
          ghp::GuestFreeRaw(ghp::ToGuest(texture->mappedMemory));
          texture->mappedMemory = nullptr;
        }
        FreeTextureDescriptor(texture->descriptorIndex);
        texture->textureView.reset();
        texture->textureHolder.reset();
        texture->texture = nullptr;
        texture->~GuestTexture();
        ghp::GuestFreeRaw(ghp::ToGuest(texture));
        break;
      }
      case ResourceType::VertexBuffer:
      case ResourceType::IndexBuffer: {
        auto* buffer = static_cast<GuestBuffer*>(resource);
        if (buffer->mappedMemory != nullptr) {
          ghp::GuestFreeRaw(ghp::ToGuest(buffer->mappedMemory));
          buffer->mappedMemory = nullptr;
        }
        buffer->buffer.reset();
        buffer->~GuestBuffer();
        ghp::GuestFreeRaw(ghp::ToGuest(buffer));
        break;
      }
      case ResourceType::RenderTarget:
      case ResourceType::DepthStencil: {
        auto* surface = static_cast<GuestSurface*>(resource);
        if (g_renderTarget == surface)
          g_renderTarget = nullptr;
        if (g_lastPresentableRenderTarget == surface)
          g_lastPresentableRenderTarget = nullptr;
        if (g_implicitRenderTarget == surface)
          g_implicitRenderTarget = nullptr;
        if (g_depthStencil == surface)
          g_depthStencil = nullptr;
        if (g_implicitDepthStencil == surface)
          g_implicitDepthStencil = nullptr;
        ClearResolveSurfaceAperture(surface);
        // Drop deferred StretchRect links before freeing — otherwise
        // FlushPendingStretchRectCommands can UAF this surface (null
        // plume::toD3D12 crash after VBlank unblocks more frames).
        g_pendingSurfaceCopies.erase(surface);
        g_pendingMsaaResolves.erase(surface);
        for (GuestTexture* dest : surface->destinationTextures) {
          if (dest != nullptr)
            dest->sourceSurface = nullptr;
        }
        surface->destinationTextures.clear();
        if (surface->mappedMemory != nullptr) {
          ghp::GuestFreeRaw(ghp::ToGuest(surface->mappedMemory));
          surface->mappedMemory = nullptr;
        }
        FreeTextureDescriptor(surface->descriptorIndex);
        surface->framebuffers.clear();
        surface->textureView.reset();
        surface->textureHolder.reset();
        surface->texture = nullptr;
        surface->~GuestSurface();
        ghp::GuestFreeRaw(ghp::ToGuest(surface));
        break;
      }
      case ResourceType::VertexDeclaration: {
        auto* decl = static_cast<GuestVertexDeclaration*>(resource);
        decl->~GuestVertexDeclaration();
        ghp::GuestFreeRaw(ghp::ToGuest(decl));
        break;
      }
      case ResourceType::VertexShader:
      case ResourceType::PixelShader: {
        auto* shader = static_cast<GuestShader*>(resource);
        shader->~GuestShader();
        ghp::GuestFreeRaw(ghp::ToGuest(shader));
        break;
      }
      default:
        break;
    }
  }
  resources.clear();
}

// Pending resource-layout transitions, flushed once per state-changing call.
// Key by GuestBaseTexture* (not RenderTexture*) so a destroyed/recreated host
// texture cannot leave a dangling RenderTexture* in the map across Flush.
std::unordered_map<GuestBaseTexture*, RenderTextureLayout> g_barrierMap;
std::vector<RenderTextureBarrier> g_barriers;
std::unordered_set<RenderTexture*> g_initializedAttachments;

bool IsLiveHostTexture(GuestBaseTexture* texture) {
  return texture != nullptr && IsFm2Resource(texture) && texture->textureHolder != nullptr &&
         texture->texture != nullptr && texture->texture == texture->textureHolder.get();
}

// FM2 EDRAM predicated tiling binds 1280x256 color RTs near frame end. Those
// are intermediates, not the composited frontbuffer — sticky Present must not
// adopt them (would stretch a tile band to full swapchain). Prefer host
// viewport-sized (or larger) color targets until resolve-aperture present
// lands.
bool IsFramebufferSizedPresentSource(GuestBaseTexture* texture) {
  if (!IsLiveHostTexture(texture))
    return false;
  const uint32_t frameW = Video::s_viewportWidth;
  const uint32_t frameH = Video::s_viewportHeight;
  if (frameW == 0 || frameH == 0)
    return true;
  if (texture->width == frameW && texture->height < frameH)
    return false;
  return texture->width >= frameW && texture->height >= frameH;
}

void AddBarrier(GuestBaseTexture* texture, RenderTextureLayout layout) {
  // Require a live FM2 guest object whose raw texture pointer still matches
  // the owning unique_ptr. After failed createTexture / device-removed, or
  // guest overwrite of the GuestTexture header, texture can be non-null
  // garbage and FlushBarriers would AV inside plume::barriers.
  if (!IsLiveHostTexture(texture))
    return;
  if (texture->layout == layout)
    return;
  g_barrierMap[texture] = layout;
  texture->layout = layout;
}

void FlushBarriers() {
  if (g_barrierMap.empty())
    return;
  g_barriers.clear();
  for (auto& [guestTex, layout] : g_barrierMap) {
    // Re-validate at flush: guest object may have been freed or host texture
    // reset since AddBarrier keyed this entry.
    if (!IsLiveHostTexture(guestTex))
      continue;
    g_barriers.emplace_back(guestTex->texture, layout);
  }
  g_barrierMap.clear();
  if (g_barriers.empty())
    return;
  CommandList()->barriers(RenderBarrierStage::GRAPHICS, g_barriers.data(),
                          uint32_t(g_barriers.size()));
}

void MarkAttachmentInitialized(GuestBaseTexture* texture) {
  if (texture == nullptr || texture->texture == nullptr)
    return;
  g_initializedAttachments.insert(texture->texture);
  texture->hostInitialized = true;
}

// D3D12 CREATE_NOT_ZEROED RTs/DSVs must Discard/Clear/Copy before other uses.
// Partial clearRect does not count as initialization — Discard first.
void EnsureAttachmentInitialized(GuestBaseTexture* texture) {
  if (texture == nullptr || texture->texture == nullptr)
    return;
  if (texture->hostInitialized)
    return;
  if (!texture->requiresHostInitialization) {
    texture->hostInitialized = true;
    return;
  }
  RenderCommandList* cl = CommandList();
  if (cl == nullptr)
    return;
  cl->discardTexture(texture->texture);
  MarkAttachmentInitialized(texture);
}

RenderSampleCounts GetSampleCount(GuestBaseTexture* texture) {
  if (texture != nullptr && (texture->type == ResourceType::RenderTarget ||
                             texture->type == ResourceType::DepthStencil)) {
    return static_cast<GuestSurface*>(texture)->sampleCount;
  }
  return RenderSampleCount::COUNT_1;
}

void EnsureShaderResourceDescriptor(GuestBaseTexture* texture) {
  if (texture == nullptr || texture->texture == nullptr)
    return;
  if (texture->descriptorIndex == 0)
    texture->descriptorIndex = AllocTextureDescriptor();
  TextureDescriptorSet()->setTexture(texture->descriptorIndex, texture->texture,
                                     RenderTextureLayout::SHADER_READ, texture->textureView.get());
}

void BindTextureDescriptor(uint32_t index, GuestBaseTexture* texture,
                           RenderTextureViewDimension viewDimension) {
  AddBarrier(texture, RenderTextureLayout::SHADER_READ);
  EnsureShaderResourceDescriptor(texture);
  g_sharedConstants.texture2DIndices[index] =
      (texture && viewDimension == RenderTextureViewDimension::TEXTURE_2D)
          ? texture->descriptorIndex
          : kNullTexture2DDescriptor;
  g_sharedConstants.texture3DIndices[index] =
      (texture && viewDimension == RenderTextureViewDimension::TEXTURE_3D)
          ? texture->descriptorIndex
          : kNullTexture3DDescriptor;
  g_sharedConstants.textureCubeIndices[index] =
      (texture && viewDimension == RenderTextureViewDimension::TEXTURE_CUBE)
          ? texture->descriptorIndex
          : kNullTextureCubeDescriptor;
}

GuestSurface* AsSurface(GuestBaseTexture* texture) {
  if (texture == nullptr)
    return nullptr;
  if (texture->type == ResourceType::RenderTarget || texture->type == ResourceType::DepthStencil) {
    return static_cast<GuestSurface*>(texture);
  }
  return nullptr;
}

// D3D12 CopyTextureRegion / ResolveSubresource require matching formats.
// FM2 Resolve→aperture often pairs R16G16B16A16 scene RTs with B8G8R8A8
// frontbuffers; copying those removes the device (DXGI_ERROR_INVALID_CALL).
bool FormatsCompatibleForGpuCopy(RenderFormat src, RenderFormat dst) {
  return src != RenderFormat::UNKNOWN && src == dst;
}

// When StretchRect cannot land on the aperture dest, present the live scene
// surface instead of an empty / stale frontbuffer. Swap may overwrite
// g_frontbufferPresentSource afterward; the override survives until Present.
void PreferStretchRectSourceForPresent(GuestBaseTexture* dest, GuestBaseTexture* source) {
  if (!IsLiveHostTexture(source))
    return;
  g_stretchRectPresentOverride.store(source, std::memory_order_relaxed);
  GuestBaseTexture* cur = ConsumeFrontbufferPresentSource();
  if (cur == dest || cur == nullptr) {
    SetFrontbufferPresentSource(source);
  }
}

// Unleashed 1x StretchRect uses a fullscreen blit (format conversion). Dest
// must be a color RT; src is sampled as a shader resource.
bool StretchRectShaderBlit(GuestSurface* surface, GuestTexture* texture) {
  if (!IsLiveHostTexture(surface) || !IsLiveHostTexture(texture))
    return false;
  if (RenderFormatIsDepth(texture->format) || RenderFormatIsDepth(surface->format))
    return false;

  RenderPipeline* pipeline = GetBlitPipeline(texture->format);
  if (pipeline == nullptr || PipelineLayout() == nullptr)
    return false;

  EnsureShaderResourceDescriptor(surface);
  if (texture->framebuffer == nullptr) {
    const RenderTexture* color = texture->texture;
    RenderFramebufferDesc desc(&color, 1);
    texture->framebuffer = Device()->createFramebuffer(desc);
  }
  if (texture->framebuffer == nullptr)
    return false;

  RenderCommandList* commandList = CommandList();
  if (commandList == nullptr)
    return false;

  AddBarrier(surface, RenderTextureLayout::SHADER_READ);
  AddBarrier(texture, RenderTextureLayout::COLOR_WRITE);
  FlushBarriers();

  if (g_framebuffer != texture->framebuffer.get()) {
    commandList->setFramebuffer(texture->framebuffer.get());
    g_framebuffer = texture->framebuffer.get();
  }

  TextureDescriptorSet()->setTexture(surface->descriptorIndex, surface->texture,
                                     RenderTextureLayout::SHADER_READ, surface->textureView.get());

  const uint32_t descriptorIndex = surface->descriptorIndex;
  commandList->setGraphicsPipelineLayout(PipelineLayout());
  commandList->setPipeline(pipeline);
  commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 0);
  commandList->setGraphicsDescriptorSet(SamplerDescriptorSet(), 3);
  commandList->setGraphicsPushConstants(0, &descriptorIndex, 0, sizeof(descriptorIndex));
  commandList->setViewports(
      RenderViewport(0.0f, 0.0f, float(texture->width), float(texture->height), 0.0f, 1.0f));
  commandList->setScissors(RenderRect(0, 0, texture->width, texture->height));
  commandList->drawInstanced(3, 1, 0, 0);

  MarkAttachmentInitialized(texture);
  g_stretchRectPresentOverride.store(nullptr, std::memory_order_relaxed);
  g_dirtyStates.renderTargetAndDepthStencil = true;
  g_dirtyStates.viewport = true;
  g_dirtyStates.pipelineState = true;
  g_dirtyStates.scissorRect = true;
  return true;
}

bool PopulateBarriersForStretchRect(GuestSurface* renderTarget, GuestSurface* depthStencil) {
  bool addedAny = false;
  for (GuestSurface* surface : {renderTarget, depthStencil}) {
    if (surface == nullptr || !IsLiveHostTexture(surface) || surface->destinationTextures.empty()) {
      continue;
    }
    const bool multiSampling = surface->sampleCount != RenderSampleCount::COUNT_1;
    const RenderTextureLayout srcLayout =
        multiSampling ? RenderTextureLayout::RESOLVE_SOURCE : RenderTextureLayout::COPY_SOURCE;
    const RenderTextureLayout dstLayout =
        multiSampling ? RenderTextureLayout::RESOLVE_DEST : RenderTextureLayout::COPY_DEST;
    bool anyCompatible = false;
    for (GuestTexture* texture : surface->destinationTextures) {
      if (!IsLiveHostTexture(texture))
        continue;
      if (!FormatsCompatibleForGpuCopy(surface->format, texture->format))
        continue;
      AddBarrier(texture, dstLayout);
      anyCompatible = true;
    }
    if (!anyCompatible)
      continue;
    AddBarrier(surface, srcLayout);
    addedAny = true;
  }
  return addedAny;
}

void ExecutePendingStretchRectCommands(GuestSurface* renderTarget, GuestSurface* depthStencil) {
  RenderCommandList* commandList = CommandList();
  if (commandList == nullptr)
    return;

  for (GuestSurface* surface : {renderTarget, depthStencil}) {
    if (surface == nullptr || !IsFm2Resource(surface) || surface->destinationTextures.empty()) {
      continue;
    }
    // Same live-host gate as AddBarrier: texture* can be non-null garbage after
    // holder reset / device-removed / UAF, and plume::toD3D12 would AV.
    if (!IsLiveHostTexture(surface)) {
      for (GuestTexture* texture : surface->destinationTextures) {
        if (texture != nullptr)
          texture->sourceSurface = nullptr;
      }
      surface->destinationTextures.clear();
      continue;
    }
    const bool multiSampling = surface->sampleCount != RenderSampleCount::COUNT_1;

    for (GuestTexture* texture : surface->destinationTextures) {
      if (texture == nullptr || !IsLiveHostTexture(texture)) {
        if (texture != nullptr)
          texture->sourceSurface = nullptr;
        continue;
      }
      if (!FormatsCompatibleForGpuCopy(surface->format, texture->format)) {
        static uint64_t stretchFmtSkip = 0;
        if (++stretchFmtSkip <= 24 || stretchFmtSkip % 300 == 1) {
          REXGPU_WARN("StretchRect: format mismatch {}x{} fmt={} -> {}x{} fmt={} (n={})",
                      surface->width, surface->height, int(surface->format), texture->width,
                      texture->height, int(texture->format), stretchFmtSkip);
        }
        if (!StretchRectShaderBlit(surface, texture)) {
          PreferStretchRectSourceForPresent(texture, surface);
        } else {
          for (uint32_t i = 0; i < std::size(g_textures); ++i) {
            if (g_textures[i] == texture) {
              BindTextureDescriptor(i, texture, texture->viewDimension);
            }
          }
        }
        texture->sourceSurface = nullptr;
        continue;
      }
      if (multiSampling) {
        commandList->resolveTexture(texture->texture, surface->texture);
      } else {
        // 1x→1x must copy, not ResolveSubresourceRegion (D3D12 requires MSAA src).
        commandList->copyTextureRegion(RenderTextureCopyLocation::Subresource(texture->texture, 0),
                                       RenderTextureCopyLocation::Subresource(surface->texture, 0));
      }
      MarkAttachmentInitialized(texture);
      // Compatible resolve landed — aperture is valid again.
      g_stretchRectPresentOverride.store(nullptr, std::memory_order_relaxed);
      texture->sourceSurface = nullptr;

      // Any sampler slot that still points at this texture must rebind the
      // resolved texture (not the MSAA surface) after the copy.
      for (uint32_t i = 0; i < std::size(g_textures); ++i) {
        if (g_textures[i] == texture) {
          BindTextureDescriptor(i, texture, texture->viewDimension);
        }
      }
    }
    surface->destinationTextures.clear();
  }
}

void RegisterStretchRect(GuestTexture* texture, GuestSurface* surface) {
  if (texture == nullptr || surface == nullptr)
    return;
  if (texture->sourceSurface != nullptr) {
    texture->sourceSurface->destinationTextures.erase(texture);
  }
  texture->sourceSurface = surface;
  surface->destinationTextures.insert(texture);
  g_pendingSurfaceCopies.insert(surface);

  for (uint32_t i = 0; i < std::size(g_textures); ++i) {
    if (g_textures[i] != texture)
      continue;
    if (surface->sampleCount != RenderSampleCount::COUNT_1) {
      BindTextureDescriptor(i, texture, texture->viewDimension);
      g_pendingMsaaResolves.insert(surface);
    } else {
      // Sample the live surface until the deferred copy runs (Unleashed
      // SetSurface path for non-MSAA StretchRect destinations).
      BindTextureDescriptor(i, surface, RenderTextureViewDimension::TEXTURE_2D);
    }
  }
}

RenderComparisonFunction ConvertCmpFunc(uint32_t v) {
  switch (v) {
    case D3DCMP_NEVER:
      return RenderComparisonFunction::NEVER;
    case D3DCMP_LESS:
      return RenderComparisonFunction::LESS;
    case D3DCMP_EQUAL:
      return RenderComparisonFunction::EQUAL;
    case D3DCMP_LESSEQUAL:
      return RenderComparisonFunction::LESS_EQUAL;
    case D3DCMP_GREATER:
      return RenderComparisonFunction::GREATER;
    case D3DCMP_NOTEQUAL:
      return RenderComparisonFunction::NOT_EQUAL;
    case D3DCMP_GREATEREQUAL:
      return RenderComparisonFunction::GREATER_EQUAL;
    case D3DCMP_ALWAYS:
      return RenderComparisonFunction::ALWAYS;
    default:
      return RenderComparisonFunction::ALWAYS;
  }
}

RenderBlend ConvertBlendMode(uint32_t v) {
  switch (v) {
    case D3DBLEND_ZERO:
      return RenderBlend::ZERO;
    case D3DBLEND_ONE:
      return RenderBlend::ONE;
    case D3DBLEND_SRCCOLOR:
      return RenderBlend::SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR:
      return RenderBlend::INV_SRC_COLOR;
    case D3DBLEND_SRCALPHA:
      return RenderBlend::SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA:
      return RenderBlend::INV_SRC_ALPHA;
    case D3DBLEND_DESTCOLOR:
      return RenderBlend::DEST_COLOR;
    case D3DBLEND_INVDESTCOLOR:
      return RenderBlend::INV_DEST_COLOR;
    case D3DBLEND_DESTALPHA:
      return RenderBlend::DEST_ALPHA;
    case D3DBLEND_INVDESTALPHA:
      return RenderBlend::INV_DEST_ALPHA;
    default:
      return RenderBlend::ONE;
  }
}

RenderBlendOperation ConvertBlendOp(uint32_t v) {
  switch (v) {
    case D3DBLENDOP_ADD:
      return RenderBlendOperation::ADD;
    case D3DBLENDOP_SUBTRACT:
      return RenderBlendOperation::SUBTRACT;
    case D3DBLENDOP_MIN:
      return RenderBlendOperation::MIN;
    case D3DBLENDOP_MAX:
      return RenderBlendOperation::MAX;
    case D3DBLENDOP_REVSUBTRACT:
      return RenderBlendOperation::REV_SUBTRACT;
    default:
      return RenderBlendOperation::ADD;
  }
}

// Xenos EStencilOp (0=keep,1=zero,2=replace,3=incr-sat,4=decr-sat,5=invert,
// 6=incr,7=decr), matching real D3DSTENCILOP ordinal layout.
RenderStencilOp ConvertStencilOp(uint32_t v) {
  switch (v) {
    case 0: return RenderStencilOp::KEEP;
    case 1: return RenderStencilOp::ZERO;
    case 2: return RenderStencilOp::REPLACE;
    case 3: return RenderStencilOp::INCREMENT_AND_CLAMP;
    case 4: return RenderStencilOp::DECREMENT_AND_CLAMP;
    case 5: return RenderStencilOp::INVERT;
    case 6: return RenderStencilOp::INCREMENT_AND_WRAP;
    case 7: return RenderStencilOp::DECREMENT_AND_WRAP;
    default: return RenderStencilOp::KEEP;
  }
}

void SetAlphaTestMode(bool enable) {
  uint32_t specConstants = enable ? SPEC_CONSTANT_ALPHA_TEST : 0;
  specConstants |= g_pipelineState.specConstants & ~uint32_t(SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
}

// FM2's clip planes and enable mask live at fixed byte offsets inside the
// guest device struct (this repo's GuestDevice is a byte-exact overlay of
// the real object, so these offsets refer to real, live guest memory).
constexpr uint32_t kGuestClipPlanesOffset = 0x2820;
constexpr uint32_t kGuestClipPlaneEnableOffset = 0x2944;
constexpr uint32_t kGuestClipPlaneMask = 0x3F;
constexpr uint32_t kGuestScissorEnableOffset = 0x2E48;

struct GuestClipPlane {
  rex::be<float> x, y, z, w;
};

GuestClipPlane* ClipPlanes(GuestDevice* device) {
  return reinterpret_cast<GuestClipPlane*>(reinterpret_cast<uint8_t*>(device) +
                                           kGuestClipPlanesOffset);
}

uint32_t ClipPlaneEnableMask(GuestDevice* device) {
  auto* value = reinterpret_cast<rex::be<uint32_t>*>(reinterpret_cast<uint8_t*>(device) +
                                                     kGuestClipPlaneEnableOffset);
  return value->get() & kGuestClipPlaneMask;
}

bool ScissorTestEnabled(GuestDevice* device) {
  auto* value = reinterpret_cast<rex::be<uint32_t>*>(reinterpret_cast<uint8_t*>(device) +
                                                     kGuestScissorEnableOffset);
  return value->get() != 0;
}

// Simplified framebuffer cache: just caches one RenderFramebuffer per unique
// (color, depth) attachment pair. Tile 1280x256 surfaces are allocated at
// full 720p host size in ProcCreateSurfaceHost (safe create-time grow).
void SetFramebuffer(GuestBaseTexture* colorTarget, GuestSurface* depthTarget, bool /*forClear*/) {
  const GuestBaseTexture* dimensionSource = colorTarget != nullptr ? colorTarget : depthTarget;
  if (dimensionSource != nullptr && dimensionSource->width != 0 && dimensionSource->height != 0) {
    g_sharedConstants.halfPixelOffsetX = 1.0f / float(dimensionSource->width);
    g_sharedConstants.halfPixelOffsetY = -1.0f / float(dimensionSource->height);
  }

  // Don't shift away the high half of 64-bit host pointers.
  const uint64_t key =
      (uint64_t(uintptr_t(colorTarget)) * 0x9E3779B97F4A7C15ull) ^ uint64_t(uintptr_t(depthTarget));
  auto it = g_framebufferCache.find(key);
  if (it != g_framebufferCache.end()) {
    g_framebuffer = it->second.get();
  } else {
    const RenderTexture* colorTex = colorTarget != nullptr ? colorTarget->texture : nullptr;
    const RenderTexture* depthTex = depthTarget != nullptr ? depthTarget->texture : nullptr;
    if (colorTex == nullptr && depthTex == nullptr) {
      g_framebuffer = nullptr;
      CommandList()->setFramebuffer(nullptr);
      return;
    }
    RenderFramebufferDesc desc(colorTex != nullptr ? &colorTex : nullptr,
                               colorTex != nullptr ? 1u : 0u);
    desc.depthAttachment = depthTex;
    auto fb = Device()->createFramebuffer(desc);
    g_framebuffer = fb.get();
    g_framebufferCache.emplace(key, std::move(fb));
  }
  CommandList()->setFramebuffer(g_framebuffer);
}

}  // namespace

namespace {

std::mutex g_surfaceApertureMutex;
struct SurfaceApertureEntry {
  GuestBaseTexture* tex = nullptr;
  bool resolveDest = false;
};
std::unordered_map<uint32_t, SurfaceApertureEntry> g_surfaceAperture;
std::atomic<GuestBaseTexture*> g_frontbufferPresentSource{nullptr};

}  // namespace

void RegisterResolveSurfaceAperture(uint32_t guestAddr, GuestBaseTexture* host) {
  guestAddr &= 0x1FFFFFFFu;
  if (host == nullptr || host->texture == nullptr || guestAddr < 0x08000000u ||
      guestAddr >= 0x20000000u) {
    return;
  }
  std::lock_guard<std::mutex> lk(g_surfaceApertureMutex);
  SurfaceApertureEntry& e = g_surfaceAperture[guestAddr & ~0xFFFu];
  e.tex = host;
  e.resolveDest = true;
}

GuestBaseTexture* LookupResolveSurfaceAperture(uint32_t guestAddr) {
  std::lock_guard<std::mutex> lk(g_surfaceApertureMutex);
  auto it = g_surfaceAperture.find(guestAddr & 0x1FFFFFFFu & ~0xFFFu);
  if (it == g_surfaceAperture.end())
    return nullptr;
  GuestBaseTexture* tex = it->second.tex;
  if (tex == nullptr || tex->texture == nullptr)
    return nullptr;
  return tex;
}

void ClearResolveSurfaceAperture(GuestBaseTexture* host) {
  if (host == nullptr)
    return;
  std::lock_guard<std::mutex> lk(g_surfaceApertureMutex);
  for (auto it = g_surfaceAperture.begin(); it != g_surfaceAperture.end();) {
    if (it->second.tex == host) {
      it = g_surfaceAperture.erase(it);
    } else {
      ++it;
    }
  }
  GuestBaseTexture* expected = host;
  g_frontbufferPresentSource.compare_exchange_strong(expected, nullptr, std::memory_order_relaxed);
}

void SetFrontbufferPresentSource(GuestBaseTexture* tex) {
  g_frontbufferPresentSource.store(tex, std::memory_order_relaxed);
}

GuestBaseTexture* ConsumeFrontbufferPresentSource() {
  return g_frontbufferPresentSource.load(std::memory_order_relaxed);
}

GuestBaseTexture* ConsumeStretchRectPresentOverride() {
  // Sticky until a format-compatible StretchRect succeeds (cleared there).
  // Do not exchange here — Swap can land many frames between HDR resolves.
  GuestBaseTexture* tex = g_stretchRectPresentOverride.load(std::memory_order_relaxed);
  if (tex != nullptr && !IsLiveHostTexture(tex)) {
    g_stretchRectPresentOverride.store(nullptr, std::memory_order_relaxed);
    return nullptr;
  }
  return tex;
}

RenderBufferReference UploadFrameData(const void* src, uint64_t size, bool byteSwap) {
  return CurrentUploadAllocator().Upload(src, size, byteSwap);
}

void RetainTempUploadBuffer(std::unique_ptr<RenderBuffer> buffer) {
  if (buffer == nullptr)
    return;
  g_tempUploadBuffers[CurrentRecordingFrame() % kNumFrames].push_back(std::move(buffer));
}

void FlushPendingStretchRectCommands() {
  // Caller is expected to already be on the render thread (Present / Flush /
  // Draw). Nested RenderQueue::Run is fine if not.
  bool foundAny = false;
  for (GuestSurface* surface : g_pendingSurfaceCopies) {
    if (surface != nullptr && surface->type != ResourceType::DepthStencil) {
      foundAny |= PopulateBarriersForStretchRect(surface, nullptr);
    }
  }
  for (GuestSurface* surface : g_pendingMsaaResolves) {
    const bool isDepth = surface != nullptr && surface->type == ResourceType::DepthStencil;
    foundAny |=
        PopulateBarriersForStretchRect(isDepth ? nullptr : surface, isDepth ? surface : nullptr);
  }

  const bool havePending = !g_pendingSurfaceCopies.empty() || !g_pendingMsaaResolves.empty();
  if (havePending) {
    // Unleashed StretchRect samples/copies the color surface after it leaves
    // COLOR_WRITE. Leaving the RT bound as the active framebuffer blocks the
    // 1x copyTextureRegion path (and would block a shader blit too).
    if (foundAny && g_framebuffer != nullptr) {
      CommandList()->setFramebuffer(nullptr);
      g_framebuffer = nullptr;
      g_dirtyStates.renderTargetAndDepthStencil = true;
    }
    if (foundAny)
      FlushBarriers();
    static uint64_t stretchFlush = 0;
    ++stretchFlush;
    if (stretchFlush <= 12 || stretchFlush % 300 == 1) {
      REXGPU_INFO("FlushPendingStretchRect: draining {} surface(s) (n={})",
                  g_pendingSurfaceCopies.size(), stretchFlush);
    }
    // Always Execute: format-incompatible dests still need present-source
    // rewrite even when no COPY barriers were populated.
    for (GuestSurface* surface : g_pendingSurfaceCopies) {
      if (surface != nullptr && surface->type != ResourceType::DepthStencil) {
        ExecutePendingStretchRectCommands(surface, nullptr);
      }
    }
    for (GuestSurface* surface : g_pendingMsaaResolves) {
      const bool isDepth = surface != nullptr && surface->type == ResourceType::DepthStencil;
      ExecutePendingStretchRectCommands(isDepth ? nullptr : surface, isDepth ? surface : nullptr);
    }
  }

  // Clear dangling sourceSurface links on any leftovers (e.g. depth skipped).
  for (GuestSurface* surface : g_pendingSurfaceCopies) {
    if (surface == nullptr)
      continue;
    for (GuestTexture* texture : surface->destinationTextures) {
      if (texture != nullptr)
        texture->sourceSurface = nullptr;
    }
    surface->destinationTextures.clear();
  }
  g_pendingSurfaceCopies.clear();
  g_pendingMsaaResolves.clear();
}

// ---------------------------------------------------------------------------
// Per-frame bookkeeping.
// ---------------------------------------------------------------------------

void BeginRenderStateFrame() {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::BeginRenderStateFrame;
  RenderQueue::Run(cmd);
}

namespace {

void ProcBeginRenderStateFrame() {
  if (IsDeviceLost())
    return;
  ++g_frameIndex;
  g_framebuffer = nullptr;
  g_dirtyStates = DirtyStates(true);
  // Upload allocator reset happens in OnRecordingFrameReady after the
  // slot's fence retires -- do not Reset() here (would clobber in-flight
  // uploads from the other pipelined frame).
  if (!g_sharedConstantsInitialized) {
    for (uint32_t i = 0; i < std::size(g_sharedConstants.texture2DIndices); ++i) {
      g_sharedConstants.texture2DIndices[i] = kNullTexture2DDescriptor;
      g_sharedConstants.texture3DIndices[i] = kNullTexture3DDescriptor;
      g_sharedConstants.textureCubeIndices[i] = kNullTextureCubeDescriptor;
    }
    g_sharedConstantsInitialized = true;
  }

  RenderCommandList* commandList = CommandList();
  if (commandList == nullptr)
    return;
  commandList->setGraphicsPipelineLayout(PipelineLayout());
  commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 0);
  commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 1);
  commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 2);
  commandList->setGraphicsDescriptorSet(SamplerDescriptorSet(), 3);
}

}  // namespace

// Render-thread entry: Video::ProcBeginCommandList calls this after the prior
// slot's fence retires. Swap used to Run BeginRenderStateFrame; that nested
// sync was removed to avoid freezes, which left g_frameIndex stuck at 0 and
// starved once-per-frame guest texture uploads (lastUploadFrame == 0 forever).
void NotifyRenderFrameBegin() {
  ProcBeginRenderStateFrame();
}

uint64_t CurrentFrameIndex() {
  return g_frameIndex;
}

// ---------------------------------------------------------------------------
// Render state.
// ---------------------------------------------------------------------------

void ApplyRenderState(uint32_t state, uint32_t value) {
  switch (state) {
    case D3DRS_ZENABLE:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zEnable, value != 0);
      g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
      break;
    case D3DRS_ZWRITEENABLE:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable, value != 0);
      break;
    case D3DRS_ALPHATESTENABLE:
      SetAlphaTestMode(value != 0);
      break;
    case D3DRS_SRCBLEND:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlend, ConvertBlendMode(value));
      break;
    case D3DRS_DESTBLEND:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlend,
                    ConvertBlendMode(value));
      break;
    case D3DRS_CULLMODE: {
      RenderCullMode cull = RenderCullMode::NONE;
      switch (value) {
        case D3DCULL_NONE:
        case D3DCULL_NONE_2:
          cull = RenderCullMode::NONE;
          break;
        case D3DCULL_CW:
          cull = RenderCullMode::FRONT;
          break;
        case D3DCULL_CCW:
          cull = RenderCullMode::BACK;
          break;
      }
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.cullMode, cull);
      break;
    }
    case D3DRS_ZFUNC:
      // No compare-func flip: FM2's reversed viewport (see SetViewport)
      // already carries its reverse-Z scheme faithfully end to end.
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zFunc, ConvertCmpFunc(value));
      break;
    case D3DRS_ALPHAREF:
      g_sharedConstants.alphaThreshold = float(value) / 256.0f;
      break;
    case D3DRS_ALPHABLENDENABLE:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.alphaBlendEnable, value != 0);
      break;
    case D3DRS_BLENDOP:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.blendOp, ConvertBlendOp(value));
      break;
    case D3DRS_SCISSORTESTENABLE:
      SetDirtyValue(g_dirtyStates.scissorRect, g_scissorTestEnable, value != 0);
      break;
    case D3DRS_SLOPESCALEDEPTHBIAS:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.slopeScaledDepthBias,
                    *reinterpret_cast<float*>(&value));
      break;
    case D3DRS_DEPTHBIAS:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthBias,
                    int32_t(*reinterpret_cast<float*>(&value) * (1 << 24)));
      break;
    case D3DRS_SRCBLENDALPHA:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlendAlpha,
                    ConvertBlendMode(value));
      break;
    case D3DRS_DESTBLENDALPHA:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlendAlpha,
                    ConvertBlendMode(value));
      break;
    case D3DRS_COLORWRITEENABLE:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.colorWriteEnable, value);
      g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
      break;
    default:
      break;
  }
}

void SetRenderState(GuestDevice* /*device*/, uint32_t state, uint32_t value) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetRenderState;
  cmd.setRenderState.state = state;
  cmd.setRenderState.value = value;
  RenderQueue::Enqueue(cmd);
}

void SetViewportEnable(GuestDevice* /*device*/, uint32_t value) {
  // The Xenos ViewportEnable render state maps to PA_CL_CLIP_CNTL.clip_disable.
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetViewportEnable;
  cmd.setViewportEnable.value = value;
  RenderQueue::Enqueue(cmd);
}

void ProcSetViewportEnable(uint32_t value) {
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthClipEnabled, value != 0);
}

void UpdateClipPlaneConstants(GuestDevice* device) {
  const uint32_t enabledMask = ClipPlaneEnableMask(device);
  g_sharedConstants.clipPlaneEnabled = enabledMask != 0 ? 1 : 0;
  if (enabledMask == 0)
    return;

  const uint32_t planeIndex = std::countr_zero(enabledMask);
  const GuestClipPlane& plane = ClipPlanes(device)[planeIndex];
  g_sharedConstants.clipPlane[0] = plane.x.get();
  g_sharedConstants.clipPlane[1] = plane.y.get();
  g_sharedConstants.clipPlane[2] = plane.z.get();
  g_sharedConstants.clipPlane[3] = plane.w.get();
}

void SetDepthState(uint32_t zEnable, uint32_t zWriteEnable, uint32_t cmpFunc) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetDepthState;
  cmd.setDepthState.zEnable = zEnable;
  cmd.setDepthState.zWriteEnable = zWriteEnable;
  cmd.setDepthState.cmpFunc = cmpFunc;
  RenderQueue::Enqueue(cmd);
}

void ProcSetDepthState(uint32_t zEnable, uint32_t zWriteEnable, uint32_t cmpFunc) {
  const bool ze = zEnable != 0;
  if (g_pipelineState.zEnable != ze)
    g_dirtyStates.renderTargetAndDepthStencil = true;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zEnable, ze);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable, zWriteEnable != 0);
  // No compare-func flip -- see SetRenderState's D3DRS_ZFUNC case.
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zFunc, ConvertCmpFunc(cmpFunc));
}

void SetStencilState(const GuestStencilState& s) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetStencilState;
  cmd.setStencilState.enable = s.enable ? 1u : 0u;
  cmd.setStencilState.twoSided = s.twoSided ? 1u : 0u;
  cmd.setStencilState.frontFunc = s.frontFunc;
  cmd.setStencilState.frontFail = s.frontFail;
  cmd.setStencilState.frontDepthFail = s.frontDepthFail;
  cmd.setStencilState.frontPass = s.frontPass;
  cmd.setStencilState.backFunc = s.backFunc;
  cmd.setStencilState.backFail = s.backFail;
  cmd.setStencilState.backDepthFail = s.backDepthFail;
  cmd.setStencilState.backPass = s.backPass;
  cmd.setStencilState.readMask = s.readMask;
  cmd.setStencilState.writeMask = s.writeMask;
  cmd.setStencilState.ref = s.ref;
  RenderQueue::Enqueue(cmd);
}

void ProcSetStencilState(uint32_t enable, uint32_t twoSided, uint32_t frontFunc, uint32_t frontFail,
                         uint32_t frontDepthFail, uint32_t frontPass, uint32_t backFunc,
                         uint32_t backFail, uint32_t backDepthFail, uint32_t backPass,
                         uint32_t readMask, uint32_t writeMask, uint32_t ref) {
  const bool en = enable != 0;
  if (g_pipelineState.stencilEnable != en)
    g_dirtyStates.renderTargetAndDepthStencil = true;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilEnable, en);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontFunc,
                ConvertCmpFunc(frontFunc));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontFail,
                ConvertStencilOp(frontFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontDepthFail,
                ConvertStencilOp(frontDepthFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontPass,
                ConvertStencilOp(frontPass));

  const uint32_t useBack = twoSided != 0;
  const uint32_t bf = useBack ? backFunc : frontFunc;
  const uint32_t bfail = useBack ? backFail : frontFail;
  const uint32_t bdfail = useBack ? backDepthFail : frontDepthFail;
  const uint32_t bpass = useBack ? backPass : frontPass;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackFunc, ConvertCmpFunc(bf));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackFail,
                ConvertStencilOp(bfail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackDepthFail,
                ConvertStencilOp(bdfail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackPass,
                ConvertStencilOp(bpass));

  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilRef, uint8_t(ref));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilReadMask,
                         uint8_t(readMask));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilWriteMask,
                         uint8_t(writeMask));
}

// ---------------------------------------------------------------------------
// Texture binding.
// ---------------------------------------------------------------------------

namespace {

void ProcSetTexture(uint32_t index, GuestTexture* texture) {
  if (IsDeviceLost())
    return;

  // Unleashed ProcSetTexture: if a StretchRect linked this texture to a
  // source surface, either sample the surface directly (1x) or mark MSAA
  // for resolve-before-draw and bind the destination texture.
  if (texture != nullptr && texture->sourceSurface != nullptr) {
    GuestSurface* surface = texture->sourceSurface;
    if (surface->sampleCount != RenderSampleCount::COUNT_1) {
      g_pendingMsaaResolves.insert(surface);
      BindTextureDescriptor(index, texture, texture->viewDimension);
    } else {
      BindTextureDescriptor(index, surface, RenderTextureViewDimension::TEXTURE_2D);
    }
    g_textures[index] = texture;
    return;
  }

  GuestBaseTexture* bound = texture;
  RenderTextureViewDimension viewDimension =
      texture ? texture->viewDimension : RenderTextureViewDimension::UNKNOWN;

  if (texture != nullptr && texture->sourceTexture != nullptr) {
    bound = texture->sourceTexture;
    viewDimension = RenderTextureViewDimension::TEXTURE_2D;
  }

  BindTextureDescriptor(index, bound, viewDimension);
  g_textures[index] = texture;
}

void ProcSetTextureBase(uint32_t index, GuestBaseTexture* texture) {
  if (IsDeviceLost())
    return;
  if (texture == nullptr || texture->texture == nullptr) {
    BindTextureDescriptor(index, nullptr, RenderTextureViewDimension::UNKNOWN);
    g_textures[index] = nullptr;
    return;
  }
  GuestBaseTexture* bound = texture->sourceTexture != nullptr ? texture->sourceTexture : texture;
  BindTextureDescriptor(index, bound, RenderTextureViewDimension::TEXTURE_2D);
  g_textures[index] = nullptr;
}

void CompleteVertexDeclaration(GuestVertexDeclaration* decl);

void ApplyVertexDeclarationMetadata(GuestVertexDeclaration* declaration) {
  if (declaration != nullptr)
    CompleteVertexDeclaration(declaration);

  g_sharedConstants.swappedTexcoords = declaration != nullptr ? declaration->swappedTexcoords : 0;
  g_sharedConstants.swappedBlendWeights =
      declaration != nullptr ? declaration->swappedBlendWeights : 0;

  constexpr uint32_t kDeclarationSpecConstants = SPEC_CONSTANT_R11G11B10_NORMAL |
                                                 SPEC_CONSTANT_UNPACK_UBYTE4_BASIS |
                                                 SPEC_CONSTANT_POSITION_F16;
  uint32_t specConstants = g_pipelineState.specConstants & ~kDeclarationSpecConstants;
  if (declaration != nullptr) {
    if (declaration->hasR11G11B10Normal)
      specConstants |= SPEC_CONSTANT_R11G11B10_NORMAL;
    if (declaration->hasUByte4TangentBasis)
      specConstants |= SPEC_CONSTANT_UNPACK_UBYTE4_BASIS;
    if (declaration->hasFloat16Position)
      specConstants |= SPEC_CONSTANT_POSITION_F16;
  }
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
}

void ProcSetVertexShader(GuestShader* shader) {
  GuestShader* live = (shader != nullptr && IsFm2Resource(shader)) ? shader : nullptr;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexShader, live);
}

void ProcSetPixelShader(GuestShader* shader) {
  GuestShader* live = (shader != nullptr && IsFm2Resource(shader)) ? shader : nullptr;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.pixelShader, live);
}

void ProcSetVertexDeclaration(GuestVertexDeclaration* declaration) {
  GuestVertexDeclaration* live =
      (declaration != nullptr && IsFm2Resource(declaration)) ? declaration : nullptr;
  // Tier A step 3: decl → swappedTexcoords / blendWeights + SPEC_CONSTANT_* bits.
  ApplyVertexDeclarationMetadata(live);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexDeclaration, live);
}

void ProcSetStreamSource(uint32_t index, GuestBuffer* buffer, uint32_t offset, uint32_t stride) {
  if (index >= 16u)
    return;

  GuestBuffer* live = (buffer != nullptr && IsFm2Resource(buffer) && buffer->buffer != nullptr &&
                       offset <= buffer->dataSize)
                          ? buffer
                          : nullptr;

  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[index],
                uint8_t(live ? stride : 0));

  bool dirty = false;
  SetDirtyValue(dirty, g_vertexBufferViews[index].buffer,
                live ? live->buffer->at(offset) : RenderBufferReference{});
  SetDirtyValue(dirty, g_vertexBufferViews[index].size, live ? (live->dataSize - offset) : 0u);
  SetDirtyValue(dirty, g_inputSlots[index].stride, live ? stride : 0u);
  if (dirty) {
    g_dirtyStates.vertexStreamFirst =
        std::min<uint8_t>(g_dirtyStates.vertexStreamFirst, uint8_t(index));
    g_dirtyStates.vertexStreamLast =
        std::max<uint8_t>(g_dirtyStates.vertexStreamLast, uint8_t(index));
  }
}

void ProcSetIndices(GuestBuffer* buffer) {
  GuestBuffer* live =
      (buffer != nullptr && IsFm2Resource(buffer) && buffer->buffer != nullptr) ? buffer : nullptr;
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer,
                live ? live->buffer->at(0) : RenderBufferReference{});
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format,
                live ? live->format : RenderFormat::R16_UINT);
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, live ? live->dataSize : 0u);
}

void ProcSetViewport(float x, float y, float width, float height, float minZ, float maxZ) {
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.x, x);
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.y, y);
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.width, width);
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.height, height);
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.minDepth, minZ);
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.maxDepth, maxZ);

  uint32_t specConstants = g_pipelineState.specConstants;
  if (minZ > maxZ) {
    specConstants |= SPEC_CONSTANT_REVERSE_Z;
  } else {
    specConstants &= ~uint32_t(SPEC_CONSTANT_REVERSE_Z);
  }
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
  g_dirtyStates.scissorRect |= g_dirtyStates.viewport;
}

void ProcSetScissorRect(bool scissorEnable, int32_t top, int32_t left, int32_t bottom,
                        int32_t right) {
  SetDirtyValue(g_dirtyStates.scissorRect, g_scissorTestEnable, scissorEnable);
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.top, top);
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.left, left);
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.bottom, bottom);
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.right, right);
}

void SetRenderTargetInternal(GuestBaseTexture* renderTarget) {
  SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_renderTarget, renderTarget);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.renderTargetFormat,
                renderTarget ? renderTarget->format : RenderFormat::UNKNOWN);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.sampleCount,
                GetSampleCount(renderTarget));
  SetAlphaTestMode((g_pipelineState.specConstants &
                    (SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE)) != 0);

  // Remember the last full-frame color target for Present. Swap often runs
  // after the guest has unbound the RT; blitting nullptr yields a clear.
  // Skip EDRAM tile-height binds (e.g. 1280x256) so they cannot displace the
  // real 720p present source.
  if (IsFramebufferSizedPresentSource(renderTarget)) {
    g_lastPresentableRenderTarget = renderTarget;
  }

  if (renderTarget != nullptr && renderTarget->width != 0 && renderTarget->height != 0) {
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.x, 0.0f);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.y, 0.0f);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.width, float(renderTarget->width));
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.height, float(renderTarget->height));
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.minDepth, 0.0f);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.maxDepth, 1.0f);
    g_dirtyStates.scissorRect |= g_dirtyStates.viewport;
  }
}

void ProcSetRenderTarget(GuestBaseTexture* renderTarget) {
  GuestBaseTexture* target = renderTarget != nullptr ? renderTarget : g_implicitRenderTarget;
  SetRenderTargetInternal(target);
}

void ProcSetImplicitRenderTarget(GuestBaseTexture* renderTarget) {
  g_implicitRenderTarget = renderTarget;
  SetRenderTargetInternal(renderTarget);
}

void ProcSetDepthStencilSurface(GuestSurface* depthStencil) {
  SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_depthStencil, depthStencil);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthStencilFormat,
                depthStencil ? depthStencil->format : RenderFormat::UNKNOWN);
  g_dirtyStates.viewport = true;
  if (depthStencil != nullptr)
    g_implicitDepthStencil = depthStencil;
}

void ProcDestructResource(GuestResource* resource) {
  g_tempResources[CurrentRecordingFrame() % kNumFrames].push_back(resource);
}

}  // namespace

void SetTexture(GuestDevice* /*device*/, uint32_t index, GuestTexture* texture) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetTexture;
  cmd.setTexture.index = index;
  cmd.setTexture.texture = texture;
  RenderQueue::Enqueue(cmd);
}

void SetTextureBase(GuestDevice* /*device*/, uint32_t index, GuestBaseTexture* texture) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetTextureBase;
  cmd.setTextureBase.index = index;
  cmd.setTextureBase.texture = texture;
  RenderQueue::Enqueue(cmd);
}

void SetVertexShader(GuestDevice* /*device*/, GuestShader* shader) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetVertexShader;
  cmd.setVertexShader.shader = shader;
  RenderQueue::Enqueue(cmd);
}

void SetPixelShader(GuestDevice* /*device*/, GuestShader* shader) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetPixelShader;
  cmd.setPixelShader.shader = shader;
  RenderQueue::Enqueue(cmd);
}

void SetVertexDeclaration(GuestDevice* /*device*/, GuestVertexDeclaration* declaration) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetVertexDeclaration;
  cmd.setVertexDeclaration.declaration = declaration;
  RenderQueue::Enqueue(cmd);
}

void SetStreamSource(GuestDevice* /*device*/, uint32_t index, GuestBuffer* buffer, uint32_t offset,
                     uint32_t stride) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetStreamSource;
  cmd.setStreamSource.index = index;
  cmd.setStreamSource.buffer = buffer;
  cmd.setStreamSource.offset = offset;
  cmd.setStreamSource.stride = stride;
  RenderQueue::Enqueue(cmd);
}

void SetIndices(GuestDevice* /*device*/, GuestBuffer* buffer) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetIndices;
  cmd.setIndices.buffer = buffer;
  RenderQueue::Enqueue(cmd);
}

void SetViewport(GuestDevice* /*device*/, GuestViewport* viewport) {
  // D3D9 validation: a zero-sized viewport is INVALIDCALL and leaves state
  // unchanged. Read guest be<> values on the caller thread, then enqueue.
  if (viewport->width.get() == 0 || viewport->height.get() == 0)
    return;
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetViewport;
  cmd.setViewport.x = float(viewport->x.get());
  cmd.setViewport.y = float(viewport->y.get());
  cmd.setViewport.width = float(viewport->width.get());
  cmd.setViewport.height = float(viewport->height.get());
  cmd.setViewport.minDepth = viewport->minZ.get();
  cmd.setViewport.maxDepth = viewport->maxZ.get();
  RenderQueue::Enqueue(cmd);
}

void SetScissorRect(GuestDevice* device, GuestRect* rect) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetScissorRect;
  cmd.setScissorRect.scissorEnable = ScissorTestEnabled(device);
  cmd.setScissorRect.top = rect->top.get();
  cmd.setScissorRect.left = rect->left.get();
  cmd.setScissorRect.bottom = rect->bottom.get();
  cmd.setScissorRect.right = rect->right.get();
  RenderQueue::Enqueue(cmd);
}

void SetRenderTarget(GuestDevice* /*device*/, uint32_t index, GuestBaseTexture* renderTarget) {
  if (index != 0)
    return;  // FM2 only ever uses a single color render target.
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetRenderTarget;
  cmd.setRenderTarget.renderTarget = renderTarget;
  RenderQueue::Enqueue(cmd);
}

void SetImplicitRenderTarget(GuestBaseTexture* renderTarget) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetImplicitRenderTarget;
  cmd.setImplicitRenderTarget.renderTarget = renderTarget;
  RenderQueue::Enqueue(cmd);
}

GuestBaseTexture* GetCurrentColorRenderTarget() {
  // Prefer a full-frame color target. Tile-sized current binds must not win
  // Present over sticky/implicit 720p surfaces.
  if (IsFramebufferSizedPresentSource(g_renderTarget))
    return g_renderTarget;
  if (IsFramebufferSizedPresentSource(g_lastPresentableRenderTarget)) {
    return g_lastPresentableRenderTarget;
  }
  if (IsFramebufferSizedPresentSource(g_implicitRenderTarget))
    return g_implicitRenderTarget;
  return nullptr;
}

void PrepareFramePresent() {
  // PresentImpl/ExecuteCommandList on the render thread snapshots g_renderTarget after prior
  // Enqueue'd SetRT jobs (FIFO with Present's Run). Guest-side snapshot here
  // would race async binds -- kept as a no-op hook for call-site compatibility.
}

void SetDepthStencilSurface(GuestDevice* /*device*/, GuestSurface* depthStencil) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetDepthStencilSurface;
  cmd.setDepthStencilSurface.depthStencil = depthStencil;
  RenderQueue::Enqueue(cmd);
}

void OnRecordingFrameReady(uint32_t frame) {
  DestructTempResources(frame);
  g_uploadAllocators[frame % kNumFrames].Reset();
  g_tempUploadBuffers[frame % kNumFrames].clear();
  // Intermediary is shared; safe to reset once the queue has drained jobs that
  // pointed into it (Present/WaitForGPU call this after sync Run).
  g_intermediaryUploadAllocator.Reset();
}

void ScheduleResourceDestruction(GuestResource* resource) {
  if (resource == nullptr || !IsFm2Resource(resource))
    return;
  // Invalidate magic immediately so guest re-uses of this address don't look
  // like live FM2 resources while destruction is pending.
  resource->magic = 0;
  RenderCommand cmd{};
  cmd.type = RenderCommandType::DestructResource;
  cmd.destructResource.resource = resource;
  RenderQueue::Enqueue(cmd);
}

// ---------------------------------------------------------------------------
// Clear / Resolve (POD enqueue; Proc* live below with draw helpers).
// ---------------------------------------------------------------------------

void Clear(GuestDevice* /*device*/, uint32_t flags, const float* color, float z) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::Clear;
  cmd.clear.flags = flags;
  cmd.clear.color[0] = color != nullptr ? color[0] : 0.0f;
  cmd.clear.color[1] = color != nullptr ? color[1] : 0.0f;
  cmd.clear.color[2] = color != nullptr ? color[2] : 0.0f;
  cmd.clear.color[3] = color != nullptr ? color[3] : 0.0f;
  cmd.clear.z = z;
  RenderQueue::Enqueue(cmd);
}

void ResolveToTexture(GuestBaseTexture* destTexture, const GuestPoint* destPoint,
                      const GuestRect* sourceRect) {
  // Unleashed StretchRect pattern: link dest to the current RT and defer the
  // copy/MSAA resolve until FlushPendingStretchRectCommands (before Present /
  // draw). Immediate path kept for non-texture destinations or region copies.
  RenderCommand cmd{};
  cmd.type = RenderCommandType::ResolveToTexture;
  cmd.resolveToTexture.destTexture = destTexture;
  cmd.resolveToTexture.destX = destPoint != nullptr ? uint32_t(destPoint->x.get()) : 0;
  cmd.resolveToTexture.destY = destPoint != nullptr ? uint32_t(destPoint->y.get()) : 0;
  cmd.resolveToTexture.hasSrc = sourceRect != nullptr;
  if (sourceRect != nullptr) {
    cmd.resolveToTexture.srcLeft = sourceRect->left.get();
    cmd.resolveToTexture.srcTop = sourceRect->top.get();
    cmd.resolveToTexture.srcRight = sourceRect->right.get();
    cmd.resolveToTexture.srcBottom = sourceRect->bottom.get();
  }
  RenderQueue::Enqueue(cmd);
}

// ---------------------------------------------------------------------------
// Phase 4: draw dispatch + constant transport.
// ---------------------------------------------------------------------------

namespace {

// ---------------------------------------------------------------------------
// Vertex declaration -> plume input layout translation.
//
// FM2's D3DVERTEXELEMENT9.Type is not the classic small D3DDECLTYPE_* integer
// space -- it's the raw Xbox 360 GPUVERTEXFETCHFORMAT dword, and the values in
// guest_device.h's GuestDeclType enum are the exact dwords FM2 actually uses
// (confirmed against the values' encoded k_* format field, not guessed).
// ---------------------------------------------------------------------------

RenderFormat ConvertDeclType(uint32_t type) {
  switch (type) {
    case D3DDECLTYPE_FLOAT1:
      return RenderFormat::R32_FLOAT;
    case D3DDECLTYPE_FLOAT2:
      return RenderFormat::R32G32_FLOAT;
    case D3DDECLTYPE_FLOAT3:
      return RenderFormat::R32G32B32_FLOAT;
    case D3DDECLTYPE_FLOAT4:
      return RenderFormat::R32G32B32A32_FLOAT;
    case D3DDECLTYPE_D3DCOLOR:
      return RenderFormat::B8G8R8A8_UNORM;
    case D3DDECLTYPE_UBYTE4:
    case D3DDECLTYPE_UBYTE4_2:
      return RenderFormat::R8G8B8A8_UINT;
    case D3DDECLTYPE_SHORT2:
      return RenderFormat::R16G16_SINT;
    case D3DDECLTYPE_SHORT4:
      return RenderFormat::R16G16B16A16_SINT;
    case D3DDECLTYPE_UBYTE4N:
    case D3DDECLTYPE_UBYTE4N_2:
      return RenderFormat::R8G8B8A8_UNORM;
    case D3DDECLTYPE_SHORT2N:
      return RenderFormat::R16G16_SNORM;
    case D3DDECLTYPE_SHORT4N:
      return RenderFormat::R16G16B16A16_SNORM;
    case D3DDECLTYPE_USHORT2N:
      return RenderFormat::R16G16_UNORM;
    case D3DDECLTYPE_USHORT4N:
      return RenderFormat::R16G16B16A16_UNORM;
    case D3DDECLTYPE_UINT1:
      return RenderFormat::R32_UINT;
    case D3DDECLTYPE_UDEC3:
    case D3DDECLTYPE_DEC3N:
    case D3DDECLTYPE_DEC3N_2:
    case D3DDECLTYPE_DEC3N_3:
      return RenderFormat::R32_UINT;  // packed 10/10/10/2; the shader bit-unpacks the raw value.
    case D3DDECLTYPE_FLOAT16_2:
      return RenderFormat::R16G16_FLOAT;
    case D3DDECLTYPE_FLOAT16_4:
      return RenderFormat::R16G16B16A16_FLOAT;
    default:
      return RenderFormat::UNKNOWN;
  }
}

// POSITION0 is fetched by the translated shader as a raw bit pattern
// (reinterpreted, never converted) regardless of its declared type -- a
// FLOAT16 position read as a plain bitcast instead of unpacked half-float
// collapses to near-zero instead of the real value.
RenderFormat ConvertPositionDeclType(uint32_t type, bool& outFloat16) {
  outFloat16 = false;
  switch (type) {
    case D3DDECLTYPE_FLOAT1:
      return RenderFormat::R32_UINT;
    case D3DDECLTYPE_FLOAT2:
      return RenderFormat::R32G32_UINT;
    case D3DDECLTYPE_FLOAT3:
      return RenderFormat::R32G32B32_UINT;
    case D3DDECLTYPE_FLOAT4:
      return RenderFormat::R32G32B32A32_UINT;
    case D3DDECLTYPE_FLOAT16_2:
      outFloat16 = true;
      return RenderFormat::R16G16_UINT;
    case D3DDECLTYPE_FLOAT16_4:
      outFloat16 = true;
      return RenderFormat::R16G16B16A16_UINT;
    default:
      return ConvertDeclType(type);
  }
}

const char* ConvertDeclUsage(uint8_t usage) {
  switch (usage) {
    case D3DDECLUSAGE_POSITION:
      return "POSITION";
    case D3DDECLUSAGE_BLENDWEIGHT:
      return "BLENDWEIGHT";
    case D3DDECLUSAGE_BLENDINDICES:
      return "BLENDINDICES";
    case D3DDECLUSAGE_NORMAL:
      return "NORMAL";
    case D3DDECLUSAGE_PSIZE:
      return "PSIZE";
    case D3DDECLUSAGE_TEXCOORD:
      return "TEXCOORD";
    case D3DDECLUSAGE_TANGENT:
      return "TANGENT";
    case D3DDECLUSAGE_BINORMAL:
      return "BINORMAL";
    case D3DDECLUSAGE_TESSFACTOR:
      return "TESSFACTOR";
    case D3DDECLUSAGE_POSITIONT:
      return "POSITIONT";
    case D3DDECLUSAGE_COLOR:
      return "COLOR";
    case D3DDECLUSAGE_FOG:
      return "FOG";
    case D3DDECLUSAGE_DEPTH:
      return "DEPTH";
    case D3DDECLUSAGE_SAMPLE:
      return "SAMPLE";
    default:
      return "TEXCOORD";
  }
}

// Byte size of one GuestDeclType fetch. Used to reject vertex declarations
// whose stream-0 footprint cannot fit the currently bound VB stride.
uint32_t DeclTypeByteSize(uint32_t type) {
  switch (type) {
    case D3DDECLTYPE_FLOAT1:
      return 4;
    case D3DDECLTYPE_FLOAT2:
      return 8;
    case D3DDECLTYPE_FLOAT3:
      return 12;
    case D3DDECLTYPE_FLOAT4:
      return 16;
    case D3DDECLTYPE_D3DCOLOR:
    case D3DDECLTYPE_UBYTE4:
    case D3DDECLTYPE_UBYTE4_2:
    case D3DDECLTYPE_UBYTE4N:
    case D3DDECLTYPE_UBYTE4N_2:
    case D3DDECLTYPE_SHORT2:
    case D3DDECLTYPE_SHORT2N:
    case D3DDECLTYPE_USHORT2N:
    case D3DDECLTYPE_UINT1:
    case D3DDECLTYPE_UDEC3:
    case D3DDECLTYPE_DEC3N:
    case D3DDECLTYPE_DEC3N_2:
    case D3DDECLTYPE_DEC3N_3:
    case D3DDECLTYPE_FLOAT16_2:
      return 4;
    case D3DDECLTYPE_SHORT4:
    case D3DDECLTYPE_SHORT4N:
    case D3DDECLTYPE_USHORT4N:
    case D3DDECLTYPE_FLOAT16_4:
      return 8;
    default:
      break;
  }
  // Fall back through ConvertDeclType for any Xbox fetch dword we recognize
  // by format but forgot to list above.
  switch (ConvertDeclType(type)) {
    case RenderFormat::R32_FLOAT:
    case RenderFormat::R32_UINT:
    case RenderFormat::R8G8B8A8_UNORM:
    case RenderFormat::R8G8B8A8_UINT:
    case RenderFormat::B8G8R8A8_UNORM:
    case RenderFormat::R16G16_FLOAT:
    case RenderFormat::R16G16_UINT:
    case RenderFormat::R16G16_SINT:
    case RenderFormat::R16G16_UNORM:
    case RenderFormat::R16G16_SNORM:
      return 4;
    case RenderFormat::R32G32_FLOAT:
    case RenderFormat::R32G32_UINT:
    case RenderFormat::R16G16B16A16_FLOAT:
    case RenderFormat::R16G16B16A16_UINT:
    case RenderFormat::R16G16B16A16_SINT:
    case RenderFormat::R16G16B16A16_UNORM:
    case RenderFormat::R16G16B16A16_SNORM:
      return 8;
    case RenderFormat::R32G32B32_FLOAT:
    case RenderFormat::R32G32B32_UINT:
      return 12;
    case RenderFormat::R32G32B32A32_FLOAT:
    case RenderFormat::R32G32B32A32_UINT:
      return 16;
    default:
      return 0;
  }
}

// True when every stream-0 element of decl fits inside streamStride bytes.
bool DeclarationFitsStreamStride(const GuestVertexDeclaration* decl, uint32_t streamStride) {
  if (decl == nullptr || decl->vertexElements == nullptr)
    return false;
  // Unknown stride must not vacuously accept every layout (fm2mmgrok10).
  if (streamStride == 0)
    return false;
  for (uint32_t i = 0; i < decl->vertexElementCount; ++i) {
    const GuestVertexElement& e = decl->vertexElements[i];
    if (e.stream != 0)
      continue;
    // Offset alone past the stride is always illegal (even if type size unknown).
    if (uint32_t(e.offset) >= streamStride)
      return false;
    const uint32_t size = DeclTypeByteSize(e.type);
    if (size != 0 && uint32_t(e.offset) + size > streamStride)
      return false;
  }
  return true;
}

uint32_t DeclarationStream0PackedEnd(const GuestVertexDeclaration* decl) {
  if (decl == nullptr || decl->vertexElements == nullptr)
    return 0;
  uint32_t packedEnd = 0;
  for (uint32_t i = 0; i < decl->vertexElementCount; ++i) {
    const GuestVertexElement& e = decl->vertexElements[i];
    if (e.stream != 0)
      continue;
    const uint32_t size = DeclTypeByteSize(e.type);
    if (size == 0)
      continue;
    packedEnd = std::max(packedEnd, uint32_t(e.offset) + size);
  }
  return packedEnd;
}

struct BuiltElement {
  const char* semantic;
  uint8_t usageIndex;
  RenderFormat format;
  uint32_t slot;
  uint32_t offset;
};

// Resolves a declaration's raw D3DVERTEXELEMENT9 array into a plume input
// layout, exactly once per declaration (cached on GuestVertexDeclaration).
// Every standard attribute a shader might reference gets a fallback element
// on an always-unbound slot (15) if this specific declaration doesn't supply
// it, so PSO creation never fails just because one shader wants an attribute
// a *different* shader's declaration happens to omit.
void CompleteVertexDeclaration(GuestVertexDeclaration* decl) {
  if (decl == nullptr || decl->inputElements != nullptr)
    return;

  std::vector<BuiltElement> built;
  built.reserve(size_t(decl->vertexElementCount) + 16);

  for (uint32_t i = 0; i < decl->vertexElementCount; ++i) {
    const GuestVertexElement& e = decl->vertexElements[i];
    if (e.stream == 0xFFu)
      continue;

    RenderFormat format = ConvertDeclType(e.type);
    if (e.usage == D3DDECLUSAGE_POSITION && e.usageIndex == 0) {
      bool isFloat16 = false;
      format = ConvertPositionDeclType(e.type, isFloat16);
      if (isFloat16)
        decl->hasFloat16Position = true;
    } else if (e.usage == D3DDECLUSAGE_POSITION && e.usageIndex == 1) {
      decl->indexVertexStream = e.stream;
    } else if (e.usage == D3DDECLUSAGE_NORMAL || e.usage == D3DDECLUSAGE_TANGENT ||
               e.usage == D3DDECLUSAGE_BINORMAL) {
      if (e.type == D3DDECLTYPE_UBYTE4 || e.type == D3DDECLTYPE_UBYTE4_2) {
        // Already hardware-UNORM-converted to a float4 by the input
        // assembler -- NOT R11G11B10-packed, must not also set that flag.
        format = RenderFormat::R8G8B8A8_UNORM;
        decl->hasUByte4TangentBasis = true;
      } else if (e.type != D3DDECLTYPE_FLOAT3 && e.type != D3DDECLTYPE_FLOAT4) {
        format = RenderFormat::R32_UINT;  // packed DEC3N/UDEC3 family; shader bit-unpacks raw.
        decl->hasR11G11B10Normal = true;
      }
    } else if (e.usage == D3DDECLUSAGE_TEXCOORD) {
      switch (e.type) {
        case D3DDECLTYPE_SHORT2:
        case D3DDECLTYPE_SHORT4:
        case D3DDECLTYPE_SHORT2N:
        case D3DDECLTYPE_SHORT4N:
        case D3DDECLTYPE_USHORT2N:
        case D3DDECLTYPE_USHORT4N:
        case D3DDECLTYPE_FLOAT16_2:
        case D3DDECLTYPE_FLOAT16_4:
          decl->swappedTexcoords |= 1u << e.usageIndex;
          break;
        default:
          break;
      }
    }
    if (format == RenderFormat::UNKNOWN)
      format = RenderFormat::R32_UINT;

    built.push_back({ConvertDeclUsage(e.usage), e.usageIndex, format, e.stream, e.offset});
    if (e.stream < 16)
      decl->vertexStreams[e.stream] = true;
  }

  auto has = [&](const char* semantic, uint8_t usageIndex) {
    for (const BuiltElement& b : built) {
      if (b.usageIndex == usageIndex && std::strcmp(b.semantic, semantic) == 0)
        return true;
    }
    return false;
  };
  auto addDummy = [&](const char* semantic, uint8_t usageIndex, RenderFormat format) {
    if (!has(semantic, usageIndex))
      built.push_back({semantic, usageIndex, format, 15, 0});
  };
  addDummy("POSITION", 0, RenderFormat::R32G32B32A32_UINT);
  addDummy("NORMAL", 0, RenderFormat::R32_UINT);
  addDummy("TANGENT", 0, RenderFormat::R32_UINT);
  addDummy("BINORMAL", 0, RenderFormat::R32_UINT);
  for (uint8_t i = 0; i < 8; ++i)
    addDummy("TEXCOORD", i, RenderFormat::R32_FLOAT);
  addDummy("COLOR", 0, RenderFormat::R32_FLOAT);
  addDummy("COLOR", 1, RenderFormat::R32_FLOAT);
  addDummy("BLENDWEIGHT", 0, RenderFormat::R32_FLOAT);
  addDummy("BLENDINDICES", 0, RenderFormat::R32_UINT);

  decl->inputElementCount = uint32_t(built.size());
  decl->inputElements = std::make_unique<RenderInputElement[]>(built.size());
  for (uint32_t i = 0; i < built.size(); ++i) {
    const BuiltElement& b = built[i];
    decl->inputElements[i] =
        RenderInputElement(b.semantic, b.usageIndex, i, b.format, b.slot, b.offset);
  }
}

// FM2 never binds a vertex declaration through the device field for real
// (SetActivePassId's write there is a texture/shader pass token, not a
// declaration address -- see d3d_hooks.cpp), so the real input layout has to
// be recovered by matching the bound vertex shader's parsed header
// usage/usageIndex set against every declaration FM2 has ever created,
// picking the tightest-fitting exact-count match. Declarations must be a
// superset of what the shader's header lists (order-independent); among
// those, an exact element-count match wins decisively, then a stream-0
// footprint that exactly equals the bound VB stride, with denser packs as a
// tiebreak. Declarations whose stream-0 elements overflow the stride are
// rejected outright.
GuestVertexDeclaration* MatchDeclarationForShader(GuestShader* vs, uint32_t streamStride) {
  if (vs == nullptr || vs->headerElements.empty())
    return nullptr;
  // Unknown stride: refusing to guess prevents locking in a 32B FLOAT3 layout
  // that later draws bind with an 8B VB (fm2mmgrok7/10).
  if (streamStride == 0)
    return nullptr;

  GuestVertexDeclaration* best = nullptr;
  int bestScore = -1;
  for (GuestVertexDeclaration* decl : SnapshotGameDeclarations()) {
    if (decl == nullptr || decl->vertexElements == nullptr || decl->vertexElementCount == 0)
      continue;

    bool covers = true;
    for (const ShaderHeaderElement& he : vs->headerElements) {
      bool found = false;
      for (uint32_t i = 0; i < decl->vertexElementCount && !found; ++i) {
        const GuestVertexElement& e = decl->vertexElements[i];
        found = e.usage == he.usage && e.usageIndex == he.usageIndex;
      }
      if (!found) {
        covers = false;
        break;
      }
    }
    if (!covers)
      continue;
    if (!DeclarationFitsStreamStride(decl, streamStride))
      continue;

    const uint32_t packedEnd = DeclarationStream0PackedEnd(decl);
    int score = 0;
    if (decl->vertexElementCount == uint32_t(vs->headerElements.size()))
      score += 100000;
    if (packedEnd != 0) {
      if (packedEnd == streamStride)
        score += 10000;
      // Prefer denser packs that still fit, but never reward overflow (already
      // rejected above).
      score += int(packedEnd);
    }
    // Stride-8 FM2 meshes pack POSITION as FLOAT16_4. A FLOAT2 POSITION decl
    // also fits 8 bytes and can win the tie, but feeds R32G32_UINT into a
    // shader that f16-unpacks a uint4 -- .zw stay (0,1) and reverse-Z depth
    // collapses (fm2mmgrok8 draw 199).
    for (uint32_t i = 0; i < decl->vertexElementCount; ++i) {
      const GuestVertexElement& e = decl->vertexElements[i];
      if (e.stream != 0 || e.usage != D3DDECLUSAGE_POSITION || e.usageIndex != 0)
        continue;
      if (e.type == D3DDECLTYPE_FLOAT16_4)
        score += 5000;
      else if (e.type == D3DDECLTYPE_FLOAT16_2)
        score += 2500;
      break;
    }
    if (score > bestScore) {
      bestScore = score;
      best = decl;
    }
  }
  return best;
}

// Live VB stride for stream 0. Prefer the input-slot stride (what SetVertexBuffers
// will actually bind). Never take max() with a stale pipeline vertexStrides
// value — that let a leftover 32B stride accept FLOAT3 decls while the bound
// VB stayed at 8 (fm2mmgrok10 draw 200).
uint32_t EffectiveStream0Stride(GuestDevice* device) {
  uint32_t stride = g_inputSlots[0].stride;
  if (stride == 0)
    stride = g_pipelineState.vertexStrides[0];
  if (stride == 0 && device != nullptr) {
    // Xbox stores stride/4 as a byte at device+0x2FD8+stream (080plume).
    const uint8_t dwords = reinterpret_cast<const uint8_t*>(device)[0x2FD8];
    if (dwords != 0)
      stride = uint32_t(dwords) * 4u;
  }
  return stride;
}

GuestVertexDeclaration* ResolveVertexDeclaration(GuestDevice* device) {
  const uint32_t streamStride = EffectiveStream0Stride(device);

  // Keep host stride mirrors coherent when we recovered stride from guest
  // memory (e.g. object-pass replay restored ctx+0x2FD8 but skipped Bind).
  if (streamStride != 0 && g_inputSlots[0].stride == 0) {
    g_inputSlots[0].stride = streamStride;
    g_pipelineState.vertexStrides[0] =
        uint8_t(streamStride > 255u ? 255u : streamStride);
  }

  // Always try shader-header matching first. SetActivePassId mirrors a pass
  // token into device->vertexDeclaration; when that token happens to alias an
  // FM2 GuestVertexDeclaration it short-circuits past the matcher and can lock
  // in a stride-incompatible layout (fm2mmgrok6/7: 32B FLOAT3 decl + 8B VB).
  GuestVertexDeclaration* matched =
      MatchDeclarationForShader(g_pipelineState.vertexShader, streamStride);
  if (matched != nullptr)
    return matched;

  const uint32_t declAddr = device != nullptr ? device->vertexDeclaration.get() : 0;
  if (declAddr != 0) {
    auto* decl = ghp::ToHost<GuestVertexDeclaration>(declAddr);
    if (IsFm2Resource(decl) && decl->type == ResourceType::VertexDeclaration &&
        DeclarationFitsStreamStride(decl, streamStride)) {
      return decl;
    }
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// PSO cache.
// ---------------------------------------------------------------------------

std::unordered_map<uint64_t, std::unique_ptr<RenderPipeline>> g_pipelines;

// PSO creation can fail for reasons that will never change on retry (a
// permanently-mismatched/uncompilable shader in the cache) -- without this,
// GetPipeline() would re-run the full DXC compile+link path from scratch on
// every single draw call using that state, since a failed attempt leaves
// g_pipelines[hash] null indistinguishable from "never tried".
std::unordered_set<uint64_t> g_failedPipelines;

// Canonicalizes logically-equivalent states onto the same cache key: state
// that can't affect the result when its owning enable bit is off must be
// zeroed first, or two draws that only differ in "don't-care" bits would
// wastefully build (and leak descriptor-table-consuming) separate PSOs.
void SanitizePipelineState(PipelineState& ps) {
  if (!ps.zEnable) {
    ps.zFunc = RenderComparisonFunction::ALWAYS;
    ps.depthBias = 0;
    ps.slopeScaledDepthBias = 0.0f;
  }
  if (!ps.stencilEnable) {
    ps.stencilFrontFunc = ps.stencilBackFunc = RenderComparisonFunction::ALWAYS;
    ps.stencilFrontFail = ps.stencilFrontDepthFail = ps.stencilFrontPass = RenderStencilOp::KEEP;
    ps.stencilBackFail = ps.stencilBackDepthFail = ps.stencilBackPass = RenderStencilOp::KEEP;
    ps.stencilReadMask = ps.stencilWriteMask = 0xFF;
    ps.stencilRef = 0;
  }
  if (!ps.zEnable && !ps.stencilEnable)
    ps.depthStencilFormat = RenderFormat::UNKNOWN;
  if (!ps.alphaBlendEnable) {
    ps.srcBlend = RenderBlend::ONE;
    ps.destBlend = RenderBlend::ZERO;
    ps.blendOp = RenderBlendOperation::ADD;
    ps.srcBlendAlpha = RenderBlend::ONE;
    ps.destBlendAlpha = RenderBlend::ZERO;
    ps.blendOpAlpha = RenderBlendOperation::ADD;
  }
  if (ps.vertexDeclaration != nullptr) {
    for (uint32_t i = 0; i < 16; ++i) {
      if (!ps.vertexDeclaration->vertexStreams[i])
        ps.vertexStrides[i] = 0;
    }
  }
  uint32_t usableSpecMask = 0;
  if (ps.vertexShader != nullptr && ps.vertexShader->shaderCacheEntry != nullptr)
    usableSpecMask |= ps.vertexShader->shaderCacheEntry->spec_constants_mask;
  if (ps.pixelShader != nullptr && ps.pixelShader->shaderCacheEntry != nullptr)
    usableSpecMask |= ps.pixelShader->shaderCacheEntry->spec_constants_mask;
  ps.specConstants &= usableSpecMask;
}

// Lazily created, always-resident flat/unlit shader used in place of the
// real pixel shader for draws issued while IsInsideRecordedBatch() -- see
// render_state.h's comment on that flag for why.
RenderShader* GetPlaceholderPixelShader() {
  static std::unique_ptr<RenderShader> shader = Device()->createShader(
      g_placeholder_ps_dxil, sizeof(g_placeholder_ps_dxil), "main", RenderShaderFormat::DXIL);
  return shader.get();
}

// One-time diagnostic logging of *why* a PSO build was rejected -- these
// paths used to fail completely silently, making an all-black screen
// indistinguishable from "every draw's pipeline build is failing."
void LogPipelineRejectOnce(const char* reason) {
  static std::unordered_set<std::string> s_logged;
  if (s_logged.insert(reason).second) {
    REXGPU_WARN("CreateGraphicsPipeline: rejected ({}) -- this draw will be skipped", reason);
  }
}

std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const PipelineState& ps,
                                                       bool placeholderShader) {
  RenderShader* vertexShader = LoadShader(ps.vertexShader, ps.specConstants);
  if (vertexShader == nullptr) {
    if (ps.vertexShader == nullptr) {
      LogPipelineRejectOnce("no vertex shader bound");
    } else {
      static std::unordered_set<uint64_t> s_loggedShaders;
      const uint64_t hash = ps.vertexShader->shaderCacheEntry != nullptr
                                ? ps.vertexShader->shaderCacheEntry->hash
                                : 0;
      if (s_loggedShaders.insert(hash ^ ps.specConstants).second) {
        REXGPU_WARN(
            "CreateGraphicsPipeline: vertex shader failed to load (hash=0x{:016X} cacheEntry={} "
            "specMask={} specConstants={}) -- this draw will be skipped",
            hash, ps.vertexShader->shaderCacheEntry != nullptr,
            ps.vertexShader->shaderCacheEntry != nullptr
                ? ps.vertexShader->shaderCacheEntry->spec_constants_mask
                : 0,
            ps.specConstants);
      }
    }
    return nullptr;
  }

  RenderShader* pixelShader = placeholderShader ? GetPlaceholderPixelShader()
                                                : LoadShader(ps.pixelShader, ps.specConstants);
  if (!placeholderShader && ps.pixelShader != nullptr && pixelShader == nullptr) {
    LogPipelineRejectOnce("pixel shader failed to load");
    return nullptr;
  }

  GuestVertexDeclaration* decl = ps.vertexDeclaration;
  if (decl == nullptr) {
    LogPipelineRejectOnce("no vertex declaration resolved");
    return nullptr;
  }
  CompleteVertexDeclaration(decl);
  if (decl->inputElements == nullptr) {
    LogPipelineRejectOnce("vertex declaration has no input elements");
    return nullptr;
  }

  RenderGraphicsPipelineDesc desc;
  desc.pipelineLayout = PipelineLayout();
  desc.vertexShader = vertexShader;
  desc.pixelShader = pixelShader;
  desc.depthFunction = ps.zFunc;
  desc.depthEnabled = ps.zEnable;
  desc.depthWriteEnabled = ps.zWriteEnable;
  desc.depthBias = ps.depthBias;
  desc.slopeScaledDepthBias = ps.slopeScaledDepthBias;
  desc.depthClipEnabled = ps.depthClipEnabled;
  desc.stencilEnabled = ps.stencilEnable;
  desc.stencilReadMask = ps.stencilReadMask;
  desc.stencilWriteMask = ps.stencilWriteMask;
  desc.stencilReference = ps.stencilRef;
  desc.stencilFrontFace = {ps.stencilFrontPass, ps.stencilFrontFail, ps.stencilFrontDepthFail,
                           ps.stencilFrontFunc};
  desc.stencilBackFace = {ps.stencilBackPass, ps.stencilBackFail, ps.stencilBackDepthFail,
                          ps.stencilBackFunc};
  desc.primitiveTopology = ps.primitiveTopology;
  desc.cullMode = ps.cullMode;
  desc.renderTargetCount = ps.renderTargetFormat != RenderFormat::UNKNOWN ? 1u : 0u;
  desc.renderTargetFormat[0] = ps.renderTargetFormat;
  if (desc.renderTargetCount != 0) {
    RenderBlendDesc& blend = desc.renderTargetBlend[0];
    blend.blendEnabled = ps.alphaBlendEnable;
    blend.srcBlend = ps.srcBlend;
    blend.dstBlend = ps.destBlend;
    blend.blendOp = ps.blendOp;
    blend.srcBlendAlpha = ps.srcBlendAlpha;
    blend.dstBlendAlpha = ps.destBlendAlpha;
    blend.blendOpAlpha = ps.blendOpAlpha;
    blend.renderTargetWriteMask = uint8_t(ps.colorWriteEnable);
  }
  desc.depthTargetFormat = ps.depthStencilFormat;
  desc.multisampling.sampleCount = ps.sampleCount;
  desc.inputElements = decl->inputElements.get();
  desc.inputElementsCount = decl->inputElementCount;

  RenderInputSlot slots[16];
  uint32_t slotCount = 0;
  bool slotSeen[16]{};
  for (uint32_t i = 0; i < decl->inputElementCount; ++i) {
    const uint32_t slot = decl->inputElements[i].slotIndex;
    if (slot >= 16 || slotSeen[slot])
      continue;
    slotSeen[slot] = true;
    slots[slotCount++] = RenderInputSlot(slot, ps.vertexStrides[slot],
                                         RenderInputSlotClassification::PER_VERTEX_DATA);
  }
  desc.inputSlots = slots;
  desc.inputSlotsCount = slotCount;

  RenderSpecConstant specConstant(0, ps.specConstants);
  if (ps.specConstants != 0) {
    desc.specConstants = &specConstant;
    desc.specConstantsCount = 1;
  }

  auto pipelineStart = std::chrono::steady_clock::now();
  auto pipeline = Device()->createGraphicsPipeline(desc);
  auto pipelineMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - pipelineStart)
                        .count();
  REXGPU_INFO("CreateGraphicsPipeline: Device()->createGraphicsPipeline took {} ms", pipelineMs);
  return pipeline;
}

RenderPipeline* GetPipeline(PipelineState ps, bool placeholderShader) {
  SanitizePipelineState(ps);
  if (ps.renderTargetFormat == RenderFormat::UNKNOWN &&
      ps.depthStencilFormat == RenderFormat::UNKNOWN) {
    LogPipelineRejectOnce("no color or depth attachment bound");
    return nullptr;
  }
  if (ps.vertexDeclaration == nullptr) {
    LogPipelineRejectOnce("no vertex declaration resolved (GetPipeline)");
    return nullptr;
  }

  uint64_t hash = XXH3_64bits(&ps, sizeof(ps));
  if (placeholderShader)
    hash ^= 0x9E3779B97F4A7C15ull;
  if (g_failedPipelines.contains(hash)) {
    return nullptr;
  }
  auto& pipeline = g_pipelines[hash];
  if (pipeline == nullptr) {
    pipeline = CreateGraphicsPipeline(ps, placeholderShader);
    if (pipeline == nullptr) {
      g_pipelines.erase(hash);
      g_failedPipelines.insert(hash);
      return nullptr;
    }
    static bool loggedFirstSuccess = false;
    if (!loggedFirstSuccess) {
      loggedFirstSuccess = true;
      REXGPU_INFO("CreateGraphicsPipeline: first PSO built successfully");
    }
  }
  return pipeline.get();
}

// ---------------------------------------------------------------------------
// Constant upload sizes. See UploadAllocator (declared near the other
// per-frame tracking globals above) for the actual upload mechanics.
// ---------------------------------------------------------------------------

constexpr uint32_t kVsFloatConstantBytes = 256 * 16;  // 256 float4 registers.
// ps_3_0's guaranteed minimum (and FM2's actual usable range) is 224 float4
// registers, not 256 -- the guest's storage array reserves 256 defensively,
// but only the first 224 are part of the real constant-buffer ABI.
constexpr uint32_t kPsFloatConstantBytes = 224 * 16;

RenderPrimitiveTopology ConvertPrimitiveType(uint32_t type) {
  // Match SOURCE / Unleashed: unknown Xbox types (e.g. D3DPT_RECTLIST=8) still
  // attempt a triangle-list draw rather than skipping the entire draw (which
  // left UI/compositing black). QUADLIST/FAN use TRIANGLE_LIST + re-index.
  switch (type) {
    case D3DPT_POINTLIST: return RenderPrimitiveTopology::POINT_LIST;
    case D3DPT_LINELIST: return RenderPrimitiveTopology::LINE_LIST;
    case D3DPT_LINESTRIP: return RenderPrimitiveTopology::LINE_STRIP;
    case D3DPT_TRIANGLELIST:
    case D3DPT_QUADLIST:
    case D3DPT_TRIANGLEFAN:
    case D3DPT_RECTLIST:
      return RenderPrimitiveTopology::TRIANGLE_LIST;
    case D3DPT_TRIANGLESTRIP: return RenderPrimitiveTopology::TRIANGLE_STRIP;
    default: return RenderPrimitiveTopology::TRIANGLE_LIST;
  }
}

bool g_insideRecordedBatch = false;
bool g_hasBoundPipeline = false;

// QUADLIST / TRIANGLEFAN → triangle-list index expansion (SOURCE pattern).
template <uint32_t PrimitiveType>
struct PrimitiveIndexData {
  std::vector<uint16_t> indexData;

  uint32_t prepare(uint32_t guestPrimCount) {
    uint32_t primCount;
    uint32_t indexCountPerPrimitive;
    if constexpr (PrimitiveType == D3DPT_TRIANGLEFAN) {
      if (guestPrimCount < 3)
        return 0;
      primCount = guestPrimCount - 2;
      indexCountPerPrimitive = 3;
    } else {
      // QUADLIST: guestPrimCount is vertex count (4 verts per quad).
      primCount = guestPrimCount / 4;
      indexCountPerPrimitive = 6;
    }
    const uint32_t indexCount = primCount * indexCountPerPrimitive;
    if (indexCount == 0)
      return 0;

    if (indexData.size() < indexCount) {
      const size_t oldPrimCount = indexData.size() / indexCountPerPrimitive;
      indexData.resize(indexCount);
      for (size_t i = oldPrimCount; i < primCount; ++i) {
        if constexpr (PrimitiveType == D3DPT_TRIANGLEFAN) {
          indexData[i * 3 + 0] = 0;
          indexData[i * 3 + 1] = uint16_t(i + 1);
          indexData[i * 3 + 2] = uint16_t(i + 2);
        } else {
          indexData[i * 6 + 0] = uint16_t(i * 4 + 0);
          indexData[i * 6 + 1] = uint16_t(i * 4 + 1);
          indexData[i * 6 + 2] = uint16_t(i * 4 + 2);
          indexData[i * 6 + 3] = uint16_t(i * 4 + 0);
          indexData[i * 6 + 4] = uint16_t(i * 4 + 2);
          indexData[i * 6 + 5] = uint16_t(i * 4 + 3);
        }
      }
    }

    RenderBufferReference ref =
        CurrentUploadAllocator().Upload(indexData.data(), indexCount * sizeof(uint16_t), false);
    if (ref.ref == nullptr)
      return 0;
    g_indexBufferView.buffer = ref;
    g_indexBufferView.size = indexCount * sizeof(uint16_t);
    g_indexBufferView.format = RenderFormat::R16_UINT;
    g_dirtyStates.indices = true;
    return indexCount;
  }
};

PrimitiveIndexData<D3DPT_TRIANGLEFAN> g_triangleFanIndexData;
PrimitiveIndexData<D3DPT_QUADLIST> g_quadIndexData;

// Returns non-zero index count when the draw must use generated indices.
uint32_t PrepareConvertedIndices(uint32_t primitiveType, uint32_t vertexOrPrimCount) {
  if (primitiveType == D3DPT_QUADLIST) return g_quadIndexData.prepare(vertexOrPrimCount);
  if (primitiveType == D3DPT_TRIANGLEFAN) return g_triangleFanIndexData.prepare(vertexOrPrimCount);
  return 0;
}

}  // namespace

void SetInsideRecordedBatch(bool inside) { g_insideRecordedBatch = inside; }
bool IsInsideRecordedBatch() { return g_insideRecordedBatch; }

bool HasBoundPipeline() { return g_hasBoundPipeline; }

void FlushRenderState(GuestDevice* device, uint32_t primitiveType) {
  std::lock_guard lock(RecordingMutex());
  g_hasBoundPipeline = false;

  const RenderPrimitiveTopology topology = ConvertPrimitiveType(primitiveType);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.primitiveTopology, topology);

  // Drain deferred Resolve/StretchRect (incl. MSAA) before we bind RTs for
  // drawing -- mirrors Unleashed FlushRenderStateForRenderThread.
  FlushPendingStretchRectCommands();

  if (g_dirtyStates.renderTargetAndDepthStencil || g_framebuffer == nullptr) {
    AddBarrier(g_renderTarget, RenderTextureLayout::COLOR_WRITE);
    AddBarrier(g_depthStencil, RenderTextureLayout::DEPTH_WRITE);
    FlushBarriers();
    // SetFramebuffer may ResizeTileSurface (new CREATE_NOT_ZEROED texture).
    // Discard/init must run AFTER grow, not before.
    SetFramebuffer(g_renderTarget, g_depthStencil, false);
    EnsureAttachmentInitialized(g_renderTarget);
    EnsureAttachmentInitialized(g_depthStencil);
    g_dirtyStates.renderTargetAndDepthStencil = false;
  }
  if (g_dirtyStates.viewport) {
    // Tile surfaces are host-allocated at full frame height; guest still sets
    // a 1280x256 viewport — expand so the single recorded pass fills 720p.
    RenderViewport vp = g_viewport;
    if (g_renderTarget != nullptr && g_renderTarget->type == ResourceType::RenderTarget) {
      auto* surface = static_cast<GuestSurface*>(g_renderTarget);
      if (surface->tileGrownFromHeight != 0 && vp.x == 0.0f && vp.y == 0.0f &&
          uint32_t(vp.width) == g_renderTarget->width &&
          uint32_t(vp.height) == surface->tileGrownFromHeight &&
          g_renderTarget->height > surface->tileGrownFromHeight) {
        vp.height = float(g_renderTarget->height);
      }
    }
    // Xbox reverse-Z viewports arrive as minZ>maxZ. D3D12 requires
    // MinDepth<=MaxDepth; an inverted range depth-clips every sample
    // (RenderDoc: SamplesPassed=0, RT 100% black). Keep the reverse-Z
    // SPEC_CONSTANT from ProcSetViewport; only normalize the host viewport.
    if (vp.minDepth > vp.maxDepth) {
      std::swap(vp.minDepth, vp.maxDepth);
    }
    CommandList()->setViewports(vp);
    g_dirtyStates.viewport = false;
  }
  if (g_dirtyStates.scissorRect) {
    RenderRect scissor = g_scissorTestEnable
                             ? g_scissorRect
                             : RenderRect(0, 0, int32_t(g_viewport.x + g_viewport.width),
                                          int32_t(g_viewport.y + g_viewport.height));
    if (!g_scissorTestEnable && g_renderTarget != nullptr &&
        g_renderTarget->type == ResourceType::RenderTarget) {
      auto* surface = static_cast<GuestSurface*>(g_renderTarget);
      if (surface->tileGrownFromHeight != 0 &&
          scissor.bottom == int32_t(surface->tileGrownFromHeight) &&
          g_renderTarget->height > surface->tileGrownFromHeight) {
        scissor.bottom = int32_t(g_renderTarget->height);
      }
    }
    CommandList()->setScissors(scissor);
    g_dirtyStates.scissorRect = false;
  }

  {
    GuestVertexDeclaration* decl = ResolveVertexDeclaration(device);
    if (decl != nullptr) CompleteVertexDeclaration(decl);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexDeclaration, decl);
  }

  RenderPipeline* pipeline = GetPipeline(g_pipelineState, g_insideRecordedBatch);
  if (pipeline == nullptr) return;
  RenderPipelineLayout* layout = PipelineLayout();
  RenderCommandList* commandList = CommandList();
  if (layout == nullptr || commandList == nullptr) return;
  commandList->setGraphicsPipelineLayout(layout);
  if (TextureDescriptorSet() != nullptr) {
    commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 0);
    commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 1);
    commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 2);
  }
  if (SamplerDescriptorSet() != nullptr) {
    commandList->setGraphicsDescriptorSet(SamplerDescriptorSet(), 3);
  }
  commandList->setPipeline(pipeline);
  g_dirtyStates.pipelineState = false;

  g_sharedConstants.booleans = (device->vertexShaderBoolConstants[0].get() & 0xFFFFu) |
                               ((device->pixelShaderBoolConstants[0].get() & 0xFFFFu) << 16);
  g_sharedConstants.swappedTexcoords =
      g_pipelineState.vertexDeclaration != nullptr ? g_pipelineState.vertexDeclaration->swappedTexcoords : 0;

  // Constants are byte-swapped straight out of guest memory every draw: the
  // guest writes them directly into the real device struct via its own
  // (deliberately unhooked) constant-setter functions, so there is no
  // reliable CPU-side dirty signal to track here.
  CurrentUploadAllocator().UploadAndBindRootDescriptor(device->vertexShaderFloatConstants, kVsFloatConstantBytes, 0,
                                                true);
  CurrentUploadAllocator().UploadAndBindRootDescriptor(device->pixelShaderFloatConstants, kPsFloatConstantBytes, 1,
                                                true);
  CurrentUploadAllocator().UploadAndBindRootDescriptor(&g_sharedConstants, sizeof(g_sharedConstants), 2, false);

  if (g_dirtyStates.vertexStreamFirst <= g_dirtyStates.vertexStreamLast) {
    // [vertexStreamFirst, vertexStreamLast] is only the min/max index *ever*
    // touched since the last flush -- streams can be bound non-contiguously
    // (e.g. slots 0 and 2 but not 1), so this range can include untouched
    // slots. Bind only maximal contiguous runs of actually-bound slots
    // (buffer.ref != nullptr) instead of blindly passing the whole range,
    // which would hand plume a garbage/empty view for any gap slot.
    uint32_t i = g_dirtyStates.vertexStreamFirst;
    const uint32_t last = g_dirtyStates.vertexStreamLast;
    while (i <= last) {
      if (g_vertexBufferViews[i].buffer.ref == nullptr) {
        ++i;
        continue;
      }
      uint32_t runEnd = i;
      while (runEnd < last && g_vertexBufferViews[runEnd + 1].buffer.ref != nullptr)
        ++runEnd;
      CommandList()->setVertexBuffers(i, &g_vertexBufferViews[i], runEnd - i + 1, &g_inputSlots[i]);
      i = runEnd + 1;
    }
    g_dirtyStates.vertexStreamFirst = 15;
    g_dirtyStates.vertexStreamLast = 0;
  }
  if (g_dirtyStates.indices) {
    if (g_indexBufferView.buffer.ref != nullptr)
      CommandList()->setIndexBuffer(&g_indexBufferView);
    g_dirtyStates.indices = false;
  }

  g_hasBoundPipeline = true;
}

void DrawInstanced(uint32_t vertexCount, uint32_t startVertex) {
  // Intended for render-thread callers (nested under Dispatch / Run).
  if (IsDeviceLost())
    return;
  CommandList()->drawInstanced(vertexCount, 1, startVertex, 0);
}

void DrawIndexedInstanced(uint32_t indexCount, uint32_t startIndex, int32_t baseVertexIndex) {
  if (IsDeviceLost())
    return;
  CommandList()->drawIndexedInstanced(indexCount, 1, startIndex, baseVertexIndex, 0);
}

namespace {

void ProcClear(uint32_t flags, const float rgba[4], float z) {
  if (IsDeviceLost())
    return;

  if (g_renderTarget != nullptr && !IsLiveHostTexture(g_renderTarget)) {
    g_renderTarget = nullptr;
  }
  if (g_depthStencil != nullptr && !IsLiveHostTexture(g_depthStencil)) {
    g_depthStencil = nullptr;
  }

  AddBarrier(g_renderTarget, RenderTextureLayout::COLOR_WRITE);
  AddBarrier(g_depthStencil, RenderTextureLayout::DEPTH_WRITE);
  FlushBarriers();
  EnsureAttachmentInitialized(g_renderTarget);
  EnsureAttachmentInitialized(g_depthStencil);

  const bool onePass = (g_renderTarget == nullptr) || (g_depthStencil == nullptr) ||
                       (g_renderTarget->width == g_depthStencil->width &&
                        g_renderTarget->height == g_depthStencil->height);
  if (onePass)
    SetFramebuffer(g_renderTarget, g_depthStencil, true);

  RenderRect clearRect(int32_t(g_viewport.x), int32_t(g_viewport.y),
                       int32_t(g_viewport.x + g_viewport.width),
                       int32_t(g_viewport.y + g_viewport.height));
  if (g_scissorTestEnable) {
    clearRect.left = std::max(clearRect.left, g_scissorRect.left);
    clearRect.top = std::max(clearRect.top, g_scissorRect.top);
    clearRect.right = std::min(clearRect.right, g_scissorRect.right);
    clearRect.bottom = std::min(clearRect.bottom, g_scissorRect.bottom);
  }

  RenderCommandList* commandList = CommandList();
  if (g_renderTarget != nullptr && g_renderTarget->texture != nullptr &&
      (flags & D3DCLEAR_TARGET) != 0) {
    if (!onePass)
      SetFramebuffer(g_renderTarget, nullptr, true);
    if (g_framebuffer != nullptr) {
      commandList->clearColor(0, RenderColor(rgba[0], rgba[1], rgba[2], rgba[3]), &clearRect, 1);
      MarkAttachmentInitialized(g_renderTarget);
    }
  }
  const bool clearDepth = (flags & D3DCLEAR_ZBUFFER) != 0;
  const bool clearStencil = (flags & D3DCLEAR_STENCIL) != 0;
  if (g_depthStencil != nullptr && g_depthStencil->texture != nullptr &&
      (clearDepth || clearStencil)) {
    if (!onePass)
      SetFramebuffer(nullptr, g_depthStencil, true);
    if (g_framebuffer != nullptr) {
      commandList->clearDepthStencil(clearDepth, clearStencil, z, 0, &clearRect, 1);
      MarkAttachmentInitialized(g_depthStencil);
      g_implicitDepthStencil = g_depthStencil;
    }
  }
}

void ProcResolveToTexture(GuestBaseTexture* destTexture, uint32_t destX, uint32_t destY,
                          bool hasSrc, const RenderRect& srcRect) {
  if (IsDeviceLost())
    return;
  if (!IsLiveHostTexture(destTexture))
    return;
  // Swap/Resolve often run after the guest unbound the color RT. Unleashed keeps
  // using the live surface via pending StretchRect links; we fall back to the
  // last full-frame presentable RT so aperture resolves are not no-ops.
  GuestBaseTexture* source = g_renderTarget;
  if (!IsLiveHostTexture(source)) {
    source = g_lastPresentableRenderTarget;
  }
  if (!IsLiveHostTexture(source) || source == destTexture) {
    static uint64_t resolveNoSrc = 0;
    if (++resolveNoSrc <= 24) {
      REXGPU_WARN("ResolveToTexture: no source RT (dest={}x{} n={})", destTexture->width,
                  destTexture->height, resolveNoSrc);
    }
    return;
  }

  GuestSurface* surface = AsSurface(source);
  auto* destAsTexture = (destTexture->type == ResourceType::Texture ||
                         destTexture->type == ResourceType::VolumeTexture)
                            ? static_cast<GuestTexture*>(destTexture)
                            : nullptr;

  if (surface != nullptr && destAsTexture != nullptr && !hasSrc && destX == 0 && destY == 0) {
    RegisterStretchRect(destAsTexture, surface);
    static uint64_t resolveDeferred = 0;
    if (++resolveDeferred <= 24 || resolveDeferred % 300 == 1) {
      REXGPU_INFO("ResolveToTexture: deferred StretchRect {}x{} -> {}x{} (n={})", surface->width,
                  surface->height, destAsTexture->width, destAsTexture->height, resolveDeferred);
    }
    return;
  }

  // Immediate region copy/resolve: refuse format-mismatched GPU copies (device
  // removal). Prefer presenting the live source RT when the dest is the
  // aperture frontbuffer.
  if (!FormatsCompatibleForGpuCopy(source->format, destTexture->format)) {
    static uint64_t resolveFmtSkip = 0;
    if (++resolveFmtSkip <= 24 || resolveFmtSkip % 300 == 1) {
      REXGPU_WARN("ResolveToTexture: format mismatch {}x{} fmt={} -> {}x{} fmt={} (n={})",
                  source->width, source->height, int(source->format), destTexture->width,
                  destTexture->height, int(destTexture->format), resolveFmtSkip);
    }
    if (surface == nullptr || destAsTexture == nullptr ||
        !StretchRectShaderBlit(surface, destAsTexture)) {
      PreferStretchRectSourceForPresent(destTexture, source);
    }
    return;
  }

  // Region / surface-dest path: must not copy while source is still bound.
  if (g_framebuffer != nullptr) {
    CommandList()->setFramebuffer(nullptr);
    g_framebuffer = nullptr;
    g_dirtyStates.renderTargetAndDepthStencil = true;
  }

  AddBarrier(source, RenderTextureLayout::RESOLVE_SOURCE);
  AddBarrier(destTexture, RenderTextureLayout::RESOLVE_DEST);
  FlushBarriers();

  const bool multiSampling =
      surface != nullptr && surface->sampleCount != RenderSampleCount::COUNT_1;
  if (multiSampling) {
    if (hasSrc) {
      CommandList()->resolveTextureRegion(destTexture->texture, destX, destY, source->texture,
                                          &srcRect);
    } else if (destX == 0 && destY == 0) {
      CommandList()->resolveTexture(destTexture->texture, source->texture);
    } else {
      CommandList()->resolveTextureRegion(destTexture->texture, destX, destY, source->texture,
                                          nullptr);
    }
  } else {
    // 1x surfaces: copy, never ResolveSubresourceRegion.
    AddBarrier(source, RenderTextureLayout::COPY_SOURCE);
    AddBarrier(destTexture, RenderTextureLayout::COPY_DEST);
    FlushBarriers();
    RenderBox srcBox;
    const RenderBox* srcBoxPtr = nullptr;
    if (hasSrc) {
      srcBox = RenderBox(srcRect.left, srcRect.top, srcRect.right, srcRect.bottom);
      srcBoxPtr = &srcBox;
    }
    CommandList()->copyTextureRegion(
        RenderTextureCopyLocation::Subresource(destTexture->texture, 0),
        RenderTextureCopyLocation::Subresource(source->texture, 0), destX, destY, 0, srcBoxPtr);
  }
  MarkAttachmentInitialized(destTexture);
  g_stretchRectPresentOverride.store(nullptr, std::memory_order_relaxed);
}

void ProcDrawPrimitive(GuestDevice* device, uint32_t primitiveType, uint32_t startVertex,
                       uint32_t vertexCount) {
  if (IsDeviceLost())
    return;
  g_hasBoundPipeline = false;
  const uint32_t convertedIndexCount = PrepareConvertedIndices(primitiveType, vertexCount);
  FlushRenderState(device, primitiveType);
  if (!g_hasBoundPipeline)
    return;
  if (convertedIndexCount != 0) {
    CommandList()->drawIndexedInstanced(convertedIndexCount, 1, 0, int32_t(startVertex), 0);
  } else {
    CommandList()->drawInstanced(vertexCount, 1, startVertex, 0);
  }
}

void ProcDrawIndexedPrimitive(GuestDevice* device, uint32_t primitiveType, int32_t baseVertexIndex,
                              uint32_t startIndex, uint32_t indexCount) {
  if (IsDeviceLost())
    return;
  FlushRenderState(device, primitiveType);
  if (!g_hasBoundPipeline)
    return;
  CommandList()->drawIndexedInstanced(indexCount, 1, startIndex, baseVertexIndex, 0);
}

void ProcDrawPrimitiveUP(GuestDevice* device, uint32_t primitiveType, uint32_t vertexCount,
                         uint8_t* copy, uint32_t stride, uint32_t bytes) {
  if (IsDeviceLost())
    return;
  g_hasBoundPipeline = false;

  const uint8_t savedStride0 = g_pipelineState.vertexStrides[0];
  g_pipelineState.vertexStrides[0] = uint8_t(stride);
  const uint32_t convertedIndexCount = PrepareConvertedIndices(primitiveType, vertexCount);
  FlushRenderState(device, primitiveType);
  g_pipelineState.vertexStrides[0] = savedStride0;
  if (!g_hasBoundPipeline)
    return;

  RenderBufferReference ref = CurrentUploadAllocator().Upload(copy, bytes, false);
  if (ref.ref == nullptr) {
    g_hasBoundPipeline = false;
    return;
  }
  RenderPipelineLayout* layout = PipelineLayout();
  RenderCommandList* commandList = CommandList();
  RenderPipeline* pipeline = GetPipeline(g_pipelineState, g_insideRecordedBatch);
  if (layout == nullptr || commandList == nullptr || pipeline == nullptr) {
    g_hasBoundPipeline = false;
    return;
  }
  commandList->setGraphicsPipelineLayout(layout);
  commandList->setPipeline(pipeline);
  RenderVertexBufferView view(ref, bytes);
  RenderInputSlot slot(0, stride, RenderInputSlotClassification::PER_VERTEX_DATA);
  commandList->setVertexBuffers(0, &view, 1, &slot);
  if (convertedIndexCount != 0) {
    commandList->drawIndexedInstanced(convertedIndexCount, 1, 0, 0, 0);
  } else {
    commandList->drawInstanced(vertexCount, 1, 0, 0);
  }
  g_dirtyStates.vertexStreamFirst = 0;
}

}  // namespace

void DrawVertices(GuestDevice* device, uint32_t primitiveType, uint32_t startVertex,
                  uint32_t vertexCount) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::DrawPrimitive;
  cmd.drawPrimitive.device = device;
  cmd.drawPrimitive.primitiveType = primitiveType;
  cmd.drawPrimitive.startVertex = startVertex;
  cmd.drawPrimitive.vertexCount = vertexCount;
  RenderQueue::Enqueue(cmd);
}

void DrawIndexedVertices(GuestDevice* device, uint32_t primitiveType, int32_t baseVertexIndex,
                         uint32_t startIndex, uint32_t indexCount) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::DrawIndexedPrimitive;
  cmd.drawIndexedPrimitive.device = device;
  cmd.drawIndexedPrimitive.primitiveType = primitiveType;
  cmd.drawIndexedPrimitive.baseVertexIndex = baseVertexIndex;
  cmd.drawIndexedPrimitive.startIndex = startIndex;
  cmd.drawIndexedPrimitive.indexCount = indexCount;
  RenderQueue::Enqueue(cmd);
}

void DrawUserPointerVertices(GuestDevice* device, uint32_t primitiveType, uint32_t vertexCount,
                             const void* data, uint32_t stride) {
  if (data == nullptr || vertexCount == 0 || stride == 0) return;
  const uint32_t bytes = vertexCount * stride;
  // Copy on the guest thread so Enqueue can return before the guest reuses
  // its stack/heap buffer (Unleashed intermediary upload pattern).
  uint8_t* copy = g_intermediaryUploadAllocator.AllocateCopy(data, bytes);
  if (copy == nullptr) {
    REXGPU_WARN("DrawUserPointerVertices: intermediary upload exhausted ({} bytes)", bytes);
    return;
  }
  RenderCommand cmd{};
  cmd.type = RenderCommandType::DrawPrimitiveUP;
  cmd.drawPrimitiveUP.device = device;
  cmd.drawPrimitiveUP.primitiveType = primitiveType;
  cmd.drawPrimitiveUP.vertexCount = vertexCount;
  cmd.drawPrimitiveUP.vertexData = copy;
  cmd.drawPrimitiveUP.stride = stride;
  cmd.drawPrimitiveUP.bytes = bytes;
  RenderQueue::Enqueue(cmd);
}

void DispatchRenderCommand(const RenderCommand& cmd) {
  // WaitForGPU holds RecordingMutex on the guest thread across Run(); taking
  // it again here on the render thread would deadlock.
  if (cmd.type == RenderCommandType::WaitForGpu) {
    ProcWaitForGpu();
    return;
  }
  // Texture create is Device()-only (no command-list recording). Running it
  // without RecordingMutex avoids deadlocking when Present holds that mutex
  // across DXGI present / fence wait while Resolve→TranslateGuestTexture sync
  // Runs on another guest thread.
  if (cmd.type == RenderCommandType::CreateTranslatedTextureHost) {
    ProcCreateTranslatedTextureHost(
        cmd.createTranslatedTextureHost.texture, cmd.createTranslatedTextureHost.width,
        cmd.createTranslatedTextureHost.height, cmd.createTranslatedTextureHost.format,
        cmd.createTranslatedTextureHost.baseAddress, cmd.createTranslatedTextureHost.createdOut);
    return;
  }

  std::lock_guard lock(RecordingMutex());
  switch (cmd.type) {
    case RenderCommandType::DestructResource:
      ProcDestructResource(cmd.destructResource.resource);
      break;
    case RenderCommandType::SetViewport:
      ProcSetViewport(cmd.setViewport.x, cmd.setViewport.y, cmd.setViewport.width,
                      cmd.setViewport.height, cmd.setViewport.minDepth, cmd.setViewport.maxDepth);
      break;
    case RenderCommandType::SetScissorRect:
      ProcSetScissorRect(cmd.setScissorRect.scissorEnable, cmd.setScissorRect.top,
                         cmd.setScissorRect.left, cmd.setScissorRect.bottom,
                         cmd.setScissorRect.right);
      break;
    case RenderCommandType::SetRenderTarget:
      ProcSetRenderTarget(cmd.setRenderTarget.renderTarget);
      break;
    case RenderCommandType::SetImplicitRenderTarget:
      ProcSetImplicitRenderTarget(cmd.setImplicitRenderTarget.renderTarget);
      break;
    case RenderCommandType::SetDepthStencilSurface:
      ProcSetDepthStencilSurface(cmd.setDepthStencilSurface.depthStencil);
      break;
    case RenderCommandType::SetRenderState:
      ApplyRenderState(cmd.setRenderState.state, cmd.setRenderState.value);
      break;
    case RenderCommandType::SetViewportEnable:
      ProcSetViewportEnable(cmd.setViewportEnable.value);
      break;
    case RenderCommandType::SetDepthState:
      ProcSetDepthState(cmd.setDepthState.zEnable, cmd.setDepthState.zWriteEnable,
                        cmd.setDepthState.cmpFunc);
      break;
    case RenderCommandType::SetStencilState:
      ProcSetStencilState(
          cmd.setStencilState.enable, cmd.setStencilState.twoSided, cmd.setStencilState.frontFunc,
          cmd.setStencilState.frontFail, cmd.setStencilState.frontDepthFail,
          cmd.setStencilState.frontPass, cmd.setStencilState.backFunc, cmd.setStencilState.backFail,
          cmd.setStencilState.backDepthFail, cmd.setStencilState.backPass,
          cmd.setStencilState.readMask, cmd.setStencilState.writeMask, cmd.setStencilState.ref);
      break;
    case RenderCommandType::SetTexture:
      ProcSetTexture(cmd.setTexture.index, cmd.setTexture.texture);
      break;
    case RenderCommandType::SetTextureBase:
      ProcSetTextureBase(cmd.setTextureBase.index, cmd.setTextureBase.texture);
      break;
    case RenderCommandType::SetVertexShader:
      ProcSetVertexShader(cmd.setVertexShader.shader);
      break;
    case RenderCommandType::SetPixelShader:
      ProcSetPixelShader(cmd.setPixelShader.shader);
      break;
    case RenderCommandType::SetVertexDeclaration:
      ProcSetVertexDeclaration(cmd.setVertexDeclaration.declaration);
      break;
    case RenderCommandType::SetStreamSource:
      ProcSetStreamSource(cmd.setStreamSource.index, cmd.setStreamSource.buffer,
                          cmd.setStreamSource.offset, cmd.setStreamSource.stride);
      break;
    case RenderCommandType::SetIndices:
      ProcSetIndices(cmd.setIndices.buffer);
      break;
    case RenderCommandType::Clear:
      ProcClear(cmd.clear.flags, cmd.clear.color, cmd.clear.z);
      break;
    case RenderCommandType::ResolveToTexture: {
      RenderRect srcRect(cmd.resolveToTexture.srcLeft, cmd.resolveToTexture.srcTop,
                         cmd.resolveToTexture.srcRight, cmd.resolveToTexture.srcBottom);
      ProcResolveToTexture(cmd.resolveToTexture.destTexture, cmd.resolveToTexture.destX,
                           cmd.resolveToTexture.destY, cmd.resolveToTexture.hasSrc, srcRect);
      break;
    }
    case RenderCommandType::DrawPrimitive:
      ProcDrawPrimitive(cmd.drawPrimitive.device, cmd.drawPrimitive.primitiveType,
                        cmd.drawPrimitive.startVertex, cmd.drawPrimitive.vertexCount);
      break;
    case RenderCommandType::DrawIndexedPrimitive:
      ProcDrawIndexedPrimitive(
          cmd.drawIndexedPrimitive.device, cmd.drawIndexedPrimitive.primitiveType,
          cmd.drawIndexedPrimitive.baseVertexIndex, cmd.drawIndexedPrimitive.startIndex,
          cmd.drawIndexedPrimitive.indexCount);
      break;
    case RenderCommandType::DrawPrimitiveUP:
      ProcDrawPrimitiveUP(cmd.drawPrimitiveUP.device, cmd.drawPrimitiveUP.primitiveType,
                          cmd.drawPrimitiveUP.vertexCount, cmd.drawPrimitiveUP.vertexData,
                          cmd.drawPrimitiveUP.stride, cmd.drawPrimitiveUP.bytes);
      break;
    case RenderCommandType::ExecuteCommandList:
      ProcExecuteCommandList();
      break;
    case RenderCommandType::BeginCommandList:
      ProcBeginCommandList();
      break;
    case RenderCommandType::WaitForGpu:
      break;  // handled above
    case RenderCommandType::BeginRenderStateFrame:
      ProcBeginRenderStateFrame();
      break;
    case RenderCommandType::CreateTextureHost:
      ProcCreateTextureHost(cmd.createTextureHost.texture, cmd.createTextureHost.width,
                            cmd.createTextureHost.height, cmd.createTextureHost.depth,
                            cmd.createTextureHost.levels, cmd.createTextureHost.usage,
                            cmd.createTextureHost.format, cmd.createTextureHost.volume);
      break;
    case RenderCommandType::CreateSurfaceHost:
      ProcCreateSurfaceHost(cmd.createSurfaceHost.surface, cmd.createSurfaceHost.width,
                            cmd.createSurfaceHost.height, cmd.createSurfaceHost.format,
                            cmd.createSurfaceHost.sampleCount, cmd.createSurfaceHost.depth);
      break;
    case RenderCommandType::UnlockTextureRect:
      ProcUnlockTextureRect(cmd.unlockTextureRect.texture);
      break;
    case RenderCommandType::UnlockBuffer16:
      ProcUnlockBuffer16(cmd.unlockBuffer.buffer);
      break;
    case RenderCommandType::UnlockBuffer32:
      ProcUnlockBuffer32(cmd.unlockBuffer.buffer);
      break;
    case RenderCommandType::CopyBufferFromUpload:
      ProcCopyBufferFromUpload(cmd.copyBufferFromUpload.dst, cmd.copyBufferFromUpload.src,
                               cmd.copyBufferFromUpload.size);
      break;
    case RenderCommandType::CopyTextureFromUpload:
      ProcCopyTextureFromUpload(cmd.copyTextureFromUpload.dst, cmd.copyTextureFromUpload.src,
                                cmd.copyTextureFromUpload.format, cmd.copyTextureFromUpload.width,
                                cmd.copyTextureFromUpload.height,
                                cmd.copyTextureFromUpload.rowTexels, cmd.copyTextureFromUpload.mip,
                                cmd.copyTextureFromUpload.srcOffset);
      break;
    case RenderCommandType::CreateTranslatedTextureHost:
      break;  // handled above
  }
}

}  // namespace fm2::render
