#include <rex/ui/metal/presenter.h>
#include <rex/ui/metal/provider.h>
#include <rex/ui/surface_mac.h>
#include <rex/logging/macros.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "shaders/blit_metallib.h"

namespace rex {
namespace ui {
namespace metal {

MetalPresenter::MetalPresenter(MetalProvider* provider,
                                HostGpuLossCallback host_gpu_loss_callback)
    : Presenter(host_gpu_loss_callback), provider_(provider) {
  guest_output_textures_.fill(nullptr);
  guest_output_submissions_.fill(0);
  if (provider_) {
    device_ = provider_->GetDevice();
  }
}

MetalPresenter::~MetalPresenter() { Shutdown(); }

bool MetalPresenter::Initialize() {
  if (!device_) {
    if (provider_) {
      device_ = provider_->GetDevice();
    }
    if (!device_) {
      REXLOG_ERROR("MetalPresenter: No Metal device");
      return false;
    }
  }

  command_queue_ = device_->newCommandQueue();
  if (!command_queue_) {
    REXLOG_ERROR("MetalPresenter: Failed to create command queue");
    return false;
  }

  dispatch_data_t libData = dispatch_data_create(
      blit_metallib, blit_metallib_len,
      dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
  NS::Error* err = nullptr;
  blit_lib_ = device_->newLibrary(libData, &err);
  dispatch_release(libData);
  if (blit_lib_) {
    REXLOG_INFO("MetalPresenter: Blit shader loaded");
  } else {
    REXLOG_ERROR("MetalPresenter: Failed to load blit shader: {}",
                 err ? err->localizedDescription()->utf8String() : "unknown");
  }

  REXLOG_INFO("MetalPresenter: Initialized on device {}", device_->name()->utf8String());
  return true;
}

void MetalPresenter::Shutdown() {
  for (auto& tex : guest_output_textures_) {
    if (tex) tex->release();
    tex = nullptr;
  }
  if (command_queue_) { command_queue_->release(); command_queue_ = nullptr; }
  metal_layer_ = nullptr;
}

Surface::TypeFlags MetalPresenter::GetSupportedSurfaceTypes() const {
  return Surface::kTypeFlag_CAMetalLayer;
}

bool MetalPresenter::CaptureGuestOutput(RawImage& image_out) {
  return false;
}

bool MetalPresenter::CopyTextureToGuestOutput(
    MTL::Texture* source_texture, MTL::Texture* dest_texture,
    uint32_t source_width, uint32_t source_height,
    bool force_swap_rb, bool use_pwl_gamma_ramp,
    uint64_t* submission_out) {
  if (!source_texture || !dest_texture || !command_queue_) {
    return false;
  }

  MTL::CommandBuffer* cmd = command_queue_->commandBuffer();
  if (!cmd) return false;

  MTL::BlitCommandEncoder* blit = cmd->blitCommandEncoder();
  if (!blit) return false;

  blit->copyFromTexture(source_texture, 0, 0,
                        MTL::Origin(0, 0, 0),
                        MTL::Size(source_width, source_height, 1),
                        dest_texture, 0, 0,
                        MTL::Origin(0, 0, 0));
  blit->endEncoding();
  cmd->commit();

  if (submission_out) {
    *submission_out = 0;
  }
  return true;
}

Presenter::PaintResult MetalPresenter::PaintAndPresentImpl(
    bool execute_ui_drawers) {
  if (!metal_layer_ || !device_) {
    return PaintResult::kNotPresented;
  }

  MTL::CommandQueue* queue = command_queue_;
  if (!queue) {
    return PaintResult::kNotPresented;
  }

  auto* pool = NS::AutoreleasePool::alloc()->init();
  CA::MetalLayer* layer = reinterpret_cast<CA::MetalLayer*>(metal_layer_);
  CA::MetalDrawable* drawable = layer->nextDrawable();
  if (!drawable) {
    pool->drain();
    return PaintResult::kNotPresented;
  }

  MTL::Texture* dst = drawable->texture();

  uint32_t mailbox_index = last_guest_output_mailbox_index_.load(
      std::memory_order_relaxed);
  MTL::Texture* guest_output_texture = nullptr;
  if (mailbox_index < guest_output_textures_.size()) {
    guest_output_texture = guest_output_textures_[mailbox_index];
  }

  MTL::CommandBuffer* cmd = queue->commandBuffer();
  if (!cmd) { pool->drain(); return PaintResult::kNotPresented; }

  if (guest_output_texture && blit_lib_) {
    MTL::RenderPipelineState* pipe = GetOrCreateBlitPipeline(dst->pixelFormat());
    if (pipe) {
      MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::alloc()->init();
      auto* ca = rpd->colorAttachments()->object(0);
      ca->setTexture(dst);
      ca->setLoadAction(MTL::LoadActionClear);
      ca->setClearColor(MTL::ClearColor(0, 0, 0, 1));
      ca->setStoreAction(MTL::StoreActionStore);
      MTL::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
      if (enc) {
        enc->setRenderPipelineState(pipe);
        enc->setFragmentTexture(guest_output_texture, 0);
        enc->setFragmentSamplerState(GetNearestSampler(), 0);
        float src_h = guest_output_texture->height() > 720 ? 720.0f
                          : (float)guest_output_texture->height();
        struct {
          float w, h, src_w, src_h;
        } ub = {(float)dst->width(), (float)dst->height(),
                (float)guest_output_texture->width(), src_h};
        enc->setVertexBytes(&ub, sizeof(ub), 0);
        enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0),
                            NS::UInteger(3));
        enc->endEncoding();
      }
      rpd->release();
    }
  } else {
    static bool logged_once = false;
    if (!logged_once) {
      REXLOG_WARN("Metal Paint: No guest output texture (mailbox_idx={}, tex={})",
                  mailbox_index, (void*)guest_output_texture);
      logged_once = true;
    }
  }

