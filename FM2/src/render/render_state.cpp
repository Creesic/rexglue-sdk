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
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <plume_render_interface.h>

#include "render/guest_device.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_state.h"

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

// The per-draw constant buffer contents Phase 4 will push to shaders.
struct SharedConstants {
  uint32_t texture2DIndices[16]{};
  uint32_t texture3DIndices[16]{};
  uint32_t textureCubeIndices[16]{};
  float clipPlane[4]{};
  uint32_t clipPlaneEnabled = 0;
  float alphaThreshold = 0.0f;
};

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

}  // namespace fm2::render
