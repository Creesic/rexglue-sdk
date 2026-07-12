// render/d3d_resource_hooks.cpp
//
// Phase 2 (resource hooks): buffer/texture/surface/vertex-declaration
// creation and lock/unlock, pure-replace against plume (no XDK shadow
// object, no alias table -- the native object's own guest address, from
// GuestNew placing it inside guest memory, IS the D3D9 handle from here on).
// Nothing draws yet.

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <unordered_set>
#include <vector>

#include <plume_render_interface.h>
#include <rex/hash.h>  // XXH3_64bits
#include <rex/logging.h>

#include "render/guest_device.h"
#include "render/guest_heap.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"

using namespace plume;
using namespace fm2::ghp;

namespace fm2::render {

// ---------------------------------------------------------------------------
// Format helpers
// ---------------------------------------------------------------------------

// Xenos D3DFORMAT (the GuestFormat enum values) -> Plume RenderFormat.
RenderFormat ConvertFormat(uint32_t d3dFormat) {
  switch (d3dFormat) {
    case 0x1A22AB60:  // D3DFMT_A16B16G16R16F
    case 0x1A2201BF:  // D3DFMT_A16B16G16R16F_2
      return RenderFormat::R16G16B16A16_FLOAT;
    case 0x1A200186:  // D3DFMT_A8B8G8R8
      return RenderFormat::R8G8B8A8_UNORM;
    case 0x18280186:  // D3DFMT_A8R8G8B8
    case 0x28280086:  // D3DFMT_X8R8G8B8
      return RenderFormat::B8G8R8A8_UNORM;
    case 0x1A220197:  // D3DFMT_D24FS8
    case 0x2D200196:  // D3DFMT_D24S8
      return RenderFormat::D32_FLOAT_S8_UINT;
    case 0x2D22AB9F:  // D3DFMT_G16R16F
    case 0x2D20AB8D:  // D3DFMT_G16R16F_2
      return RenderFormat::R16G16_FLOAT;
    case 1:  // D3DFMT_INDEX16
      return RenderFormat::R16_UINT;
    case 6:  // D3DFMT_INDEX32
      return RenderFormat::R32_UINT;
    case 0x28000102:  // D3DFMT_L8
    case 0x28000002:  // D3DFMT_L8_2
      return RenderFormat::R8_UNORM;
  }

  switch (d3dFormat & 0x3F) {
    case 22:  // k_24_8 (D24S8 variants)
    case 23:  // k_24_8_FLOAT (D24FS8 variants)
      return RenderFormat::D32_FLOAT_S8_UINT;
  }

  static std::unordered_set<uint32_t> s_warnedFormats;
  if (s_warnedFormats.insert(d3dFormat).second) {
    REXGPU_WARN(
        "ConvertFormat: unknown D3DFORMAT 0x{:08X} (gpu format {}) -- "
        "defaulting to R8G8B8A8",
        d3dFormat, d3dFormat & 0x3F);
  }
  return RenderFormat::R8G8B8A8_UNORM;
}

namespace {

uint32_t FormatBytes(RenderFormat format) {
  switch (format) {
    case RenderFormat::R16G16B16A16_FLOAT:
    case RenderFormat::D32_FLOAT_S8_UINT:  // R32G8X24-typeless: 8 bytes/texel
      return 8;
    case RenderFormat::R8G8B8A8_UNORM:
    case RenderFormat::B8G8R8A8_UNORM:
    case RenderFormat::R16G16_FLOAT:
    case RenderFormat::D32_FLOAT:
    case RenderFormat::R32_UINT:
      return 4;
    case RenderFormat::R16_UINT:
      return 2;
    case RenderFormat::R8_UNORM:
      return 1;
    default:
      return 4;
  }
}

RenderHeapType BufferHeapType() {
  return Device()->getCapabilities().gpuUploadHeap ? RenderHeapType::GPU_UPLOAD
                                                    : RenderHeapType::DEFAULT;
}

// D3D12 requires copy row pitch aligned to 256 bytes.
uint32_t ComputeTexturePitch(const GuestBaseTexture* texture) {
  uint32_t bpp = FormatBytes(texture->format);
  uint32_t pitch = texture->width * bpp;
  return (pitch + 255u) & ~255u;
}

}  // namespace

// ---------------------------------------------------------------------------
// Buffers
// ---------------------------------------------------------------------------

GuestBuffer* CreateVertexBuffer(uint32_t length) {
  auto* buffer = GuestNew<GuestBuffer>(ResourceType::VertexBuffer);
  buffer->buffer = Device()->createBuffer(RenderBufferDesc::VertexBuffer(
      length, BufferHeapType(), RenderBufferFlag::INDEX));
  buffer->dataSize = length;
  return buffer;
}

GuestBuffer* CreateIndexBuffer(uint32_t length, uint32_t format) {
  auto* buffer = GuestNew<GuestBuffer>(ResourceType::IndexBuffer);
  buffer->buffer =
      Device()->createBuffer(RenderBufferDesc::IndexBuffer(length, BufferHeapType()));
  buffer->dataSize = length;
  buffer->guestFormat = format;
  // Index buffers MUST be a 16/32-bit index format -- never a color format.
  // ConvertFormat falls through to R8G8B8A8_UNORM for unrecognized index
  // format values, which D3D12 rejects as an index format.
  buffer->format =
      (format == 6 /* D3DFMT_INDEX32 */) ? RenderFormat::R32_UINT : RenderFormat::R16_UINT;
  return buffer;
}

// Lock returns a guest-visible staging pointer that the guest writes (BE) into.
static uint32_t LockBuffer(GuestBuffer* buffer, uint32_t flags) {
  buffer->lockedReadOnly = (flags & 0x10) != 0;
  if (buffer->mappedMemory == nullptr) {
    uint32_t addr = GuestAllocRaw(buffer->dataSize, 0x10);
    buffer->mappedMemory = ToHost<void>(addr);
  }
  return ToGuest(buffer->mappedMemory);
}

// Byte-swap the guest staging contents into the host buffer.
template <typename T>
static void UploadBufferSwapped(GuestBuffer* buffer) {
  if (buffer->lockedReadOnly || buffer->mappedMemory == nullptr) return;
  if (!buffer->buffer) return;

  const T* src = reinterpret_cast<const T*>(buffer->mappedMemory);
  const size_t count = buffer->dataSize / sizeof(T);

  auto swapInto = [&](T* dest) {
    for (size_t i = 0; i < count; ++i) dest[i] = std::byteswap(src[i]);
  };

  if (Device()->getCapabilities().gpuUploadHeap) {
    T* dest = reinterpret_cast<T*>(buffer->buffer->map());
    if (dest == nullptr) return;
    swapInto(dest);
    buffer->buffer->unmap();
  } else {
    auto upload = Device()->createBuffer(RenderBufferDesc::UploadBuffer(buffer->dataSize));
    T* dest = reinterpret_cast<T*>(upload->map());
    if (dest == nullptr) return;
    swapInto(dest);
    upload->unmap();
    RenderBuffer* dst = buffer->buffer.get();
    RenderBuffer* srcBuf = upload.get();
    const uint64_t size = buffer->dataSize;
    ExecuteUpload([&](RenderCommandList* cl) { cl->copyBufferRegion(dst->at(0), srcBuf->at(0), size); });
  }
}

uint32_t LockVertexBuffer(GuestBuffer* buffer, uint32_t flags) { return LockBuffer(buffer, flags); }
void UnlockVertexBuffer(GuestBuffer* buffer) { UploadBufferSwapped<uint32_t>(buffer); }

uint32_t LockIndexBuffer(GuestBuffer* buffer, uint32_t flags) { return LockBuffer(buffer, flags); }
void UnlockIndexBuffer(GuestBuffer* buffer) {
  if (buffer->guestFormat == 6 /* D3DFMT_INDEX32 */)
    UploadBufferSwapped<uint32_t>(buffer);
  else
    UploadBufferSwapped<uint16_t>(buffer);
}

// ---------------------------------------------------------------------------
// Textures / surfaces
// ---------------------------------------------------------------------------

GuestTexture* CreateTexture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels,
                            uint32_t /*usage*/, uint32_t format, uint32_t /*pool*/, uint32_t type) {
  const bool volume = (type == 17 /* D3DRTYPE_VOLUMETEXTURE */);
  auto* texture = GuestNew<GuestTexture>();
  texture->type = volume ? ResourceType::VolumeTexture : ResourceType::Texture;

  // D3D9 convention: Levels == 0 means "generate the full mip chain down to
  // 1x1". Passing 0 straight through to D3D12 is invalid and fails resource
  // creation (observed: CreateResource error 0x887A0005 / device removed).
  if (levels == 0) {
    const uint32_t maxDim = std::max(width, height);
    levels = maxDim > 0 ? static_cast<uint32_t>(std::bit_width(maxDim)) : 1;
  }

  RenderTextureDesc desc;
  desc.dimension = volume ? RenderTextureDimension::TEXTURE_3D : RenderTextureDimension::TEXTURE_2D;
  desc.width = width;
  desc.height = height;
  desc.depth = depth;
  desc.mipLevels = levels;
  desc.arraySize = 1;
  desc.format = ConvertFormat(format);
  if (RenderFormatIsDepth(desc.format)) {
    desc.flags = RenderTextureFlag::DEPTH_TARGET;
  } else if (volume) {
    desc.flags = RenderTextureFlag::NONE;
  } else {
    desc.flags = RenderTextureFlag::RENDER_TARGET;
  }

  texture->textureHolder = Device()->createTexture(desc);
  texture->texture = texture->textureHolder.get();

  RenderTextureViewDesc viewDesc;
  viewDesc.format = desc.format;
  viewDesc.dimension = volume ? RenderTextureViewDimension::TEXTURE_3D : RenderTextureViewDimension::TEXTURE_2D;
  viewDesc.mipLevels = levels;
  switch (format) {
    case 0x1A220197:  // D3DFMT_D24FS8
    case 0x2D200196:  // D3DFMT_D24S8
    case 0x28000102:  // D3DFMT_L8
    case 0x28000002:  // D3DFMT_L8_2
      viewDesc.componentMapping =
          RenderComponentMapping(RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::ONE);
      break;
    case 0x28280086:  // D3DFMT_X8R8G8B8
      viewDesc.componentMapping =
          RenderComponentMapping(RenderSwizzle::G, RenderSwizzle::B, RenderSwizzle::A, RenderSwizzle::ONE);
      break;
    default:
      break;
  }
  texture->textureView = texture->texture->createTextureView(viewDesc);

  texture->width = width;
  texture->height = height;
  texture->depth = depth;
  texture->levels = levels;
  texture->format = desc.format;
  texture->requiresHostInitialization =
      desc.flags == RenderTextureFlag::RENDER_TARGET || desc.flags == RenderTextureFlag::DEPTH_TARGET;
  texture->hostInitialized = !texture->requiresHostInitialization;
  texture->viewDimension = viewDesc.dimension;
  texture->descriptorIndex = AllocTextureDescriptor();
  TextureDescriptorSet()->setTexture(texture->descriptorIndex, texture->texture,
                                     RenderTextureLayout::SHADER_READ, texture->textureView.get());
  return texture;
}

