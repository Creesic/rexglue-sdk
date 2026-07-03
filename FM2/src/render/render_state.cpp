#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cfloat>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <plume_render_interface.h>
#include <rex/hash.h> // XXH3_64bits
#include <rex/logging.h>

#include "render/guest_device.h"
#include "render/guest_heap.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_state.h"

// Spec-constant bits (XenosRecomp shared header).
#define SPEC_CONSTANT_R11G11B10_NORMAL (1 << 0)
#define SPEC_CONSTANT_ALPHA_TEST (1 << 1)
#define SPEC_CONSTANT_BICUBIC_GI_FILTER (1 << 2)
#define SPEC_CONSTANT_ALPHA_TO_COVERAGE (1 << 3)
#define SPEC_CONSTANT_REVERSE_Z (1 << 4)
#define SPEC_CONSTANT_UNPACK_UBYTE4_BASIS (1 << 6)

using namespace plume;

namespace fm2::render {

void BindTextureDescriptor(uint32_t index, GuestBaseTexture *texture,
                           RenderTextureViewDimension viewDimension);
void TrackRecentRenderTarget(GuestBaseTexture *rt);
void EnsureShaderResourceDescriptor(GuestBaseTexture *texture);

namespace {

#pragma pack(push, 1)
struct PipelineState {
  GuestShader *vertexShader = nullptr;
  GuestShader *pixelShader = nullptr;
  GuestVertexDeclaration *vertexDeclaration = nullptr;
  bool instancing = false;
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
  uint32_t colorWriteEnable = uint32_t(RenderColorWriteEnable::ALL);
  RenderPrimitiveTopology primitiveTopology =
      RenderPrimitiveTopology::TRIANGLE_LIST;
  uint8_t vertexStrides[16]{};
  RenderFormat renderTargetFormat{};
  RenderFormat depthStencilFormat{};
  RenderSampleCounts sampleCount = RenderSampleCount::COUNT_1;
  bool enableAlphaToCoverage = false;
  bool depthClipEnabled = true;
  uint32_t specConstants = 0;

  bool stencilEnable = false;
  uint8_t stencilReadMask = 0xFF;
  uint8_t stencilWriteMask = 0xFF;
  uint8_t stencilRef = 0;
  RenderComparisonFunction stencilFrontFunc = RenderComparisonFunction::ALWAYS;
  RenderStencilOp stencilFrontFail = RenderStencilOp::KEEP;
  RenderStencilOp stencilFrontDepthFail = RenderStencilOp::KEEP;
  RenderStencilOp stencilFrontPass = RenderStencilOp::KEEP;
  RenderComparisonFunction stencilBackFunc = RenderComparisonFunction::ALWAYS;
  RenderStencilOp stencilBackFail = RenderStencilOp::KEEP;
  RenderStencilOp stencilBackDepthFail = RenderStencilOp::KEEP;
  RenderStencilOp stencilBackPass = RenderStencilOp::KEEP;
};
#pragma pack(pop)
struct SharedConstants {
  uint32_t texture2DIndices[16]{};
  uint32_t texture3DIndices[16]{};
  uint32_t textureCubeIndices[16]{};
  uint32_t samplerIndices[16]{};
  uint32_t booleans{};
  uint32_t swappedTexcoords{};
  uint32_t swappedNormals{};
  uint32_t swappedBinormals{};
  uint32_t swappedTangents{};
  uint32_t swappedBlendWeights{};
  float halfPixelOffsetX{};
  float halfPixelOffsetY{};
  float clipPlane[4]{};
  uint32_t clipPlaneEnabled{};
  float alphaThreshold{};
  uint32_t conditionalSurveyIndex{};
  uint32_t conditionalRenderingIndex{};
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
      : renderTargetAndDepthStencil(value), viewport(value),
        pipelineState(value), scissorRect(value),
        vertexStreamFirst(value ? 0 : 255), vertexStreamLast(value ? 15 : 0),
        indices(value) {}
};

GuestBaseTexture *g_renderTarget;
GuestBaseTexture *g_implicitRenderTarget;
GuestSurface *g_depthStencil;
GuestBaseTexture *g_lastTouchedRenderTarget;
// Diagnostic: render all geometry as wireframe (set true to enable).
bool g_wireframeMode = false;
GuestSurface *g_implicitDepthStencil;
RenderFramebuffer *g_framebuffer;
RenderViewport g_viewport(0.0f, 0.0f, 1280.0f, 720.0f);
PipelineState g_pipelineState;
SharedConstants g_sharedConstants;
GuestTexture *g_textures[16];
bool g_scissorTestEnable = false;
bool g_sharedConstantsInitialized = false;
RenderRect g_scissorRect;
RenderVertexBufferView g_vertexBufferViews[16];
RenderInputSlot g_inputSlots[16];
RenderIndexBufferView g_indexBufferView({}, 0, RenderFormat::R16_UINT);
DirtyStates g_dirtyStates(true);
// Set by FlushRenderState: whether a valid graphics pipeline is bound. Draws
// skip when false (e.g. a guest shader missing from the translated cache).
bool g_pipelineBound = false;

struct GuestClipPlane {
  rex::be<float> x;
  rex::be<float> y;
  rex::be<float> z;
  rex::be<float> w;
};

constexpr size_t kGuestClipPlanesOffset = 0x2820;
constexpr size_t kGuestClipPlaneEnableOffset = 0x2944;
constexpr size_t kGuestScissorEnableOffset = 0x2E48;
constexpr uint32_t kGuestClipPlaneMask = 0x3F;

GuestClipPlane *ClipPlanes(GuestDevice *device) {
  return reinterpret_cast<GuestClipPlane *>(
      reinterpret_cast<uint8_t *>(device) + kGuestClipPlanesOffset);
}

uint32_t ClipPlaneEnableMask(GuestDevice *device) {
  auto *value = reinterpret_cast<rex::be<uint32_t> *>(
      reinterpret_cast<uint8_t *>(device) + kGuestClipPlaneEnableOffset);
  return value->get() & kGuestClipPlaneMask;
}

bool ScissorTestEnabled(GuestDevice *device) {
  auto *value = reinterpret_cast<rex::be<uint32_t> *>(
      reinterpret_cast<uint8_t *>(device) + kGuestScissorEnableOffset);
  return value->get() != 0;
}

std::unordered_map<uint64_t, std::unique_ptr<RenderPipeline>> g_pipelines;
std::unordered_map<uint64_t,
                   std::pair<uint32_t, std::unique_ptr<RenderSampler>>>
    g_samplerStates;
std::unordered_map<GuestShader *, GuestVertexDeclaration *>
    g_vertexShaderDeclarations;
std::unordered_map<RenderTexture *, std::unique_ptr<RenderFramebuffer>>
    g_colorFramebuffers;
std::unordered_set<GuestBaseTexture *> g_pendingStretchRectSurfaces;
std::unordered_set<uint64_t> g_loggedStretchRectFormatMismatches;
std::unique_ptr<GuestVertexDeclaration> g_texturedQuadDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_simpleElementDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_materialVertexDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_dynamicMeshVertexDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_batchedTriangleVertexDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_gpuSkin40VertexDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_screenQuadDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_particleSpriteDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_particleSpriteDynamicDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_particleSubUVDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_particleSubUVDynamicDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_particleBeamTrailDeclaration;
std::unique_ptr<GuestVertexDeclaration> g_particleBeamTrailDynamicDeclaration;

enum class PipelineRejectReason {
  None,
  NoAttachments,
  MissingVertexShader,
  MissingVertexShaderCache,
  MissingHostVertexShader,
  MissingVertexDeclaration,
  CreateFailed,
};

PipelineRejectReason g_lastPipelineRejectReason = PipelineRejectReason::None;

constexpr uint64_t kSimpleElementVertexShaderHash = 0xE5478DE588433B53ull;

bool SceneReverseZ();

bool IsSimpleElementVertexStride(uint32_t vertexStride) {
  return vertexStride == 44 || vertexStride == 48;
}

const char *PipelineRejectReasonName(PipelineRejectReason reason) {
  switch (reason) {
  case PipelineRejectReason::None:
    return "none";
  case PipelineRejectReason::NoAttachments:
    return "no attachments";
  case PipelineRejectReason::MissingVertexShader:
    return "missing vertex shader";
  case PipelineRejectReason::MissingVertexShaderCache:
    return "missing vertex shader cache entry";
  case PipelineRejectReason::MissingHostVertexShader:
    return "failed to create/load host vertex shader";
  case PipelineRejectReason::MissingVertexDeclaration:
    return "missing vertex declaration";
  case PipelineRejectReason::CreateFailed:
    return "createGraphicsPipeline failed";
  }
  return "unknown";
}

// Diagnostic: per-second draw-outcome tally written straight to the FM2 clean
// log. LogDrawSkip's REXGPU_WARN sink does NOT reach C:\temp\fm2-clean.log
// (only REXGPU_INFO does), so route a counted copy through the proven channel
// here. `skipped=false` means the draw reached drawIndexed/drawInstanced.
void DrawOutcomeTally(bool skipped) {
  static std::atomic<uint64_t> s_ok{0};
  static std::atomic<uint64_t> s_okColor{0};
  static std::atomic<uint64_t> s_okPS{0};
  static std::atomic<uint64_t> s_skip{0};
  static std::atomic<uint64_t>
      s_reason[int(PipelineRejectReason::CreateFailed) + 1]{};
  static std::atomic<uint64_t> s_lastSec{0};
  if (skipped) {
    s_skip.fetch_add(1, std::memory_order_relaxed);
    const int r = int(g_lastPipelineRejectReason);
    if (r >= 0 && r < int(std::size(s_reason)))
      s_reason[r].fetch_add(1, std::memory_order_relaxed);
  } else {
    s_ok.fetch_add(1, std::memory_order_relaxed);
    // Split: does this successful draw actually write color, or is it a
    // depth-only prepass draw (colorWrite=0)? If nearly all successes are
    // depth-only, the screen stays at its clear color despite ok>0.
    if (g_pipelineState.colorWriteEnable != 0)
      s_okColor.fetch_add(1, std::memory_order_relaxed);
    // okPS: successful draws that have a pixel shader. The depth prepass has no
    // PS; the forward color pass does. okPS>0 with okColor==0 means color draws
    // ARE succeeding but are wrongly forced depth-only (stale color-write).
    if (g_pipelineState.pixelShader != nullptr)
      s_okPS.fetch_add(1, std::memory_order_relaxed);
    // First N successes: dump the bound RT/DS + formats so we can see whether a
    // real COLOR target is bound (vs only a depth surface, which would explain a
    // depth-buffer-looking present).
    static std::atomic<uint32_t> s_dump{0};
    if (s_dump.fetch_add(1, std::memory_order_relaxed) < 24) {
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(
            f,
            "FM2_OK_STATE rt=%p rtFmt=%d ds=%p dsFmt=%d colorWrite=0x%X "
            "ps=%p\n",
            (void *)g_renderTarget,
            g_renderTarget ? (int)g_renderTarget->format
                           : (int)RenderFormat::UNKNOWN,
            (void *)g_depthStencil,
            g_depthStencil ? (int)g_depthStencil->format
                           : (int)RenderFormat::UNKNOWN,
            g_pipelineState.colorWriteEnable, (void *)g_pipelineState.pixelShader);
        std::fflush(f);
        std::fclose(f);
      }
    }
  }
  using clock = std::chrono::steady_clock;
  const uint64_t nowSec = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          clock::now().time_since_epoch())
          .count());
  uint64_t last = s_lastSec.load(std::memory_order_relaxed);
  if (last == 0) {
    s_lastSec.store(nowSec, std::memory_order_relaxed);
    return;
  }
  if (nowSec == last ||
      !s_lastSec.compare_exchange_strong(last, nowSec,
                                         std::memory_order_relaxed)) {
    return;
  }
  const uint64_t ok = s_ok.exchange(0, std::memory_order_relaxed);
  const uint64_t okColor = s_okColor.exchange(0, std::memory_order_relaxed);
  const uint64_t okPS = s_okPS.exchange(0, std::memory_order_relaxed);
  const uint64_t sk = s_skip.exchange(0, std::memory_order_relaxed);
  const auto take = [&](PipelineRejectReason reason) {
    return (unsigned long long)s_reason[int(reason)].exchange(
        0, std::memory_order_relaxed);
  };
  if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
    std::fprintf(f,
                 "FM2_DRAW_OUTCOME sec=%llu ok=%llu okColor=%llu okPS=%llu "
                 "skip=%llu no_attach=%llu "
                 "missing_vs=%llu missing_vs_cache=%llu missing_host_vs=%llu "
                 "missing_decl=%llu create_fail=%llu\n",
                 (unsigned long long)nowSec, (unsigned long long)ok,
                 (unsigned long long)okColor, (unsigned long long)okPS,
                 (unsigned long long)sk,
                 take(PipelineRejectReason::NoAttachments),
                 take(PipelineRejectReason::MissingVertexShader),
                 take(PipelineRejectReason::MissingVertexShaderCache),
                 take(PipelineRejectReason::MissingHostVertexShader),
                 take(PipelineRejectReason::MissingVertexDeclaration),
                 take(PipelineRejectReason::CreateFailed));
    std::fflush(f);
    std::fclose(f);
  }
}

void LogDrawSkip(const char *drawName, uint32_t primitiveType, uint32_t count) {
  DrawOutcomeTally(/*skipped=*/true);
  // Decisive skip-state snapshot to the proven clean-log channel (REXGPU_WARN
  // below does not reach C:\temp\fm2-clean.log). Reports the pre-sanitize
  // pipeline formats + gating flags so we can tell *why* attachments dropped:
  // colorWrite=0 (RT dropped), z/stencil off (DS dropped), or UNKNOWN surface
  // format. First 24 only, to avoid flooding.
  {
    static std::atomic<uint32_t> s_snap{0};
    if (s_snap.fetch_add(1, std::memory_order_relaxed) < 24) {
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(
            f,
            "FM2_DRAW_SKIP_STATE %s reason=%s colorWrite=0x%X zEnable=%d "
            "stencilEnable=%d rtFmt=%d dsFmt=%d rt=%p rtFmtTex=%d ds=%p\n",
            drawName, PipelineRejectReasonName(g_lastPipelineRejectReason),
            g_pipelineState.colorWriteEnable, (int)g_pipelineState.zEnable,
            (int)g_pipelineState.stencilEnable,
            (int)g_pipelineState.renderTargetFormat,
            (int)g_pipelineState.depthStencilFormat, (void *)g_renderTarget,
            g_renderTarget ? (int)g_renderTarget->format
                           : (int)RenderFormat::UNKNOWN,
            (void *)g_depthStencil);
        std::fflush(f);
        std::fclose(f);
      }
    }
  }
  static uint32_t s_total = 0;
  static uint32_t s_perReason[int(PipelineRejectReason::CreateFailed) + 1] = {};
  int reasonIdx = int(g_lastPipelineRejectReason);
  bool inBounds =
      reasonIdx >= 0 && reasonIdx < int(std::size(s_perReason));
  uint32_t reasonCount = inBounds ? s_perReason[reasonIdx]++ : 0;
  uint32_t total = s_total++;

  // Log first 8 of each reject reason individually so no reason is silently swamped.
  if (reasonCount < 8) {
    REXGPU_WARN("{} skipped: reason={} prim={} count={} vs={} vsHash=0x{:016X} "
                "ps={} psHash=0x{:016X} "
                "decl={} rt={} ds={} colorWrite=0x{:X} zEnable={}",
                drawName, PipelineRejectReasonName(g_lastPipelineRejectReason),
                primitiveType, count, (const void *)g_pipelineState.vertexShader,
                g_pipelineState.vertexShader &&
                        g_pipelineState.vertexShader->shaderCacheEntry
                    ? g_pipelineState.vertexShader->shaderCacheEntry->hash
                    : 0,
                (const void *)g_pipelineState.pixelShader,
                g_pipelineState.pixelShader &&
                        g_pipelineState.pixelShader->shaderCacheEntry
                    ? g_pipelineState.pixelShader->shaderCacheEntry->hash
                    : 0,
                (const void *)g_pipelineState.vertexDeclaration,
                (const void *)g_renderTarget, (const void *)g_depthStencil,
                g_pipelineState.colorWriteEnable, g_pipelineState.zEnable);
  }
  // Periodic summary so we can see cumulative counts without flooding.
  if ((total & 63) == 63) {
    REXGPU_WARN("draw skip summary (total={}): no_attach={} missing_vs={} "
                "missing_vs_cache={} missing_host_vs={} missing_decl={} "
                "create_fail={}",
                total + 1,
                s_perReason[int(PipelineRejectReason::NoAttachments)],
                s_perReason[int(PipelineRejectReason::MissingVertexShader)],
                s_perReason[int(PipelineRejectReason::MissingVertexShaderCache)],
                s_perReason[int(PipelineRejectReason::MissingHostVertexShader)],
                s_perReason[int(PipelineRejectReason::MissingVertexDeclaration)],
                s_perReason[int(PipelineRejectReason::CreateFailed)]);
  }
}

const char *KnownDeclarationName(GuestVertexDeclaration *declaration) {
  if (declaration == nullptr)
    return "null";
  if (g_simpleElementDeclaration &&
      declaration == g_simpleElementDeclaration.get())
    return "SimpleElement";
  if (g_texturedQuadDeclaration &&
      declaration == g_texturedQuadDeclaration.get())
    return "TexturedQuad";
  if (g_materialVertexDeclaration &&
      declaration == g_materialVertexDeclaration.get())
    return "MaterialTile";
  if (g_dynamicMeshVertexDeclaration &&
      declaration == g_dynamicMeshVertexDeclaration.get())
    return "DynamicMesh";
  if (g_batchedTriangleVertexDeclaration &&
      declaration == g_batchedTriangleVertexDeclaration.get())
    return "BatchedTriangle";
  if (g_gpuSkin40VertexDeclaration &&
      declaration == g_gpuSkin40VertexDeclaration.get())
    return "GpuSkin40";
  if (g_screenQuadDeclaration && declaration == g_screenQuadDeclaration.get())
    return "ScreenQuad";
  if (g_particleSpriteDeclaration &&
      declaration == g_particleSpriteDeclaration.get())
    return "ParticleSprite";
  if (g_particleSpriteDynamicDeclaration &&
      declaration == g_particleSpriteDynamicDeclaration.get())
    return "ParticleSpriteDynamic";
  if (g_particleSubUVDeclaration &&
      declaration == g_particleSubUVDeclaration.get())
    return "ParticleSubUV";
  if (g_particleSubUVDynamicDeclaration &&
      declaration == g_particleSubUVDynamicDeclaration.get())
    return "ParticleSubUVDynamic";
  if (g_particleBeamTrailDeclaration &&
      declaration == g_particleBeamTrailDeclaration.get())
    return "ParticleBeamTrail";
  if (g_particleBeamTrailDynamicDeclaration &&
      declaration == g_particleBeamTrailDynamicDeclaration.get())
    return "ParticleBeamTrailDynamic";
  return "Guest";
}

uint64_t ShaderHash(GuestShader *shader) {
  return shader && shader->shaderCacheEntry ? shader->shaderCacheEntry->hash
                                            : 0;
}

float ReadBigEndianFloat(const uint8_t *data) {
  uint32_t bits;
  std::memcpy(&bits, data, sizeof(bits));
  bits = std::byteswap(bits);
  return std::bit_cast<float>(bits);
}

float ReadDeviceFloatConstant(GuestDevice *device, uint32_t index) {
  if (device == nullptr || index >= std::size(device->vertexShaderFloatConstants))
    return 0.0f;
  const auto *data = reinterpret_cast<const uint8_t *>(
      device->vertexShaderFloatConstants + index);
  return ReadBigEndianFloat(data);
}

uint32_t ReadGuestDeviceU32(GuestDevice *device, size_t offset) {
  if (device == nullptr)
    return 0;
  return reinterpret_cast<const rex::be<uint32_t> *>(
             reinterpret_cast<const uint8_t *>(device) + offset)
      ->get();
}

uint8_t ReadGuestDeviceU8(GuestDevice *device, size_t offset) {
  if (device == nullptr)
    return 0;
  return *(reinterpret_cast<const uint8_t *>(device) + offset);
}

template <typename T> void SetDirtyValue(bool &dirty, T &dest, const T &src);

bool IsShadowIndexedUPDraw(uint32_t primitiveType, uint32_t minVertexIndex,
                           uint32_t numVertices, uint32_t numPrimitives,
                           uint32_t indexStride, const void *vertexData,
                           uint32_t vertexStride) {
  return primitiveType == D3DPT_TRIANGLELIST && minVertexIndex == 0 &&
         numVertices == 8 && numPrimitives == 12 && indexStride == 2 &&
         vertexStride == 12 && vertexData != nullptr;
}

void SyncShadowIndexedUPColorWrite(GuestDevice *device, uint32_t primitiveType,
                                   uint32_t minVertexIndex,
                                   uint32_t numVertices, uint32_t numPrimitives,
                                   uint32_t indexStride, const void *vertexData,
                                   uint32_t vertexStride) {
  if (!IsShadowIndexedUPDraw(primitiveType, minVertexIndex, numVertices,
                             numPrimitives, indexStride, vertexData,
                             vertexStride)) {
    return;
  }

  bool dirty = false;
  SetDirtyValue(dirty, g_pipelineState.colorWriteEnable,
                ReadGuestDeviceU32(device, 11852) & 0xF);
  if (g_pipelineState.colorWriteEnable == 0) {
    SetDirtyValue(dirty, g_pipelineState.alphaBlendEnable, false);
  }
  if (dirty) {
    g_dirtyStates.pipelineState = true;
    g_dirtyStates.renderTargetAndDepthStencil = true;
  }
}

