// render/render_state.cpp
//
// Native D3D state, draw, and resource translation for FM2. The old diagnostic
// object-pass replay, shader probing, EDRAM heuristics, and hardcoded trace
// output are deliberately excluded; guest D3D state owns draw submission.

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <chrono>
#include <cmath>
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

namespace fm4::render {

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
  bool separateAlphaBlendEnable = false;
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
  bool twoSidedStencilMode = false;
  uint8_t stencilFrontReadMask = 0xFF, stencilFrontWriteMask = 0xFF, stencilFrontRef = 0;
  uint8_t stencilBackReadMask = 0xFF, stencilBackWriteMask = 0xFF, stencilBackRef = 0;
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
  uint32_t samplerIndices[16]{};  // Bindless indices from the texture fetch constants each draw.
  uint32_t booleans[8]{};         // VS words 0-3, PS words 4-7.
  uint32_t swappedTexcoords = 0;
  uint32_t swappedNormals = 0;
  uint32_t swappedBinormals = 0;
  uint32_t swappedTangents = 0;
  uint32_t swappedBlendWeights = 0;
  float halfPixelOffsetX = 0.0f;
  float halfPixelOffsetY = 0.0f;
  uint32_t halfPixelPadding = 0;
  float clipPlane[4]{};
  uint32_t clipPlaneEnabled = 0;
  float alphaThreshold = 0.0f;
  uint32_t conditionalSurveyIndex = 0;
  uint32_t conditionalRenderingIndex = 0;
  uint32_t vteFlags = 8;
  uint32_t loopConstants[32]{};  // VS 0-15, PS 16-31; Xenos packed count/start/step.
  uint32_t trailingPadding[3]{};
};
static_assert(sizeof(SharedConstants) == 496);
static_assert(offsetof(SharedConstants, texture2DIndices) == 0);
static_assert(offsetof(SharedConstants, texture3DIndices) == 64);
static_assert(offsetof(SharedConstants, textureCubeIndices) == 128);
static_assert(offsetof(SharedConstants, samplerIndices) == 192);
static_assert(offsetof(SharedConstants, booleans) == 256);
static_assert(offsetof(SharedConstants, swappedTexcoords) == 288);
static_assert(offsetof(SharedConstants, halfPixelOffsetX) == 308);
static_assert(offsetof(SharedConstants, halfPixelPadding) == 316);
static_assert(offsetof(SharedConstants, clipPlane) == 320);
static_assert(offsetof(SharedConstants, clipPlaneEnabled) == 336);
static_assert(offsetof(SharedConstants, vteFlags) == 352);
static_assert(offsetof(SharedConstants, loopConstants) == 356);
static_assert(offsetof(SharedConstants, trailingPadding) == 484);

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
// Last full-frame color RT an actual DRAW rendered into (latched at pipeline
// bind in FlushRenderState, not at SetRenderTarget). Unleashed presents its
// single g_backBuffer so it never needs this; FM2 draws the scene into one RT
// and then binds a separate display buffer, so a bind-time latch presents the
// empty display buffer (black). The reference repo learned this twice
// (g_lastTouchedRenderTarget / g_scenePresentRT); this is the minimal port of
// that lesson.
GuestBaseTexture* g_lastDrawnRenderTarget = nullptr;
// StretchRect format-skip present override (survives Swap re-setting aperture).
std::atomic<GuestBaseTexture*> g_stretchRectPresentOverride{nullptr};
GuestSurface* g_depthStencil = nullptr;
GuestSurface* g_implicitDepthStencil = nullptr;
// Last depth surface seen per exact (width, height, sampleCount) — FM2's
// EDRAM model never re-binds depth when switching passes (720p scene ↔ 512
// envmap), so the stale wrong-size depth must be swapped for the matching one
// at draw time (see FlushRenderState rescue). Both directions occur.
std::unordered_map<uint64_t, GuestSurface*> g_lastDepthBySize;

uint64_t DepthSizeKey(uint32_t width, uint32_t height, RenderSampleCounts samples) {
  return (uint64_t(width) << 32) | (uint64_t(height) << 8) | uint64_t(samples);
}
RenderFramebuffer* g_framebuffer = nullptr;
std::unordered_map<uint64_t, std::unique_ptr<RenderFramebuffer>> g_framebufferCache;
std::unordered_map<uint64_t, std::pair<uint32_t, std::unique_ptr<RenderSampler>>> g_samplerStates;
RenderViewport g_viewport{0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f};
bool g_viewportEnabled = true;
PipelineState g_pipelineState;
GuestVertexDeclaration* g_boundVertexDeclaration = nullptr;
SharedConstants g_sharedConstants;
bool g_sharedConstantsInitialized = false;
GuestTexture* g_textures[16]{};
alignas(16) uint32_t g_vertexShaderConstants[0x400]{};
alignas(16) uint32_t g_pixelShaderConstants[0x380]{};
bool g_scissorTestEnable = false;
RenderRect g_scissorRect;
RenderVertexBufferView g_vertexBufferViews[16];
RenderInputSlot g_inputSlots[16];
RenderIndexBufferView g_indexBufferView{RenderBufferReference{}, 0, RenderFormat::R16_UINT};
DirtyStates g_dirtyStates(true);
uint64_t g_frameIndex = 0;

struct FrameTraceStats {
  uint32_t attemptedDraws = 0;
  uint32_t issuedDraws = 0;
  uint32_t clears = 0;
  uint32_t resolves = 0;
  uint64_t shapeHash = 0xCBF29CE484222325ull;
  uint64_t textureHash = 0x9E3779B97F4A7C15ull;
  uint64_t sharedHash = 0x9E3779B97F4A7C15ull;
  uint64_t vertexConstantHash = 0x9E3779B97F4A7C15ull;
  uint64_t pixelConstantHash = 0x9E3779B97F4A7C15ull;
  uint64_t vertexDataHash = 0x9E3779B97F4A7C15ull;
  uint64_t clearHash = 0x9E3779B97F4A7C15ull;
};

FrameTraceStats g_frameTrace;
uint64_t g_frameTraceIndex = 0;

struct DrawDataTrace {
  std::array<uint32_t, sizeof(SharedConstants) / sizeof(uint32_t)> shared{};
  std::array<uint32_t, std::size(g_vertexShaderConstants)> vertexConstants{};
  std::array<uint32_t, std::size(g_pixelShaderConstants)> pixelConstants{};
};

struct TraceDifference {
  uint32_t draw = UINT32_MAX;
  uint32_t dword = UINT32_MAX;
  uint32_t before = 0;
  uint32_t after = 0;
};

std::vector<DrawDataTrace> g_currentDrawDataTrace;
std::vector<DrawDataTrace> g_previousDrawDataTrace;
uint64_t g_previousFrameShapeHash = 0;

void MixFrameTrace(uint64_t& hash, uint64_t value) {
  hash ^= value + 0x9E3779B97F4A7C15ull + (hash << 6) + (hash >> 2);
}

void MixFrameTraceBytes(uint64_t& hash, const void* data, size_t size) {
  MixFrameTrace(hash, XXH3_64bits(data, size));
}

uint64_t ShaderTraceId(const GuestShader* shader) {
  return shader != nullptr && shader->shaderCacheEntry != nullptr ? shader->shaderCacheEntry->hash
                                                                  : 0;
}

uint64_t VertexDeclarationTraceId(const GuestVertexDeclaration* declaration) {
  if (declaration == nullptr)
    return 0;
  if (declaration->hash != 0)
    return declaration->hash;
  return declaration->vertexElements != nullptr
             ? XXH3_64bits(declaration->vertexElements.get(),
                           declaration->vertexElementCount * sizeof(GuestVertexElement))
             : 0;
}

void MixTextureShape(uint64_t& hash, const GuestBaseTexture* texture) {
  if (texture == nullptr) {
    MixFrameTrace(hash, 0);
    return;
  }
  MixFrameTrace(hash, uint32_t(texture->type));
  MixFrameTrace(hash, uint64_t(texture->width) << 32 | texture->height);
  MixFrameTrace(hash, uint32_t(texture->format));
  if (texture->type == ResourceType::RenderTarget || texture->type == ResourceType::DepthStencil) {
    MixFrameTrace(hash, uint32_t(static_cast<const GuestSurface*>(texture)->sampleCount));
  }
}

template <size_t N>
TraceDifference FindFirstTraceDifference(const std::vector<DrawDataTrace>& before,
                                         const std::vector<DrawDataTrace>& after,
                                         const std::array<uint32_t, N> DrawDataTrace::* member) {
  for (uint32_t draw = 0; draw < before.size(); ++draw) {
    const auto& oldValues = before[draw].*member;
    const auto& newValues = after[draw].*member;
    for (uint32_t dword = 0; dword < N; ++dword) {
      if (oldValues[dword] != newValues[dword]) {
        return {draw, dword, oldValues[dword], newValues[dword]};
      }
    }
  }
  return {};
}

void CaptureDrawDataTrace() {
  if (g_frameTraceIndex >= 64)
    return;
  DrawDataTrace& trace = g_currentDrawDataTrace.emplace_back();
  std::memcpy(trace.shared.data(), &g_sharedConstants, sizeof(g_sharedConstants));
  std::copy(std::begin(g_vertexShaderConstants), std::end(g_vertexShaderConstants),
            trace.vertexConstants.begin());
  std::copy(std::begin(g_pixelShaderConstants), std::end(g_pixelShaderConstants),
            trace.pixelConstants.begin());
}

void TraceIssuedDraw(uint32_t kind, const uint32_t* geometry, size_t geometryCount,
                     const void* vertexData = nullptr, size_t vertexDataSize = 0) {
  ++g_frameTrace.issuedDraws;
  MixFrameTrace(g_frameTrace.shapeHash, kind);
  PipelineState semanticPipeline = g_pipelineState;
  semanticPipeline.vertexShader = nullptr;
  semanticPipeline.pixelShader = nullptr;
  semanticPipeline.vertexDeclaration = nullptr;
  MixFrameTrace(g_frameTrace.shapeHash, ShaderTraceId(g_pipelineState.vertexShader));
  MixFrameTrace(g_frameTrace.shapeHash, ShaderTraceId(g_pipelineState.pixelShader));
  MixFrameTrace(g_frameTrace.shapeHash,
                VertexDeclarationTraceId(g_pipelineState.vertexDeclaration));
  MixFrameTraceBytes(g_frameTrace.shapeHash, &semanticPipeline, sizeof(semanticPipeline));
  MixFrameTraceBytes(g_frameTrace.shapeHash, geometry, geometryCount * sizeof(*geometry));
  MixFrameTraceBytes(g_frameTrace.shapeHash, &g_viewport, sizeof(g_viewport));
  MixFrameTrace(g_frameTrace.shapeHash, g_scissorTestEnable);
  MixFrameTraceBytes(g_frameTrace.shapeHash, &g_scissorRect, sizeof(g_scissorRect));
  MixFrameTraceBytes(g_frameTrace.textureHash, g_textures, sizeof(g_textures));
  MixFrameTraceBytes(g_frameTrace.sharedHash, &g_sharedConstants, sizeof(g_sharedConstants));
  MixFrameTraceBytes(g_frameTrace.vertexConstantHash, g_vertexShaderConstants,
                     sizeof(g_vertexShaderConstants));
  MixFrameTraceBytes(g_frameTrace.pixelConstantHash, g_pixelShaderConstants,
                     sizeof(g_pixelShaderConstants));
  if (vertexData != nullptr && vertexDataSize != 0) {
    MixFrameTraceBytes(g_frameTrace.vertexDataHash, vertexData, vertexDataSize);
  }
  CaptureDrawDataTrace();
}

