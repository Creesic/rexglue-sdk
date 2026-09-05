// render/guest_resources.h

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <compare>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <plume_render_interface.h>

#include "generated/shader_cache.h"

#ifdef _WIN32
struct IDxcBlobEncoding;
#endif

namespace fm2::render {

enum class ResourceType {
  Texture,
  VolumeTexture,
  VertexBuffer,
  IndexBuffer,
  RenderTarget,
  DepthStencil,
  VertexDeclaration,
  VertexShader,
  PixelShader,
};

// Sentinel at offset 0 (where a real guest D3DResource has its Common flags),
// so hooks can tell our Guest* objects apart from genuine guest D3D resources
// created through paths we don't hook (e.g. XeInitD3DDevice internals).
inline constexpr uint32_t kFm2ResourceMagic = 0x464D3252;  // 'FM2R'

// These objects live in guest memory (ghp::GuestNew) and are handed to the
// title as its own D3D resources, so FM2's D3D reads and writes their headers
// in place. Footprints and offsets taken from FM2 itself:
//   D3DDevice_CreateVertexBuffer @0x82369ED8 -> 0x20 bytes; writes Common,
//       ReferenceCount, BaseFlush, Format.dword[0..1]
//   D3DDevice_CreateSurface     @0x8236BFC0 -> 0x30 bytes; read-modify-writes
//       +0x00, +0x1C and +0x20 (EDRAM tile address bits)
//   D3DDevice_CreateTexture     @0x8236BEA0 -> 0x34 bytes; writes the 6-dword
//       GPUTEXTURE_FETCH_CONSTANT spanning +0x18..+0x2F
// The largest guest footprint is 0x34. Reserve 0x40 so no host-side member can
// share storage with it: host pointers previously sat at +0x10/+0x18/+0x20 and
// the guest's +0x1C read-modify-write silently corrupted the texture pointer
// (IsLiveHostTexture then rejected the render target and Present went black).
inline constexpr uint32_t kGuestResourceHeaderBytes = 0x40;

inline constexpr bool TextureFetchIsCube(uint32_t dword5) {
  return ((dword5 >> 9) & 0x3u) == 3u;
}

inline constexpr int32_t TextureFetchExponentAdjust(uint32_t dword3) {
  const uint32_t bits = (dword3 >> 13) & 63u;
  return int32_t(bits ^ 32u) - 32;
}

inline constexpr uint32_t TextureFetchMipMaxLevel(uint32_t dword4) {
  return (dword4 >> 6) & 0xFu;
}

inline constexpr uint32_t TextureFetchMipAddress(uint32_t dword5) {
  return ((dword5 >> 12) & 0xFFFFFu) << 12;
}

inline constexpr uint32_t TextureFetchMipLevelCount(uint32_t dword4, uint32_t dword5,
                                                    uint32_t width, uint32_t height) {
  if (TextureFetchMipAddress(dword5) == 0 || width == 0 || height == 0)
    return 1;
  const uint32_t maxDimensionLevel = std::bit_width(std::max(width, height)) - 1u;
  return std::min(TextureFetchMipMaxLevel(dword4), maxDimensionLevel) + 1u;
}

// Raw XG storage formats. Block size and dimensions come from Plume's existing
// metadata; DXT5A is the same eight-byte 4x4 block as BC4, not a new decoder.
inline constexpr plume::RenderFormat XenosTextureStorageFormat(uint32_t gpuFormat) {
  using plume::RenderFormat;
  switch (gpuFormat) {
    case 2: return RenderFormat::R8_UNORM;
    case 6: return RenderFormat::B8G8R8A8_UNORM;
    case 7:
    case 54: return RenderFormat::R10G10B10A2_UNORM;
    case 10: return RenderFormat::R8G8_UNORM;
    case 18: return RenderFormat::BC1_UNORM;
    case 19: return RenderFormat::BC2_UNORM;
    case 20: return RenderFormat::BC3_UNORM;
    case 26: return RenderFormat::R16G16B16A16_UNORM;
    case 29:
    case 32: return RenderFormat::R16G16B16A16_FLOAT;
    case 59: return RenderFormat::BC4_UNORM;  // k_DXT5A
    default: return RenderFormat::UNKNOWN;
  }
}

inline plume::RenderComponentMapping TranslatedTextureComponentMapping(plume::RenderFormat format) {
  using plume::RenderFormat;
  using plume::RenderSwizzle;
  if (format == RenderFormat::R8_UNORM)
    return {RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::ONE};
  // Xenos DXT5A supplies its scalar to every component (Xenia's RRRR mapping).
  if (format == RenderFormat::BC4_UNORM)
    return {RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::R};
  return {};
}

inline bool CanReceiveColorResolveBlit(plume::RenderFormat format) {
  return format != plume::RenderFormat::UNKNOWN && !plume::RenderFormatIsDepth(format) &&
         plume::RenderFormatBlockWidth(format) == 1;
}

// Decoded storage identity for raw XG textures. One guest base may alternate
// between several layouts; recorded draws must keep a stable host object for
// each layout so later resolves update the object they actually sample.
struct XenosTextureInfo {
  plume::RenderFormat format = plume::RenderFormat::UNKNOWN;
  uint32_t gpuFormat = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t baseAddress = 0;
  uint32_t mipAddress = 0;
  uint32_t mipMaxLevel = 0;
  uint32_t mipLevels = 1;
  uint32_t pitchTexels = 0;
  uint32_t blockDim = 1;
  uint32_t bytesPerBlock = 4;
  uint32_t endian = 0;
  bool cube = false;
  bool tiled = false;
  bool packedMips = false;
  bool valid = false;