GuestSurface* CreateSurface(uint32_t width, uint32_t height, uint32_t format, uint32_t /*multiSample*/) {
  RenderTextureDesc desc;
  desc.dimension = RenderTextureDimension::TEXTURE_2D;
  desc.width = width;
  desc.height = height;
  desc.depth = 1;
  desc.mipLevels = 1;
  desc.arraySize = 1;
  desc.format = ConvertFormat(format);
  const bool depth = RenderFormatIsDepth(desc.format);
  desc.flags = depth ? RenderTextureFlag::DEPTH_TARGET : RenderTextureFlag::RENDER_TARGET;

  auto* surface = GuestNew<GuestSurface>(depth ? ResourceType::DepthStencil : ResourceType::RenderTarget);
  surface->textureHolder = Device()->createTexture(desc);
  surface->texture = surface->textureHolder.get();
  RenderTextureViewDesc viewDesc;
  viewDesc.format = desc.format;
  viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
  viewDesc.mipLevels = 1;
  surface->textureView = surface->texture->createTextureView(viewDesc);
  surface->width = width;
  surface->height = height;
  surface->format = desc.format;
  surface->guestFormat = format;
  surface->requiresHostInitialization = true;
  surface->hostInitialized = false;
  surface->descriptorIndex = AllocTextureDescriptor();
  TextureDescriptorSet()->setTexture(surface->descriptorIndex, surface->texture,
                                     RenderTextureLayout::SHADER_READ, surface->textureView.get());
  return surface;
}