void LogAndResetFrameTrace() {
  ++g_frameTraceIndex;
  if (g_frameTraceIndex <= 64 || g_frameTraceIndex % 300 == 1) {
    REXGPU_INFO(
        "FrameTrace: n={} draws={}/{} skipped={} clears={} resolves={} shape=0x{:016X} "
        "tex=0x{:016X} shared=0x{:016X} vs=0x{:016X} ps=0x{:016X} "
        "vertex=0x{:016X} clear=0x{:016X}",
        g_frameTraceIndex, g_frameTrace.issuedDraws, g_frameTrace.attemptedDraws,
        g_frameTrace.attemptedDraws - g_frameTrace.issuedDraws, g_frameTrace.clears,
        g_frameTrace.resolves, g_frameTrace.shapeHash, g_frameTrace.textureHash,
        g_frameTrace.sharedHash, g_frameTrace.vertexConstantHash, g_frameTrace.pixelConstantHash,
        g_frameTrace.vertexDataHash, g_frameTrace.clearHash);
  }
  if (g_frameTraceIndex <= 64 && g_previousFrameShapeHash == g_frameTrace.shapeHash &&
      !g_currentDrawDataTrace.empty() &&
      g_previousDrawDataTrace.size() == g_currentDrawDataTrace.size()) {
    const TraceDifference shared = FindFirstTraceDifference(
        g_previousDrawDataTrace, g_currentDrawDataTrace, &DrawDataTrace::shared);
    const TraceDifference vs = FindFirstTraceDifference(
        g_previousDrawDataTrace, g_currentDrawDataTrace, &DrawDataTrace::vertexConstants);
    const TraceDifference ps = FindFirstTraceDifference(
        g_previousDrawDataTrace, g_currentDrawDataTrace, &DrawDataTrace::pixelConstants);
    if (shared.draw != UINT32_MAX) {
      REXGPU_INFO("FrameDelta: n={} shared draw={} dword={} 0x{:08X}->0x{:08X}", g_frameTraceIndex,
                  shared.draw, shared.dword, shared.before, shared.after);
    }
    if (vs.draw != UINT32_MAX) {
      REXGPU_INFO("FrameDelta: n={} vs draw={} c{}.{} 0x{:08X}->0x{:08X}", g_frameTraceIndex,
                  vs.draw, vs.dword / 4, vs.dword % 4, vs.before, vs.after);
    }
    if (ps.draw != UINT32_MAX) {
      REXGPU_INFO("FrameDelta: n={} ps draw={} c{}.{} 0x{:08X}->0x{:08X}", g_frameTraceIndex,
                  ps.draw, ps.dword / 4, ps.dword % 4, ps.before, ps.after);
    }
  }
  g_previousFrameShapeHash = g_frameTrace.shapeHash;
  g_previousDrawDataTrace.swap(g_currentDrawDataTrace);
  g_currentDrawDataTrace.clear();
  g_frameTrace = {};
}

// Unleashed-style deferred StretchRect / Resolve: surfaces accumulate
// destination textures, drained by FlushPendingStretchRectCommands before
// Present and before draws that need a fresh sample of the resolved content.
std::unordered_set<GuestSurface*> g_pendingSurfaceCopies;
std::unordered_set<GuestSurface*> g_pendingMsaaResolves;

// ---------------------------------------------------------------------------
// Constant/vertex/buffer upload allocator (Phase 4). Chunks are owned per GPU
// frame slot and reset only after that slot's fence retires
// (OnRecordingFrameReady), so 2-frame pipelining cannot overwrite in-flight
// CBVs, geometry, or buffer copies.
// ---------------------------------------------------------------------------

class UploadAllocator {
 public:
  void Reset() {
    currentChunk_ = 0;
    for (Chunk& chunk : chunks_)
      chunk.offset = 0;
  }

  // Copies `size` bytes from `src` into the next aligned region of the
  // frame's upload buffer, returning a reference usable as a root CBV or a
  // vertex/index buffer view. If `byteSwap`, swaps complete big-endian dwords
  // into the buffer and preserves any trailing bytes; otherwise does a plain
  // copy for host-native data.
  // Grows in reusable chunks so large menu/resource upload bursts do not
  // create and retain one D3D12 upload resource for every guest Unlock.
  RenderBufferReference Upload(const void* src, uint64_t size, bool byteSwap) {
    if (src == nullptr || size == 0)
      return RenderBufferReference{};
    Chunk* chunk = AcquireChunk(size);
    if (chunk == nullptr)
      return RenderBufferReference{};

    chunk->offset = Align(chunk->offset);

    uint8_t* dst = chunk->mapped + chunk->offset;
    if (byteSwap) {
      const uint32_t* s = reinterpret_cast<const uint32_t*>(src);
      uint32_t* d = reinterpret_cast<uint32_t*>(dst);
      for (uint64_t i = 0; i < size / sizeof(uint32_t); ++i)
        d[i] = std::byteswap(s[i]);
      const uint64_t swappedBytes = size & ~uint64_t{3};
      if (swappedBytes != size) {
        std::memcpy(dst + swappedBytes, static_cast<const uint8_t*>(src) + swappedBytes,
                    size - swappedBytes);
      }
    } else {
      std::memcpy(dst, src, size);
    }
    RenderBufferReference ref = chunk->buffer->at(chunk->offset);
    chunk->offset += size;
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
  struct Chunk {
    std::unique_ptr<RenderBuffer> buffer;
    uint8_t* mapped = nullptr;
    uint64_t capacity = 0;
    uint64_t offset = 0;
  };

  // A menu transition uploads many 2.2 MiB and 568 KiB buffers in one burst.
  // A 64 MiB base chunk keeps that burst to one or two persistent resources
  // per frame rather than dozens of short-lived committed resources.
  static constexpr uint64_t kChunkSize = 64 * 1024 * 1024;
  static constexpr uint64_t kAlignment = 256;

  static uint64_t Align(uint64_t value) { return (value + kAlignment - 1) & ~(kAlignment - 1); }

  Chunk* AcquireChunk(uint64_t size) {
    for (size_t i = currentChunk_; i < chunks_.size(); ++i) {
      Chunk& chunk = chunks_[i];
      if (Align(chunk.offset) + size <= chunk.capacity) {
        currentChunk_ = i;
        return &chunk;
      }
    }

    const uint64_t capacity = std::max(kChunkSize, Align(size));
    auto buffer = Device()->createBuffer(RenderBufferDesc::UploadBuffer(
        capacity, RenderBufferFlag::CONSTANT | RenderBufferFlag::VERTEX | RenderBufferFlag::INDEX));
    if (buffer == nullptr) {
      REXGPU_ERROR("UploadAllocator: failed to create {}-byte upload chunk", capacity);
      return nullptr;
    }
    uint8_t* mapped = reinterpret_cast<uint8_t*>(buffer->map());
    if (mapped == nullptr) {
      REXGPU_ERROR("UploadAllocator: failed to map {}-byte upload chunk", capacity);
      return nullptr;
    }
    chunks_.push_back(Chunk{std::move(buffer), mapped, capacity, 0});
    currentChunk_ = chunks_.size() - 1;
    return &chunks_.back();
  }

  std::vector<Chunk> chunks_;
  size_t currentChunk_ = 0;
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
    if (size == 0)
      return nullptr;
    constexpr uint64_t kChunkSize = 16 * 1024 * 1024;
    const uint64_t alignedSize = (uint64_t(size) + 0xFu) & ~0xFull;
    if (index_ < chunks_.size() && offset_ + alignedSize > chunks_[index_].capacity) {
      ++index_;
      offset_ = 0;
    }
    if (chunks_.size() <= index_) {
      const uint64_t capacity = std::max(kChunkSize, alignedSize);
      chunks_.push_back(Chunk{std::make_unique<uint8_t[]>(capacity), capacity});
    } else if (chunks_[index_].capacity < alignedSize) {
      // This chunk is not in use in the current generation: Reset or the
      // previous overflow advanced us here only after earlier queued users had
      // drained, so replacing it cannot invalidate live command pointers.
      const uint64_t capacity = std::max(kChunkSize, alignedSize);
      chunks_[index_] = Chunk{std::make_unique<uint8_t[]>(capacity), capacity};
    }
    uint8_t* result = chunks_[index_].memory.get() + offset_;
    offset_ += alignedSize;
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
  struct Chunk {
    std::unique_ptr<uint8_t[]> memory;
    uint64_t capacity = 0;
  };

  std::mutex mutex_;
  std::vector<Chunk> chunks_;
  size_t index_ = 0;
  uint64_t offset_ = 0;
};
IntermediaryUploadAllocator g_intermediaryUploadAllocator;

std::array<std::vector<GuestResource*>, kNumFrames> g_tempResources;

void DestructTempResources(uint32_t frame) {
  auto& resources = g_tempResources[frame % kNumFrames];
  for (GuestResource* resource : resources) {
    if (resource == nullptr || !IsFm4Resource(resource))
      continue;
    switch (resource->type) {
      case ResourceType::Texture:
      case ResourceType::VolumeTexture: {
        auto* texture = static_cast<GuestTexture*>(resource);
        if (g_renderTarget == texture)
          g_renderTarget = nullptr;
        if (g_lastPresentableRenderTarget == texture)
          g_lastPresentableRenderTarget = nullptr;
        if (g_lastDrawnRenderTarget == texture)
          g_lastDrawnRenderTarget = nullptr;
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
        if (g_lastDrawnRenderTarget == surface)
          g_lastDrawnRenderTarget = nullptr;
        if (g_implicitRenderTarget == surface)
          g_implicitRenderTarget = nullptr;
        if (g_depthStencil == surface)
          g_depthStencil = nullptr;
        if (g_implicitDepthStencil == surface)
          g_implicitDepthStencil = nullptr;
        for (auto it = g_lastDepthBySize.begin(); it != g_lastDepthBySize.end();) {
          it = it->second == surface ? g_lastDepthBySize.erase(it) : std::next(it);
        }
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
  return texture != nullptr && IsFm4Resource(texture) && texture->textureHolder != nullptr &&
         texture->texture != nullptr && texture->texture == texture->textureHolder.get();
}

// FM2 EDRAM predicated tiling binds 1280x256 color RTs near frame end. Those
// are intermediates, not the composited frontbuffer — sticky Present must not
// adopt them (would stretch a tile band to full swapchain).
//
// Measure against the guest frame, never Video::s_viewport*: that tracks the
// host swapchain, so on any window larger than 720p every guest RT failed this
// test, g_lastPresentableRenderTarget was never latched, and Resolve dropped
// every frame the guest had already unbound the RT ("no source RT" -> black).
bool IsFramebufferSizedPresentSource(GuestBaseTexture* texture) {
  if (!IsLiveHostTexture(texture))
    return false;
  if (texture->width == kFm4FrameWidth && texture->height < kFm4FrameHeight)
    return false;
  return texture->width >= kFm4FrameWidth && texture->height >= kFm4FrameHeight;
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

bool AttachmentsCompatible(GuestBaseTexture* colorTarget, GuestSurface* depthTarget) {
  if (colorTarget == nullptr || depthTarget == nullptr)
    return true;
  return colorTarget->width == depthTarget->width && colorTarget->height == depthTarget->height &&
         GetSampleCount(colorTarget) == GetSampleCount(depthTarget);
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

bool CanDirectStretchRectCopy(GuestSurface* surface, GuestTexture* texture) {
  return IsLiveHostTexture(surface) && IsLiveHostTexture(texture) &&
         FormatsCompatibleForGpuCopy(surface->format, texture->format) &&
         surface->width == texture->width && surface->height == texture->height;
}

bool ClipResolveCopyRegion(GuestBaseTexture* source, GuestBaseTexture* destination,
                           RenderRect& sourceRect, uint32_t destX, uint32_t destY) {
  if (!IsLiveHostTexture(source) || !IsLiveHostTexture(destination) ||
      destX >= destination->width || destY >= destination->height) {
    return false;
  }

  sourceRect.left = std::clamp(sourceRect.left, 0, int32_t(source->width));
  sourceRect.top = std::clamp(sourceRect.top, 0, int32_t(source->height));
  sourceRect.right = std::clamp(sourceRect.right, sourceRect.left, int32_t(source->width));
  sourceRect.bottom = std::clamp(sourceRect.bottom, sourceRect.top, int32_t(source->height));
  const uint32_t copyWidth =
      std::min(uint32_t(sourceRect.right - sourceRect.left), destination->width - destX);
  const uint32_t copyHeight =
      std::min(uint32_t(sourceRect.bottom - sourceRect.top), destination->height - destY);
  if (copyWidth == 0 || copyHeight == 0)
    return false;
  sourceRect.right = sourceRect.left + int32_t(copyWidth);
  sourceRect.bottom = sourceRect.top + int32_t(copyHeight);
  return true;
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
  // Fail closed on non-RT-capable destinations (e.g. sampled-only translated
  // textures): CreateRenderTargetView on a FLAG_NONE resource removes the
  // device (InfoQueue id=42 -> RemoveDevice INVALID_CALL).
  if (!texture->hostRenderTargetCapable)
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
      if (!CanDirectStretchRectCopy(surface, texture))
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
    if (surface == nullptr || !IsFm4Resource(surface) || surface->destinationTextures.empty()) {
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
      if (!CanDirectStretchRectCopy(surface, texture)) {
        static uint64_t stretchFmtSkip = 0;
        if (++stretchFmtSkip <= 24 || stretchFmtSkip % 300 == 1) {
          REXGPU_WARN("StretchRect: shader path {}x{} fmt={} -> {}x{} fmt={} samples={} (n={})",
                      surface->width, surface->height, int(surface->format), texture->width,
                      texture->height, int(texture->format), surface->sampleCount, stretchFmtSkip);
        }
        // A full-subresource D3D12 copy is invalid when the destination is
        // smaller, and ResolveSubresource cannot scale. Single-sample paths
        // use the existing fullscreen shader blit for both scaling and format
        // conversion; MSAA falls back without submitting an invalid command.
        if (surface->sampleCount != RenderSampleCount::COUNT_1 ||
            !StretchRectShaderBlit(surface, texture)) {
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
    case 0:
      return RenderStencilOp::KEEP;
    case 1:
      return RenderStencilOp::ZERO;
    case 2:
      return RenderStencilOp::REPLACE;
    case 3:
      return RenderStencilOp::INCREMENT_AND_CLAMP;
    case 4:
      return RenderStencilOp::DECREMENT_AND_CLAMP;
    case 5:
      return RenderStencilOp::INVERT;
    case 6:
      return RenderStencilOp::INCREMENT_AND_WRAP;
    case 7:
      return RenderStencilOp::DECREMENT_AND_WRAP;
    default:
      return RenderStencilOp::KEEP;
  }
}

RenderTextureAddressMode ConvertAddressMode(uint32_t v) {
  switch (v) {
    case D3DTADDRESS_WRAP:
      return RenderTextureAddressMode::WRAP;
    case D3DTADDRESS_MIRROR:
      return RenderTextureAddressMode::MIRROR;
    case D3DTADDRESS_CLAMP:
    case 4:
      return RenderTextureAddressMode::CLAMP;
    case D3DTADDRESS_MIRRORONCE:
    case 5:
    case 7:
      return RenderTextureAddressMode::MIRROR_ONCE;
    case D3DTADDRESS_BORDER:
      return RenderTextureAddressMode::BORDER;
    default:
      return RenderTextureAddressMode::WRAP;
  }
}

RenderFilter ConvertFilter(uint32_t v) {
  switch (v) {
    case D3DTEXF_POINT:
    case D3DTEXF_NONE:
      return RenderFilter::NEAREST;
    case D3DTEXF_LINEAR:
      return RenderFilter::LINEAR;
    default:
      return RenderFilter::NEAREST;
  }
}

RenderBorderColor ConvertBorderColor(uint32_t v) {
  return v == 1 ? RenderBorderColor::OPAQUE_WHITE : RenderBorderColor::TRANSPARENT_BLACK;
}

void SetAlphaTestMode(bool enable) {
  uint32_t specConstants = enable ? SPEC_CONSTANT_ALPHA_TEST : 0;
  specConstants |= g_pipelineState.specConstants &
                   ~uint32_t(SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
}

// Clip planes, the enable mask and the scissor-test flag live at fixed byte
// offsets inside the guest device struct (GuestDevice is a byte-exact overlay
// of the real object, so these offsets refer to real, live guest memory).
// The offsets themselves are FM4's and live in guest_device.h.

struct GuestClipPlane {
  rex::be<float> x, y, z, w;
};

GuestClipPlane* ClipPlanes(GuestDevice* device) {
  return reinterpret_cast<GuestClipPlane*>(reinterpret_cast<uint8_t*>(device) +
                                           kGuestClipPlanesOffset);
}

bool ScissorTestEnabled(GuestDevice* device) {
  return GuestScissorEnable(device);
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

uint8_t* AllocateIntermediaryData(uint32_t size) {
  return g_intermediaryUploadAllocator.Allocate(size);
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
    case D3DRS_SEPARATEALPHABLENDENABLE:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.separateAlphaBlendEnable,
                    value != 0);
      break;
    case D3DRS_BLENDOP:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.blendOp, ConvertBlendOp(value));
      break;
    case D3DRS_BLENDOPALPHA:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.blendOpAlpha,
                    ConvertBlendOp(value));
      break;
    case D3DRS_STENCILENABLE: {
      const bool enabled = value != 0;
      if (g_pipelineState.stencilEnable != enabled)
        g_dirtyStates.renderTargetAndDepthStencil = true;
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilEnable, enabled);
      break;
    }
    case D3DRS_TWOSIDEDSTENCILMODE:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.twoSidedStencilMode, value != 0);
      break;
    case D3DRS_STENCILFUNC:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontFunc,
                    ConvertCmpFunc(value));
      break;
    case D3DRS_STENCILFAIL:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontFail,
                    ConvertStencilOp(value));
      break;
    case D3DRS_STENCILZFAIL:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontDepthFail,
                    ConvertStencilOp(value));
      break;
    case D3DRS_STENCILPASS:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontPass,
                    ConvertStencilOp(value));
      break;
    case D3DRS_CCWSTENCILFUNC:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackFunc,
                    ConvertCmpFunc(value));
      break;
    case D3DRS_CCWSTENCILFAIL:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackFail,
                    ConvertStencilOp(value));
      break;
    case D3DRS_CCWSTENCILZFAIL:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackDepthFail,
                    ConvertStencilOp(value));
      break;
    case D3DRS_CCWSTENCILPASS:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackPass,
                    ConvertStencilOp(value));
      break;
    case D3DRS_STENCILREF:
      SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontRef,
                             uint8_t(value));
      break;
    case D3DRS_STENCILMASK:
      SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontReadMask,
                             uint8_t(value));
      break;
    case D3DRS_STENCILWRITEMASK:
      SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontWriteMask,
                             uint8_t(value));
      break;
    case D3DRS_CCWSTENCILREF:
      SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilBackRef,
                             uint8_t(value));
      break;
    case D3DRS_CCWSTENCILMASK:
      SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilBackReadMask,
                             uint8_t(value));
      break;
    case D3DRS_CCWSTENCILWRITEMASK:
      SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilBackWriteMask,
                             uint8_t(value));
      break;
    case D3DRS_SCISSORTESTENABLE:
      SetDirtyValue(g_dirtyStates.scissorRect, g_scissorTestEnable, value != 0);
      break;
    case D3DRS_SLOPESCALEDEPTHBIAS:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.slopeScaledDepthBias,
                    std::bit_cast<float>(value));
      break;
    case D3DRS_DEPTHBIAS:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthBias,
                    int32_t(std::bit_cast<float>(value) * (1 << 24)));
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
  // This controls PA_CL_VTE_CNTL's viewport scale/offset bits, not clipping.
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetViewportEnable;
  cmd.setViewportEnable.value = value;
  RenderQueue::Enqueue(cmd);
}

