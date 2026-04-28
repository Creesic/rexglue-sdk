#include <rex/ui/metal/presenter.h>
#include <rex/ui/metal/provider.h>
#include <rex/logging/macros.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace rex {
namespace ui {
namespace metal {

MetalPresenter::MetalPresenter(MetalProvider* provider,
                               HostGpuLossCallback host_gpu_loss_callback)
    : Presenter(host_gpu_loss_callback), provider_(provider) {
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
  REXLOG_INFO("MetalPresenter: Initialized on device {}", device_->name()->utf8String());
  return true;
}

void MetalPresenter::Shutdown() {
  metal_layer_ = nullptr;
}

Surface::TypeFlags MetalPresenter::GetSupportedSurfaceTypes() const {
  return Surface::kTypeFlag_CAMetalLayer;
}

bool MetalPresenter::CaptureGuestOutput(RawImage& image_out) {
  return false;
}

Presenter::PaintResult MetalPresenter::PaintAndPresentImpl(
    bool execute_ui_drawers) {
  if (!metal_layer_ || !device_) {
    return PaintResult::kNotPresented;
  }

  MTL::CommandQueue* queue = provider_->GetCommandQueue();
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

  MTL::CommandBuffer* cmd = queue->commandBuffer();
  if (!cmd) {
    pool->drain();
    return PaintResult::kNotPresented;
  }

  MTL::Texture* src = provider_->GetFrontbufferTexture();
  if (src) {
    MTL::BlitCommandEncoder* blit = cmd->blitCommandEncoder();
    if (blit) {
      blit->copyFromTexture(src, 0, 0, MTL::Origin(0, 0, 0),
                            MTL::Size(src->width(), src->height(), 1),
                            drawable->texture(), 0, 0,
                            MTL::Origin(0, 0, 0));
      blit->endEncoding();
    }
  }

  cmd->presentDrawable(drawable);
  cmd->commit();

  pool->drain();
  return PaintResult::kPresented;
}

Presenter::SurfacePaintConnectResult
MetalPresenter::ConnectOrReconnectPaintingToSurfaceFromUIThread(
    Surface& new_surface, uint32_t new_surface_width,
    uint32_t new_surface_height, bool was_paintable,
    bool& is_vsync_implicit_out) {
  is_vsync_implicit_out = true;
  return SurfacePaintConnectResult::kSuccess;
}

void MetalPresenter::DisconnectPaintingFromSurfaceFromUIThreadImpl() {
  metal_layer_ = nullptr;
}

bool MetalPresenter::RefreshGuestOutputImpl(
    uint32_t mailbox_index, uint32_t frontbuffer_width,
    uint32_t frontbuffer_height,
    std::function<bool(GuestOutputRefreshContext& context)> refresher,
    bool& is_8bpc_out_ref) {
  return false;
}

}  // namespace metal
}  // namespace ui
}  // namespace rex
