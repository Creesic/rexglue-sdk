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
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
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
#include "render/render_state.h"
#include "render/shaders/placeholder_ps.hlsl.dxil.h"

// Spec-constant bits (XenosRecomp shared ABI -- must match the offline
// shader-translation tool's bit layout exactly).
#define SPEC_CONSTANT_ALPHA_TEST (1 << 1)
#define SPEC_CONSTANT_ALPHA_TO_COVERAGE (1 << 3)
#define SPEC_CONSTANT_REVERSE_Z (1 << 4)

using namespace plume;

namespace fm2::render {

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

// ---------------------------------------------------------------------------
// Constant/vertex upload allocator (Phase 4). Present() fully drains the GPU
// (waitForCommandFence) before the next frame's BeginRenderStateFrame(), so a
// single persistently-mapped upload buffer reset once per frame is safe with
// no extra fencing -- by the time it's reused, every draw that read from it
// has long finished.
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
    if (offset_ + size > kBufferSize) return RenderBufferReference{};

    uint8_t* dst = mapped_ + offset_;
    if (byteSwap) {
      const uint32_t* s = reinterpret_cast<const uint32_t*>(src);
      uint32_t* d = reinterpret_cast<uint32_t*>(dst);
      for (uint64_t i = 0; i < size / sizeof(uint32_t); ++i) d[i] = std::byteswap(s[i]);
    } else {
      std::memcpy(dst, src, size);
    }
    RenderBufferReference ref = buffer_->at(offset_);
    offset_ += size;
    return ref;
  }

  void UploadAndBindRootDescriptor(const void* src, uint64_t size, uint32_t rootIndex, bool byteSwap) {
    RenderBufferReference ref = Upload(src, size, byteSwap);
    if (ref.ref == nullptr) return;
    CommandList()->setGraphicsRootDescriptor(ref, rootIndex);
  }

 private:
  static constexpr uint64_t kBufferSize = 4 * 1024 * 1024;
  static constexpr uint64_t kAlignment = 256;

  void EnsureCreated() {
    if (buffer_ != nullptr) return;
    // Used both as a root CBV source (FlushRenderState) and as a scratch
    // vertex buffer (DrawUserPointerVertices) -- flag for both usages.
    buffer_ = Device()->createBuffer(
        RenderBufferDesc::UploadBuffer(kBufferSize, RenderBufferFlag::CONSTANT | RenderBufferFlag::VERTEX));
    mapped_ = reinterpret_cast<uint8_t*>(buffer_->map());
  }

  std::unique_ptr<RenderBuffer> buffer_;
  uint8_t* mapped_ = nullptr;
  uint64_t offset_ = 0;
};
UploadAllocator g_uploadAllocator;

// Pending resource-layout transitions, flushed once per state-changing call.
std::unordered_map<RenderTexture*, RenderTextureLayout> g_barrierMap;
std::vector<RenderTextureBarrier> g_barriers;
std::unordered_set<RenderTexture*> g_initializedAttachments;

void AddBarrier(GuestBaseTexture* texture, RenderTextureLayout layout) {
  if (texture == nullptr || texture->texture == nullptr) return;
  if (texture->layout == layout) return;
  g_barrierMap[texture->texture] = layout;
  texture->layout = layout;
}

void FlushBarriers() {
  if (g_barrierMap.empty()) return;
  g_barriers.clear();
  for (auto& [tex, layout] : g_barrierMap) {
    g_barriers.emplace_back(tex, layout);
  }
  CommandList()->barriers(RenderBarrierStage::GRAPHICS, g_barriers.data(),
                          uint32_t(g_barriers.size()));
  g_barrierMap.clear();
}

void MarkAttachmentInitialized(GuestBaseTexture* texture) {
  if (texture == nullptr || texture->texture == nullptr) return;
  g_initializedAttachments.insert(texture->texture);
  texture->hostInitialized = true;
}

RenderSampleCounts GetSampleCount(GuestBaseTexture* texture) {
  if (texture != nullptr &&
      (texture->type == ResourceType::RenderTarget || texture->type == ResourceType::DepthStencil)) {
    return static_cast<GuestSurface*>(texture)->sampleCount;
  }
  return RenderSampleCount::COUNT_1;
}

void EnsureShaderResourceDescriptor(GuestBaseTexture* texture) {
  if (texture == nullptr || texture->texture == nullptr) return;
  if (texture->descriptorIndex == 0) texture->descriptorIndex = AllocTextureDescriptor();
  TextureDescriptorSet()->setTexture(texture->descriptorIndex, texture->texture,
                                     RenderTextureLayout::SHADER_READ, texture->textureView.get());
}

void BindTextureDescriptor(uint32_t index, GuestBaseTexture* texture,
                           RenderTextureViewDimension viewDimension) {
  AddBarrier(texture, RenderTextureLayout::SHADER_READ);
  EnsureShaderResourceDescriptor(texture);
  g_sharedConstants.texture2DIndices[index] =
      (texture && viewDimension == RenderTextureViewDimension::TEXTURE_2D) ? texture->descriptorIndex
                                                                            : kNullTexture2DDescriptor;
  g_sharedConstants.texture3DIndices[index] =
      (texture && viewDimension == RenderTextureViewDimension::TEXTURE_3D) ? texture->descriptorIndex
                                                                            : kNullTexture3DDescriptor;
  g_sharedConstants.textureCubeIndices[index] =
      (texture && viewDimension == RenderTextureViewDimension::TEXTURE_CUBE) ? texture->descriptorIndex
                                                                              : kNullTextureCubeDescriptor;
}

