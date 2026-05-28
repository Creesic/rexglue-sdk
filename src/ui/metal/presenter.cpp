#include <rex/ui/metal/presenter.h>
#include <rex/ui/metal/provider.h>
#include <rex/ui/surface_mac.h>
#include <rex/logging/macros.h>
#include <rex/cvar.h>
#include <rex/graphics/metal/metal4_context.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>

#include "shaders/blit_metallib.h"

REXCVAR_DEFINE_BOOL(metal_hud, true, "GPU", "Enable Metal performance HUD overlay");
REXCVAR_DEFINE_BOOL(metal_frame_timing, true, "GPU", "Log GPU frame timing info");

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
    mtl4_ = provider_->GetMetal4Context();
  }
}

MetalPresenter::~MetalPresenter() { Shutdown(); }

bool MetalPresenter::Initialize() {
  if (!device_) {
    if (provider_) {
      device_ = provider_->GetDevice();
      mtl4_ = provider_->GetMetal4Context();
    }
    if (!device_) {
      REXLOG_ERROR("MetalPresenter: No Metal device");
      return false;
    }
  }
  if (!mtl4_) {
    REXLOG_ERROR("MetalPresenter: No Metal4Context");
    return false;
  }

  guest_output_shared_event_ = device_->newSharedEvent();
  if (!guest_output_shared_event_) {
    REXLOG_WARN("MetalPresenter: SharedEvent unavailable; no submission tracking");
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

  REXLOG_INFO("MetalPresenter: Initialized (MTL4) on device {}", device_->name()->utf8String());
  return true;
}

void MetalPresenter::Shutdown() {
  if (gamma_ramp_pwl_texture_) { gamma_ramp_pwl_texture_->release(); gamma_ramp_pwl_texture_ = nullptr; }
  if (gamma_ramp_table_texture_) { gamma_ramp_table_texture_->release(); gamma_ramp_table_texture_ = nullptr; }
  if (gamma_ramp_buffer_) { gamma_ramp_buffer_->release(); gamma_ramp_buffer_ = nullptr; }
  if (guest_output_shared_event_) { guest_output_shared_event_->release(); guest_output_shared_event_ = nullptr; }
  for (auto& tex : guest_output_textures_) {
    if (tex) tex->release();
    tex = nullptr;
  }
  metal_layer_ = nullptr;
  mtl4_ = nullptr;
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
  if (submission_out) {
    *submission_out = 0;
  }
  static int copy_log_count = 0;
  if (copy_log_count < 12) {
    fprintf(stderr,
            "[present] copy src=%p dst=%p %ux%u srcfmt=%u dstfmt=%u srcuse=0x%X dstuse=0x%X rb=%d gamma=%d\n",
            (void*)source_texture, (void*)dest_texture, source_width,
            source_height,
            source_texture ? static_cast<uint32_t>(source_texture->pixelFormat())
                           : 0u,
            dest_texture ? static_cast<uint32_t>(dest_texture->pixelFormat())
                         : 0u,
            source_texture ? static_cast<uint32_t>(source_texture->usage()) : 0u,
            dest_texture ? static_cast<uint32_t>(dest_texture->usage()) : 0u,
            force_swap_rb ? 1 : 0, use_pwl_gamma_ramp ? 1 : 0);
    fflush(stderr);
    ++copy_log_count;
  }
  if (source_texture && device_ &&
      (source_texture->pixelFormat() == MTL::PixelFormatRGBA8Unorm ||
       source_texture->pixelFormat() == MTL::PixelFormatRGBA16Float)) {
    static int readback_log_count = 0;
    if (readback_log_count < 8) {
      struct SamplePoint {
        uint32_t x;
        uint32_t y;
      };
      SamplePoint points[4] = {
          {0, 0},
          {std::min<uint32_t>(100, static_cast<uint32_t>(source_texture->width() - 1)),
           std::min<uint32_t>(100, static_cast<uint32_t>(source_texture->height() - 1))},
          {static_cast<uint32_t>(source_texture->width() / 2),
           static_cast<uint32_t>(source_texture->height() / 2)},
          {std::max<uint32_t>(1, static_cast<uint32_t>(source_texture->width())) - 1,
           std::max<uint32_t>(1, static_cast<uint32_t>(source_texture->height())) - 1},
      };
      const bool is_rgba8 =
          source_texture->pixelFormat() == MTL::PixelFormatRGBA8Unorm;
      size_t sample_bytes = is_rgba8 ? size_t(4) : size_t(8);
      MTL::Buffer* readback_buffer = device_->newBuffer(
          sample_bytes * size_t(4), MTL::ResourceStorageModeShared);
      if (readback_buffer) {
        MTL4::CommandBuffer* readback_cmd = mtl4_->BeginStandaloneCommandBuffer();
        if (readback_cmd) {
          MTL4::ComputeCommandEncoder* readback_enc =
              readback_cmd->computeCommandEncoder();
          if (readback_enc) {
            for (size_t i = 0; i < 4; ++i) {
              readback_enc->copyFromTexture(
                  source_texture, 0, 0,
                  MTL::Origin(points[i].x, points[i].y, 0), MTL::Size(1, 1, 1),
                  readback_buffer, i * sample_bytes, sample_bytes, sample_bytes);
            }
            readback_enc->endEncoding();
            mtl4_->CommitStandaloneAndWait(readback_cmd);
            const uint8_t* px =
                reinterpret_cast<const uint8_t*>(readback_buffer->contents());
            if (is_rgba8) {
              fprintf(
                  stderr,
                  "[present] src8 samples: (0,0)=0x%02X%02X%02X%02X "
                  "(%u,%u)=0x%02X%02X%02X%02X "
                  "(%u,%u)=0x%02X%02X%02X%02X "
                  "(%u,%u)=0x%02X%02X%02X%02X",
                  px[0], px[1], px[2], px[3], points[1].x, points[1].y, px[4],
                  px[5], px[6], px[7], points[2].x, points[2].y, px[8], px[9],
                  px[10], px[11], points[3].x, points[3].y, px[12], px[13],
                  px[14], px[15]);
              if (readback_log_count < 4) {
                uint32_t probe_w = std::min<uint32_t>(
                    1280, static_cast<uint32_t>(source_texture->width()));
                uint32_t probe_h = static_cast<uint32_t>(source_texture->height());
                size_t probe_row_bytes = size_t(probe_w) * size_t(4);
                size_t probe_size = probe_row_bytes * size_t(probe_h);
                MTL::Buffer* probe_buffer = device_->newBuffer(
                    probe_size, MTL::ResourceStorageModeShared);
                if (probe_buffer) {
                  MTL4::CommandBuffer* probe_cmd =
                      mtl4_->BeginStandaloneCommandBuffer();
                  if (probe_cmd) {
                    MTL4::ComputeCommandEncoder* probe_enc =
                        probe_cmd->computeCommandEncoder();
                    if (probe_enc) {
                      probe_enc->copyFromTexture(
                          source_texture, 0, 0, MTL::Origin(0, 0, 0),
                          MTL::Size(probe_w, probe_h, 1), probe_buffer, 0,
                          probe_row_bytes, probe_size);
                      probe_enc->endEncoding();
                      mtl4_->CommitStandaloneAndWait(probe_cmd);
                      const uint8_t* probe =
                          reinterpret_cast<const uint8_t*>(
                              probe_buffer->contents());
                      uint32_t nonzero = 0;
                      uint32_t first_x = probe_w;
                      uint32_t first_y = probe_h;
                      uint32_t last_x = 0;
                      uint32_t last_y = 0;
                      for (uint32_t y = 0; y < probe_h; ++y) {
                        const uint8_t* row = probe + size_t(y) * probe_row_bytes;
                        for (uint32_t x = 0; x < probe_w; ++x) {
                          const uint8_t* p = row + size_t(x) * 4u;
                          if (p[0] || p[1] || p[2] || p[3]) {
                            ++nonzero;
                            first_x = std::min(first_x, x);
                            first_y = std::min(first_y, y);
                            last_x = std::max(last_x, x);
                            last_y = std::max(last_y, y);
                          }
                        }
                      }
                      if (nonzero) {
                        fprintf(stderr,
                                " [scan8] nonzero=%u bbox=[%u,%u]-[%u,%u]",
                                nonzero, first_x, first_y, last_x, last_y);
                      } else {
                        fprintf(stderr, " [scan8] nonzero=0");
                      }
                    } else {
                      probe_cmd->release();
                    }
                  }
                  probe_buffer->release();
                }
              }
              fprintf(stderr, "\n");
            } else {
              auto u16 = reinterpret_cast<const uint16_t*>(px);
              fprintf(
                  stderr,
                  "[present] src16f samples: (0,0)={%04X,%04X,%04X,%04X} "
                  "(%u,%u)={%04X,%04X,%04X,%04X} "
                  "(%u,%u)={%04X,%04X,%04X,%04X} "
                  "(%u,%u)={%04X,%04X,%04X,%04X}\n",
                  u16[0], u16[1], u16[2], u16[3], points[1].x, points[1].y,
                  u16[4], u16[5], u16[6], u16[7], points[2].x, points[2].y,
                  u16[8], u16[9], u16[10], u16[11], points[3].x, points[3].y,
                  u16[12], u16[13], u16[14], u16[15]);
              if (readback_log_count < 4) {
                uint32_t probe_w = std::min<uint32_t>(
                    1280, static_cast<uint32_t>(source_texture->width()));
                uint32_t probe_h = static_cast<uint32_t>(source_texture->height());
                size_t probe_row_bytes = size_t(probe_w) * size_t(8);
                size_t probe_size = probe_row_bytes * size_t(probe_h);
                MTL::Buffer* probe_buffer = device_->newBuffer(
                    probe_size, MTL::ResourceStorageModeShared);
                if (probe_buffer) {
                  MTL4::CommandBuffer* probe_cmd =
                      mtl4_->BeginStandaloneCommandBuffer();
                  if (probe_cmd) {
                    MTL4::ComputeCommandEncoder* probe_enc =
                        probe_cmd->computeCommandEncoder();
                    if (probe_enc) {
                      probe_enc->copyFromTexture(
                          source_texture, 0, 0, MTL::Origin(0, 0, 0),
                          MTL::Size(probe_w, probe_h, 1), probe_buffer, 0,
                          probe_row_bytes, probe_size);
                      probe_enc->endEncoding();
                      mtl4_->CommitStandaloneAndWait(probe_cmd);
                      const uint16_t* probe =
                          reinterpret_cast<const uint16_t*>(
                              probe_buffer->contents());
                      uint32_t nonzero = 0;
                      uint32_t first_x = probe_w;
                      uint32_t first_y = probe_h;
                      uint32_t last_x = 0;
                      uint32_t last_y = 0;
                      for (uint32_t y = 0; y < probe_h; ++y) {
                        const uint16_t* row =
                            probe + (size_t(y) * size_t(probe_w) * 4u);
                        for (uint32_t x = 0; x < probe_w; ++x) {
                          const uint16_t* p = row + size_t(x) * 4u;
                          if (p[0] || p[1] || p[2] || p[3]) {
                            ++nonzero;
                            first_x = std::min(first_x, x);
                            first_y = std::min(first_y, y);
                            last_x = std::max(last_x, x);
                            last_y = std::max(last_y, y);
                          }
                        }
                      }
                      if (nonzero) {
                        fprintf(stderr,
                                " [scan16f] nonzero=%u bbox=[%u,%u]-[%u,%u]",
                                nonzero, first_x, first_y, last_x, last_y);
                      } else {
                        fprintf(stderr, " [scan16f] nonzero=0");
                      }
                    } else {
                      probe_cmd->release();
                    }
                  }
                  probe_buffer->release();
                }
              }
              fprintf(stderr, "\n");
            }
            fflush(stderr);
            ++readback_log_count;
          } else {
            readback_cmd->release();
          }
        }
        readback_buffer->release();
      }
    }
  }
  if (!source_texture || !dest_texture || !mtl4_) {
    return false;
  }

  MTL4::CommandBuffer* cmd = mtl4_->BeginCommandBuffer();
  if (!cmd) return false;
  bool copy_success = false;
  uint32_t copy_width = std::min<uint32_t>(
      source_width,
      std::min<uint32_t>(static_cast<uint32_t>(source_texture->width()),
                         static_cast<uint32_t>(dest_texture->width())));
  uint32_t copy_height = std::min<uint32_t>(
      source_height,
      std::min<uint32_t>(static_cast<uint32_t>(source_texture->height()),
                         static_cast<uint32_t>(dest_texture->height())));

  MTL::PixelFormat source_format = source_texture->pixelFormat();
  MTL::PixelFormat dest_format = dest_texture->pixelFormat();
  bool can_raw_copy = source_format == dest_format;
  if (!can_raw_copy && force_swap_rb &&
      source_format == MTL::PixelFormatRGBA8Unorm &&
      dest_format == MTL::PixelFormatBGRA8Unorm) {
    can_raw_copy = true;
  }
  if (copy_width && copy_height && can_raw_copy) {
    static int raw_copy_log_count = 0;
    if (raw_copy_log_count < 8) {
      fprintf(stderr, "[present] copy-path=rawcopy %ux%u\n", copy_width,
              copy_height);
      fflush(stderr);
      ++raw_copy_log_count;
    }
    MTL4::ComputeCommandEncoder* compute = cmd->computeCommandEncoder();
    if (compute) {
      compute->copyFromTexture(source_texture, 0, 0, MTL::Origin(0, 0, 0),
                               MTL::Size(copy_width, copy_height, 1),
                               dest_texture, 0, 0, MTL::Origin(0, 0, 0));
      compute->endEncoding();
      copy_success = true;
    }
  } else if (blit_lib_) {
    static int shader_copy_log_count = 0;
    if (shader_copy_log_count < 8) {
      fprintf(stderr, "[present] copy-path=shadercopy srcfmt=%u dstfmt=%u\n",
              static_cast<uint32_t>(source_texture->pixelFormat()),
              static_cast<uint32_t>(dest_texture->pixelFormat()));
      fflush(stderr);
      ++shader_copy_log_count;
    }
    MTL::RenderPipelineState* pipe =
        GetOrCreateBlitPipeline(dest_texture->pixelFormat());
    if (pipe) {
      MTL4::RenderPassDescriptor* rpd =
          MTL4::RenderPassDescriptor::alloc()->init();
      auto* ca = rpd->colorAttachments()->object(0);
      ca->setTexture(dest_texture);
      ca->setLoadAction(MTL::LoadActionClear);
      ca->setClearColor(MTL::ClearColor(0, 0, 0, 1));
      ca->setStoreAction(MTL::StoreActionStore);
      MTL4::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
      if (enc) {
        enc->setRenderPipelineState(pipe);
        struct {
          float w, h, src_w, src_h;
        } ub = {(float)dest_texture->width(), (float)dest_texture->height(),
                (float)source_texture->width(), (float)source_texture->height()};
        mtl4_->SetVertexAddress(mtl4_->AllocInlineConstant(&ub, sizeof(ub)), 0);
        mtl4_->SetFragmentTexture(source_texture->gpuResourceID(), 0);
        mtl4_->SetFragmentSampler(GetNearestSampler()->gpuResourceID(), 0);
        mtl4_->FlushRenderBindings(enc);
        enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0),
                            NS::UInteger(3));
        enc->endEncoding();
        copy_success = true;
      }
      rpd->release();
    }
  }

  if (!copy_success) {
    mtl4_->Commit(cmd);
    return false;
  }

  uint64_t submission_id = 0;
  if (guest_output_shared_event_) {
    submission_id = guest_output_submission_counter_.fetch_add(1,
                    std::memory_order_relaxed) + 1;
  }

  mtl4_->Commit(cmd);

  if (submission_id && guest_output_shared_event_) {
    mtl4_->SignalEvent(guest_output_shared_event_, submission_id);
    // Validation mode: force producer completion before exposing mailbox.
    // This removes any queue-order ambiguity between copy submission and
    // subsequent presenter sampling.
    bool signaled = guest_output_shared_event_->waitUntilSignaledValue(
        submission_id, /*timeoutNS=*/5000000000ULL);
    if (!signaled) {
      static bool logged_wait_timeout = false;
      if (!logged_wait_timeout) {
        logged_wait_timeout = true;
        REXLOG_WARN(
            "MetalPresenter: timeout waiting for guest output submission {}",
            submission_id);
      }
    }
  }

  if (submission_out) {
    *submission_out = submission_id;
  }
  return true;
}

