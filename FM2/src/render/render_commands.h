// render/render_commands.h
//
// Unleashed-style POD render commands. Guest threads enqueue these; the
// dedicated render thread dispatches Proc* handlers.

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace fm2::render {

struct ConstantSnapshotRange {
  uint32_t index{};  // uint32_t index into the stage's float-constant file.
  uint32_t size{};   // byte count.
};

// Xenos tracks float constants in 64 MSB-first dirty groups, four float4
// registers (16 dwords / 64 bytes) per group. Pixel shaders expose only the
// first 56 groups (224 float4 registers).
constexpr ConstantSnapshotRange GetConstantSnapshotRange(uint64_t dirtyFlags,
                                                         uint32_t maxGroupCount) {
  if (dirtyFlags == 0 || maxGroupCount == 0)
    return {};
  const uint32_t startGroup = std::countl_zero(dirtyFlags);
  const uint32_t endGroup = std::min(maxGroupCount, uint32_t(64 - std::countr_zero(dirtyFlags)));
  if (startGroup >= endGroup)
    return {};
  return ConstantSnapshotRange{startGroup * 16u, (endGroup - startGroup) * 64u};
}

// Remove and return the first contiguous run of dirty constant groups. This is
// used while recording reusable command buffers so clean groups between two
// dirty runs are not accidentally baked into the template.
constexpr ConstantSnapshotRange PopConstantSnapshotRange(uint64_t& dirtyFlags,
                                                          uint32_t maxGroupCount) {
  maxGroupCount = std::min(maxGroupCount, 64u);
  while (dirtyFlags != 0 && std::countl_zero(dirtyFlags) >= maxGroupCount)
    dirtyFlags &= dirtyFlags - 1;
  if (dirtyFlags == 0)
    return {};

  const uint32_t startGroup = std::countl_zero(dirtyFlags);
  uint32_t endGroup = startGroup;
  while (endGroup < maxGroupCount &&
         (dirtyFlags & (uint64_t{1} << (63u - endGroup))) != 0) {
    dirtyFlags &= ~(uint64_t{1} << (63u - endGroup));
    ++endGroup;
  }
  return ConstantSnapshotRange{startGroup * 16u, (endGroup - startGroup) * 64u};
}

// Per-draw constants emitted through APIs that return a command-buffer payload
// for the caller to fill (notably D3DDevice_GpuBeginShaderConstantF4). Those
// values never enter GuestDevice's +0x700/+0x1700 files, so stage them until the
// next native draw and overlay only the registers that the guest emitted.
struct PendingShaderConstantFile {
  static constexpr uint32_t kRegisterCount = 256;
  static constexpr uint32_t kDwordsPerRegister = 4;

  std::array<uint32_t, kRegisterCount * kDwordsPerRegister> values{};
  std::array<uint64_t, kRegisterCount / 64> coverage{};

  bool empty() const {
    return std::all_of(coverage.begin(), coverage.end(), [](uint64_t bits) { return bits == 0; });
  }

  void Stage(uint32_t startRegister, const uint32_t* source, uint32_t registerCount) {
    if (source == nullptr || startRegister >= kRegisterCount || registerCount == 0)
      return;
    const uint32_t count = std::min(registerCount, kRegisterCount - startRegister);
    std::memcpy(values.data() + startRegister * kDwordsPerRegister, source,
                size_t(count) * kDwordsPerRegister * sizeof(uint32_t));
    for (uint32_t reg = startRegister; reg < startRegister + count; ++reg)
      coverage[reg / 64] |= uint64_t{1} << (reg % 64);
  }

  void StageDirtyGroups(uint64_t dirtyGroups, const uint32_t* source,
                        uint32_t maxGroupCount) {
    if (source == nullptr)
      return;
    maxGroupCount = std::min(maxGroupCount, kRegisterCount / 4);
    while (dirtyGroups != 0) {
      const uint32_t group = std::countl_zero(dirtyGroups);
      if (group >= maxGroupCount)
        break;
      Stage(group * 4, source + group * 16, 4);
      dirtyGroups &= ~(uint64_t{1} << (63 - group));
    }
  }

  uint64_t DirtyGroups(uint32_t maxGroupCount) const {
    uint64_t groups = 0;
    maxGroupCount = std::min(maxGroupCount, kRegisterCount / 4);
    for (uint32_t group = 0; group < maxGroupCount; ++group) {
      const uint32_t firstRegister = group * 4;
      const uint64_t registerMask = uint64_t{0xF} << (firstRegister % 64);
      if ((coverage[firstRegister / 64] & registerMask) != 0)
        groups |= uint64_t{1} << (63u - group);
    }
    return groups;
  }

  void Overlay(uint32_t* destination, uint32_t destinationRegisterCount) const {
    if (destination != nullptr) {
      destinationRegisterCount = std::min(destinationRegisterCount, kRegisterCount);
      for (uint32_t word = 0; word < coverage.size(); ++word) {
        uint64_t bits = coverage[word];
        while (bits != 0) {
          const uint32_t reg = word * 64u + uint32_t(std::countr_zero(bits));
          bits &= bits - 1;
          if (reg >= destinationRegisterCount)
            continue;
          std::memcpy(destination + reg * kDwordsPerRegister,
                      values.data() + reg * kDwordsPerRegister,
                      kDwordsPerRegister * sizeof(uint32_t));
        }
      }
    }
  }

  void OverlayAndClear(uint32_t* destination, uint32_t destinationRegisterCount) {
    Overlay(destination, destinationRegisterCount);
    coverage.fill(0);
  }
};

// State that FM2 patches into a reusable D3D command buffer immediately
// before execution. Keep this producer-owned copy independent of the recorded
// template commands: the guest context can advance before the render thread
// reaches the replay job.
struct DeferredExecutionSnapshot {
  static constexpr uint32_t kVsConstantOffset = 0x700;
  // The VS float file occupies c0-c255; the PS float file begins immediately
  // after it at context +0x1700.
  static constexpr uint32_t kVsConstantBytes = 256 * 16;
  static constexpr uint32_t kVsBooleanOffset = 0x2700;
  static constexpr uint32_t kPsBooleanOffset = 0x2710;
  static constexpr uint32_t kViewportOffset = 0x3168;
  static constexpr uint32_t kContextBytes = 0x3180;

  std::array<uint8_t, kVsConstantBytes> vertexConstants{};
  std::array<uint32_t, 8> booleans{};
  uint32_t viewportReverseZ = 0;
  PendingShaderConstantFile vertexExecutionConstants{};
  PendingShaderConstantFile pixelExecutionConstants{};
};

static_assert(std::is_trivially_copyable_v<DeferredExecutionSnapshot>);
static_assert(DeferredExecutionSnapshot::kVsConstantOffset +
                  DeferredExecutionSnapshot::kVsConstantBytes ==
              0x1700);
static_assert(sizeof(DeferredExecutionSnapshot) == 12392);

inline bool CaptureDeferredExecutionSnapshot(DeferredExecutionSnapshot& snapshot,
                                             const uint8_t* context) {
  if (context == nullptr)
    return false;

  snapshot.vertexExecutionConstants = {};
  snapshot.pixelExecutionConstants = {};
  std::memcpy(snapshot.vertexConstants.data(),
              context + DeferredExecutionSnapshot::kVsConstantOffset,
              snapshot.vertexConstants.size());
  for (uint32_t i = 0; i < 4; ++i) {
    uint32_t word;
    std::memcpy(&word, context + DeferredExecutionSnapshot::kVsBooleanOffset + i * sizeof(word),
                sizeof(word));
    snapshot.booleans[i] = std::byteswap(word);
    std::memcpy(&word, context + DeferredExecutionSnapshot::kPsBooleanOffset + i * sizeof(word),
                sizeof(word));
    snapshot.booleans[4 + i] = std::byteswap(word);
  }
  uint32_t minZBits;
  uint32_t maxZBits;
  std::memcpy(&minZBits, context + DeferredExecutionSnapshot::kViewportOffset + 16,
              sizeof(minZBits));
  std::memcpy(&maxZBits, context + DeferredExecutionSnapshot::kViewportOffset + 20,
              sizeof(maxZBits));
  const float minZ = std::bit_cast<float>(std::byteswap(minZBits));
  const float maxZ = std::bit_cast<float>(std::byteswap(maxZBits));
  snapshot.viewportReverseZ = std::isfinite(minZ) && std::isfinite(maxZ) && minZ > maxZ;
  return true;
}

inline void InitializeDeferredVertexConstants(const DeferredExecutionSnapshot& snapshot,
                                              uint32_t* destination,
                                              uint32_t destinationRegisterCount) {
  if (destination == nullptr || destinationRegisterCount == 0)
    return;
  const uint32_t registerCount = std::min(destinationRegisterCount, 256u);
  std::memcpy(destination, snapshot.vertexConstants.data(), size_t(registerCount) * 16);
  snapshot.vertexExecutionConstants.Overlay(destination, registerCount);
}

inline bool NormalizeUnitFullscreenUpQuad(uint8_t* data, uint32_t vertexCount, uint32_t stride,
                                          uint32_t bytes) {
  if (data == nullptr || vertexCount != 4 || stride != 20 || bytes != vertexCount * stride)
    return false;

  auto read = [](const uint8_t* source) {
    uint32_t word;
    std::memcpy(&word, source, sizeof(word));
    return std::bit_cast<float>(std::byteswap(word));
  };
  float minX = read(data), maxX = minX;
  float minY = read(data + 4), maxY = minY;
  float minU = read(data + 12), maxU = minU;
  float minV = read(data + 16), maxV = minV;
  if (!std::isfinite(minX) || !std::isfinite(minY) || !std::isfinite(minU) ||
      !std::isfinite(minV)) {
    return false;
  }
  for (uint32_t vertex = 1; vertex < vertexCount; ++vertex) {
    const uint8_t* source = data + vertex * stride;
    const float x = read(source);
    const float y = read(source + 4);
    const float u = read(source + 12);
    const float v = read(source + 16);
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(u) || !std::isfinite(v))
      return false;
    minX = std::min(minX, x);
    maxX = std::max(maxX, x);
    minY = std::min(minY, y);
    maxY = std::max(maxY, y);
    minU = std::min(minU, u);
    maxU = std::max(maxU, u);
    minV = std::min(minV, v);
    maxV = std::max(maxV, v);
  }
  constexpr float kTolerance = 1.0e-6f;
  auto isNear = [=](float value, float expected) {
    return std::fabs(value - expected) <= kTolerance;
  };
  if (!isNear(minX, 0.0f) || !isNear(maxX, 1.0f) || !isNear(minY, 0.0f) || !isNear(maxY, 1.0f) ||
      !isNear(minU, 0.0f) || !isNear(maxU, 1.0f) || !isNear(minV, 0.0f) || !isNear(maxV, 1.0f)) {
    return false;
  }

  auto write = [](uint8_t* destination, float value) {
    const uint32_t word = std::byteswap(std::bit_cast<uint32_t>(value));
    std::memcpy(destination, &word, sizeof(word));
  };
  for (uint32_t vertex = 0; vertex < vertexCount; ++vertex) {
    uint8_t* destination = data + vertex * stride;
    write(destination, read(destination) * 2.0f - 1.0f);
    write(destination + 4, 1.0f - read(destination + 4) * 2.0f);
  }
  return true;
}