RenderComparisonFunction ConvertCmpFunc(uint32_t v) {
  switch (v) {
    case D3DCMP_NEVER: return RenderComparisonFunction::NEVER;
    case D3DCMP_LESS: return RenderComparisonFunction::LESS;
    case D3DCMP_EQUAL: return RenderComparisonFunction::EQUAL;
    case D3DCMP_LESSEQUAL: return RenderComparisonFunction::LESS_EQUAL;
    case D3DCMP_GREATER: return RenderComparisonFunction::GREATER;
    case D3DCMP_NOTEQUAL: return RenderComparisonFunction::NOT_EQUAL;
    case D3DCMP_GREATEREQUAL: return RenderComparisonFunction::GREATER_EQUAL;
    case D3DCMP_ALWAYS: return RenderComparisonFunction::ALWAYS;
    default: return RenderComparisonFunction::ALWAYS;
  }
}

RenderBlend ConvertBlendMode(uint32_t v) {
  switch (v) {
    case D3DBLEND_ZERO: return RenderBlend::ZERO;
    case D3DBLEND_ONE: return RenderBlend::ONE;
    case D3DBLEND_SRCCOLOR: return RenderBlend::SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR: return RenderBlend::INV_SRC_COLOR;
    case D3DBLEND_SRCALPHA: return RenderBlend::SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA: return RenderBlend::INV_SRC_ALPHA;
    case D3DBLEND_DESTCOLOR: return RenderBlend::DEST_COLOR;
    case D3DBLEND_INVDESTCOLOR: return RenderBlend::INV_DEST_COLOR;
    case D3DBLEND_DESTALPHA: return RenderBlend::DEST_ALPHA;
    case D3DBLEND_INVDESTALPHA: return RenderBlend::INV_DEST_ALPHA;
    default: return RenderBlend::ONE;
  }
}

RenderBlendOperation ConvertBlendOp(uint32_t v) {
  switch (v) {
    case D3DBLENDOP_ADD: return RenderBlendOperation::ADD;
    case D3DBLENDOP_SUBTRACT: return RenderBlendOperation::SUBTRACT;
    case D3DBLENDOP_MIN: return RenderBlendOperation::MIN;
    case D3DBLENDOP_MAX: return RenderBlendOperation::MAX;
    case D3DBLENDOP_REVSUBTRACT: return RenderBlendOperation::REV_SUBTRACT;
    default: return RenderBlendOperation::ADD;
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
  return reinterpret_cast<GuestClipPlane*>(reinterpret_cast<uint8_t*>(device) + kGuestClipPlanesOffset);
}

uint32_t ClipPlaneEnableMask(GuestDevice* device) {
  auto* value =
      reinterpret_cast<rex::be<uint32_t>*>(reinterpret_cast<uint8_t*>(device) + kGuestClipPlaneEnableOffset);
  return value->get() & kGuestClipPlaneMask;
}

bool ScissorTestEnabled(GuestDevice* device) {
  auto* value =
      reinterpret_cast<rex::be<uint32_t>*>(reinterpret_cast<uint8_t*>(device) + kGuestScissorEnableOffset);
  return value->get() != 0;
}

// Simplified framebuffer cache: just caches one RenderFramebuffer per unique
// (color, depth) attachment pair. The reference's version also had an
// EDRAM-tile-resize heuristic bolted on for the still-unresolved
// predicated-tiling replay path; that's Phase 4/draw-dispatch territory, not
// simple state tracking, so it's not here.
void SetFramebuffer(GuestBaseTexture* colorTarget, GuestSurface* depthTarget, bool /*forClear*/) {
  const GuestBaseTexture* dimensionSource = colorTarget != nullptr ? colorTarget : depthTarget;
  if (dimensionSource != nullptr && dimensionSource->width != 0 && dimensionSource->height != 0) {
    g_sharedConstants.halfPixelOffsetX = 1.0f / float(dimensionSource->width);
    g_sharedConstants.halfPixelOffsetY = -1.0f / float(dimensionSource->height);
  }

  const uint64_t key = (uint64_t(uintptr_t(colorTarget)) << 32) ^ uint64_t(uintptr_t(depthTarget));
  auto it = g_framebufferCache.find(key);
  if (it != g_framebufferCache.end()) {
    g_framebuffer = it->second.get();
  } else {
    const RenderTexture* colorTex = colorTarget != nullptr ? colorTarget->texture : nullptr;
    const RenderTexture* depthTex = depthTarget != nullptr ? depthTarget->texture : nullptr;
    RenderFramebufferDesc desc(colorTex != nullptr ? &colorTex : nullptr, colorTex != nullptr ? 1u : 0u);
    desc.depthAttachment = depthTex;
    auto fb = Device()->createFramebuffer(desc);
    g_framebuffer = fb.get();
    g_framebufferCache.emplace(key, std::move(fb));
  }
  CommandList()->setFramebuffer(g_framebuffer);
}

}  // namespace