  void NormalizeStorageIdentity(uint32_t firstPackedMip) {
    // Packing cannot change any stored level if the tail starts beyond it.
    // Keep the bit for small base levels and chains that actually reach the tail.
    if (valid && mipMaxLevel < firstPackedMip)
      packedMips = false;
  }

  auto operator<=>(const XenosTextureInfo&) const = default;
};

struct GuestResource {
  // Deliberately aliases the guest's D3DResource::Common; IsFm2Resource reads
  // it through an arbitrary guest pointer to identify our objects.
  uint32_t magic = kFm2ResourceMagic;
  // Host LE atomic. Guest D3DResource_AddRef/Release use BE lwarx/stwcx on
  // this same offset -- those paths must be fully hooked for FM2 objects
  // (see D3DResource_AddRef @ 0x82369D90 / D3DResource_Release @ 0x82369E08)
  // or refcount corruption / double-free.
  std::atomic<uint32_t> refCount{1};
  // Guest-owned bytes: fences, identifier, BaseFlush and the fetch constant.
  // Never place a host member here.
  uint8_t guestHeader[kGuestResourceHeaderBytes - 8]{};
  ResourceType type;

  explicit GuestResource(ResourceType t) : type(t) {}
};

static_assert(offsetof(GuestResource, refCount) == 4,
              "refCount must alias the guest's D3DResource::ReferenceCount");
static_assert(offsetof(GuestResource, type) == kGuestResourceHeaderBytes,
              "host members must start past the guest-written resource header");

// True only for pointers to our own guest-allocated Guest* objects.
inline bool IsFm2Resource(const void* p) {
  return p != nullptr && *reinterpret_cast<const uint32_t*>(p) == kFm2ResourceMagic;
}

struct GuestBaseTexture;
struct GuestSurface;

struct PendingResolve {
  GuestBaseTexture* destination = nullptr;
  plume::RenderRect sourceRect{};
  uint32_t destX = 0;
  uint32_t destY = 0;
  bool hasSourceRect = false;
};

struct GuestBaseTexture : GuestResource {
  std::unique_ptr<plume::RenderTexture> textureHolder;
  plume::RenderTexture* texture = nullptr;
  std::unique_ptr<plume::RenderTextureView> textureView;
  uint32_t width = 0;
  uint32_t height = 0;
  plume::RenderFormat format = plume::RenderFormat::UNKNOWN;
  uint32_t descriptorIndex = 0;
  plume::RenderTextureLayout layout = plume::RenderTextureLayout::UNKNOWN;
  bool requiresHostInitialization = false;
  bool hostInitialized = true;
  // True only when the host texture was created with RENDER_TARGET capability.
  // StretchRectShaderBlit must fail closed on non-RT destinations: drawing into
  // a NONE-flag resource makes D3D12 remove the device (invalid RTV).
  bool hostRenderTargetCapable = false;
  // Xbox 360 D3DFORMAT bit 8 (0x100): the guest treats this texture's memory
  // as TILED. Lock/Unlock writers produce pre-tiled GPU-ready bytes (asset
  // blobs / XGTileTextureLevel), so the unlock upload must untile them.
  bool guestTiled = false;
  // Size of the lock staging allocation backing mappedMemory (bounds guard for
  // the tiled unlock read pattern).
  uint32_t mappedSizeBytes = 0;
  // Mip level the current Lock targets (D3DTexture_LockRect's Level param).
  // Unlock uploads into this subresource; surfaces are always level 0.
  uint32_t lockedLevel = 0;
  // Total mip levels of the host texture (1 for surfaces).
  uint32_t levels = 1;
  // Frame index of the last guest-memory upload; lets us upload a sampled
  // texture at most once per frame instead of once per draw (huge perf win).
  uint64_t lastUploadFrame = ~0ull;
  // Resolves update host storage, not guest RAM. Keep that stale RAM out of
  // implicit bind-time refreshes even after a deferred resolve has completed.
  // Explicit UnlockRect uploads carry their own data and bypass this gate.
  std::atomic<bool> guestMemoryStale{false};
  bool NeedsGuestUpload(bool requested, uint64_t frame) const {
    return requested && !guestMemoryStale.load(std::memory_order_relaxed) &&
           lastUploadFrame != frame;
  }
  GuestBaseTexture* sourceTexture = nullptr;
  uint32_t pendingResolveCount = 0;
  std::vector<PendingResolve> pendingResolves;
  // Host pointer to guest-visible lock staging, shared by LockRect for both
  // textures and surfaces (D3DSurface_LockRect locks render targets too).
  void* mappedMemory = nullptr;