void ProcSetViewportEnable(uint32_t value) {
  g_viewportEnabled = value != 0;
}

void SetClipPlaneState(GuestDevice* device, uint32_t enabledMask) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetClipPlaneState;
  enabledMask &= kGuestClipPlaneMask;
  cmd.setClipPlaneState.enabled = enabledMask != 0 ? 1u : 0u;
  if (enabledMask != 0) {
    const GuestClipPlane& plane = ClipPlanes(device)[std::countr_zero(enabledMask)];
    cmd.setClipPlaneState.plane[0] = plane.x.get();
    cmd.setClipPlaneState.plane[1] = plane.y.get();
    cmd.setClipPlaneState.plane[2] = plane.z.get();
    cmd.setClipPlaneState.plane[3] = plane.w.get();
  }
  RenderQueue::Enqueue(cmd);
}

void ProcSetClipPlaneState(uint32_t enabled, const float* plane) {
  g_sharedConstants.clipPlaneEnabled = enabled;
  std::copy_n(plane, 4, g_sharedConstants.clipPlane);
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
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.twoSidedStencilMode, twoSided != 0);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontFunc,
                ConvertCmpFunc(frontFunc));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontFail,
                ConvertStencilOp(frontFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontDepthFail,
                ConvertStencilOp(frontDepthFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontPass,
                ConvertStencilOp(frontPass));

  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackFunc,
                ConvertCmpFunc(backFunc));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackFail,
                ConvertStencilOp(backFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackDepthFail,
                ConvertStencilOp(backDepthFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackPass,
                ConvertStencilOp(backPass));

  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontRef,
                         uint8_t(ref));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilBackRef, uint8_t(ref));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontReadMask,
                         uint8_t(readMask));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilBackReadMask,
                         uint8_t(readMask));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontWriteMask,
                         uint8_t(writeMask));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilBackWriteMask,
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
  GuestShader* live = (shader != nullptr && IsFm4Resource(shader)) ? shader : nullptr;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexShader, live);
}

void ProcSetPixelShader(GuestShader* shader) {
  GuestShader* live = (shader != nullptr && IsFm4Resource(shader)) ? shader : nullptr;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.pixelShader, live);
}

void ProcSetVertexDeclaration(GuestVertexDeclaration* declaration) {
  GuestVertexDeclaration* live =
      (declaration != nullptr && IsFm4Resource(declaration)) ? declaration : nullptr;
  g_boundVertexDeclaration = live;
  // Tier A step 3: decl → swappedTexcoords / blendWeights + SPEC_CONSTANT_* bits.
  ApplyVertexDeclarationMetadata(live);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexDeclaration, live);
}

void ProcSetStreamSource(uint32_t index, GuestBuffer* buffer, uint32_t offset, uint32_t stride) {
  if (index >= 16u)
    return;

  GuestBuffer* live = (buffer != nullptr && IsFm4Resource(buffer) && buffer->buffer != nullptr &&
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
      (buffer != nullptr && IsFm4Resource(buffer) && buffer->buffer != nullptr) ? buffer : nullptr;
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer,
                live ? live->buffer->at(0) : RenderBufferReference{});
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format,
                live ? live->format : RenderFormat::R16_UINT);
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, live ? live->dataSize : 0u);
}

void ProcSetDrawGeometrySnapshot(const DrawGeometrySnapshot& snapshot) {
  for (uint32_t index = 0; index < std::size(snapshot.streams); ++index) {
    const DrawStreamSnapshot& stream = snapshot.streams[index];
    MixFrameTrace(g_frameTrace.vertexDataHash, reinterpret_cast<uintptr_t>(stream.buffer));
    MixFrameTrace(g_frameTrace.vertexDataHash, uint64_t(stream.offset) << 32 | stream.stride);
    MixFrameTrace(g_frameTrace.vertexDataHash, stream.rawSize);
    if (stream.rawData != nullptr && stream.rawSize != 0) {
      MixFrameTraceBytes(g_frameTrace.vertexDataHash, stream.rawData, stream.rawSize);
    }
    if (stream.rawData == nullptr || stream.rawSize == 0 || stream.stride == 0) {
      ProcSetStreamSource(index, stream.buffer, stream.offset, stream.stride);
      continue;
    }

    const RenderBufferReference ref =
        CurrentUploadAllocator().Upload(stream.rawData, stream.rawSize, false);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[index],
                  uint8_t(ref.ref != nullptr ? stream.stride : 0u));
    bool dirty = false;
    SetDirtyValue(dirty, g_vertexBufferViews[index].buffer, ref);
    SetDirtyValue(dirty, g_vertexBufferViews[index].size, ref.ref != nullptr ? stream.rawSize : 0u);
    SetDirtyValue(dirty, g_inputSlots[index].stride, ref.ref != nullptr ? stream.stride : 0u);
    if (dirty) {
      g_dirtyStates.vertexStreamFirst =
          std::min<uint8_t>(g_dirtyStates.vertexStreamFirst, uint8_t(index));
      g_dirtyStates.vertexStreamLast =
          std::max<uint8_t>(g_dirtyStates.vertexStreamLast, uint8_t(index));
    }
  }
  MixFrameTrace(g_frameTrace.vertexDataHash, reinterpret_cast<uintptr_t>(snapshot.indexBuffer));
  MixFrameTrace(g_frameTrace.vertexDataHash,
                uint64_t(snapshot.rawIndexSize) << 32 | snapshot.rawIndexStride);
  if (snapshot.rawIndexData != nullptr && snapshot.rawIndexSize != 0) {
    MixFrameTraceBytes(g_frameTrace.vertexDataHash, snapshot.rawIndexData, snapshot.rawIndexSize);
  }
  if (snapshot.rawIndexData != nullptr && snapshot.rawIndexSize != 0 &&
      (snapshot.rawIndexStride == 2u || snapshot.rawIndexStride == 4u)) {
    const RenderBufferReference ref =
        CurrentUploadAllocator().Upload(snapshot.rawIndexData, snapshot.rawIndexSize, false);
    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer, ref);
    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format,
                  snapshot.rawIndexStride == 4u ? RenderFormat::R32_UINT : RenderFormat::R16_UINT);
    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size,
                  ref.ref != nullptr ? snapshot.rawIndexSize : 0u);
  } else {
    ProcSetIndices(snapshot.indexBuffer);
  }
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
  if (depthStencil != nullptr) {
    g_implicitDepthStencil = depthStencil;
    // Latch per exact size for the stale-depth rescue (both pass directions).
    g_lastDepthBySize[DepthSizeKey(depthStencil->width, depthStencil->height,
                                   depthStencil->sampleCount)] = depthStencil;
  }
}

void ProcDestructResource(GuestResource* resource) {
  g_tempResources[CurrentRecordingFrame() % kNumFrames].push_back(resource);
}

}  // namespace

void SetTexture(GuestDevice* /*device*/, uint32_t index, GuestTexture* texture,
                uint32_t guestAddress) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetTexture;
  cmd.setTexture.index = index;
  cmd.setTexture.texture = texture;
  cmd.setTexture.guestAddress = guestAddress;
  RenderQueue::Enqueue(cmd);
}

