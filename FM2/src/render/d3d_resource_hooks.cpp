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
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <plume_render_interface.h>
#include <rex/hash.h>  // XXH3_64bits
#include <rex/logging.h>

#include "render/guest_device.h"
#include "render/guest_heap.h"
#include "render/guest_resources.h"
#include "render/render_commands.h"
#include "render/render_internal.h"
#include "render/render_queue.h"
#include "render/render_state.h"

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
    case 0x2D20014A:  // k_8_8 (R8G8) variant — same 0x2D20 class as D24S8/G16R16F
      return RenderFormat::R8G8_UNORM;
  }

  switch (d3dFormat & 0x3F) {
    case 10:  // k_8_8
      return RenderFormat::R8G8_UNORM;
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
    case RenderFormat::R8G8_UNORM:
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
  buffer->buffer = Device()->createBuffer(
      RenderBufferDesc::VertexBuffer(length, BufferHeapType(), RenderBufferFlag::INDEX));
  buffer->dataSize = length;
  return buffer;
}

GuestBuffer* CreateIndexBuffer(uint32_t length, uint32_t format) {
  auto* buffer = GuestNew<GuestBuffer>(ResourceType::IndexBuffer);
  buffer->buffer = Device()->createBuffer(RenderBufferDesc::IndexBuffer(length, BufferHeapType()));
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
  if (buffer->lockedReadOnly || buffer->mappedMemory == nullptr)
    return;
  if (!buffer->buffer)
    return;

  RenderCommand cmd{};
  cmd.type = (sizeof(T) == 2) ? RenderCommandType::UnlockBuffer16 : RenderCommandType::UnlockBuffer32;
  cmd.unlockBuffer.buffer = buffer;
  // Async like Unleashed: FIFO with later draws/Present ensures the upload
  // lands before the buffer is sampled.
  RenderQueue::Enqueue(cmd);
}

template <typename T>
static void ProcUnlockBufferT(GuestBuffer* buffer) {
  if (buffer == nullptr || buffer->mappedMemory == nullptr || buffer->buffer == nullptr) return;

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
    return;
  }

  auto upload = Device()->createBuffer(RenderBufferDesc::UploadBuffer(buffer->dataSize));
  if (!upload) {
    REXGPU_ERROR("UnlockBuffer: failed to create staging upload buffer (size={})", buffer->dataSize);
    return;
  }
  T* dest = reinterpret_cast<T*>(upload->map());
  if (dest == nullptr) return;
  swapInto(dest);
  upload->unmap();

  RenderBuffer* dst = buffer->buffer.get();
  RenderBuffer* srcBuf = upload.get();
  const uint64_t size = buffer->dataSize;
  // Graphics CL path (Unleashed non-copy-queue): keep staging until frame fence.
  RenderCommandList* cl = CommandList();
  if (cl == nullptr) return;
  cl->barriers(RenderBarrierStage::COPY, RenderBufferBarrier(dst, RenderBufferAccess::WRITE));
  cl->copyBufferRegion(dst->at(0), srcBuf->at(0), size);
  cl->barriers(RenderBarrierStage::GRAPHICS, RenderBufferBarrier(dst, RenderBufferAccess::READ));
  RetainTempUploadBuffer(std::move(upload));
}

void ProcUnlockBuffer16(GuestBuffer* buffer) { ProcUnlockBufferT<uint16_t>(buffer); }
void ProcUnlockBuffer32(GuestBuffer* buffer) { ProcUnlockBufferT<uint32_t>(buffer); }

uint32_t LockVertexBuffer(GuestBuffer* buffer, uint32_t flags) {
  return LockBuffer(buffer, flags);
}
void UnlockVertexBuffer(GuestBuffer* buffer) {
  UploadBufferSwapped<uint32_t>(buffer);
}

uint32_t LockIndexBuffer(GuestBuffer* buffer, uint32_t flags) {
  return LockBuffer(buffer, flags);
}
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
                            uint32_t usage, uint32_t format, uint32_t /*pool*/, uint32_t type) {
  if (IsDeviceLost()) {
    auto* texture = GuestNew<GuestTexture>();
    texture->type = (type == 17) ? ResourceType::VolumeTexture : ResourceType::Texture;
    texture->width = width;
    texture->height = height;
    texture->depth = depth;
    texture->levels = levels == 0 ? 1 : levels;
    texture->format = ConvertFormat(format);
    return texture;
  }

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

  RenderCommand cmd{};
  cmd.type = RenderCommandType::CreateTextureHost;
  cmd.createTextureHost.texture = texture;
  cmd.createTextureHost.width = width;
  cmd.createTextureHost.height = height;
  cmd.createTextureHost.depth = depth;
  cmd.createTextureHost.levels = levels;
  cmd.createTextureHost.usage = usage;
  cmd.createTextureHost.format = format;
  cmd.createTextureHost.volume = volume;
  RenderQueue::Run(cmd);
  return texture;
}

// EXPERIMENT (bug-053 follow-up): the title reads its own D3D header back off
// resources we allocate -- GetSurfaceLayout takes the GPU format from
// fetch-constant dword1 and looks up bits-per-pixel from it, so a zeroed header
// reads as format 0 -> 1bpp -> the AlignTextureDimensions divide-by-zero trap at
// 0x8236A4D8. Publish format and dimensions (bit layout per
// ParseTextureFetchConstant); base address is deliberately left 0.
void PublishGuestFetchConstant(GuestBaseTexture* resource, uint32_t guestFormat, uint32_t width,
                               uint32_t height) {
  if (resource == nullptr) return;
  auto* fc = reinterpret_cast<rex::be<uint32_t>*>(reinterpret_cast<uint8_t*>(resource) + 0x18);
  const uint32_t pitchBlocks = ((width + 31u) / 32u) & 0x1FFu;
  fc[0] = pitchBlocks << 22;
  fc[1] = guestFormat & 0x3Fu;
  fc[2] = ((width ? width - 1u : 0u) & 0x1FFFu) | (((height ? height - 1u : 0u) & 0x1FFFu) << 13);
}