struct GuestBaseTexture;
struct GuestBuffer;
struct GuestResource;
struct GuestShader;
struct GuestSurface;
struct GuestTexture;
struct GuestVertexDeclaration;
struct GuestDevice;

struct DrawStreamSnapshot {
  GuestBuffer* buffer;
  uint32_t offset;
  uint32_t stride;
  uint8_t* rawData;
  uint32_t rawSize;
};

struct DrawGeometrySnapshot {
  DrawStreamSnapshot streams[16];
  GuestBuffer* indexBuffer;
  uint8_t* rawIndexData;
  uint32_t rawIndexSize;
  uint32_t rawIndexStride;
};

enum class RenderCommandType : uint32_t {
  DestructResource,
  SetViewport,
  SetScissorRect,
  SetRenderTarget,
  SetImplicitRenderTarget,
  SetDepthStencilSurface,
  SetRenderState,
  SetViewportEnable,
  SetClipPlaneState,
  SetDepthState,
  SetStencilState,
  SetTexture,
  SetTextureBase,
  SetSamplerState,
  SetBooleans,
  SetLoopConstants,
  SetVertexShaderConstants,
  SetPixelShaderConstants,
  SetVertexShader,
  SetPixelShader,
  SetVertexDeclaration,
  SetStreamSource,
  SetIndices,
  SetDrawGeometrySnapshot,
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
  CopyTextureSubresourcesFromUpload,
  CreateTranslatedTextureHost,
  ApplyVertexShaderConstantFixup,
  ApplyPixelShaderConstantFixup,
};