  cmd->presentDrawable(drawable);
  cmd->commit();
  pool->drain();
  return PaintResult::kPresented;
}

MTL::RenderPipelineState* MetalPresenter::GetOrCreateBlitPipeline(MTL::PixelFormat fmt) {
  if (blit_pipe_ && blit_pipe_fmt_ == fmt) return blit_pipe_;
  if (!blit_lib_) return nullptr;
  auto* vf = blit_lib_->newFunction(NS::String::string("blit_vs", NS::UTF8StringEncoding));
  auto* ff = blit_lib_->newFunction(NS::String::string("blit_fs", NS::UTF8StringEncoding));
  auto* desc = MTL::RenderPipelineDescriptor::alloc()->init();
  desc->setVertexFunction(vf);
  desc->setFragmentFunction(ff);
  desc->colorAttachments()->object(0)->setPixelFormat(fmt);
  NS::Error* err = nullptr;
  blit_pipe_ = device_->newRenderPipelineState(desc, MTL::PipelineOptionNone, nullptr, &err);
  if (blit_pipe_) {
    blit_pipe_fmt_ = fmt;
    REXLOG_INFO("MetalPresenter: Blit pipeline created fmt={}", (int)fmt);
  } else {
    REXLOG_ERROR("MetalPresenter: Blit pipeline failed: {}",
                 err ? err->localizedDescription()->utf8String() : "null");
  }
  desc->release();
  if (vf) vf->release();
  if (ff) ff->release();
  return blit_pipe_;
}

MTL::SamplerState* MetalPresenter::GetNearestSampler() {
  if (nearest_sampler_) return nearest_sampler_;
  auto* desc = MTL::SamplerDescriptor::alloc()->init();
  desc->setMinFilter(MTL::SamplerMinMagFilterNearest);
  desc->setMagFilter(MTL::SamplerMinMagFilterNearest);
  desc->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
  desc->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
  nearest_sampler_ = device_->newSamplerState(desc);
  desc->release();
  return nearest_sampler_;
}

Presenter::SurfacePaintConnectResult
MetalPresenter::ConnectOrReconnectPaintingToSurfaceFromUIThread(
    Surface& new_surface, uint32_t new_surface_width,
    uint32_t new_surface_height, bool was_paintable,
    bool& is_vsync_implicit_out) {
  is_vsync_implicit_out = true;

  Surface::TypeIndex surface_type = new_surface.GetType();
  if (surface_type != Surface::kTypeIndex_CAMetalLayer) {
    REXLOG_ERROR("MetalPresenter: Unsupported surface type {}", (int)surface_type);
    return SurfacePaintConnectResult::kFailureSurfaceUnusable;
  }

  auto& metal_surface = static_cast<const CAMetalLayerSurface&>(new_surface);
  CAMetalLayer* raw_layer = metal_surface.layer();
  if (!raw_layer) {
    REXLOG_ERROR("MetalPresenter: Null CAMetalLayer");
    return SurfacePaintConnectResult::kFailureSurfaceUnusable;
  }

  CA::MetalLayer* layer = reinterpret_cast<CA::MetalLayer*>(raw_layer);
  layer->setDevice(reinterpret_cast<MTL::Device*>(device_));
  layer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);
  layer->setDrawableSize(CGSize{(double)new_surface_width, (double)new_surface_height});

  metal_layer_ = raw_layer;

