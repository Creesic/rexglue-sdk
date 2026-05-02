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

namespace {
constexpr bool kMetalVerboseDiagnostics = false;
}

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

  MTL::Texture* dst = drawable->texture();
  MTL::Texture* src = provider_->GetFrontbufferTexture();

  static std::atomic<int> paint_count{0};
  int pc = paint_count.fetch_add(1);
  if (pc < 10) {
    fprintf(stderr, "[metal] PAINT #%d: src=%p dst=%p blit_lib=%p\n", pc, src, dst, blit_lib_);
    fflush(stderr);
  }

  MTL::CommandBuffer* cmd = queue->commandBuffer();
  if (!cmd) { pool->drain(); return PaintResult::kNotPresented; }

  if (src && blit_lib_) {
    MTL::RenderPipelineState* pipe = GetOrCreateBlitPipeline(dst->pixelFormat());
    if (pipe) {
      static std::atomic<int> actual_blit_count{0};
      int abc = actual_blit_count.fetch_add(1);
      if (abc < 5) {
        fprintf(stderr, "[metal] ACTUAL BLIT #%d: src=%p %ux%u fmt=%d -> dst=%p %ux%u fmt=%d\n",
                abc, src, (unsigned)src->width(), (unsigned)src->height(), (int)src->pixelFormat(),
                dst, (unsigned)dst->width(), (unsigned)dst->height(), (int)dst->pixelFormat());
        fflush(stderr);
      }
      MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::alloc()->init();
      auto* ca = rpd->colorAttachments()->object(0);
      ca->setTexture(dst);
      ca->setLoadAction(MTL::LoadActionClear);
      ca->setClearColor(MTL::ClearColor(0, 1, 1, 1));
      ca->setStoreAction(MTL::StoreActionStore);
      MTL::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
      if (enc) {
        enc->setRenderPipelineState(pipe);
        enc->setFragmentTexture(src, 0);
        enc->setFragmentSamplerState(GetNearestSampler(), 0);
        float src_h = src->height() > 720 ? 720.0f : (float)src->height();
        struct { float w, h, src_w, src_h; } ub = {
          (float)dst->width(), (float)dst->height(),
          (float)src->width(), src_h
        };
        enc->setVertexBytes(&ub, sizeof(ub), 0);
        enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));
        enc->endEncoding();
      }
      rpd->release();
    }
  }

  static std::atomic<int> present_count{0};
  int prc = present_count.fetch_add(1);
  cmd->addCompletedHandler([prc](MTL::CommandBuffer* cb) {
    if (prc < 10) {
      fprintf(stderr, "[metal] PRESENT COMPLETE #%d: status=%d err=%s\n",
              prc, (int)cb->status(),
              cb->error() ? cb->error()->localizedDescription()->utf8String() : "none");
      fflush(stderr);
    }
  });
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
  if (!blit_pipe_) {
    fprintf(stderr, "[metal] BLIT PIPE FAILED: %s\n",
            err ? err->localizedDescription()->utf8String() : "null"); fflush(stderr);
  } else {
    blit_pipe_fmt_ = fmt;
    fprintf(stderr, "[metal] BLIT PIPE OK fmt=%d\n", (int)fmt); fflush(stderr);
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
    std::function<bool(GuestOutputRefreshContext& context)> refresher,
    bool& is_8bpc_out_ref) {
  MetalGuestOutputRefreshContext context(is_8bpc_out_ref);
  return refresher(context);
}

}  // namespace metal
}  // namespace ui
}  // namespace rex