void ProcCreateTextureHost(GuestTexture* texture, uint32_t width, uint32_t height, uint32_t depth,
                           uint32_t levels, uint32_t usage, uint32_t format, bool volume) {
  if (texture == nullptr || IsDeviceLost()) return;
  PublishGuestFetchConstant(texture, format, width, height);

  RenderTextureDesc desc;
  desc.dimension = volume ? RenderTextureDimension::TEXTURE_3D : RenderTextureDimension::TEXTURE_2D;
  desc.width = width;
  desc.height = height;
  desc.depth = depth;
  desc.mipLevels = levels;
  desc.arraySize = 1;
  desc.format = ConvertFormat(format);
  // Match Unleashed: only request RT when the guest usage asks for it.
  if (RenderFormatIsDepth(desc.format)) {
    desc.flags = RenderTextureFlag::DEPTH_TARGET;
  } else if (usage != 0) {
    desc.flags = RenderTextureFlag::RENDER_TARGET;
  } else {
    desc.flags = RenderTextureFlag::NONE;
  }

  texture->textureHolder = Device()->createTexture(desc);
  texture->texture = texture->textureHolder.get();
  if (texture->texture == nullptr) {
    NoteDeviceLost("CreateTexture");
    REXGPU_ERROR(
        "CreateTexture: Plume createTexture failed ({}x{}x{} levels={} fmt=0x{:08X} type={} usage={})",
        width, height, depth, levels, format, volume ? 17 : 0, usage);
    texture->width = width;
    texture->height = height;
    texture->depth = depth;
    texture->levels = levels;
    texture->format = desc.format;
    return;
  }

  RenderTextureViewDesc viewDesc;
  viewDesc.format = desc.format;
  viewDesc.dimension =
      volume ? RenderTextureViewDimension::TEXTURE_3D : RenderTextureViewDimension::TEXTURE_2D;
  viewDesc.mipLevels = levels;
  switch (format) {
    case 0x1A220197:  // D3DFMT_D24FS8
    case 0x2D200196:  // D3DFMT_D24S8
    case 0x28000102:  // D3DFMT_L8
    case 0x28000002:  // D3DFMT_L8_2
      viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::R, RenderSwizzle::R,
                                                         RenderSwizzle::R, RenderSwizzle::ONE);
      break;
    case 0x28280086:  // D3DFMT_X8R8G8B8
      viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::G, RenderSwizzle::B,
                                                         RenderSwizzle::A, RenderSwizzle::ONE);
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
  texture->requiresHostInitialization = desc.flags == RenderTextureFlag::RENDER_TARGET ||
                                        desc.flags == RenderTextureFlag::DEPTH_TARGET;
  texture->hostInitialized = !texture->requiresHostInitialization;
  texture->viewDimension = viewDesc.dimension;
  texture->descriptorIndex = AllocTextureDescriptor();
  TextureDescriptorSet()->setTexture(texture->descriptorIndex, texture->texture,
                                     RenderTextureLayout::SHADER_READ, texture->textureView.get());
}

void ProcCreateTranslatedTextureHost(GuestTexture* texture, uint32_t width, uint32_t height,
                                     uint32_t format, uint32_t baseAddress, bool* createdOut) {
  if (createdOut != nullptr) *createdOut = false;
  if (texture == nullptr || IsDeviceLost()) return;

  const auto fmt = static_cast<RenderFormat>(format);
  RenderTextureDesc desc;
  desc.dimension = RenderTextureDimension::TEXTURE_2D;
  desc.width = width;
  desc.height = height;
  desc.depth = 1;
  desc.mipLevels = 1;
  desc.arraySize = 1;
  desc.format = fmt;
  // Only request RT for formats that can actually be render targets. BC/etc.
  // translated textures must stay COPY_DEST+SRV — marking them RT removes the
  // device. Aperture/frontbuffer (BGRA8) needs RT for format-mismatch shader blit.
  const bool colorRt = !RenderFormatIsDepth(fmt) && fmt != RenderFormat::UNKNOWN &&
                       fmt < RenderFormat::BC1_TYPELESS;
  desc.flags = colorRt ? RenderTextureFlag::RENDER_TARGET : RenderTextureFlag::NONE;
  texture->textureHolder = Device()->createTexture(desc);
  texture->texture = texture->textureHolder.get();
  if (texture->texture == nullptr) {
    static std::unordered_set<uint32_t> s_warned;
    if (s_warned.insert(baseAddress).second) {
      NoteDeviceLost("TranslateGuestTexture");
      REXGPU_WARN("TranslateGuestTexture: failed to create {}x{} fmt={} texture (base=0x{:08X})",
                  width, height, int(fmt), baseAddress);
    }
    return;
  }

  RenderTextureViewDesc viewDesc;
  viewDesc.format = fmt;
  viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
  viewDesc.mipLevels = 1;
  if (fmt == RenderFormat::R8_UNORM) {
    viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::R, RenderSwizzle::R,
                                                       RenderSwizzle::R, RenderSwizzle::ONE);
  }
  texture->textureView = texture->texture->createTextureView(viewDesc);
  texture->width = width;
  texture->height = height;
  texture->depth = 1;
  texture->levels = 1;
  texture->format = fmt;
  texture->requiresHostInitialization = false;
  texture->hostInitialized = true;
  texture->viewDimension = RenderTextureViewDimension::TEXTURE_2D;
  texture->descriptorIndex = AllocTextureDescriptor();
  TextureDescriptorSet()->setTexture(texture->descriptorIndex, texture->texture,
                                     RenderTextureLayout::SHADER_READ, texture->textureView.get());
  if (createdOut != nullptr) *createdOut = true;
}

