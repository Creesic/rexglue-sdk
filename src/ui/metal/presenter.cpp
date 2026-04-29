#include <rex/ui/metal/presenter.h>
#include <rex/ui/metal/provider.h>
#include <rex/ui/surface_mac.h>
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
  static std::atomic<int> present_count{0};
  int pc = present_count.fetch_add(1);
  if (pc < 10) {
    fprintf(stderr, "[metal] PaintAndPresent: src=%p drawable=%p (%ux%u)\n",
            src, drawable ? drawable->texture() : nullptr,
            drawable ? (unsigned)drawable->texture()->width() : 0,
            drawable ? (unsigned)drawable->texture()->height() : 0); fflush(stderr);
  }
  if (src) {
    MTL::BlitCommandEncoder* blit = cmd->blitCommandEncoder();
    if (blit) {
      uint32_t copy_w = std::min(src->width(), drawable->texture()->width());
      uint32_t copy_h = std::min(src->height(), drawable->texture()->height());
      blit->copyFromTexture(src, 0, 0, MTL::Origin(0, 0, 0),
                             MTL::Size(copy_w, copy_h, 1),
                             drawable->texture(), 0, 0,
                             MTL::Origin(0, 0, 0));
      if (copy_w < (uint32_t)drawable->texture()->width()) {
        blit->copyFromTexture(src, 0, 0, MTL::Origin(0, 0, 0),
                               MTL::Size(copy_w, copy_h, 1),
                               drawable->texture(), 0, 0,
                               MTL::Origin(copy_w, 0, 0));
      }
      blit->endEncoding();
      if (pc < 3) {
        fprintf(stderr, "[metal] PaintAndPresent: blitted %ux%u from %p to drawable (%ux%u)\n",
                copy_w, copy_h, src,
                (unsigned)drawable->texture()->width(),
                (unsigned)drawable->texture()->height());
        fflush(stderr);
      }
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
  fprintf(stderr, "[metal] ConnectOrReconnect: type=%d %ux%u was_paintable=%d\n",
          (int)new_surface.GetType(), new_surface_width, new_surface_height, was_paintable);
  fflush(stderr);
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
    std::function<bool(GuestOutputRefreshContext& context)> refresher,
    bool& is_8bpc_out_ref) {
  static std::atomic<int> refresh_count{0};
  int rc = refresh_count.fetch_add(1);
  if (rc < 5) {
    fprintf(stderr, "[metal] RefreshGuestOutputImpl: mailbox=%u %ux%u\n",
            mailbox_index, frontbuffer_width, frontbuffer_height); fflush(stderr);
  }
  MetalGuestOutputRefreshContext context(is_8bpc_out_ref);
  bool ok = refresher(context);
  if (rc < 5) {
    fprintf(stderr, "[metal] RefreshGuestOutputImpl: refresher returned %d\n", ok); fflush(stderr);
  }
  return ok;
}

}  // namespace metal
}  // namespace ui
}  // namespace rex