void LogShadowIndexedUPDraw(uint32_t primitiveType, uint32_t minVertexIndex,
                            uint32_t numVertices, uint32_t numPrimitives,
                            uint32_t indexStride, const void *vertexData,
                            uint32_t vertexStride,
                            GuestVertexDeclaration *guestDeclaration,
                            GuestDevice *device, bool pipelineBound) {
  if (!IsShadowIndexedUPDraw(primitiveType, minVertexIndex, numVertices,
                             numPrimitives, indexStride, vertexData,
                             vertexStride)) {
    return;
  }

  static uint32_t s_logs = 0;
  if (s_logs++ >= 64)
    return;

  float v[8][3]{};
  const uint8_t *src = reinterpret_cast<const uint8_t *>(vertexData);
  for (uint32_t i = 0; i < 8; ++i) {
    const uint8_t *vertex = src + i * vertexStride;
    v[i][0] = ReadBigEndianFloat(vertex + 0);
    v[i][1] = ReadBigEndianFloat(vertex + 4);
    v[i][2] = ReadBigEndianFloat(vertex + 8);
  }

  float c[4][4]{};
  for (uint32_t row = 0; row < 4; ++row) {
    for (uint32_t col = 0; col < 4; ++col) {
      c[row][col] = ReadDeviceFloatConstant(device, row * 4 + col);
    }
  }

  float ndcMin[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
  float ndcMax[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
  float wMin = FLT_MAX;
  float wMax = -FLT_MAX;
  float clipSample[2][4]{};
  for (uint32_t i = 0; i < 8; ++i) {
    const float in[4] = {v[i][0], v[i][1], v[i][2], 1.0f};
    float clip[4]{};
    for (uint32_t row = 0; row < 4; ++row)
      for (uint32_t col = 0; col < 4; ++col)
        clip[col] += in[row] * c[row][col];
    if (i == 0 || i == 4)
      std::copy(clip, clip + 4, clipSample[i == 4 ? 1 : 0]);
    wMin = std::min(wMin, clip[3]);
    wMax = std::max(wMax, clip[3]);
    if (clip[3] != 0.0f) {
      for (uint32_t axis = 0; axis < 3; ++axis) {
        const float ndc = clip[axis] / clip[3];
        ndcMin[axis] = std::min(ndcMin[axis], ndc);
        ndcMax[axis] = std::max(ndcMax[axis], ndc);
      }
    }
  }

  REXGPU_WARN(
      "ShadowIndexedUP trace: phase={} bound={} vsHash=0x{:016X} "
      "psHash=0x{:016X} guestDecl={}({}) selectedDecl={}({}) rt={} {}x{} "
      "rtFmt={} ds={} {}x{} dsFmt={} guestDsFmt=0x{:08X} reverseZ={} "
      "guest10460=0x{:08X} guest10548=0x{:08X} guest10560=0x{:08X} "
      "guest10564=0x{:08X} guest10568=0x{:08X} "
      "guest10562=0x{:02X} guest11852=0x{:08X} "
      "colorWrite=0x{:X} blend={} cull={} depthClip={} zEnable={} "
      "zWrite={} zFunc={} "
      "stencil={} ref={} read=0x{:02X} write=0x{:02X} "
      "front(func={} fail={} zfail={} pass={}) "
      "back(func={} fail={} zfail={} pass={}) viewport=({},{} {}x{} z={}..{})",
      g_pipelineState.pixelShader ? "project" : "mark", pipelineBound,
      ShaderHash(g_pipelineState.vertexShader),
      ShaderHash(g_pipelineState.pixelShader), (const void *)guestDeclaration,
      KnownDeclarationName(guestDeclaration),
      (const void *)g_pipelineState.vertexDeclaration,
      KnownDeclarationName(g_pipelineState.vertexDeclaration),
      (const void *)g_renderTarget, g_renderTarget ? g_renderTarget->width : 0,
      g_renderTarget ? g_renderTarget->height : 0,
      g_renderTarget ? int(g_renderTarget->format) : int(RenderFormat::UNKNOWN),
      (const void *)g_depthStencil, g_depthStencil ? g_depthStencil->width : 0,
      g_depthStencil ? g_depthStencil->height : 0,
      g_depthStencil ? int(g_depthStencil->format) : int(RenderFormat::UNKNOWN),
      g_depthStencil ? g_depthStencil->guestFormat : 0, SceneReverseZ(),
      ReadGuestDeviceU32(device, 10460), ReadGuestDeviceU32(device, 10548),
      ReadGuestDeviceU32(device, 10560), ReadGuestDeviceU32(device, 10564),
      ReadGuestDeviceU32(device, 10568), ReadGuestDeviceU8(device, 10562),
      ReadGuestDeviceU32(device, 11852), g_pipelineState.colorWriteEnable,
      g_pipelineState.alphaBlendEnable, int(g_pipelineState.cullMode),
      g_pipelineState.depthClipEnabled, g_pipelineState.zEnable,
      g_pipelineState.zWriteEnable, int(g_pipelineState.zFunc),
      g_pipelineState.stencilEnable,
      g_pipelineState.stencilRef, g_pipelineState.stencilReadMask,
      g_pipelineState.stencilWriteMask, int(g_pipelineState.stencilFrontFunc),
      int(g_pipelineState.stencilFrontFail),
      int(g_pipelineState.stencilFrontDepthFail),
      int(g_pipelineState.stencilFrontPass),
      int(g_pipelineState.stencilBackFunc),
      int(g_pipelineState.stencilBackFail),
      int(g_pipelineState.stencilBackDepthFail),
      int(g_pipelineState.stencilBackPass), g_viewport.x, g_viewport.y,
      g_viewport.width, g_viewport.height, g_viewport.minDepth,
      g_viewport.maxDepth);
  REXGPU_WARN("ShadowIndexedUP vertices: v0=({:.3f},{:.3f},{:.3f}) "
              "v1=({:.3f},{:.3f},{:.3f}) v2=({:.3f},{:.3f},{:.3f}) "
              "v3=({:.3f},{:.3f},{:.3f})",
              v[0][0], v[0][1], v[0][2], v[1][0], v[1][1], v[1][2], v[2][0],
              v[2][1], v[2][2], v[3][0], v[3][1], v[3][2]);
  REXGPU_WARN("ShadowIndexedUP vertices: v4=({:.3f},{:.3f},{:.3f}) "
              "v5=({:.3f},{:.3f},{:.3f}) v6=({:.3f},{:.3f},{:.3f}) "
              "v7=({:.3f},{:.3f},{:.3f})",
              v[4][0], v[4][1], v[4][2], v[5][0], v[5][1], v[5][2], v[6][0],
              v[6][1], v[6][2], v[7][0], v[7][1], v[7][2]);
  REXGPU_WARN(
      "ShadowIndexedUP VS c0=({:.6g},{:.6g},{:.6g},{:.6g}) "
      "c1=({:.6g},{:.6g},{:.6g},{:.6g}) "
      "c2=({:.6g},{:.6g},{:.6g},{:.6g}) "
      "c3=({:.6g},{:.6g},{:.6g},{:.6g})",
      c[0][0], c[0][1], c[0][2], c[0][3], c[1][0], c[1][1], c[1][2],
      c[1][3], c[2][0], c[2][1], c[2][2], c[2][3], c[3][0], c[3][1],
      c[3][2], c[3][3]);
  REXGPU_WARN(
      "ShadowIndexedUP CPU clip v0=({:.6g},{:.6g},{:.6g},{:.6g}) "
      "v4=({:.6g},{:.6g},{:.6g},{:.6g}) ndcMin=({:.6g},{:.6g},{:.6g}) "
      "ndcMax=({:.6g},{:.6g},{:.6g}) w={}..{}",
      clipSample[0][0], clipSample[0][1], clipSample[0][2],
      clipSample[0][3], clipSample[1][0], clipSample[1][1],
      clipSample[1][2], clipSample[1][3], ndcMin[0], ndcMin[1], ndcMin[2],
      ndcMax[0], ndcMax[1], ndcMax[2], wMin, wMax);
}

void LogSuspiciousIndexedUPDraw(uint32_t primitiveType, uint32_t minVertexIndex,
                                uint32_t numVertices, uint32_t numPrimitives,
                                uint32_t indexStride, uint32_t vertexStride,
                                GuestVertexDeclaration *guestDeclaration,
                                GuestVertexDeclaration *selectedDeclaration) {
  const uint64_t vsHash =
      g_pipelineState.vertexShader &&
              g_pipelineState.vertexShader->shaderCacheEntry
          ? g_pipelineState.vertexShader->shaderCacheEntry->hash
          : 0;
  const uint64_t psHash =
      g_pipelineState.pixelShader &&
              g_pipelineState.pixelShader->shaderCacheEntry
          ? g_pipelineState.pixelShader->shaderCacheEntry->hash
          : 0;

  const bool simpleElementFamily = vsHash == kSimpleElementVertexShaderHash;
  const bool batchedMesh112Family =
      primitiveType == D3DPT_TRIANGLELIST && numPrimitives == 112 &&
      numVertices >= 200 && IsSimpleElementVertexStride(vertexStride);
  if (!simpleElementFamily && !batchedMesh112Family) {
    return;
  }

  static uint32_t s_simpleLogs = 0;
  static uint32_t s_batchedMesh112Logs = 0;
  if (simpleElementFamily && !batchedMesh112Family && s_simpleLogs++ >= 4)
    return;
  if (batchedMesh112Family && s_batchedMesh112Logs++ >= 16)
    return;

  REXGPU_WARN("DrawIndexedPrimitiveUP trace: family={} prim={} minVertex={} "
              "vertices={} "
              "prims={} indexStride={} vertexStride={} guestDecl={}({}) "
              "selectedDecl={}({}) vsHash=0x{:016X} psHash=0x{:016X}",
              batchedMesh112Family ? "BatchedMesh112" : "SimpleElement",
              primitiveType, minVertexIndex, numVertices, numPrimitives,
              indexStride, vertexStride, (const void *)guestDeclaration,
              KnownDeclarationName(guestDeclaration),
              (const void *)selectedDeclaration,
              KnownDeclarationName(selectedDeclaration), vsHash, psHash);
}

void LogSuspiciousIndexedDraw(const char *hookName, uint32_t primitiveType,
                              int32_t baseVertexIndex, uint32_t startIndex,
                              uint32_t indexCount,
                              GuestVertexDeclaration *guestDeclaration,
                              GuestVertexDeclaration *selectedDeclaration) {
  if (primitiveType != D3DPT_TRIANGLELIST)
    return;

  const uint64_t vsHash =
      g_pipelineState.vertexShader &&
              g_pipelineState.vertexShader->shaderCacheEntry
          ? g_pipelineState.vertexShader->shaderCacheEntry->hash
          : 0;
  const uint64_t psHash =
      g_pipelineState.pixelShader &&
              g_pipelineState.pixelShader->shaderCacheEntry
          ? g_pipelineState.pixelShader->shaderCacheEntry->hash
          : 0;

  struct DeclarationFacts {
    bool texcoord12 = false;
    bool texcoord24 = false;
    bool blendIndices = false;
    bool blendWeight = false;
  };

  auto getDeclarationFacts = [](GuestVertexDeclaration *declaration) {
    DeclarationFacts facts;
    if (declaration == nullptr)
      return facts;

    for (uint32_t i = 0; i < declaration->vertexElementCount; ++i) {
      const GuestVertexElement &e = declaration->vertexElements[i];
      if (e.stream == 0xFF || e.type == D3DDECLTYPE_UNUSED)
        break;
      facts.texcoord12 |= e.stream == 0 && e.offset == 12 &&
                          e.type == D3DDECLTYPE_FLOAT2 &&
                          e.usage == D3DDECLUSAGE_TEXCOORD && e.usageIndex == 0;
      facts.texcoord24 |= e.stream == 0 && e.offset == 24 &&
                          e.type == D3DDECLTYPE_FLOAT2 &&
                          e.usage == D3DDECLUSAGE_TEXCOORD && e.usageIndex == 0;
      facts.blendIndices |= e.type == D3DDECLTYPE_UBYTE4 &&
                            e.usage == D3DDECLUSAGE_BLENDINDICES &&
                            e.usageIndex == 0;
      facts.blendWeight |= e.type == D3DDECLTYPE_UBYTE4N &&
                           e.usage == D3DDECLUSAGE_BLENDWEIGHT &&
                           e.usageIndex == 0;
    }
    return facts;
  };

  const DeclarationFacts selectedFacts =
      getDeclarationFacts(selectedDeclaration);
  const bool indexed336Family = indexCount == 336;
  const bool gpuSkin40Candidate =
      g_inputSlots[0].stride == 40 && indexCount > 512 &&
      g_renderTarget != nullptr && g_renderTarget->width == 1280 &&
      g_renderTarget->height == 720 &&
      (!selectedFacts.blendIndices || !selectedFacts.blendWeight ||
       selectedFacts.texcoord12);

  if (!indexed336Family && !gpuSkin40Candidate)
    return;

  static uint32_t s_indexed336Logs = 0;
  static uint32_t s_gpuSkin40Logs = 0;
  if (indexed336Family && !gpuSkin40Candidate && s_indexed336Logs++ >= 32)
    return;
  if (gpuSkin40Candidate) {
    static std::unordered_set<uint64_t> s_gpuSkin40Keys;
    uint64_t key = vsHash ^ std::rotl(psHash, 17) ^
                   (uint64_t(indexCount) << 32) ^
                   (uint64_t(int(g_indexBufferView.format)) << 24) ^
                   (uint64_t(g_inputSlots[0].stride) << 16) ^
                   (reinterpret_cast<uintptr_t>(selectedDeclaration) >> 4);
    if (!s_gpuSkin40Keys.emplace(key).second)
      return;
    if (s_gpuSkin40Logs++ >= 24)
      return;
  }

  REXGPU_WARN(
      "{} trace: family={} prim={} baseVertex={} startIndex={} "
      "indices={} vertexStride={} indexSize={} indexFmt={} guestDecl={}({}) "
      "selectedDecl={}({}) rt={} rtSize={}x{} rtFmt={} viewport=({},{} "
      "{}x{}) vsHash=0x{:016X} psHash=0x{:016X} tex12={} tex24={} "
      "blendIdx={} blendWeight={}",
      hookName, gpuSkin40Candidate ? "GpuSkin40Candidate" : "Indexed336",
      primitiveType, baseVertexIndex, startIndex, indexCount,
      g_inputSlots[0].stride, g_indexBufferView.size,
      int(g_indexBufferView.format), (const void *)guestDeclaration,
      KnownDeclarationName(guestDeclaration), (const void *)selectedDeclaration,
      KnownDeclarationName(selectedDeclaration), (const void *)g_renderTarget,
      g_renderTarget ? g_renderTarget->width : 0,
      g_renderTarget ? g_renderTarget->height : 0,
      g_renderTarget ? int(g_renderTarget->format) : 0, g_viewport.x,
      g_viewport.y, g_viewport.width, g_viewport.height, vsHash, psHash,
      selectedFacts.texcoord12, selectedFacts.texcoord24,
      selectedFacts.blendIndices, selectedFacts.blendWeight);

  static std::unordered_set<GuestVertexDeclaration *> s_loggedDeclarations;
  if (selectedDeclaration == nullptr ||
      !s_loggedDeclarations.emplace(selectedDeclaration).second) {
    return;
  }

  for (uint32_t i = 0; i < selectedDeclaration->vertexElementCount; ++i) {
    const GuestVertexElement &e = selectedDeclaration->vertexElements[i];
    REXGPU_WARN("{} decl[{}]: stream={} offset={} type=0x{:08X} usage={} "
                "usageIndex={}",
                hookName, i, e.stream, e.offset, e.type, e.usage, e.usageIndex);
  }
}

template <typename T> void SetDirtyValue(bool &dirty, T &dest, const T &src) {
  if (dest != src) {
    dest = src;
    dirty = true;
  }
}

// ---------------------------------------------------------------------------
// Transient per-frame upload allocator (CONSTANT|VERTEX|INDEX upload heap).
// ---------------------------------------------------------------------------

struct UploadResult {
  RenderBuffer *buffer;
  uint64_t offset;
  uint8_t *memory;
};

struct UploadAllocator {
  static constexpr uint64_t kBufferSize = 16 * 1024 * 1024;

  struct Buffer {
    std::unique_ptr<RenderBuffer> buffer;
    uint8_t *memory = nullptr;
  };
  std::vector<Buffer> buffers;
  uint32_t index = 0;
  uint64_t offset = 0;

  void reset() {
    index = 0;
    offset = 0;
  }

  UploadResult allocate(uint64_t size, uint64_t alignment) {
    offset = (offset + alignment - 1) & ~(alignment - 1);
    if (offset + size > kBufferSize) {
      ++index;
      offset = 0;
    }
    if (buffers.size() <= index)
      buffers.resize(index + 1);

    Buffer &buf = buffers[index];
    if (buf.buffer == nullptr) {
      buf.buffer = Device()->createBuffer(RenderBufferDesc::UploadBuffer(
          kBufferSize, RenderBufferFlag::CONSTANT | RenderBufferFlag::VERTEX |
                           RenderBufferFlag::INDEX));
      buf.memory = reinterpret_cast<uint8_t *>(buf.buffer->map());
    }
    if (buf.memory == nullptr)
      return {buf.buffer.get(), 0, nullptr};
    uint64_t at = offset;
    offset += size;
    return {buf.buffer.get(), at, buf.memory + at};
  }

  template <bool ByteSwap, typename T>
  UploadResult allocateCopy(const T *src, uint64_t size, uint64_t alignment) {
    UploadResult result = allocate(size, alignment);
    if (result.memory == nullptr)
      return result;
    if constexpr (ByteSwap) {
      T *dst = reinterpret_cast<T *>(result.memory);
      for (uint64_t i = 0; i < size / sizeof(T); ++i)
        dst[i] = std::byteswap(src[i]);
    } else {
      std::memcpy(result.memory, src, size);
    }
    return result;
  }
};

UploadAllocator g_uploadAllocator;

UploadResult UploadGuestVertexData(const void *data, uint32_t size,
                                   uint64_t alignment) {
  // 2026-07-02 guard: force-enqueued deferred render nodes (see the
  // kForceAllNodeEnqueue experiment in d3d_hooks.cpp) reach draws whose
  // vertex data pointer/size can reference unmapped guest memory; the
  // byteswap loop below then AVs (crash at fm2.exe+0x5cab0, twice, both
  // menu approaches). The host must never fault on guest data: validate
  // the range against the guest heaps and fail the upload instead.
  // The pointer may live in EITHER host mapping: the virtual window at
  // virtual_membase() (4GB) or the physical window at physical_membase()
  // (512MB, TranslatePhysical masks & 0x1FFFFFFF) -- PM4 geometry arrives
  // via TranslatePhysical. (First guard version only accepted the virtual
  // window and null-bound every PM4 vertex upload -> full black screen.)
  {
    auto *memory = ghp::GuestMemory();
    const auto *bytes = static_cast<const uint8_t *>(data);
    bool ok = false;
    if (memory != nullptr && size != 0) {
      const uint8_t *pbase = memory->physical_membase();
      const uint8_t *vbase = memory->virtual_membase();
      if (pbase != nullptr && bytes >= pbase &&
          bytes + size <= pbase + 0x20000000ull) {
        const uint32_t phys = static_cast<uint32_t>(bytes - pbase);
        ok = memory->GetPhysicalHeap()->QueryRangeAccess(
                 phys, phys + size - 1u) != rex::memory::PageAccess::kNoAccess;
      } else if (vbase != nullptr && bytes >= vbase &&
                 bytes + size <= vbase + 0x100000000ull) {
        const uint32_t va = memory->HostToGuestVirtual(bytes);
        auto *heap = memory->LookupHeap(va);
        ok = heap != nullptr &&
             heap->QueryRangeAccess(va, va + size - 1u) !=
                 rex::memory::PageAccess::kNoAccess;
      }
    }
    if (!ok) {
      static std::atomic<uint32_t> s_n{0};
      if (s_n.fetch_add(1, std::memory_order_relaxed) < 16u) {
        REXLOG_WARN("FM2_UPLOAD_GUARD rejected guest vertex upload host={} "
                    "size={}",
                    data, size);
      }
      return {};
    }
  }
  UploadResult result = g_uploadAllocator.allocate(size, alignment);
  if (result.memory == nullptr)
    return result;
  const uint8_t *srcBytes = reinterpret_cast<const uint8_t *>(data);
  uint8_t *dstBytes = result.memory;

  const uint32_t dwordCount = size / sizeof(uint32_t);
  const uint32_t *src = reinterpret_cast<const uint32_t *>(srcBytes);
  uint32_t *dst = reinterpret_cast<uint32_t *>(dstBytes);
  for (uint32_t i = 0; i < dwordCount; ++i)
    dst[i] = std::byteswap(src[i]);

  const uint32_t swappedSize = dwordCount * sizeof(uint32_t);
  if (swappedSize != size) {
    std::memcpy(dstBytes + swappedSize, srcBytes + swappedSize,
                size - swappedSize);
  }
  return result;
}

void SetRootDescriptor(const UploadResult &allocation, uint32_t index) {
  if (allocation.memory == nullptr)
    return;
  CommandList()->setGraphicsRootDescriptor(
      allocation.buffer->at(allocation.offset), index);
}

// ---------------------------------------------------------------------------
// Per-frame upload cache for genuine guest D3D vertex/index buffers (UE3 RHI
// builds those headers itself; the data lives in guest physical memory and is
// re-uploaded at most once per frame, keyed on its host pointer).
// ---------------------------------------------------------------------------

uint64_t g_frameIndex = 0;

struct GuestDataUpload {
  uint64_t frame = ~0ull;
  uint32_t size = 0;
  RenderBufferReference ref;
};

std::unordered_map<const void *, GuestDataUpload> g_guestVertexUploads;
std::unordered_map<const void *, GuestDataUpload> g_guestIndexUploads;

// ---------------------------------------------------------------------------
// Barriers
// ---------------------------------------------------------------------------

std::unordered_map<RenderTexture *, RenderTextureLayout> g_barrierMap;
std::vector<RenderTextureBarrier> g_barriers;
std::unordered_set<RenderTexture *> g_initializedAttachments;
std::unordered_set<RenderTexture *> g_pendingAttachmentDiscards;

RenderSampleCounts GetSampleCount(GuestBaseTexture *texture);

void MarkAttachmentInitialized(RenderTexture *texture) {
  if (texture != nullptr)
    g_initializedAttachments.insert(texture);
}

void MarkAttachmentInitialized(GuestBaseTexture *texture) {
  if (texture != nullptr) {
    texture->hostInitialized = true;
    MarkAttachmentInitialized(texture->texture);
  }
}

void QueueAttachmentDiscard(RenderTexture *texture) {
  if (texture == nullptr || g_initializedAttachments.contains(texture))
    return;
  g_initializedAttachments.insert(texture);
  g_pendingAttachmentDiscards.insert(texture);
}

void DiscardIfNeeded(RenderCommandList *commandList, RenderTexture *texture) {
  if (texture == nullptr || g_initializedAttachments.contains(texture))
    return;
  g_initializedAttachments.insert(texture);
  commandList->discardTexture(texture);
}

bool RequiresValidContents(RenderTextureLayout layout) {
  switch (layout) {
  case RenderTextureLayout::COPY_DEST:
  case RenderTextureLayout::RESOLVE_DEST:
    return false;
  default:
    return true;
  }
}

void QueueHostInitializationIfNeeded(GuestBaseTexture *texture,
                                     RenderTextureLayout layout) {
  if (texture == nullptr || texture->texture == nullptr ||
      !texture->requiresHostInitialization || texture->hostInitialized ||
      !RequiresValidContents(layout)) {
    return;
  }
  texture->hostInitialized = true;
  QueueAttachmentDiscard(texture->texture);
}

void AddBarrier(GuestBaseTexture *texture, RenderTextureLayout layout) {
  if (texture != nullptr && texture->texture != nullptr) {
    QueueHostInitializationIfNeeded(texture, layout);
    if (texture->layout == layout)
      return;
    g_barrierMap[texture->texture] = layout;
    texture->layout = layout;
  }
}

void FlushBarriers() {
  if (g_barrierMap.empty() && g_pendingAttachmentDiscards.empty())
    return;
  for (auto &[texture, layout] : g_barrierMap)
    g_barriers.emplace_back(texture, layout);
  RenderCommandList *commandList = CommandList();
  if (!g_barriers.empty()) {
    commandList->barriers(
        RenderBarrierStage::GRAPHICS | RenderBarrierStage::COPY, g_barriers);
  }
  for (RenderTexture *texture : g_pendingAttachmentDiscards)
    commandList->discardTexture(texture);
  g_pendingAttachmentDiscards.clear();
  g_barrierMap.clear();
  g_barriers.clear();
}

bool AddPendingStretchRectBarriers(GuestBaseTexture *surface) {
  if (surface == nullptr || surface->pendingResolves.empty())
    return false;

  RenderSampleCounts sampleCount = GetSampleCount(surface);
  AddBarrier(surface, sampleCount != RenderSampleCount::COUNT_1
                          ? RenderTextureLayout::RESOLVE_SOURCE
                          : RenderTextureLayout::COPY_SOURCE);
  for (const PendingResolve &resolve : surface->pendingResolves) {
    GuestBaseTexture *destination = resolve.destination;
    AddBarrier(destination, sampleCount != RenderSampleCount::COUNT_1
                                ? RenderTextureLayout::RESOLVE_DEST
                                : RenderTextureLayout::COPY_DEST);
  }
  return true;
}

uint32_t BoundTextureSlotMask(GuestBaseTexture *texture) {
  uint32_t mask = 0;
  for (uint32_t i = 0; i < std::size(g_textures); ++i) {
    if (static_cast<GuestBaseTexture *>(g_textures[i]) == texture)
      mask |= 1u << i;
  }
  return mask;
}

void LogStretchRectFormatMismatch(GuestBaseTexture *source,
                                  GuestBaseTexture *destination) {
  const uint64_t key =
      (uint64_t(reinterpret_cast<uintptr_t>(source->texture)) >> 4) ^
      (uint64_t(reinterpret_cast<uintptr_t>(destination->texture)) << 17);
  if (!g_loggedStretchRectFormatMismatches.emplace(key).second)
    return;

  REXGPU_WARN("StretchRect format mismatch: srcTex={} srcDesc={} srcFmt={} "
              "srcSize={}x{} dstTex={} dstDesc={} dstFmt={} dstSize={}x{} "
              "boundSlots=0x{:04X}",
              static_cast<const void *>(source->texture),
              source->descriptorIndex, int(source->format), source->width,
              source->height, static_cast<const void *>(destination->texture),
              destination->descriptorIndex, int(destination->format),
              destination->width, destination->height,
              BoundTextureSlotMask(destination));
}

struct ResolveBlitScratch {
  std::unique_ptr<RenderTexture> texture;
  std::unique_ptr<RenderFramebuffer> framebuffer;
  RenderTextureLayout layout = RenderTextureLayout::UNKNOWN;
};
std::unordered_map<uint64_t, ResolveBlitScratch> g_resolveBlitScratch;

bool BlitFormatConvertedResolve(RenderCommandList *commandList,
                                GuestBaseTexture *source,
                                GuestBaseTexture *destination) {
  RenderPipeline *pipeline = GetBlitPipeline(destination->format);
  if (pipeline == nullptr || source->textureView == nullptr)
    return false;

  const uint64_t key = (uint64_t(destination->format) << 40) |
                       (uint64_t(destination->width) << 20) |
                       uint64_t(destination->height);
  ResolveBlitScratch &scratch = g_resolveBlitScratch[key];
  if (scratch.texture == nullptr) {
    RenderTextureDesc desc;
    desc.dimension = RenderTextureDimension::TEXTURE_2D;
    desc.width = destination->width;
    desc.height = destination->height;
    desc.depth = 1;
    desc.mipLevels = 1;
    desc.arraySize = 1;
    desc.format = destination->format;
    desc.flags = RenderTextureFlag::RENDER_TARGET;
    scratch.texture = Device()->createTexture(desc);
    if (scratch.texture == nullptr) {
      g_resolveBlitScratch.erase(key);
      return false;
    }
    const RenderTexture *color = scratch.texture.get();
    scratch.framebuffer =
        Device()->createFramebuffer(RenderFramebufferDesc(&color, 1));
    REXGPU_INFO("Resolve blit scratch: {}x{} fmt={} (src fmt={})",
                destination->width, destination->height,
                int(destination->format), int(source->format));
  }

  EnsureShaderResourceDescriptor(source);
  AddBarrier(source, RenderTextureLayout::SHADER_READ);
  FlushBarriers();
  if (scratch.layout != RenderTextureLayout::COLOR_WRITE) {
    commandList->barriers(
        RenderBarrierStage::GRAPHICS,
        RenderTextureBarrier(scratch.texture.get(),
                             RenderTextureLayout::COLOR_WRITE));
    scratch.layout = RenderTextureLayout::COLOR_WRITE;
  }
  DiscardIfNeeded(commandList, scratch.texture.get());

  commandList->setPipeline(pipeline);
  commandList->setFramebuffer(scratch.framebuffer.get());
  commandList->setViewports(RenderViewport(
      0.0f, 0.0f, float(destination->width), float(destination->height)));
  commandList->setScissors(
      RenderRect(0, 0, destination->width, destination->height));
  const uint32_t descriptorIndex = source->descriptorIndex;
  commandList->setGraphicsPushConstants(0, &descriptorIndex, 0,
                                        sizeof(descriptorIndex));
  commandList->drawInstanced(3, 1, 0, 0);

  commandList->barriers(RenderBarrierStage::COPY,
                        RenderTextureBarrier(scratch.texture.get(),
                                             RenderTextureLayout::COPY_SOURCE));
  scratch.layout = RenderTextureLayout::COPY_SOURCE;
  commandList->copyTexture(destination->texture, scratch.texture.get());
  MarkAttachmentInitialized(destination);

  // The injected draw clobbered the framebuffer/viewport/scissor bindings;
  // force the next guest flush to rebind them.
  g_framebuffer = nullptr;
  g_dirtyStates.renderTargetAndDepthStencil = true;
  g_dirtyStates.viewport = true;
  g_dirtyStates.scissorRect = true;
  return true;
}

RenderRect FullResolveRect(GuestBaseTexture *source) {
  return RenderRect(0, 0, int32_t(source->width), int32_t(source->height));
}

bool IsFullSurfaceResolve(GuestBaseTexture *source,
                          GuestBaseTexture *destination,
                          const PendingResolve &resolve) {
  if (resolve.destX != 0 || resolve.destY != 0)
    return false;
  if (!resolve.hasSourceRect)
    return source->width == destination->width &&
           source->height == destination->height;
  return resolve.sourceRect == FullResolveRect(source) &&
         source->width == destination->width &&
         source->height == destination->height;
}

bool ClipResolveRegion(GuestBaseTexture *source, GuestBaseTexture *destination,
                       const PendingResolve &resolve, RenderRect &srcRect,
                       uint32_t &dstX, uint32_t &dstY) {
  srcRect =
      resolve.hasSourceRect ? resolve.sourceRect : FullResolveRect(source);
  dstX = resolve.destX;
  dstY = resolve.destY;

  srcRect.left = std::clamp(srcRect.left, int32_t(0), int32_t(source->width));
  srcRect.top = std::clamp(srcRect.top, int32_t(0), int32_t(source->height));
  srcRect.right = std::clamp(srcRect.right, srcRect.left,
                             int32_t(source->width));
  srcRect.bottom = std::clamp(srcRect.bottom, srcRect.top,
                              int32_t(source->height));

  const uint32_t srcWidth = uint32_t(srcRect.right - srcRect.left);
  const uint32_t srcHeight = uint32_t(srcRect.bottom - srcRect.top);
  if (srcWidth == 0 || srcHeight == 0 || dstX >= destination->width ||
      dstY >= destination->height)
    return false;

  const uint32_t copyWidth = std::min(srcWidth, destination->width - dstX);
  const uint32_t copyHeight = std::min(srcHeight, destination->height - dstY);
  srcRect.right = srcRect.left + int32_t(copyWidth);
  srcRect.bottom = srcRect.top + int32_t(copyHeight);
  return copyWidth != 0 && copyHeight != 0;
}

void CompletePendingResolve(GuestBaseTexture *source,
                            GuestBaseTexture *destination) {
  if (destination == nullptr)
    return;
  if (destination->pendingResolveCount > 0)
    --destination->pendingResolveCount;
  if (destination->pendingResolveCount == 0 &&
      destination->sourceTexture == source) {
    destination->sourceTexture = nullptr;
  }
}

void ExecutePendingStretchRects(GuestBaseTexture *surface) {
  if (surface == nullptr || surface->pendingResolves.empty())
    return;

  RenderSampleCounts sampleCount = GetSampleCount(surface);
  RenderCommandList *commandList = CommandList();
  std::vector<PendingResolve> pending = std::move(surface->pendingResolves);
  surface->pendingResolves.clear();
  for (const PendingResolve &resolve : pending) {
    GuestBaseTexture *destination = resolve.destination;
    if (destination == nullptr || destination->texture == nullptr) {
      CompletePendingResolve(surface, destination);
      continue;
    }
    RenderRect srcRect;
    uint32_t dstX = 0;
    uint32_t dstY = 0;
    if (!ClipResolveRegion(surface, destination, resolve, srcRect, dstX,
                           dstY)) {
      CompletePendingResolve(surface, destination);
      continue;
    }

    const bool fullSurface = IsFullSurfaceResolve(surface, destination, resolve);
    // TEMP DIAGNOSTIC 2026-07-02: trace every executed resolve copy (tile-band
    // placement investigation). Remove with the other FM2_* diagnostics.
    {
      static std::atomic<uint32_t> s_n{0};
      if (s_n.fetch_add(1, std::memory_order_relaxed) < 96) {
        if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
          std::fprintf(f,
                       "FM2_STRETCH_EXEC src=%p %ux%u fmt=%d msaa=%d -> dst=%p "
                       "%ux%u fmt=%d dstXY=(%u,%u) srcRect=(%d,%d,%d,%d) "
                       "full=%d\n",
                       static_cast<void *>(surface), surface->width,
                       surface->height, int(surface->format),
                       sampleCount != RenderSampleCount::COUNT_1 ? 1 : 0,
                       static_cast<void *>(destination), destination->width,
                       destination->height, int(destination->format), dstX,
                       dstY, srcRect.left, srcRect.top, srcRect.right,
                       srcRect.bottom, fullSurface ? 1 : 0);
          std::fflush(f);
          std::fclose(f);
        }
      }
    }
    AddBarrier(destination, sampleCount != RenderSampleCount::COUNT_1
                                ? RenderTextureLayout::RESOLVE_DEST
                                : RenderTextureLayout::COPY_DEST);
    FlushBarriers();
    if (sampleCount != RenderSampleCount::COUNT_1) {
      if (fullSurface) {
        commandList->resolveTexture(destination->texture, surface->texture);
      } else {
        commandList->resolveTextureRegion(destination->texture, dstX, dstY,
                                          surface->texture, &srcRect);
      }
      MarkAttachmentInitialized(destination);
    } else if (surface->format != destination->format) {
      if (fullSurface &&
          !BlitFormatConvertedResolve(commandList, surface, destination)) {
        LogStretchRectFormatMismatch(surface, destination);
      } else if (!fullSurface) {
        LogStretchRectFormatMismatch(surface, destination);
      }
    } else {
      AddBarrier(surface, RenderTextureLayout::COPY_SOURCE);
      FlushBarriers();
      if (fullSurface) {
        commandList->copyTexture(destination->texture, surface->texture);
      } else {
        const RenderTextureCopyLocation dst =
            RenderTextureCopyLocation::Subresource(destination->texture, 0);
        const RenderTextureCopyLocation src =
            RenderTextureCopyLocation::Subresource(surface->texture, 0);
        const RenderBox srcBox(srcRect.left, srcRect.top, srcRect.right,
                               srcRect.bottom);
        commandList->copyTextureRegion(dst, src, dstX, dstY, 0, &srcBox);
      }
      MarkAttachmentInitialized(destination);
    }
    AddBarrier(destination, RenderTextureLayout::SHADER_READ);
    CompletePendingResolve(surface, destination);
    for (uint32_t i = 0; i < std::size(g_textures); ++i) {
      if (static_cast<GuestBaseTexture *>(g_textures[i]) == destination) {
        BindTextureDescriptor(i, destination,
                              RenderTextureViewDimension::TEXTURE_2D);
      }
    }
  }
  g_pendingStretchRectSurfaces.erase(surface);
}

void FlushPendingStretchRects(GuestBaseTexture *renderTarget,
                              GuestSurface *depthStencil) {
  std::vector<GuestBaseTexture *> surfaces(g_pendingStretchRectSurfaces.begin(),
                                           g_pendingStretchRectSurfaces.end());
  if (renderTarget != nullptr)
    surfaces.emplace_back(renderTarget);
  if (depthStencil != nullptr)
    surfaces.emplace_back(depthStencil);

  bool addedAny = false;
  for (GuestBaseTexture *surface : surfaces)
    addedAny |= AddPendingStretchRectBarriers(surface);
  if (!addedAny)
    return;

  FlushBarriers();
  for (GuestBaseTexture *surface : surfaces)
    ExecutePendingStretchRects(surface);
  FlushBarriers();
}

// ---------------------------------------------------------------------------
// Enum translation
// ---------------------------------------------------------------------------

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
    return RenderBlend::ZERO;
  }
}

