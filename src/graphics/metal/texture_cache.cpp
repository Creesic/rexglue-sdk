#include <rex/graphics/metal/texture_cache.h>
#include <rex/graphics/metal/shared_memory.h>
#include <rex/graphics/metal/command_processor.h>
#include <rex/graphics/pipeline/texture/conversion.h>
#include <rex/graphics/pipeline/texture/info.h>
#include <rex/logging/macros.h>
#include <rex/math.h>
#include <rex/memory.h>

#include <algorithm>
#include <cstring>

namespace {
constexpr bool kMetalVerboseDiagnostics = false;

bool AreDimensionsCompatible(
    rex::graphics::xenos::FetchOpDimension fetch_dimension,
    rex::graphics::xenos::DataDimension texture_dimension) {
  switch (fetch_dimension) {
    case rex::graphics::xenos::FetchOpDimension::k1D:
    case rex::graphics::xenos::FetchOpDimension::k2D:
      return texture_dimension == rex::graphics::xenos::DataDimension::k1D ||
             texture_dimension ==
                 rex::graphics::xenos::DataDimension::k2DOrStacked ||
             texture_dimension == rex::graphics::xenos::DataDimension::k3D;
    case rex::graphics::xenos::FetchOpDimension::k3DOrStacked:
      return texture_dimension == rex::graphics::xenos::DataDimension::k3D ||
             texture_dimension ==
                 rex::graphics::xenos::DataDimension::k2DOrStacked;
    case rex::graphics::xenos::FetchOpDimension::kCube:
      return texture_dimension == rex::graphics::xenos::DataDimension::kCube;
    default:
      return false;
  }
}

MTL::TextureSwizzleChannels ToMetalTextureSwizzle(uint32_t xenos_swizzle) {
  static const MTL::TextureSwizzle kMap[] = {
      MTL::TextureSwizzleRed,    MTL::TextureSwizzleGreen,
      MTL::TextureSwizzleBlue,   MTL::TextureSwizzleAlpha,
      MTL::TextureSwizzleZero,   MTL::TextureSwizzleOne,
      MTL::TextureSwizzleZero,   MTL::TextureSwizzleZero,
  };
  MTL::TextureSwizzleChannels swizzle;
  swizzle.red = kMap[(xenos_swizzle >> 0) & 0x7];
  swizzle.green = kMap[(xenos_swizzle >> 3) & 0x7];
  swizzle.blue = kMap[(xenos_swizzle >> 6) & 0x7];
  swizzle.alpha = kMap[(xenos_swizzle >> 9) & 0x7];
  return swizzle;
}
}

