// render/guest_resources.h

#pragma once

#include <atomic>
#include <cstdint>
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

namespace fm4::render {

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
inline constexpr uint32_t kFm4ResourceMagic = 0x464D3452;  // 'FM4R'

// These objects live in guest memory (ghp::GuestNew) and are handed to the
// title as its own D3D resources, so FM4's D3D reads and writes their headers
// in place. Footprints and offsets taken from FM4 itself (ida40 typed structs):
//   D3DDevice_CreateVertexBuffer @0x826E7348 -> D3DVertexBuffer is 32 bytes;
//       GPUVERTEX_FETCH_CONSTANT at +0x18 (deep dive 4.1)
//   D3DDevice_CreateIndexBuffer  @0x826E7410 -> D3DIndexBuffer is 32 bytes;
//       Address at +0x18, Size at +0x1C
//   D3DDevice_CreateSurface      @0x826DF248 -> D3DSurface is 48 bytes
//   D3DDevice_CreateTexture      @0x826DF128 -> D3DBaseTexture is 52 bytes;
//       GPUTEXTURE_FETCH_CONSTANT spanning +0x1C..+0x33
// The largest guest footprint is 52 (0x34). Reserve 0x40 so no host-side member
// can share storage with it: host pointers previously sat at +0x10/+0x18/+0x20
// and the guest's +0x1C read-modify-write silently corrupted the texture pointer
// (IsLiveHostTexture then rejected the render target and Present went black).
inline constexpr uint32_t kGuestResourceHeaderBytes = 0x40;

// Offset of GPUTEXTURE_FETCH_CONSTANT inside the guest D3DBaseTexture header.
// FM2P wrote it at 0x18; FM4's typed D3DBaseTexture puts it at 0x1C (7 header
// dwords, then 24 bytes of fetch constant, total 52).
inline constexpr uint32_t kGuestTextureFetchConstantOffset = 0x1C;

// Offset of D3DSurface::Format inside the guest D3DSurface header (ida40 typed
// struct: 6 header dwords, then SurfaceInfo/DepthInfo/HiControl/Parent, then
// Format at +0x28 and Size at +0x2C). A surface carries no fetch constant.
inline constexpr uint32_t kGuestSurfaceFormatOffset = 0x28;

struct GuestResource {
  // Deliberately aliases the guest's D3DResource::Common; IsFm4Resource reads
  // it through an arbitrary guest pointer to identify our objects.
  uint32_t magic = kFm4ResourceMagic;
  // Host LE atomic. Guest D3DResource_AddRef/Release use BE lwarx/stwcx on
  // this same offset -- those paths must be fully hooked for our objects
  // (see D3DResource_AddRef @ 0x82392748 / D3DResource_Release @ 0x82386FB8)
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
inline bool IsFm4Resource(const void* p) {
  return p != nullptr && *reinterpret_cast<const uint32_t*>(p) == kFm4ResourceMagic;
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
  bool NeedsGuestUpload(bool requested, uint64_t frame) const {
    return requested && lastUploadFrame != frame;
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
  uint32_t indexVertexStream = 0;
  bool hasR11G11B10Normal = false;
  bool hasUByte4TangentBasis = false;
  bool hasFloat16Position = false;
  bool vertexStreams[16]{};

  GuestVertexDeclaration() : GuestResource(ResourceType::VertexDeclaration) {}
};

// Vertex/pixel shader: maps a guest shader to its XenosRecomp-translated host
// shader via the generated shader cache (keyed by microcode hash).
// One input element a vertex shader declares in its container header
// (usage/usageIndex). FM2's vfetch instructions carry no format/offset, so
// this is matched against FM2's real D3DVERTEXELEMENT9 declarations (which
// carry the format/offset) to recover the input layout -- FM2 never binds the
// declaration through the device field, so this is how we recover it.
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

  explicit GuestShader(ResourceType t) : GuestResource(t) {}
  ~GuestShader();
};

}  // namespace fm4::render