Presenter::PaintResult MetalPresenter::PaintAndPresentImpl(
    bool execute_ui_drawers) {
  if (!metal_layer_ || !device_ || !mtl4_) {
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

  uint32_t mailbox_index = UINT32_MAX;
  GuestOutputProperties guest_output_properties;
  GuestOutputPaintConfig guest_output_paint_config;
  MTL::Texture* guest_output_texture = nullptr;
  {
    std::unique_lock<std::mutex> guest_output_consumer_lock(
        this->ConsumeGuestOutput(mailbox_index, &guest_output_properties,
                                 &guest_output_paint_config));
    if (mailbox_index != UINT32_MAX && mailbox_index < guest_output_textures_.size()) {
      uint64_t await_submission = guest_output_submissions_[mailbox_index];
      if (await_submission > guest_output_waited_submission_ &&
          guest_output_shared_event_) {
        uint64_t completed_submission =
            guest_output_shared_event_->signaledValue();
        if (await_submission > completed_submission) {
          mtl4_->WaitEvent(guest_output_shared_event_, await_submission);
        }
        guest_output_waited_submission_ = await_submission;
      }
      guest_output_texture = guest_output_textures_[mailbox_index];
    }
  }

  static int paint_log_count = 0;
  if (paint_log_count < 12) {
    fprintf(stderr,
            "[present] paint mailbox=%u tex=%p direct=%p draw=%ux%u tex=%ux%u\n",
            mailbox_index, (void*)guest_output_texture,
            (void*)nullptr,
            static_cast<uint32_t>(dst->width()),
            static_cast<uint32_t>(dst->height()),
            guest_output_texture
                ? static_cast<uint32_t>(guest_output_texture->width())
                : 0u,
            guest_output_texture
                ? static_cast<uint32_t>(guest_output_texture->height())
                : 0u);
    fflush(stderr);
    ++paint_log_count;
  }

  MTL4::CommandBuffer* cmd = mtl4_->BeginCommandBuffer();
  if (!cmd) { pool->drain(); return PaintResult::kNotPresented; }

  if (guest_output_texture && blit_lib_) {
    MTL::RenderPipelineState* pipe = GetOrCreateBlitPipeline(dst->pixelFormat());
    if (pipe) {
      MTL4::RenderPassDescriptor* rpd = MTL4::RenderPassDescriptor::alloc()->init();
      auto* ca = rpd->colorAttachments()->object(0);
      ca->setTexture(dst);
      ca->setLoadAction(MTL::LoadActionClear);
      ca->setClearColor(MTL::ClearColor(0, 0, 0, 1));
      ca->setStoreAction(MTL::StoreActionStore);
      MTL4::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
      if (enc) {
        enc->setRenderPipelineState(pipe);

        float src_h = guest_output_texture->height() > 720 ? 720.0f
                          : (float)guest_output_texture->height();
        struct {
          float w, h, src_w, src_h;
        } ub = {(float)dst->width(), (float)dst->height(),
                (float)guest_output_texture->width(), src_h};

        mtl4_->SetVertexAddress(mtl4_->AllocInlineConstant(&ub, sizeof(ub)), 0);
        mtl4_->SetFragmentTexture(guest_output_texture->gpuResourceID(), 0);
        mtl4_->SetFragmentSampler(GetNearestSampler()->gpuResourceID(), 0);
        mtl4_->FlushRenderBindings(enc);

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

  mtl4_->Commit(cmd);
  mtl4_->SignalDrawable(drawable);

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
    desc->setUsage(MTL::TextureUsageShaderWrite | MTL::TextureUsageShaderRead |
                   MTL::TextureUsageRenderTarget);
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
