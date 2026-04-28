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

  fprintf(stderr, "[metal] CreateRenderTarget: depth=%d fmt=%d %ux%u -> %ux%u\n",
          key.is_depth, (int)format, w, h, scaled_w, scaled_h); fflush(stderr);

  MTL::Texture* tex = CreateRenderTargetTexture(scaled_w, scaled_h, format);
  fprintf(stderr, "[metal] CreateRenderTarget: tex=%p\n", tex); fflush(stderr);
  rt->SetTexture(tex);
  if (tex) tex->release();

  rt->SetNeedsInitialClear(true);
  return rt;
}

MTL::Texture* MetalRenderTargetCache::CreateRenderTargetTexture(
    uint32_t width, uint32_t height, MTL::PixelFormat format,
    uint32_t sample_count) {
  if (format == MTL::PixelFormatInvalid) return nullptr;
  if (!device_) {
    fprintf(stderr, "[metal] CreateRenderTargetTexture: no device!\n"); fflush(stderr);
    return nullptr;
  }

  MTL::TextureDescriptor* desc = MTL::TextureDescriptor::texture2DDescriptor(
      format, width, height,
      MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
  if (sample_count > 1) {
    desc->setTextureType(MTL::TextureType2DMultisample);
    desc->setSampleCount(sample_count);
  }
  desc->setStorageMode(MTL::StorageModePrivate);

  fprintf(stderr, "[metal] CreateRenderTargetTexture: %ux%u fmt=%d device=%p desc=%p\n",
          width, height, (int)format, device_, desc); fflush(stderr);
  MTL::Texture* tex = device_->newTexture(desc);
  fprintf(stderr, "[metal] CreateRenderTargetTexture: tex=%p\n", tex); fflush(stderr);
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

MTL::Texture* MetalRenderTargetCache::GetColorTarget(uint32_t index) const {
  if (index < 4 && current_color_rt_[index]) {
    return current_color_rt_[index]->texture();
  }
  return nullptr;
}

MTL::Texture* MetalRenderTargetCache::GetDepthTarget() const {
  if (current_depth_rt_) {
    return current_depth_rt_->texture();
  }
  return nullptr;
}

bool MetalRenderTargetCache::Update(
    bool is_rasterization_done,
    reg::RB_DEPTHCONTROL normalized_depth_control,
    uint32_t normalized_color_mask,
    const Shader& vertex_shader) {
  fprintf(stderr, "[metal] RT::Update: calling base Update\n"); fflush(stderr);
  if (!RenderTargetCache::Update(is_rasterization_done,
                                  normalized_depth_control,
                                  normalized_color_mask,
                                  vertex_shader)) {
    fprintf(stderr, "[metal] RT::Update: base Update FAILED\n"); fflush(stderr);
    return false;
  }
  fprintf(stderr, "[metal] RT::Update: base Update OK\n"); fflush(stderr);

  const RenderTarget* const* accumulated =
      last_update_accumulated_render_targets();

  fprintf(stderr, "[metal] RT::Update: depth=%p c0=%p c1=%p c2=%p c3=%p\n",
          accumulated[0], accumulated[1], accumulated[2], accumulated[3], accumulated[4]); fflush(stderr);

  std::memset(current_color_rt_, 0, sizeof(current_color_rt_));
  current_depth_rt_ = nullptr;
  std::memset(current_color_formats_, 0, sizeof(current_color_formats_));
  current_depth_format_ = MTL::PixelFormatInvalid;
  current_stencil_format_ = MTL::PixelFormatInvalid;

  if (accumulated[0]) {
    auto* depth_rt = static_cast<MetalRenderTarget*>(
        const_cast<RenderTarget*>(accumulated[0]));
    current_depth_rt_ = depth_rt;
    current_depth_format_ = GetMetalDepthFormat(depth_rt->key().GetDepthFormat());
    current_stencil_format_ = current_depth_format_;
  }

  for (uint32_t i = 0; i < xenos::kMaxColorRenderTargets; i++) {
    if (accumulated[1 + i]) {
      auto* color_rt = static_cast<MetalRenderTarget*>(
          const_cast<RenderTarget*>(accumulated[1 + i]));
      current_color_rt_[i] = color_rt;
      current_color_formats_[i] = GetMetalColorFormat(color_rt->key().GetColorFormat());
    }
  }

  auto rb_surface_info = register_file().Get<reg::RB_SURFACE_INFO>();
  current_sample_count_ = 1u << uint32_t(rb_surface_info.msaa_samples);

  return UpdateRenderPass();
}

bool MetalRenderTargetCache::UpdateRenderPass() {
  fprintf(stderr, "[metal] UpdateRenderPass: enter\n"); fflush(stderr);
  command_processor_.EndRenderEncoder();

  fprintf(stderr, "[metal] UpdateRenderPass: EndRenderEncoder done\n"); fflush(stderr);
  MTL::CommandBuffer* cmd = command_processor_.EnsureCommandBuffer();
  fprintf(stderr, "[metal] UpdateRenderPass: cmd=%p\n", cmd); fflush(stderr);
  if (!cmd) {
    REXLOG_ERROR("MetalRenderTargetCache: No command buffer for render pass");
    return false;
  }

  MTL::RenderPassDescriptor* desc = MTL::RenderPassDescriptor::alloc()->init();
  fprintf(stderr, "[metal] UpdateRenderPass: desc=%p\n", desc); fflush(stderr);

  if (current_depth_rt_) {
    MTL::RenderPassDepthAttachmentDescriptor* depth = desc->depthAttachment();
    depth->setTexture(current_depth_rt_->texture());
    depth->setLoadAction(MTL::LoadActionLoad);
    depth->setStoreAction(MTL::StoreActionStore);

    if (current_stencil_format_ == MTL::PixelFormatDepth32Float_Stencil8) {
      MTL::RenderPassStencilAttachmentDescriptor* stencil = desc->stencilAttachment();
      stencil->setTexture(current_depth_rt_->texture());
      stencil->setLoadAction(MTL::LoadActionLoad);
      stencil->setStoreAction(MTL::StoreActionStore);
    }

    if (current_depth_rt_->needs_initial_clear()) {
      depth->setLoadAction(MTL::LoadActionClear);
      depth->setClearDepth(1.0);
      if (current_stencil_format_ == MTL::PixelFormatDepth32Float_Stencil8) {
        desc->stencilAttachment()->setLoadAction(MTL::LoadActionClear);
        desc->stencilAttachment()->setClearStencil(0);
      }
      current_depth_rt_->SetNeedsInitialClear(false);
    }
  }

  bool has_any_attachment = current_depth_rt_ != nullptr;
  for (uint32_t i = 0; i < xenos::kMaxColorRenderTargets; i++) {
    if (current_color_rt_[i]) {
      has_any_attachment = true;
      MTL::RenderPassColorAttachmentDescriptor* color =
          desc->colorAttachments()->object(i);
      color->setTexture(current_color_rt_[i]->texture());
      color->setLoadAction(MTL::LoadActionLoad);
      color->setStoreAction(MTL::StoreActionStore);

      if (current_color_rt_[i]->needs_initial_clear()) {
        color->setLoadAction(MTL::LoadActionClear);
        color->setClearColor(MTL::ClearColor(0, 0, 0, 0));
        current_color_rt_[i]->SetNeedsInitialClear(false);
      }
    }
  }

  fprintf(stderr, "[metal] UpdateRenderPass: has_any=%d depth_tex=%p c0_tex=%p\n",
          has_any_attachment, 
          current_depth_rt_ ? current_depth_rt_->texture() : nullptr,
          current_color_rt_[0] ? current_color_rt_[0]->texture() : nullptr); fflush(stderr);

  if (!has_any_attachment) {
    desc->release();
    fprintf(stderr, "[metal] UpdateRenderPass: no attachments\n"); fflush(stderr);
    return true;
  }

  MTL::RenderCommandEncoder* encoder = cmd->renderCommandEncoder(desc);
  if (!encoder) {
    REXLOG_ERROR("MetalRenderTargetCache: Failed to create render command encoder");
    desc->release();
    return false;
  }

  fprintf(stderr, "[metal] UpdateRenderPass: created encoder=%p depth=%p color0=%p\n",
          encoder, current_depth_rt_ ? current_depth_rt_->texture() : nullptr,
          current_color_rt_[0] ? current_color_rt_[0]->texture() : nullptr); fflush(stderr);

  command_processor_.SetRenderEncoder(encoder, desc);
  encoder->release();

  return true;
}

MetalRenderTargetCache::MetalRenderTarget::~MetalRenderTarget() {
  if (texture_) { texture_->release(); texture_ = nullptr; }
  if (msaa_texture_) { msaa_texture_->release(); msaa_texture_ = nullptr; }
  if (draw_texture_) { draw_texture_->release(); draw_texture_ = nullptr; }
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