  explicit GuestBaseTexture(ResourceType t) : GuestResource(t) {}
};

// D3DFMT_* texture/volume texture.
struct GuestTexture : GuestBaseTexture {
  uint32_t depth = 0;
  plume::RenderTextureViewDimension viewDimension = plume::RenderTextureViewDimension::TEXTURE_2D;
  std::unique_ptr<plume::RenderFramebuffer> framebuffer;
  // Unleashed StretchRect link: surface this texture will receive a resolve/
  // copy from. Cleared when the pending copy executes.
  GuestSurface* sourceSurface = nullptr;

  GuestTexture() : GuestBaseTexture(ResourceType::Texture) {}
};

inline uint32_t GetTextureLevelCount(const void* texture) {
  if (texture == nullptr)
    return 0;
  if (IsFm2Resource(texture)) {
    const auto* resource = static_cast<const GuestResource*>(texture);
    if (resource->type != ResourceType::Texture && resource->type != ResourceType::VolumeTexture)
      return 0;
    return static_cast<const GuestBaseTexture*>(resource)->levels;
  }
  // D3DBaseTexture::Format starts at +0x1C. The original GetLevelCount
  // at 8236BC38 reads mip_max_level from dword 4 (+0x2C), big-endian.
  uint32_t dword4;
  std::memcpy(&dword4, static_cast<const uint8_t*>(texture) + 0x2C, sizeof(dword4));
  return TextureFetchMipMaxLevel(std::byteswap(dword4)) + 1;
}

// Vertex/index buffer.
struct GuestBuffer : GuestResource {
  std::unique_ptr<plume::RenderBuffer> buffer;
  void* mappedMemory = nullptr;  // host pointer to guest-visible lock staging
  uint32_t dataSize = 0;
  plume::RenderFormat format = plume::RenderFormat::UNKNOWN;  // index format
  uint32_t guestFormat = 0;
  bool lockedReadOnly = false;

