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
REXCVAR_DEFINE_BOOL(metal_present_debug_probes, false, "GPU",
                    "Enable extra Metal present-path debug probes/logs");

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

  MTL4::ArgumentTableDescriptor* blit_v_desc =
      MTL4::ArgumentTableDescriptor::alloc()->init();
  blit_v_desc->setMaxBufferBindCount(1);
  blit_v_desc->setMaxTextureBindCount(0);
  blit_v_desc->setMaxSamplerStateBindCount(0);
  blit_v_desc->setInitializeBindings(true);
  err = nullptr;
  blit_vertex_arg_table_ = device_->newArgumentTable(blit_v_desc, &err);
  blit_v_desc->release();
  if (!blit_vertex_arg_table_) {
    REXLOG_ERROR("MetalPresenter: Failed to create blit vertex argument table: {}",
                 err ? err->localizedDescription()->utf8String() : "unknown");
    return false;
  }

  MTL4::ArgumentTableDescriptor* blit_f_desc =
      MTL4::ArgumentTableDescriptor::alloc()->init();
  blit_f_desc->setMaxBufferBindCount(0);
  blit_f_desc->setMaxTextureBindCount(1);
  blit_f_desc->setMaxSamplerStateBindCount(1);
  blit_f_desc->setInitializeBindings(true);
  err = nullptr;
  blit_fragment_arg_table_ = device_->newArgumentTable(blit_f_desc, &err);
  blit_f_desc->release();
  if (!blit_fragment_arg_table_) {
    REXLOG_ERROR(
        "MetalPresenter: Failed to create blit fragment argument table: {}",
        err ? err->localizedDescription()->utf8String() : "unknown");
    return false;
  }

  REXLOG_INFO("MetalPresenter: Initialized (MTL4) on device {}", device_->name()->utf8String());
  return true;
}