namespace rex {
namespace graphics {
namespace metal {

MetalTextureCache::MetalTextureCache(
    const RegisterFile& register_file, MetalSharedMemory& shared_memory,
    uint32_t draw_resolution_scale_x, uint32_t draw_resolution_scale_y,
    MetalCommandProcessor& command_processor)
    : TextureCache(register_file, shared_memory, draw_resolution_scale_x,
                   draw_resolution_scale_y),
      command_processor_(command_processor),
      device_(command_processor.GetMetalDevice()) {
  bound_textures_.fill(nullptr);
  bound_samplers_.fill(nullptr);
}

MetalTextureCache::~MetalTextureCache() {
  ReleaseResolvedTextureAliases();
  for (auto* tex : bound_textures_) {
    if (tex) tex->release();
  }
  for (auto& [key, sampler] : sampler_state_cache_) {
    if (sampler) sampler->release();
  }
}

void MetalTextureCache::ClearCache() {
  TextureCache::ClearCache();
  ReleaseResolvedTextureAliases();
  for (auto* tex : bound_textures_) {
    if (tex) tex->release();
  }
  bound_textures_.fill(nullptr);
  bound_samplers_.fill(nullptr);
  bound_texture_count_ = 0;
}

void MetalTextureCache::ReleaseResolvedTextureAliases() {
  for (ResolvedTextureAlias& alias : resolved_texture_aliases_) {
    if (alias.texture) {
      alias.texture->release();
      alias.texture = nullptr;
    }
  }
  resolved_texture_aliases_.clear();
}

void MetalTextureCache::RegisterResolvedTexture(uint32_t base_address,
                                                MTL::Texture* texture,
                                                uint32_t width,
                                                uint32_t height) {
  if (!texture || !width || !height) {
    return;
  }
  base_address &= 0x1FFFFFFF;

  for (ResolvedTextureAlias& alias : resolved_texture_aliases_) {
    if (alias.base_address == base_address) {
      if (alias.texture != texture) {
        if (alias.texture) {
          alias.texture->release();
        }
        alias.texture = texture;
        alias.texture->retain();
      }
      alias.width = width;
      alias.height = height;
      ResetTextureBindings();
      return;
    }
  }

  if (resolved_texture_aliases_.size() >= 16) {
    ResolvedTextureAlias& oldest = resolved_texture_aliases_.front();
    if (oldest.texture) {
      oldest.texture->release();
    }
    resolved_texture_aliases_.erase(resolved_texture_aliases_.begin());
  }

  ResolvedTextureAlias alias;
  alias.base_address = base_address;
  alias.width = width;
  alias.height = height;
  alias.texture = texture;
  alias.texture->retain();
  resolved_texture_aliases_.push_back(alias);
  ResetTextureBindings();
}

MTL::Texture* MetalTextureCache::GetResolvedTextureAlias(
    const TextureKey& key) const {
  if (!key.is_valid || key.base_page == 0 ||
      key.dimension != xenos::DataDimension::k2DOrStacked) {
    return nullptr;
  }

  uint32_t base_address = (key.base_page << 12) & 0x1FFFFFFF;
  for (const ResolvedTextureAlias& alias : resolved_texture_aliases_) {
    if (alias.base_address != base_address || !alias.texture) {
      continue;
    }
    if (key.GetWidth() <= alias.width && key.GetHeight() <= alias.height) {
      return alias.texture;
    }
  }
  return nullptr;
}

void MetalTextureCache::MetalTexture::ReleaseViews() {
  for (auto& [_, view] : swizzled_view_cache_) {
    if (view) {
      view->release();
    }
  }
  swizzled_view_cache_.clear();
}

MTL::Texture* MetalTextureCache::MetalTexture::GetOrCreateView(
    uint32_t host_swizzle, xenos::FetchOpDimension dimension, bool is_signed) {
  if (!metal_texture_) {
    return nullptr;
  }

  MTL::TextureType view_type = metal_texture_->textureType();
  switch (dimension) {
    case xenos::FetchOpDimension::kCube:
      view_type = MTL::TextureTypeCube;
      break;
    case xenos::FetchOpDimension::k3DOrStacked:
      view_type = key().dimension == xenos::DataDimension::k3D
                      ? MTL::TextureType3D
                      : MTL::TextureType2DArray;
      break;
    case xenos::FetchOpDimension::k1D:
    case xenos::FetchOpDimension::k2D:
    default:
      view_type = key().dimension == xenos::DataDimension::k3D
                      ? MTL::TextureType3D
                      : MTL::TextureType2DArray;
      break;
  }

  auto get_signed_view_format = [&](MTL::PixelFormat base_format) {
    if (!is_signed) {
      return base_format;
    }
    switch (key().format) {
      case xenos::TextureFormat::k_8:
      case xenos::TextureFormat::k_8_A:
      case xenos::TextureFormat::k_8_B:
        return MTL::PixelFormatR8Snorm;
      case xenos::TextureFormat::k_8_8:
        return MTL::PixelFormatRG8Snorm;
      case xenos::TextureFormat::k_8_8_8_8:
      case xenos::TextureFormat::k_8_8_8_8_A:
        return MTL::PixelFormatRGBA8Snorm;
      case xenos::TextureFormat::k_16:
        return MTL::PixelFormatR16Snorm;
      case xenos::TextureFormat::k_16_16:
        return MTL::PixelFormatRG16Snorm;
      case xenos::TextureFormat::k_16_16_16_16:
        return MTL::PixelFormatRGBA16Snorm;
      default:
        return base_format;
    }
  };

  MTL::PixelFormat view_format =
      get_signed_view_format(metal_texture_->pixelFormat());
  if (host_swizzle == xenos::XE_GPU_TEXTURE_SWIZZLE_RGBA &&
      view_format == metal_texture_->pixelFormat() &&
      view_type == metal_texture_->textureType()) {
    return metal_texture_;
  }

  uint64_t view_key = uint64_t(host_swizzle) | (uint64_t(dimension) << 32) |
                      (uint64_t(is_signed) << 40) |
                      (uint64_t(view_format) << 48);
  auto found = swizzled_view_cache_.find(view_key);
  if (found != swizzled_view_cache_.end()) {
    return found->second;
  }

  NS::Range level_range =
      NS::Range::Make(0, metal_texture_->mipmapLevelCount());
  uint32_t slice_count = 1;
  switch (view_type) {
    case MTL::TextureType2DArray:
      slice_count = metal_texture_->arrayLength();
      break;
    case MTL::TextureTypeCube:
      slice_count = 6;
      break;
    case MTL::TextureTypeCubeArray:
      slice_count = metal_texture_->arrayLength() * 6;
      break;
    default:
      slice_count = 1;
      break;
  }
  NS::Range slice_range = NS::Range::Make(0, slice_count);

  MTL::Texture* view = metal_texture_->newTextureView(
      view_format, view_type, level_range, slice_range,
      ToMetalTextureSwizzle(host_swizzle));
  if (!view) {
    return view_type == metal_texture_->textureType() ? metal_texture_ : nullptr;
  }

  swizzled_view_cache_.emplace(view_key, view);
  return view;
}

uint32_t MetalTextureCache::GetHostFormatSwizzle(TextureKey key) const {
  switch (key.format) {
    case xenos::TextureFormat::k_8:
    case xenos::TextureFormat::k_8_A:
    case xenos::TextureFormat::k_8_B:
    case xenos::TextureFormat::k_DXT3A:
    case xenos::TextureFormat::k_DXT5A:
    case xenos::TextureFormat::k_16:
    case xenos::TextureFormat::k_16_EXPAND:
    case xenos::TextureFormat::k_16_FLOAT:
    case xenos::TextureFormat::k_24_8:
    case xenos::TextureFormat::k_24_8_FLOAT:
    case xenos::TextureFormat::k_32_FLOAT:
      return xenos::XE_GPU_TEXTURE_SWIZZLE_RRRR;
    case xenos::TextureFormat::k_8_8:
    case xenos::TextureFormat::k_16_16:
    case xenos::TextureFormat::k_16_16_EXPAND:
    case xenos::TextureFormat::k_16_16_FLOAT:
    case xenos::TextureFormat::k_DXN:
    case xenos::TextureFormat::k_32_32_FLOAT:
      return xenos::XE_GPU_TEXTURE_SWIZZLE_RGGG;
    case xenos::TextureFormat::k_5_6_5:
    case xenos::TextureFormat::k_6_5_5:
    case xenos::TextureFormat::k_10_11_11:
    case xenos::TextureFormat::k_11_11_10:
    case xenos::TextureFormat::k_Cr_Y1_Cb_Y0_REP:
    case xenos::TextureFormat::k_Y1_Cr_Y0_Cb_REP:
      return xenos::XE_GPU_TEXTURE_SWIZZLE_RGBB;
    default:
      return xenos::XE_GPU_TEXTURE_SWIZZLE_RGBA;
  }
}

uint32_t MetalTextureCache::GetMaxHostTextureWidthHeight(
    xenos::DataDimension dimension) const {
  return 8192;
}

uint32_t MetalTextureCache::GetMaxHostTextureDepthOrArraySize(
    xenos::DataDimension dimension) const {
  return 2048;
}

std::unique_ptr<TextureCache::Texture> MetalTextureCache::CreateTexture(
    TextureKey key) {
  MTL::PixelFormat pixel_format = GetMetalPixelFormat(key.format, false);

  MTL::TextureDescriptor* desc = MTL::TextureDescriptor::alloc()->init();

  bool is_3d = key.dimension == xenos::DataDimension::k3D;
  uint32_t width = key.GetWidth();
  uint32_t height = key.GetHeight();
  uint32_t depth_or_array = key.GetDepthOrArraySize();

  if (is_3d) {
    desc->setTextureType(MTL::TextureType3D);
    desc->setDepth(depth_or_array);
  } else if (key.dimension == xenos::DataDimension::kCube) {
    desc->setTextureType(MTL::TextureTypeCube);
    desc->setArrayLength(depth_or_array / 6);
  } else {
    desc->setTextureType(MTL::TextureType2DArray);
    desc->setArrayLength(std::max(depth_or_array, 1u));
  }

  desc->setWidth(width);
  desc->setHeight(height);
  desc->setPixelFormat(pixel_format);

  uint32_t mip_levels = key.mip_max_level + 1;
  if (mip_levels > 1) {
    uint32_t max_mips = 0;
    uint32_t w = width, h = height;
    while (w > 0 || h > 0) { ++max_mips; w >>= 1; h >>= 1; }
    mip_levels = std::min(mip_levels, max_mips);
    if (mip_levels < 1) mip_levels = 1;
  }
  desc->setMipmapLevelCount(mip_levels);

  desc->setUsage(MTL::TextureUsageShaderRead |
                 MTL::TextureUsagePixelFormatView);
  desc->setSwizzle(ToMetalTextureSwizzle(xenos::XE_GPU_TEXTURE_SWIZZLE_RGBA));
  desc->setResourceOptions(MTL::ResourceStorageModeShared);

  MTL::Texture* metal_texture = device_->newTexture(desc);
  desc->release();

  if (!metal_texture) {
    REXLOG_ERROR("MetalTextureCache: Failed to create texture {}x{} fmt={}",
                 width, height, uint32_t(key.format));
  } else {
    if constexpr (kMetalVerboseDiagnostics) {
    static int create_count = 0;
    if (create_count++ < 10) {
      fprintf(stderr, "[metal] CreateTexture: %ux%u fmt=%d tiled=%d mips=%u mtltex=%p\n",
              width, height, (int)key.format, (int)key.tiled, mip_levels, metal_texture);
      fflush(stderr);
    }
    }
  }

  std::unique_ptr<TextureCache::Texture> tex(
      new MetalTexture(*this, key, metal_texture));
  return tex;
}

static void CopyAndSwapEndian(xenos::Endian endian, void* dst, const void* src,
                              size_t length) {
  switch (endian) {
    case xenos::Endian::k8in16:
      memory::copy_and_swap_16_unaligned(dst, src, length / 2);
      break;
    case xenos::Endian::k8in32:
      memory::copy_and_swap_32_unaligned(dst, src, length / 4);
      break;
    case xenos::Endian::k16in32:
      memory::copy_and_swap_16_in_32_unaligned(dst, src, length);
      break;
    default:
      std::memcpy(dst, src, length);
      break;
  }
}

bool MetalTextureCache::LoadTextureDataFromResidentMemoryImpl(
    Texture& texture, bool load_base, bool load_mips) {
  MetalTexture& metal_texture = static_cast<MetalTexture&>(texture);
  MTL::Texture* mtl_tex = metal_texture.metal_texture();
  if (!mtl_tex) return true;

  TextureKey key = texture.key();
  const texture_util::TextureGuestLayout& guest_layout = texture.guest_layout();

  MetalSharedMemory& metal_shared_memory =
      static_cast<MetalSharedMemory&>(shared_memory());
  MTL::Buffer* shared_mem_buffer = metal_shared_memory.GetBuffer();
  if (!shared_mem_buffer) return false;

  const uint8_t* guest_base = static_cast<const uint8_t*>(
      shared_mem_buffer->contents());
  if (!guest_base) return false;

  const FormatInfo* format_info = FormatInfo::Get(key.format);
  if (!format_info) return false;

  uint32_t bytes_per_block = format_info->bytes_per_block();
  uint32_t block_width = format_info->block_width;
  uint32_t block_height = format_info->block_height;
  bool is_compressed = (block_width > 1 || block_height > 1);

  uint32_t width = key.GetWidth();
  uint32_t height = key.GetHeight();
  uint32_t depth_or_array = key.GetDepthOrArraySize();
  bool is_3d = key.dimension == xenos::DataDimension::k3D;
  bool is_tiled = key.tiled;

  if constexpr (kMetalVerboseDiagnostics) {
  static int load_count = 0;
  if (load_count++ < 10) {
    fprintf(stderr, "[metal] LoadTexture: %ux%u fmt=%d tiled=%d base=%d mips=%d base_addr=0x%X\n",
            width, height, (int)key.format, (int)is_tiled, (int)load_base, (int)load_mips,
            key.base_page << 12);
    fflush(stderr);
  }
  }

  uint32_t base_address = key.base_page << 12;

  if (load_base && guest_layout.base.level_data_extent_bytes > 0) {
    const auto& level = guest_layout.base;
    uint32_t level_width = width;
    uint32_t level_height = height;

    const uint8_t* src_data = guest_base + base_address;

    if (!is_tiled) {
      uint32_t row_pitch = level.row_pitch_bytes;
      if (row_pitch == 0) row_pitch = width * bytes_per_block / block_width;

      if (is_compressed) {
        MTL::Region region;
        region.origin = MTL::Origin(0, 0, 0);
        region.size = MTL::Size(level_width / block_width,
                                level_height / block_height,
                                is_3d ? depth_or_array : 1);
        if (is_3d) {
          mtl_tex->replaceRegion(region, 0, 0, src_data, row_pitch,
                                 level.z_slice_stride_block_rows * row_pitch);
        } else {
          mtl_tex->replaceRegion(region, 0, src_data, row_pitch);
        }
      } else {
        MTL::Region region;
        region.origin = MTL::Origin(0, 0, 0);
        region.size = MTL::Size(level_width, level_height,
                                is_3d ? depth_or_array : 1);
        if (is_3d) {
          mtl_tex->replaceRegion(region, 0, 0, src_data, row_pitch,
                                 level.z_slice_stride_block_rows * row_pitch);
        } else {
          mtl_tex->replaceRegion(region, 0, src_data, row_pitch);
        }
      }
    } else {
      uint32_t out_pitch = level_width * bytes_per_block / block_width;
      uint32_t out_row_pitch = std::max(out_pitch, uint32_t(256));
      uint32_t out_size = out_row_pitch * (level_height / block_height);

      std::vector<uint8_t> untiled(out_size);
      if (untiled.empty()) return true;

      texture_conversion::UntileInfo untile_info;
      untile_info.offset_x = 0;
      untile_info.offset_y = 0;
      untile_info.width = level_width / block_width;
      untile_info.height = level_height / block_height;
      untile_info.input_pitch = level.row_pitch_bytes / bytes_per_block;
      untile_info.output_pitch = level_width / block_width;
      untile_info.input_format_info = format_info;
      untile_info.output_format_info = format_info;
      untile_info.copy_callback = [endianness = key.endianness](
          void* out, const void* in, size_t len) {
        CopyAndSwapEndian(endianness, out, in, len);
      };

      texture_conversion::Untile(untiled.data(), src_data, &untile_info);

      MTL::Region region;
      region.origin = MTL::Origin(0, 0, 0);
      if (is_compressed) {
        region.size = MTL::Size(level_width / block_width,
                                level_height / block_height, 1);
        mtl_tex->replaceRegion(region, 0, untiled.data(), out_row_pitch);
      } else {
        region.size = MTL::Size(level_width, level_height, 1);
        mtl_tex->replaceRegion(region, 0, untiled.data(), out_row_pitch);
      }
    }
  }

  if (load_mips && key.mip_max_level > 0) {
    uint32_t mip_address = key.mip_page << 12;

    for (uint32_t level = 1; level <= key.mip_max_level; ++level) {
      uint32_t guest_level_idx = level - 1;
      if (guest_level_idx >= xenos::kTextureMaxMips) break;

      const auto& level_layout = guest_layout.mips[guest_level_idx];
      if (level_layout.level_data_extent_bytes == 0) continue;

      uint32_t level_width = std::max(width >> level, uint32_t(1));
      uint32_t level_height = std::max(height >> level, uint32_t(1));

      uint32_t offset = 0;
      if (guest_level_idx > 0) {
        offset = guest_layout.mip_offsets_bytes[guest_level_idx] -
                 guest_layout.mip_offsets_bytes[0];
      }

      const uint8_t* src_data = guest_base + mip_address + offset;

      uint32_t row_pitch = level_layout.row_pitch_bytes;
      if (row_pitch == 0) row_pitch = level_width * bytes_per_block / block_width;

      if (is_compressed) {
        MTL::Region region;
        region.origin = MTL::Origin(0, 0, 0);
        region.size = MTL::Size(level_width / block_width,
                                level_height / block_height, 1);
        mtl_tex->replaceRegion(region, level, src_data, row_pitch);
      } else {
        MTL::Region region;
        region.origin = MTL::Origin(0, 0, 0);
        region.size = MTL::Size(level_width, level_height, 1);
        mtl_tex->replaceRegion(region, level, src_data, row_pitch);
      }
    }
  }

  return true;
}

void MetalTextureCache::RequestTextures(uint32_t used_texture_mask) {
  if constexpr (kMetalVerboseDiagnostics) {
  static int req_count = 0;
  if (req_count < 5) {
    fprintf(stderr, "[metal] RequestTextures: mask=0x%08X\n", used_texture_mask);
    fflush(stderr);
    req_count++;
  }
  }

  TextureCache::RequestTextures(used_texture_mask);

  if constexpr (kMetalVerboseDiagnostics) {
  static int post_count = 0;
  if (post_count < 5) {
    for (uint32_t i = 0; i < 32; ++i) {
      if (bound_textures_[i]) {
        fprintf(stderr, "[metal]   bound_tex[%u] = %p (%ux%u)\n", i,
                bound_textures_[i],
                bound_textures_[i]->width(), bound_textures_[i]->height());
        fflush(stderr);
      }
    }
    post_count++;
  }
  }

  const auto& regs = register_file();
  uint32_t remaining = used_texture_mask;
  uint32_t idx = 0;
  while (rex::bit_scan_forward(remaining, &idx)) {
    uint32_t bit = UINT32_C(1) << idx;
    remaining &= ~bit;

    xenos::xe_gpu_texture_fetch_t fetch = regs.GetTextureFetch(idx);
    if (fetch.type != xenos::FetchConstantType::kTexture) {
      continue;
    }
    MTL::SamplerState* sampler = GetOrCreateSamplerState(fetch);
    if (sampler) {
      bound_samplers_[idx] = sampler;
    }
  }

  bound_texture_count_ = used_texture_mask ? (32 - __builtin_clz(used_texture_mask)) : 0;
}

bool MetalTextureCache::AnyUsedTextureRequestWorkPending(
    uint32_t used_texture_mask) const {
  return false;
}

MTL::Texture* MetalTextureCache::GetBoundTexture(uint32_t index) const {
  if (index < kMaxBoundTextures) return bound_textures_[index];
  return nullptr;
}

MTL::Texture* MetalTextureCache::GetBoundTexture(
    uint32_t fetch_constant, xenos::FetchOpDimension dimension,
    bool signed_version) const {
  const TextureBinding* binding = GetValidTextureBinding(fetch_constant);
  if (!binding) return nullptr;
  if (!AreDimensionsCompatible(dimension, binding->key.dimension)) {
    return nullptr;
  }
  if (MTL::Texture* resolved_alias = GetResolvedTextureAlias(binding->key)) {
    return resolved_alias;
  }
  Texture* texture = signed_version && binding->texture_signed
                         ? binding->texture_signed
                         : binding->texture;
  if (!texture) return nullptr;
  return const_cast<MetalTexture*>(static_cast<const MetalTexture*>(texture))
      ->GetOrCreateView(binding->host_swizzle, dimension, signed_version);
}

MTL::SamplerState* MetalTextureCache::GetBoundSampler(uint32_t index) const {
  if (index < kMaxBoundTextures) return bound_samplers_[index];
  return nullptr;
}

MTL::Texture* MetalTextureCache::RequestSwapTexture(
    uint32_t& width_scaled_out, uint32_t& height_scaled_out,
    xenos::TextureFormat& format_out, uint32_t* width_unscaled_out,
    uint32_t* height_unscaled_out) {
  const auto& regs = register_file();
  xenos::xe_gpu_texture_fetch_t fetch = regs.GetTextureFetch(0);

  TextureKey key;
  BindingInfoFromFetchConstant(fetch, key, nullptr);
  if (!key.is_valid || key.base_page == 0 ||
      key.dimension != xenos::DataDimension::k2DOrStacked) {
    return nullptr;
  }

  MetalTexture* texture = static_cast<MetalTexture*>(FindOrCreateTexture(key));
  if (!texture || !LoadTextureData(*texture)) {
    return nullptr;
  }

  texture->MarkAsUsed();
  key = texture->key();
  if (width_unscaled_out) {
    *width_unscaled_out = key.GetWidth();
  }
  if (height_unscaled_out) {
    *height_unscaled_out = key.GetHeight();
  }
  width_scaled_out = key.GetWidth() * (key.scaled_resolve ? draw_resolution_scale_x() : 1);
  height_scaled_out = key.GetHeight() * (key.scaled_resolve ? draw_resolution_scale_y() : 1);
  format_out = key.format;
  return texture->metal_texture();
}

MTL::PixelFormat MetalTextureCache::GetMetalPixelFormat(
    xenos::TextureFormat format, bool is_signed) const {
  switch (format) {
    case xenos::TextureFormat::k_1_5_5_5:
      return MTL::PixelFormatBGR5A1Unorm;
    case xenos::TextureFormat::k_2_10_10_10:
      return MTL::PixelFormatRGB10A2Unorm;
    case xenos::TextureFormat::k_4_4_4_4:
      return MTL::PixelFormatABGR4Unorm;
    case xenos::TextureFormat::k_5_6_5:
      return MTL::PixelFormatB5G6R5Unorm;
    case xenos::TextureFormat::k_8:
      return is_signed ? MTL::PixelFormatR8Snorm : MTL::PixelFormatR8Unorm;
    case xenos::TextureFormat::k_8_8:
      return is_signed ? MTL::PixelFormatRG8Snorm : MTL::PixelFormatRG8Unorm;
    case xenos::TextureFormat::k_8_8_8_8:
      return is_signed ? MTL::PixelFormatRGBA8Snorm : MTL::PixelFormatRGBA8Unorm;
    case xenos::TextureFormat::k_16:
      return is_signed ? MTL::PixelFormatR16Snorm : MTL::PixelFormatR16Unorm;
    case xenos::TextureFormat::k_16_16:
      return is_signed ? MTL::PixelFormatRG16Snorm : MTL::PixelFormatRG16Unorm;
    case xenos::TextureFormat::k_16_16_16_16:
      return is_signed ? MTL::PixelFormatRGBA16Snorm : MTL::PixelFormatRGBA16Unorm;
    case xenos::TextureFormat::k_16_FLOAT:
      return MTL::PixelFormatR16Float;
    case xenos::TextureFormat::k_16_16_FLOAT:
      return MTL::PixelFormatRG16Float;
    case xenos::TextureFormat::k_16_16_16_16_FLOAT:
      return MTL::PixelFormatRGBA16Float;
    case xenos::TextureFormat::k_32_FLOAT:
      return MTL::PixelFormatR32Float;
    case xenos::TextureFormat::k_32_32_FLOAT:
      return MTL::PixelFormatRG32Float;
    case xenos::TextureFormat::k_32_32_32_32_FLOAT:
      return MTL::PixelFormatRGBA32Float;
    case xenos::TextureFormat::k_DXT1:
      return MTL::PixelFormatBC1_RGBA;
    case xenos::TextureFormat::k_DXT2_3:
      return MTL::PixelFormatBC2_RGBA;
    case xenos::TextureFormat::k_DXT4_5:
      return MTL::PixelFormatBC3_RGBA;
    default:
      return MTL::PixelFormatRGBA8Unorm;
  }
}

static MTL::SamplerAddressMode ClampModeToMetal(xenos::ClampMode mode) {
  switch (mode) {
    case xenos::ClampMode::kRepeat:
      return MTL::SamplerAddressModeRepeat;
    case xenos::ClampMode::kMirroredRepeat:
      return MTL::SamplerAddressModeMirrorRepeat;
    case xenos::ClampMode::kClampToEdge:
      return MTL::SamplerAddressModeClampToEdge;
    case xenos::ClampMode::kMirrorClampToEdge:
      return MTL::SamplerAddressModeMirrorClampToEdge;
    case xenos::ClampMode::kClampToHalfway:
    case xenos::ClampMode::kMirrorClampToHalfway:
      return MTL::SamplerAddressModeClampToEdge;
    case xenos::ClampMode::kClampToBorder:
    case xenos::ClampMode::kMirrorClampToBorder:
      return MTL::SamplerAddressModeClampToZero;
    default:
      return MTL::SamplerAddressModeClampToEdge;
  }
}

MTL::SamplerState* MetalTextureCache::GetOrCreateSamplerState(
    const DxbcShader::SamplerBinding& binding) {
  xenos::xe_gpu_texture_fetch_t fetch =
      register_file().GetTextureFetch(binding.fetch_constant);
  if (binding.mag_filter != xenos::TextureFilter::kUseFetchConst) {
    fetch.mag_filter = binding.mag_filter;
  }
  if (binding.min_filter != xenos::TextureFilter::kUseFetchConst) {
    fetch.min_filter = binding.min_filter;
  }
  if (binding.mip_filter != xenos::TextureFilter::kUseFetchConst) {
    fetch.mip_filter = binding.mip_filter;
  }
  if (binding.aniso_filter != xenos::AnisoFilter::kUseFetchConst) {
    fetch.aniso_filter = binding.aniso_filter;
  }
  return GetOrCreateSamplerState(fetch);
}

MTL::SamplerState* MetalTextureCache::GetOrCreateSamplerState(
    const xenos::xe_gpu_texture_fetch_t& fetch) {
  uint64_t key = (uint64_t(fetch.clamp_x) | (uint64_t(fetch.clamp_y) << 3) |
                  (uint64_t(fetch.clamp_z) << 6) |
                  (uint64_t(fetch.mag_filter) << 9) |
                  (uint64_t(fetch.min_filter) << 11) |
                  (uint64_t(fetch.mip_filter) << 13) |
                  (uint64_t(fetch.aniso_filter) << 15) |
                  (uint64_t(fetch.border_color) << 18));

  auto it = sampler_state_cache_.find(key);
  if (it != sampler_state_cache_.end() && it->second) {
    return it->second;
  }

  MTL::SamplerDescriptor* desc = MTL::SamplerDescriptor::alloc()->init();

  desc->setSAddressMode(ClampModeToMetal(fetch.clamp_x));
  desc->setTAddressMode(ClampModeToMetal(fetch.clamp_y));
  desc->setRAddressMode(ClampModeToMetal(fetch.clamp_z));

  bool aniso = fetch.aniso_filter != xenos::AnisoFilter::kDisabled &&
               fetch.aniso_filter != xenos::AnisoFilter::kUseFetchConst;
  if (aniso) {
    uint32_t max_aniso = 1;
    switch (fetch.aniso_filter) {
      case xenos::AnisoFilter::kMax_2_1: max_aniso = 2; break;
      case xenos::AnisoFilter::kMax_4_1: max_aniso = 4; break;
      case xenos::AnisoFilter::kMax_8_1: max_aniso = 8; break;
      case xenos::AnisoFilter::kMax_16_1: max_aniso = 16; break;
      default: break;
    }
    desc->setMaxAnisotropy(max_aniso);
  }

  bool mag_linear = fetch.mag_filter == xenos::TextureFilter::kLinear ||
                    fetch.mag_filter == xenos::TextureFilter::kUseFetchConst;
  bool min_linear = fetch.min_filter == xenos::TextureFilter::kLinear ||
                    fetch.min_filter == xenos::TextureFilter::kUseFetchConst;
  bool mip_linear = fetch.mip_filter == xenos::TextureFilter::kLinear ||
                    fetch.mip_filter == xenos::TextureFilter::kUseFetchConst;

  if (mag_linear) {
    desc->setMagFilter(MTL::SamplerMinMagFilterLinear);
  } else {
    desc->setMagFilter(MTL::SamplerMinMagFilterNearest);
  }
  if (min_linear) {
    desc->setMinFilter(MTL::SamplerMinMagFilterLinear);
  } else {
    desc->setMinFilter(MTL::SamplerMinMagFilterNearest);
  }

  if (mip_linear) {
    desc->setMipFilter(MTL::SamplerMipFilterLinear);
  } else if (fetch.mip_filter == xenos::TextureFilter::kBaseMap) {
    desc->setMipFilter(MTL::SamplerMipFilterNotMipmapped);
  } else {
    desc->setMipFilter(MTL::SamplerMipFilterNearest);
  }

  MTL::SamplerState* sampler = device_->newSamplerState(desc);
  desc->release();

  if (sampler) {
    sampler_state_cache_[key] = sampler;
  }

  return sampler;
}

void MetalTextureCache::UpdateTextureBindingsImpl(
    uint32_t fetch_constant_mask) {
  uint32_t remaining = fetch_constant_mask;
  uint32_t idx = 0;
  while (rex::bit_scan_forward(remaining, &idx)) {
    uint32_t bit = UINT32_C(1) << idx;
    remaining &= ~bit;

    if (bound_textures_[idx]) {
      bound_textures_[idx]->release();
      bound_textures_[idx] = nullptr;
    }

    const TextureBinding* binding = GetValidTextureBinding(idx);
    if (!binding || !binding->texture) continue;

    MetalTexture* metal_tex =
        static_cast<MetalTexture*>(binding->texture);
    MTL::Texture* mtl_texture = metal_tex->metal_texture();
    if (mtl_texture) {
      mtl_texture->retain();
      bound_textures_[idx] = mtl_texture;
    }
  }
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