RenderBlendOperation ConvertBlendOp(uint32_t v) {
  switch (v) {
  case D3DBLENDOP_ADD:
    return RenderBlendOperation::ADD;
  case D3DBLENDOP_SUBTRACT:
    return RenderBlendOperation::SUBTRACT;
  case D3DBLENDOP_REVSUBTRACT:
    return RenderBlendOperation::REV_SUBTRACT;
  case D3DBLENDOP_MIN:
    return RenderBlendOperation::MIN;
  case D3DBLENDOP_MAX:
    return RenderBlendOperation::MAX;
  default:
    return RenderBlendOperation::ADD;
  }
}


constexpr uint32_t kD3DFMT_D24FS8 = 0x1A220197u;

bool SceneReverseZ() {
  return g_depthStencil != nullptr &&
         g_depthStencil->guestFormat == kD3DFMT_D24FS8;
}

RenderComparisonFunction FlipCmpFunc(RenderComparisonFunction f) {
  switch (f) {
  case RenderComparisonFunction::LESS:
    return RenderComparisonFunction::GREATER;
  case RenderComparisonFunction::LESS_EQUAL:
    return RenderComparisonFunction::GREATER_EQUAL;
  case RenderComparisonFunction::GREATER:
    return RenderComparisonFunction::LESS;
  case RenderComparisonFunction::GREATER_EQUAL:
    return RenderComparisonFunction::LESS_EQUAL;
  default:
    return f; // NEVER/EQUAL/NOT_EQUAL/ALWAYS are unaffected by Z direction
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
    return RenderComparisonFunction::NEVER;
  }
}

// Xenos RB_DEPTHCONTROL stencil-op fields hold EStencilOp values (1:1 with the
// 360 _D3DSTENCILOP enum, per TranslateStencilOp @ 0x824de838).
RenderStencilOp ConvertStencilOp(uint32_t v) {
  switch (v) {
  case 0: // SO_Keep
    return RenderStencilOp::KEEP;
  case 1: // SO_Zero
    return RenderStencilOp::ZERO;
  case 2: // SO_Replace
    return RenderStencilOp::REPLACE;
  case 3: // SO_SaturatedIncrement
    return RenderStencilOp::INCREMENT_AND_CLAMP;
  case 4: // SO_SaturatedDecrement
    return RenderStencilOp::DECREMENT_AND_CLAMP;
  case 5: // SO_Invert
    return RenderStencilOp::INVERT;
  case 6: // SO_Increment (wrap)
    return RenderStencilOp::INCREMENT_AND_WRAP;
  case 7: // SO_Decrement (wrap)
    return RenderStencilOp::DECREMENT_AND_WRAP;
  default:
    return RenderStencilOp::KEEP;
  }
}

RenderPrimitiveTopology ConvertPrimitiveType(uint32_t v) {
  switch (v) {
  case D3DPT_POINTLIST:
    return RenderPrimitiveTopology::POINT_LIST;
  case D3DPT_LINELIST:
    return RenderPrimitiveTopology::LINE_LIST;
  case D3DPT_LINESTRIP:
    return RenderPrimitiveTopology::LINE_STRIP;
  case D3DPT_TRIANGLELIST:
  case D3DPT_QUADLIST:
    return RenderPrimitiveTopology::TRIANGLE_LIST;
  case D3DPT_TRIANGLESTRIP:
    return RenderPrimitiveTopology::TRIANGLE_STRIP;
  case D3DPT_TRIANGLEFAN:
    return RenderPrimitiveTopology::TRIANGLE_LIST;
  default:
    return RenderPrimitiveTopology::TRIANGLE_LIST;
  }
}

RenderTextureAddressMode ConvertAddressMode(uint32_t v) {
  switch (v) {
  case D3DTADDRESS_WRAP:
    return RenderTextureAddressMode::WRAP;
  case D3DTADDRESS_MIRROR:
    return RenderTextureAddressMode::MIRROR;
  case D3DTADDRESS_CLAMP:
  case 4: // Xenos kClampToHalfway; no exact host equivalent.
    return RenderTextureAddressMode::CLAMP;
  case D3DTADDRESS_MIRRORONCE:
  case 5: // Xenos kMirrorClampToHalfway; no exact host equivalent.
  case 7: // Xenos kMirrorClampToBorder; D3D9/D3D12 normalize to mirror-once.
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
  return v == 1 ? RenderBorderColor::OPAQUE_WHITE
                : RenderBorderColor::TRANSPARENT_BLACK;
}

const char *ConvertDeclUsage(uint32_t usage) {
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
    return "UNKNOWN";
  }
}

RenderFormat ConvertDeclType(uint32_t type) {
  // FM2's D3DVERTEXELEMENT9.Type is the Xbox-360 packed GPUVERTEXFETCHFORMAT:
  //   bits 0-5  = data format (GPUVERTEXFETCHFORMAT)
  //   bits 8-9  = number format (0=UNORM, 1=SNORM, 2=UINT, 3=SINT)
  //   bit  11   = BGRA component swap
  // The canonical D3DDECLTYPE_* constants are only one instance each; real
  // declarations vary in the upper bits, so decode the fields rather than match
  // the whole dword (which previously returned UNKNOWN -> broken input layout).
  const uint32_t fmt = type & 0x3Fu;
  const uint32_t nf = (type >> 8) & 0x3u;
  switch (fmt) {
  case 0x06: // k_8_8_8_8 (UBYTE4 / UBYTE4N / D3DCOLOR)
    if (nf == 2)
      return RenderFormat::R8G8B8A8_UINT;
    return ((type >> 11) & 1u) ? RenderFormat::B8G8R8A8_UNORM
                               : RenderFormat::R8G8B8A8_UNORM;
  case 0x07: // k_2_10_10_10 (DEC3N / UDEC3) -- shader unpacks from raw uint
    return RenderFormat::R32_UINT;
  case 0x10: // k_10_11_11 (packed) -- 32-bit, shader unpacks from raw uint
  case 0x11: // k_11_11_10 (packed) -- 32-bit, shader unpacks from raw uint
    return RenderFormat::R32_UINT;
  case 0x22: // k_32_32 (raw uint2)
    return RenderFormat::R32G32_UINT;
  case 0x23: // k_32_32_32_32 (raw uint4)
    return RenderFormat::R32G32B32A32_UINT;
  case 0x19: // k_16_16 (SHORT2 family)
    return nf == 3   ? RenderFormat::R16G16_SINT
           : nf == 1 ? RenderFormat::R16G16_SNORM
                     : RenderFormat::R16G16_UNORM;
  case 0x1A: // k_16_16_16_16 (SHORT4 family)
    return nf == 3   ? RenderFormat::R16G16B16A16_SINT
           : nf == 1 ? RenderFormat::R16G16B16A16_SNORM
                     : RenderFormat::R16G16B16A16_UNORM;
  case 0x1F: // k_16_16_FLOAT
    return RenderFormat::R16G16_FLOAT;
  case 0x20: // k_16_16_16_16_FLOAT
    return RenderFormat::R16G16B16A16_FLOAT;
  case 0x21: // k_32 (UINT1)
    return RenderFormat::R32_UINT;
  case 0x24: // k_32_FLOAT (FLOAT1)
    return RenderFormat::R32_FLOAT;
  case 0x25: // k_32_32_FLOAT (FLOAT2)
    return RenderFormat::R32G32_FLOAT;
  case 0x26: // k_32_32_32_32_FLOAT (FLOAT4)
    return RenderFormat::R32G32B32A32_FLOAT;
  case 0x39: // k_32_32_32_FLOAT (FLOAT3)
    return RenderFormat::R32G32B32_FLOAT;
  default:
    return RenderFormat::UNKNOWN;
  }
}

RenderFormat ConvertPositionDeclType(uint32_t type) {
  // POSITION0 is read as a raw uint vector by the recompiled shader (which then
  // reinterprets the bits), so the input layout must expose the raw bytes.
  switch (type & 0x3Fu) {
  case 0x24: // FLOAT1 / k_32_FLOAT
  case 0x21: // UINT1 / k_32
    return RenderFormat::R32_UINT;
  case 0x25: // FLOAT2 / k_32_32_FLOAT
    return RenderFormat::R32G32_UINT;
  case 0x39: // FLOAT3 / k_32_32_32_FLOAT
    return RenderFormat::R32G32B32_UINT;
  case 0x26: // FLOAT4 / k_32_32_32_32_FLOAT
    return RenderFormat::R32G32B32A32_UINT;
  case 0x1F: // FLOAT16_2 / k_16_16_FLOAT
    return RenderFormat::R16G16_UINT;
  case 0x20: // FLOAT16_4 / k_16_16_16_16_FLOAT
    return RenderFormat::R16G16B16A16_UINT;
  default:
    return ConvertDeclType(type);
  }
}


struct SemanticLocation {
  uint32_t usage;
  uint32_t usageIndex;
  uint32_t location;
};

constexpr SemanticLocation kSemanticLocations[] = {
    {D3DDECLUSAGE_POSITION, 0, 0},      {D3DDECLUSAGE_NORMAL, 0, 1},
    {D3DDECLUSAGE_TANGENT, 0, 2},       {D3DDECLUSAGE_BINORMAL, 0, 3},
    {D3DDECLUSAGE_POSITION, 1, 4},      {D3DDECLUSAGE_TEXCOORD, 0, 13},
    {D3DDECLUSAGE_TEXCOORD, 1, 14},     {D3DDECLUSAGE_TEXCOORD, 2, 15},
    {D3DDECLUSAGE_TEXCOORD, 3, 16},     {D3DDECLUSAGE_COLOR, 0, 17},
    {D3DDECLUSAGE_COLOR, 1, 18},        {D3DDECLUSAGE_BLENDWEIGHT, 0, 19},
    {D3DDECLUSAGE_BLENDINDICES, 0, 20}, {D3DDECLUSAGE_TEXCOORD, 4, 21},
    {D3DDECLUSAGE_TEXCOORD, 5, 22},     {D3DDECLUSAGE_TEXCOORD, 6, 23},
    {D3DDECLUSAGE_TEXCOORD, 7, 24},
};

uint32_t LookupLocation(uint32_t usage, uint32_t usageIndex) {
  for (const auto &loc : kSemanticLocations)
    if (loc.usage == usage && loc.usageIndex == usageIndex)
      return loc.location;
  return ~0u;
}

void CompleteVertexDeclaration(GuestVertexDeclaration *decl) {
  if (decl->inputElements != nullptr || decl->vertexElements == nullptr)
    return;

  std::vector<RenderInputElement> inputElements;

  for (uint32_t i = 0; i < decl->vertexElementCount; ++i) {
    const GuestVertexElement &e = decl->vertexElements[i];
    if (e.stream == 0xFF || e.type == D3DDECLTYPE_UNUSED)
      break;
    if (e.usage == D3DDECLUSAGE_POSITION && e.usageIndex == 2)
      continue;

    RenderInputElement &ie = inputElements.emplace_back();
    ie.semanticName = ConvertDeclUsage(e.usage);
    ie.semanticIndex = e.usageIndex;
    ie.location = LookupLocation(e.usage, e.usageIndex);
    ie.format = ConvertDeclType(e.type);
    ie.slotIndex = e.stream;
    ie.alignedByteOffset = e.offset;

    switch (e.usage) {
    case D3DDECLUSAGE_POSITION:
      if (e.usageIndex == 0) {
        ie.format = ConvertPositionDeclType(e.type);
      }
      if (e.usageIndex == 1)
        decl->indexVertexStream = e.stream;
      break;
    case D3DDECLUSAGE_NORMAL:
    case D3DDECLUSAGE_TANGENT:
    case D3DDECLUSAGE_BINORMAL:
      if (e.type == D3DDECLTYPE_FLOAT3)
        ie.format = RenderFormat::R32G32B32_UINT;
      else {
        if (e.type == D3DDECLTYPE_UBYTE4 || e.type == D3DDECLTYPE_UBYTE4_2) {
          ie.format = RenderFormat::R8G8B8A8_UNORM;
          decl->hasUByte4TangentBasis = true;
        }
        decl->hasR11G11B10Normal = true;
      }
      break;
    case D3DDECLUSAGE_TEXCOORD:
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
      }
      break;
    case D3DDECLUSAGE_BLENDWEIGHT:
      break;
    }
    // Safety net: an UNKNOWN format makes CreateInputLayout reject the whole PSO
    // (E_INVALIDARG), dropping every draw with this declaration. Expose the raw
    // bytes as uint instead and log it so the missing format can be mapped properly.
    if (ie.format == RenderFormat::UNKNOWN) {
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(f,
                     "FM2_DECL_UNKNOWN_FMT usage=%u idx=%u type=0x%X fmt=0x%X nf=%u "
                     "stream=%u offset=%u\n",
                     static_cast<unsigned>(e.usage), static_cast<unsigned>(e.usageIndex),
                     static_cast<unsigned>(e.type), static_cast<unsigned>(e.type & 0x3Fu),
                     static_cast<unsigned>((e.type >> 8) & 0x3u),
                     static_cast<unsigned>(e.stream), static_cast<unsigned>(e.offset));
        std::fclose(f);
      }
      ie.format = RenderFormat::R32_UINT;
    }
    decl->vertexStreams[e.stream] = true;
  }

  auto addDummyElement = [&](uint32_t usage, uint32_t usageIndex) {
    const uint32_t location = LookupLocation(usage, usageIndex);
    for (const RenderInputElement &ie : inputElements)
      if (ie.location == location)
        return;
    RenderFormat format = RenderFormat::R32_FLOAT;
    switch (usage) {
    case D3DDECLUSAGE_NORMAL:
    case D3DDECLUSAGE_TANGENT:
    case D3DDECLUSAGE_BINORMAL:
    case D3DDECLUSAGE_BLENDINDICES:
      format = RenderFormat::R32_UINT;
      break;
    }
    inputElements.emplace_back(ConvertDeclUsage(usage), usageIndex, location,
                               format, 15, 0);
  };
  addDummyElement(D3DDECLUSAGE_POSITION, 0);
  addDummyElement(D3DDECLUSAGE_NORMAL, 0);
  addDummyElement(D3DDECLUSAGE_TANGENT, 0);
  addDummyElement(D3DDECLUSAGE_BINORMAL, 0);
  for (uint32_t i = 0; i < 8; ++i)
    addDummyElement(D3DDECLUSAGE_TEXCOORD, i);
  addDummyElement(D3DDECLUSAGE_COLOR, 0);
  addDummyElement(D3DDECLUSAGE_COLOR, 1);
  addDummyElement(D3DDECLUSAGE_BLENDWEIGHT, 0);
  addDummyElement(D3DDECLUSAGE_BLENDINDICES, 0);

  decl->inputElementCount = uint32_t(inputElements.size());
  decl->inputElements =
      std::make_unique<RenderInputElement[]>(inputElements.size());
  std::copy(inputElements.begin(), inputElements.end(),
            decl->inputElements.get());
}

// ---------------------------------------------------------------------------
// Pipeline build + cache
// ---------------------------------------------------------------------------

void SanitizePipelineState(PipelineState &ps) {
  if (!ps.zEnable) {
    ps.zWriteEnable = false;
    ps.zFunc = RenderComparisonFunction::LESS;
    ps.slopeScaledDepthBias = 0.0f;
    ps.depthBias = 0;
  }
  if (!ps.stencilEnable) {
    // Canonicalize so depth-only / stencil-off states collapse to one key.
    ps.stencilReadMask = 0xFF;
    ps.stencilWriteMask = 0xFF;
    ps.stencilRef = 0;
    ps.stencilFrontFunc = RenderComparisonFunction::ALWAYS;
    ps.stencilBackFunc = RenderComparisonFunction::ALWAYS;
    ps.stencilFrontFail = ps.stencilFrontDepthFail = ps.stencilFrontPass =
        RenderStencilOp::KEEP;
    ps.stencilBackFail = ps.stencilBackDepthFail = ps.stencilBackPass =
        RenderStencilOp::KEEP;
  }
  // The depth-stencil attachment is needed whenever depth OR stencil is in use;
  // only drop it (and the PSO's DSV) when neither is.
  if (!ps.zEnable && !ps.stencilEnable) {
    ps.depthStencilFormat = RenderFormat::UNKNOWN;
  }
  if (!ps.colorWriteEnable) {
    ps.alphaBlendEnable = false;
    ps.renderTargetFormat = RenderFormat::UNKNOWN;
  }
  if (!ps.alphaBlendEnable) {
    ps.srcBlend = RenderBlend::ONE;
    ps.destBlend = RenderBlend::ZERO;
    ps.blendOp = RenderBlendOperation::ADD;
    ps.srcBlendAlpha = RenderBlend::ONE;
    ps.destBlendAlpha = RenderBlend::ZERO;
    ps.blendOpAlpha = RenderBlendOperation::ADD;
  }

  if (ps.vertexDeclaration != nullptr) {
    for (uint32_t i = 0; i < 16; ++i)
      if (!ps.vertexDeclaration->vertexStreams[i])
        ps.vertexStrides[i] = 0;
  }

  uint32_t mask = 0;
  if (ps.vertexShader && ps.vertexShader->shaderCacheEntry)
    mask |= ps.vertexShader->shaderCacheEntry->spec_constants_mask;
  if (ps.pixelShader && ps.pixelShader->shaderCacheEntry)
    mask |= ps.pixelShader->shaderCacheEntry->spec_constants_mask;
  ps.specConstants &= mask;
}

std::unique_ptr<RenderPipeline>
CreateGraphicsPipeline(const PipelineState &ps) {
  if (ps.vertexShader == nullptr) {
    g_lastPipelineRejectReason = PipelineRejectReason::MissingVertexShader;
    return nullptr;
  }
  if (ps.vertexShader->shaderCacheEntry == nullptr) {
    g_lastPipelineRejectReason = PipelineRejectReason::MissingVertexShaderCache;
    return nullptr;
  }
  RenderShader *vertexShader = LoadShader(ps.vertexShader, ps.specConstants);
  if (vertexShader == nullptr) {
    g_lastPipelineRejectReason = PipelineRejectReason::MissingHostVertexShader;
    return nullptr;
  }
  if (ps.vertexDeclaration == nullptr) {
    g_lastPipelineRejectReason = PipelineRejectReason::MissingVertexDeclaration;
    return nullptr;
  }

  RenderGraphicsPipelineDesc desc;
  desc.pipelineLayout = PipelineLayout();
  desc.vertexShader = vertexShader;
  desc.pixelShader =
      ps.pixelShader ? LoadShader(ps.pixelShader, ps.specConstants) : nullptr;
  // DIAG: a pixel shader that is SET but fails to load yields a depth-only PSO
  // (no PS) -> geometry renders but writes no color -> black. Distinguish the
  // cases per-second so we can see whether the PS host translation is missing.
  {
    static std::atomic<uint64_t> s_noPs{0}, s_psOk{0}, s_psFailNoCache{0},
        s_psFailOther{0}, s_lastSec{0};
    if (ps.pixelShader == nullptr)
      s_noPs.fetch_add(1, std::memory_order_relaxed);
    else if (desc.pixelShader != nullptr)
      s_psOk.fetch_add(1, std::memory_order_relaxed);
    else if (ps.pixelShader->shaderCacheEntry == nullptr)
      s_psFailNoCache.fetch_add(1, std::memory_order_relaxed);
    else
      s_psFailOther.fetch_add(1, std::memory_order_relaxed);
    using clock = std::chrono::steady_clock;
    const uint64_t nowSec = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            clock::now().time_since_epoch())
            .count());
    uint64_t last = s_lastSec.load(std::memory_order_relaxed);
    if (last != 0 && nowSec != last &&
        s_lastSec.compare_exchange_strong(last, nowSec,
                                          std::memory_order_relaxed)) {
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(f,
                     "FM2_PS_LOAD sec=%llu no_ps=%llu ps_ok=%llu "
                     "ps_fail_nocache=%llu ps_fail_other=%llu\n",
                     (unsigned long long)nowSec,
                     (unsigned long long)s_noPs.exchange(0),
                     (unsigned long long)s_psOk.exchange(0),
                     (unsigned long long)s_psFailNoCache.exchange(0),
                     (unsigned long long)s_psFailOther.exchange(0));
        std::fflush(f);
        std::fclose(f);
      }
    } else if (last == 0) {
      s_lastSec.store(nowSec, std::memory_order_relaxed);
    }
  }
  desc.depthFunction = ps.zFunc;
  desc.depthEnabled = ps.zEnable;
  desc.depthWriteEnabled = ps.zWriteEnable;
  desc.depthBias = ps.depthBias;
  desc.slopeScaledDepthBias = ps.slopeScaledDepthBias;
  desc.depthClipEnabled = ps.depthClipEnabled;
  desc.primitiveTopology = ps.primitiveTopology;
  desc.cullMode = ps.cullMode;
  // Diagnostic wireframe: render geometry as edges so we can verify draws/
  // transforms independent of shading/textures. Disable culling too so back
  // faces are visible. Toggle g_wireframeMode (rebuild) to turn off.
  if (g_wireframeMode) {
    desc.fillMode = RenderFillMode::WIREFRAME;
    desc.cullMode = RenderCullMode::NONE;
  }
  desc.renderTargetFormat[0] = ps.renderTargetFormat;
  desc.renderTargetBlend[0].blendEnabled = ps.alphaBlendEnable;
  desc.renderTargetBlend[0].srcBlend = ps.srcBlend;
  desc.renderTargetBlend[0].dstBlend = ps.destBlend;
  desc.renderTargetBlend[0].blendOp = ps.blendOp;
  desc.renderTargetBlend[0].srcBlendAlpha = ps.srcBlendAlpha;
  desc.renderTargetBlend[0].dstBlendAlpha = ps.destBlendAlpha;
  desc.renderTargetBlend[0].blendOpAlpha = ps.blendOpAlpha;
  desc.renderTargetBlend[0].renderTargetWriteMask =
      uint8_t(ps.colorWriteEnable);
  desc.renderTargetCount =
      ps.renderTargetFormat != RenderFormat::UNKNOWN ? 1 : 0;
  desc.depthTargetFormat = ps.depthStencilFormat;
  desc.stencilEnabled = ps.stencilEnable;
  desc.stencilReadMask = ps.stencilReadMask;
  desc.stencilWriteMask = ps.stencilWriteMask;
  desc.stencilReference = ps.stencilRef;
  desc.stencilFrontFace.compareFunction = ps.stencilFrontFunc;
  desc.stencilFrontFace.failOp = ps.stencilFrontFail;
  desc.stencilFrontFace.depthFailOp = ps.stencilFrontDepthFail;
  desc.stencilFrontFace.passOp = ps.stencilFrontPass;
  desc.stencilBackFace.compareFunction = ps.stencilBackFunc;
  desc.stencilBackFace.failOp = ps.stencilBackFail;
  desc.stencilBackFace.depthFailOp = ps.stencilBackDepthFail;
  desc.stencilBackFace.passOp = ps.stencilBackPass;
  desc.multisampling.sampleCount = ps.sampleCount;
  desc.alphaToCoverageEnabled = ps.enableAlphaToCoverage;
  desc.inputElements = ps.vertexDeclaration->inputElements.get();
  desc.inputElementsCount = ps.vertexDeclaration->inputElementCount;

  RenderSpecConstant specConstant{};
  specConstant.value = ps.specConstants;
  if (ps.specConstants != 0) {
    desc.specConstants = &specConstant;
    desc.specConstantsCount = 1;
  }

  RenderInputSlot inputSlots[16]{};
  uint32_t inputSlotIndices[16]{};
  uint32_t inputSlotCount = 0;
  for (uint32_t i = 0; i < ps.vertexDeclaration->inputElementCount; ++i) {
    const RenderInputElement &ie = ps.vertexDeclaration->inputElements[i];
    uint32_t &slotIndex = inputSlotIndices[ie.slotIndex];
    if (slotIndex == 0)
      slotIndex = ++inputSlotCount;

    RenderInputSlot &slot = inputSlots[slotIndex - 1];
    slot.index = ie.slotIndex;
    slot.stride = ps.vertexStrides[ie.slotIndex];
    slot.classification =
        (ps.instancing && ie.slotIndex != 0 && ie.slotIndex != 15)
            ? RenderInputSlotClassification::PER_INSTANCE_DATA
            : RenderInputSlotClassification::PER_VERTEX_DATA;
  }
  desc.inputSlots = inputSlots;
  desc.inputSlotsCount = inputSlotCount;

  auto pipeline = Device()->createGraphicsPipeline(desc);
  if (pipeline == nullptr)
    g_lastPipelineRejectReason = PipelineRejectReason::CreateFailed;
  return pipeline;
}