// ---------------------------------------------------------------------------
// Per-frame bookkeeping.
// ---------------------------------------------------------------------------

void BeginRenderStateFrame() {
  ++g_frameIndex;
  g_framebuffer = nullptr;
  g_dirtyStates = DirtyStates(true);
  g_uploadAllocator.Reset();
  if (!g_sharedConstantsInitialized) {
    for (uint32_t i = 0; i < std::size(g_sharedConstants.texture2DIndices); ++i) {
      g_sharedConstants.texture2DIndices[i] = kNullTexture2DDescriptor;
      g_sharedConstants.texture3DIndices[i] = kNullTexture3DDescriptor;
      g_sharedConstants.textureCubeIndices[i] = kNullTextureCubeDescriptor;
    }
    g_sharedConstantsInitialized = true;
  }

  RenderCommandList* commandList = CommandList();
  commandList->setGraphicsPipelineLayout(PipelineLayout());
  commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 0);
  commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 1);
  commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 2);
  commandList->setGraphicsDescriptorSet(SamplerDescriptorSet(), 3);
}

uint64_t CurrentFrameIndex() { return g_frameIndex; }

// ---------------------------------------------------------------------------
// Render state.
// ---------------------------------------------------------------------------

void SetRenderState(GuestDevice* /*device*/, uint32_t state, uint32_t value) {
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
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlend, ConvertBlendMode(value));
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
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlendAlpha, ConvertBlendMode(value));
      break;
    case D3DRS_DESTBLENDALPHA:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlendAlpha, ConvertBlendMode(value));
      break;
    case D3DRS_COLORWRITEENABLE:
      SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.colorWriteEnable, value);
      g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
      break;
    default:
      break;
  }
}

void SetViewportEnable(GuestDevice* /*device*/, uint32_t value) {
  // The Xenos ViewportEnable render state maps to PA_CL_CLIP_CNTL.clip_disable.
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthClipEnabled, value != 0);
}

void UpdateClipPlaneConstants(GuestDevice* device) {
  const uint32_t enabledMask = ClipPlaneEnableMask(device);
  g_sharedConstants.clipPlaneEnabled = enabledMask != 0 ? 1 : 0;
  if (enabledMask == 0) return;

  const uint32_t planeIndex = std::countr_zero(enabledMask);
  const GuestClipPlane& plane = ClipPlanes(device)[planeIndex];
  g_sharedConstants.clipPlane[0] = plane.x.get();
  g_sharedConstants.clipPlane[1] = plane.y.get();
  g_sharedConstants.clipPlane[2] = plane.z.get();
  g_sharedConstants.clipPlane[3] = plane.w.get();
}

void SetDepthState(uint32_t zEnable, uint32_t zWriteEnable, uint32_t cmpFunc) {
  const bool ze = zEnable != 0;
  if (g_pipelineState.zEnable != ze) g_dirtyStates.renderTargetAndDepthStencil = true;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zEnable, ze);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable, zWriteEnable != 0);
  // No compare-func flip -- see SetRenderState's D3DRS_ZFUNC case.
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zFunc, ConvertCmpFunc(cmpFunc));
}

void SetStencilState(const GuestStencilState& s) {
  if (g_pipelineState.stencilEnable != s.enable) g_dirtyStates.renderTargetAndDepthStencil = true;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilEnable, s.enable);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontFunc, ConvertCmpFunc(s.frontFunc));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontFail, ConvertStencilOp(s.frontFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontDepthFail,
               ConvertStencilOp(s.frontDepthFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontPass, ConvertStencilOp(s.frontPass));

  const uint32_t backFunc = s.twoSided ? s.backFunc : s.frontFunc;
  const uint32_t backFail = s.twoSided ? s.backFail : s.frontFail;
  const uint32_t backDepthFail = s.twoSided ? s.backDepthFail : s.frontDepthFail;
  const uint32_t backPass = s.twoSided ? s.backPass : s.frontPass;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackFunc, ConvertCmpFunc(backFunc));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackFail, ConvertStencilOp(backFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackDepthFail,
               ConvertStencilOp(backDepthFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackPass, ConvertStencilOp(backPass));

  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilRef, uint8_t(s.ref));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilReadMask, uint8_t(s.readMask));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState, g_pipelineState.stencilWriteMask, uint8_t(s.writeMask));
}

// ---------------------------------------------------------------------------
// Texture binding.
// ---------------------------------------------------------------------------

void SetTexture(GuestDevice* /*device*/, uint32_t index, GuestTexture* texture) {
  BindTextureDescriptor(index, texture,
                        texture ? texture->viewDimension : RenderTextureViewDimension::UNKNOWN);
  g_textures[index] = texture;
}

// ---------------------------------------------------------------------------
// Shader / declaration binding (state tracking only; PSO build is Phase 4).
// ---------------------------------------------------------------------------

void SetVertexShader(GuestDevice* /*device*/, GuestShader* shader) {
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexShader, shader);
}

void SetPixelShader(GuestDevice* /*device*/, GuestShader* shader) {
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.pixelShader, shader);
}