struct TextureUploadRegion {
  uint32_t width;
  uint32_t height;
  uint32_t rowTexels;
  uint32_t mip;
  uint32_t arrayIndex;
  uint64_t srcOffset;
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
      uint32_t value;
    } setViewportEnable;

    struct {
      uint32_t enabled;
      float plane[4];
    } setClipPlaneState;

    struct {
      uint32_t zEnable;
      uint32_t zWriteEnable;
      uint32_t cmpFunc;
    } setDepthState;

    struct {
      uint32_t enable;  // bool as uint32_t for POD packing
      uint32_t twoSided;
      uint32_t frontFunc, frontFail, frontDepthFail, frontPass;
      uint32_t backFunc, backFail, backDepthFail, backPass;
      uint32_t readMask, writeMask, ref;
    } setStencilState;

    struct {
      uint32_t index;
      GuestTexture* texture;
      uint32_t guestAddress;
    } setTexture;

    struct {
      uint32_t index;
      GuestBaseTexture* texture;
      uint32_t guestAddress;
    } setTextureBase;

    struct {
      uint32_t index;
      uint32_t data0;
      uint32_t data3;
      uint32_t data5;
    } setSamplerState;

    struct {
      uint32_t words[8];
    } setBooleans;

    struct {
      uint32_t values[32];
    } setLoopConstants;

    struct {
      uint8_t* memory;
      uint32_t index;
      uint32_t size;
    } setShaderConstants;

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

    DrawGeometrySnapshot setDrawGeometrySnapshot;

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
      uint32_t postClearFlags;
      float postClearColor[4];
      float postClearZ;
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
      const uint8_t* data;
      uint32_t size;
      uint32_t level;
    } unlockTextureRect;

    struct {
      GuestBuffer* buffer;
      const uint8_t* data;
      uint32_t size;
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
      uint32_t arrayIndex;
      uint64_t srcOffset;
    } copyTextureFromUpload;

    struct {
      void* dst;
      void* src;
      uint32_t format;  // plume::RenderFormat
      const TextureUploadRegion* regions;
      uint32_t regionCount;
    } copyTextureSubresourcesFromUpload;

    struct {
      GuestTexture* texture;
      uint32_t width;
      uint32_t height;
      uint32_t format;  // plume::RenderFormat
      uint32_t baseAddress;
      uint32_t levels;
      bool cube;
      bool* createdOut;
    } createTranslatedTextureHost;
  };
};