GuestSurface* CreateSurface(uint32_t width, uint32_t height, uint32_t format,
                            uint32_t multiSample) {
  if (IsDeviceLost()) {
    const bool depth = RenderFormatIsDepth(ConvertFormat(format));
    auto* surface =
        GuestNew<GuestSurface>(depth ? ResourceType::DepthStencil : ResourceType::RenderTarget);
    surface->width = width;
    surface->height = height;
    surface->format = ConvertFormat(format);
    surface->guestFormat = format;
    return surface;
  }

  // Xbox D3DMULTISAMPLE_TYPE: 0=NONE, 2=2x, 4=4x, ...
  RenderSampleCounts sampleCount = RenderSampleCount::COUNT_1;
  if (multiSample >= 8) {
    sampleCount = RenderSampleCount::COUNT_8;
  } else if (multiSample >= 4) {
    sampleCount = RenderSampleCount::COUNT_4;
  } else if (multiSample >= 2) {
    sampleCount = RenderSampleCount::COUNT_2;
  }

  const bool depth = RenderFormatIsDepth(ConvertFormat(format));
  auto* surface =
      GuestNew<GuestSurface>(depth ? ResourceType::DepthStencil : ResourceType::RenderTarget);

  RenderCommand cmd{};
  cmd.type = RenderCommandType::CreateSurfaceHost;
  cmd.createSurfaceHost.surface = surface;
  cmd.createSurfaceHost.width = width;
  cmd.createSurfaceHost.height = height;
  cmd.createSurfaceHost.format = format;
  cmd.createSurfaceHost.sampleCount = sampleCount;
  cmd.createSurfaceHost.depth = depth;
  RenderQueue::Run(cmd);
  return surface;
}

void ProcCreateSurfaceHost(GuestSurface* surface, uint32_t width, uint32_t height, uint32_t format,
                           uint32_t sampleCount, bool depth) {
  if (surface == nullptr || IsDeviceLost()) return;
  PublishGuestFetchConstant(surface, format, width, height);

  // FM2 EDRAM tile surfaces are created as 1280x256. Hardware would replay the
  // recorded pass per band; we never see those replays, so allocate the host
  // texture at full 720p up front (no mid-CL recreate / DEVICE_REMOVED).
  uint32_t hostWidth = width;
  uint32_t hostHeight = height;
  if (width == kFm2FrameWidth && height == kFm2TileHeight) {
    surface->tileGrownFromHeight = height;
    hostHeight = kFm2FrameHeight;
    static uint64_t growAtCreate = 0;
    ++growAtCreate;
    if (growAtCreate <= 12 || growAtCreate % 300 == 1) {
      REXGPU_INFO("CreateSurface: tile {}x{} -> host {}x{} depth={} (n={})", width, height,
                  hostWidth, hostHeight, depth, growAtCreate);
    }
  }

  RenderTextureDesc desc;
  desc.dimension = RenderTextureDimension::TEXTURE_2D;
  desc.width = hostWidth;
  desc.height = hostHeight;
  desc.depth = 1;
  desc.mipLevels = 1;
  desc.arraySize = 1;
  desc.format = ConvertFormat(format);
  desc.flags = depth ? RenderTextureFlag::DEPTH_TARGET : RenderTextureFlag::RENDER_TARGET;
  desc.multisampling.sampleCount = sampleCount;

  surface->textureHolder = Device()->createTexture(desc);
  surface->texture = surface->textureHolder.get();
  if (surface->texture == nullptr) {
    NoteDeviceLost("CreateSurface");
    REXGPU_ERROR("CreateSurface: Plume createTexture failed ({}x{} fmt=0x{:08X} msaa={})", hostWidth,
                 hostHeight, format, int(sampleCount));
    surface->width = hostWidth;
    surface->height = hostHeight;
    surface->format = desc.format;
    surface->guestFormat = format;
    surface->sampleCount = sampleCount;
    return;
  }
  RenderTextureViewDesc viewDesc;
  viewDesc.format = desc.format;
  viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
  viewDesc.mipLevels = 1;
  surface->textureView = surface->texture->createTextureView(viewDesc);
  if (surface->textureView == nullptr) {
    NoteDeviceLost("CreateSurface view");
    REXGPU_ERROR("CreateSurface: createTextureView failed ({}x{})", hostWidth, hostHeight);
    surface->textureHolder.reset();
    surface->texture = nullptr;
    surface->width = hostWidth;
    surface->height = hostHeight;
    surface->format = desc.format;
    surface->guestFormat = format;
    surface->sampleCount = sampleCount;
    return;
  }
  surface->width = hostWidth;
  surface->height = hostHeight;
  surface->format = desc.format;
  surface->guestFormat = format;
  surface->sampleCount = sampleCount;
  surface->requiresHostInitialization = true;
  surface->hostInitialized = false;
  surface->descriptorIndex = AllocTextureDescriptor();
  TextureDescriptorSet()->setTexture(surface->descriptorIndex, surface->texture,
                                     RenderTextureLayout::SHADER_READ, surface->textureView.get());
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
  if (outPitch)
    *outPitch = pitch;
  if (outBits)
    *outBits = ToGuest(texture->mappedMemory);
}