// LockRect returns pitch + a guest-visible staging pointer. Shared by
// D3DDevice_CreateTexture-created textures and D3DDevice_CreateSurface-created
// render targets/depth-stencil surfaces alike (both derive GuestBaseTexture).
void LockRect(GuestBaseTexture* texture, uint32_t* outPitch, uint32_t* outBits) {
  uint32_t pitch = ComputeTexturePitch(texture);
  uint32_t slicePitch = pitch * texture->height;
  if (texture->mappedMemory == nullptr) {
    uint32_t addr = GuestAllocRaw(slicePitch, 0x10);
    texture->mappedMemory = ToHost<void>(addr);
  }
  if (outPitch) *outPitch = pitch;
  if (outBits) *outBits = ToGuest(texture->mappedMemory);
}

void UnlockRect(GuestBaseTexture* texture) {
  if (texture->mappedMemory == nullptr) return;
  uint32_t pitch = ComputeTexturePitch(texture);
  uint32_t slicePitch = pitch * texture->height;

  auto upload = Device()->createBuffer(RenderBufferDesc::UploadBuffer(slicePitch));
  std::memcpy(upload->map(), texture->mappedMemory, slicePitch);
  upload->unmap();

  RenderTexture* dst = texture->texture;
  RenderBuffer* src = upload.get();
  const RenderFormat fmt = texture->format;
  const uint32_t w = texture->width, h = texture->height;
  const uint32_t rowTexels = pitch / FormatBytes(fmt);
  ExecuteUpload([&](RenderCommandList* cl) {
    cl->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(dst, RenderTextureLayout::COPY_DEST));
    cl->copyTextureRegion(RenderTextureCopyLocation::Subresource(dst, 0),
                          RenderTextureCopyLocation::PlacedFootprint(src, fmt, w, h, 1, rowTexels));
  });
  texture->hostInitialized = true;
}