  explicit GuestBuffer(ResourceType t) : GuestResource(t) {}
};

// Render target / depth-stencil surface.
struct GuestSurface : GuestBaseTexture {
  uint32_t guestFormat = 0;
  plume::RenderSampleCounts sampleCount = plume::RenderSampleCount::COUNT_1;
  // Original guest height before ResizeTileSurface grew this surface to full
  // frame height (0 = never grown). TranslateGuestSurface treats a lookup
  // with the original dimensions as a cache HIT so the per-frame guest
  // header re-translation does not erase + recreate (and leak) the pair.
  uint32_t tileGrownFromHeight = 0;
  // Framebuffers keyed by their (paired) color attachment; the backbuffer's
  // texture changes per frame, so the depth surface owns the cache.
  std::unordered_map<const plume::RenderTexture*, std::unique_ptr<plume::RenderFramebuffer>>
      framebuffers;
  // Textures waiting for a StretchRect / Resolve copy from this surface
  // (Unleashed destinationTextures). Drained by FlushPendingStretchRectCommands.
  std::unordered_set<GuestTexture*> destinationTextures;

  explicit GuestSurface(ResourceType t) : GuestBaseTexture(t) {}

  // XGSetSurfaceHeader initializes a 0x30-byte XDK header, including Common
  // and ReferenceCount. Those first eight bytes are native ownership here.
  void UpdateGuestHeader(const void* header) {
    std::memcpy(guestHeader, static_cast<const uint8_t*>(header) + 8, 0x30 - 8);
  }

  bool MatchesHostLayout(uint32_t newWidth, uint32_t newHeight,
                         plume::RenderFormat newFormat,
                         plume::RenderSampleCounts newSamples) const {
    return width == newWidth && height == newHeight && format == newFormat &&
           sampleCount == newSamples;
  }
};

struct GuestVertexElement {
  uint16_t stream;
  uint16_t offset;
  uint32_t type;
  uint8_t method;
  uint8_t usage;
  uint8_t usageIndex;
  uint8_t padding;
};

struct GuestVertexDeclaration : GuestResource {
  uint64_t hash = 0;
  std::unique_ptr<plume::RenderInputElement[]> inputElements;
  std::unique_ptr<GuestVertexElement[]> vertexElements;
  uint32_t inputElementCount = 0;
  uint32_t vertexElementCount = 0;
  // Derived during input-layout translation (render_state.cpp).
  uint32_t swappedTexcoords = 0;
  uint32_t swappedBlendWeights = 0;
  uint32_t r11g11b10Texcoords = 0;
  uint32_t indexVertexStream = 0;
  bool hasR11G11B10Normal = false;
  bool hasUByte4TangentBasis = false;
  bool hasFloat16Position = false;
  // Prepatched shader fetches consume raw words and perform their own decoding.
  bool bakedVertexFetch = false;
  std::array<uint32_t, 16> bakedStrides{};
  bool vertexStreams[16]{};

  GuestVertexDeclaration() : GuestResource(ResourceType::VertexDeclaration) {}
};

// Vertex/pixel shader: maps a guest shader to its XenosRecomp-translated host
// shader via the generated shader cache (keyed by microcode hash).
// One input element a vertex shader declares in its container header
// (usage/usageIndex). Unpatched shaders use the ordered D3D declaration;
// prepatched containers (flag 0x40) carry their own complete fetch layout.
struct ShaderHeaderElement {
  uint8_t usage = 0;
  uint8_t usageIndex = 0;
};

struct GuestShader : GuestResource {
  std::mutex mutex;
  std::unique_ptr<plume::RenderShader> shader;
  std::unordered_map<uint32_t, std::unique_ptr<plume::RenderShader>> specializedShaders;
#ifdef _WIN32
  IDxcBlobEncoding* dxilLibraryBlob = nullptr;
#endif
  ShaderCacheEntry* shaderCacheEntry = nullptr;
  // Vertex shaders only: the input declaration parsed from the shader header.
  std::vector<ShaderHeaderElement> headerElements;
  bool bakedVertexFetch = false;
  std::unique_ptr<GuestVertexDeclaration> bakedDeclaration;

  explicit GuestShader(ResourceType t) : GuestResource(t) {}
  ~GuestShader();
};

}  // namespace fm2::render