void UnlockRect(GuestBaseTexture* texture) {
  if (texture == nullptr || texture->mappedMemory == nullptr || texture->texture == nullptr)
    return;
  RenderCommand cmd{};
  cmd.type = RenderCommandType::UnlockTextureRect;
  cmd.unlockTextureRect.texture = texture;
  RenderQueue::Enqueue(cmd);
}

void ProcUnlockTextureRect(GuestBaseTexture* texture) {
  if (texture == nullptr || texture->mappedMemory == nullptr || texture->texture == nullptr)
    return;

  uint32_t pitch = ComputeTexturePitch(texture);
  uint32_t slicePitch = pitch * texture->height;

  RenderBufferReference ref = UploadFrameData(texture->mappedMemory, slicePitch, false);
  if (ref.ref == nullptr) {
    // Frame upload exhausted -- fall back to a dedicated staging buffer + copy queue.
    auto upload = Device()->createBuffer(RenderBufferDesc::UploadBuffer(slicePitch));
    if (!upload) {
      REXGPU_ERROR("UnlockTextureRect: failed to create staging upload buffer (size={})", slicePitch);
      return;
    }
    void* mapped = upload->map();
    if (mapped == nullptr) return;
    std::memcpy(mapped, texture->mappedMemory, slicePitch);
    upload->unmap();

    // Already on the render thread (Dispatch); call Proc* directly so staging
    // stays live across the copy-queue submit without nested Run().
    ProcCopyTextureFromUpload(texture->texture, upload.get(), uint32_t(texture->format),
                              texture->width, texture->height,
                              pitch / FormatBytes(texture->format), 0, 0);
    texture->hostInitialized = true;
    texture->layout = RenderTextureLayout::COPY_DEST;
    return;
  }

  RenderCommandList* cl = CommandList();
  if (cl == nullptr) return;
  cl->barriers(RenderBarrierStage::COPY,
               RenderTextureBarrier(texture->texture, RenderTextureLayout::COPY_DEST));
  cl->copyTextureRegion(
      RenderTextureCopyLocation::Subresource(texture->texture, 0),
      RenderTextureCopyLocation::PlacedFootprint(ref.ref, texture->format, texture->width,
                                                 texture->height, 1,
                                                 pitch / FormatBytes(texture->format), ref.offset));
  texture->layout = RenderTextureLayout::COPY_DEST;
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
  if (resource == nullptr)
    return;
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
// Guest (XG-header) texture translation.
//
// FM2 also creates some textures via the low-level XG* XDK API
// (XGSetTextureHeader + a raw Xenos fetch constant written directly into the
// header) instead of always going through D3DDevice_CreateTexture. Those
// objects carry no kFm2ResourceMagic tag, so D3DDevice_SetTexture can't bind
// them the normal pure-replace way. This section parses the header's fetch
// constant directly and materializes a native GuestTexture from the guest's
// own (possibly tiled/packed) texture data, so SetTexture has something real
// to bind instead of falling back to null -- the underlying black-screen
// symptom this port fixes.
// ---------------------------------------------------------------------------

namespace {

uint32_t TiledOffset2D(uint32_t x, uint32_t y, uint32_t width, uint32_t bytesPerElement) {
  uint32_t alignedWidth = (width + 31) & ~31u;
  uint32_t logBpp = (bytesPerElement >> 2) + ((bytesPerElement >> 1) >> (bytesPerElement >> 2));
  uint32_t macro = ((x >> 5) + (y >> 5) * (alignedWidth >> 5)) << (logBpp + 7);
  uint32_t micro = ((x & 7) + ((y & 6) << 2)) << logBpp;
  uint32_t offset =
      macro + ((micro & ~15u) << 1) + (micro & 15u) + ((y & 8) << (3 + logBpp)) + ((y & 1) << 4);
  return (((offset & ~511u) << 3) + ((offset & 448u) << 2) + (offset & 63u) + ((y & 16) << 7) +
          (((((y & 8) >> 2) + (x >> 3)) & 3) << 6)) >>
         logBpp;
}

struct XenosTextureInfo {
  RenderFormat format = RenderFormat::UNKNOWN;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t baseAddress = 0;  // guest byte address of mip 0
  uint32_t pitchTexels = 0;  // row pitch in texels
  uint32_t blockDim = 1;     // 4 for BC formats
  uint32_t bytesPerBlock = 4;
  uint32_t endian = 0;  // 0 none, 1 = 8-in-16, 2 = 8-in-32
  bool tiled = false;
  bool packedMips = false;
  bool valid = false;
};

void GetPackedBaseOffsetBlocks(const XenosTextureInfo& info, uint32_t& outX, uint32_t& outY) {
  outX = 0;
  outY = 0;
  if (!info.packedMips)
    return;
  const uint32_t log2Width = uint32_t(std::bit_width(info.width - 1));
  const uint32_t log2Height = uint32_t(std::bit_width(info.height - 1));
  if (std::min(log2Width, log2Height) > 4)
    return;  // min dimension > 16 texels: not packed
  uint32_t xTexels = 0, yTexels = 0;
  if (log2Width > log2Height)
    yTexels = 16;  // wider than tall: laid out vertically
  else
    xTexels = 16;  // taller than wide (or square): laid out horizontally
  outX = xTexels / info.blockDim;
  outY = yTexels / info.blockDim;
}

// GPUTEXTURE_FETCH_CONSTANT: 6 dwords, big-endian.
XenosTextureInfo ParseTextureFetchConstant(const rex::be<uint32_t>* fc) {
  XenosTextureInfo info;
  const uint32_t fc0 = fc[0].get();
  const uint32_t fc1 = fc[1].get();
  const uint32_t fc2 = fc[2].get();

  info.pitchTexels = ((fc0 >> 22) & 0x1FF) * 32;
  info.tiled = (fc0 >> 31) != 0;
  const uint32_t gpuFormat = fc1 & 0x3F;
  info.endian = (fc1 >> 6) & 0x3;
  info.baseAddress = ((fc1 >> 12) & 0xFFFFF) << 12;
  info.width = (fc2 & 0x1FFF) + 1;
  info.height = ((fc2 >> 13) & 0x1FFF) + 1;
  info.packedMips = ((fc[5].get() >> 11) & 0x1) != 0;

  switch (gpuFormat) {
    case 2:  // k_8 (L8/A8)
      info.format = RenderFormat::R8_UNORM;
      info.bytesPerBlock = 1;
      break;
    case 6:  // k_8_8_8_8 (A8R8G8B8 cooked; 8-in-32 swap yields BGRA bytes)
      info.format = RenderFormat::B8G8R8A8_UNORM;
      info.bytesPerBlock = 4;
      break;
    case 10:  // k_8_8
      info.format = RenderFormat::R8G8_UNORM;
      info.bytesPerBlock = 2;
      break;
    case 18:  // k_DXT1
      info.format = RenderFormat::BC1_UNORM;
      info.blockDim = 4;
      info.bytesPerBlock = 8;
      break;
    case 19:  // k_DXT2_3
      info.format = RenderFormat::BC2_UNORM;
      info.blockDim = 4;
      info.bytesPerBlock = 16;
      break;
    case 20:  // k_DXT4_5
      info.format = RenderFormat::BC3_UNORM;
      info.blockDim = 4;
      info.bytesPerBlock = 16;
      break;
    case 26:  // k_16_16_16_16
      info.format = RenderFormat::R16G16B16A16_UNORM;
      info.bytesPerBlock = 8;
      break;
    case 32:  // k_16_16_16_16_FLOAT (scene-color resolve targets)
      info.format = RenderFormat::R16G16B16A16_FLOAT;
      info.bytesPerBlock = 8;
      break;
    default: {
      static std::unordered_set<uint32_t> s_warnedFormats;
      if (s_warnedFormats.insert(gpuFormat).second) {
        REXGPU_WARN("TranslateGuestTexture: unsupported Xenos format {} ({}x{})", gpuFormat,
                    info.width, info.height);
      }
      return info;
    }
  }
  if (info.pitchTexels == 0)
    info.pitchTexels = info.width;
  info.valid = info.width != 0 && info.height != 0 && info.baseAddress != 0;
  return info;
}

void EndianSwapBuffer(uint8_t* data, size_t size, uint32_t endian) {
  if (endian == 1) {
    auto* p = reinterpret_cast<uint16_t*>(data);
    for (size_t i = 0; i < size / 2; ++i)
      p[i] = std::byteswap(p[i]);
  } else if (endian == 2) {
    auto* p = reinterpret_cast<uint32_t*>(data);
    for (size_t i = 0; i < size / 4; ++i)
      p[i] = std::byteswap(p[i]);
  }
}

bool UploadGuestTextureData(GuestTexture* texture, const XenosTextureInfo& info) {
  if (texture == nullptr || texture->texture == nullptr || !info.valid)
    return false;
  if (texture->width != info.width || texture->height != info.height ||
      texture->format != info.format) {
    return false;
  }

  const uint32_t wBlocks = (info.width + info.blockDim - 1) / info.blockDim;
  const uint32_t hBlocks = (info.height + info.blockDim - 1) / info.blockDim;
  const uint32_t pitchBlocks = std::max(wBlocks, info.pitchTexels / info.blockDim);

  // Small textures live at a block offset inside their packed 32x32 tile.
  uint32_t packedX = 0, packedY = 0;
  GetPackedBaseOffsetBlocks(info, packedX, packedY);

  // Guard against reading unmapped guest memory: textures bound from the PM4
  // command stream point at raw GPU memory that may not be heap-backed. A
  // tiled texture's footprint spans pitchBlocks * align(hBlocks,32) blocks;
  // require the whole conservative footprint to be readable or we'd fault.
  const uint32_t alignedHBlocks = (hBlocks + packedY + 31u) & ~31u;
  const uint64_t footprint = uint64_t(pitchBlocks + packedX) * alignedHBlocks * info.bytesPerBlock;
  // Fetch-constant bases are guest PHYSICAL addresses -- the game writes
  // texture data through its physical-memory aliases, so gate readability on
  // the physical heap's commit state, not the virtual one.
  auto* mem = ghp::GuestMemory();
  const uint32_t physBase = info.baseAddress & 0x1FFFFFFFu;
  const bool sizeOk = footprint != 0 && footprint <= 0x4000000ull;
  const bool readable =
      sizeOk && (physBase + footprint) <= 0x20000000ull &&
      mem->GetPhysicalHeap()->QueryRangeAccess(physBase, uint32_t(physBase + footprint - 1)) !=
          rex::memory::PageAccess::kNoAccess;
  if (!readable) {
    static std::unordered_set<uint32_t> s_warned;
    if (s_warned.insert(info.baseAddress).second) {
      REXGPU_WARN(
          "UploadGuestTextureData: base 0x{:08X} footprint {} not readable ({}x{} fmt={} "
          "tiled={}) -- skipped",
          info.baseAddress, footprint, info.width, info.height, int(info.format), info.tiled);
    }
    return false;
  }

  std::vector<uint8_t> linear(size_t(wBlocks) * hBlocks * info.bytesPerBlock);
  const uint8_t* src = mem->TranslatePhysical<const uint8_t*>(info.baseAddress);

  if (info.tiled) {
    for (uint32_t by = 0; by < hBlocks; ++by) {
      for (uint32_t bx = 0; bx < wBlocks; ++bx) {
        const uint32_t element =
            TiledOffset2D(bx + packedX, by + packedY, pitchBlocks, info.bytesPerBlock);
        // Bounds guard: TiledOffset2D can overshoot the committed footprint
        // for small/odd dims -> OOB read/crash. Skip if so.
        if (uint64_t(element) * info.bytesPerBlock + info.bytesPerBlock > footprint)
          continue;
        std::memcpy(linear.data() + (size_t(by) * wBlocks + bx) * info.bytesPerBlock,
                    src + size_t(element) * info.bytesPerBlock, info.bytesPerBlock);
      }
    }
  } else {
    for (uint32_t by = 0; by < hBlocks; ++by) {
      std::memcpy(linear.data() + size_t(by) * wBlocks * info.bytesPerBlock,
                  src + (size_t(by + packedY) * pitchBlocks + packedX) * info.bytesPerBlock,
                  size_t(wBlocks) * info.bytesPerBlock);
    }
  }
  EndianSwapBuffer(linear.data(), linear.size(), info.endian);

  const uint32_t srcRowPitch = wBlocks * info.bytesPerBlock;
  const uint32_t dstRowPitch = (srcRowPitch + 255u) & ~255u;
  auto upload =
      Device()->createBuffer(RenderBufferDesc::UploadBuffer(size_t(dstRowPitch) * hBlocks));
  if (!upload) {
    REXGPU_ERROR("UploadGuestTextureData: failed to create staging upload buffer (size={})",
                 size_t(dstRowPitch) * hBlocks);
    return false;
  }
  auto* mapped = reinterpret_cast<uint8_t*>(upload->map());
  if (mapped == nullptr)
    return false;
  for (uint32_t by = 0; by < hBlocks; ++by) {
    std::memcpy(mapped + size_t(by) * dstRowPitch, linear.data() + size_t(by) * srcRowPitch,
                srcRowPitch);
  }
  upload->unmap();

  const uint32_t rowTexels = (dstRowPitch / info.bytesPerBlock) * info.blockDim;
  RenderCommand cmd{};
  cmd.type = RenderCommandType::CopyTextureFromUpload;
  cmd.copyTextureFromUpload.dst = texture->texture;
  cmd.copyTextureFromUpload.src = upload.get();
  cmd.copyTextureFromUpload.format = uint32_t(info.format);
  cmd.copyTextureFromUpload.width = info.width;
  cmd.copyTextureFromUpload.height = info.height;
  cmd.copyTextureFromUpload.rowTexels = rowTexels;
  cmd.copyTextureFromUpload.mip = 0;
  cmd.copyTextureFromUpload.srcOffset = 0;
  RenderQueue::Run(cmd);
  texture->hostInitialized = true;
  texture->layout = RenderTextureLayout::COPY_DEST;
  return true;
}

std::mutex g_guestTextureAliasMutex;
std::unordered_map<uint32_t, GuestTexture*> g_guestTextureAliases;
std::vector<std::unique_ptr<GuestTexture>> g_guestTextureStorage;
// Avoid hammering CreateResource after DEVICE_REMOVED / permanent create fails.
std::unordered_set<uint32_t> g_failedGuestTextureBases;

// Shared by TranslateGuestTexture/TranslateGuestTextureFetch: builds a native
// GuestTexture for a parsed Xenos fetch constant, uploads its guest data if
// requested, and publishes it into the alias table under info.baseAddress.
GuestTexture* CreateAndRegisterGuestTexture(const XenosTextureInfo& info, bool uploadGuestData) {
  if (g_failedGuestTextureBases.contains(info.baseAddress)) return nullptr;

  auto textureStorage = std::make_unique<GuestTexture>();
  GuestTexture* texture = textureStorage.get();
  texture->type = ResourceType::Texture;

  bool created = false;
  RenderCommand cmd{};
  cmd.type = RenderCommandType::CreateTranslatedTextureHost;
  cmd.createTranslatedTextureHost.texture = texture;
  cmd.createTranslatedTextureHost.width = info.width;
  cmd.createTranslatedTextureHost.height = info.height;
  cmd.createTranslatedTextureHost.format = uint32_t(info.format);
  cmd.createTranslatedTextureHost.baseAddress = info.baseAddress;
  cmd.createTranslatedTextureHost.createdOut = &created;
  RenderQueue::Run(cmd);

  if (!created) {
    g_failedGuestTextureBases.insert(info.baseAddress);
    return nullptr;
  }

  if (uploadGuestData) UploadGuestTextureData(texture, info);

  REXGPU_INFO(
      "TranslateGuestTexture: base=0x{:08X} {}x{} fmt={} tiled={} endian={} upload={} -> desc {}",
      info.baseAddress, info.width, info.height, int(info.format), info.tiled, info.endian,
      uploadGuestData, texture->descriptorIndex);

  GuestTexture* result = texture;
  std::lock_guard<std::mutex> lock(g_guestTextureAliasMutex);
  g_guestTextureStorage.push_back(std::move(textureStorage));
  g_guestTextureAliases[info.baseAddress] = result;
  return result;
}

}  // namespace

// Translates a raw Xenos GPUTEXTURE_FETCH_CONSTANT (as written by the guest
// PM4 command stream / XG* API, not a GuestTexture) into a native texture.
// Re-uploads a cache hit's guest data at most once per frame rather than
// once per draw -- many draws sample the same texture every frame, and
// uploading per draw would flood the GPU with synchronous copies.
GuestTexture* TranslateGuestTextureFetch(const void* guestFetch, bool uploadGuestData) {
  if (guestFetch == nullptr || Device() == nullptr)
    return nullptr;
  const uint32_t guestAddress = ToGuest(guestFetch);

  const auto* fetch = reinterpret_cast<const rex::be<uint32_t>*>(guestFetch);
  XenosTextureInfo info = ParseTextureFetchConstant(fetch);
  if (!info.valid) {
    static std::unordered_set<uint32_t> s_warned;
    if (s_warned.insert(guestAddress).second) {
      REXGPU_WARN("TranslateGuestTextureFetch: 0x{:08X} invalid fetch ({}x{} fmt={} base=0x{:08X})",
                  guestAddress, info.width, info.height, int(info.format), info.baseAddress);
    }
    return nullptr;
  }

  GuestTexture* cached = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_guestTextureAliasMutex);
    auto it = g_guestTextureAliases.find(info.baseAddress);
    if (it != g_guestTextureAliases.end() && it->second != nullptr) {
      cached = it->second;
      if (cached->width != info.width || cached->height != info.height ||
          cached->format != info.format) {
        g_guestTextureAliases.erase(it);
        cached = nullptr;
      }
    }
  }
  if (cached != nullptr) {
    const uint64_t frame = CurrentFrameIndex();
    if (uploadGuestData && cached->lastUploadFrame != frame) {
      UploadGuestTextureData(cached, info);
      cached->lastUploadFrame = frame;
    }
    return cached;
  }

  return CreateAndRegisterGuestTexture(info, uploadGuestData);
}