void SetTextureBase(GuestDevice* /*device*/, uint32_t index, GuestBaseTexture* texture,
                    uint32_t guestAddress) {
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetTextureBase;
  cmd.setTextureBase.index = index;
  cmd.setTextureBase.texture = texture;
  cmd.setTextureBase.guestAddress = guestAddress;
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

namespace {

std::mutex g_producerGeometryMutex;
std::unordered_map<GuestDevice*, DrawGeometrySnapshot> g_producerGeometry;

uint32_t DecodeRawBufferSize(uint32_t fetchSize) {
  return ((fetchSize >> 2) & 0xFFFFFFu) * 4u;
}

uint8_t* SnapshotRawPhysicalBuffer(uint32_t fetchBase, uint32_t fetchSize,
                                   uint32_t swapElementBytes, bool preserveBaseLowBits) {
  auto* memory = ghp::GuestMemory();
  const uint32_t size = DecodeRawBufferSize(fetchSize);
  const uint32_t physicalAddress = fetchBase & (preserveBaseLowBits ? 0x1FFFFFFFu : 0x1FFFFFFCu);
  if (memory == nullptr || size == 0 || size > 4u * 1024u * 1024u ||
      uint64_t(physicalAddress) + size > 0x20000000ull ||
      memory->GetPhysicalHeap()->QueryRangeAccess(physicalAddress, physicalAddress + size - 1u) ==
          rex::memory::PageAccess::kNoAccess) {
    return nullptr;
  }

  const uint8_t* source = memory->TranslatePhysical<const uint8_t*>(physicalAddress);
  uint8_t* copy = g_intermediaryUploadAllocator.Allocate(size);
  if (source == nullptr || copy == nullptr)
    return nullptr;

  if (swapElementBytes == 4u) {
    const uint32_t dwordCount = size / 4u;
    const auto* sourceWords = reinterpret_cast<const uint32_t*>(source);
    auto* copyWords = reinterpret_cast<uint32_t*>(copy);
    for (uint32_t index = 0; index < dwordCount; ++index)
      copyWords[index] = std::byteswap(sourceWords[index]);
    const uint32_t swappedBytes = dwordCount * 4u;
    if (swappedBytes != size)
      std::memcpy(copy + swappedBytes, source + swappedBytes, size - swappedBytes);
  } else if (swapElementBytes == 2u) {
    const uint32_t wordCount = size / 2u;
    const auto* sourceWords = reinterpret_cast<const uint16_t*>(source);
    auto* copyWords = reinterpret_cast<uint16_t*>(copy);
    for (uint32_t index = 0; index < wordCount; ++index)
      copyWords[index] = std::byteswap(sourceWords[index]);
    if ((size & 1u) != 0)
      copy[size - 1u] = source[size - 1u];
  } else {
    std::memcpy(copy, source, size);
  }
  return copy;
}

}  // namespace

void SetStreamSource(GuestDevice* device, uint32_t index, GuestBuffer* buffer, uint32_t offset,
                     uint32_t stride) {
  if (device != nullptr && index < 16u) {
    std::lock_guard lock(g_producerGeometryMutex);
    DrawGeometrySnapshot& snapshot = g_producerGeometry[device];
    snapshot.streams[index] = {buffer, offset, stride};
  }
  RenderCommand cmd{};
  cmd.type = RenderCommandType::SetStreamSource;
  cmd.setStreamSource.index = index;
  cmd.setStreamSource.buffer = buffer;
  cmd.setStreamSource.offset = offset;
  cmd.setStreamSource.stride = stride;
  RenderQueue::Enqueue(cmd);
}

void SetIndices(GuestDevice* device, GuestBuffer* buffer) {
  if (device != nullptr) {
    std::lock_guard lock(g_producerGeometryMutex);
    g_producerGeometry[device].indexBuffer = buffer;
  }
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
  // Prefer the last RT that actually received a draw over the last one merely
  // bound: FM2 binds the (empty) display buffer after drawing the scene/menu
  // into another RT, and presenting the bound one shows black.
  if (IsFramebufferSizedPresentSource(g_lastDrawnRenderTarget)) {
    static bool loggedFirstDrawnWin = false;
    if (!loggedFirstDrawnWin) {
      loggedFirstDrawnWin = true;
      REXGPU_INFO("Present source: last-DRAWN RT {}x{} won over last-bound (first occurrence)",
                  g_lastDrawnRenderTarget->width, g_lastDrawnRenderTarget->height);
    }
    return g_lastDrawnRenderTarget;
  }
  if (IsFramebufferSizedPresentSource(g_lastPresentableRenderTarget)) {
    return g_lastPresentableRenderTarget;
  }
  if (IsFramebufferSizedPresentSource(g_implicitRenderTarget))
    return g_implicitRenderTarget;
  return nullptr;
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
  if (resource == nullptr || !IsFm4Resource(resource))
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
                      const GuestRect* sourceRect, uint32_t postClearFlags,
                      const float* postClearColor, float postClearZ) {
  // Unleashed StretchRect pattern: link dest to the current RT and defer the
  // copy/MSAA resolve until FlushPendingStretchRectCommands (before Present /
  // draw). Immediate path kept for non-texture destinations or region copies.
  RenderCommand cmd{};
  cmd.type = RenderCommandType::ResolveToTexture;
  cmd.resolveToTexture.destTexture = destTexture;
  cmd.resolveToTexture.destX =
      destPoint != nullptr ? uint32_t(std::max(destPoint->x.get(), int32_t{0})) : 0;
  cmd.resolveToTexture.destY =
      destPoint != nullptr ? uint32_t(std::max(destPoint->y.get(), int32_t{0})) : 0;
  cmd.resolveToTexture.hasSrc = sourceRect != nullptr;
  if (sourceRect != nullptr) {
    cmd.resolveToTexture.srcLeft = sourceRect->left.get();
    cmd.resolveToTexture.srcTop = sourceRect->top.get();
    cmd.resolveToTexture.srcRight = sourceRect->right.get();
    cmd.resolveToTexture.srcBottom = sourceRect->bottom.get();
  }
  cmd.resolveToTexture.postClearFlags = postClearFlags;
  for (uint32_t i = 0; i < std::size(cmd.resolveToTexture.postClearColor); ++i) {
    cmd.resolveToTexture.postClearColor[i] = postClearColor != nullptr ? postClearColor[i] : 0.0f;
  }
  cmd.resolveToTexture.postClearZ = postClearZ;
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

// True when `type` is one of the exact GuestDeclType dwords listed in
// guest_device.h (the instances FM2 was originally observed using). Variant
// dwords still decode via the field fallback in ConvertDeclType.
bool IsCanonicalDeclType(uint32_t type) {
  switch (type) {
    case D3DDECLTYPE_FLOAT1:
    case D3DDECLTYPE_FLOAT2:
    case D3DDECLTYPE_FLOAT3:
    case D3DDECLTYPE_FLOAT4:
    case D3DDECLTYPE_D3DCOLOR:
    case D3DDECLTYPE_UBYTE4:
    case D3DDECLTYPE_UBYTE4_2:
    case D3DDECLTYPE_SHORT2:
    case D3DDECLTYPE_SHORT4:
    case D3DDECLTYPE_UBYTE4N:
    case D3DDECLTYPE_UBYTE4N_2:
    case D3DDECLTYPE_SHORT2N:
    case D3DDECLTYPE_SHORT4N:
    case D3DDECLTYPE_USHORT2N:
    case D3DDECLTYPE_USHORT4N:
    case D3DDECLTYPE_UINT1:
    case D3DDECLTYPE_UDEC3:
    case D3DDECLTYPE_DEC3N:
    case D3DDECLTYPE_DEC3N_2:
    case D3DDECLTYPE_DEC3N_3:
    case D3DDECLTYPE_FLOAT16_2:
    case D3DDECLTYPE_FLOAT16_4:
      return true;
    default:
      return false;
  }
}

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
      break;
  }
  // Fallback: decode the packed Xbox-360 GPUVERTEXFETCHFORMAT fields instead
  // of failing the whole dword. The canonical GuestDeclType constants above are
  // only one instance each -- real FM2 declarations vary in the upper bits
  // (usage/stride metadata), so an exact-dword miss must not collapse a float3
  // or color attribute to UNKNOWN (previously forced R32_UINT -> garbage
  // attribute data). Field layout (matches ReXGlue080plume ConvertDeclType):
  //   bits 0-5  = data format (GPUVERTEXFETCHFORMAT)
  //   bits 8-9  = number format (0=UNORM, 1=SNORM, 2=UINT, 3=SINT)
  //   bit  11   = BGRA component swap
  {
    const uint32_t fmt = type & 0x3Fu;
    const uint32_t nf = (type >> 8) & 0x3u;
    switch (fmt) {
      case 0x06:  // k_8_8_8_8 (UBYTE4 / UBYTE4N / D3DCOLOR family)
        if (nf == 2)
          return RenderFormat::R8G8B8A8_UINT;
        return ((type >> 11) & 1u) ? RenderFormat::B8G8R8A8_UNORM : RenderFormat::R8G8B8A8_UNORM;
      case 0x07:  // k_2_10_10_10 -- shader bit-unpacks the raw dword.
      case 0x10:  // k_10_11_11 (packed)
      case 0x11:  // k_11_11_10 (packed)
        return RenderFormat::R32_UINT;
      case 0x19:  // k_16_16 (SHORT2 family)
        return nf == 3   ? RenderFormat::R16G16_SINT
               : nf == 1 ? RenderFormat::R16G16_SNORM
                         : RenderFormat::R16G16_UNORM;
      case 0x1A:  // k_16_16_16_16 (SHORT4 family)
        return nf == 3   ? RenderFormat::R16G16B16A16_SINT
               : nf == 1 ? RenderFormat::R16G16B16A16_SNORM
                         : RenderFormat::R16G16B16A16_UNORM;
      case 0x1F:  // k_16_16_FLOAT
        return RenderFormat::R16G16_FLOAT;
      case 0x20:  // k_16_16_16_16_FLOAT
        return RenderFormat::R16G16B16A16_FLOAT;
      case 0x21:  // k_32
        return RenderFormat::R32_UINT;
      case 0x22:  // k_32_32
        return RenderFormat::R32G32_UINT;
      case 0x23:  // k_32_32_32_32
        return RenderFormat::R32G32B32A32_UINT;
      case 0x24:  // k_32_FLOAT
        return RenderFormat::R32_FLOAT;
      case 0x25:  // k_32_32_FLOAT
        return RenderFormat::R32G32_FLOAT;
      case 0x39:  // k_32_32_32_FLOAT
        return RenderFormat::R32G32B32_FLOAT;
      case 0x26:  // k_32_32_32_32_FLOAT
        return RenderFormat::R32G32B32A32_FLOAT;
      default:
        return RenderFormat::UNKNOWN;
    }
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
      break;
  }
  // Field-decode fallback for variant dwords (same rationale as
  // ConvertDeclType): POSITION must expose raw bits, so map float fetches to
  // the equally-sized UINT formats.
  switch (type & 0x3Fu) {
    case 0x24:  // k_32_FLOAT
    case 0x21:  // k_32
      return RenderFormat::R32_UINT;
    case 0x25:  // k_32_32_FLOAT
    case 0x22:  // k_32_32
      return RenderFormat::R32G32_UINT;
    case 0x39:  // k_32_32_32_FLOAT
      return RenderFormat::R32G32B32_UINT;
    case 0x26:  // k_32_32_32_32_FLOAT
    case 0x23:  // k_32_32_32_32
      return RenderFormat::R32G32B32A32_UINT;
    case 0x1F:  // k_16_16_FLOAT
      outFloat16 = true;
      return RenderFormat::R16G16_UINT;
    case 0x20:  // k_16_16_16_16_FLOAT
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
    if (!IsCanonicalDeclType(e.type)) {
      // Variant fetch dword handled by the field-decode fallback (or UNKNOWN).
      // Log the first few so unmapped formats become visible instead of
      // silently degrading (mirrors 080plume's FM4_DECL_UNKNOWN_FMT logging).
      static std::atomic<uint32_t> s_variantDeclLogs{0};
      if (s_variantDeclLogs.fetch_add(1, std::memory_order_relaxed) < 16) {
        REXGPU_WARN(
            "CompleteVertexDeclaration: non-canonical decl type=0x{:X} fmt=0x{:X} nf={} "
            "usage={} idx={} stream={} offset={} -> format={}",
            e.type, e.type & 0x3Fu, (e.type >> 8) & 0x3u, uint32_t(e.usage), uint32_t(e.usageIndex),
            uint32_t(e.stream), uint32_t(e.offset), uint32_t(format));
      }
    }
    if (e.usage == D3DDECLUSAGE_POSITION && e.usageIndex == 0) {
      bool isFloat16 = false;
      format = ConvertPositionDeclType(e.type, isFloat16);
      if (isFloat16)
        decl->hasFloat16Position = true;
    } else if (e.usage == D3DDECLUSAGE_POSITION && e.usageIndex == 1) {
      decl->indexVertexStream = e.stream;
    } else if (e.usage == D3DDECLUSAGE_NORMAL || e.usage == D3DDECLUSAGE_TANGENT ||
               e.usage == D3DDECLUSAGE_BINORMAL) {
      // Classify by the fetch-format field (bits 0-5) so variant dwords take
      // the same path as their canonical GuestDeclType instance.
      const uint32_t fmt = e.type & 0x3Fu;
      if (fmt == 0x06u) {  // k_8_8_8_8 (UBYTE4 family)
        // Already hardware-UNORM-converted to a float4 by the input
        // assembler -- NOT R11G11B10-packed, must not also set that flag.
        format = RenderFormat::R8G8B8A8_UNORM;
        decl->hasUByte4TangentBasis = true;
      } else if (fmt != 0x39u && fmt != 0x26u) {  // not FLOAT3 / FLOAT4
        format = RenderFormat::R32_UINT;  // packed DEC3N/UDEC3 family; shader bit-unpacks raw.
        decl->hasR11G11B10Normal = true;
      }
    } else if (e.usage == D3DDECLUSAGE_TEXCOORD) {
      switch (e.type & 0x3Fu) {
        case 0x19:  // k_16_16 (SHORT2/SHORT2N/USHORT2N)
        case 0x1A:  // k_16_16_16_16 (SHORT4/SHORT4N/USHORT4N)
        case 0x1F:  // k_16_16_FLOAT (FLOAT16_2)
        case 0x20:  // k_16_16_16_16_FLOAT (FLOAT16_4)
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

// Fallback for low-level draw paths that do not reach the verified
// SetVertexDeclaration wrapper. Match the bound shader's parsed inputs against
// declarations FM2 created, preferring exact element counts and stream-0
// footprints. Declarations whose elements overflow the live stride are
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
    // Xbox stores stride/4 as a byte at device+0x3250+stream.
    const uint8_t dwords = device->streamStrideDwords[0];
    if (dwords != 0)
      stride = uint32_t(dwords) * 4u;
  }
  return stride;
}

GuestVertexDeclaration* ResolveVertexDeclaration(GuestDevice* device) {
  const uint32_t streamStride = EffectiveStream0Stride(device);

  // Keep host stride mirrors coherent when a low-level guest path wrote
  // device+0x3250 without reaching the BindVertexStream wrapper.
  if (streamStride != 0 && g_inputSlots[0].stride == 0) {
    g_inputSlots[0].stride = streamStride;
    g_pipelineState.vertexStrides[0] = uint8_t(streamStride > 255u ? 255u : streamStride);
  }

  // SetVertexDeclaration is ordered through the render queue. Reading the
  // mutable guest device here can observe a later frame and override that
  // draw-local bind.
  if (g_boundVertexDeclaration != nullptr && IsFm4Resource(g_boundVertexDeclaration) &&
      g_boundVertexDeclaration->type == ResourceType::VertexDeclaration &&
      DeclarationFitsStreamStride(g_boundVertexDeclaration, streamStride)) {
    return g_boundVertexDeclaration;
  }

  return MatchDeclarationForShader(g_pipelineState.vertexShader, streamStride);
}

void TraceVertexDeclarationChoice(GuestDevice* device, GuestVertexDeclaration* queued,
                                  GuestVertexDeclaration* resolved) {
  if (g_frameTraceIndex >= 64)
    return;

  const uint32_t draw = g_frameTrace.attemptedDraws - 1;
  const uint32_t deviceAddress = device != nullptr ? device->vertexDeclaration.get() : 0;
  GuestVertexDeclaration* deviceDeclaration =
      deviceAddress != 0 ? ghp::ToHost<GuestVertexDeclaration>(deviceAddress) : nullptr;
  const uint32_t streamStride = EffectiveStream0Stride(device);
  GuestVertexDeclaration* matched =
      MatchDeclarationForShader(g_pipelineState.vertexShader, streamStride);
  if (draw != 0 && (queued == nullptr || queued == matched))
    return;
  const uint64_t vertexShaderHash = ShaderTraceId(g_pipelineState.vertexShader);
  const char* source = resolved == nullptr             ? "none"
                       : resolved == queued            ? "queued"
                       : resolved == deviceDeclaration ? "device"
                                                       : "matched";
  const uint64_t hash = VertexDeclarationTraceId(resolved);
  REXGPU_INFO(
      "FrameDecl: n={} draw={} source={} queued=0x{:016X}/0x{:016X} "
      "device=0x{:08X}/0x{:016X} resolved=0x{:016X}/0x{:016X} stride={} elements={} "
      "swappedTexcoords=0x{:X} vs=0x{:016X} inputs={} matched=0x{:016X}/0x{:016X}",
      g_frameTraceIndex + 1, draw, source, uint64_t(reinterpret_cast<uintptr_t>(queued)),
      VertexDeclarationTraceId(queued), deviceAddress,
      uint64_t(reinterpret_cast<uintptr_t>(deviceDeclaration)),
      uint64_t(reinterpret_cast<uintptr_t>(resolved)), hash, streamStride,
      resolved != nullptr ? resolved->vertexElementCount : 0,
      resolved != nullptr ? resolved->swappedTexcoords : 0, vertexShaderHash,
      g_pipelineState.vertexShader != nullptr ? g_pipelineState.vertexShader->headerElements.size()
                                              : 0,
      uint64_t(reinterpret_cast<uintptr_t>(matched)), VertexDeclarationTraceId(matched));

  static std::unordered_set<uint64_t> loggedVertexShaders;
  const uint64_t shaderLogKey =
      vertexShaderHash != 0 ? vertexShaderHash
                            : uint64_t(reinterpret_cast<uintptr_t>(g_pipelineState.vertexShader));
  if (g_pipelineState.vertexShader != nullptr && loggedVertexShaders.insert(shaderLogKey).second) {
    for (uint32_t i = 0; i < g_pipelineState.vertexShader->headerElements.size(); ++i) {
      const ShaderHeaderElement& input = g_pipelineState.vertexShader->headerElements[i];
      REXGPU_INFO("FrameDeclShaderInput: vs=0x{:016X} input={} usage={} usageIndex={}",
                  vertexShaderHash, i, input.usage, input.usageIndex);
    }
  }

  static std::unordered_set<uint64_t> loggedDeclarations;
  if (resolved == nullptr || resolved->vertexElements == nullptr ||
      !loggedDeclarations.insert(hash).second) {
    return;
  }
  for (uint32_t i = 0; i < resolved->vertexElementCount; ++i) {
    const GuestVertexElement& element = resolved->vertexElements[i];
    if (element.usage == D3DDECLUSAGE_TEXCOORD) {
      REXGPU_INFO(
          "FrameDeclTexcoord: hash=0x{:016X} element={} stream={} offset={} type=0x{:08X} "
          "usageIndex={}",
          hash, i, element.stream, element.offset, element.type, element.usageIndex);
    }
  }
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
    ps.twoSidedStencilMode = false;
    ps.stencilFrontFunc = ps.stencilBackFunc = RenderComparisonFunction::ALWAYS;
    ps.stencilFrontFail = ps.stencilFrontDepthFail = ps.stencilFrontPass = RenderStencilOp::KEEP;
    ps.stencilBackFail = ps.stencilBackDepthFail = ps.stencilBackPass = RenderStencilOp::KEEP;
    ps.stencilFrontReadMask = ps.stencilBackReadMask = 0xFF;
    ps.stencilFrontWriteMask = ps.stencilBackWriteMask = 0xFF;
    ps.stencilFrontRef = ps.stencilBackRef = 0;
  } else if (!ps.twoSidedStencilMode) {
    ps.stencilBackFunc = ps.stencilFrontFunc;
    ps.stencilBackFail = ps.stencilFrontFail;
    ps.stencilBackDepthFail = ps.stencilFrontDepthFail;
    ps.stencilBackPass = ps.stencilFrontPass;
    ps.stencilBackReadMask = ps.stencilFrontReadMask;
    ps.stencilBackWriteMask = ps.stencilFrontWriteMask;
    ps.stencilBackRef = ps.stencilFrontRef;
  }
  if (!ps.zEnable && !ps.stencilEnable)
    ps.depthStencilFormat = RenderFormat::UNKNOWN;
  if (!ps.alphaBlendEnable) {
    ps.separateAlphaBlendEnable = false;
    ps.srcBlend = RenderBlend::ONE;
    ps.destBlend = RenderBlend::ZERO;
    ps.blendOp = RenderBlendOperation::ADD;
    ps.srcBlendAlpha = RenderBlend::ONE;
    ps.destBlendAlpha = RenderBlend::ZERO;
    ps.blendOpAlpha = RenderBlendOperation::ADD;
  } else if (!ps.separateAlphaBlendEnable) {
    ps.srcBlendAlpha = ps.srcBlend;
    ps.destBlendAlpha = ps.destBlend;
    ps.blendOpAlpha = ps.blendOp;
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

// One-time diagnostic logging of *why* a PSO build was rejected -- these
// paths used to fail completely silently, making an all-black screen
// indistinguishable from "every draw's pipeline build is failing."
void LogPipelineRejectOnce(const char* reason) {
  static std::unordered_set<std::string> s_logged;
  if (s_logged.insert(reason).second) {
    REXGPU_WARN("CreateGraphicsPipeline: rejected ({}) -- this draw will be skipped", reason);
  }
}

std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const PipelineState& ps) {
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

  RenderShader* pixelShader = LoadShader(ps.pixelShader, ps.specConstants);
  if (ps.pixelShader != nullptr && pixelShader == nullptr) {
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
  desc.stencilReadMask = ps.stencilFrontReadMask;
  desc.stencilWriteMask = ps.stencilFrontWriteMask;
  desc.stencilReference = ps.stencilFrontRef;
  // FM2P's plume (renderbag@4cc792a) carries independentStencilMasksAndReference
  // plus stencilBack{ReadMask,WriteMask,Reference}; this tree's plume
  // (zolaware@de0f70f) has neither, and its D3D12 backend applies the single
  // stencilReadMask/WriteMask/Reference to both faces. Two-sided stencil keeps
  // its independent front/back *ops* below; only the masks and reference are
  // shared. ps.twoSidedStencilMode / ps.stencilBack{ReadMask,WriteMask,Ref}
  // stay tracked in the pipeline state for when plume regains the fields.
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

RenderPipeline* GetPipeline(PipelineState ps) {
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
  if (g_failedPipelines.contains(hash)) {
    return nullptr;
  }
  auto& pipeline = g_pipelines[hash];
  if (pipeline == nullptr) {
    pipeline = CreateGraphicsPipeline(ps);
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

thread_local PendingShaderConstantFile t_pendingDrawVsConstants;
thread_local PendingShaderConstantFile t_pendingDrawPsConstants;

void ProcSetBooleans(const uint32_t* words) {
  if (words != nullptr)
    std::memcpy(g_sharedConstants.booleans, words, sizeof(g_sharedConstants.booleans));
}

void ProcSetLoopConstants(const uint32_t* values) {
  if (values != nullptr)
    std::memcpy(g_sharedConstants.loopConstants, values, sizeof(g_sharedConstants.loopConstants));
}

void ProcSetSamplerState(uint32_t index, uint32_t data0, uint32_t data3, uint32_t data5) {
  if (index >= 16 || Device() == nullptr || SamplerDescriptorSet() == nullptr)
    return;

  RenderSamplerDesc desc{};
  desc.addressU = ConvertAddressMode((data0 >> 10) & 0x7);
  desc.addressV = ConvertAddressMode((data0 >> 13) & 0x7);
  desc.addressW = ConvertAddressMode((data0 >> 16) & 0x7);
  desc.magFilter = ConvertFilter((data3 >> 19) & 0x3);
  desc.minFilter = ConvertFilter((data3 >> 21) & 0x3);
  desc.mipmapMode = RenderMipmapMode(ConvertFilter((data3 >> 23) & 0x3));
  desc.maxAnisotropy = 16;
  desc.anisotropyEnabled = false;
  desc.borderColor = ConvertBorderColor(data5 & 0x3);

  const uint64_t hash = XXH3_64bits(&desc, sizeof(desc));
  auto [it, inserted] = g_samplerStates.try_emplace(hash);
  auto& [descriptorIndex, sampler] = it->second;
  if (inserted)
    descriptorIndex = uint32_t(g_samplerStates.size());
  if (descriptorIndex == 0 || descriptorIndex > kSamplerDescriptorSize) {
    g_sharedConstants.samplerIndices[index] = 0;
    return;
  }
  if (sampler == nullptr) {
    sampler = Device()->createSampler(desc);
    if (sampler != nullptr) {
      SamplerDescriptorSet()->setSampler(descriptorIndex - 1, sampler.get());
    }
  }
  g_sharedConstants.samplerIndices[index] = sampler != nullptr ? descriptorIndex - 1 : 0;
}

void ProcSetShaderConstants(bool vertex, const uint8_t* memory, uint32_t index, uint32_t size) {
  uint32_t* destination = vertex ? g_vertexShaderConstants : g_pixelShaderConstants;
  const uint32_t capacity = vertex ? uint32_t(std::size(g_vertexShaderConstants))
                                   : uint32_t(std::size(g_pixelShaderConstants));
  if (memory == nullptr || size == 0 || (size & 3u) != 0 || index > capacity ||
      size / sizeof(uint32_t) > capacity - index) {
    return;
  }
  std::memcpy(destination + index, memory, size);
}

struct LocalRenderCommandQueue {
  std::array<RenderCommand, 32> commands{};
  uint32_t count = 0;

  RenderCommand& Enqueue() {
    assert(count < commands.size());
    return commands[count++];
  }

  void Submit() const { RenderQueue::EnqueueBulk(commands.data(), count); }
};

bool QueueConstantSnapshot(LocalRenderCommandQueue& queue, RenderCommandType type,
                           const uint32_t* source, ConstantSnapshotRange range) {
  if (source == nullptr || range.size == 0)
    return false;
  uint8_t* copy = g_intermediaryUploadAllocator.AllocateCopy(source + range.index, range.size);
  if (copy == nullptr) {
    REXGPU_WARN("Shader constant snapshot allocation failed ({} bytes)", range.size);
    return false;
  }
  RenderCommand& cmd = queue.Enqueue();
  cmd.type = type;
  cmd.setShaderConstants.memory = copy;
  cmd.setShaderConstants.index = range.index;
  cmd.setShaderConstants.size = range.size;
  return true;
}

void QueueDrawStateSnapshots(GuestDevice* device, LocalRenderCommandQueue& queue) {
  if (device == nullptr)
    return;

  thread_local GuestDevice* lastDevice = nullptr;
  const bool forceFullSnapshot = lastDevice != device || RenderQueue::IsRecording();
  lastDevice = device;

  // Vertex/index bindings are mutable guest context state just like shader
  // constants. Capture them on the producer thread so a later unbind cannot
  // reach the render thread before this draw is flushed. Keep the exact offset
  // observed by SetStreamSource when the live guest pointer/stride still match.
  DrawGeometrySnapshot geometry{};
  {
    std::lock_guard lock(g_producerGeometryMutex);
    const auto it = g_producerGeometry.find(device);
    if (it != g_producerGeometry.end())
      geometry = it->second;
  }
  for (uint32_t index = 0; index < std::size(geometry.streams); ++index) {
    const auto* address = &device->boundVertexStreams[index];
    const uint8_t strideDwords = device->streamStrideDwords[index];
    GuestBuffer* liveBuffer =
        address->get() != 0 ? ghp::ToHost<GuestBuffer>(address->get()) : nullptr;
    const uint32_t liveStride = uint32_t(strideDwords) * 4u;
    DrawStreamSnapshot& stream = geometry.streams[index];
    if (liveBuffer == nullptr || liveStride == 0) {
      stream = {};
    } else if (!IsFm4Resource(liveBuffer)) {
      const auto* fetchBase = reinterpret_cast<const rex::be<uint32_t>*>(
          reinterpret_cast<const uint8_t*>(device) + kGuestVertexFetchBase -
          kGuestVertexFetchStride * index);
      const auto* fetchSize = fetchBase + 1;
      const uint32_t rawSize = DecodeRawBufferSize(fetchSize->get());
      uint8_t* rawData = SnapshotRawPhysicalBuffer(fetchBase->get(), fetchSize->get(), 4u, false);
      stream = {nullptr, 0, liveStride, rawData, rawData != nullptr ? rawSize : 0u};
    } else if (stream.buffer != liveBuffer || stream.stride != liveStride) {
      stream = {liveBuffer, 0, liveStride};
    }
  }
  const auto* indexAddress = &device->boundIndexBuffer;
  geometry.indexBuffer = nullptr;
  geometry.rawIndexData = nullptr;
  geometry.rawIndexSize = 0;
  geometry.rawIndexStride = 0;
  if (indexAddress->get() != 0) {
    GuestBuffer* liveIndexBuffer = ghp::ToHost<GuestBuffer>(indexAddress->get());
    if (IsFm4Resource(liveIndexBuffer)) {
      geometry.indexBuffer = liveIndexBuffer;
    } else {
      const auto* common = ghp::ToHost<const rex::be<uint32_t>>(indexAddress->get());
      const auto* fetchBase = ghp::ToHost<const rex::be<uint32_t>>(indexAddress->get() + 0x18u);
      const auto* fetchSize = ghp::ToHost<const rex::be<uint32_t>>(indexAddress->get() + 0x1Cu);
      if (common != nullptr && fetchBase != nullptr && fetchSize != nullptr) {
        geometry.rawIndexStride = (common->get() & 0x80000000u) != 0 ? 4u : 2u;
        geometry.rawIndexSize = DecodeRawBufferSize(fetchSize->get());
        geometry.rawIndexData = SnapshotRawPhysicalBuffer(fetchBase->get(), fetchSize->get(),
                                                          geometry.rawIndexStride, true);
        if (geometry.rawIndexData == nullptr) {
          geometry.rawIndexSize = 0;
          geometry.rawIndexStride = 0;
        }
      }
    }
  }
  RenderCommand& geometryCommand = queue.Enqueue();
  geometryCommand.type = RenderCommandType::SetDrawGeometrySnapshot;
  geometryCommand.setDrawGeometrySnapshot = geometry;

  constexpr uint64_t kBooleanDirtyMask = uint64_t{1} << 56;
  uint64_t booleanFlags = device->dirtyFlags[4].get();
  if (forceFullSnapshot || (booleanFlags & kBooleanDirtyMask) != 0) {
    RenderCommand& cmd = queue.Enqueue();
    cmd.type = RenderCommandType::SetBooleans;
    for (uint32_t i = 0; i < 4; ++i) {
      cmd.setBooleans.words[i] = device->vertexShaderBoolConstants[i].get();
      cmd.setBooleans.words[4 + i] = device->pixelShaderBoolConstants[i].get();
    }
    device->dirtyFlags[4] = booleanFlags & ~kBooleanDirtyMask;
  }

  // The guest's 16 packed integer registers per stage are the Xenos loop
  // constants (count/start/step). Snapshot every draw; their dirty-bit mapping
  // is separate from the Boolean bit and was previously not mirrored at all.
  {
    RenderCommand& cmd = queue.Enqueue();
    cmd.type = RenderCommandType::SetLoopConstants;
    for (uint32_t i = 0; i < 16; ++i) {
      cmd.setLoopConstants.values[i] = device->vertexShaderIntConstants[i].get();
      cmd.setLoopConstants.values[16 + i] = device->pixelShaderIntConstants[i].get();
    }
  }

  uint64_t samplerFlags = device->dirtyFlags[3].get();
  for (uint32_t i = 0; i < 16; ++i) {
    const uint64_t mask = uint64_t{1} << (31u - i);
    if (!forceFullSnapshot && (samplerFlags & mask) == 0)
      continue;
    const GuestFetchConstant& state = device->textureFetchConstants[i];
    RenderCommand& cmd = queue.Enqueue();
    cmd.type = RenderCommandType::SetSamplerState;
    cmd.setSamplerState.index = i;
    cmd.setSamplerState.data0 = state.data[0].get();
    cmd.setSamplerState.data3 = state.data[3].get();
    cmd.setSamplerState.data5 = state.data[5].get();
    samplerFlags &= ~mask;
  }
  device->dirtyFlags[3] = samplerFlags;

  uint64_t vsFlags = device->dirtyFlags[0].get();
  std::array<uint32_t, PendingShaderConstantFile::kRegisterCount *
                           PendingShaderConstantFile::kDwordsPerRegister>
      mergedVsConstants;
  const bool hasPendingVs = !t_pendingDrawVsConstants.empty();
  ConstantSnapshotRange vsRange = (forceFullSnapshot || hasPendingVs)
                                      ? ConstantSnapshotRange{0, kVsFloatConstantBytes}
                                      : GetConstantSnapshotRange(vsFlags, 64);
  const uint32_t* vsSource = reinterpret_cast<const uint32_t*>(device->vertexShaderFloatConstants);
  if (hasPendingVs) {
    std::memcpy(mergedVsConstants.data(), vsSource, kVsFloatConstantBytes);
    t_pendingDrawVsConstants.OverlayAndClear(mergedVsConstants.data(), 256);
    vsSource = mergedVsConstants.data();
  }
  if (QueueConstantSnapshot(queue, RenderCommandType::SetVertexShaderConstants, vsSource,
                            vsRange)) {
    device->dirtyFlags[0] = 0;
  }

  uint64_t psFlags = device->dirtyFlags[1].get();
  std::array<uint32_t, PendingShaderConstantFile::kRegisterCount *
                           PendingShaderConstantFile::kDwordsPerRegister>
      mergedPsConstants;
  const bool hasPendingPs = !t_pendingDrawPsConstants.empty();
  ConstantSnapshotRange psRange = (forceFullSnapshot || hasPendingPs)
                                      ? ConstantSnapshotRange{0, kPsFloatConstantBytes}
                                      : GetConstantSnapshotRange(psFlags, 56);
  const uint32_t* psSource = reinterpret_cast<const uint32_t*>(device->pixelShaderFloatConstants);
  if (hasPendingPs) {
    std::memcpy(mergedPsConstants.data(), psSource, kPsFloatConstantBytes);
    t_pendingDrawPsConstants.OverlayAndClear(mergedPsConstants.data(), 224);
    psSource = mergedPsConstants.data();
  }
  if (QueueConstantSnapshot(queue, RenderCommandType::SetPixelShaderConstants, psSource, psRange)) {
    device->dirtyFlags[1] = 0;
  }
}

RenderPrimitiveTopology ConvertPrimitiveType(uint32_t type) {
  // Match SOURCE / Unleashed: unknown Xbox types (e.g. D3DPT_RECTLIST=8) still
  // attempt a triangle-list draw rather than skipping the entire draw (which
  // left UI/compositing black). QUADLIST/FAN use TRIANGLE_LIST + re-index.
  switch (type) {
    case D3DPT_POINTLIST:
      return RenderPrimitiveTopology::POINT_LIST;
    case D3DPT_LINELIST:
      return RenderPrimitiveTopology::LINE_LIST;
    case D3DPT_LINESTRIP:
      return RenderPrimitiveTopology::LINE_STRIP;
    case D3DPT_TRIANGLELIST:
    case D3DPT_QUADLIST:
    case D3DPT_TRIANGLEFAN:
    case D3DPT_RECTLIST:
      return RenderPrimitiveTopology::TRIANGLE_LIST;
    case D3DPT_TRIANGLESTRIP:
      return RenderPrimitiveTopology::TRIANGLE_STRIP;
    default:
      return RenderPrimitiveTopology::TRIANGLE_LIST;
  }
}

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
  if (primitiveType == D3DPT_QUADLIST)
    return g_quadIndexData.prepare(vertexOrPrimCount);
  if (primitiveType == D3DPT_TRIANGLEFAN)
    return g_triangleFanIndexData.prepare(vertexOrPrimCount);
  return 0;
}

}  // namespace

void StageDrawShaderConstants(bool vertex, uint32_t startRegister, const void* beDwords,
                              uint32_t registerCount) {
  PendingShaderConstantFile& pending = vertex ? t_pendingDrawVsConstants : t_pendingDrawPsConstants;
  pending.Stage(startRegister, static_cast<const uint32_t*>(beDwords), registerCount);
}

bool HasBoundPipeline() {
  return g_hasBoundPipeline;
}

void FlushRenderState(GuestDevice* device, uint32_t primitiveType) {
  std::lock_guard lock(RecordingMutex());
  g_hasBoundPipeline = false;
  if (device == nullptr)
    return;

  // FM2's deferred draw-list nodes carry color-write state that the direct
  // D3D9 mirrors don't see. A bound pixel shader denotes a color pass; keep
  // stale zero write masks from turning it into an accidental depth prepass.
  if (g_pipelineState.pixelShader != nullptr && g_pipelineState.colorWriteEnable == 0 &&
      g_renderTarget != nullptr) {
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.colorWriteEnable, 0xFu);
    g_dirtyStates.renderTargetAndDepthStencil = true;
  }

  GuestBaseTexture* renderTarget = g_pipelineState.colorWriteEnable != 0 ? g_renderTarget : nullptr;
  GuestSurface* depthStencil =
      (g_pipelineState.zEnable || g_pipelineState.stencilEnable) ? g_depthStencil : nullptr;
  if (depthStencil == nullptr && (g_pipelineState.zEnable || g_pipelineState.stencilEnable) &&
      renderTarget != nullptr && g_implicitDepthStencil != nullptr &&
      AttachmentsCompatible(renderTarget, g_implicitDepthStencil)) {
    depthStencil = g_implicitDepthStencil;
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthStencilFormat,
                  depthStencil->format);
    g_dirtyStates.renderTargetAndDepthStencil = true;
  }
  if (!AttachmentsCompatible(renderTarget, depthStencil)) {
    // Stale cross-pass depth bind (e.g. 512x512 envmap depth still bound when
    // scene draws resume on the 720p color RT): EDRAM had no such pairing
    // constraint, so FM2 never re-binds. Swap in the last full-frame depth of
    // the color RT's sample count instead of dropping depth entirely —
    // dropping disables depth testing for every scene draw (wrong occlusion).
    GuestSurface* rescue = nullptr;
    if (depthStencil != nullptr && renderTarget != nullptr) {
      auto it = g_lastDepthBySize.find(
          DepthSizeKey(renderTarget->width, renderTarget->height, GetSampleCount(renderTarget)));
      if (it != g_lastDepthBySize.end()) {
        GuestSurface* candidate = it->second;
        if (candidate != nullptr && candidate != depthStencil && IsLiveHostTexture(candidate) &&
            AttachmentsCompatible(renderTarget, candidate)) {
          rescue = candidate;
        }
      }
    }
    if (rescue != nullptr) {
      static uint64_t rescuedDepth = 0;
      if (++rescuedDepth <= 12 || rescuedDepth % 3000 == 1) {
        REXGPU_INFO("FlushRenderState: swapped stale {}x{} depth for full-frame {}x{}@{} (n={})",
                    depthStencil->width, depthStencil->height, rescue->width, rescue->height,
                    GetSampleCount(rescue), rescuedDepth);
      }
      depthStencil = rescue;
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthStencilFormat,
                    depthStencil->format);
      g_dirtyStates.renderTargetAndDepthStencil = true;
    } else {
      static uint64_t incompatibleAttachments = 0;
      if (++incompatibleAttachments <= 24 || incompatibleAttachments % 300 == 1) {
        REXGPU_WARN(
            "FlushRenderState: dropping incompatible depth target color={}x{}@{} depth={}x{}@{} "
            "(n={})",
            renderTarget != nullptr ? renderTarget->width : 0,
            renderTarget != nullptr ? renderTarget->height : 0, GetSampleCount(renderTarget),
            depthStencil != nullptr ? depthStencil->width : 0,
            depthStencil != nullptr ? depthStencil->height : 0, GetSampleCount(depthStencil),
            incompatibleAttachments);
      }
      depthStencil = nullptr;
      g_dirtyStates.renderTargetAndDepthStencil = true;
    }
  }

  const RenderPrimitiveTopology topology = ConvertPrimitiveType(primitiveType);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.primitiveTopology, topology);

  // Drain deferred Resolve/StretchRect (incl. MSAA) before we bind RTs for
  // drawing -- mirrors Unleashed FlushRenderStateForRenderThread.
  FlushPendingStretchRectCommands();

  if (g_dirtyStates.renderTargetAndDepthStencil || g_framebuffer == nullptr) {
    AddBarrier(renderTarget, RenderTextureLayout::COLOR_WRITE);
    AddBarrier(depthStencil, RenderTextureLayout::DEPTH_WRITE);
    FlushBarriers();
    SetFramebuffer(renderTarget, depthStencil, false);
    EnsureAttachmentInitialized(renderTarget);
    EnsureAttachmentInitialized(depthStencil);
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
    GuestVertexDeclaration* queued = g_boundVertexDeclaration;
    GuestVertexDeclaration* decl = ResolveVertexDeclaration(device);
    // Completes the decl and applies SPEC_CONSTANT_POSITION_F16 / R11G11B10 /
    // UBYTE4 + shared swizzle metadata before PSO lookup.
    ApplyVertexDeclarationMetadata(decl);
    TraceVertexDeclarationChoice(device, queued, decl);
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexDeclaration, decl);
  }

  PipelineState pipelineState = g_pipelineState;
  // Surface creation preserves the Xbox MSAA count. D3D12 requires the PSO
  // SampleDesc to match the bound attachments exactly; forcing COUNT_1 here
  // made draws disappear and eventually removed the device.
  pipelineState.sampleCount =
      renderTarget != nullptr ? GetSampleCount(renderTarget) : GetSampleCount(depthStencil);
  if (depthStencil == nullptr) {
    pipelineState.zEnable = false;
    pipelineState.zWriteEnable = false;
    pipelineState.stencilEnable = false;
    pipelineState.depthStencilFormat = RenderFormat::UNKNOWN;
  }
  RenderPipeline* pipeline = GetPipeline(pipelineState);
  if (pipeline == nullptr)
    return;
  RenderPipelineLayout* layout = PipelineLayout();
  RenderCommandList* commandList = CommandList();
  if (layout == nullptr || commandList == nullptr)
    return;
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

  g_sharedConstants.swappedTexcoords = g_pipelineState.vertexDeclaration != nullptr
                                           ? g_pipelineState.vertexDeclaration->swappedTexcoords
                                           : 0;

  // Snapshot commands already copied the exact guest-thread state into these
  // persistent render-thread files. Never dereference mutable guest state here:
  // the producer may already be preparing the next draw.
  CurrentUploadAllocator().UploadAndBindRootDescriptor(g_vertexShaderConstants,
                                                       kVsFloatConstantBytes, 0, true);
  CurrentUploadAllocator().UploadAndBindRootDescriptor(g_pixelShaderConstants,
                                                       kPsFloatConstantBytes, 1, true);
  CurrentUploadAllocator().UploadAndBindRootDescriptor(&g_sharedConstants,
                                                       sizeof(g_sharedConstants), 2, false);

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

  // A draw is now guaranteed to land on g_renderTarget — remember it as the
  // preferred present source (full-frame targets only; EDRAM tile bands must
  // not displace the real 720p scene/menu RT).
  if (IsFramebufferSizedPresentSource(g_renderTarget))
    g_lastDrawnRenderTarget = g_renderTarget;
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

  ++g_frameTrace.clears;
  MixFrameTrace(g_frameTrace.shapeHash, flags);
  MixTextureShape(g_frameTrace.shapeHash, g_renderTarget);
  MixFrameTraceBytes(g_frameTrace.clearHash, rgba, sizeof(float) * 4);
  MixFrameTraceBytes(g_frameTrace.clearHash, &z, sizeof(z));

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

  const bool onePass = AttachmentsCompatible(g_renderTarget, g_depthStencil);
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
  ++g_frameTrace.resolves;
  MixTextureShape(g_frameTrace.shapeHash, destTexture);
  MixFrameTrace(g_frameTrace.shapeHash, uint64_t(destX) << 32 | destY);
  MixFrameTraceBytes(g_frameTrace.shapeHash, &srcRect, sizeof(srcRect));
  if (!IsLiveHostTexture(destTexture))
    return;
  // Swap/Resolve often run after the guest unbound the color RT. Unleashed keeps
  // using the live surface via pending StretchRect links; fall back to the last
  // presentable RT so aperture resolves are not no-ops.
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

  // D3D12 does not clip CopyTextureRegion / ResolveSubresourceRegion. FM2 can
  // resolve a larger EDRAM surface into a smaller aperture texture, so bound
  // both the explicit and implicit-full source rectangles to the destination.
  RenderRect clippedSrc =
      hasSrc ? srcRect : RenderRect(0, 0, int32_t(source->width), int32_t(source->height));
  if (!ClipResolveCopyRegion(source, destTexture, clippedSrc, destX, destY)) {
    static uint64_t resolveEmptySkip = 0;
    if (++resolveEmptySkip <= 24 || resolveEmptySkip % 300 == 1) {
      REXGPU_WARN("ResolveToTexture: empty/out-of-range region {}x{} -> {}x{} at {},{} (n={})",
                  source->width, source->height, destTexture->width, destTexture->height, destX,
                  destY, resolveEmptySkip);
    }
    return;
  }

  // Region / surface-dest path: must not copy while source is still bound.
  if (g_framebuffer != nullptr) {
    CommandList()->setFramebuffer(nullptr);
    g_framebuffer = nullptr;
    g_dirtyStates.renderTargetAndDepthStencil = true;
  }

  const bool multiSampling =
      surface != nullptr && surface->sampleCount != RenderSampleCount::COUNT_1;
  if (multiSampling) {
    AddBarrier(source, RenderTextureLayout::RESOLVE_SOURCE);
    AddBarrier(destTexture, RenderTextureLayout::RESOLVE_DEST);
    FlushBarriers();
    const bool fullSubresource =
        destX == 0 && destY == 0 && clippedSrc.left == 0 && clippedSrc.top == 0 &&
        clippedSrc.right == int32_t(source->width) &&
        clippedSrc.bottom == int32_t(source->height) && source->width == destTexture->width &&
        source->height == destTexture->height;
    if (fullSubresource) {
      CommandList()->resolveTexture(destTexture->texture, source->texture);
    } else {
      CommandList()->resolveTextureRegion(destTexture->texture, destX, destY, source->texture,
                                          &clippedSrc);
    }
  } else {
    // 1x surfaces: copy, never ResolveSubresourceRegion.
    AddBarrier(source, RenderTextureLayout::COPY_SOURCE);
    AddBarrier(destTexture, RenderTextureLayout::COPY_DEST);
    FlushBarriers();
    const RenderBox srcBox(clippedSrc.left, clippedSrc.top, clippedSrc.right, clippedSrc.bottom);
    CommandList()->copyTextureRegion(
        RenderTextureCopyLocation::Subresource(destTexture->texture, 0),
        RenderTextureCopyLocation::Subresource(source->texture, 0), destX, destY, 0, &srcBox);
  }
  MarkAttachmentInitialized(destTexture);
  g_stretchRectPresentOverride.store(nullptr, std::memory_order_relaxed);
}