void SetVertexDeclaration(GuestDevice* /*device*/, GuestVertexDeclaration* declaration) {
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexDeclaration, declaration);
}

// ---------------------------------------------------------------------------
// Vertex/index buffer binding.
// ---------------------------------------------------------------------------

void SetStreamSource(GuestDevice* /*device*/, uint32_t index, GuestBuffer* buffer, uint32_t offset,
                     uint32_t stride) {
  if (index >= 16u) return;

  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[index],
               uint8_t(buffer ? stride : 0));

  bool dirty = false;
  SetDirtyValue(dirty, g_vertexBufferViews[index].buffer,
               buffer ? buffer->buffer->at(offset) : RenderBufferReference{});
  SetDirtyValue(dirty, g_vertexBufferViews[index].size, buffer ? (buffer->dataSize - offset) : 0u);
  SetDirtyValue(dirty, g_inputSlots[index].stride, buffer ? stride : 0u);
  if (dirty) {
    g_dirtyStates.vertexStreamFirst = std::min<uint8_t>(g_dirtyStates.vertexStreamFirst, uint8_t(index));
    g_dirtyStates.vertexStreamLast = std::max<uint8_t>(g_dirtyStates.vertexStreamLast, uint8_t(index));
  }
}

void SetIndices(GuestDevice* /*device*/, GuestBuffer* buffer) {
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer,
               buffer ? buffer->buffer->at(0) : RenderBufferReference{});
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format,
               buffer ? buffer->format : RenderFormat::R16_UINT);
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, buffer ? buffer->dataSize : 0u);
}

// ---------------------------------------------------------------------------
// Viewport / scissor.
// ---------------------------------------------------------------------------

void SetViewport(GuestDevice* /*device*/, GuestViewport* viewport) {
  // D3D9 validation: a zero-sized viewport is INVALIDCALL and leaves state
  // unchanged.
  if (viewport->width.get() == 0 || viewport->height.get() == 0) return;
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.x, float(viewport->x.get()));
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.y, float(viewport->y.get()));
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.width, float(viewport->width.get()));
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.height, float(viewport->height.get()));
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.minDepth, viewport->minZ.get());
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.maxDepth, viewport->maxZ.get());

  uint32_t specConstants = g_pipelineState.specConstants;
  if (viewport->minZ.get() > viewport->maxZ.get()) {
    specConstants |= SPEC_CONSTANT_REVERSE_Z;
  } else {
    specConstants &= ~uint32_t(SPEC_CONSTANT_REVERSE_Z);
  }
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);

  g_dirtyStates.scissorRect |= g_dirtyStates.viewport;
}

void SetScissorRect(GuestDevice* device, GuestRect* rect) {
  SetDirtyValue(g_dirtyStates.scissorRect, g_scissorTestEnable, ScissorTestEnabled(device));
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.top, rect->top.get());
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.left, rect->left.get());
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.bottom, rect->bottom.get());
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.right, rect->right.get());
}

// ---------------------------------------------------------------------------
// Render target / depth-stencil binding.
// ---------------------------------------------------------------------------

namespace {

void SetRenderTargetInternal(GuestBaseTexture* renderTarget) {
  SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_renderTarget, renderTarget);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.renderTargetFormat,
               renderTarget ? renderTarget->format : RenderFormat::UNKNOWN);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.sampleCount, GetSampleCount(renderTarget));
  SetAlphaTestMode((g_pipelineState.specConstants &
                    (SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE)) != 0);

  // D3D9/Xenon semantics: SetRenderTarget resets the viewport to cover the
  // whole surface.
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

}  // namespace

void SetRenderTarget(GuestDevice* /*device*/, uint32_t index, GuestBaseTexture* renderTarget) {
  if (index != 0) return;  // FM2 only ever uses a single color render target.
  SetRenderTargetInternal(renderTarget ? renderTarget : g_implicitRenderTarget);
}

void SetImplicitRenderTarget(GuestBaseTexture* renderTarget) {
  g_implicitRenderTarget = renderTarget;
  SetRenderTargetInternal(renderTarget);
}

GuestBaseTexture* GetCurrentColorRenderTarget() { return g_renderTarget; }

void PrepareFramePresent() {
  static bool loggedFirstTarget = false;
  static uint64_t callCount = 0;
  ++callCount;
  if (g_renderTarget != nullptr && !loggedFirstTarget) {
    loggedFirstTarget = true;
    REXGPU_INFO("PrepareFramePresent: first non-null render target seen after {} present call(s)", callCount);
  } else if (g_renderTarget == nullptr && callCount % 300 == 0) {
    REXGPU_WARN("PrepareFramePresent: still no render target bound after {} present call(s)", callCount);
  }
  SetPresentSource(g_renderTarget);
}

void SetDepthStencilSurface(GuestDevice* /*device*/, GuestSurface* depthStencil) {
  SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_depthStencil, depthStencil);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthStencilFormat,
               depthStencil ? depthStencil->format : RenderFormat::UNKNOWN);
  g_dirtyStates.viewport = true;
  if (depthStencil != nullptr) g_implicitDepthStencil = depthStencil;
}

// ---------------------------------------------------------------------------
// Clear.
// ---------------------------------------------------------------------------