// Translates a raw XG-header guest texture object (see the section comment
// above) into a native GuestTexture. The header's Common field (offset 0)
// must tag it as a texture resource; the fetch constant lives 7 dwords in
// (GPUTEXTURE_FETCH_CONSTANT at header +0x1C).
GuestTexture* TranslateGuestTexture(void* guestHeader, bool uploadGuestData) {
  const uint32_t guestAddress = ToGuest(guestHeader);

  const auto* header = reinterpret_cast<const rex::be<uint32_t>*>(guestHeader);
  if ((header[0].get() & 0xF) != 3) {
    static std::unordered_set<uint32_t> s_warned;
    if (s_warned.insert(guestAddress).second) {
      REXGPU_WARN(
          "TranslateGuestTexture: 0x{:08X} unhandled resource type nibble {} (common=0x{:08X})",
          guestAddress, header[0].get() & 0xF, header[0].get());
    }
    return nullptr;
  }
  XenosTextureInfo info = ParseTextureFetchConstant(header + 7);
  if (!info.valid) {
    static std::unordered_set<uint32_t> s_warned;
    if (s_warned.insert(guestAddress).second) {
      REXGPU_WARN(
          "TranslateGuestTexture: 0x{:08X} invalid fetch constant ({}x{} fmt={} base=0x{:08X})",
          guestAddress, info.width, info.height, int(info.format), info.baseAddress);
    }
    return nullptr;
  }

  {
    std::lock_guard<std::mutex> lock(g_guestTextureAliasMutex);
    auto it = g_guestTextureAliases.find(info.baseAddress);
    if (it != g_guestTextureAliases.end() && it->second != nullptr) {
      GuestTexture* cached = it->second;
      // Same memory redescribed with a different shape: drop the stale entry.
      if (cached->width == info.width && cached->height == info.height &&
          cached->format == info.format) {
        return cached;
      }
      g_guestTextureAliases.erase(it);
    }
  }

  return CreateAndRegisterGuestTexture(info, uploadGuestData);
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
    if (count > 64)
      break;  // safety
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
    if (i < count && d.stream < 16)
      decl->vertexStreams[d.stream] = true;
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
  if (desc == nullptr)
    return;
  desc->format = surface->guestFormat;
  desc->type = 3 /* D3DRTYPE_SURFACE */;
  desc->usage = 0;
  desc->pool = 0;
  desc->multiSampleType =
      static_cast<uint32_t>(surface->sampleCount == RenderSampleCount::COUNT_4   ? 2
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
  if (size < 128 || *reinterpret_cast<const uint32_t*>(data) != FourCC('D', 'D', 'S', ' '))
    return info;

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
  if (texture->texture == nullptr) {
    REXGPU_ERROR("LoadTextureFromMemory: Plume createTexture failed ({}x{})", dds.width,
                 dds.height);
    texture->width = dds.width;
    texture->height = dds.height;
    texture->depth = dds.depth;
    texture->levels = dds.mipCount;
    texture->format = dds.format;
    return texture;
  }

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
  if (dds.headerSize + srcOff > size)
    return texture;  // truncated data; skip upload

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
  for (uint32_t i = 0; i < slices.size(); ++i) {
    const Slice& s = slices[i];
    const uint32_t rowTexels = (s.dstRowPitch / bpb) * blockW;
    RenderCommand cmd{};
    cmd.type = RenderCommandType::CopyTextureFromUpload;
    cmd.copyTextureFromUpload.dst = dstTex;
    cmd.copyTextureFromUpload.src = srcBuf;
    cmd.copyTextureFromUpload.format = uint32_t(fmt);
    cmd.copyTextureFromUpload.width = s.width;
    cmd.copyTextureFromUpload.height = s.height;
    cmd.copyTextureFromUpload.rowTexels = rowTexels;
    cmd.copyTextureFromUpload.mip = i;
    cmd.copyTextureFromUpload.srcOffset = s.dstOffset;
    RenderQueue::Run(cmd);
  }

  return texture;
}

// ---------------------------------------------------------------------------
// Vertex/pixel shaders: map guest microcode to its XenosRecomp-translated
// host shader via the generated shader cache, keyed by microcode hash.
// ---------------------------------------------------------------------------

ShaderCacheEntry* FindShaderCacheEntry(uint64_t hash) {
  ShaderCacheEntry* end = g_shaderCacheEntries + g_shaderCacheEntryCount;
  ShaderCacheEntry* it =
      std::lower_bound(g_shaderCacheEntries, end, hash,
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

void RegisterShaderContainerForUcodeLookup(const uint32_t* function, uint32_t size,
                                           GuestShader* shader, ResourceType type) {
  if (function == nullptr || shader == nullptr || size == 0u || size > 0x40000u)
    return;
  std::lock_guard lock(g_ucodeIndexMutex);
  for (const auto& r : g_ucodeContainers) {
    if (r.host == reinterpret_cast<const uint8_t*>(function))
      return;
  }
  g_ucodeContainers.push_back({reinterpret_cast<const uint8_t*>(function), ToGuest(function), size,
                               shader, type == ResourceType::PixelShader});
}

GuestShader* FindShaderByInlineUcode(const void* ucode, uint32_t bytes, bool pixel) {
  if (ucode == nullptr || bytes < 16u || bytes > 0x20000u)
    return nullptr;
  const uint64_t h = XXH3_64bits(ucode, bytes) ^ (pixel ? 1ull : 0ull);
  std::lock_guard lock(g_ucodeIndexMutex);
  auto it = g_inlineUcodeCache.find(h);
  if (it != g_inlineUcodeCache.end())
    return it->second;
  GuestShader* found = nullptr;
  const uint8_t first = *static_cast<const uint8_t*>(ucode);
  for (const auto& r : g_ucodeContainers) {
    if (r.pixel != pixel || r.size < bytes)
      continue;
    const uint8_t* end = r.host + (r.size - bytes);
    for (const uint8_t* p = r.host; p <= end; ++p) {
      p = static_cast<const uint8_t*>(std::memchr(p, first, size_t(end - p) + 1u));
      if (p == nullptr)
        break;
      if (std::memcmp(p, ucode, bytes) == 0) {
        found = r.shader;
        break;
      }
    }
    if (found != nullptr)
      break;
  }
  g_inlineUcodeCache.emplace(h, found);  // cache negatives too
  return found;
}

GuestShader* FindShaderByUcodeAddress(uint32_t guestAddr, bool pixel) {
  guestAddr &= 0x1FFFFFFFu;
  std::lock_guard lock(g_ucodeIndexMutex);
  for (const auto& r : g_ucodeContainers) {
    const uint32_t base = r.guest & 0x1FFFFFFFu;
    if (r.pixel == pixel && guestAddr >= base && guestAddr < base + r.size)
      return r.shader;
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
        if (idx >= totalDw)
          break;
        const uint32_t v = std::byteswap(function[idx]);
        headerEls.push_back(
            ShaderHeaderElement{uint8_t((v >> 12) & 0xFu), uint8_t((v >> 16) & 0xFu)});
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
    auto* cached = reinterpret_cast<GuestShader*>(entry->guest_shader);
    if (cached->type != type) {
      // The cache stores one compiled DXIL variant per microcode hash, but this
      // hash's bytes are also being reused for the other shader stage. Treat it
      // like a cache miss for this stage instead of handing back the
      // wrongly-typed shader: pipeline.cpp would otherwise link that DXIL
      // (compiled for the other stage) against this stage's profile and fail
      // -- every single draw, since the mismatch is deterministic.
      static std::unordered_set<uint64_t> s_typeMismatchWarned;
      if (s_typeMismatchWarned.insert(hash).second) {
        REXGPU_WARN(
            "Shader cache type mismatch: hash=0x{:016X} already cached as type={}, requested "
            "type={} -- treating as a cache miss for this stage",
            hash, int(cached->type), int(type));
      }
      return finish(GuestNew<GuestShader>(type));
    }
    RegisterShaderContainerForUcodeLookup(function, size, cached, type);
    return finish(cached);
  }

  // Dump the raw ShaderContainer so XenosRecomp can translate it offline
  // (missed_shaders/*.bin is a real, still-used mechanism -- see
  // docs/migration-from-plume.md on the seed shader cache's provenance).
  static std::unordered_set<uint64_t> s_dumped;
  if (s_dumped.insert(hash).second) {
    std::filesystem::create_directories("missed_shaders");
    char path[64];
    std::snprintf(path, sizeof(path), "missed_shaders/%016llX.bin",
                  static_cast<unsigned long long>(hash));
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
  if (!guestAddress || shader == nullptr)
    return;
  std::lock_guard lock(g_shaderAliasMutex);
  g_shaderAliases[guestAddress] = shader;
}

GuestShader* LookupShaderAlias(uint32_t guestAddress) {
  if (!guestAddress)
    return nullptr;
  std::lock_guard lock(g_shaderAliasMutex);
  auto it = g_shaderAliases.find(guestAddress);
  return it != g_shaderAliases.end() ? it->second : nullptr;
}

}  // namespace fm2::render