void ProcDrawPrimitive(GuestDevice* device, uint32_t primitiveType, uint32_t startVertex,
                       uint32_t vertexCount) {
  if (IsDeviceLost())
    return;
  ++g_frameTrace.attemptedDraws;
  g_hasBoundPipeline = false;
  const uint32_t convertedIndexCount = PrepareConvertedIndices(primitiveType, vertexCount);
  FlushRenderState(device, primitiveType);
  if (!g_hasBoundPipeline)
    return;
  const uint32_t geometry[] = {primitiveType, startVertex, vertexCount, convertedIndexCount};
  TraceIssuedDraw(1, geometry, std::size(geometry));
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
  ++g_frameTrace.attemptedDraws;
  FlushRenderState(device, primitiveType);
  if (!g_hasBoundPipeline)
    return;
  const uint32_t geometry[] = {primitiveType, uint32_t(baseVertexIndex), startIndex, indexCount};
  TraceIssuedDraw(2, geometry, std::size(geometry));
  CommandList()->drawIndexedInstanced(indexCount, 1, startIndex, baseVertexIndex, 0);
}

void ProcDrawPrimitiveUP(GuestDevice* device, uint32_t primitiveType, uint32_t vertexCount,
                         uint8_t* copy, uint32_t stride, uint32_t bytes) {
  if (IsDeviceLost())
    return;
  ++g_frameTrace.attemptedDraws;
  g_hasBoundPipeline = false;

  const uint8_t savedStride0 = g_pipelineState.vertexStrides[0];
  g_pipelineState.vertexStrides[0] = uint8_t(stride);
  const uint32_t convertedIndexCount = PrepareConvertedIndices(primitiveType, vertexCount);
  FlushRenderState(device, primitiveType);
  g_pipelineState.vertexStrides[0] = savedStride0;
  if (!g_hasBoundPipeline)
    return;
  const uint32_t geometry[] = {primitiveType, vertexCount, stride, bytes, convertedIndexCount};
  TraceIssuedDraw(3, geometry, std::size(geometry), copy, bytes);

  const uint8_t* uploadData = copy;
  std::array<uint8_t, 4 * 20> normalizedQuad;
  if (!g_viewportEnabled && primitiveType == D3DPT_TRIANGLESTRIP &&
      bytes == normalizedQuad.size()) {
    std::memcpy(normalizedQuad.data(), copy, bytes);
    if (NormalizeUnitFullscreenUpQuad(normalizedQuad.data(), vertexCount, stride, bytes))
      uploadData = normalizedQuad.data();
  }

  RenderBufferReference ref = CurrentUploadAllocator().Upload(uploadData, bytes, true);
  if (ref.ref == nullptr) {
    g_hasBoundPipeline = false;
    return;
  }
  RenderPipelineLayout* layout = PipelineLayout();
  RenderCommandList* commandList = CommandList();
  // FlushRenderState already selected and bound the PSO using the temporary
  // user-pointer stride and the actual attachment sample count.
  if (layout == nullptr || commandList == nullptr) {
    g_hasBoundPipeline = false;
    return;
  }
  commandList->setGraphicsPipelineLayout(layout);
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
  LocalRenderCommandQueue queue;
  QueueDrawStateSnapshots(device, queue);
  RenderCommand& cmd = queue.Enqueue();
  cmd.type = RenderCommandType::DrawPrimitive;
  cmd.drawPrimitive.device = device;
  cmd.drawPrimitive.primitiveType = primitiveType;
  cmd.drawPrimitive.startVertex = startVertex;
  cmd.drawPrimitive.vertexCount = vertexCount;
  queue.Submit();
}