void Clear(GuestDevice* /*device*/, uint32_t flags, const float* color, float z) {
  AddBarrier(g_renderTarget, RenderTextureLayout::COLOR_WRITE);
  AddBarrier(g_depthStencil, RenderTextureLayout::DEPTH_WRITE);
  FlushBarriers();

  const bool onePass = (g_renderTarget == nullptr) || (g_depthStencil == nullptr) ||
                       (g_renderTarget->width == g_depthStencil->width &&
                        g_renderTarget->height == g_depthStencil->height);
  if (onePass) SetFramebuffer(g_renderTarget, g_depthStencil, true);

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
  if (g_renderTarget != nullptr && (flags & D3DCLEAR_TARGET) != 0) {
    if (!onePass) SetFramebuffer(g_renderTarget, nullptr, true);
    commandList->clearColor(0, RenderColor(color[0], color[1], color[2], color[3]), &clearRect, 1);
    MarkAttachmentInitialized(g_renderTarget);
  }
  const bool clearDepth = (flags & D3DCLEAR_ZBUFFER) != 0;
  const bool clearStencil = (flags & D3DCLEAR_STENCIL) != 0;
  if (g_depthStencil != nullptr && (clearDepth || clearStencil)) {
    if (!onePass) SetFramebuffer(nullptr, g_depthStencil, true);
    // Pass the guest's clear z through unflipped: FM2 is natively reverse-Z
    // (viewport minZ=1/maxZ=0, ZFunc GREATER-family), and SetViewport already
    // preserves that reversed depth range, so no additional flip is needed
    // here for the scheme to be coherent end to end.
    commandList->clearDepthStencil(clearDepth, clearStencil, z, 0, &clearRect, 1);
    MarkAttachmentInitialized(g_depthStencil);
    g_implicitDepthStencil = g_depthStencil;
  }
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
    case D3DDECLTYPE_FLOAT1: return RenderFormat::R32_FLOAT;
    case D3DDECLTYPE_FLOAT2: return RenderFormat::R32G32_FLOAT;
    case D3DDECLTYPE_FLOAT3: return RenderFormat::R32G32B32_FLOAT;
    case D3DDECLTYPE_FLOAT4: return RenderFormat::R32G32B32A32_FLOAT;
    case D3DDECLTYPE_D3DCOLOR: return RenderFormat::B8G8R8A8_UNORM;
    case D3DDECLTYPE_UBYTE4:
    case D3DDECLTYPE_UBYTE4_2: return RenderFormat::R8G8B8A8_UINT;
    case D3DDECLTYPE_SHORT2: return RenderFormat::R16G16_SINT;
    case D3DDECLTYPE_SHORT4: return RenderFormat::R16G16B16A16_SINT;
    case D3DDECLTYPE_UBYTE4N:
    case D3DDECLTYPE_UBYTE4N_2: return RenderFormat::R8G8B8A8_UNORM;
    case D3DDECLTYPE_SHORT2N: return RenderFormat::R16G16_SNORM;
    case D3DDECLTYPE_SHORT4N: return RenderFormat::R16G16B16A16_SNORM;
    case D3DDECLTYPE_USHORT2N: return RenderFormat::R16G16_UNORM;
    case D3DDECLTYPE_USHORT4N: return RenderFormat::R16G16B16A16_UNORM;
    case D3DDECLTYPE_UINT1: return RenderFormat::R32_UINT;
    case D3DDECLTYPE_UDEC3:
    case D3DDECLTYPE_DEC3N:
    case D3DDECLTYPE_DEC3N_2:
    case D3DDECLTYPE_DEC3N_3:
      return RenderFormat::R32_UINT;  // packed 10/10/10/2; the shader bit-unpacks the raw value.
    case D3DDECLTYPE_FLOAT16_2: return RenderFormat::R16G16_FLOAT;
    case D3DDECLTYPE_FLOAT16_4: return RenderFormat::R16G16B16A16_FLOAT;
    default: return RenderFormat::UNKNOWN;
  }
}

// POSITION0 is fetched by the translated shader as a raw bit pattern
// (reinterpreted, never converted) regardless of its declared type -- a
// FLOAT16 position read as a plain bitcast instead of unpacked half-float
// collapses to near-zero instead of the real value.
RenderFormat ConvertPositionDeclType(uint32_t type, bool& outFloat16) {
  outFloat16 = false;
  switch (type) {
    case D3DDECLTYPE_FLOAT1: return RenderFormat::R32_UINT;
    case D3DDECLTYPE_FLOAT2: return RenderFormat::R32G32_UINT;
    case D3DDECLTYPE_FLOAT3: return RenderFormat::R32G32B32_UINT;
    case D3DDECLTYPE_FLOAT4: return RenderFormat::R32G32B32A32_UINT;
    case D3DDECLTYPE_FLOAT16_2:
      outFloat16 = true;
      return RenderFormat::R16G16_UINT;
    case D3DDECLTYPE_FLOAT16_4:
      outFloat16 = true;
      return RenderFormat::R16G16B16A16_UINT;
    default: return ConvertDeclType(type);
  }
}