// ---------------------------------------------------------------------------
// Generic unlock dispatch (FM2_D3DResource_UnlockResource): the resource
// pointer's own type tag says whether it's a buffer or a texture/surface.
// ---------------------------------------------------------------------------

// Named to avoid colliding with the Win32 resource-loading UnlockResource
// macro (winbase.h) that ends up visible transitively in this translation
// unit.
void UnlockGuestResource(GuestResource* resource) {
  if (resource == nullptr) return;
  switch (resource->type) {
    case ResourceType::VertexBuffer:
      UnlockVertexBuffer(static_cast<GuestBuffer*>(resource));
      return;
    case ResourceType::IndexBuffer:
      UnlockIndexBuffer(static_cast<GuestBuffer*>(resource));
      return;
    case ResourceType::Texture:
    case ResourceType::VolumeTexture:
    case ResourceType::RenderTarget:
    case ResourceType::DepthStencil:
      UnlockRect(static_cast<GuestBaseTexture*>(resource));
      return;
    default:
      return;
  }
}

// ---------------------------------------------------------------------------
// Vertex declaration
// ---------------------------------------------------------------------------

// Every D3DVERTEXELEMENT9 declaration FM2 creates. Draws match their shader's
// header usage/usageIndex set against this registry to recover the real
// input layout (FM2 never binds a declaration through the device field).
namespace {
std::vector<GuestVertexDeclaration*> g_gameDeclarations;
std::mutex g_gameDeclMutex;
}  // namespace

std::vector<GuestVertexDeclaration*> SnapshotGameDeclarations() {
  std::lock_guard<std::mutex> lock(g_gameDeclMutex);
  return g_gameDeclarations;
}

// Count guest vertex elements up to the D3DDECL_END terminator (stream 0xFF).
GuestVertexDeclaration* CreateVertexDeclaration(const GuestVertexElement* guestElements) {
  uint32_t count = 0;
  while (std::byteswap(guestElements[count].stream) != 0xFF) {
    ++count;
    if (count > 64) break;  // safety
  }

  auto* decl = GuestNew<GuestVertexDeclaration>();
  decl->vertexElementCount = count;
  decl->vertexElements = std::make_unique<GuestVertexElement[]>(count + 1);
  for (uint32_t i = 0; i <= count; ++i) {
    GuestVertexElement& d = decl->vertexElements[i];
    if (i == count) {
      d.stream = 0xFF;
      d.offset = 0;
      d.type = 0xFFFFFFFF;
      d.method = 0;
      d.usage = 0;
      d.usageIndex = 0;
      d.padding = 0;
    } else {
      const auto& s = guestElements[i];
      d.stream = std::byteswap(s.stream);
      d.offset = std::byteswap(s.offset);
      d.type = std::byteswap(s.type);
      d.method = s.method;
      d.usage = s.usage;
      d.usageIndex = s.usageIndex;
      d.padding = s.padding;
    }
    if (i < count && d.stream < 16) decl->vertexStreams[d.stream] = true;
  }
  // RenderInputElement translation + hashing happens at pipeline-build time
  // (Phase 3).
  {
    std::lock_guard<std::mutex> lock(g_gameDeclMutex);
    g_gameDeclarations.push_back(decl);
  }
  return decl;
}