void DrawIndexedVertices(GuestDevice* device, uint32_t primitiveType, int32_t baseVertexIndex,
                         uint32_t startIndex, uint32_t indexCount) {
  LocalRenderCommandQueue queue;
  QueueDrawStateSnapshots(device, queue);
  RenderCommand& cmd = queue.Enqueue();
  cmd.type = RenderCommandType::DrawIndexedPrimitive;
  cmd.drawIndexedPrimitive.device = device;
  cmd.drawIndexedPrimitive.primitiveType = primitiveType;
  cmd.drawIndexedPrimitive.baseVertexIndex = baseVertexIndex;
  cmd.drawIndexedPrimitive.startIndex = startIndex;
  cmd.drawIndexedPrimitive.indexCount = indexCount;
  queue.Submit();
}

void DrawUserPointerVertices(GuestDevice* device, uint32_t primitiveType, uint32_t vertexCount,
                             const void* data, uint32_t stride) {
  if (data == nullptr || vertexCount == 0 || stride == 0)
    return;
  const uint32_t bytes = vertexCount * stride;
  // Copy on the guest thread so Enqueue can return before the guest reuses
  // its stack/heap buffer (Unleashed intermediary upload pattern).
  uint8_t* copy = g_intermediaryUploadAllocator.AllocateCopy(data, bytes);
  if (copy == nullptr) {
    REXGPU_WARN("DrawUserPointerVertices: intermediary upload exhausted ({} bytes)", bytes);
    return;
  }
  LocalRenderCommandQueue queue;
  QueueDrawStateSnapshots(device, queue);
  RenderCommand& cmd = queue.Enqueue();
  cmd.type = RenderCommandType::DrawPrimitiveUP;
  cmd.drawPrimitiveUP.device = device;
  cmd.drawPrimitiveUP.primitiveType = primitiveType;
  cmd.drawPrimitiveUP.vertexCount = vertexCount;
  cmd.drawPrimitiveUP.vertexData = copy;
  cmd.drawPrimitiveUP.stride = stride;
  cmd.drawPrimitiveUP.bytes = bytes;
  queue.Submit();
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
    case RenderCommandType::SetClipPlaneState:
      ProcSetClipPlaneState(cmd.setClipPlaneState.enabled, cmd.setClipPlaneState.plane);
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
    case RenderCommandType::SetSamplerState:
      ProcSetSamplerState(cmd.setSamplerState.index, cmd.setSamplerState.data0,
                          cmd.setSamplerState.data3, cmd.setSamplerState.data5);
      break;
    case RenderCommandType::SetBooleans:
      ProcSetBooleans(cmd.setBooleans.words);
      break;
    case RenderCommandType::SetLoopConstants:
      ProcSetLoopConstants(cmd.setLoopConstants.values);
      break;
    case RenderCommandType::SetVertexShaderConstants:
      ProcSetShaderConstants(true, cmd.setShaderConstants.memory, cmd.setShaderConstants.index,
                             cmd.setShaderConstants.size);
      break;
    case RenderCommandType::SetPixelShaderConstants:
      ProcSetShaderConstants(false, cmd.setShaderConstants.memory, cmd.setShaderConstants.index,
                             cmd.setShaderConstants.size);
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
    case RenderCommandType::SetDrawGeometrySnapshot:
      ProcSetDrawGeometrySnapshot(cmd.setDrawGeometrySnapshot);
      break;
    case RenderCommandType::Clear:
      ProcClear(cmd.clear.flags, cmd.clear.color, cmd.clear.z);
      break;
    case RenderCommandType::ResolveToTexture: {
      RenderRect srcRect(cmd.resolveToTexture.srcLeft, cmd.resolveToTexture.srcTop,
                         cmd.resolveToTexture.srcRight, cmd.resolveToTexture.srcBottom);
      ProcResolveToTexture(cmd.resolveToTexture.destTexture, cmd.resolveToTexture.destX,
                           cmd.resolveToTexture.destY, cmd.resolveToTexture.hasSrc, srcRect);
      if (cmd.resolveToTexture.postClearFlags != 0) {
        // Predicated-tiling band sequences: hardware re-renders EDRAM between
        // band resolves, so resolve→clear→resolve is safe there. We render all
        // bands in ONE pass (tile RTs grown to 720p at create), and the guest
        // then issues its band resolves back-to-back — clearing after an
        // intermediate band destroys the rows the NEXT band still needs
        // (RenderDoc frame 400: RT full at band 1, 100% black at band 2 ->
        // only the top band ever reached the frontbuffer). Apply the post
        // clear only when this resolve's dest region reaches the bottom of
        // the destination (last band, or full-surface resolve).
        const GuestBaseTexture* dest = cmd.resolveToTexture.destTexture;
        bool isIntermediateBand = false;
        if (dest != nullptr && cmd.resolveToTexture.hasSrc) {
          const int32_t copyHeight = srcRect.bottom - srcRect.top;
          isIntermediateBand =
              int32_t(cmd.resolveToTexture.destY) + copyHeight < int32_t(dest->height);
        }
        if (!isIntermediateBand) {
          // A full-surface resolve is normally deferred until the next draw or
          // present. Resolve-with-clear requires the copy to consume the old
          // EDRAM contents before the source is cleared for the next pass.
          FlushPendingStretchRectCommands();
          ProcClear(cmd.resolveToTexture.postClearFlags, cmd.resolveToTexture.postClearColor,
                    cmd.resolveToTexture.postClearZ);
        } else {
          static uint64_t bandClearDeferred = 0;
          if (++bandClearDeferred <= 12 || bandClearDeferred % 300 == 1) {
            REXGPU_INFO(
                "ResolveToTexture: deferring post-clear past intermediate band destY={} (n={})",
                cmd.resolveToTexture.destY, bandClearDeferred);
          }
        }
      }
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
      LogAndResetFrameTrace();
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
      ProcUnlockBuffer16(cmd.unlockBuffer.buffer, cmd.unlockBuffer.data, cmd.unlockBuffer.size);
      break;
    case RenderCommandType::UnlockBuffer32:
      ProcUnlockBuffer32(cmd.unlockBuffer.buffer, cmd.unlockBuffer.data, cmd.unlockBuffer.size);
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

void DispatchRecordedRenderCommands(const RenderCommand* commands, size_t count,
                                    const uint8_t* liveVsConstants,
                                    uint32_t liveVsConstantBytes) {
  if (commands == nullptr || count == 0)
    return;

  std::lock_guard lock(RecordingMutex());
  FlushPendingStretchRectCommands();

  struct SavedState {
    PipelineState pipelineState;
    GuestBaseTexture* renderTarget;
    GuestBaseTexture* implicitRenderTarget;
    GuestSurface* depthStencil;
    GuestSurface* implicitDepthStencil;
    RenderViewport viewport;
    bool viewportEnabled;
    GuestVertexDeclaration* boundVertexDeclaration;
    SharedConstants sharedConstants;
    bool sharedConstantsInitialized;
    std::array<GuestTexture*, 16> textures;
    std::array<uint32_t, std::size(g_vertexShaderConstants)> vertexConstants;
    std::array<uint32_t, std::size(g_pixelShaderConstants)> pixelConstants;
    bool scissorTestEnable;
    RenderRect scissorRect;
    std::array<RenderVertexBufferView, 16> vertexBufferViews;
    std::array<RenderInputSlot, 16> inputSlots;
    RenderIndexBufferView indexBufferView;
  } saved{g_pipelineState,
          g_renderTarget,
          g_implicitRenderTarget,
          g_depthStencil,
          g_implicitDepthStencil,
          g_viewport,
          g_viewportEnabled,
          g_boundVertexDeclaration,
          g_sharedConstants,
          g_sharedConstantsInitialized,
          {},
          {},
          {},
          g_scissorTestEnable,
          g_scissorRect,
          {},
          {},
          g_indexBufferView};
  std::copy(std::begin(g_textures), std::end(g_textures), saved.textures.begin());
  std::copy(std::begin(g_vertexShaderConstants), std::end(g_vertexShaderConstants),
            saved.vertexConstants.begin());
  std::copy(std::begin(g_pixelShaderConstants), std::end(g_pixelShaderConstants),
            saved.pixelConstants.begin());
  std::copy(std::begin(g_vertexBufferViews), std::end(g_vertexBufferViews),
            saved.vertexBufferViews.begin());
  std::copy(std::begin(g_inputSlots), std::end(g_inputSlots), saved.inputSlots.begin());

  const bool haveLiveTransform = liveVsConstants != nullptr && liveVsConstantBytes >= 12u * 16u;
  for (size_t i = 0; i < count; ++i) {
    const RenderCommandType type = commands[i].type;
    const bool isDraw = type == RenderCommandType::DrawPrimitive ||
                        type == RenderCommandType::DrawIndexedPrimitive ||
                        type == RenderCommandType::DrawPrimitiveUP;
    if (isDraw) {
      if (haveLiveTransform)
        ProcSetShaderConstants(true, liveVsConstants, 0, 12u * 16u);
    }
    DispatchRenderCommand(commands[i]);
  }

  g_pipelineState = saved.pipelineState;
  g_renderTarget = saved.renderTarget;
  g_implicitRenderTarget = saved.implicitRenderTarget;
  g_depthStencil = saved.depthStencil;
  g_implicitDepthStencil = saved.implicitDepthStencil;
  g_viewport = saved.viewport;
  g_viewportEnabled = saved.viewportEnabled;
  g_boundVertexDeclaration = saved.boundVertexDeclaration;
  g_sharedConstants = saved.sharedConstants;
  g_sharedConstantsInitialized = saved.sharedConstantsInitialized;
  std::copy(saved.textures.begin(), saved.textures.end(), std::begin(g_textures));
  std::copy(saved.vertexConstants.begin(), saved.vertexConstants.end(),
            std::begin(g_vertexShaderConstants));
  std::copy(saved.pixelConstants.begin(), saved.pixelConstants.end(),
            std::begin(g_pixelShaderConstants));
  g_scissorTestEnable = saved.scissorTestEnable;
  g_scissorRect = saved.scissorRect;
  std::copy(saved.vertexBufferViews.begin(), saved.vertexBufferViews.end(),
            std::begin(g_vertexBufferViews));
  std::copy(saved.inputSlots.begin(), saved.inputSlots.end(), std::begin(g_inputSlots));
  g_indexBufferView = saved.indexBufferView;
  g_framebuffer = nullptr;
  g_hasBoundPipeline = false;
  g_pendingMsaaResolves.clear();
  g_dirtyStates = DirtyStates(true);
}

// Called once from the Direct3D_CreateDevice hook (Task 5). Prints the live
// values of the fields whose offsets were derived rather than measured, so a
// wrong offset shows up in the log as an implausible number instead of as a
// black screen three tasks later.
void LogGuestDeviceLayout(const GuestDevice* device) {
  if (device == nullptr) {
    REXLOG_ERROR("fm4render: LogGuestDeviceLayout(nullptr)");
    return;
  }
  const auto* bytes = reinterpret_cast<const uint8_t*>(device);
  const auto be32 = [bytes](uint32_t off) {
    return reinterpret_cast<const rex::be<uint32_t>*>(bytes + off)->get();
  };
  REXLOG_INFO("fm4render: device layout sizeof=0x{:X} ring=0x{:08X} token=0x{:08X} flags=0x{:02X}",
              sizeof(GuestDevice), be32(0x2B10), be32(0x2B08), bytes[0x2B3D]);
  REXLOG_INFO("fm4render:   control=0x{:08X} color=0x{:08X} mode=0x{:08X} blend0=0x{:08X}",
              be32(kGuestControlPacketOffset), be32(kGuestColorControlOffset),
              be32(kGuestModeControlOffset), be32(kGuestBlendControl0Offset));
  REXLOG_INFO("fm4render:   decl=0x{:08X} ib=0x{:08X} vb0=0x{:08X} stride0={}",
              device->vertexDeclaration.get(), device->boundIndexBuffer.get(),
              device->boundVertexStreams[0].get(), device->streamStrideDwords[0] * 4u);
  const rex::be<float>* vp = GuestViewportFloats(device);
  if (vp != nullptr) {
    REXLOG_INFO("fm4render:   viewport {} {} {}x{} z[{} {}] scissor={}", vp[0].get(), vp[1].get(),
                vp[2].get(), vp[3].get(), vp[4].get(), vp[5].get(), GuestScissorEnable(device));
  } else {
    REXLOG_WARN("fm4render: guest viewport offset unknown; draws with no explicit SetViewport use the full render target");
  }
  if (kGuestScissorEnableOffset == 0) {
    REXLOG_WARN("fm4render: guest scissor-enable offset unknown; scissor test always off");
  }
}

}  // namespace fm4::render