RenderPipeline *GetPipeline(PipelineState ps) {
  SanitizePipelineState(ps);
  g_lastPipelineRejectReason = PipelineRejectReason::None;
  if (ps.renderTargetFormat == RenderFormat::UNKNOWN &&
      ps.depthStencilFormat == RenderFormat::UNKNOWN) {
    g_lastPipelineRejectReason = PipelineRejectReason::NoAttachments;
    return nullptr;
  }
  uint64_t hash = XXH3_64bits(&ps, sizeof(ps));
  auto &pipeline = g_pipelines[hash];
  if (pipeline == nullptr)
    pipeline = CreateGraphicsPipeline(ps);
  return pipeline.get();
}

// ---------------------------------------------------------------------------
// Framebuffer + viewport
// ---------------------------------------------------------------------------

RenderSampleCounts GetSampleCount(GuestBaseTexture *texture) {
  if (texture != nullptr && (texture->type == ResourceType::RenderTarget ||
                             texture->type == ResourceType::DepthStencil))
    return static_cast<GuestSurface *>(texture)->sampleCount;
  return RenderSampleCount::COUNT_1;
}


struct OversizedColorScratch {
  std::unique_ptr<RenderTexture> texture;
};
std::unordered_map<uint64_t, OversizedColorScratch> g_oversizedColorScratch;

RenderTexture *GetOversizedDepthColorTarget(uint32_t width, uint32_t height,
                                            RenderFormat format) {
  const uint64_t key =
      (uint64_t(format) << 40) | (uint64_t(width) << 20) | uint64_t(height);
  OversizedColorScratch &scratch = g_oversizedColorScratch[key];
  if (scratch.texture == nullptr) {
    RenderTextureDesc desc;
    desc.dimension = RenderTextureDimension::TEXTURE_2D;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.mipLevels = 1;
    desc.arraySize = 1;
    desc.format = format;
    desc.flags = RenderTextureFlag::RENDER_TARGET;
    scratch.texture = Device()->createTexture(desc);
    if (scratch.texture == nullptr) {
      g_oversizedColorScratch.erase(key);
      return nullptr;
    }
    CommandList()->barriers(
        RenderBarrierStage::GRAPHICS,
        RenderTextureBarrier(scratch.texture.get(),
                             RenderTextureLayout::COLOR_WRITE));
  }
  return scratch.texture.get();
}

void ResizeTileSurface(GuestSurface *surface, uint32_t width, uint32_t height);

// Predicated-tiling window emulation (2026-07-02). Each tile pass renders the
// FULL frame viewport shifted so its 256-row band lands at tile row 0 (Xenos
// PA_SC_WINDOW_OFFSET); SetRenderTargetInternal's D3D9 viewport reset instead
// squished the whole frame into the tile. Originally the band resolves in
// StretchRect identified the tile surface; with the deferred state stream
// live (2026-07-02 session 2) the resolve topology changed and that detection
// never fires, so tile surfaces are now identified at BIND time below.
static GuestBaseTexture *g_tileRenderTarget = nullptr;
static uint32_t g_tileFrameWidth = 0;
static uint32_t g_tileFrameHeight = 0;

void SetFramebuffer(GuestBaseTexture *renderTarget, GuestSurface *depthStencil,
                    bool forClear) {
  if (!forClear && !g_dirtyStates.renderTargetAndDepthStencil)
    return;

  // Bind-time EDRAM tile identification (2026-07-02 session 2): the game now
  // renders whole frames into a frame-wide x 256 surface (the EDRAM tile of
  // the recorded tiling pass -- hardware would replay it 3x with different
  // window offsets), recreated fresh per menu screen, and the new resolve
  // form is a single full-surface copy (no banded destPt) so the old
  // detection in StretchRect never triggers. A color RT of exactly
  // 1280x256 with FM2's 720p frame is unambiguous: grow it to full frame
  // height so the single pass rasterizes every row, and let the tile
  // viewport/scissor expansion in FlushViewport take it from there.
  {
    constexpr uint32_t kFm2FrameWidth = 1280u;
    constexpr uint32_t kFm2FrameHeight = 720u;
    constexpr uint32_t kFm2TileHeight = 256u;
    if (renderTarget != nullptr &&
        renderTarget->type == ResourceType::RenderTarget &&
        renderTarget->width == kFm2FrameWidth &&
        renderTarget->height == kFm2TileHeight) {
      ResizeTileSurface(static_cast<GuestSurface *>(renderTarget),
                        kFm2FrameWidth, kFm2FrameHeight);
      g_tileRenderTarget = renderTarget;
      g_tileFrameWidth = kFm2FrameWidth;
      g_tileFrameHeight = kFm2FrameHeight;
      g_dirtyStates.viewport = true;
      g_dirtyStates.scissorRect = true;
    }
  }

  // Fix #18 follow-up: if the tile COLOR surface was grown to frame height
  // but its depth partner wasn't (it wasn't bound at band-resolve time),
  // grow the depth here where the mismatched pair actually meets --
  // otherwise depth-tested draws are clamped to the old 256-row area.
  if (renderTarget != nullptr && depthStencil != nullptr &&
      depthStencil->width == renderTarget->width &&
      depthStencil->height < renderTarget->height) {
    ResizeTileSurface(depthStencil, renderTarget->width,
                      renderTarget->height);
  }

  GuestSurface *container = nullptr;
  RenderTexture *key = nullptr;
  if (renderTarget && depthStencil) {
    container = depthStencil;
    key = renderTarget->texture;
  } else if (renderTarget) {
    key = renderTarget->texture;
  } else if (depthStencil) {
    container = depthStencil;
    key = nullptr;
  }

  RenderCommandList *commandList = CommandList();
  if (container != nullptr) {
    auto &framebuffer = container->framebuffers[key];
    if (framebuffer == nullptr) {
      RenderFramebufferDesc desc;
      const RenderTexture *color =
          renderTarget ? renderTarget->texture : nullptr;
      if (renderTarget && depthStencil &&
          (depthStencil->height > renderTarget->height ||
           depthStencil->width > renderTarget->width)) {
        RenderTexture *scratch = GetOversizedDepthColorTarget(
            depthStencil->width, depthStencil->height, renderTarget->format);
        if (scratch != nullptr)
          color = scratch;
      }
      if (renderTarget) {
        desc.colorAttachments = &color;
        desc.colorAttachmentsCount = 1;
      }
      if (depthStencil)
        desc.depthAttachment = depthStencil->texture;
      framebuffer = Device()->createFramebuffer(desc);
    }
    if (g_framebuffer != framebuffer.get()) {
      commandList->setFramebuffer(framebuffer.get());
      g_framebuffer = framebuffer.get();
    }
  } else if (renderTarget != nullptr) {
    auto &framebuffer = g_colorFramebuffers[renderTarget->texture];
    if (framebuffer == nullptr) {
      const RenderTexture *color = renderTarget->texture;
      RenderFramebufferDesc desc(&color, 1);
      framebuffer = Device()->createFramebuffer(desc);
    }
    if (g_framebuffer != framebuffer.get()) {
      commandList->setFramebuffer(framebuffer.get());
      g_framebuffer = framebuffer.get();
    }
  } else if (g_framebuffer != nullptr) {
    commandList->setFramebuffer(nullptr);
    g_framebuffer = nullptr;
  }

  if (g_framebuffer != nullptr) {
    g_sharedConstants.halfPixelOffsetX =
        1.0f / float(g_framebuffer->getWidth());
    g_sharedConstants.halfPixelOffsetY =
        -1.0f / float(g_framebuffer->getHeight());
  }

  g_dirtyStates.renderTargetAndDepthStencil = forClear;
}

static int32_t g_tileViewportOffsetY = 0;

// Fix #18 (2026-07-02): recorded commands may still reference a resized tile
// surface's old resources; retire them instead of destroying.
static std::vector<std::unique_ptr<RenderTexture>> g_retiredTileTextures;
static std::vector<std::unique_ptr<RenderTextureView>> g_retiredTileViews;
static std::vector<std::unique_ptr<RenderFramebuffer>> g_retiredTileFramebuffers;

// Grow a predicated-tiling surface's host texture to full frame size. The
// game records its tile pass ONCE (hardware replays it per tile with
// different window offsets, which our stream never sees), so a 256-row host
// surface can never hold the full frame; a full-height surface lets the one
// recorded pass rasterize all rows and the band resolves slice it.
void ResizeTileSurface(GuestSurface *surface, uint32_t width,
                       uint32_t height) {
  RenderTextureDesc desc;
  desc.dimension = RenderTextureDimension::TEXTURE_2D;
  desc.width = width;
  desc.height = height;
  desc.depth = 1;
  desc.mipLevels = 1;
  desc.arraySize = 1;
  desc.format = surface->format;
  desc.flags = RenderFormatIsDepth(surface->format)
                   ? RenderTextureFlag::DEPTH_TARGET
                   : RenderTextureFlag::RENDER_TARGET;
  auto texture = Device()->createTexture(desc);
  if (texture == nullptr)
    return;
  // Remember the pre-grow guest height so TranslateGuestSurface keeps
  // treating the game's unchanged guest header as a cache hit (leak fix:
  // the mismatch used to recreate + regrow the pair every frame).
  if (surface->tileGrownFromHeight == 0)
    surface->tileGrownFromHeight = surface->height;
  g_retiredTileTextures.emplace_back(std::move(surface->textureHolder));
  g_retiredTileViews.emplace_back(std::move(surface->textureView));
  for (auto &entry : surface->framebuffers)
    g_retiredTileFramebuffers.emplace_back(std::move(entry.second));
  surface->framebuffers.clear();
  surface->textureHolder = std::move(texture);
  surface->texture = surface->textureHolder.get();
  RenderTextureViewDesc viewDesc;
  viewDesc.format = surface->format;
  viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
  viewDesc.mipLevels = 1;
  surface->textureView = surface->texture->createTextureView(viewDesc);
  surface->width = width;
  surface->height = height;
  surface->layout = RenderTextureLayout::UNKNOWN;
  surface->hostInitialized = false;
  surface->sourceTexture = nullptr;
  if (surface->descriptorIndex != 0) {
    TextureDescriptorSet()->setTexture(surface->descriptorIndex,
                                       surface->texture,
                                       RenderTextureLayout::SHADER_READ,
                                       surface->textureView.get());
  }
}

void FlushViewport() {
  RenderCommandList *commandList = CommandList();
  // The tiling window offset changes between passes without any viewport/RT
  // state change (the dirty flag may have been consumed by non-tile draws in
  // between), so force a re-flush whenever the applied offset is stale.
  static int32_t s_appliedTileOffsetY = INT32_MIN;
  const bool tileBound = g_renderTarget != nullptr &&
                         g_renderTarget == g_tileRenderTarget &&
                         g_tileFrameHeight > g_renderTarget->height;
  if (tileBound && s_appliedTileOffsetY != g_tileViewportOffsetY)
    g_dirtyStates.viewport = true;
  if (g_dirtyStates.viewport) {
    s_appliedTileOffsetY = tileBound ? g_tileViewportOffsetY : INT32_MIN;
    RenderViewport vp = g_viewport;
    // 2026-07-02 (live state stream): the game's own per-tile viewport
    // (1280x256) now EXECUTES (it was silently dropped with the rest of the
    // deferred command stream before), clamping rasterization to band 1 of
    // the full-height tile surface -> content only in the top third of the
    // frame. When the grown tile surface is bound, expand the tile-window
    // viewport back to the full surface so the single recorded pass
    // rasterizes every row (the hardware would replay it per band instead).
    if (g_renderTarget != nullptr && g_renderTarget == g_tileRenderTarget &&
        g_renderTarget->height == g_tileFrameHeight && vp.x == 0.0f &&
        vp.y == 0.0f && uint32_t(vp.width) == g_renderTarget->width &&
        vp.height > 0.0f && uint32_t(vp.height) < g_renderTarget->height) {
      vp.height = float(g_renderTarget->height);
    }
    if (g_renderTarget != nullptr && g_renderTarget == g_tileRenderTarget &&
        g_tileFrameHeight > g_renderTarget->height && vp.x == 0.0f &&
        vp.y == 0.0f) {
      if (uint32_t(vp.height) == g_renderTarget->height) {
        // Tile pass with the RT-reset viewport: restore the full-frame
        // viewport shifted by the current tiling window offset.
        vp.y = float(g_tileViewportOffsetY);
        vp.width = float(g_tileFrameWidth);
        vp.height = float(g_tileFrameHeight);
      } else if (uint32_t(vp.height) == g_tileFrameHeight) {
        // Game-set full-frame viewport on the tile RT: apply the offset only.
        vp.y = float(g_tileViewportOffsetY);
      }
      // TEMP DIAGNOSTIC 2026-07-02: confirm the tiling viewport override.
      static std::atomic<uint32_t> s_tvp{0};
      if (s_tvp.fetch_add(1, std::memory_order_relaxed) < 24) {
        if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
          std::fprintf(f,
                       "FM2_TILEVP in=(%.0f,%.0f %.0fx%.0f) out=(%.0f,%.0f "
                       "%.0fx%.0f) offY=%d rt=%ux%u frame=%ux%u\n",
                       g_viewport.x, g_viewport.y, g_viewport.width,
                       g_viewport.height, vp.x, vp.y, vp.width, vp.height,
                       g_tileViewportOffsetY, g_renderTarget->width,
                       g_renderTarget->height, g_tileFrameWidth,
                       g_tileFrameHeight);
          std::fclose(f);
        }
      }
    }
    if (SceneReverseZ()) {
      vp.minDepth = 1.0f;
      vp.maxDepth = 0.0f;
    }
    // TEMP DIAGNOSTIC 2026-07-01 (depth-rejection investigation): capture the
    // game-side viewport depth range vs what the reverse-Z override submits.
    {
      static std::atomic<uint32_t> s_n{0};
      if (s_n.fetch_add(1, std::memory_order_relaxed) < 32) {
        if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
          std::fprintf(f,
                       "FM2_ZVIEWPORT game=(%f..%f) applied=(%f..%f) revZ=%d "
                       "size=%.0fx%.0f\n",
                       g_viewport.minDepth, g_viewport.maxDepth, vp.minDepth,
                       vp.maxDepth, SceneReverseZ() ? 1 : 0, g_viewport.width,
                       g_viewport.height);
          std::fclose(f);
        }
      }
    }
    commandList->setViewports(vp);
    g_dirtyStates.viewport = false;
  }
  if (g_dirtyStates.scissorRect) {
    RenderRect rect =
        g_scissorTestEnable
            ? g_scissorRect
            : RenderRect(int32_t(g_viewport.x), int32_t(g_viewport.y),
                         int32_t(g_viewport.x + g_viewport.width),
                         int32_t(g_viewport.y + g_viewport.height));
    // Tile-window scissor expansion (see the matching viewport override
    // above): the game's live per-tile 1280x256 scissor must not clamp the
    // single recorded pass on the grown full-height tile surface.
    if (g_renderTarget != nullptr && g_renderTarget == g_tileRenderTarget &&
        g_renderTarget->height == g_tileFrameHeight && rect.left == 0 &&
        rect.top == 0 && uint32_t(rect.right) == g_renderTarget->width &&
        rect.bottom > 0 && uint32_t(rect.bottom) < g_renderTarget->height) {
      rect.bottom = int32_t(g_renderTarget->height);
    }
    commandList->setScissors(rect);
    g_dirtyStates.scissorRect = false;
  }
}

// ---------------------------------------------------------------------------
// Per-draw constant + sampler upload
// ---------------------------------------------------------------------------

void FlushSamplerStates(GuestDevice *device) {
  for (uint32_t i = 0; i < 16; ++i) {
    const GuestSamplerState &s = device->samplerStates[i];
    uint32_t data0 = s.data[0].get();
    uint32_t data3 = s.data[3].get();
    uint32_t data5 = s.data[5].get();

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

    uint64_t hash = XXH3_64bits(&desc, sizeof(desc));
    auto &[descriptorIndex, sampler] = g_samplerStates[hash];
    if (sampler == nullptr) {
      descriptorIndex = uint32_t(g_samplerStates.size());
      sampler = Device()->createSampler(desc);
      SamplerDescriptorSet()->setSampler(descriptorIndex - 1, sampler.get());
    }
    g_sharedConstants.samplerIndices[i] = descriptorIndex - 1;
  }
}

// session 6P-3: unified VS float-constant file, register-indexed (reg N at
// [N*4..]), raw big-endian -- mirrors the render-pass uploads
// (FM2_RenderContext_UploadMatrixConstants) the GuestDevice never receives in
// plume_native. FlushRenderState uploads this (byte-swapped) instead of device+0x700
// once any pass upload has happened.
alignas(16) static uint32_t g_passVsConstants[0x400] = {};
// PM4-applied VS ALU constants (2026-07-02 session 3): fed in stream order by
// the command-buffer scanner (ApplyPm4VsConstants) from SET_CONSTANT /
// Type-0 ALU / LOAD_ALU_CONSTANT packets -- the ONLY carrier of the
// per-element 2D placement matrices. Raw big-endian dwords, dword-indexed;
// coverage tracked per vec4 register.
alignas(16) static uint32_t g_pm4VsConstants[0x400] = {};
static uint64_t g_pm4VsConstantsCoverage[4] = {};
// Registers written in the CURRENT command-buffer delta only (cleared by
// BeginPm4ConstantDelta before each scan). 3D shaders overlay just these:
// accumulated coverage goes stale across passes and stomps live-file values
// (user-verified regression: bright flashing + missing 3D), while the fresh
// set is by construction what the stream set for THIS draw (the car's
// per-object WVP pattern).
static uint64_t g_pm4VsConstantsFreshCoverage[4] = {};
static std::atomic<bool> g_pm4VsConstantsValid{false};
// PS half of the PM4 ALU space (packet idx 0x400-0x7FF / Type-0 base
// 0x4400-0x47FF). Material/paint colors ride here -- unapplied, the pixel
// shaders read transient live-file values and surfaces FLASH primary colors
// per frame (user-verified on the car + showroom panels).
alignas(16) static uint32_t g_pm4PsConstants[0x400] = {};
static uint64_t g_pm4PsConstantsCoverage[4] = {};
static uint64_t g_pm4PsConstantsFreshCoverage[4] = {};
static std::atomic<bool> g_pm4PsConstantsValid{false};
// Bit N set = register N was written by a pass upload (drives the per-register
// overlay in FlushRenderState; see the merged-file comment there).
static uint64_t g_passVsConstantsCoverage[4] = {};
static std::atomic<bool> g_passVsConstantsValid{false};
// Live register files on the game's PM4 render context (see
// SetLiveFloatConstantFiles in render_internal.h). Render-thread only.
static const uint32_t *g_liveVsFloatConstants = nullptr;
static const uint32_t *g_livePsFloatConstants = nullptr;
// UI glyph ModelView rows (registers 0-3, raw big-endian) captured by the
// UI-submit hook (see SetUiGlyphModelView in render_internal.h).
alignas(16) static uint32_t g_uiGlyphModelView[16] = {};
static std::atomic<bool> g_uiGlyphModelViewValid{false};
// 2026-07-02 session 5: last SET_CONSTANT idx=0 regs=16 payload (the 4x4 2D
// placement matrix; see SetGlyphPlacementMatrix in render_internal.h). Fed
// ONLY by that distinctive packet -- never by regs=3 writes or Type-0 bursts,
// the two writers that stomp c0-c3 in the accumulated shadow.
alignas(16) static uint32_t g_glyphPlacementMatrix[16] = {};
static std::atomic<bool> g_glyphPlacementMatrixValid{false};