// ---------------------------------------------------------------------------
// Descriptor getters
// ---------------------------------------------------------------------------

void GetSurfaceDesc(const GuestSurface* surface, GuestSurfaceDesc* desc) {
  if (desc == nullptr) return;
  desc->format = surface->guestFormat;
  desc->type = 3 /* D3DRTYPE_SURFACE */;
  desc->usage = 0;
  desc->pool = 0;
  desc->multiSampleType = static_cast<uint32_t>(surface->sampleCount == RenderSampleCount::COUNT_4  ? 2
                                                : surface->sampleCount == RenderSampleCount::COUNT_2 ? 1
                                                                                                     : 0);
  desc->multiSampleQuality = 0;
  desc->width = surface->width;
  desc->height = surface->height;
}

// ---------------------------------------------------------------------------
// DDS texture loading (FM2_D3D_CreateTextureFromMemoryBuffer). Parses the DDS
// blob and uploads it directly -- bypasses the original D3DX
// texture-from-memory pipeline entirely, which (via TileSurface) reads a raw
// GPU-memory-address field out of the D3D9 texture object it creates; our
// GuestTexture has no such field, so letting that original algorithm run
// corrupts memory.
// ---------------------------------------------------------------------------

namespace {

constexpr uint32_t kPitchAlignment = 0x100;
constexpr uint32_t kPlacementAlignment = 0x200;

constexpr uint32_t FourCC(char a, char b, char c, char d) {
  return uint32_t(a) | (uint32_t(b) << 8) | (uint32_t(c) << 16) | (uint32_t(d) << 24);
}

struct DdsInfo {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t depth = 1;
  uint32_t mipCount = 1;
  uint32_t headerSize = 128;
  RenderFormat format = RenderFormat::R8G8B8A8_UNORM;
  uint32_t blockWidth = 1;
  uint32_t blockHeight = 1;
  uint32_t bytesPerBlock = 4;
  bool valid = false;
};

DdsInfo ParseDds(const uint8_t* data, uint32_t size) {
  DdsInfo info;
  if (size < 128 || *reinterpret_cast<const uint32_t*>(data) != FourCC('D', 'D', 'S', ' ')) return info;

  auto rd = [&](uint32_t off) { return *reinterpret_cast<const uint32_t*>(data + off); };
  info.height = rd(12);
  info.width = rd(16);
  info.depth = rd(24) ? rd(24) : 1;
  info.mipCount = rd(28) ? rd(28) : 1;

  const uint32_t pfFlags = rd(80);
  const uint32_t fourCC = rd(84);

  auto setBlock = [&](RenderFormat fmt, uint32_t bpb) {
    info.format = fmt;
    info.blockWidth = info.blockHeight = 4;
    info.bytesPerBlock = bpb;
  };

  if (pfFlags & 0x4) {  // DDPF_FOURCC
    if (fourCC == FourCC('D', 'X', 'T', '1')) {
      setBlock(RenderFormat::BC1_UNORM, 8);
    } else if (fourCC == FourCC('D', 'X', 'T', '3')) {
      setBlock(RenderFormat::BC2_UNORM, 16);
    } else if (fourCC == FourCC('D', 'X', 'T', '5')) {
      setBlock(RenderFormat::BC3_UNORM, 16);
    } else if (fourCC == FourCC('A', 'T', 'I', '2') || fourCC == FourCC('B', 'C', '5', 'U')) {
      setBlock(RenderFormat::BC5_UNORM, 16);
    } else if (fourCC == FourCC('D', 'X', '1', '0')) {
      info.headerSize = 148;
      const uint32_t dxgi = rd(128);  // DDS_HEADER_DXT10.dxgiFormat
      switch (dxgi) {
        case 71:
          setBlock(RenderFormat::BC1_UNORM, 8);
          break;
        case 74:
          setBlock(RenderFormat::BC2_UNORM, 16);
          break;
        case 77:
          setBlock(RenderFormat::BC3_UNORM, 16);
          break;
        case 83:
          setBlock(RenderFormat::BC5_UNORM, 16);
          break;
        case 98:
          setBlock(RenderFormat::BC7_UNORM, 16);
          break;
        case 28:
          info.format = RenderFormat::R8G8B8A8_UNORM;
          info.bytesPerBlock = 4;
          break;
        case 87:
          info.format = RenderFormat::B8G8R8A8_UNORM;
          info.bytesPerBlock = 4;
          break;
        default:
          info.format = RenderFormat::R8G8B8A8_UNORM;
          info.bytesPerBlock = 4;
          break;
      }
    } else {
      info.format = RenderFormat::R8G8B8A8_UNORM;
      info.bytesPerBlock = 4;
    }
  } else {
    // Uncompressed RGB(A). 32-bit assumed (B8G8R8A8 is the common D3D9 layout).
    info.format = RenderFormat::B8G8R8A8_UNORM;
    info.bytesPerBlock = 4;
  }

  info.valid = info.width != 0 && info.height != 0;
  return info;
}

}  // namespace