void DispatchRenderCommand(const RenderCommand& cmd);
void DispatchRecordedRenderCommands(const RenderCommand* commands, size_t count,
                                    const DeferredExecutionSnapshot* executionSnapshot);

void ProcExecuteCommandList();
void ProcBeginCommandList();
void ProcWaitForGpu();
void ProcCreateTextureHost(GuestTexture* texture, uint32_t width, uint32_t height, uint32_t depth,
                           uint32_t levels, uint32_t usage, uint32_t format, bool volume);
void ProcCreateSurfaceHost(GuestSurface* surface, uint32_t width, uint32_t height, uint32_t format,
                           uint32_t sampleCount, bool depth);
void ProcUnlockTextureRect(GuestBaseTexture* texture, const uint8_t* data, uint32_t size,
                           uint32_t level);
void ProcUnlockBuffer16(GuestBuffer* buffer, const uint8_t* data, uint32_t size);
void ProcUnlockBuffer32(GuestBuffer* buffer, const uint8_t* data, uint32_t size);
void ProcCopyBufferFromUpload(void* dst, void* src, uint64_t size);
void ProcCopyTextureFromUpload(void* dst, void* src, uint32_t format, uint32_t width,
                                uint32_t height, uint32_t rowTexels, uint32_t mip,
                                uint32_t arrayIndex, uint64_t srcOffset);
void ProcCopyTextureSubresourcesFromUpload(void* dst, void* src, uint32_t format,
                                           const TextureUploadRegion* regions,
                                           uint32_t regionCount);
void ProcCreateTranslatedTextureHost(GuestTexture* texture, uint32_t width, uint32_t height,
                                      uint32_t format, uint32_t baseAddress, uint32_t levels,
                                      bool cube,
                                      bool* createdOut);
void ProcSetViewportEnable(uint32_t value);
void ProcSetClipPlaneState(uint32_t enabled, const float* plane);
void ProcSetDepthState(uint32_t zEnable, uint32_t zWriteEnable, uint32_t cmpFunc);
void ProcSetStencilState(uint32_t enable, uint32_t twoSided, uint32_t frontFunc, uint32_t frontFail,
                         uint32_t frontDepthFail, uint32_t frontPass, uint32_t backFunc,
                         uint32_t backFail, uint32_t backDepthFail, uint32_t backPass,
                         uint32_t readMask, uint32_t writeMask, uint32_t ref);

}  // namespace fm2::render