void FlushRenderState(GuestDevice *device) {
  // Forza programs per-draw color-write through its PM4 draw-list node, which the
  // native renderer doesn't decode -- so the mirrored/context color-write is stale
  // (0) for forward-pass draws and they would render depth-only, leaving the screen
  // at its clear color. A draw with a pixel shader is a color pass (Forza's depth
  // prepass is position-only with NO pixel shader), so enable color writes for it.
  if (g_pipelineState.pixelShader != nullptr &&
      g_pipelineState.colorWriteEnable == 0 && g_renderTarget != nullptr) {
    g_pipelineState.colorWriteEnable = uint32_t(RenderColorWriteEnable::ALL);
  }

  GuestBaseTexture *renderTarget =
      g_pipelineState.colorWriteEnable ? g_renderTarget : nullptr;
  GuestSurface *depthStencil =
      (g_pipelineState.zEnable || g_pipelineState.stencilEnable)
          ? g_depthStencil
          : nullptr;
  if (depthStencil == nullptr &&
      (g_pipelineState.zEnable || g_pipelineState.stencilEnable) &&
      g_renderTarget && g_implicitDepthStencil != nullptr &&
      g_implicitDepthStencil->width == g_renderTarget->width &&
      g_implicitDepthStencil->height == g_renderTarget->height) {
    depthStencil = g_implicitDepthStencil;
    SetDirtyValue(g_dirtyStates.pipelineState,
                  g_pipelineState.depthStencilFormat, depthStencil->format);
    g_dirtyStates.renderTargetAndDepthStencil = true;
  }

  // DIAG (T11): per-draw census of the bound color RT + whether the VS has
  // POSITION0, to find whether the MAIN content (POSITION0 quads / 3D car) is
  // drawn at all in plume_native and which RT it targets vs the sprite subpass
  // (no POSITION0). Logs first 24 of each class + periodic totals.
  {
    const GuestShader *censusVs = g_pipelineState.vertexShader;
    bool hasPos = false, known = false;
    if (censusVs != nullptr && !censusVs->headerElements.empty()) {
      known = true;
      for (const auto &he : censusVs->headerElements)
        if (he.usage == 0)
          hasPos = true;
    }
    const char *cls = hasPos ? "POS" : (known ? "NOPOS" : "UNKNOWN");
    const uint64_t h = (censusVs && censusVs->shaderCacheEntry)
                           ? censusVs->shaderCacheEntry->hash
                           : 0ull;
    static std::mutex s_mtx;
    static std::unordered_set<uint64_t> s_seen;
    static std::atomic<uint32_t> s_total{0}, s_pos{0}, s_nopos{0}, s_unk{0};
    const uint32_t t = s_total.fetch_add(1, std::memory_order_relaxed);
    if (hasPos)
      s_pos.fetch_add(1, std::memory_order_relaxed);
    else if (known)
      s_nopos.fetch_add(1, std::memory_order_relaxed);
    else
      s_unk.fetch_add(1, std::memory_order_relaxed);
    bool isNew = false;
    {
      std::lock_guard<std::mutex> lk(s_mtx);
      if (s_seen.size() < 128)
        isNew = s_seen.insert(h).second;
    }
    if (isNew) {
      const GuestBaseTexture *rt = g_renderTarget;
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(
            f, "FM2_RTCENSUS2 %-7s rt=%p %ux%u fmt=%d cw=%u hash=0x%016llX\n",
            cls, (const void *)rt, rt ? rt->width : 0u, rt ? rt->height : 0u,
            rt ? int(rt->format) : -1, g_pipelineState.colorWriteEnable,
            (unsigned long long)h);
        std::fflush(f);
        std::fclose(f);
      }
    }
    if ((t & 0xFFF) == 0) {
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(f,
                     "FM2_RTCENSUS_TOTAL total=%u pos=%u nopos=%u unknown=%u\n",
                     t + 1, s_pos.load(), s_nopos.load(), s_unk.load());
        std::fflush(f);
        std::fclose(f);
      }
    }
  }

  FlushPendingStretchRects(renderTarget, depthStencil);

  AddBarrier(renderTarget, RenderTextureLayout::COLOR_WRITE);
  AddBarrier(depthStencil, RenderTextureLayout::DEPTH_WRITE);
  FlushBarriers();

  SetFramebuffer(renderTarget, depthStencil, false);
  FlushViewport();

  RenderCommandList *commandList = CommandList();

  // DEBUG (session 6P-2): force-clear the bound color RT to bright blue before each
  // draw to test whether the RT we bind actually reaches the screen / viewer. If the
  // presented RT (or grid RT cells) turn blue, the bind+present pipeline works and
  // the draws simply aren't rasterizing visible pixels; if they stay black, we are
  // binding/presenting a different RT than the census reports. Toggle off when done.
  // DEBUG (session 6P-3): clear the bound RT to blue ONLY when we switch to a new
  // RT (start of its draw batch), so the batch's draws accumulate on blue. If the
  // RT ends up showing geometry on blue -> draws DO rasterize (issue is present
  // selection); if it stays solid blue -> draws produce nothing (transform/const).
  static constexpr bool kDebugClearRtBlue = false;
  static GuestBaseTexture *s_lastBlueRt = nullptr;
  if (kDebugClearRtBlue && renderTarget != nullptr &&
      renderTarget != s_lastBlueRt) {
    s_lastBlueRt = renderTarget;
    RenderRect full(0, 0, int32_t(renderTarget->width),
                    int32_t(renderTarget->height));
    commandList->clearColor(0, RenderColor(0.0f, 0.25f, 1.0f, 1.0f), &full, 1);
  }

  PipelineState pipelineState = g_pipelineState;
  // DEBUG (session 6P-3): force cull off to test whether backface culling (cull=1
  // in FM2_DRAWSTATE) is rejecting every triangle (wrong winding) -> solid blue.
  static constexpr bool kDebugForceCullNone = true;
  if (kDebugForceCullNone)
    pipelineState.cullMode = RenderCullMode::NONE;
  // DEBUG (session 6P-3): the PSO is built for the surface's intended MSAA count
  // (often 4) but the actual plume render targets are single-sampled (count 1).
  // D3D12 rejects the sample-desc mismatch (debug id=614/616) and the draw renders
  // nothing. Force the pipeline to COUNT_1 to match the real RTs.
  static constexpr bool kDebugForceSingleSample = true;
  if (kDebugForceSingleSample)
    pipelineState.sampleCount = RenderSampleCount::COUNT_1;
  if (depthStencil == nullptr) {
    pipelineState.zEnable = false;
    pipelineState.zWriteEnable = false;
    pipelineState.stencilEnable = false;
    pipelineState.depthStencilFormat = RenderFormat::UNKNOWN;
  }
  RenderPipeline *pipeline = GetPipeline(pipelineState);
  if (pipeline != nullptr)
    commandList->setPipeline(pipeline);
  g_pipelineBound = (pipeline != nullptr);

  // DEBUG (session 6P-3): one-shot full draw-state + constant dump for the first
  // 3D draws (POS VS + PS), to find why nothing rasterizes. Logs cull/depth/blend/
  // viewport/scissor and c0..c3 as byte-swapped floats from BOTH +0x0 and +0x700 so
  // we can see which offset holds a sane WVP matrix (and rule depth/cull in or out).
  {
    const GuestShader *dvs = g_pipelineState.vertexShader;
    bool is3d = dvs != nullptr && !dvs->headerElements.empty() &&
                g_pipelineState.pixelShader != nullptr;
    if (is3d) {
      bool hasPos = false;
      for (const auto &he : dvs->headerElements)
        if (he.usage == 0) { hasPos = true; break; }
      is3d = hasPos;
    }
    static std::atomic<uint32_t> s_dn{0};
    if (is3d && g_passVsConstantsValid.load(std::memory_order_relaxed) &&
        s_dn.fetch_add(1, std::memory_order_relaxed) < 10) {
      const uint8_t *db = reinterpret_cast<const uint8_t *>(device);
      auto sf = [&](uint32_t off) -> double {
        uint32_t r = __builtin_bswap32(
            *reinterpret_cast<const uint32_t *>(db + off));
        float fv;
        std::memcpy(&fv, &r, 4);
        return double(fv);
      };
      auto pf = [&](uint32_t idx) -> double {
        uint32_t r = __builtin_bswap32(g_passVsConstants[idx]);
        float fv;
        std::memcpy(&fv, &r, 4);
        return double(fv);
      };
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(
            f,
            "FM2_DRAWSTATE cull=%d zEn=%d zFunc=%d zWr=%d dClip=%d blend=%d "
            "cw=0x%X vp=(%.0f,%.0f %.0fx%.0f z=%.2f..%.2f) scis=%d rZ=%d "
            "rt=%p %ux%u ibFmt=%d ibSz=%u ibRef=%d idirty=%d\n"
            "  @0x780 c0=[%.3f %.3f %.3f %.3f] c1=[%.3f %.3f %.3f %.3f] "
            "c2=[%.3f %.3f %.3f %.3f] c3=[%.3f %.3f %.3f %.3f]\n"
            "  @0x700 c0=[%.3f %.3f %.3f %.3f] c1=[%.3f %.3f %.3f %.3f]\n"
            "  PASS valid=%d c0=[%.3f %.3f %.3f %.3f] c1=[%.3f %.3f %.3f %.3f]\n",
            int(g_pipelineState.cullMode), g_pipelineState.zEnable,
            int(g_pipelineState.zFunc), g_pipelineState.zWriteEnable,
            g_pipelineState.depthClipEnabled, g_pipelineState.alphaBlendEnable,
            g_pipelineState.colorWriteEnable, g_viewport.x, g_viewport.y,
            g_viewport.width, g_viewport.height, g_viewport.minDepth,
            g_viewport.maxDepth, g_scissorTestEnable, SceneReverseZ(),
            (const void *)renderTarget, renderTarget ? renderTarget->width : 0,
            renderTarget ? renderTarget->height : 0,
            int(g_indexBufferView.format), g_indexBufferView.size,
            g_indexBufferView.buffer.ref != nullptr ? 1 : 0,
            g_dirtyStates.indices ? 1 : 0, sf(0x780), sf(0x784),
            sf(0x788), sf(0x78C), sf(0x790), sf(0x794), sf(0x798), sf(0x79C),
            sf(0x7A0), sf(0x7A4), sf(0x7A8), sf(0x7AC), sf(0x7B0), sf(0x7B4),
            sf(0x7B8), sf(0x7BC), sf(0x700), sf(0x704), sf(0x708), sf(0x70C),
            sf(0x710), sf(0x714), sf(0x718), sf(0x71C),
            g_passVsConstantsValid.load(std::memory_order_relaxed) ? 1 : 0,
            pf(0), pf(1), pf(2), pf(3), pf(4), pf(5), pf(6), pf(7));
        std::fclose(f);
      }
      // The car VS (Shader fb3c0c13) reads its WVP matrix from cbuffer regs
      // c7..c18. Dump those regs (full vec4) from device+0x780 (the struct field
      // SetVertexShaderConstantF writes), device+0x700 (UploadMatrixConstants
      // base / what FlushRenderState currently uploads), and g_passVsConstants,
      // to find which source actually holds the matrix at the regs the VS reads.
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(f, "  CMAT780");
        for (uint32_t r = 7; r <= 14; ++r)
          std::fprintf(f, " c%u[%.2f %.2f %.2f %.2f]", r, sf(0x780 + r * 16),
                       sf(0x780 + r * 16 + 4), sf(0x780 + r * 16 + 8),
                       sf(0x780 + r * 16 + 12));
        std::fprintf(f, "\n  CMAT700");
        for (uint32_t r = 7; r <= 14; ++r)
          std::fprintf(f, " c%u[%.2f %.2f %.2f %.2f]", r, sf(0x700 + r * 16),
                       sf(0x700 + r * 16 + 4), sf(0x700 + r * 16 + 8),
                       sf(0x700 + r * 16 + 12));
        std::fprintf(f, "\n  CMATpass");
        for (uint32_t r = 7; r <= 14; ++r)
          std::fprintf(f, " c%u[%.2f %.2f %.2f %.2f]", r, pf(r * 4),
                       pf(r * 4 + 1), pf(r * 4 + 2), pf(r * 4 + 3));
        std::fprintf(f, "\n");
        std::fclose(f);
      }
      // STAGE: geometry inputs bound at draw time (does the game's vertex/index
      // data survive to here, or is it 0?).
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        const auto *decl = g_pipelineState.vertexDeclaration;
        std::fprintf(f, "  GEO decl=%p elems=%u idxStream=%d | vtx:",
                     (const void *)decl, decl ? decl->inputElementCount : 0u,
                     decl ? int(decl->indexVertexStream) : -1);
        for (uint32_t s = 0; s < 6; ++s)
          std::fprintf(f, " s%u[ref=%d sz=%u str=%u]", s,
                       g_vertexBufferViews[s].buffer.ref != nullptr ? 1 : 0,
                       (unsigned)g_vertexBufferViews[s].size,
                       (unsigned)g_inputSlots[s].stride);
        std::fprintf(f, "\n");
        std::fclose(f);
      }
    }
  }

  // Booleans and sampler indices feed the shared constants (16 bits each in
  // the XenosRecomp ABI).
  g_sharedConstants.booleans =
      (device->vertexShaderBoolConstants[0].get() & 0xFFFF) |
      ((device->pixelShaderBoolConstants[0].get() & 0xFFFF) << 16);
  FlushSamplerStates(device);

  // Constants are byte-swapped out of guest memory each draw (no dirty-range
  // tracking; the guest writes them directly to device memory).
  {
    // Confirm the corrected constant base (+0x700) carries data the old +0x780
    // missed (esp. the low vertex registers / transform matrix). Target the
    // no-POSITION HUD shaders (position derived from c0*(|tc0|+c4)+c1*tc0.y+c3),
    // which collapse to the origin when c0/c1/c3/c4 are zero.
    const GuestShader *diagVs = g_pipelineState.vertexShader;
    bool diagHud = diagVs != nullptr && !diagVs->headerElements.empty();
    if (diagHud) {
      bool noPos = true;
      for (const auto &he : diagVs->headerElements)
        if (he.usage == 0)
          noPos = false;
      const uint64_t vhash =
          diagVs->shaderCacheEntry ? diagVs->shaderCacheEntry->hash : 0ull;
      diagHud = noPos || vhash == 0x292FF29403B1DDF8ull;
    }
    static std::atomic<uint32_t> s_n{0};
    if (diagHud && s_n.fetch_add(1, std::memory_order_relaxed) < 20) {
      const uint8_t *b = reinterpret_cast<const uint8_t *>(device);
      const uint32_t *v700 = reinterpret_cast<const uint32_t *>(b + 0x700);
      const uint32_t *v1700 = reinterpret_cast<const uint32_t *>(b + 0x1700);
      uint32_t nz700 = 0;
      for (uint32_t i = 0; i < 256 * 4; ++i)
        if (v700[i] != 0)
          ++nz700;
      const uint64_t h =
          diagVs->shaderCacheEntry ? diagVs->shaderCacheEntry->hash : 0ull;
      // Scan the device block (0..0x2000, in 0x80=8-register chunks) for WHERE
      // non-zero constants actually live -- the HUD c0/c1/c3/c4 are zero at +0x700,
      // so find the real offset (the game may write them elsewhere).
      char scan[400];
      int sp = 0;
      const uint32_t *bw = reinterpret_cast<const uint32_t *>(b);
      for (uint32_t off = 0; off < 0x2000u && sp < 360; off += 0x80u) {
        uint32_t nz = 0;
        for (uint32_t i = 0; i < 0x80u / 4u; ++i)
          if (bw[off / 4u + i] != 0)
            ++nz;
        if (nz != 0)
          sp += std::snprintf(scan + sp, sizeof(scan) - sp, "+0x%X:%u ", off, nz);
      }
      scan[sp < int(sizeof(scan)) ? sp : int(sizeof(scan)) - 1] = '\0';
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(f,
                     "FM2_HUDCONST hash=0x%016llX nz@0x700=%u v700[0..4]=%08X,%08X,"
                     "%08X,%08X,%08X v1700[0..4]=%08X,%08X,%08X,%08X,%08X | "
                     "nzBlocks=%s\n",
                     (unsigned long long)h, nz700, v700[0], v700[1], v700[2],
                     v700[3], v700[4], v1700[0], v1700[1], v1700[2], v1700[3],
                     v1700[4], scan);
        std::fclose(f);
      }
    }
  }
  // 3D-DRAW PROBE (session 6P-2): the forward color pass outputs BLACK while
  // sampling GRAY (0x80808080) resolve-dests -- gray-in can't give black-out, so
  // the zeroing factor is likely PS material/lighting constants (+0x1700) == 0 or
  // invalid bindless texture indices. Log per 3D draw (has-position VS + a PS,
  // excluding the HUD shader): VS const nz@0x700, PS const nz@0x1700, and the first
  // bound texture2D indices (0 = sampling the default/black descriptor).
  {
    const GuestShader *vs3 = g_pipelineState.vertexShader;
    bool has3D = vs3 != nullptr && !vs3->headerElements.empty() &&
                 g_pipelineState.pixelShader != nullptr;
    if (has3D) {
      bool hasPos = false;
      for (const auto &he : vs3->headerElements)
        if (he.usage == 0) {
          hasPos = true;
          break;
        }
      const uint64_t vh =
          vs3->shaderCacheEntry ? vs3->shaderCacheEntry->hash : 0ull;
      has3D = hasPos && vh != 0x292FF29403B1DDF8ull;
    }
    // Dedup by distinct VS hash (render thread only) so we see EVERY 3D shader
    // that renders, not just the first 24 draws (which were all reflection). A
    // car-body MATERIAL shader (sampling a loaded diffuse) would show as a new hash.
    static uint32_t s_n3dCount = 0;
    static uint64_t s_3dHashes[32] = {};
    bool logIt = false;
    if (has3D) {
      const uint64_t vhd =
          vs3->shaderCacheEntry ? vs3->shaderCacheEntry->hash : 0ull;
      bool seen = false;
      for (uint32_t i = 0; i < s_n3dCount && i < 32u; ++i)
        if (s_3dHashes[i] == vhd) {
          seen = true;
          break;
        }
      if (!seen && s_n3dCount < 32u) {
        s_3dHashes[s_n3dCount++] = vhd;
        logIt = true;
      }
    }
    if (logIt) {
      const uint8_t *b = reinterpret_cast<const uint8_t *>(device);
      const uint32_t *v700 = reinterpret_cast<const uint32_t *>(b + 0x700);
      const uint32_t *v1700 = reinterpret_cast<const uint32_t *>(b + 0x1700);
      uint32_t nz700 = 0, nz1700 = 0;
      for (uint32_t i = 0; i < 256u * 4u; ++i) {
        if (v700[i] != 0)
          ++nz700;
        if (v1700[i] != 0)
          ++nz1700;
      }
      const uint64_t vh =
          vs3->shaderCacheEntry ? vs3->shaderCacheEntry->hash : 0ull;
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(f,
                     "FM2_3DCONST vh=0x%016llX nzVS@700=%u nzPS@1700=%u "
                     "texIdx=%u,%u,%u,%u\n",
                     (unsigned long long)vh, nz700, nz1700,
                     g_sharedConstants.texture2DIndices[0],
                     g_sharedConstants.texture2DIndices[1],
                     g_sharedConstants.texture2DIndices[2],
                     g_sharedConstants.texture2DIndices[3]);
        std::fclose(f);
      }
    }
  }
  // VS float-constant file (2026-07-02 rework). Two register-aligned sources:
  //  - the XDK SetVertexShaderConstantF file (struct field, device+0x780) --
  //    holds everything the game sets through the D3D API (globals like
  //    c253-c255, materials);
  //  - the render-pass uploads (UploadMatrixConstants writes register r to
  //    context+0x700+r*16 on the RENDER CONTEXT, mirrored per-register into
  //    g_passVsConstants) -- holds the scene transforms.
  // The previous code uploaded ONLY g_passVsConstants once valid, zeroing every
  // register the pass never wrote (c0-c8 in practice) -> scene VS color math
  // read zeros -> black tile scene. Merge instead: SetF file as base, overlay
  // exactly the registers a pass upload has written.
  alignas(16) static uint32_t s_mergedVsConstants[0x400];
  const uint32_t *vsConstSrc = device->vertexShaderFloatConstants;
  // Session 4 outcome: BOTH takes regressed and are off. Take 1
  // (ring-wrap-following + accumulated 3D) applied stale ring bytes; take 2
  // (write-time SetF-hook transport + accumulated 3D) applied constants at
  // RECORD time while deferred draws execute later, smearing per-element
  // values. Session-3 behavior restored: scanner-fed shadow, 2D=accumulated,
  // 3D=fresh-per-delta (this flag false).
  static constexpr bool kCpAccumulate3D = false;
  // Hoisted for both the VS and PS PM4-overlay policies below: 2D UI shaders
  // have no POSITION element (position rides in TEXCOORD halves).
  bool noPos = false;
  {
    const GuestShader *vs2d = g_pipelineState.vertexShader;
    noPos = vs2d != nullptr && !vs2d->headerElements.empty();
    if (noPos) {
      for (const auto &he : vs2d->headerElements)
        if (he.usage == 0) {
          noPos = false;
          break;
        }
    }
  }
  if (g_liveVsFloatConstants != nullptr) {
    // PM4 draw: base = the issuing context's real ALU register file
    // (ctx+0x710, register-aligned). FM2 uses MULTIPLE render contexts
    // (g_FM2_ActivePassRenderContext_ switches per pass) and on hardware the
    // ring merges their constant emits -- e.g. the UI ModelView (c0-c2) is
    // uploaded on the UI/movie context while the glyph draws are issued on
    // the main one (proven by the Xenia-vs-plume arcade capture diff). Model
    // that by overlaying the cross-context SetVertexShaderConstantFN mirror
    // (g_passVsConstants, fed in call order by the 0x8236D958 hook) on top.
    // PM4 draw: upload the issuing context's live file verbatim for 3D (POS)
    // shaders -- broad mirror overlays plant wrong values (a camera row
    // landed in a 3D shader's color register c2 -> the magenta menu).
    vsConstSrc = g_liveVsFloatConstants;
    // Targeted cross-context fix for 2D/no-POSITION shaders (UI glyphs,
    // sprites): their position math reads ModelView rows at registers 0-3,
    // which the game uploads via SetVertexShaderConstantFN on the ACTIVE
    // PASS context -- a different object than the issuing one, so the live
    // block lacks them and all text collapses to screen center (proven by
    // the Xenia arcade diff). The SetF mirror is call-ordered across all
    // contexts; overlay just regs 0-3 for these shaders.
    {
      // DISABLED 2026-07-02: a single captured UI matrix is wrong too --
      // UiOrScreenDrawListSubmit runs PER UI ELEMENT, each staging its OWN
      // ModelView before emitting its slice of the recorded list, so any one
      // matrix smears every other element (worse than the centered collapse).
      static constexpr bool kUseUiGlyphModelViewOverlay = false;
      if (kUseUiGlyphModelViewOverlay && noPos &&
          g_uiGlyphModelViewValid.load(std::memory_order_relaxed)) {
        std::memcpy(s_mergedVsConstants, g_liveVsFloatConstants,
                    sizeof(s_mergedVsConstants));
        std::memcpy(s_mergedVsConstants, g_uiGlyphModelView,
                    sizeof(g_uiGlyphModelView));
        vsConstSrc = s_mergedVsConstants;
      }
      // 2026-07-02 session 3 (supersedes the disabled single-matrix overlay):
      // the per-element placement matrices travel as PM4 SET_CONSTANT /
      // Type-0 ALU packets in the command buffer (RenderDoc: SET_CONSTANT
      // idx=0 regs=16 with the 0.05-scale row right before each 2D element
      // draw; live file has color+zeros there). The scanner applies them in
      // stream order into g_pm4VsConstants BEFORE each draw, so each 2D draw
      // sees exactly the constants the Xenos CP would have applied.
      // 3D shaders (car-mesh follow-up): the CAR's per-object WVP (c7..c18
      // per the scanner's original diagnostic) ALSO travels only as PM4 --
      // the car renderer never calls the CPU SetF path the scene-object pass
      // uses. But overlaying the ACCUMULATED coverage for 3D regressed hard
      // (user-verified: bright flashing + missing meshes -- stale shadow regs
      // stomp live-file materials/colors). That staleness was the scanner's
      // wrap-skip dropping whole deltas; with the ring now followed across
      // wraps (kCpAccumulate3D, session 4) the accumulated shadow is
      // stream-exact and 3D consumes it too. 2D keeps the accumulated set
      // (user-verified improvement).
      const bool use2dOverlay =
          noPos && g_pm4VsConstantsValid.load(std::memory_order_relaxed);
      const bool use3dOverlay =
          !noPos && g_pm4VsConstantsValid.load(std::memory_order_relaxed);
      if (use2dOverlay || use3dOverlay) {
        const uint64_t *cover = (use2dOverlay || kCpAccumulate3D)
                                    ? g_pm4VsConstantsCoverage
                                    : g_pm4VsConstantsFreshCoverage;
        std::memcpy(s_mergedVsConstants, g_liveVsFloatConstants,
                    sizeof(s_mergedVsConstants));
        bool any = false;
        for (uint32_t w = 0; w < 4u; ++w) {
          uint64_t bits = cover[w];
          while (bits != 0) {
            const uint32_t reg = w * 64u + uint32_t(std::countr_zero(bits));
            bits &= bits - 1u;
            std::memcpy(s_mergedVsConstants + reg * 4u,
                        g_pm4VsConstants + reg * 4u, 16u);
            any = true;
          }
        }
        if (any || use2dOverlay)
          vsConstSrc = s_mergedVsConstants;
      }
      // 2026-07-02 session 5 (press-A fix): the accumulated shadow's c0-c3 is
      // whichever of THREE writers came last -- the per-draw regs=16 placement
      // matrix (correct, c0.x~0.05), an unrelated regs=3 write (c0.x=-0.0,
      // what draws were seeing), or a Type-0 dirty-flush burst. Overlay c0-c3
      // for 2D shaders from the dedicated "last regs=16 idx=0" shadow, which
      // only ever holds the matrix. 3D untouched.
      static constexpr bool kUseGlyphPlacementMatrixShadow = true;
      const bool glyphShadowValid =
          g_glyphPlacementMatrixValid.load(std::memory_order_relaxed);
      if (kUseGlyphPlacementMatrixShadow && noPos && glyphShadowValid) {
        if (vsConstSrc != s_mergedVsConstants) {
          std::memcpy(s_mergedVsConstants, vsConstSrc,
                      sizeof(s_mergedVsConstants));
          vsConstSrc = s_mergedVsConstants;
        }
        std::memcpy(s_mergedVsConstants, g_glyphPlacementMatrix,
                    sizeof(g_glyphPlacementMatrix));
      }
      // DIAG (session 5, temporary): per-draw proof of the final uploaded
      // c0 for 2D shaders -- first 16 noPos draws + a periodic sample.
      if (noPos) {
        static std::atomic<uint32_t> s_n2d{0};
        const uint32_t n2d = s_n2d.fetch_add(1, std::memory_order_relaxed);
        if (n2d < 16 || (n2d & 0x7FF) == 0) {
          auto bef = [&](uint32_t dw) -> float {
            uint32_t v = vsConstSrc[dw];
            v = ((v >> 24) & 0xFFu) | ((v >> 8) & 0xFF00u) |
                ((v << 8) & 0xFF0000u) | (v << 24);
            float f;
            std::memcpy(&f, &v, 4);
            return f;
          };
          const GuestShader *dvs = g_pipelineState.vertexShader;
          const uint64_t dh = (dvs && dvs->shaderCacheEntry)
                                  ? dvs->shaderCacheEntry->hash
                                  : 0ull;
          auto liveF = [&](uint32_t dw) -> float {
            if (g_liveVsFloatConstants == nullptr) return -999.0f;
            uint32_t v = g_liveVsFloatConstants[dw];
            v = ((v >> 24) & 0xFFu) | ((v >> 8) & 0xFF00u) |
                ((v << 8) & 0xFF0000u) | (v << 24);
            float f;
            std::memcpy(&f, &v, 4);
            return f;
          };
          if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
            std::fprintf(f,
                         "FM2_GLYPHMTX_DRAW n=%u vh=0x%016llX shadow=%d "
                         "c0=(%g,%g,%g,%g) c3=(%g,%g,%g,%g) up_c9=(%g,%g,%g,%g)"
                         " up_c10=(%g,%g,%g,%g) live_c9.x=%g live_c10.x=%g"
                         " pm4cov0=%016llX\n",
                         n2d, (unsigned long long)dh, glyphShadowValid ? 1 : 0,
                         bef(0), bef(1), bef(2), bef(3), bef(12), bef(13),
                         bef(14), bef(15), bef(36), bef(37), bef(38), bef(39),
                         bef(40), bef(41), bef(42), bef(43), liveF(36),
                         liveF(40),
                         (unsigned long long)g_pm4VsConstantsCoverage[0]);
            std::fclose(f);
          }
        }
      }
    }
  } else if (g_passVsConstantsValid.load(std::memory_order_relaxed)) {
    std::memcpy(s_mergedVsConstants, device->vertexShaderFloatConstants,
                sizeof(s_mergedVsConstants));
    for (uint32_t w = 0; w < 4u; ++w) {
      uint64_t bits = g_passVsConstantsCoverage[w];
      while (bits != 0) {
        const uint32_t reg = w * 64u + uint32_t(std::countr_zero(bits));
        bits &= bits - 1u;
        std::memcpy(s_mergedVsConstants + reg * 4u,
                    g_passVsConstants + reg * 4u, 16u);
      }
    }
    vsConstSrc = s_mergedVsConstants;
  }
  SetRootDescriptor(g_uploadAllocator.allocateCopy<true>(
                        vsConstSrc,
                        sizeof(device->vertexShaderFloatConstants), 0x100),
                    0);
  const uint32_t *psConstSrc = g_livePsFloatConstants != nullptr
                                   ? g_livePsFloatConstants
                                   : device->pixelShaderFloatConstants;
  // PS PM4 overlay (2026-07-02): material/paint colors travel as PM4 ALU
  // writes in the PS half (idx 0x400+); unapplied, PS reads transient
  // live-file values -> per-frame FLASHING primary colors (user-verified on
  // the car + showroom panels). Same policy as VS (kCpAccumulate3D).
  alignas(16) static uint32_t s_mergedPsConstants[0x400];
  if (g_livePsFloatConstants != nullptr &&
      g_pm4PsConstantsValid.load(std::memory_order_relaxed)) {
    const uint64_t *cover = (noPos || kCpAccumulate3D)
                                ? g_pm4PsConstantsCoverage
                                : g_pm4PsConstantsFreshCoverage;
    std::memcpy(s_mergedPsConstants, g_livePsFloatConstants,
                sizeof(s_mergedPsConstants));
    bool any = false;
    for (uint32_t w = 0; w < 4u; ++w) {
      uint64_t bits = cover[w];
      while (bits != 0) {
        const uint32_t reg = w * 64u + uint32_t(std::countr_zero(bits));
        bits &= bits - 1u;
        std::memcpy(s_mergedPsConstants + reg * 4u, g_pm4PsConstants + reg * 4u,
                    16u);
        any = true;
      }
    }
    if (any || noPos)
      psConstSrc = s_mergedPsConstants;
  }
  SetRootDescriptor(g_uploadAllocator.allocateCopy<true>(
                        psConstSrc,
                        sizeof(device->pixelShaderFloatConstants), 0x100),
                    1);
  SetRootDescriptor(g_uploadAllocator.allocateCopy<false>(
                        &g_sharedConstants, sizeof(g_sharedConstants), 0x100),
                    2);

  if (g_dirtyStates.vertexStreamFirst <= g_dirtyStates.vertexStreamLast) {
    commandList->setVertexBuffers(
        g_dirtyStates.vertexStreamFirst,
        g_vertexBufferViews + g_dirtyStates.vertexStreamFirst,
        g_dirtyStates.vertexStreamLast - g_dirtyStates.vertexStreamFirst + 1,
        g_inputSlots + g_dirtyStates.vertexStreamFirst);
  }
  if (g_dirtyStates.indices && g_indexBufferView.buffer.ref != nullptr) {
    // Defense-in-depth: an index buffer can only be R16_UINT/R32_UINT. A color
    // format here (e.g. R8G8B8A8_UNORM from a mis-converted buffer) makes
    // IASetIndexBuffer fail silently -> no indices -> no geometry.
    if (g_indexBufferView.format != RenderFormat::R16_UINT &&
        g_indexBufferView.format != RenderFormat::R32_UINT)
      g_indexBufferView.format = RenderFormat::R16_UINT;
    commandList->setIndexBuffer(&g_indexBufferView);
  }

  if (g_pipelineBound && renderTarget != nullptr) {
    g_lastTouchedRenderTarget = renderTarget;
    TrackRecentRenderTarget(renderTarget);
  }

  g_dirtyStates = DirtyStates(false);
}

void SetAlphaTestMode(bool enable) {
  uint32_t specConstants = enable ? SPEC_CONSTANT_ALPHA_TEST : 0;
  specConstants |=
      g_pipelineState.specConstants &
      ~(SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants,
                specConstants);
}

