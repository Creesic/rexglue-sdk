#include <rex/graphics/metal/texture_cache.h>
#include <rex/graphics/metal/shared_memory.h>
#include <rex/graphics/metal/command_processor.h>
#include <rex/logging/macros.h>

#include <cstring>

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
  for (auto* tex : bound_textures_) {
    if (tex) tex->release();
  }
  for (auto& [key, sampler] : sampler_state_cache_) {
    if (sampler) sampler->release();
  }
}

void MetalTextureCache::ClearCache() {
  TextureCache::ClearCache();
  for (auto* tex : bound_textures_) {
    if (tex) tex->release();
  }
  bound_textures_.fill(nullptr);
  bound_samplers_.fill(nullptr);
  bound_texture_count_ = 0;
}

uint32_t MetalTextureCache::GetHostFormatSwizzle(TextureKey key) const {
  return 0;
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
  std::unique_ptr<TextureCache::Texture> tex(new MetalTexture(*this, key, nullptr));
  return tex;
}

bool MetalTextureCache::LoadTextureDataFromResidentMemoryImpl(
    Texture& texture, bool load_base, bool load_mips) {
  return true;
}

void MetalTextureCache::RequestTextures(uint32_t used_texture_mask) {
}

bool MetalTextureCache::AnyUsedTextureRequestWorkPending(
    uint32_t used_texture_mask) const {
  return false;
}

MTL::Texture* MetalTextureCache::GetBoundTexture(uint32_t index) const {
  if (index < kMaxBoundTextures) return bound_textures_[index];
  return nullptr;
}

MTL::SamplerState* MetalTextureCache::GetBoundSampler(uint32_t index) const {
  if (index < kMaxBoundTextures) return bound_samplers_[index];
  return nullptr;
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

MTL::SamplerState* MetalTextureCache::GetOrCreateSamplerState(
    const xenos::xe_gpu_texture_fetch_t& fetch) {
  return nullptr;
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