GuestTexture* LoadTextureFromMemory(const uint8_t* data, uint32_t size) {
  DdsInfo dds = ParseDds(data, size);
  if (!dds.valid) {
    // Unrecognized container: hand back a 1x1 texture so the guest has a
    // valid handle instead of crashing.
    return CreateTexture(1, 1, 1, 1, 0, 0x1A200186 /*A8B8G8R8*/, 0, 0);
  }

  auto* texture = GuestNew<GuestTexture>();
  texture->type = ResourceType::Texture;

  RenderTextureDesc desc;
  desc.dimension = RenderTextureDimension::TEXTURE_2D;
  desc.width = dds.width;
  desc.height = dds.height;
  desc.depth = dds.depth;
  desc.mipLevels = dds.mipCount;
  desc.arraySize = 1;
  desc.format = dds.format;
  desc.flags = RenderTextureFlag::NONE;

  texture->textureHolder = Device()->createTexture(desc);
  texture->texture = texture->textureHolder.get();

  RenderTextureViewDesc viewDesc;
  viewDesc.format = dds.format;
  viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
  viewDesc.mipLevels = dds.mipCount;
  texture->textureView = texture->texture->createTextureView(viewDesc);

  texture->width = dds.width;
  texture->height = dds.height;
  texture->depth = dds.depth;
  texture->levels = dds.mipCount;
  texture->format = dds.format;
  texture->requiresHostInitialization = false;
  texture->hostInitialized = true;
  texture->viewDimension = RenderTextureViewDimension::TEXTURE_2D;
  texture->descriptorIndex = AllocTextureDescriptor();
  TextureDescriptorSet()->setTexture(texture->descriptorIndex, texture->texture,
                                     RenderTextureLayout::SHADER_READ, texture->textureView.get());

  // Lay out the mip chain into an upload buffer with aligned placed footprints.
  struct Slice {
    uint32_t width, height, srcOffset, dstOffset, srcRowPitch, dstRowPitch, rowCount;
  };
  std::vector<Slice> slices;
  uint32_t srcOff = 0, dstOff = 0;
  for (uint32_t mip = 0; mip < dds.mipCount; ++mip) {
    Slice s;
    s.width = std::max(1u, dds.width >> mip);
    s.height = std::max(1u, dds.height >> mip);
    s.srcOffset = srcOff;
    s.dstOffset = dstOff;
    uint32_t blocksW = (s.width + dds.blockWidth - 1) / dds.blockWidth;
    s.srcRowPitch = blocksW * dds.bytesPerBlock;
    s.dstRowPitch = (s.srcRowPitch + kPitchAlignment - 1) & ~(kPitchAlignment - 1);
    s.rowCount = (s.height + dds.blockHeight - 1) / dds.blockHeight;
    srcOff += s.srcRowPitch * s.rowCount;
    dstOff += (s.dstRowPitch * s.rowCount + kPlacementAlignment - 1) & ~(kPlacementAlignment - 1);
    slices.push_back(s);
  }
  if (dds.headerSize + srcOff > size) return texture;  // truncated data; skip upload

  auto upload = Device()->createBuffer(RenderBufferDesc::UploadBuffer(dstOff));
  auto* mapped = reinterpret_cast<uint8_t*>(upload->map());
  for (const Slice& s : slices) {
    const uint8_t* src = data + dds.headerSize + s.srcOffset;
    uint8_t* dst = mapped + s.dstOffset;
    for (uint32_t r = 0; r < s.rowCount; ++r) {
      std::memcpy(dst, src, s.srcRowPitch);
      src += s.srcRowPitch;
      dst += s.dstRowPitch;
    }
  }
  upload->unmap();

  RenderTexture* dstTex = texture->texture;
  RenderBuffer* srcBuf = upload.get();
  const RenderFormat fmt = dds.format;
  const uint32_t blockW = dds.blockWidth, bpb = dds.bytesPerBlock;
  ExecuteUpload([&](RenderCommandList* cl) {
    cl->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(dstTex, RenderTextureLayout::COPY_DEST));
    for (uint32_t i = 0; i < slices.size(); ++i) {
      const Slice& s = slices[i];
      uint32_t rowTexels = (s.dstRowPitch / bpb) * blockW;
      cl->copyTextureRegion(
          RenderTextureCopyLocation::Subresource(dstTex, i),
          RenderTextureCopyLocation::PlacedFootprint(srcBuf, fmt, s.width, s.height, 1, rowTexels, s.dstOffset));
    }
  });

  return texture;
}