// QUADLIST / TRIANGLEFAN re-index helper (builds 16-bit index data on demand).
template <uint32_t PrimitiveType> struct PrimitiveIndexData {
  std::vector<uint16_t> indexData;

  uint32_t prepare(uint32_t guestPrimCount) {
    uint32_t primCount;
    uint32_t indexCountPerPrimitive;
    if constexpr (PrimitiveType == D3DPT_TRIANGLEFAN) {
      primCount = guestPrimCount - 2;
      indexCountPerPrimitive = 3;
    } else {
      primCount = guestPrimCount / 4;
      indexCountPerPrimitive = 6;
    }
    uint32_t indexCount = primCount * indexCountPerPrimitive;

    if (indexData.size() < indexCount) {
      size_t oldPrimCount = indexData.size() / indexCountPerPrimitive;
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

    UploadResult allocation = g_uploadAllocator.allocateCopy<false>(
        indexData.data(), indexCount * 2, 2);
    g_indexBufferView.buffer = allocation.buffer->at(allocation.offset);
    g_indexBufferView.size = indexCount * 2;
    g_indexBufferView.format = RenderFormat::R16_UINT;
    g_dirtyStates.indices = true;
    return indexCount;
  }
};

PrimitiveIndexData<D3DPT_TRIANGLEFAN> g_triangleFanIndexData;
PrimitiveIndexData<D3DPT_QUADLIST> g_quadIndexData;

void SetPrimitiveType(uint32_t primitiveType) {
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.primitiveTopology,
                ConvertPrimitiveType(primitiveType));
}

// Match the bound vertex shader's header usage/usageIndex set to one of FM2's
// created D3DVERTEXELEMENT9 declarations (which carry the real format/offset).
// FM2 never binds the declaration via the device field, so this recovers the
// input layout. Returns nullptr if no declaration covers all the shader inputs.
GuestVertexDeclaration *MatchDeclarationForShader(GuestShader *vs,
                                                  uint32_t streamStride) {
  if (vs == nullptr || vs->headerElements.empty())
    return nullptr;
  const std::vector<GuestVertexDeclaration *> decls = SnapshotGameDeclarations();
  GuestVertexDeclaration *best = nullptr;
  int bestScore = -1;
  for (GuestVertexDeclaration *decl : decls) {
    if (decl == nullptr || decl->vertexElements == nullptr ||
        decl->vertexElementCount == 0)
      continue;
    bool covers = true;
    for (const ShaderHeaderElement &he : vs->headerElements) {
      bool found = false;
      for (uint32_t i = 0; i < decl->vertexElementCount; ++i) {
        const GuestVertexElement &e = decl->vertexElements[i];
        if (e.usage == he.usage && e.usageIndex == he.usageIndex) {
          found = true;
          break;
        }
      }
      if (!found) {
        covers = false;
        break;
      }
    }
    if (!covers)
      continue;
    uint32_t maxOff = 0;
    for (uint32_t i = 0; i < decl->vertexElementCount; ++i)
      maxOff = std::max<uint32_t>(maxOff, decl->vertexElements[i].offset);
    int score = 0;
    if (decl->vertexElementCount == vs->headerElements.size())
      score += 100000; // exact element-set match
    if (streamStride != 0 && maxOff < streamStride)
      score += int(maxOff); // prefer the most tightly packed decl that fits
    if (score > bestScore) {
      bestScore = score;
      best = decl;
    }
  }
  return best;
}

void SyncVertexDeclarationFromDevice(GuestDevice *device) {
  if (device == nullptr)
    return;
  uint32_t guestDeclaration = device->vertexDeclaration.get();
  // DIAG: settle whether the per-draw vertex declaration bind (guest device
  // +0x2E24) is set at draw time. First 24 only.
  {
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 24) {
      GuestVertexDeclaration *resolved = nullptr;
      if (guestDeclaration != 0) {
        resolved = ghp::ToHost<GuestVertexDeclaration>(guestDeclaration);
        if (!IsFm2Resource(resolved))
          resolved = LookupVertexDeclarationAlias(guestDeclaration);
      }
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(f,
                     "FM2_SYNC_DECL guestDecl=0x%08X resolved=%p inElems=%u\n",
                     guestDeclaration, (void *)resolved,
                     resolved ? resolved->inputElementCount : 0u);
        std::fflush(f);
        std::fclose(f);
      }
    }
  }
  if (guestDeclaration == 0) {
    // FM2 never binds the declaration via the device field. Recover it by
    // matching the bound vertex shader's embedded header usage set to one of the
    // declarations FM2 created (which carry the real format/offset).
    GuestVertexDeclaration *matched = MatchDeclarationForShader(
        g_pipelineState.vertexShader, g_inputSlots[0].stride);
    if (matched != nullptr) {
      SetVertexDeclaration(device, matched);
      static std::atomic<uint32_t> s_m{0};
      if (g_inputSlots[0].stride != 0 &&
          s_m.fetch_add(1, std::memory_order_relaxed) < 32) {
        if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
          const GuestShader *vs = g_pipelineState.vertexShader;
          const uint64_t h =
              vs->shaderCacheEntry ? vs->shaderCacheEntry->hash : 0ull;
          std::fprintf(f, "FM2_DECLMATCH hash=0x%016llX stride=%u declEls=%u shdr[",
                       (unsigned long long)h, g_inputSlots[0].stride,
                       matched->vertexElementCount);
          for (const auto &he : vs->headerElements)
            std::fprintf(f, "%u.%u ", he.usage, he.usageIndex);
          std::fprintf(f, "] decl[");
          for (uint32_t i = 0; i < matched->vertexElementCount; ++i) {
            const auto &e = matched->vertexElements[i];
            std::fprintf(f, "u%u.%u@%u/t%u/s%u ", e.usage, e.usageIndex, e.offset,
                         e.type, e.stream);
          }
          std::fprintf(f, "]\n");
          std::fflush(f);
          std::fclose(f);
        }
      }
    }
    return;
  }
  GuestVertexDeclaration *declaration =
      ghp::ToHost<GuestVertexDeclaration>(guestDeclaration);
  if (!IsFm2Resource(declaration))
    declaration = LookupVertexDeclarationAlias(guestDeclaration);
  SetVertexDeclaration(device, declaration);
}

void RestoreVertexDeclarationForShader(GuestDevice *device) {
  if (g_pipelineState.vertexDeclaration != nullptr ||
      g_pipelineState.vertexShader == nullptr)
    return;
  auto it = g_vertexShaderDeclarations.find(g_pipelineState.vertexShader);
  if (it != g_vertexShaderDeclarations.end())
    SetVertexDeclaration(device, it->second);
}

GuestVertexDeclaration *SimpleElementDeclaration() {
  if (g_simpleElementDeclaration != nullptr)
    return g_simpleElementDeclaration.get();

  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 4;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(5);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {
      0, 16, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[2] = {0, 24, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_COLOR,
                             0, 0};
  decl->vertexElements[3] = {0, 40, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,
                             1, 0};
  decl->vertexElements[4] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_simpleElementDeclaration = std::move(decl);
  return g_simpleElementDeclaration.get();
}

GuestVertexDeclaration *TexturedQuadDeclaration() {
  if (g_texturedQuadDeclaration != nullptr)
    return g_texturedQuadDeclaration.get();

  // UE3 canvas/text vertex (20 bytes): FVector2D Position, FColor Color,
  // FVector2D UV.
  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 3;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(4);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {0, 8, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,
                             0, 0};
  decl->vertexElements[2] = {
      0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[3] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_texturedQuadDeclaration = std::move(decl);
  return g_texturedQuadDeclaration.get();
}

GuestVertexDeclaration *MaterialVertexDeclaration() {
  if (g_materialVertexDeclaration != nullptr)
    return g_materialVertexDeclaration.get();

  // UE3 FMaterialTileVertex (32 bytes): FVector Position, FPackedNormal
  // TangentX, FPackedNormal TangentZ, DWORD Color, FVector2D UV.
  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 5;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(6);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {
      0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_TANGENT, 0, 0};
  decl->vertexElements[2] = {
      0, 16, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_NORMAL, 0, 0};
  decl->vertexElements[3] = {0, 20, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,
                             0, 0};
  decl->vertexElements[4] = {
      0, 24, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[5] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_materialVertexDeclaration = std::move(decl);
  return g_materialVertexDeclaration.get();
}

GuestVertexDeclaration *DynamicMeshVertexDeclaration() {
  if (g_dynamicMeshVertexDeclaration != nullptr)
    return g_dynamicMeshVertexDeclaration.get();

  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 5;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(6);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {
      0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[2] = {
      0, 20, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_TANGENT, 0, 0};
  decl->vertexElements[3] = {
      0, 24, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_NORMAL, 0, 0};
  decl->vertexElements[4] = {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,
                             0, 0};
  decl->vertexElements[5] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_dynamicMeshVertexDeclaration = std::move(decl);
  return g_dynamicMeshVertexDeclaration.get();
}

GuestVertexDeclaration *BatchedTriangleVertexDeclaration() {
  if (g_batchedTriangleVertexDeclaration != nullptr)
    return g_batchedTriangleVertexDeclaration.get();

  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 6;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(7);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {
      0, 16, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_TANGENT, 0, 0};
  decl->vertexElements[2] = {
      0, 20, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BINORMAL, 0, 0};
  decl->vertexElements[3] = {
      0, 24, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_NORMAL, 0, 0};
  decl->vertexElements[4] = {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,
                             0, 0};
  decl->vertexElements[5] = {
      0, 32, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[6] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_batchedTriangleVertexDeclaration = std::move(decl);
  return g_batchedTriangleVertexDeclaration.get();
}

GuestVertexDeclaration *GpuSkin40VertexDeclaration() {
  if (g_gpuSkin40VertexDeclaration != nullptr)
    return g_gpuSkin40VertexDeclaration.get();

  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 7;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(8);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {
      0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_TANGENT, 0, 0};
  decl->vertexElements[2] = {
      0, 16, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_NORMAL, 0, 0};
  decl->vertexElements[3] = {
      0, 20, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BINORMAL, 0, 0};
  decl->vertexElements[4] = {
      0, 24, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[5] = {
      0, 32, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0, 0};
  decl->vertexElements[6] = {
      0, 36, D3DDECLTYPE_UBYTE4N, 0, D3DDECLUSAGE_BLENDWEIGHT, 0, 0};
  decl->vertexElements[7] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_gpuSkin40VertexDeclaration = std::move(decl);
  return g_gpuSkin40VertexDeclaration.get();
}

GuestVertexDeclaration *ScreenQuadDeclaration() {
  if (g_screenQuadDeclaration != nullptr)
    return g_screenQuadDeclaration.get();

  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 4;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(5);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {
      0, 16, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[2] = {0, 24, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,
                             0, 0};
  decl->vertexElements[3] = {
      0, 24, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 1, 0};
  decl->vertexElements[4] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_screenQuadDeclaration = std::move(decl);
  return g_screenQuadDeclaration.get();
}

GuestVertexDeclaration *ParticleSpriteDeclaration() {
  if (g_particleSpriteDeclaration != nullptr)
    return g_particleSpriteDeclaration.get();

  // Xbox UE3 has PARTICLES_USE_INDEXED_SPRITES enabled:
  // FParticleSpriteVertex is 60 bytes and omits per-corner TEXCOORD0.
  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 5;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(6);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL,
                             0, 0};
  decl->vertexElements[2] = {0, 24, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TANGENT,
                             0, 0};
  decl->vertexElements[3] = {
      0, 36, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0, 0};
  decl->vertexElements[4] = {
      0, 44, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1, 0};
  decl->vertexElements[5] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_particleSpriteDeclaration = std::move(decl);
  return g_particleSpriteDeclaration.get();
}

GuestVertexDeclaration *ParticleSpriteDynamicDeclaration() {
  if (g_particleSpriteDynamicDeclaration != nullptr)
    return g_particleSpriteDynamicDeclaration.get();

  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 6;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(7);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL,
                             0, 0};
  decl->vertexElements[2] = {0, 24, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TANGENT,
                             0, 0};
  decl->vertexElements[3] = {
      0, 36, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0, 0};
  decl->vertexElements[4] = {
      0, 44, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1, 0};
  decl->vertexElements[5] = {
      0, 60, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 3, 0};
  decl->vertexElements[6] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_particleSpriteDynamicDeclaration = std::move(decl);
  return g_particleSpriteDynamicDeclaration.get();
}

GuestVertexDeclaration *ParticleSubUVDeclaration() {
  if (g_particleSubUVDeclaration != nullptr)
    return g_particleSubUVDeclaration.get();

  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 7;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(8);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL,
                             0, 0};
  decl->vertexElements[2] = {0, 24, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TANGENT,
                             0, 0};
  decl->vertexElements[3] = {
      0, 36, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0, 0};
  decl->vertexElements[4] = {
      0, 44, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1, 0};
  decl->vertexElements[5] = {
      0, 60, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[6] = {
      0, 76, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 2, 0};
  decl->vertexElements[7] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_particleSubUVDeclaration = std::move(decl);
  return g_particleSubUVDeclaration.get();
}

GuestVertexDeclaration *ParticleSubUVDynamicDeclaration() {
  if (g_particleSubUVDynamicDeclaration != nullptr)
    return g_particleSubUVDynamicDeclaration.get();

  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 8;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(9);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL,
                             0, 0};
  decl->vertexElements[2] = {0, 24, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TANGENT,
                             0, 0};
  decl->vertexElements[3] = {
      0, 36, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0, 0};
  decl->vertexElements[4] = {
      0, 44, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1, 0};
  decl->vertexElements[5] = {
      0, 60, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[6] = {
      0, 76, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 2, 0};
  decl->vertexElements[7] = {
      0, 92, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 3, 0};
  decl->vertexElements[8] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_particleSubUVDynamicDeclaration = std::move(decl);
  return g_particleSubUVDynamicDeclaration.get();
}

GuestVertexDeclaration *ParticleBeamTrailDeclaration() {
  if (g_particleBeamTrailDeclaration != nullptr)
    return g_particleBeamTrailDeclaration.get();

  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 6;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(7);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL,
                             0, 0};
  decl->vertexElements[2] = {0, 24, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TANGENT,
                             0, 0};
  decl->vertexElements[3] = {
      0, 36, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0, 0};
  decl->vertexElements[4] = {
      0, 44, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1, 0};
  decl->vertexElements[5] = {
      0, 60, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[6] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_particleBeamTrailDeclaration = std::move(decl);
  return g_particleBeamTrailDeclaration.get();
}

GuestVertexDeclaration *ParticleBeamTrailDynamicDeclaration() {
  if (g_particleBeamTrailDynamicDeclaration != nullptr)
    return g_particleBeamTrailDynamicDeclaration.get();

  auto decl = std::make_unique<GuestVertexDeclaration>();
  decl->vertexElementCount = 7;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(8);
  decl->vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION,
                             0, 0};
  decl->vertexElements[1] = {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL,
                             0, 0};
  decl->vertexElements[2] = {0, 24, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TANGENT,
                             0, 0};
  decl->vertexElements[3] = {
      0, 36, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0, 0};
  decl->vertexElements[4] = {
      0, 44, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1, 0};
  decl->vertexElements[5] = {
      0, 60, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 0, 0};
  decl->vertexElements[6] = {
      0, 76, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 2, 0};
  decl->vertexElements[7] = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0, 0};
  decl->vertexStreams[0] = true;

  g_particleBeamTrailDynamicDeclaration = std::move(decl);
  return g_particleBeamTrailDynamicDeclaration.get();
}

GuestVertexDeclaration *
SelectParticleVertexDeclarationByStride(uint32_t primitiveType,
                                        uint32_t vertexStride) {
  const bool triangleList = primitiveType == D3DPT_TRIANGLELIST;
  const bool triangleStrip = primitiveType == D3DPT_TRIANGLESTRIP;

  if (triangleStrip) {
    if (vertexStride == 76)
      return ParticleBeamTrailDeclaration();
    if (vertexStride == 92)
      return ParticleBeamTrailDynamicDeclaration();
  } else if (triangleList) {
    if (vertexStride == 60)
      return ParticleSpriteDeclaration();
    if (vertexStride == 76)
      return ParticleSpriteDynamicDeclaration();
    if (vertexStride == 92)
      return ParticleSubUVDeclaration();
    if (vertexStride == 108)
      return ParticleSubUVDynamicDeclaration();
  }

  return nullptr;
}

bool SelectVertexDeclaration(GuestDevice *device,
                             GuestVertexDeclaration *substitute,
                             GuestVertexDeclaration **previous) {
  if (substitute == nullptr || g_pipelineState.vertexDeclaration == substitute)
    return false;

  *previous = g_pipelineState.vertexDeclaration;
  SetVertexDeclaration(device, substitute);
  return true;
}


GuestVertexDeclaration *SelectStride32QuadDeclaration(const void *vertexData,
                                                      uint32_t minVertexIndex,
                                                      uint32_t numVertices) {
  if (vertexData == nullptr)
    return MaterialVertexDeclaration();
  auto dwordAt = [vertexData, minVertexIndex](uint32_t vertex, uint32_t word) {
    const uint32_t *v = reinterpret_cast<const uint32_t *>(
        reinterpret_cast<const uint8_t *>(vertexData) +
        size_t(minVertexIndex + vertex) * 32);
    return std::byteswap(v[word]);
  };
  const uint32_t *v = reinterpret_cast<const uint32_t *>(
      reinterpret_cast<const uint8_t *>(vertexData) +
      size_t(minVertexIndex) * 32);
  const uint32_t stride = 32 / 4;
  bool pos4 = std::byteswap(v[3]) == 0x3F800000u;
  if (numVertices > 1)
    pos4 = pos4 && std::byteswap(v[stride + 3]) == 0x3F800000u;
  if (pos4 && numVertices >= 4) {
    bool words16Vary = false;
    bool words24Vary = false;
    const uint32_t count = std::min<uint32_t>(numVertices, 4);
    const uint32_t word4 = dwordAt(0, 4);
    const uint32_t word5 = dwordAt(0, 5);
    const uint32_t word6 = dwordAt(0, 6);
    const uint32_t word7 = dwordAt(0, 7);
    for (uint32_t i = 1; i < count; ++i) {
      words16Vary |= dwordAt(i, 4) != word4 || dwordAt(i, 5) != word5;
      words24Vary |= dwordAt(i, 6) != word6 || dwordAt(i, 7) != word7;
    }
    if (!words16Vary && words24Vary)
      return MaterialVertexDeclaration();
  }
  return pos4 ? ScreenQuadDeclaration() : MaterialVertexDeclaration();
}

bool IsLegacyTexturedQuadDeclaration(GuestVertexDeclaration *declaration) {
  if (declaration == nullptr || declaration->vertexElementCount < 3)
    return false;

  const GuestVertexElement &position = declaration->vertexElements[0];
  const GuestVertexElement &color = declaration->vertexElements[1];
  const GuestVertexElement &texcoord = declaration->vertexElements[2];
  return position.stream == 0 && position.offset == 0 &&
         position.type == D3DDECLTYPE_FLOAT2 &&
         position.usage == D3DDECLUSAGE_POSITION && position.usageIndex == 0 &&
         color.stream == 0 && color.offset == 8 &&
         color.type == D3DDECLTYPE_D3DCOLOR &&
         color.usage == D3DDECLUSAGE_COLOR && color.usageIndex == 0 &&
         texcoord.stream == 0 && texcoord.offset == 12 &&
         texcoord.type == D3DDECLTYPE_FLOAT2 &&
         texcoord.usage == D3DDECLUSAGE_TEXCOORD && texcoord.usageIndex == 0;
}

bool SelectIndexedMeshDeclarationForStaleCanvasDecl(
    GuestDevice *device, uint32_t primitiveType, uint32_t indexCount,
    uint32_t vertexStride, GuestVertexDeclaration **previous) {
  if (primitiveType != D3DPT_TRIANGLELIST || vertexStride != 40)
    return false;
  if (!IsLegacyTexturedQuadDeclaration(g_pipelineState.vertexDeclaration))
    return false;

  GuestVertexDeclaration *declaration = indexCount == 336
                                            ? BatchedTriangleVertexDeclaration()
                                            : DynamicMeshVertexDeclaration();
  *previous = g_pipelineState.vertexDeclaration;
  SetVertexDeclaration(device, declaration);
  return true;
}

bool IsGpuSkin40DeclarationWithPackedTexcoord(
    GuestVertexDeclaration *declaration) {
  if (declaration == nullptr)
    return false;

  bool hasPosition = false;
  bool hasPackedTexcoord = false;
  bool hasBlendIndices = false;
  bool hasBlendWeight = false;
  for (uint32_t i = 0; i < declaration->vertexElementCount; ++i) {
    const GuestVertexElement &e = declaration->vertexElements[i];
    if (e.stream == 0xFF || e.type == D3DDECLTYPE_UNUSED)
      break;
    hasPosition |= e.stream == 0 && e.offset == 0 &&
                   e.type == D3DDECLTYPE_FLOAT3 &&
                   e.usage == D3DDECLUSAGE_POSITION && e.usageIndex == 0;
    hasPackedTexcoord |= e.stream == 0 && e.offset == 12 &&
                         e.type == D3DDECLTYPE_FLOAT2 &&
                         e.usage == D3DDECLUSAGE_TEXCOORD && e.usageIndex == 0;
    hasBlendIndices |=
        e.stream == 0 && e.offset == 32 && e.type == D3DDECLTYPE_UBYTE4 &&
        e.usage == D3DDECLUSAGE_BLENDINDICES && e.usageIndex == 0;
    hasBlendWeight |= e.stream == 0 && e.offset == 36 &&
                      e.type == D3DDECLTYPE_UBYTE4N &&
                      e.usage == D3DDECLUSAGE_BLENDWEIGHT && e.usageIndex == 0;
  }
  return hasPosition && hasPackedTexcoord && hasBlendIndices && hasBlendWeight;
}

bool SelectGpuSkin40Declaration(GuestDevice *device, uint32_t primitiveType,
                                uint32_t vertexStride,
                                GuestVertexDeclaration **previous) {
  if (primitiveType != D3DPT_TRIANGLELIST || vertexStride != 40)
    return false;
  if (!IsGpuSkin40DeclarationWithPackedTexcoord(
          g_pipelineState.vertexDeclaration))
    return false;

  *previous = g_pipelineState.vertexDeclaration;
  SetVertexDeclaration(device, GpuSkin40VertexDeclaration());
  return true;
}

bool SelectTexturedQuadVertexDeclaration(GuestDevice *device,
                                         uint32_t primitiveType,
                                         uint32_t primitiveCount,
                                         GuestVertexDeclaration **previous) {
  if (!((primitiveType == D3DPT_TRIANGLEFAN ||
         primitiveType == D3DPT_TRIANGLESTRIP) &&
        (primitiveCount == 4 || primitiveCount == 6))) {
    return false;
  }
  if (!IsLegacyTexturedQuadDeclaration(g_pipelineState.vertexDeclaration))
    return false;

  *previous = g_pipelineState.vertexDeclaration;
  SetVertexDeclaration(device, TexturedQuadDeclaration());
  return true;
}

bool SelectShaderVertexDeclaration(GuestDevice *device, uint32_t vertexStride,
                                   GuestVertexDeclaration **previous) {
  *previous = nullptr;
  if (g_pipelineState.vertexShader == nullptr ||
      g_pipelineState.vertexShader->shaderCacheEntry == nullptr) {
    return false;
  }

  const uint64_t hash = g_pipelineState.vertexShader->shaderCacheEntry->hash;
  if (hash == kSimpleElementVertexShaderHash &&
      IsSimpleElementVertexStride(vertexStride)) {
    *previous = g_pipelineState.vertexDeclaration;
    SetVertexDeclaration(device, SimpleElementDeclaration());
    return true;
  }
  return false;
}


bool SelectQuadVertexDeclarationByStride(GuestDevice *device,
                                         uint32_t primitiveType,
                                         uint32_t primitiveCount,
                                         uint32_t vertexStride,
                                         GuestVertexDeclaration **previous,
                                         const void *vertexData = nullptr) {
  const bool singleQuad = (primitiveType == D3DPT_TRIANGLEFAN ||
                           primitiveType == D3DPT_TRIANGLESTRIP) &&
                          (primitiveCount == 4 || primitiveCount == 6);
  const bool textBatch = vertexStride == 20 &&
                         primitiveType == D3DPT_TRIANGLELIST &&
                         primitiveCount >= 6 && primitiveCount % 6 == 0;
  if (!singleQuad && !textBatch) {
    return false;
  }
  GuestVertexDeclaration *substitute = nullptr;
  if (vertexStride == 20) {
    substitute = TexturedQuadDeclaration();
  } else if (vertexStride == 32) {
    // With guest data available (UP draws), sniff apart the two 32-byte quad
    // structs; otherwise assume FMaterialTileVertex.
    substitute =
        vertexData != nullptr
            ? SelectStride32QuadDeclaration(vertexData, 0, primitiveCount)
            : MaterialVertexDeclaration();
  } else if (IsSimpleElementVertexStride(vertexStride)) {
    substitute = SimpleElementDeclaration();
  }
  if (substitute == nullptr || g_pipelineState.vertexDeclaration == substitute)
    return false;

  *previous = g_pipelineState.vertexDeclaration;
  SetVertexDeclaration(device, substitute);
  return true;
}

// Instancing: the index buffer is fed as a vertex stream (POSITION1 stream).
uint32_t CheckInstancing() {
  uint32_t indexCount = 0;
  if (g_pipelineState.vertexDeclaration == nullptr)
    return 0;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.instancing,
                g_pipelineState.vertexDeclaration->indexVertexStream != 0);
  if (g_pipelineState.instancing)
    indexCount = g_vertexBufferViews[g_pipelineState.vertexDeclaration
                                         ->indexVertexStream]
                     .size /
                 4;
  return indexCount;
}

void UnsetInstancingStream() {
  if (g_pipelineState.vertexDeclaration == nullptr)
    return;
  bool dirty = false;
  uint32_t index = g_pipelineState.vertexDeclaration->indexVertexStream;
  SetDirtyValue(dirty, g_vertexBufferViews[index].buffer,
                RenderBufferReference{});
  SetDirtyValue(dirty, g_vertexBufferViews[index].size, 0u);
  SetDirtyValue(dirty, g_inputSlots[index].stride, 0u);
  if (dirty) {
    g_dirtyStates.vertexStreamFirst =
        std::min<uint8_t>(g_dirtyStates.vertexStreamFirst, index);
    g_dirtyStates.vertexStreamLast =
        std::max<uint8_t>(g_dirtyStates.vertexStreamLast, index);
  }
}

} // namespace

// session 6P-3: mirror of the render-pass VS constant uploads
// (FM2_RenderContext_UploadMatrixConstants). Public (called from the d3d_hooks
// REX_HOOK); writes into the anon-namespace g_passVsConstants the flush reads.
void MirrorPassVsConstants(uint32_t startRegister, const void *src,
                           uint32_t vector4fCount) {
  if (src == nullptr || startRegister >= 0x100u || vector4fCount == 0)
    return;
  // Register indexing matches the 0x700-based block convention our
  // XenosRecomp shaders are compiled against (SetVertexShaderConstantFN's
  // API register D == shader-table register D). A "-1 hardware alignment"
  // correction was tried 2026-07-02 and shifted every shader's constants ->
  // all black; see the PM4 draw hooks' comment.
  const uint32_t count = std::min(vector4fCount, 0x100u - startRegister);
  std::memcpy(g_passVsConstants + startRegister * 4u, src, count * 16u);
  for (uint32_t r = startRegister; r < startRegister + count; ++r)
    g_passVsConstantsCoverage[r / 64u] |= uint64_t(1) << (r % 64u);
  g_passVsConstantsValid.store(true, std::memory_order_relaxed);
}

void SetLiveFloatConstantFiles(const void *vsFile, const void *psFile) {
  g_liveVsFloatConstants = static_cast<const uint32_t *>(vsFile);
  g_livePsFloatConstants = static_cast<const uint32_t *>(psFile);
}

// 2026-07-02 session 3: PM4 ALU constant apply (see render_internal.h). The
// scanner hands us the packet payload verbatim (big-endian dwords, same
// layout as the live context file), dword-indexed into the 256-vec4 VS file.
void ApplyPm4VsConstants(uint32_t dwordIndex, const void *beDwords,
                         uint32_t dwordCount) {
  if (beDwords == nullptr || dwordIndex >= 0x400u || dwordCount == 0)
    return;
  dwordCount = std::min(dwordCount, 0x400u - dwordIndex);
  std::memcpy(g_pm4VsConstants + dwordIndex, beDwords, dwordCount * 4u);
  const uint32_t lastReg = (dwordIndex + dwordCount - 1u) / 4u;
  for (uint32_t r = dwordIndex / 4u; r <= lastReg; ++r) {
    g_pm4VsConstantsCoverage[r / 64u] |= uint64_t(1) << (r % 64u);
    g_pm4VsConstantsFreshCoverage[r / 64u] |= uint64_t(1) << (r % 64u);
  }
  g_pm4VsConstantsValid.store(true, std::memory_order_relaxed);
}

// PS twin of ApplyPm4VsConstants (packet idx already rebased to 0).
void ApplyPm4PsConstants(uint32_t dwordIndex, const void *beDwords,
                         uint32_t dwordCount) {
  if (beDwords == nullptr || dwordIndex >= 0x400u || dwordCount == 0)
    return;
  dwordCount = std::min(dwordCount, 0x400u - dwordIndex);
  std::memcpy(g_pm4PsConstants + dwordIndex, beDwords, dwordCount * 4u);
  const uint32_t lastReg = (dwordIndex + dwordCount - 1u) / 4u;
  for (uint32_t r = dwordIndex / 4u; r <= lastReg; ++r) {
    g_pm4PsConstantsCoverage[r / 64u] |= uint64_t(1) << (r % 64u);
    g_pm4PsConstantsFreshCoverage[r / 64u] |= uint64_t(1) << (r % 64u);
  }
  g_pm4PsConstantsValid.store(true, std::memory_order_relaxed);
}

// Called by the scanner before each command-buffer delta walk: the "fresh"
// register set only ever describes the delta between the previous PM4 draw
// and this one.
void BeginPm4ConstantDelta() {
  for (uint32_t w = 0; w < 4u; ++w) {
    g_pm4VsConstantsFreshCoverage[w] = 0;
    g_pm4PsConstantsFreshCoverage[w] = 0;
  }
}

void SetUiGlyphModelView(const void *rows4) {
  std::memcpy(g_uiGlyphModelView, rows4, sizeof(g_uiGlyphModelView));
  g_uiGlyphModelViewValid.store(true, std::memory_order_relaxed);
}

// 2026-07-02 session 5: dedicated glyph-placement-matrix shadow (see
// render_internal.h). Called by the scanner ONLY for SET_CONSTANT idx=0
// regs=16 packets.
void SetGlyphPlacementMatrix(const void *beDwords16) {
  if (beDwords16 == nullptr) return;
  std::memcpy(g_glyphPlacementMatrix, beDwords16,
              sizeof(g_glyphPlacementMatrix));
  g_glyphPlacementMatrixValid.store(true, std::memory_order_relaxed);
  // DIAG (session 5, temporary): prove what the dedicated shadow holds --
  // wraps are constant (FM2_PM4WRAP) and stale ring bytes can parse as a
  // fake idx=0 regs=16, so log the c0 row of the first captures + a periodic
  // sample. Floats are big-endian in the payload.
  static std::atomic<uint32_t> s_n{0};
  const uint32_t n = s_n.fetch_add(1, std::memory_order_relaxed);
  if (n < 8 || (n & 0x3FF) == 0) {
    auto bef = [&](uint32_t dw) -> float {
      uint32_t v = g_glyphPlacementMatrix[dw];
      v = ((v >> 24) & 0xFFu) | ((v >> 8) & 0xFF00u) | ((v << 8) & 0xFF0000u) |
          (v << 24);
      float f;
      std::memcpy(&f, &v, 4);
      return f;
    };
    if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
      std::fprintf(f, "FM2_GLYPHMTX_CAP n=%u c0=(%g,%g,%g,%g) c3=(%g,%g,%g,%g)\n",
                   n, bef(0), bef(1), bef(2), bef(3), bef(12), bef(13), bef(14),
                   bef(15));
      std::fclose(f);
    }
  }
}