void MetalPresenter::Shutdown() {
  if (gamma_ramp_pwl_texture_) { gamma_ramp_pwl_texture_->release(); gamma_ramp_pwl_texture_ = nullptr; }
  if (gamma_ramp_table_texture_) { gamma_ramp_table_texture_->release(); gamma_ramp_table_texture_ = nullptr; }
  if (gamma_ramp_buffer_) { gamma_ramp_buffer_->release(); gamma_ramp_buffer_ = nullptr; }
  if (guest_output_shared_event_) { guest_output_shared_event_->release(); guest_output_shared_event_ = nullptr; }
  if (blit_fragment_arg_table_) { blit_fragment_arg_table_->release(); blit_fragment_arg_table_ = nullptr; }
  if (blit_vertex_arg_table_) { blit_vertex_arg_table_->release(); blit_vertex_arg_table_ = nullptr; }
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
  if (!source_texture || !dest_texture || !mtl4_) {
    return false;
  }

  uint32_t copy_width = std::min<uint32_t>(
      source_width,
      std::min<uint32_t>(static_cast<uint32_t>(source_texture->width()),
                         static_cast<uint32_t>(dest_texture->width())));
  uint32_t copy_height = std::min<uint32_t>(
      source_height,
      std::min<uint32_t>(static_cast<uint32_t>(source_texture->height()),
                         static_cast<uint32_t>(dest_texture->height())));
  if (!copy_width || !copy_height) {
    return false;
  }

  MTL::Texture* sample_texture = source_texture;
  MTL::Texture* present_view = nullptr;
  if (sample_texture->textureType() == MTL::TextureType2DArray) {
    NS::Range level_range = NS::Range::Make(0, sample_texture->mipmapLevelCount());
    NS::Range slice_range = NS::Range::Make(0, 1);
    present_view = sample_texture->newTextureView(
        sample_texture->pixelFormat(), MTL::TextureType2D, level_range,
        slice_range, sample_texture->swizzle());
    if (present_view) {
      sample_texture = present_view;
    }
  }

  MTL::Texture* swizzle_view = nullptr;
  bool swap_rb_in_shader = force_swap_rb;
  if (force_swap_rb) {
    NS::Range level_range = NS::Range::Make(0, sample_texture->mipmapLevelCount());
    NS::Range slice_range = NS::Range::Make(0, sample_texture->arrayLength());
    MTL::TextureSwizzleChannels swizzle = {
        MTL::TextureSwizzleBlue, MTL::TextureSwizzleGreen,
        MTL::TextureSwizzleRed, MTL::TextureSwizzleAlpha};
    swizzle_view = sample_texture->newTextureView(
        sample_texture->pixelFormat(), sample_texture->textureType(),
        level_range, slice_range, swizzle);
    if (swizzle_view) {
      sample_texture = swizzle_view;
      swap_rb_in_shader = false;
    }
  }

  auto release_views = [&]() {
    if (swizzle_view) {
      swizzle_view->release();
      swizzle_view = nullptr;
    }
    if (present_view) {
      present_view->release();
      present_view = nullptr;
    }
  };
  MTL4::ArgumentTable* copy_vertex_arg_table = nullptr;
  MTL4::ArgumentTable* copy_fragment_arg_table = nullptr;
  auto release_copy_tables = [&]() {
    if (copy_fragment_arg_table) {
      copy_fragment_arg_table->release();
      copy_fragment_arg_table = nullptr;
    }
    if (copy_vertex_arg_table) {
      copy_vertex_arg_table->release();
      copy_vertex_arg_table = nullptr;
    }
  };
  auto commit_with_view_lifetime = [&](MTL4::CommandBuffer* cb) {
    if (!cb) {
      return;
    }
    if (swizzle_view || present_view || copy_vertex_arg_table ||
        copy_fragment_arg_table) {
      if (swizzle_view) {
        swizzle_view->retain();
      }
      if (present_view) {
        present_view->retain();
      }
      if (copy_vertex_arg_table) {
        copy_vertex_arg_table->retain();
      }
      if (copy_fragment_arg_table) {
        copy_fragment_arg_table->retain();
      }
      MTL::Texture* swizzle_to_release = swizzle_view;
      MTL::Texture* present_to_release = present_view;
      MTL4::ArgumentTable* vertex_table_to_release = copy_vertex_arg_table;
      MTL4::ArgumentTable* fragment_table_to_release = copy_fragment_arg_table;
      MTL4::CommitOptions* options = MTL4::CommitOptions::alloc()->init();
      options->addFeedbackHandler(
          [swizzle_to_release, present_to_release, vertex_table_to_release,
           fragment_table_to_release](MTL4::CommitFeedback*) {
            if (swizzle_to_release) {
              swizzle_to_release->release();
            }
            if (present_to_release) {
              present_to_release->release();
            }
            if (vertex_table_to_release) {
              vertex_table_to_release->release();
            }
            if (fragment_table_to_release) {
              fragment_table_to_release->release();
            }
          });
      mtl4_->Commit(cb, options);
      options->release();
    } else {
      mtl4_->Commit(cb);
    }
  };

  const bool needs_shader_copy =
      swap_rb_in_shader || use_pwl_gamma_ramp ||
      sample_texture->pixelFormat() != dest_texture->pixelFormat() ||
      sample_texture->textureType() != MTL::TextureType2D;
  static uint32_t copy_path_log_count = 0;
  if (REXCVAR_GET(metal_present_debug_probes) && copy_path_log_count < 64) {
    fprintf(stderr,
            "[present-copy-path] %s srcfmt=%u dstfmt=%u rb=%u gamma=%u "
            "src_type=%u dst_type=%u\n",
            needs_shader_copy ? "shader" : "raw",
            static_cast<uint32_t>(sample_texture->pixelFormat()),
            static_cast<uint32_t>(dest_texture->pixelFormat()),
            force_swap_rb ? 1u : 0u, use_pwl_gamma_ramp ? 1u : 0u,
            static_cast<uint32_t>(sample_texture->textureType()),
            static_cast<uint32_t>(dest_texture->textureType()));
    fflush(stderr);
    ++copy_path_log_count;
  }

  MTL4::CommandBuffer* cmd = mtl4_->BeginCommandBuffer();
  if (!cmd) {
    release_views();
    return false;
  }

  // MTL4 explicit residency: source/destination textures in the present path
  // may not be tracked by draw-time bindings for this command buffer.
  mtl4_->AddResidentAllocation(source_texture);
  mtl4_->AddResidentAllocation(sample_texture);
  mtl4_->AddResidentAllocation(dest_texture);
  mtl4_->CommitResidency();

  bool copy_success = false;
  if (!needs_shader_copy) {
    MTL4::ComputeCommandEncoder* compute = cmd->computeCommandEncoder();
    if (compute) {
      compute->copyFromTexture(sample_texture, 0, 0, MTL::Origin(0, 0, 0),
                               MTL::Size(copy_width, copy_height, 1),
                               dest_texture, 0, 0, MTL::Origin(0, 0, 0));
      compute->endEncoding();
      copy_success = true;
    }
  } else {
    if (blit_lib_) {
      MTL::RenderPipelineState* pipe =
          GetOrCreateBlitPipeline(dest_texture->pixelFormat());
      if (pipe) {
        MTL4::ArgumentTableDescriptor* blit_v_desc =
            MTL4::ArgumentTableDescriptor::alloc()->init();
        blit_v_desc->setMaxBufferBindCount(1);
        blit_v_desc->setMaxTextureBindCount(0);
        blit_v_desc->setMaxSamplerStateBindCount(0);
        blit_v_desc->setInitializeBindings(true);
        NS::Error* table_err = nullptr;
        copy_vertex_arg_table = device_->newArgumentTable(blit_v_desc, &table_err);
        blit_v_desc->release();
        if (!copy_vertex_arg_table) {
          REXLOG_WARN(
              "MetalPresenter: Failed to create copy vertex argument table: {}",
              table_err ? table_err->localizedDescription()->utf8String()
                        : "unknown");
        }
        MTL4::ArgumentTableDescriptor* blit_f_desc =
            MTL4::ArgumentTableDescriptor::alloc()->init();
        blit_f_desc->setMaxBufferBindCount(0);
        blit_f_desc->setMaxTextureBindCount(1);
        blit_f_desc->setMaxSamplerStateBindCount(1);
        blit_f_desc->setInitializeBindings(true);
        table_err = nullptr;
        copy_fragment_arg_table =
            device_->newArgumentTable(blit_f_desc, &table_err);
        blit_f_desc->release();
        if (!copy_fragment_arg_table) {
          REXLOG_WARN(
              "MetalPresenter: Failed to create copy fragment argument table: {}",
              table_err ? table_err->localizedDescription()->utf8String()
                        : "unknown");
        }
        MTL4::RenderPassDescriptor* rpd = MTL4::RenderPassDescriptor::alloc()->init();
        auto* ca = rpd->colorAttachments()->object(0);
        ca->setTexture(dest_texture);
        ca->setLoadAction(MTL::LoadActionClear);
        ca->setClearColor(MTL::ClearColor(0, 0, 0, 1));
        ca->setStoreAction(MTL::StoreActionStore);
        MTL4::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
        if (enc) {
          enc->setRenderPipelineState(pipe);
          enc->setViewport(
              MTL::Viewport(0.0, 0.0, double(copy_width), double(copy_height), 0.0, 1.0));
          enc->setScissorRect(MTL::ScissorRect(0, 0, copy_width, copy_height));
          struct {
            float w, h, src_w, src_h;
          } ub = {(float)dest_texture->width(), (float)dest_texture->height(),
                  (float)sample_texture->width(), (float)sample_texture->height()};
          if (copy_vertex_arg_table && copy_fragment_arg_table) {
            copy_vertex_arg_table->setAddress(
                mtl4_->AllocInlineConstant(&ub, sizeof(ub)), 0);
            copy_fragment_arg_table->setTexture(sample_texture->gpuResourceID(),
                                                0);
            copy_fragment_arg_table->setSamplerState(
                GetNearestSampler()->gpuResourceID(), 0);
            enc->setArgumentTable(copy_vertex_arg_table, MTL::RenderStageVertex);
            enc->setArgumentTable(copy_fragment_arg_table,
                                  MTL::RenderStageFragment);
          }
          enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0),
                              NS::UInteger(3));
          enc->endEncoding();
          copy_success = true;
        }
        rpd->release();
      }
    }
  }

  if (!copy_success) {
    commit_with_view_lifetime(cmd);
    release_views();
    release_copy_tables();
    return false;
  }

  uint64_t submission_id = 0;
  if (guest_output_shared_event_) {
    submission_id =
        guest_output_submission_counter_.fetch_add(1, std::memory_order_relaxed) +
        1;
  }

  commit_with_view_lifetime(cmd);

  static uint32_t copy_probe_log_count = 0;
  if (REXCVAR_GET(metal_present_debug_probes) && copy_probe_log_count < 8 &&
      device_ && mtl4_) {
    constexpr size_t kProbeStride = 16;
    MTL::Buffer* probe_buffer =
        device_->newBuffer(kProbeStride * 2, MTL::ResourceStorageModeShared);
    if (probe_buffer) {
      MTL4::CommandBuffer* probe_cmd = mtl4_->BeginStandaloneCommandBuffer();
      if (probe_cmd) {
        MTL4::ComputeCommandEncoder* probe_enc = probe_cmd->computeCommandEncoder();
        if (probe_enc) {
          const uint32_t src_x = std::min<uint32_t>(
              copy_width - 1, static_cast<uint32_t>(sample_texture->width() / 2));
          const uint32_t src_y = std::min<uint32_t>(
              copy_height - 1, static_cast<uint32_t>(sample_texture->height() / 2));
          const uint32_t dst_x = std::min<uint32_t>(
              copy_width - 1, static_cast<uint32_t>(dest_texture->width() / 2));
          const uint32_t dst_y = std::min<uint32_t>(
              copy_height - 1, static_cast<uint32_t>(dest_texture->height() / 2));
          mtl4_->AddResidentAllocation(sample_texture);
          mtl4_->AddResidentAllocation(dest_texture);
          mtl4_->AddResidentAllocation(probe_buffer);
          mtl4_->CommitResidency();
          probe_enc->copyFromTexture(sample_texture, 0, 0,
                                     MTL::Origin(src_x, src_y, 0),
                                     MTL::Size(1, 1, 1), probe_buffer, 0,
                                     kProbeStride, kProbeStride);
          probe_enc->copyFromTexture(dest_texture, 0, 0,
                                     MTL::Origin(dst_x, dst_y, 0),
                                     MTL::Size(1, 1, 1), probe_buffer,
                                     kProbeStride, kProbeStride, kProbeStride);
          probe_enc->endEncoding();
          mtl4_->CommitStandaloneAndWait(probe_cmd);
          const uint8_t* b =
              reinterpret_cast<const uint8_t*>(probe_buffer->contents());
          uint32_t src_nonzero = 0;
          uint32_t dst_nonzero = 0;
          for (size_t i = 0; i < kProbeStride; ++i) {
            src_nonzero += b[i] ? 1u : 0u;
            dst_nonzero += b[kProbeStride + i] ? 1u : 0u;
          }
          fprintf(stderr,
                  "[present-probe] src_nonzero=%u dst_nonzero=%u "
                  "src8=0x%02X%02X%02X%02X dst8=0x%02X%02X%02X%02X\n",
                  src_nonzero, dst_nonzero, b[0], b[1], b[2], b[3],
                  b[kProbeStride + 0], b[kProbeStride + 1], b[kProbeStride + 2],
                  b[kProbeStride + 3]);
          fflush(stderr);
          ++copy_probe_log_count;
        } else {
          probe_cmd->release();
        }
      }
      probe_buffer->release();
    }
  }

  if (submission_id && guest_output_shared_event_) {
    mtl4_->SignalEvent(guest_output_shared_event_, submission_id);
  }

  if (submission_out) {
    *submission_out = submission_id;
  }
  release_views();
  release_copy_tables();
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
  static uint32_t drawable_probe_log_count = 0;
  const bool probe_drawable =
      REXCVAR_GET(metal_present_debug_probes) && drawable_probe_log_count < 8;
  MTL::Buffer* drawable_probe_buffer = nullptr;
  if (probe_drawable && device_) {
    drawable_probe_buffer =
        device_->newBuffer(size_t(16), MTL::ResourceStorageModeShared);
  }

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

  static uint32_t paint_probe_log_count = 0;
  if (REXCVAR_GET(metal_present_debug_probes) && paint_probe_log_count < 8 &&
      guest_output_texture && device_ && mtl4_) {
    constexpr size_t kProbeStride = 16;
    MTL::Buffer* probe_buffer =
        device_->newBuffer(kProbeStride, MTL::ResourceStorageModeShared);
    if (probe_buffer) {
      MTL4::CommandBuffer* probe_cmd = mtl4_->BeginStandaloneCommandBuffer();
      if (probe_cmd) {
        MTL4::ComputeCommandEncoder* probe_enc = probe_cmd->computeCommandEncoder();
        if (probe_enc) {
          const uint32_t src_x = static_cast<uint32_t>(guest_output_texture->width() / 2);
          const uint32_t src_y = static_cast<uint32_t>(guest_output_texture->height() / 2);
          mtl4_->AddResidentAllocation(guest_output_texture);
          mtl4_->AddResidentAllocation(probe_buffer);
          mtl4_->CommitResidency();
          probe_enc->copyFromTexture(guest_output_texture, 0, 0,
                                     MTL::Origin(src_x, src_y, 0),
                                     MTL::Size(1, 1, 1), probe_buffer, 0,
                                     kProbeStride, kProbeStride);
          probe_enc->endEncoding();
          mtl4_->CommitStandaloneAndWait(probe_cmd);
          const uint8_t* b =
              reinterpret_cast<const uint8_t*>(probe_buffer->contents());
          uint32_t nonzero = 0;
          for (size_t i = 0; i < kProbeStride; ++i) {
            nonzero += b[i] ? 1u : 0u;
          }
          fprintf(stderr,
                  "[paint-probe] nonzero=%u px8=0x%02X%02X%02X%02X tex=%ux%u\n",
                  nonzero, b[0], b[1], b[2], b[3],
                  static_cast<uint32_t>(guest_output_texture->width()),
                  static_cast<uint32_t>(guest_output_texture->height()));
          fflush(stderr);
          ++paint_probe_log_count;
        } else {
          probe_cmd->release();
        }
      }
      probe_buffer->release();
    }
  }

  MTL4::CommandBuffer* cmd = mtl4_->BeginCommandBuffer();
  if (!cmd) { pool->drain(); return PaintResult::kNotPresented; }
  MTL4::ArgumentTable* paint_vertex_arg_table = nullptr;
  MTL4::ArgumentTable* paint_fragment_arg_table = nullptr;
  auto release_paint_tables = [&]() {
    if (paint_fragment_arg_table) {
      paint_fragment_arg_table->release();
      paint_fragment_arg_table = nullptr;
    }
    if (paint_vertex_arg_table) {
      paint_vertex_arg_table->release();
      paint_vertex_arg_table = nullptr;
    }
  };

  // MTL4 explicit residency: the present pass writes directly into the
  // drawable and samples the mailbox texture, so both allocations must be
  // resident for this command buffer.
  mtl4_->AddResidentAllocation(dst);
  if (guest_output_texture) {
    mtl4_->AddResidentAllocation(guest_output_texture);
  }
  mtl4_->CommitResidency();

  if (guest_output_texture && blit_lib_) {
    MTL::RenderPipelineState* pipe = GetOrCreateBlitPipeline(dst->pixelFormat());
    if (pipe) {
      MTL4::ArgumentTableDescriptor* blit_v_desc =
          MTL4::ArgumentTableDescriptor::alloc()->init();
      blit_v_desc->setMaxBufferBindCount(1);
      blit_v_desc->setMaxTextureBindCount(0);
      blit_v_desc->setMaxSamplerStateBindCount(0);
      blit_v_desc->setInitializeBindings(true);
      NS::Error* table_err = nullptr;
      paint_vertex_arg_table = device_->newArgumentTable(blit_v_desc, &table_err);
      blit_v_desc->release();
      if (!paint_vertex_arg_table) {
        REXLOG_WARN(
            "MetalPresenter: Failed to create paint vertex argument table: {}",
            table_err ? table_err->localizedDescription()->utf8String()
                      : "unknown");
      }
      MTL4::ArgumentTableDescriptor* blit_f_desc =
          MTL4::ArgumentTableDescriptor::alloc()->init();
      blit_f_desc->setMaxBufferBindCount(0);
      blit_f_desc->setMaxTextureBindCount(1);
      blit_f_desc->setMaxSamplerStateBindCount(1);
      blit_f_desc->setInitializeBindings(true);
      table_err = nullptr;
      paint_fragment_arg_table = device_->newArgumentTable(blit_f_desc, &table_err);
      blit_f_desc->release();
      if (!paint_fragment_arg_table) {
        REXLOG_WARN(
            "MetalPresenter: Failed to create paint fragment argument table: {}",
            table_err ? table_err->localizedDescription()->utf8String()
                      : "unknown");
      }
      MTL4::RenderPassDescriptor* rpd = MTL4::RenderPassDescriptor::alloc()->init();
      auto* ca = rpd->colorAttachments()->object(0);
      ca->setTexture(dst);
      ca->setLoadAction(MTL::LoadActionClear);
      ca->setClearColor(MTL::ClearColor(0, 0, 0, 1));
      ca->setStoreAction(MTL::StoreActionStore);
      MTL4::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
      if (enc) {
        enc->setRenderPipelineState(pipe);
        enc->setViewport(
            MTL::Viewport(0.0, 0.0, double(dst->width()), double(dst->height()), 0.0, 1.0));
        enc->setScissorRect(
            MTL::ScissorRect(0, 0, uint32_t(dst->width()), uint32_t(dst->height())));

        float src_h = guest_output_texture->height() > 720 ? 720.0f
                          : (float)guest_output_texture->height();
        struct {
          float w, h, src_w, src_h;
        } ub = {(float)dst->width(), (float)dst->height(),
                (float)guest_output_texture->width(), src_h};

        if (paint_vertex_arg_table && paint_fragment_arg_table) {
          paint_vertex_arg_table->setAddress(
              mtl4_->AllocInlineConstant(&ub, sizeof(ub)), 0);
          paint_fragment_arg_table->setTexture(guest_output_texture->gpuResourceID(),
                                               0);
          paint_fragment_arg_table->setSamplerState(
              GetNearestSampler()->gpuResourceID(), 0);
          enc->setArgumentTable(paint_vertex_arg_table, MTL::RenderStageVertex);
          enc->setArgumentTable(paint_fragment_arg_table,
                                MTL::RenderStageFragment);
        }

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

  if (drawable_probe_buffer) {
    MTL4::ComputeCommandEncoder* drawable_probe_enc = cmd->computeCommandEncoder();
    if (drawable_probe_enc) {
      const uint32_t probe_x = std::min<uint32_t>(
          std::max<uint32_t>(1u, uint32_t(dst->width())) - 1u,
          uint32_t(dst->width() / 2));
      const uint32_t probe_y = std::min<uint32_t>(
          std::max<uint32_t>(1u, uint32_t(dst->height())) - 1u,
          uint32_t(dst->height() / 2));
      std::memset(drawable_probe_buffer->contents(), 0, 16);
      drawable_probe_enc->copyFromTexture(
          dst, 0, 0, MTL::Origin::Make(probe_x, probe_y, 0),
          MTL::Size::Make(1, 1, 1), drawable_probe_buffer, 0, 4, 4);
      drawable_probe_enc->endEncoding();
    } else {
      drawable_probe_buffer->release();
      drawable_probe_buffer = nullptr;
    }
  }

  MTL4::CommitOptions* paint_commit_options = nullptr;
  if (paint_vertex_arg_table || paint_fragment_arg_table ||
      drawable_probe_buffer) {
    if (paint_vertex_arg_table) {
      paint_vertex_arg_table->retain();
    }
    if (paint_fragment_arg_table) {
      paint_fragment_arg_table->retain();
    }
    if (drawable_probe_buffer) {
      drawable_probe_buffer->retain();
    }
    MTL4::ArgumentTable* vertex_table_to_release = paint_vertex_arg_table;
    MTL4::ArgumentTable* fragment_table_to_release = paint_fragment_arg_table;
    MTL::Buffer* probe_buffer_to_log = drawable_probe_buffer;
    const uint32_t probe_w = uint32_t(dst->width());
    const uint32_t probe_h = uint32_t(dst->height());
    paint_commit_options = MTL4::CommitOptions::alloc()->init();
    paint_commit_options->addFeedbackHandler(
        [vertex_table_to_release, fragment_table_to_release,
         probe_buffer_to_log, probe_w, probe_h](MTL4::CommitFeedback*) {
          if (vertex_table_to_release) {
            vertex_table_to_release->release();
          }
          if (fragment_table_to_release) {
            fragment_table_to_release->release();
          }
          if (probe_buffer_to_log) {
            const uint8_t* b = reinterpret_cast<const uint8_t*>(
                probe_buffer_to_log->contents());
            if (b) {
              uint32_t nonzero = 0;
              for (size_t i = 0; i < 16; ++i) {
                nonzero += b[i] ? 1u : 0u;
              }
              fprintf(stderr,
                      "[drawable-probe] nonzero=%u px8=0x%02X%02X%02X%02X tex=%ux%u\n",
                      nonzero, b[0], b[1], b[2], b[3], probe_w, probe_h);
              fflush(stderr);
            }
            probe_buffer_to_log->release();
          }
        });
  }
  mtl4_->WaitDrawable(drawable);
  if (paint_commit_options) {
    mtl4_->Commit(cmd, paint_commit_options);
    paint_commit_options->release();
  } else {
    mtl4_->Commit(cmd);
  }
  mtl4_->SignalDrawable(drawable);
  drawable->present();
  if (drawable_probe_buffer) {
    drawable_probe_buffer->release();
    drawable_probe_buffer = nullptr;
    ++drawable_probe_log_count;
  }
  release_paint_tables();

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
