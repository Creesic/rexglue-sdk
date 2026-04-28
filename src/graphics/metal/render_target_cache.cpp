#include <rex/graphics/metal/render_target_cache.h>
#include <rex/graphics/metal/command_processor.h>
#include <rex/graphics/metal/heap_pool.h>
#include <rex/logging/macros.h>

namespace rex {
namespace graphics {
namespace metal {

MetalRenderTargetCache::MetalRenderTargetCache(
    const RegisterFile& register_file, const memory::Memory& memory,
    TraceWriter& trace_writer, uint32_t draw_resolution_scale_x,
    uint32_t draw_resolution_scale_y,
    MetalCommandProcessor& command_processor)
    : RenderTargetCache(register_file, memory, &trace_writer,
                        draw_resolution_scale_x, draw_resolution_scale_y),
      command_processor_(command_processor) {}

MetalRenderTargetCache::~MetalRenderTargetCache() { Shutdown(true); }

bool MetalRenderTargetCache::Initialize() {
  device_ = command_processor_.GetMetalDevice();
  if (!device_) {
    REXLOG_ERROR("MetalRenderTargetCache: No device");
    return false;
  }

  edram_buffer_ = device_->newBuffer(kEdramSizeBytes, MTL::ResourceStorageModePrivate);
  if (!edram_buffer_) {
    REXLOG_ERROR("MetalRenderTargetCache: Failed to create EDRAM buffer");
    return false;
  }
  edram_buffer_->setLabel(
      NS::String::string("ReX EDRAM", NS::UTF8StringEncoding));

  heap_pool_ = std::make_unique<HeapPool>(device_, MTL::StorageModePrivate,
                                           256 * 1024 * 1024, "ReX_RT");

  REXLOG_INFO("MetalRenderTargetCache: Initialized");
  return true;
}

void MetalRenderTargetCache::Shutdown(bool from_destructor) {
  if (edram_buffer_) {
    edram_buffer_->release();
    edram_buffer_ = nullptr;
  }
  heap_pool_.reset();
  device_ = nullptr;
  if (!from_destructor) {
    ShutdownCommon();
  }
}

void MetalRenderTargetCache::ClearCache() {
  RenderTargetCache::ClearCache();
}

void MetalRenderTargetCache::BeginFrame() {
  RenderTargetCache::BeginFrame();
}

RenderTargetCache::RenderTarget* MetalRenderTargetCache::CreateRenderTarget(
    RenderTargetKey key) {
  auto* rt = new MetalRenderTarget(key);

  MTL::PixelFormat format = MTL::PixelFormatInvalid;
  if (key.is_depth) {
    format = GetMetalDepthFormat(key.GetDepthFormat());
  } else {
    format = GetMetalColorFormat(key.GetColorFormat());
  }

  constexpr uint32_t kEdramTileSizeBytes = 80;
  uint32_t pitch = key.pitch_tiles_at_32bpp * kEdramTileSizeBytes;
  uint32_t w = pitch / 4;
  uint32_t h = GetRenderTargetHeight(key.pitch_tiles_at_32bpp, key.msaa_samples);
  if (w == 0) w = 256;
  if (h == 0) h = 256;
  uint32_t scale_x = draw_resolution_scale_x();
  uint32_t scale_y = draw_resolution_scale_y();
  uint32_t scaled_w = w * scale_x;
  uint32_t scaled_h = h * scale_y;

  MTL::Texture* tex = CreateRenderTargetTexture(scaled_w, scaled_h, format);
  rt->SetTexture(tex);
  if (tex) tex->release();

  rt->SetNeedsInitialClear(true);
  return rt;
}

MTL::Texture* MetalRenderTargetCache::CreateRenderTargetTexture(
    uint32_t width, uint32_t height, MTL::PixelFormat format,
    uint32_t sample_count) {
  if (format == MTL::PixelFormatInvalid) return nullptr;

  MTL::TextureDescriptor* desc = MTL::TextureDescriptor::texture2DDescriptor(
      format, width, height,
      MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
  if (sample_count > 1) {
    desc->setTextureType(MTL::TextureType2DMultisample);
    desc->setSampleCount(sample_count);
  }
  desc->setStorageMode(MTL::StorageModePrivate);

  MTL::Texture* tex = device_->newTexture(desc);
  desc->release();
  return tex;
}

MTL::PixelFormat MetalRenderTargetCache::GetMetalColorFormat(
    xenos::ColorRenderTargetFormat format) const {
  switch (format) {
    case xenos::ColorRenderTargetFormat::k_8_8_8_8:
      return MTL::PixelFormatBGRA8Unorm;
    case xenos::ColorRenderTargetFormat::k_8_8_8_8_GAMMA:
      return MTL::PixelFormatBGRA8Unorm_sRGB;
    case xenos::ColorRenderTargetFormat::k_2_10_10_10_FLOAT:
      return MTL::PixelFormatRGB10A2Unorm;
    case xenos::ColorRenderTargetFormat::k_16_16:
      return MTL::PixelFormatRG16Snorm;
    case xenos::ColorRenderTargetFormat::k_16_16_16_16:
      return MTL::PixelFormatRGBA16Snorm;
    case xenos::ColorRenderTargetFormat::k_16_16_FLOAT:
      return MTL::PixelFormatRG16Float;
    case xenos::ColorRenderTargetFormat::k_16_16_16_16_FLOAT:
      return MTL::PixelFormatRGBA16Float;
    case xenos::ColorRenderTargetFormat::k_32_FLOAT:
      return MTL::PixelFormatR32Float;
    case xenos::ColorRenderTargetFormat::k_32_32_FLOAT:
      return MTL::PixelFormatRG32Float;
    default:
      return MTL::PixelFormatBGRA8Unorm;
  }
}

MTL::PixelFormat MetalRenderTargetCache::GetMetalDepthFormat(
    xenos::DepthRenderTargetFormat format) const {
  switch (format) {
    case xenos::DepthRenderTargetFormat::kD24S8:
      return MTL::PixelFormatDepth32Float_Stencil8;
    case xenos::DepthRenderTargetFormat::kD24FS8:
      return MTL::PixelFormatDepth32Float_Stencil8;
    default:
      return MTL::PixelFormatDepth32Float;
  }
}

MTL::PixelFormat MetalRenderTargetCache::GetColorFormat(uint32_t index) const {
  if (index < 4) return current_color_formats_[index];
  return MTL::PixelFormatInvalid;
}

MTL::PixelFormat MetalRenderTargetCache::GetDepthFormat() const {
  return current_depth_format_;
}

MTL::PixelFormat MetalRenderTargetCache::GetStencilFormat() const {
  return current_stencil_format_;
}

MetalRenderTargetCache::MetalRenderTarget*
MetalRenderTargetCache::GetOrCreateRenderTarget(const RegisterFile& regs) {
  return nullptr;
}

MTL::RenderPassDescriptor*
MetalRenderTargetCache::GetRenderPassDescriptor(uint32_t sample_count) {
  return nullptr;
}

MTL::Texture* MetalRenderTargetCache::GetColorTarget(uint32_t index) const {
  return nullptr;
}

MTL::Texture* MetalRenderTargetCache::GetDepthTarget() const {
  return nullptr;
}

MetalRenderTargetCache::MetalRenderTarget::~MetalRenderTarget() {
  if (texture_) { texture_->release(); texture_ = nullptr; }
  if (msaa_texture_) { msaa_texture_->release(); msaa_texture_ = nullptr; }
  if (draw_texture_) { draw_texture_->release(); draw_texture_ = nullptr; }
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