// ---------------------------------------------------------------------------
// Public surface
// ---------------------------------------------------------------------------

void UpdateClipPlaneConstants(GuestDevice *device) {
  const uint32_t enabledMask = ClipPlaneEnableMask(device);
  g_sharedConstants.clipPlaneEnabled = enabledMask != 0 ? 1 : 0;
  if (enabledMask == 0)
    return;

  const uint32_t planeIndex = std::countr_zero(enabledMask);
  const GuestClipPlane &plane = ClipPlanes(device)[planeIndex];
  g_sharedConstants.clipPlane[0] = plane.x.get();
  g_sharedConstants.clipPlane[1] = plane.y.get();
  g_sharedConstants.clipPlane[2] = plane.z.get();
  g_sharedConstants.clipPlane[3] = plane.w.get();
}

void FlushPendingResolvesForPresent() {
  FlushPendingStretchRects(g_renderTarget, g_depthStencil);
}

uint64_t CurrentFrameIndex() { return g_frameIndex; }

void BeginRenderStateFrame() {
  g_uploadAllocator.reset();
  ++g_frameIndex; // invalidates the per-frame guest vertex/index upload caches
  g_framebuffer = nullptr;
  g_dirtyStates = DirtyStates(true);
  if (!g_sharedConstantsInitialized) {
    for (uint32_t i = 0; i < std::size(g_sharedConstants.texture2DIndices);
         ++i) {
      g_sharedConstants.texture2DIndices[i] = kNullTexture2DDescriptor;
      g_sharedConstants.texture3DIndices[i] = kNullTexture3DDescriptor;
      g_sharedConstants.textureCubeIndices[i] = kNullTextureCubeDescriptor;
    }
    g_sharedConstantsInitialized = true;
  }

  RenderCommandList *commandList = CommandList();
  commandList->setGraphicsPipelineLayout(PipelineLayout());
  commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 0);
  commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 1);
  commandList->setGraphicsDescriptorSet(TextureDescriptorSet(), 2);
  commandList->setGraphicsDescriptorSet(SamplerDescriptorSet(), 3);
}

void SetDepthState(uint32_t zEnable, uint32_t zWriteEnable, uint32_t cmpFunc) {
  const bool ze = zEnable != 0;
  if (g_pipelineState.zEnable != ze)
    g_dirtyStates.renderTargetAndDepthStencil = true;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zEnable, ze);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable,
                zWriteEnable != 0);
  // 2026-07-01: NO FlipCmpFunc. The viewport keeps FM2's reversed depth range
  // (FlushViewport), so the game's compare function is already correct as-is;
  // flipping it here double-reversed the scheme (see Clear comment).
  RenderComparisonFunction zf = ConvertCmpFunc(cmpFunc);
  // TEMP DIAGNOSTIC 2026-07-01 (depth-rejection investigation).
  {
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 32) {
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(f,
                     "FM2_ZFUNC src=SetDepthState raw=%u conv=%d final=%d "
                     "revZ=%d zEnable=%u\n",
                     cmpFunc, int(ConvertCmpFunc(cmpFunc)), int(zf),
                     SceneReverseZ() ? 1 : 0, zEnable);
        std::fclose(f);
      }
    }
  }
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zFunc, zf);
}

void SetStencilState(const GuestStencilState &s) {
  if (g_pipelineState.stencilEnable != s.enable)
    g_dirtyStates.renderTargetAndDepthStencil = true;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilEnable,
                s.enable);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontFunc,
                ConvertCmpFunc(s.frontFunc));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontFail,
                ConvertStencilOp(s.frontFail));
  SetDirtyValue(g_dirtyStates.pipelineState,
                g_pipelineState.stencilFrontDepthFail,
                ConvertStencilOp(s.frontDepthFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFrontPass,
                ConvertStencilOp(s.frontPass));

  const uint32_t backFunc = s.twoSided ? s.backFunc : s.frontFunc;
  const uint32_t backFail = s.twoSided ? s.backFail : s.frontFail;
  const uint32_t backDepthFail =
      s.twoSided ? s.backDepthFail : s.frontDepthFail;
  const uint32_t backPass = s.twoSided ? s.backPass : s.frontPass;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackFunc,
                ConvertCmpFunc(backFunc));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackFail,
                ConvertStencilOp(backFail));
  SetDirtyValue(g_dirtyStates.pipelineState,
                g_pipelineState.stencilBackDepthFail,
                ConvertStencilOp(backDepthFail));
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilBackPass,
                ConvertStencilOp(backPass));

  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState,
                         g_pipelineState.stencilRef, uint8_t(s.ref));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState,
                         g_pipelineState.stencilReadMask, uint8_t(s.readMask));
  SetDirtyValue<uint8_t>(g_dirtyStates.pipelineState,
                         g_pipelineState.stencilWriteMask,
                         uint8_t(s.writeMask));
}

void SetRenderState(GuestDevice *device, uint32_t state, uint32_t value) {
  switch (state) {
  case D3DRS_ZENABLE:
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zEnable,
                  value != 0);
    g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
    break;
  case D3DRS_ZWRITEENABLE:
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable,
                  value != 0);
    break;
  case D3DRS_ALPHATESTENABLE:
    SetAlphaTestMode(value != 0);
    break;
  case D3DRS_SRCBLEND:
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlend,
                  ConvertBlendMode(value));
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
  case D3DRS_ZFUNC: {
    // 2026-07-01: NO FlipCmpFunc -- see SetDepthState/Clear comments. The
    // reversed viewport already carries FM2's reverse-Z scheme faithfully.
    RenderComparisonFunction zf = ConvertCmpFunc(value);
    // TEMP DIAGNOSTIC 2026-07-01 (depth-rejection investigation).
    {
      static std::atomic<uint32_t> s_n{0};
      if (s_n.fetch_add(1, std::memory_order_relaxed) < 32) {
        if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
          std::fprintf(f, "FM2_ZFUNC src=D3DRS_ZFUNC raw=%u conv=%d final=%d revZ=%d\n",
                       value, int(ConvertCmpFunc(value)), int(zf),
                       SceneReverseZ() ? 1 : 0);
          std::fclose(f);
        }
      }
    }
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zFunc, zf);
    break;
  }
  case D3DRS_ALPHAREF:
    g_sharedConstants.alphaThreshold = float(value) / 256.0f;
    break;
  case D3DRS_ALPHABLENDENABLE:
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.alphaBlendEnable,
                  value != 0);
    break;
  case D3DRS_BLENDOP:
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.blendOp,
                  ConvertBlendOp(value));
    break;
  case D3DRS_SCISSORTESTENABLE:
    SetDirtyValue(g_dirtyStates.scissorRect, g_scissorTestEnable, value != 0);
    break;
  case D3DRS_SLOPESCALEDEPTHBIAS:
    SetDirtyValue(g_dirtyStates.pipelineState,
                  g_pipelineState.slopeScaledDepthBias,
                  *reinterpret_cast<float *>(&value));
    break;
  case D3DRS_DEPTHBIAS:
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthBias,
                  int32_t(*reinterpret_cast<float *>(&value) * (1 << 24)));
    break;
  case D3DRS_SRCBLENDALPHA:
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlendAlpha,
                  ConvertBlendMode(value));
    break;
  case D3DRS_DESTBLENDALPHA:
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlendAlpha,
                  ConvertBlendMode(value));
    break;
  case D3DRS_BLENDOPALPHA:
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.blendOpAlpha,
                  ConvertBlendOp(value));
    break;
  case D3DRS_COLORWRITEENABLE:
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.colorWriteEnable,
                  value);
    g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
    break;
  default:
    break;
  }
}

void SetViewportEnable(GuestDevice * /*device*/, uint32_t value) {
  // The Xenos ViewportEnable render state maps to PA_CL_CLIP_CNTL.clip_disable.
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthClipEnabled,
                value != 0);
}

void EnsureShaderResourceDescriptor(GuestBaseTexture *texture) {
  if (texture == nullptr || texture->texture == nullptr)
    return;
  if (texture->descriptorIndex == 0)
    texture->descriptorIndex = AllocTextureDescriptor();
  TextureDescriptorSet()->setTexture(texture->descriptorIndex, texture->texture,
                                     RenderTextureLayout::SHADER_READ,
                                     texture->textureView.get());
}

void BindTextureDescriptor(uint32_t index, GuestBaseTexture *texture,
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

void SetTexture(GuestDevice * /*device*/, uint32_t index,
                GuestTexture *texture) {
  GuestBaseTexture *boundTexture = texture;
  RenderTextureViewDimension viewDimension =
      texture ? texture->viewDimension : RenderTextureViewDimension::UNKNOWN;

  if (texture != nullptr && texture->sourceTexture != nullptr &&
      GetSampleCount(texture->sourceTexture) == RenderSampleCount::COUNT_1) {
    boundTexture = texture->sourceTexture;
    viewDimension = RenderTextureViewDimension::TEXTURE_2D;
  } else if (texture != nullptr && texture->pendingResolveCount != 0) {
    FlushPendingStretchRects(nullptr, nullptr);
  }

  BindTextureDescriptor(index, boundTexture, viewDimension);
  g_textures[index] = texture;
}

void SetTextureBase(GuestDevice * /*device*/, uint32_t index,
                    GuestBaseTexture *texture) {
  if (texture == nullptr || texture->texture == nullptr)
    return;
  GuestBaseTexture *boundTexture =
      (texture->sourceTexture != nullptr &&
       GetSampleCount(texture->sourceTexture) == RenderSampleCount::COUNT_1)
          ? texture->sourceTexture
          : texture;
  BindTextureDescriptor(index, boundTexture,
                        RenderTextureViewDimension::TEXTURE_2D);
  // Not a GuestTexture, so clear any stale alias in g_textures[index].
  g_textures[index] = nullptr;
}

// Snapshot-on-resolve: the game resolves a rendered surface to guest memory and
// later samples it. Aliasing the live RT is wrong because the game reuses/over-
// writes it; we must freeze a COPY at resolve time. Creates/reuses a sampleable
// texture per resolve-dest and copies the source surface into it on the frame
// command list (in-order with the draws). Returns the snapshot to bind.
std::unordered_map<uint32_t, std::unique_ptr<GuestBaseTexture>>
    g_resolveSnapshots;
GuestBaseTexture *SnapshotSurfaceForResolve(GuestBaseTexture *source,
                                            uint32_t destBase) {
  if (source == nullptr || source->texture == nullptr || Device() == nullptr ||
      source->width == 0 || source->height == 0)
    return nullptr;
  // DEBUG ISOLATION (geometry bring-up): forcing surfaces to single-sample made
  // this path run copyTexture on the scene RT (previously skipped as MSAA), and
  // the GPU now hangs (DXGI_ERROR_DEVICE_HUNG / TDR). Skip the snapshot copy to
  // isolate whether the hang is THIS resolve copy or the scene draws themselves;
  // present still shows the scene via GetLastDrawnColorRenderTarget. If geometry
  // appears with this on, the resolve copy is the culprit -> fix it next.
  static constexpr bool kDebugSkipResolveSnapshot = true;
  if (kDebugSkipResolveSnapshot)
    return nullptr;
  // Only snapshot single-sampled sources via a plain copyTexture. MSAA sources
  // need resolveTexture, which was crashing (device-removed) on format/sample
  // mismatch -- skip them (caller falls back to live aliasing for those).
  if (GetSampleCount(source) != RenderSampleCount::COUNT_1)
    return nullptr;
  destBase &= ~0xFFFu;
  auto &slot = g_resolveSnapshots[destBase];
  if (slot == nullptr || slot->width != source->width ||
      slot->height != source->height || slot->format != source->format) {
    auto snap = std::make_unique<GuestBaseTexture>(ResourceType::Texture);
    RenderTextureDesc desc;
    desc.dimension = RenderTextureDimension::TEXTURE_2D;
    desc.width = source->width;
    desc.height = source->height;
    desc.depth = 1;
    desc.mipLevels = 1;
    desc.arraySize = 1;
    desc.format = source->format;
    desc.flags = RenderTextureFlag::NONE;
    snap->textureHolder = Device()->createTexture(desc);
    snap->texture = snap->textureHolder.get();
    if (snap->texture == nullptr)
      return nullptr;
    RenderTextureViewDesc vdesc;
    vdesc.format = source->format;
    vdesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
    vdesc.mipLevels = 1;
    snap->textureView = snap->texture->createTextureView(vdesc);
    snap->width = source->width;
    snap->height = source->height;
    snap->format = source->format;
    snap->layout = RenderTextureLayout::SHADER_READ;
    slot = std::move(snap);
  }
  GuestBaseTexture *dst = slot.get();
  if (dst->texture == source->texture)
    return dst;

  // Mirror ExecutePendingStretchRects exactly (barriers + copy, NO
  // setFramebuffer(nullptr) -- that corrupted the frame command list and crashed
  // later). Plume's barriers end the active render pass implicitly.
  RenderCommandList *commandList = CommandList();
  const RenderSampleCounts sc = GetSampleCount(source);
  if (sc != RenderSampleCount::COUNT_1) {
    AddBarrier(dst, RenderTextureLayout::RESOLVE_DEST);
    FlushBarriers();
    commandList->resolveTexture(dst->texture, source->texture);
  } else {
    AddBarrier(dst, RenderTextureLayout::COPY_DEST);
    AddBarrier(source, RenderTextureLayout::COPY_SOURCE);
    FlushBarriers();
    commandList->copyTexture(dst->texture, source->texture);
  }
  AddBarrier(dst, RenderTextureLayout::SHADER_READ);
  FlushBarriers();
  dst->layout = RenderTextureLayout::SHADER_READ;
  return dst;
}

void SetVertexShader(GuestDevice *device, GuestShader *shader) {
  SyncVertexDeclarationFromDevice(device);
  if (shader != nullptr && g_pipelineState.vertexDeclaration != nullptr)
    g_vertexShaderDeclarations[shader] = g_pipelineState.vertexDeclaration;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexShader,
                shader);
  RestoreVertexDeclarationForShader(device);
}

void SetPixelShader(GuestDevice *device, GuestShader *shader) {
  SyncVertexDeclarationFromDevice(device);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.pixelShader,
                shader);
}

void SetVertexDeclaration(GuestDevice * /*device*/,
                          GuestVertexDeclaration *declaration) {
  if (declaration != nullptr) {
    CompleteVertexDeclaration(declaration);
    g_sharedConstants.swappedTexcoords = declaration->swappedTexcoords;
    g_sharedConstants.swappedBlendWeights = declaration->swappedBlendWeights;
    uint32_t specConstants = g_pipelineState.specConstants;
    if (declaration->hasR11G11B10Normal)
      specConstants |= SPEC_CONSTANT_R11G11B10_NORMAL;
    else
      specConstants &= ~SPEC_CONSTANT_R11G11B10_NORMAL;
    if (declaration->hasUByte4TangentBasis)
      specConstants |= SPEC_CONSTANT_UNPACK_UBYTE4_BASIS;
    else
      specConstants &= ~SPEC_CONSTANT_UNPACK_UBYTE4_BASIS;
    SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants,
                  specConstants);
  }
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexDeclaration,
                declaration);
}

void SetStreamSource(GuestDevice *device, uint32_t index, GuestBuffer *buffer,
                     uint32_t offset, uint32_t stride) {
  SyncVertexDeclarationFromDevice(device);

  // DIAG: for no-POSITION HUD shaders (position derived from TEXCOORD0), dump
  // the raw big-endian guest vertex bytes so we can tell float16-normalized
  // (~0.4) from float32-pixels (~640). The position math is c0*(|tc0.x|+c4.x)
  // with c0.x=1/640, which only renders if tc0 is in pixels. Capped.
  if (index == 0 && buffer != nullptr && buffer->mappedMemory != nullptr &&
      stride != 0) {
    const GuestShader *vs = g_pipelineState.vertexShader;
    bool hud = vs != nullptr && !vs->headerElements.empty();
    if (hud) {
      bool noPos = true;
      for (const auto &he : vs->headerElements)
        if (he.usage == 0)
          noPos = false;
      const uint64_t vhash =
          vs->shaderCacheEntry ? vs->shaderCacheEntry->hash : 0ull;
      hud = noPos || vhash == 0x292FF29403B1DDF8ull;
    }
    if (hud) {
      static std::atomic<uint32_t> s_v{0};
      if (s_v.fetch_add(1, std::memory_order_relaxed) < 12) {
        const uint8_t *p =
            static_cast<const uint8_t *>(buffer->mappedMemory) + offset;
        if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
          const uint64_t h =
              vs->shaderCacheEntry ? vs->shaderCacheEntry->hash : 0ull;
          std::fprintf(f, "FM2_HUDVB hash=0x%016llX stride=%u off=%u bytes=",
                       (unsigned long long)h, stride, offset);
          for (int i = 0; i < 32; ++i)
            std::fprintf(f, "%02X", p[i]);
          std::fprintf(f, "\n");
          std::fflush(f);
          std::fclose(f);
        }
      }
    }
  }

  SetDirtyValue(g_dirtyStates.pipelineState,
                g_pipelineState.vertexStrides[index],
                uint8_t(buffer ? stride : 0));

  bool dirty = false;
  SetDirtyValue(dirty, g_vertexBufferViews[index].buffer,
                buffer ? buffer->buffer->at(offset) : RenderBufferReference{});
  SetDirtyValue(dirty, g_vertexBufferViews[index].size,
                buffer ? (buffer->dataSize - offset) : 0u);
  SetDirtyValue(dirty, g_inputSlots[index].stride, buffer ? stride : 0u);
  if (dirty) {
    g_dirtyStates.vertexStreamFirst =
        std::min<uint8_t>(g_dirtyStates.vertexStreamFirst, index);
    g_dirtyStates.vertexStreamLast =
        std::max<uint8_t>(g_dirtyStates.vertexStreamLast, index);
  }
  // STAGE 3: vertex stream bind -- does the game's vertex buffer have a plume
  // backing, and what size/stride survives into the view?
  {
    static std::atomic<uint32_t> s_ss{0};
    if (s_ss.fetch_add(1, std::memory_order_relaxed) < 24) {
      if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
        std::fprintf(
            f, "FM2_SETVTX idx=%u buf=%p backing=%d size=%u stride=%u off=%u\n",
            index, (const void *)buffer, (buffer && buffer->buffer) ? 1 : 0,
            buffer ? (unsigned)(buffer->dataSize - offset) : 0u,
            (unsigned)(buffer ? stride : 0u), (unsigned)offset);
        std::fclose(f);
      }
    }
  }
}

void SetIndices(GuestDevice * /*device*/, GuestBuffer *buffer) {
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer,
                buffer ? buffer->buffer->at(0) : RenderBufferReference{});
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format,
                buffer ? buffer->format : RenderFormat::R16_UINT);
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size,
                buffer ? buffer->dataSize : 0u);
}

void SetStreamSourceGuestData(GuestDevice *device, uint32_t index,
                              const void *data, uint32_t size,
                              uint32_t stride) {
  SyncVertexDeclarationFromDevice(device);

  SetDirtyValue(g_dirtyStates.pipelineState,
                g_pipelineState.vertexStrides[index], uint8_t(stride));

  GuestDataUpload &entry = g_guestVertexUploads[data];
  if (entry.frame != g_frameIndex || entry.size != size) {
    UploadResult result = UploadGuestVertexData(data, size, 0x10);
    entry.frame = g_frameIndex;
    entry.size = size;
    entry.ref = result.buffer->at(result.offset);
  }

  bool dirty = false;
  SetDirtyValue(dirty, g_vertexBufferViews[index].buffer, entry.ref);
  SetDirtyValue(dirty, g_vertexBufferViews[index].size, size);
  SetDirtyValue(dirty, g_inputSlots[index].stride, stride);
  if (dirty) {
    g_dirtyStates.vertexStreamFirst =
        std::min<uint8_t>(g_dirtyStates.vertexStreamFirst, index);
    g_dirtyStates.vertexStreamLast =
        std::max<uint8_t>(g_dirtyStates.vertexStreamLast, index);
  }
}

void SetIndicesGuestData(GuestDevice * /*device*/, const void *data,
                         uint32_t size, uint32_t indexStride) {
  GuestDataUpload &entry = g_guestIndexUploads[data];
  if (entry.frame != g_frameIndex || entry.size != size) {
    UploadResult result;
    if (indexStride == 4) {
      result = g_uploadAllocator.allocateCopy<true>(
          reinterpret_cast<const uint32_t *>(data), size & ~3u, 4);
    } else {
      result = g_uploadAllocator.allocateCopy<true>(
          reinterpret_cast<const uint16_t *>(data), size & ~1u, 2);
    }
    entry.frame = g_frameIndex;
    entry.size = size;
    entry.ref = result.buffer->at(result.offset);
  }

  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer, entry.ref);
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format,
                indexStride == 4 ? RenderFormat::R32_UINT
                                 : RenderFormat::R16_UINT);
  SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, size);
}

void SetViewport(GuestDevice *device, GuestViewport *viewport) {
  // D3D9 validation: a zero-sized viewport is INVALIDCALL and leaves the
  // state unchanged.
  if (viewport->width.get() == 0 || viewport->height.get() == 0)
    return;
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.x,
                       float(viewport->x.get()));
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.y,
                       float(viewport->y.get()));
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.width,
                       float(viewport->width.get()));
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.height,
                       float(viewport->height.get()));
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.minDepth,
                       viewport->minZ.get());
  SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.maxDepth,
                       viewport->maxZ.get());

  uint32_t specConstants = g_pipelineState.specConstants;
  if (viewport->minZ.get() > viewport->maxZ.get())
    specConstants |= SPEC_CONSTANT_REVERSE_Z;
  else
    specConstants &= ~SPEC_CONSTANT_REVERSE_Z;
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants,
                specConstants);

  g_dirtyStates.scissorRect |= g_dirtyStates.viewport;
}

void SetScissorRect(GuestDevice *device, GuestRect *rect) {
  SetDirtyValue(g_dirtyStates.scissorRect, g_scissorTestEnable,
                ScissorTestEnabled(device));
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.top,
                         rect->top.get());
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.left,
                         rect->left.get());
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.bottom,
                         rect->bottom.get());
  SetDirtyValue<int32_t>(g_dirtyStates.scissorRect, g_scissorRect.right,
                         rect->right.get());
}