  REXLOG_INFO("MetalPresenter: Connected to CAMetalLayer {}x{}", new_surface_width, new_surface_height);
  return SurfacePaintConnectResult::kSuccess;
}

void MetalPresenter::DisconnectPaintingFromSurfaceFromUIThreadImpl() {
  metal_layer_ = nullptr;
}

bool MetalPresenter::RefreshGuestOutputImpl(
    uint32_t mailbox_index, uint32_t frontbuffer_width,
    uint32_t frontbuffer_height,
    std::function<bool(Presenter::GuestOutputRefreshContext& context)> refresher,
    bool& is_8bpc_out_ref) {
  if (mailbox_index >= guest_output_textures_.size()) {
    is_8bpc_out_ref = false;
    return false;
  }

  MTL::Texture* guest_output_texture = guest_output_textures_[mailbox_index];

  if (!guest_output_texture ||
      guest_output_texture->width() != frontbuffer_width ||
      guest_output_texture->height() != frontbuffer_height) {
    if (guest_output_texture) {
      guest_output_texture->release();
    }

    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType2D);
    desc->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    desc->setWidth(frontbuffer_width);
    desc->setHeight(frontbuffer_height);
    desc->setUsage(MTL::TextureUsageShaderWrite | MTL::TextureUsageShaderRead);
    desc->setStorageMode(MTL::StorageModePrivate);

    guest_output_texture = device_->newTexture(desc);
    desc->release();

    if (!guest_output_texture) {
      REXLOG_ERROR("Metal RefreshGuestOutput: Failed to create texture {}x{}",
                   frontbuffer_width, frontbuffer_height);
      is_8bpc_out_ref = false;
      return false;
    }

    guest_output_textures_[mailbox_index] = guest_output_texture;
    REXLOG_INFO("Metal RefreshGuestOutput: Created texture {}x{} for mailbox {}",
                frontbuffer_width, frontbuffer_height, mailbox_index);
  }

  MetalGuestOutputRefreshContext context(is_8bpc_out_ref, guest_output_texture);
  bool success = refresher(context);

  if (!success) {
    REXLOG_WARN("Metal RefreshGuestOutput: Refresher callback failed");
    return false;
  }

  last_guest_output_mailbox_index_.store(mailbox_index, std::memory_order_relaxed);
  guest_output_submissions_[mailbox_index] = context.submission_id();
  return true;
}

bool MetalPresenter::UpdateGammaRamp(const void* table_data, size_t table_bytes,
                                     const void* pwl_data, size_t pwl_bytes) {
  if (!table_data || !pwl_data || !table_bytes || !pwl_bytes || !device_) {
    gamma_ramp_table_valid_ = false;
    gamma_ramp_pwl_valid_ = false;
    return false;
  }

  size_t total_bytes = table_bytes + pwl_bytes;
  if (!gamma_ramp_buffer_ || gamma_ramp_buffer_size_ < total_bytes) {
    if (gamma_ramp_table_texture_) { gamma_ramp_table_texture_->release(); gamma_ramp_table_texture_ = nullptr; }
    if (gamma_ramp_pwl_texture_) { gamma_ramp_pwl_texture_->release(); gamma_ramp_pwl_texture_ = nullptr; }
    if (gamma_ramp_buffer_) { gamma_ramp_buffer_->release(); gamma_ramp_buffer_ = nullptr; }

    gamma_ramp_buffer_ = device_->newBuffer(total_bytes, MTL::ResourceStorageModeShared);
    if (!gamma_ramp_buffer_) {
      gamma_ramp_buffer_size_ = 0;
      gamma_ramp_table_valid_ = false;
      gamma_ramp_pwl_valid_ = false;
      return false;
    }
    gamma_ramp_buffer_size_ = static_cast<uint32_t>(total_bytes);
  }

  void* contents = gamma_ramp_buffer_->contents();
  std::memcpy(contents, table_data, table_bytes);
  std::memcpy(reinterpret_cast<uint8_t*>(contents) + table_bytes, pwl_data, pwl_bytes);

  if (!gamma_ramp_table_texture_) {
    MTL::TextureDescriptor* table_desc = MTL::TextureDescriptor::textureBufferDescriptor(
        MTL::PixelFormatRGB10A2Unorm, 256, MTL::ResourceStorageModeShared,
        MTL::TextureUsageShaderRead);
    gamma_ramp_table_texture_ = gamma_ramp_buffer_->newTexture(
        table_desc, 0, 256 * sizeof(uint32_t));
    table_desc->release();
  }
  if (!gamma_ramp_pwl_texture_) {
    MTL::TextureDescriptor* pwl_desc = MTL::TextureDescriptor::textureBufferDescriptor(
        MTL::PixelFormatRG16Uint, 384, MTL::ResourceStorageModeShared,
        MTL::TextureUsageShaderRead);
    gamma_ramp_pwl_texture_ = gamma_ramp_buffer_->newTexture(
        pwl_desc, table_bytes, 384 * sizeof(uint32_t));
    pwl_desc->release();
  }

  gamma_ramp_table_valid_ = gamma_ramp_table_texture_ != nullptr;
  gamma_ramp_pwl_valid_ = gamma_ramp_pwl_texture_ != nullptr;
  return gamma_ramp_table_valid_ && gamma_ramp_pwl_valid_;
}

}  // namespace metal
}  // namespace ui
}  // namespace rex