// ---------------------------------------------------------------------------
// Vertex/pixel shaders: map guest microcode to its XenosRecomp-translated
// host shader via the generated shader cache, keyed by microcode hash.
// ---------------------------------------------------------------------------

ShaderCacheEntry* FindShaderCacheEntry(uint64_t hash) {
  ShaderCacheEntry* end = g_shaderCacheEntries + g_shaderCacheEntryCount;
  ShaderCacheEntry* it = std::lower_bound(
      g_shaderCacheEntries, end, hash,
      [](const ShaderCacheEntry& lhs, uint64_t rhs) { return lhs.hash < rhs; });
  return (it != end && it->hash == hash) ? it : nullptr;
}

// Shader-container registry for identifying programs that PM4-driven code
// loads by inline microcode copy or by ucode address rather than through a
// SetVertexShader/SetPixelShader call (Phase 4 territory; the registration
// happens at creation time here so it's ready when Phase 4 needs it).
namespace {
struct UcodeContainerRec {
  const uint8_t* host = nullptr;
  uint32_t guest = 0;
  uint32_t size = 0;
  GuestShader* shader = nullptr;
  bool pixel = false;
};
std::mutex g_ucodeIndexMutex;
std::vector<UcodeContainerRec> g_ucodeContainers;
std::unordered_map<uint64_t, GuestShader*> g_inlineUcodeCache;
}  // namespace

void RegisterShaderContainerForUcodeLookup(const uint32_t* function, uint32_t size, GuestShader* shader,
                                           ResourceType type) {
  if (function == nullptr || shader == nullptr || size == 0u || size > 0x40000u) return;
  std::lock_guard lock(g_ucodeIndexMutex);
  for (const auto& r : g_ucodeContainers) {
    if (r.host == reinterpret_cast<const uint8_t*>(function)) return;
  }
  g_ucodeContainers.push_back({reinterpret_cast<const uint8_t*>(function), ToGuest(function), size, shader,
                               type == ResourceType::PixelShader});
}

GuestShader* FindShaderByInlineUcode(const void* ucode, uint32_t bytes, bool pixel) {
  if (ucode == nullptr || bytes < 16u || bytes > 0x20000u) return nullptr;
  const uint64_t h = XXH3_64bits(ucode, bytes) ^ (pixel ? 1ull : 0ull);
  std::lock_guard lock(g_ucodeIndexMutex);
  auto it = g_inlineUcodeCache.find(h);
  if (it != g_inlineUcodeCache.end()) return it->second;
  GuestShader* found = nullptr;
  const uint8_t first = *static_cast<const uint8_t*>(ucode);
  for (const auto& r : g_ucodeContainers) {
    if (r.pixel != pixel || r.size < bytes) continue;
    const uint8_t* end = r.host + (r.size - bytes);
    for (const uint8_t* p = r.host; p <= end; ++p) {
      p = static_cast<const uint8_t*>(std::memchr(p, first, size_t(end - p) + 1u));
      if (p == nullptr) break;
      if (std::memcmp(p, ucode, bytes) == 0) {
        found = r.shader;
        break;
      }
    }
    if (found != nullptr) break;
  }
  g_inlineUcodeCache.emplace(h, found);  // cache negatives too
  return found;
}

GuestShader* FindShaderByUcodeAddress(uint32_t guestAddr, bool pixel) {
  guestAddr &= 0x1FFFFFFFu;
  std::lock_guard lock(g_ucodeIndexMutex);
  for (const auto& r : g_ucodeContainers) {
    const uint32_t base = r.guest & 0x1FFFFFFFu;
    if (r.pixel == pixel && guestAddr >= base && guestAddr < base + r.size) return r.shader;
  }
  return nullptr;
}