const char* ConvertDeclUsage(uint8_t usage) {
  switch (usage) {
    case D3DDECLUSAGE_POSITION: return "POSITION";
    case D3DDECLUSAGE_BLENDWEIGHT: return "BLENDWEIGHT";
    case D3DDECLUSAGE_BLENDINDICES: return "BLENDINDICES";
    case D3DDECLUSAGE_NORMAL: return "NORMAL";
    case D3DDECLUSAGE_PSIZE: return "PSIZE";
    case D3DDECLUSAGE_TEXCOORD: return "TEXCOORD";
    case D3DDECLUSAGE_TANGENT: return "TANGENT";
    case D3DDECLUSAGE_BINORMAL: return "BINORMAL";
    case D3DDECLUSAGE_TESSFACTOR: return "TESSFACTOR";
    case D3DDECLUSAGE_POSITIONT: return "POSITIONT";
    case D3DDECLUSAGE_COLOR: return "COLOR";
    case D3DDECLUSAGE_FOG: return "FOG";
    case D3DDECLUSAGE_DEPTH: return "DEPTH";
    case D3DDECLUSAGE_SAMPLE: return "SAMPLE";
    default: return "TEXCOORD";
  }
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
  if (decl == nullptr || decl->inputElements != nullptr) return;

  std::vector<BuiltElement> built;
  built.reserve(size_t(decl->vertexElementCount) + 16);

  for (uint32_t i = 0; i < decl->vertexElementCount; ++i) {
    const GuestVertexElement& e = decl->vertexElements[i];
    if (e.stream == 0xFFu) continue;

    RenderFormat format = ConvertDeclType(e.type);
    if (e.usage == D3DDECLUSAGE_POSITION && e.usageIndex == 0) {
      bool isFloat16 = false;
      format = ConvertPositionDeclType(e.type, isFloat16);
      if (isFloat16) decl->hasFloat16Position = true;
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
    if (format == RenderFormat::UNKNOWN) format = RenderFormat::R32_UINT;

    built.push_back({ConvertDeclUsage(e.usage), e.usageIndex, format, e.stream, e.offset});
    if (e.stream < 16) decl->vertexStreams[e.stream] = true;
  }

  auto has = [&](const char* semantic, uint8_t usageIndex) {
    for (const BuiltElement& b : built) {
      if (b.usageIndex == usageIndex && std::strcmp(b.semantic, semantic) == 0) return true;
    }
    return false;
  };
  auto addDummy = [&](const char* semantic, uint8_t usageIndex, RenderFormat format) {
    if (!has(semantic, usageIndex)) built.push_back({semantic, usageIndex, format, 15, 0});
  };
  addDummy("POSITION", 0, RenderFormat::R32G32B32A32_UINT);
  addDummy("NORMAL", 0, RenderFormat::R32_UINT);
  addDummy("TANGENT", 0, RenderFormat::R32_UINT);
  addDummy("BINORMAL", 0, RenderFormat::R32_UINT);
  for (uint8_t i = 0; i < 8; ++i) addDummy("TEXCOORD", i, RenderFormat::R32_FLOAT);
  addDummy("COLOR", 0, RenderFormat::R32_FLOAT);
  addDummy("COLOR", 1, RenderFormat::R32_FLOAT);
  addDummy("BLENDWEIGHT", 0, RenderFormat::R32_FLOAT);
  addDummy("BLENDINDICES", 0, RenderFormat::R32_UINT);

  decl->inputElementCount = uint32_t(built.size());
  decl->inputElements = std::make_unique<RenderInputElement[]>(built.size());
  for (uint32_t i = 0; i < built.size(); ++i) {
    const BuiltElement& b = built[i];
    decl->inputElements[i] = RenderInputElement(b.semantic, b.usageIndex, i, b.format, b.slot, b.offset);
  }
}

// FM2 never binds a vertex declaration through the device field for real
// (SetActivePassId's write there is a texture/shader pass token, not a
// declaration address -- see d3d_hooks.cpp), so the real input layout has to
// be recovered by matching the bound vertex shader's parsed header
// usage/usageIndex set against every declaration FM2 has ever created,
// picking the tightest-fitting exact-count match. Declarations must be a
// superset of what the shader's header lists (order-independent); among
// those, an exact element-count match wins decisively, with the most tightly
// packed declaration that still fits the bound stream stride as a tiebreak.
GuestVertexDeclaration* MatchDeclarationForShader(GuestShader* vs, uint32_t streamStride) {
  if (vs == nullptr || vs->headerElements.empty()) return nullptr;

  GuestVertexDeclaration* best = nullptr;
  int bestScore = -1;
  for (GuestVertexDeclaration* decl : SnapshotGameDeclarations()) {
    if (decl == nullptr || decl->vertexElements == nullptr || decl->vertexElementCount == 0) continue;

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
    if (!covers) continue;

    uint32_t maxOffset = 0;
    for (uint32_t i = 0; i < decl->vertexElementCount; ++i)
      maxOffset = std::max(maxOffset, uint32_t(decl->vertexElements[i].offset));

    int score = 0;
    if (decl->vertexElementCount == uint32_t(vs->headerElements.size())) score += 100000;
    if (streamStride != 0 && maxOffset < streamStride) score += int(maxOffset);
    if (score > bestScore) {
      bestScore = score;
      best = decl;
    }
  }
  return best;
}

GuestVertexDeclaration* ResolveVertexDeclaration(GuestDevice* device) {
  const uint32_t declAddr = device->vertexDeclaration.get();
  if (declAddr != 0) {
    auto* decl = ghp::ToHost<GuestVertexDeclaration>(declAddr);
    if (IsFm2Resource(decl) && decl->type == ResourceType::VertexDeclaration) return decl;
  }
  return MatchDeclarationForShader(g_pipelineState.vertexShader, g_pipelineState.vertexStrides[0]);
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
  if (!ps.zEnable && !ps.stencilEnable) ps.depthStencilFormat = RenderFormat::UNKNOWN;
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
      if (!ps.vertexDeclaration->vertexStreams[i]) ps.vertexStrides[i] = 0;
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
  static std::unique_ptr<RenderShader> shader =
      Device()->createShader(g_placeholder_ps_dxil, sizeof(g_placeholder_ps_dxil), "main",
                             RenderShaderFormat::DXIL);
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

std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const PipelineState& ps, bool placeholderShader) {
  RenderShader* vertexShader = LoadShader(ps.vertexShader, ps.specConstants);
  if (vertexShader == nullptr) {
    if (ps.vertexShader == nullptr) {
      LogPipelineRejectOnce("no vertex shader bound");
    } else {
      static std::unordered_set<uint64_t> s_loggedShaders;
      const uint64_t hash = ps.vertexShader->shaderCacheEntry != nullptr ? ps.vertexShader->shaderCacheEntry->hash : 0;
      if (s_loggedShaders.insert(hash ^ ps.specConstants).second) {
        REXGPU_WARN(
            "CreateGraphicsPipeline: vertex shader failed to load (hash=0x{:016X} cacheEntry={} "
            "specMask={} specConstants={}) -- this draw will be skipped",
            hash, ps.vertexShader->shaderCacheEntry != nullptr,
            ps.vertexShader->shaderCacheEntry != nullptr ? ps.vertexShader->shaderCacheEntry->spec_constants_mask : 0,
            ps.specConstants);
      }
    }
    return nullptr;
  }

  RenderShader* pixelShader =
      placeholderShader ? GetPlaceholderPixelShader() : LoadShader(ps.pixelShader, ps.specConstants);
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
    if (slot >= 16 || slotSeen[slot]) continue;
    slotSeen[slot] = true;
    slots[slotCount++] =
        RenderInputSlot(slot, ps.vertexStrides[slot], RenderInputSlotClassification::PER_VERTEX_DATA);
  }
  desc.inputSlots = slots;
  desc.inputSlotsCount = slotCount;

  RenderSpecConstant specConstant(0, ps.specConstants);
  if (ps.specConstants != 0) {
    desc.specConstants = &specConstant;
    desc.specConstantsCount = 1;
  }

  return Device()->createGraphicsPipeline(desc);
}