void SetRenderTargetInternal(GuestBaseTexture *renderTarget) {
  SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_renderTarget,
                renderTarget);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.renderTargetFormat,
                renderTarget ? renderTarget->format : RenderFormat::UNKNOWN);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.sampleCount,
                GetSampleCount(renderTarget));
  SetAlphaTestMode(
      (g_pipelineState.specConstants &
       (SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE)) != 0);

  // D3D9/Xenon semantics: SetRenderTarget resets the viewport to cover the
  // whole surface.
  if (renderTarget != nullptr && renderTarget->width != 0 &&
      renderTarget->height != 0) {
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.x, 0.0f);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.y, 0.0f);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.width,
                         float(renderTarget->width));
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.height,
                         float(renderTarget->height));
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.minDepth, 0.0f);
    SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.maxDepth, 1.0f);
    g_dirtyStates.scissorRect |= g_dirtyStates.viewport;
  }
}

void SetRenderTarget(GuestDevice * /*device*/, uint32_t index,
                     GuestBaseTexture *renderTarget) {
  if (index != 0)
    return;
  SetRenderTargetInternal(renderTarget ? renderTarget : g_implicitRenderTarget);
}

void SetImplicitRenderTarget(GuestBaseTexture *renderTarget) {
  g_implicitRenderTarget = renderTarget;
  SetRenderTargetInternal(renderTarget);
}

GuestBaseTexture *GetCurrentColorRenderTarget() { return g_renderTarget; }

// The present source should be the color surface that was actually DRAWN into
// (Forza renders the scene into one color RT, e.g. 130C7F000, then binds a
// separate display/resolve buffer that present would otherwise blit empty/black).
// g_scenePresentRT is the RT bound during PM4 scene (3D) draws, captured by the
// PM4 draw path via SetScenePresentRT. GetLastDrawnColorRenderTarget otherwise
// returns the LAST-touched RT, which is the UI/HUD RT (UI draws after the scene),
// so present shows UI not the scene. Prefer the scene RT for present.
GuestBaseTexture *g_scenePresentRT = nullptr;
void SetScenePresentRT(GuestBaseTexture *rt) {
  if (rt != nullptr)
    g_scenePresentRT = rt;
}
// g_lastTouchedRenderTarget tracks the last RT a real draw rendered to; prefer it
// for present, falling back to the currently-bound RT if nothing has been drawn.
GuestBaseTexture *GetLastDrawnColorRenderTarget() {
  if (g_scenePresentRT != nullptr)
    return g_scenePresentRT;
  return g_lastTouchedRenderTarget ? g_lastTouchedRenderTarget : g_renderTarget;
}

GuestBaseTexture *g_testGameTexture = nullptr;
void SetTestGameTexture(GuestBaseTexture *t) {
  if (t != nullptr)
    g_testGameTexture = t;
}
GuestBaseTexture *GetTestGameTexture() { return g_testGameTexture; }

// Ring of the most-recent DISTINCT color render targets that received draws,
// most-recent at index 0. Diagnostic: find which surface holds rendered content
// so present-source selection can show it.
static constexpr uint32_t kRecentRTCount = 6;
GuestBaseTexture *g_recentRTs[kRecentRTCount] = {};
void TrackRecentRenderTarget(GuestBaseTexture *rt) {
  if (rt == nullptr || rt == g_recentRTs[0])
    return;
  for (uint32_t i = kRecentRTCount - 1; i > 0; --i)
    g_recentRTs[i] = g_recentRTs[i - 1];
  g_recentRTs[0] = rt;
}
GuestBaseTexture *GetRecentColorRenderTarget(uint32_t index) {
  return index < kRecentRTCount ? g_recentRTs[index] : nullptr;
}

// VRAM viewer ring: a STABLE set of distinct guest textures the draws sample, in
// first-seen order (cells don't jump so the user can describe them). Refreshes the
// host texture pointer each frame; appends new distinct bases until full.
static constexpr uint32_t kVramViewCount = 12;
struct VramViewEntry {
  GuestBaseTexture *tex = nullptr;
  uint32_t base = 0;
};
static VramViewEntry g_vramView[kVramViewCount] = {};
void RecordVramViewTexture(uint32_t base, GuestBaseTexture *tex) {
  if (tex == nullptr || base == 0)
    return;
  for (uint32_t i = 0; i < kVramViewCount; ++i) {
    if (g_vramView[i].base == base) {
      g_vramView[i].tex = tex; // refresh (content may re-upload)
      return;
    }
    if (g_vramView[i].base == 0) {
      g_vramView[i] = {tex, base}; // append in first-seen order
      return;
    }
  }
  // Full: evict oldest (FIFO) so the grid cycles through textures the game samples
  // over time (captures the menu/car-body passes, not just the first 12). Safe now
  // that only persistent upload-path textures are recorded (apertures excluded) --
  // the earlier dangling crash was from recreated aperture/snapshot surfaces.
  for (uint32_t i = 0; i + 1 < kVramViewCount; ++i)
    g_vramView[i] = g_vramView[i + 1];
  g_vramView[kVramViewCount - 1] = {tex, base};
}
GuestBaseTexture *GetVramViewTexture(uint32_t index, uint32_t *outBase) {
  if (index >= kVramViewCount) {
    if (outBase)
      *outBase = 0;
    return nullptr;
  }
  if (outBase)
    *outBase = g_vramView[index].base;
  return g_vramView[index].tex;
}
uint32_t VramViewCount() { return kVramViewCount; }

void SetDepthStencilSurface(GuestDevice * /*device*/,
                            GuestSurface *depthStencil) {
  SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_depthStencil,
                depthStencil);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthStencilFormat,
                depthStencil ? depthStencil->format : RenderFormat::UNKNOWN);
  // viewport on the next flush.
  g_dirtyStates.viewport = true;
  if (depthStencil != nullptr)
    g_implicitDepthStencil = depthStencil;
}

void Clear(GuestDevice * /*device*/, uint32_t flags, const float *color,
           float z) {
  FlushPendingStretchRects(g_renderTarget, g_depthStencil);

  AddBarrier(g_renderTarget, RenderTextureLayout::COLOR_WRITE);
  AddBarrier(g_depthStencil, RenderTextureLayout::DEPTH_WRITE);
  FlushBarriers();

  bool onePass = (g_renderTarget == nullptr) || (g_depthStencil == nullptr) ||
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

  RenderCommandList *commandList = CommandList();
  if (g_renderTarget != nullptr && (flags & D3DCLEAR_TARGET) != 0) {
    if (!onePass)
      SetFramebuffer(g_renderTarget, nullptr, true);
    // DEBUG (session 6P-2): override the game's clear color to blue so a frame's
    // draws accumulate on a known background. If the presented RT shows ONLY blue,
    // the draws aren't rasterizing; if geometry appears on blue, draws render and
    // the remaining issue is present-source selection. Toggle off when done.
    static constexpr bool kDebugClearColorBlue = false;
    RenderColor clearCol = kDebugClearColorBlue
                               ? RenderColor(0.0f, 0.25f, 1.0f, 1.0f)
                               : RenderColor(color[0], color[1], color[2], color[3]);
    commandList->clearColor(0, clearCol, &clearRect, 1);
    MarkAttachmentInitialized(g_renderTarget);
    g_lastTouchedRenderTarget = g_renderTarget;
  }
  const bool clearDepth = (flags & D3DCLEAR_ZBUFFER) != 0;
  const bool clearStencil = (flags & D3DCLEAR_STENCIL) != 0;
  if (g_depthStencil != nullptr && (clearDepth || clearStencil)) {
    if (!onePass)
      SetFramebuffer(nullptr, g_depthStencil, true);
    // 2026-07-01: pass the game's clear z through UNFLIPPED. FM2 is natively
    // reverse-Z (D24FS8 scene depth, viewport minZ=1/maxZ=0, clears z to 0.0 =
    // far, ZFunc GREATER-family) and FlushViewport already keeps the reversed
    // depth range, so the previous `1.0f - z` flip double-reversed the scheme:
    // buffer cleared to the NEAR value while fragments compared GREATER ->
    // everything depth-rejected. Faithful passthrough is coherent end to end.
    const float depthValue = z;
    // TEMP DIAGNOSTIC 2026-07-01 (depth-rejection investigation): capture the
    // game's raw clear z vs what we submit, plus the reverse-Z decision inputs.
    {
      static std::atomic<uint32_t> s_n{0};
      if (s_n.fetch_add(1, std::memory_order_relaxed) < 32) {
        if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
          std::fprintf(f,
                       "FM2_ZCLEAR flags=0x%X z=%f applied=%f revZ=%d "
                       "dsFmt=0x%08X ds=%p\n",
                       flags, z, depthValue, SceneReverseZ() ? 1 : 0,
                       g_depthStencil->guestFormat,
                       static_cast<void *>(g_depthStencil));
          std::fclose(f);
        }
      }
    }
    commandList->clearDepthStencil(clearDepth, clearStencil, depthValue, 0,
                                   &clearRect, 1);
    MarkAttachmentInitialized(g_depthStencil);
    g_implicitDepthStencil = g_depthStencil;
  }
}

void StretchRect(GuestDevice * /*device*/, uint32_t flags,
                 const GuestRect *source, GuestBaseTexture *destination,
                 const GuestPoint *destPoint) {
  const bool isDepthStencil = (flags & 0x4) != 0;
  GuestBaseTexture *surface =
      isDepthStencil ? static_cast<GuestBaseTexture *>(g_depthStencil)
                     : g_renderTarget;
  if (!isDepthStencil && surface != nullptr &&
      g_lastTouchedRenderTarget != nullptr &&
      surface != g_lastTouchedRenderTarget &&
      surface->width == g_lastTouchedRenderTarget->width &&
      surface->height == g_lastTouchedRenderTarget->height &&
      surface->format == g_lastTouchedRenderTarget->format) {
    surface = g_lastTouchedRenderTarget;
  }
  if (surface == nullptr || surface->texture == nullptr ||
      destination == nullptr || destination->texture == nullptr)
    return;
  if (surface->texture == destination->texture)
    return;

  PendingResolve resolve;
  resolve.destination = destination;
  resolve.hasSourceRect = source != nullptr;
  if (source != nullptr) {
    resolve.sourceRect =
        RenderRect(source->left.get(), source->top.get(), source->right.get(),
                   source->bottom.get());
  }
  if (destPoint != nullptr) {
    resolve.destX = uint32_t(std::max(destPoint->x.get(), int32_t(0)));
    resolve.destY = uint32_t(std::max(destPoint->y.get(), int32_t(0)));
  }
  // Xenos predicated tiling: tile-pass resolves pass pSourceRect in FRAME
  // coordinates while the tile RT only holds the current band (the tile
  // window maps frame row destY to tile row 0). Rebase the rect into tile
  // space when it lies outside the source surface; otherwise
  // ClipResolveRegion clamps it to an empty region and the band is dropped.
  if (resolve.hasSourceRect &&
      (resolve.sourceRect.bottom > int32_t(surface->height) ||
       resolve.sourceRect.right > int32_t(surface->width))) {
    resolve.sourceRect.left -= int32_t(resolve.destX);
    resolve.sourceRect.top -= int32_t(resolve.destY);
    resolve.sourceRect.right -= int32_t(resolve.destX);
    resolve.sourceRect.bottom -= int32_t(resolve.destY);
  }
  // Band resolve = end of a tile pass: remember the tile surface + frame dims
  // and advance the tiling window offset for the NEXT pass (band k resolves at
  // destY=k*tileHeight; the next pass renders the following band, so frame row
  // destY+tileHeight must land at tile row 0). Reset after the last band.
  if (!isDepthStencil && destPoint != nullptr &&
      surface->height < destination->height &&
      surface->width == destination->width) {
    g_tileRenderTarget = surface;
    g_tileFrameWidth = destination->width;
    g_tileFrameHeight = destination->height;
    const uint32_t nextTop = resolve.destY + surface->height;
    const int32_t nextOffset =
        nextTop >= destination->height ? 0 : -int32_t(nextTop);
    if (nextOffset != g_tileViewportOffsetY) {
      g_tileViewportOffsetY = nextOffset;
      g_dirtyStates.viewport = true;
      g_dirtyStates.scissorRect = true;
    }
  }

  const RenderSampleCounts sampleCount = GetSampleCount(surface);
  const bool fullSurface = IsFullSurfaceResolve(surface, destination, resolve);
  destination->sourceTexture =
      fullSurface && sampleCount == RenderSampleCount::COUNT_1 &&
              surface->format == destination->format
          ? surface
          : nullptr;
  ++destination->pendingResolveCount;
  surface->pendingResolves.emplace_back(resolve);
  g_pendingStretchRectSurfaces.emplace(surface);

  // Predicated-tiling band resolves must snapshot the tile NOW: the next tile
  // pass redraws the same physical surface, so a copy deferred to the next
  // flush point would read the wrong pass's content (all bands end up showing
  // the final pass -> the frame repeats vertically per band).
  if (!isDepthStencil && destPoint != nullptr &&
      surface->height < destination->height &&
      surface->width == destination->width) {
    FlushPendingStretchRects(nullptr, nullptr);
    // Fix #18: grow the tile color surface (and its depth partner) to full
    // frame size so the single recorded pass rasterizes every row. From the
    // next frame on: the D3D9 RT-reset viewport covers the whole frame, the
    // frame-coordinate band srcRects fit without rebasing, and the band
    // copies slice the full-height image (this whole branch stops matching).
    if (surface->type == ResourceType::RenderTarget) {
      ResizeTileSurface(static_cast<GuestSurface *>(surface),
                        destination->width, destination->height);
      if (g_depthStencil != nullptr &&
          g_depthStencil->width == destination->width &&
          g_depthStencil->height < destination->height) {
        ResizeTileSurface(g_depthStencil, destination->width,
                          destination->height);
      }
      g_dirtyStates.renderTargetAndDepthStencil = true;
      g_dirtyStates.viewport = true;
      g_dirtyStates.scissorRect = true;
      g_framebuffer = nullptr;
    }
  }

  for (uint32_t i = 0; i < std::size(g_textures); ++i) {
    if (static_cast<GuestBaseTexture *>(g_textures[i]) == destination) {
      if (destination->sourceTexture != nullptr)
        BindTextureDescriptor(i, surface,
                              RenderTextureViewDimension::TEXTURE_2D);
      else
        BindTextureDescriptor(i, destination,
                              RenderTextureViewDimension::TEXTURE_2D);
    }
  }
}

void DrawPrimitive(GuestDevice *device, uint32_t primitiveType,
                   uint32_t startVertex, uint32_t primitiveCount) {
  SyncVertexDeclarationFromDevice(device);
  RestoreVertexDeclarationForShader(device);
  GuestVertexDeclaration *previousDeclaration = nullptr;
  bool restoreDeclaration = SelectShaderVertexDeclaration(
      device, g_inputSlots[0].stride, &previousDeclaration);
  if (!restoreDeclaration) {
    restoreDeclaration =
        SelectVertexDeclaration(device,
                                SelectParticleVertexDeclarationByStride(
                                    primitiveType, g_inputSlots[0].stride),
                                &previousDeclaration);
  }
  if (!restoreDeclaration) {
    restoreDeclaration = SelectGpuSkin40Declaration(
        device, primitiveType, g_inputSlots[0].stride, &previousDeclaration);
  }
  if (!restoreDeclaration) {
    restoreDeclaration = SelectIndexedMeshDeclarationForStaleCanvasDecl(
        device, primitiveType, primitiveCount, g_inputSlots[0].stride,
        &previousDeclaration);
  }
  if (!restoreDeclaration) {
    restoreDeclaration = SelectQuadVertexDeclarationByStride(
        device, primitiveType, primitiveCount, g_inputSlots[0].stride,
        &previousDeclaration);
  }
  if (!restoreDeclaration) {
    restoreDeclaration = SelectTexturedQuadVertexDeclaration(
        device, primitiveType, primitiveCount, &previousDeclaration);
  }
  SetPrimitiveType(primitiveType);

  uint32_t indexCount = CheckInstancing();
  if (indexCount > 0) {
    auto &view = g_vertexBufferViews[g_pipelineState.vertexDeclaration
                                         ->indexVertexStream];
    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer, view.buffer);
    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, view.size);
    SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format,
                  RenderFormat::R32_UINT);
    UnsetInstancingStream();
  }

  uint32_t convertedIndexCount = 0;
  if (indexCount == 0) {
    if (primitiveType == D3DPT_QUADLIST)
      convertedIndexCount = g_quadIndexData.prepare(primitiveCount);
    else if (primitiveType == D3DPT_TRIANGLEFAN)
      convertedIndexCount = g_triangleFanIndexData.prepare(primitiveCount);
  }

  FlushRenderState(device);
  if (!g_pipelineBound) {
    LogDrawSkip("DrawPrimitive", primitiveType, primitiveCount);
    if (restoreDeclaration)
      SetVertexDeclaration(device, previousDeclaration);
    return;
  }
  DrawOutcomeTally(/*skipped=*/false);

  RenderCommandList *commandList = CommandList();
  if (indexCount > 0)
    commandList->drawIndexedInstanced(indexCount, primitiveCount / indexCount,
                                      0, 0, 0);
  else if (convertedIndexCount > 0)
    commandList->drawIndexedInstanced(convertedIndexCount, 1, 0,
                                      int32_t(startVertex), 0);
  else
    commandList->drawInstanced(primitiveCount, 1, startVertex, 0);
  if (restoreDeclaration)
    SetVertexDeclaration(device, previousDeclaration);
}

void DrawIndexedPrimitive(GuestDevice *device, uint32_t primitiveType,
                          int32_t baseVertexIndex, uint32_t startIndex,
                          uint32_t primitiveCount) {
  SyncVertexDeclarationFromDevice(device);
  RestoreVertexDeclarationForShader(device);
  GuestVertexDeclaration *guestDeclaration = g_pipelineState.vertexDeclaration;
  GuestVertexDeclaration *previousDeclaration = nullptr;
  bool restoreDeclaration = SelectShaderVertexDeclaration(
      device, g_inputSlots[0].stride, &previousDeclaration);
  if (!restoreDeclaration) {
    restoreDeclaration =
        SelectVertexDeclaration(device,
                                SelectParticleVertexDeclarationByStride(
                                    primitiveType, g_inputSlots[0].stride),
                                &previousDeclaration);
  }
  if (!restoreDeclaration) {
    restoreDeclaration = SelectGpuSkin40Declaration(
        device, primitiveType, g_inputSlots[0].stride, &previousDeclaration);
  }
  if (!restoreDeclaration) {
    restoreDeclaration = SelectIndexedMeshDeclarationForStaleCanvasDecl(
        device, primitiveType, primitiveCount, g_inputSlots[0].stride,
        &previousDeclaration);
  }
  if (!restoreDeclaration) {
    restoreDeclaration = SelectQuadVertexDeclarationByStride(
        device, primitiveType, primitiveCount, g_inputSlots[0].stride,
        &previousDeclaration);
  }
  if (!restoreDeclaration) {
    restoreDeclaration = SelectTexturedQuadVertexDeclaration(
        device, primitiveType, primitiveCount, &previousDeclaration);
  }
  uint32_t indexCount = CheckInstancing();
  if (indexCount > 0)
    UnsetInstancingStream();

  SetPrimitiveType(primitiveType);
  LogSuspiciousIndexedDraw("DrawIndexedPrimitive", primitiveType,
                           baseVertexIndex, startIndex, primitiveCount,
                           guestDeclaration, g_pipelineState.vertexDeclaration);
  FlushRenderState(device);
  if (!g_pipelineBound) {
    LogDrawSkip("DrawIndexedPrimitive", primitiveType, primitiveCount);
    if (restoreDeclaration)
      SetVertexDeclaration(device, previousDeclaration);
    return;
  }
  DrawOutcomeTally(/*skipped=*/false);

  CommandList()->drawIndexedInstanced(primitiveCount, 1, startIndex,
                                      baseVertexIndex, 0);
  if (restoreDeclaration)
    SetVertexDeclaration(device, previousDeclaration);
}

void DrawPrimitiveUP(GuestDevice *device, uint32_t primitiveType,
                     uint32_t primitiveCount, void *vertexStreamZeroData,
                     uint32_t vertexStreamZeroStride) {
  SyncVertexDeclarationFromDevice(device);
  RestoreVertexDeclarationForShader(device);
  GuestVertexDeclaration *previousDeclaration = nullptr;
  bool restoreDeclaration = SelectShaderVertexDeclaration(
      device, vertexStreamZeroStride, &previousDeclaration);
  if (!restoreDeclaration) {
    restoreDeclaration =
        SelectVertexDeclaration(device,
                                SelectParticleVertexDeclarationByStride(
                                    primitiveType, vertexStreamZeroStride),
                                &previousDeclaration);
  }
  if (!restoreDeclaration) {
    restoreDeclaration = SelectQuadVertexDeclarationByStride(
        device, primitiveType, primitiveCount, vertexStreamZeroStride,
        &previousDeclaration, vertexStreamZeroData);
  }
  if (!restoreDeclaration) {
    restoreDeclaration = SelectTexturedQuadVertexDeclaration(
        device, primitiveType, primitiveCount, &previousDeclaration);
  }
  CheckInstancing();
  if (g_pipelineState.instancing)
    UnsetInstancingStream();

  SetPrimitiveType(primitiveType);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[0],
                uint8_t(vertexStreamZeroStride));

  uint32_t vertexDataSize = primitiveCount * vertexStreamZeroStride;
  UploadResult allocation =
      UploadGuestVertexData(vertexStreamZeroData, vertexDataSize, 0x4);

  g_vertexBufferViews[0].size = vertexDataSize;
  g_vertexBufferViews[0].buffer = allocation.buffer->at(allocation.offset);
  g_inputSlots[0].stride = vertexStreamZeroStride;
  g_dirtyStates.vertexStreamFirst = 0;

  uint32_t indexCount = 0;
  if (primitiveType == D3DPT_QUADLIST)
    indexCount = g_quadIndexData.prepare(primitiveCount);
  else if (primitiveType == D3DPT_TRIANGLEFAN)
    indexCount = g_triangleFanIndexData.prepare(primitiveCount);

  FlushRenderState(device);
  if (!g_pipelineBound) {
    LogDrawSkip("DrawPrimitiveUP", primitiveType, primitiveCount);
    if (restoreDeclaration)
      SetVertexDeclaration(device, previousDeclaration);
    return;
  }
  DrawOutcomeTally(/*skipped=*/false);

  if (indexCount != 0)
    CommandList()->drawIndexedInstanced(indexCount, 1, 0, 0, 0);
  else
    CommandList()->drawInstanced(primitiveCount, 1, 0, 0);
  if (restoreDeclaration)
    SetVertexDeclaration(device, previousDeclaration);
}

static uint32_t IndexCountForPrimitive(uint32_t primitiveType,
                                       uint32_t primitiveCount) {
  switch (primitiveType) {
  case D3DPT_POINTLIST:
    return primitiveCount;
  case D3DPT_LINELIST:
    return primitiveCount * 2;
  case D3DPT_LINESTRIP:
    return primitiveCount + 1;
  case D3DPT_TRIANGLELIST:
    return primitiveCount * 3;
  case D3DPT_TRIANGLESTRIP:
  case D3DPT_TRIANGLEFAN:
    return primitiveCount + 2;
  case D3DPT_QUADLIST:
    return primitiveCount * 4;
  default:
    return primitiveCount * 3;
  }
}

void DrawIndexedPrimitiveUP(GuestDevice *device, uint32_t primitiveType,
                            uint32_t minVertexIndex, uint32_t numVertices,
                            uint32_t numPrimitives, const void *indexData,
                            uint32_t indexStride, const void *vertexData,
                            uint32_t vertexStride) {
  SyncVertexDeclarationFromDevice(device);
  RestoreVertexDeclarationForShader(device);
  GuestVertexDeclaration *guestDeclaration = g_pipelineState.vertexDeclaration;
  GuestVertexDeclaration *previousDeclaration = nullptr;
  bool restoreDeclaration =
      SelectShaderVertexDeclaration(device, vertexStride, &previousDeclaration);
  if (!restoreDeclaration) {
    restoreDeclaration = SelectVertexDeclaration(
        device,
        SelectParticleVertexDeclarationByStride(primitiveType, vertexStride),
        &previousDeclaration);
  }
  if (!restoreDeclaration) {
    GuestVertexDeclaration *substitute = nullptr;
    if (IsSimpleElementVertexStride(vertexStride))
      substitute = SimpleElementDeclaration();
    else if (vertexStride == 20)
      substitute = TexturedQuadDeclaration();
    else if (vertexStride == 32)
      substitute = SelectStride32QuadDeclaration(vertexData, minVertexIndex,
                                                 numVertices);
    if (substitute != nullptr &&
        g_pipelineState.vertexDeclaration != substitute) {
      previousDeclaration = g_pipelineState.vertexDeclaration;
      SetVertexDeclaration(device, substitute);
      restoreDeclaration = true;
    }
  }
  LogSuspiciousIndexedUPDraw(
      primitiveType, minVertexIndex, numVertices, numPrimitives, indexStride,
      vertexStride, guestDeclaration, g_pipelineState.vertexDeclaration);
  CheckInstancing();
  if (g_pipelineState.instancing)
    UnsetInstancingStream();

  SetPrimitiveType(primitiveType);
  SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[0],
                uint8_t(vertexStride));

  const uint8_t *vertexSrc = reinterpret_cast<const uint8_t *>(vertexData) +
                             size_t(minVertexIndex) * vertexStride;
  uint32_t vertexDataSize = numVertices * vertexStride;
  UploadResult va = UploadGuestVertexData(vertexSrc, vertexDataSize, 0x4);
  g_vertexBufferViews[0].size = vertexDataSize;
  g_vertexBufferViews[0].buffer = va.buffer->at(va.offset);
  g_inputSlots[0].stride = vertexStride;
  g_dirtyStates.vertexStreamFirst = 0;

  uint32_t indexCount = IndexCountForPrimitive(primitiveType, numPrimitives);
  UploadResult ia;
  if (indexStride == 4) {
    ia = g_uploadAllocator.allocateCopy<true>(
        reinterpret_cast<const uint32_t *>(indexData), indexCount * 4, 4);
    g_indexBufferView.format = RenderFormat::R32_UINT;
  } else {
    ia = g_uploadAllocator.allocateCopy<true>(
        reinterpret_cast<const uint16_t *>(indexData), indexCount * 2, 2);
    g_indexBufferView.format = RenderFormat::R16_UINT;
  }
  g_indexBufferView.buffer = ia.buffer->at(ia.offset);
  g_indexBufferView.size = indexCount * indexStride;
  g_dirtyStates.indices = true;

  SyncShadowIndexedUPColorWrite(device, primitiveType, minVertexIndex,
                                numVertices, numPrimitives, indexStride,
                                vertexData, vertexStride);
  FlushRenderState(device);
  LogShadowIndexedUPDraw(primitiveType, minVertexIndex, numVertices,
                         numPrimitives, indexStride, vertexData, vertexStride,
                         guestDeclaration, device, g_pipelineBound);
  if (!g_pipelineBound) {
    LogDrawSkip("DrawIndexedPrimitiveUP", primitiveType, numPrimitives);
    if (restoreDeclaration)
      SetVertexDeclaration(device, previousDeclaration);
    return;
  }
  DrawOutcomeTally(/*skipped=*/false);

  CommandList()->drawIndexedInstanced(indexCount, 1, 0,
                                      -int32_t(minVertexIndex), 0);
  if (restoreDeclaration)
    SetVertexDeclaration(device, previousDeclaration);
}

} // namespace fm2::render