namespace {

GuestShader* CreateShaderFromFunction(const uint32_t* function, ResourceType type) {
  // Guest microcode is big-endian; size = header words [1] + [2].
  uint32_t size = std::byteswap(function[1]) + std::byteswap(function[2]);
  uint64_t hash = XXH3_64bits(function, size);

  // Parse the vertex shader's embedded input declaration (usage/usageIndex) from
  // its container header. FM2's vfetch instructions carry no format/offset and
  // FM2 never binds the D3DVERTEXELEMENT9 declaration via the device field, so we
  // match this semantic set against FM2's created declarations (which DO carry
  // format/offset). Layout: ShaderContainer.shaderOffset @ +0x18 -> VertexShader;
  // elements are bitfields { address:12, usage:4, usageIndex:4 } at
  // vertexElementsAndInterpolators[field18 + i].
  std::vector<ShaderHeaderElement> headerEls;
  if (type == ResourceType::VertexShader) {
    const uint32_t totalDw = size / 4u;
    const uint32_t shaderOffset = std::byteswap(function[6]);
    if ((shaderOffset & 3u) == 0u && shaderOffset / 4u + 9u <= totalDw) {
      const uint32_t* vs = function + shaderOffset / 4u;
      const uint32_t field18 = std::byteswap(vs[6]);
      const uint32_t veCount = std::byteswap(vs[7]);
      for (uint32_t i = 0; i < veCount && i < 32u; ++i) {
        const uint32_t idx = shaderOffset / 4u + 9u + field18 + i;
        if (idx >= totalDw) break;
        const uint32_t v = std::byteswap(function[idx]);
        headerEls.push_back(ShaderHeaderElement{uint8_t((v >> 12) & 0xFu), uint8_t((v >> 16) & 0xFu)});
      }
    }
  }

  auto finish = [&](GuestShader* s) -> GuestShader* {
    if (s != nullptr && type == ResourceType::VertexShader && s->headerElements.empty()) {
      s->headerElements = headerEls;
    }
    return s;
  };

  if (ShaderCacheEntry* entry = FindShaderCacheEntry(hash)) {
    if (entry->guest_shader == nullptr) {
      auto* shader = GuestNew<GuestShader>(type);
      shader->shaderCacheEntry = entry;
      entry->guest_shader = reinterpret_cast<GuestShader*>(shader);
      REXGPU_INFO("CreateShader: hash=0x{:016X} type={} -> guestAddr=0x{:08X}", hash, int(type),
                  ToGuest(shader));
      RegisterShaderContainerForUcodeLookup(function, size, shader, type);
      return finish(shader);
    }
    RegisterShaderContainerForUcodeLookup(
        function, size, reinterpret_cast<GuestShader*>(entry->guest_shader), type);
    return finish(reinterpret_cast<GuestShader*>(entry->guest_shader));
  }

  // Dump the raw ShaderContainer so XenosRecomp can translate it offline
  // (missed_shaders/*.bin is a real, still-used mechanism -- see
  // docs/migration-from-plume.md on the seed shader cache's provenance).
  static std::unordered_set<uint64_t> s_dumped;
  if (s_dumped.insert(hash).second) {
    std::filesystem::create_directories("missed_shaders");
    char path[64];
    std::snprintf(path, sizeof(path), "missed_shaders/%016llX.bin", static_cast<unsigned long long>(hash));
    if (FILE* f = std::fopen(path, "wb")) {
      std::fwrite(function, 1, size, f);
      std::fclose(f);
    }
    REXGPU_WARN("Shader cache MISS: hash=0x{:016X} size={} type={} -- dumped to {}", hash, size,
               int(type), path);
  }
  return finish(GuestNew<GuestShader>(type));
}

}  // namespace

GuestShader* CreateVertexShader(const uint32_t* function) {
  return CreateShaderFromFunction(function, ResourceType::VertexShader);
}

GuestShader* CreatePixelShader(const uint32_t* function) {
  return CreateShaderFromFunction(function, ResourceType::PixelShader);
}

namespace {
std::mutex g_shaderAliasMutex;
std::unordered_map<uint32_t, GuestShader*> g_shaderAliases;
}  // namespace

void RegisterShaderAlias(uint32_t guestAddress, GuestShader* shader) {
  if (!guestAddress || shader == nullptr) return;
  std::lock_guard lock(g_shaderAliasMutex);
  g_shaderAliases[guestAddress] = shader;
}

GuestShader* LookupShaderAlias(uint32_t guestAddress) {
  if (!guestAddress) return nullptr;
  std::lock_guard lock(g_shaderAliasMutex);
  auto it = g_shaderAliases.find(guestAddress);
  return it != g_shaderAliases.end() ? it->second : nullptr;
}

}  // namespace fm2::render