RenderPipeline* GetPipeline(PipelineState ps, bool placeholderShader) {
  SanitizePipelineState(ps);
  if (ps.renderTargetFormat == RenderFormat::UNKNOWN && ps.depthStencilFormat == RenderFormat::UNKNOWN) {
    LogPipelineRejectOnce("no color or depth attachment bound");
    return nullptr;
  }
  if (ps.vertexDeclaration == nullptr) {
    LogPipelineRejectOnce("no vertex declaration resolved (GetPipeline)");
    return nullptr;
  }

  uint64_t hash = XXH3_64bits(&ps, sizeof(ps));
  if (placeholderShader) hash ^= 0x9E3779B97F4A7C15ull;
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
  switch (type) {
    case D3DPT_POINTLIST: return RenderPrimitiveTopology::POINT_LIST;
    case D3DPT_LINELIST: return RenderPrimitiveTopology::LINE_LIST;
    case D3DPT_LINESTRIP: return RenderPrimitiveTopology::LINE_STRIP;
    case D3DPT_TRIANGLELIST: return RenderPrimitiveTopology::TRIANGLE_LIST;
    case D3DPT_TRIANGLEFAN: return RenderPrimitiveTopology::TRIANGLE_FAN;
    case D3DPT_TRIANGLESTRIP: return RenderPrimitiveTopology::TRIANGLE_STRIP;
    default: return RenderPrimitiveTopology::UNKNOWN;  // D3DPT_QUADLIST has no D3D12 equivalent.
  }
}

bool g_insideRecordedBatch = false;
bool g_hasBoundPipeline = false;

}  // namespace

void SetInsideRecordedBatch(bool inside) { g_insideRecordedBatch = inside; }
bool IsInsideRecordedBatch() { return g_insideRecordedBatch; }

bool HasBoundPipeline() { return g_hasBoundPipeline; }

void FlushRenderState(GuestDevice* device, uint32_t primitiveType) {
  g_hasBoundPipeline = false;

  const RenderPrimitiveTopology topology = ConvertPrimitiveType(primitiveType);
  if (topology == RenderPrimitiveTopology::UNKNOWN) {
    static std::unordered_set<uint32_t> s_warnedPrimitiveTypes;
    if (s_warnedPrimitiveTypes.insert(primitiveType).second) {
      REXGPU_WARN("FlushRenderState: unsupported D3DPRIMITIVETYPE {} (e.g. D3DPT_QUADLIST has no D3D12 "
                 "equivalent) -- skipping this draw",
                 primitiveType);
    }
    return;
  }
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.primitiveTopology, topology);

  if (g_dirtyStates.renderTargetAndDepthStencil || g_framebuffer == nullptr) {
    AddBarrier(g_renderTarget, RenderTextureLayout::COLOR_WRITE);
    AddBarrier(g_depthStencil, RenderTextureLayout::DEPTH_WRITE);
    FlushBarriers();
    SetFramebuffer(g_renderTarget, g_depthStencil, false);
    g_dirtyStates.renderTargetAndDepthStencil = false;
  }
  if (g_dirtyStates.viewport) {
    CommandList()->setViewports(g_viewport);
    g_dirtyStates.viewport = false;
  }
  if (g_dirtyStates.scissorRect) {
    RenderRect scissor = g_scissorTestEnable
                             ? g_scissorRect
                             : RenderRect(0, 0, int32_t(g_viewport.x + g_viewport.width),
                                         int32_t(g_viewport.y + g_viewport.height));
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
  CommandList()->setPipeline(pipeline);
  g_dirtyStates.pipelineState = false;

  g_sharedConstants.booleans = (device->vertexShaderBoolConstants[0].get() & 0xFFFFu) |
                               ((device->pixelShaderBoolConstants[0].get() & 0xFFFFu) << 16);
  g_sharedConstants.swappedTexcoords =
      g_pipelineState.vertexDeclaration != nullptr ? g_pipelineState.vertexDeclaration->swappedTexcoords : 0;

  // Constants are byte-swapped straight out of guest memory every draw: the
  // guest writes them directly into the real device struct via its own
  // (deliberately unhooked) constant-setter functions, so there is no
  // reliable CPU-side dirty signal to track here.
  g_uploadAllocator.UploadAndBindRootDescriptor(device->vertexShaderFloatConstants, kVsFloatConstantBytes, 0,
                                                true);
  g_uploadAllocator.UploadAndBindRootDescriptor(device->pixelShaderFloatConstants, kPsFloatConstantBytes, 1,
                                                true);
  g_uploadAllocator.UploadAndBindRootDescriptor(&g_sharedConstants, sizeof(g_sharedConstants), 2, false);

  if (g_dirtyStates.vertexStreamFirst <= g_dirtyStates.vertexStreamLast) {
    const uint8_t first = g_dirtyStates.vertexStreamFirst;
    const uint8_t count = g_dirtyStates.vertexStreamLast - first + 1;
    CommandList()->setVertexBuffers(first, &g_vertexBufferViews[first], count, &g_inputSlots[first]);
    g_dirtyStates.vertexStreamFirst = 15;
    g_dirtyStates.vertexStreamLast = 0;
  }
  if (g_dirtyStates.indices) {
    if (g_indexBufferView.buffer.ref != nullptr) CommandList()->setIndexBuffer(&g_indexBufferView);
    g_dirtyStates.indices = false;
  }

  g_hasBoundPipeline = true;
}

void DrawInstanced(uint32_t vertexCount, uint32_t startVertex) {
  CommandList()->drawInstanced(vertexCount, 1, startVertex, 0);
}

void DrawIndexedInstanced(uint32_t indexCount, uint32_t startIndex, int32_t baseVertexIndex) {
  CommandList()->drawIndexedInstanced(indexCount, 1, startIndex, baseVertexIndex, 0);
}

void DrawUserPointerVertices(GuestDevice* device, uint32_t primitiveType, uint32_t vertexCount,
                             const void* data, uint32_t stride) {
  g_hasBoundPipeline = false;
  if (data == nullptr || vertexCount == 0 || stride == 0) return;

  // The PSO's stream-0 input-slot stride must reflect this draw's stride
  // while the pipeline gets built/looked-up below, but stream 0's *tracked*
  // stride (whatever the last real SetStreamSource bound there) must survive
  // this call unchanged -- restore it immediately after, before this
  // function's caller returns to ordinary real draws.
  const uint8_t savedStride0 = g_pipelineState.vertexStrides[0];
  g_pipelineState.vertexStrides[0] = uint8_t(stride);
  FlushRenderState(device, primitiveType);
  g_pipelineState.vertexStrides[0] = savedStride0;
  if (!g_hasBoundPipeline) return;

  RenderBufferReference ref = g_uploadAllocator.Upload(data, uint64_t(vertexCount) * stride, false);
  if (ref.ref == nullptr) {
    g_hasBoundPipeline = false;
    return;
  }
  RenderVertexBufferView view(ref, vertexCount * stride);
  RenderInputSlot slot(0, stride, RenderInputSlotClassification::PER_VERTEX_DATA);
  CommandList()->setVertexBuffers(0, &view, 1, &slot);
  CommandList()->drawInstanced(vertexCount, 1, 0, 0);

  // This draw's stream-0 bind bypassed the tracked vertex-buffer state (it
  // never went through SetStreamSource) -- force the next real draw to
  // rebind its own buffer there instead of assuming this one is still valid.
  g_dirtyStates.vertexStreamFirst = 0;
}

}  // namespace fm2::render
