#include <rex/ui/metal/presenter.h>
#include <rex/ui/metal/provider.h>
#include <rex/ui/surface_mac.h>
#include <rex/logging/macros.h>
#include <rex/cvar.h>
#include <rex/graphics/metal/metal4_context.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <chrono>
#include <cmath>
#include <cstring>

namespace {

struct TestCubeVertex {
  float position[3];
  float color[3];
};

struct TestCubeUniforms {
  float mvp[16];
};

constexpr TestCubeVertex kTestCubeVertices[] = {
    {{-0.5f, -0.5f, 0.5f}, {1.f, 0.f, 0.f}},
    {{0.5f, -0.5f, 0.5f}, {1.f, 0.5f, 0.f}},
    {{0.5f, 0.5f, 0.5f}, {0.f, 1.f, 0.f}},
    {{-0.5f, 0.5f, 0.5f}, {0.f, 1.f, 1.f}},
    {{-0.5f, -0.5f, -0.5f}, {1.f, 0.f, 1.f}},
    {{0.5f, -0.5f, -0.5f}, {0.f, 0.f, 1.f}},
    {{0.5f, 0.5f, -0.5f}, {0.5f, 0.f, 1.f}},
    {{-0.5f, 0.5f, -0.5f}, {1.f, 1.f, 0.f}},
};

constexpr uint16_t kTestCubeIndices[] = {
    0, 1, 2, 2, 3, 0,  // front
    1, 5, 6, 6, 2, 1,  // right
    5, 4, 7, 7, 6, 5,  // back
    4, 0, 3, 3, 7, 4,  // left
    3, 2, 6, 6, 7, 3,  // top
    4, 5, 1, 1, 0, 4,  // bottom
};

void MatrixIdentity(float m[16]) {
  std::memset(m, 0, sizeof(float) * 16);
  m[0] = m[5] = m[10] = m[15] = 1.f;
}

void MatrixMultiply(float out[16], const float a[16], const float b[16]) {
  float r[16];
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      r[col * 4 + row] = a[row] * b[col * 4 + 0] + a[4 + row] * b[col * 4 + 1] +
                         a[8 + row] * b[col * 4 + 2] + a[12 + row] * b[col * 4 + 3];
    }
  }
  std::memcpy(out, r, sizeof(r));
}

void MatrixPerspective(float m[16], float fov_y_radians, float aspect, float z_near,
                       float z_far) {
  MatrixIdentity(m);
  const float f = 1.f / std::tan(fov_y_radians * 0.5f);
  m[0] = f / aspect;
  m[5] = f;
  m[10] = z_far / (z_far - z_near);
  m[11] = 1.f;
  m[14] = (-z_far * z_near) / (z_far - z_near);
  m[15] = 0.f;
}

void MatrixRotationY(float m[16], float angle_radians) {
  MatrixIdentity(m);
  const float c = std::cos(angle_radians);
  const float s = std::sin(angle_radians);
  m[0] = c;
  m[2] = s;
  m[8] = -s;
  m[10] = c;
}

void MatrixRotationX(float m[16], float angle_radians) {
  MatrixIdentity(m);
  const float c = std::cos(angle_radians);
  const float s = std::sin(angle_radians);
  m[5] = c;
  m[6] = -s;
  m[9] = s;
  m[10] = c;
}

void MatrixTranslation(float m[16], float x, float y, float z) {
  MatrixIdentity(m);
  m[12] = x;
  m[13] = y;
  m[14] = z;
}

}  // namespace

#include "shaders/blit_metallib.h"
#include "shaders/test_cube_metallib.h"
#if REX_PLATFORM_MAC
#include "shaders/debug_tri_metallib.h"
#endif

REXCVAR_DEFINE_BOOL(metal_hud, true, "GPU", "Enable Metal performance HUD overlay");
REXCVAR_DEFINE_BOOL(metal_frame_timing, true, "GPU", "Log GPU frame timing info");
REXCVAR_DEFINE_BOOL(metal_present_probe, false, "GPU",
                    "Log Metal presenter guest-output mailbox and present path bring-up");
REXCVAR_DEFINE_BOOL(metal_test_cube, true, "GPU",
                    "Draw a spinning rainbow cube when no guest frame is available");

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

  if (!InitializeCommonSurfaceIndependent()) {
    REXLOG_ERROR("MetalPresenter: Failed to initialize common presenter state");
    return false;
  }

#if REX_PLATFORM_MAC
  ui_present_queue_ = device_->newCommandQueue();
  if (!ui_present_queue_) {
    REXLOG_ERROR("MetalPresenter: Failed to create UI present command queue");
    return false;
  }
  dispatch_data_t debug_lib_data = dispatch_data_create(
      debug_tri_metallib, debug_tri_metallib_len, dispatch_get_main_queue(),
      DISPATCH_DATA_DESTRUCTOR_DEFAULT);
  err = nullptr;
  debug_tri_lib_ = device_->newLibrary(debug_lib_data, &err);
  dispatch_release(debug_lib_data);
  if (debug_tri_lib_) {
    REXLOG_INFO("MetalPresenter: Debug triangle shader loaded (MTL3 present)");
  } else {
    REXLOG_WARN("MetalPresenter: Debug triangle shader unavailable: {}",
                err ? err->localizedDescription()->utf8String() : "unknown");
  }
#endif

  REXLOG_INFO("MetalPresenter: Initialized (MTL4) on device {}", device_->name()->utf8String());
  return true;
}

void MetalPresenter::Shutdown() {
  if (nearest_sampler_) {
    nearest_sampler_->release();
    nearest_sampler_ = nullptr;
  }
  if (blit_pipe_) {
    blit_pipe_->release();
    blit_pipe_ = nullptr;
    blit_pipe_fmt_ = MTL::PixelFormatInvalid;
  }
  if (blit_lib_) {
    blit_lib_->release();
    blit_lib_ = nullptr;
  }
  if (gamma_ramp_pwl_texture_) { gamma_ramp_pwl_texture_->release(); gamma_ramp_pwl_texture_ = nullptr; }
  if (gamma_ramp_table_texture_) { gamma_ramp_table_texture_->release(); gamma_ramp_table_texture_ = nullptr; }
  if (gamma_ramp_buffer_) { gamma_ramp_buffer_->release(); gamma_ramp_buffer_ = nullptr; }
  if (guest_output_shared_event_) { guest_output_shared_event_->release(); guest_output_shared_event_ = nullptr; }
  ReleaseTestCubeResources();
#if REX_PLATFORM_MAC
  if (debug_tri_pipe_) {
    debug_tri_pipe_->release();
    debug_tri_pipe_ = nullptr;
    debug_tri_pipe_fmt_ = MTL::PixelFormatInvalid;
  }
  if (debug_tri_lib_) {
    debug_tri_lib_->release();
    debug_tri_lib_ = nullptr;
  }
  if (ui_present_queue_) {
    ui_present_queue_->release();
    ui_present_queue_ = nullptr;
  }
#endif
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
  if (!source_texture || !dest_texture || !mtl4_) {
    return false;
  }

  MTL4::CommandBuffer* cmd = mtl4_->BeginCommandBuffer();
  if (!cmd) return false;

  MTL4::ComputeCommandEncoder* compute = cmd->computeCommandEncoder();
  if (!compute) {
    mtl4_->Commit(cmd);
    return false;
  }

  compute->copyFromTexture(source_texture, 0, 0,
                            MTL::Origin(0, 0, 0),
                            MTL::Size(source_width, source_height, 1),
                            dest_texture, 0, 0,
                            MTL::Origin(0, 0, 0));
  compute->endEncoding();

  uint64_t submission_id = 0;
  if (guest_output_shared_event_) {
    submission_id = guest_output_submission_counter_.fetch_add(1,
                    std::memory_order_relaxed) + 1;
  }

  mtl4_->Commit(cmd);

  if (submission_id && guest_output_shared_event_) {
    mtl4_->SignalEvent(guest_output_shared_event_, submission_id);
  }

  if (submission_out) {
    *submission_out = submission_id;
  }
  return true;
}

Presenter::PaintResult MetalPresenter::PaintAndPresentImpl(
    bool execute_ui_drawers) {
  (void)execute_ui_drawers;
  if (!metal_layer_ || !device_ || !mtl4_) {
    static uint32_t missing_probe = 0;
    if (missing_probe++ < 8) {
      std::fprintf(stderr,
                   "[metal-present] skip: layer=%p device=%p mtl4=%p\n",
                   metal_layer_, static_cast<void*>(device_),
                   static_cast<void*>(mtl4_));
    }
    return PaintResult::kNotPresented;
  }

  auto* pool = NS::AutoreleasePool::alloc()->init();
  CA::MetalLayer* layer = reinterpret_cast<CA::MetalLayer*>(metal_layer_);
  CA::MetalDrawable* drawable = layer->nextDrawable();
  if (!drawable) {
    static uint32_t drawable_probe = 0;
    if (drawable_probe++ < 8) {
      std::fprintf(stderr,
                   "[metal-present] nextDrawable() returned null (size=%gx%g)\n",
                   layer->drawableSize().width, layer->drawableSize().height);
    }
    pool->drain();
    return PaintResult::kNotPresented;
  }

  MTL::Texture* dst = drawable->texture();

#if REX_PLATFORM_MAC
  if (REXCVAR_GET(metal_test_cube) && ui_present_queue_) {
    Presenter::PaintResult result = PaintAndPresentViaMTL3(drawable, dst);
    pool->drain();
    return result;
  }
#endif

  uint32_t mailbox_index = last_guest_output_mailbox_index_.load(
      std::memory_order_relaxed);
  MTL::Texture* guest_output_texture = nullptr;
  if (mailbox_index < guest_output_textures_.size()) {
    guest_output_texture = guest_output_textures_[mailbox_index];
  }
  if (REXCVAR_GET(metal_present_probe)) {
    static uint32_t present_probe_count = 0;
    if (present_probe_count < 16) {
      ++present_probe_count;
      REXLOG_WARN(
          "[present-probe] mailbox={} guest_tex={} guest={}x{} dst={}x{}",
          mailbox_index, fmt::ptr(guest_output_texture),
          guest_output_texture ? uint32_t(guest_output_texture->width()) : 0u,
          guest_output_texture ? uint32_t(guest_output_texture->height()) : 0u,
          uint32_t(dst->width()), uint32_t(dst->height()));
    }
  }

  MTL4::CommandBuffer* cmd = mtl4_->BeginCommandBuffer();
  if (!cmd) { pool->drain(); return PaintResult::kNotPresented; }

  if (REXCVAR_GET(metal_test_cube)) {
    PaintTestCube(cmd, dst);
  } else if (guest_output_texture && blit_lib_) {
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
    MTL4::RenderPassDescriptor* rpd = MTL4::RenderPassDescriptor::alloc()->init();
    auto* ca = rpd->colorAttachments()->object(0);
    ca->setTexture(dst);
    ca->setLoadAction(MTL::LoadActionClear);
    ca->setClearColor(MTL::ClearColor(0.05, 0.05, 0.08, 1));
    ca->setStoreAction(MTL::StoreActionStore);
    MTL4::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
    if (enc) {
      enc->endEncoding();
    }
    rpd->release();
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

bool MetalPresenter::WantsContinuousUIPaintFromUIThread() const {
  return REXCVAR_GET(metal_test_cube);
}

void MetalPresenter::ReleaseTestCubeResources() {
  if (test_cube_depth_texture_) {
    test_cube_depth_texture_->release();
    test_cube_depth_texture_ = nullptr;
  }
  if (test_cube_index_buffer_) {
    test_cube_index_buffer_->release();
    test_cube_index_buffer_ = nullptr;
  }
  if (test_cube_vertex_buffer_) {
    test_cube_vertex_buffer_->release();
    test_cube_vertex_buffer_ = nullptr;
  }
  if (test_cube_depth_state_) {
    test_cube_depth_state_->release();
    test_cube_depth_state_ = nullptr;
  }
  if (test_cube_pipe_) {
    test_cube_pipe_->release();
    test_cube_pipe_ = nullptr;
    test_cube_pipe_fmt_ = MTL::PixelFormatInvalid;
  }
  if (test_cube_lib_) {
    test_cube_lib_->release();
    test_cube_lib_ = nullptr;
  }
  test_cube_depth_width_ = 0;
  test_cube_depth_height_ = 0;
}

bool MetalPresenter::EnsureTestCubeResources(MTL::PixelFormat color_format,
                                             uint32_t width, uint32_t height) {
  if (!device_) {
    return false;
  }

  if (!test_cube_lib_) {
    dispatch_data_t lib_data = dispatch_data_create(
        test_cube_metallib, test_cube_metallib_len, dispatch_get_main_queue(),
        DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    NS::Error* err = nullptr;
    test_cube_lib_ = device_->newLibrary(lib_data, &err);
    dispatch_release(lib_data);
    if (!test_cube_lib_) {
      REXLOG_ERROR("MetalPresenter: Failed to load test cube shader: {}",
                   err ? err->localizedDescription()->utf8String() : "unknown");
      return false;
    }
    REXLOG_INFO("MetalPresenter: Test cube shader loaded");
  }

  if (!test_cube_vertex_buffer_) {
    test_cube_vertex_buffer_ = device_->newBuffer(
        sizeof(kTestCubeVertices), MTL::ResourceStorageModeShared);
    if (!test_cube_vertex_buffer_) {
      return false;
    }
    std::memcpy(test_cube_vertex_buffer_->contents(), kTestCubeVertices,
                sizeof(kTestCubeVertices));
  }

  if (!test_cube_index_buffer_) {
    test_cube_index_buffer_ = device_->newBuffer(
        sizeof(kTestCubeIndices), MTL::ResourceStorageModeShared);
    if (!test_cube_index_buffer_) {
      return false;
    }
    std::memcpy(test_cube_index_buffer_->contents(), kTestCubeIndices,
                sizeof(kTestCubeIndices));
  }

  if (!test_cube_depth_state_) {
    auto* dsd = MTL::DepthStencilDescriptor::alloc()->init();
    dsd->setDepthCompareFunction(MTL::CompareFunctionLess);
    dsd->setDepthWriteEnabled(true);
    test_cube_depth_state_ = device_->newDepthStencilState(dsd);
    dsd->release();
    if (!test_cube_depth_state_) {
      return false;
    }
  }

  if (!test_cube_pipe_ || test_cube_pipe_fmt_ != color_format) {
    if (test_cube_pipe_) {
      test_cube_pipe_->release();
      test_cube_pipe_ = nullptr;
    }
    auto* vf = test_cube_lib_->newFunction(
        NS::String::string("test_cube_vs", NS::UTF8StringEncoding));
    auto* ff = test_cube_lib_->newFunction(
        NS::String::string("test_cube_fs", NS::UTF8StringEncoding));
    auto* vd = MTL::VertexDescriptor::alloc()->init();
    vd->attributes()->object(0)->setFormat(MTL::VertexFormatFloat3);
    vd->attributes()->object(0)->setOffset(0);
    vd->attributes()->object(0)->setBufferIndex(1);
    vd->attributes()->object(1)->setFormat(MTL::VertexFormatFloat3);
    vd->attributes()->object(1)->setOffset(12);
    vd->attributes()->object(1)->setBufferIndex(1);
    vd->layouts()->object(1)->setStride(sizeof(TestCubeVertex));
    vd->layouts()->object(1)->setStepFunction(MTL::VertexStepFunctionPerVertex);
    auto* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(vf);
    desc->setFragmentFunction(ff);
    desc->setVertexDescriptor(vd);
    desc->colorAttachments()->object(0)->setPixelFormat(color_format);
    desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    NS::Error* err = nullptr;
    test_cube_pipe_ =
        device_->newRenderPipelineState(desc, MTL::PipelineOptionNone, nullptr, &err);
    if (test_cube_pipe_) {
      test_cube_pipe_fmt_ = color_format;
      REXLOG_INFO("MetalPresenter: Test cube pipeline created fmt={}", (int)color_format);
    } else {
      REXLOG_ERROR("MetalPresenter: Test cube pipeline failed: {}",
                   err ? err->localizedDescription()->utf8String() : "null");
    }
    desc->release();
    vd->release();
    if (vf) vf->release();
    if (ff) ff->release();
    if (!test_cube_pipe_) {
      return false;
    }
  }

  if (!test_cube_depth_texture_ || test_cube_depth_width_ != width ||
      test_cube_depth_height_ != height) {
    if (test_cube_depth_texture_) {
      test_cube_depth_texture_->release();
      test_cube_depth_texture_ = nullptr;
    }
    auto* depth_desc = MTL::TextureDescriptor::alloc()->init();
    depth_desc->setTextureType(MTL::TextureType2D);
    depth_desc->setPixelFormat(MTL::PixelFormatDepth32Float);
    depth_desc->setWidth(width);
    depth_desc->setHeight(height);
    depth_desc->setUsage(MTL::TextureUsageRenderTarget);
    depth_desc->setStorageMode(MTL::StorageModePrivate);
    test_cube_depth_texture_ = device_->newTexture(depth_desc);
    depth_desc->release();
    if (!test_cube_depth_texture_) {
      return false;
    }
    test_cube_depth_width_ = width;
    test_cube_depth_height_ = height;
  }

  return true;
}

void MetalPresenter::PaintTestCube(MTL4::CommandBuffer* cmd, MTL::Texture* dst) {
  if (!cmd || !dst || !mtl4_) {
    return;
  }

  const uint32_t width = uint32_t(dst->width());
  const uint32_t height = uint32_t(dst->height());
  if (!EnsureTestCubeResources(dst->pixelFormat(), width, height) || !test_cube_pipe_) {
    return;
  }

  using clock = std::chrono::steady_clock;
  static const clock::time_point start_time = clock::now();
  const float elapsed =
      std::chrono::duration<float>(clock::now() - start_time).count();

  float model[16];
  float rot_y[16];
  float rot_x[16];
  MatrixRotationY(rot_y, elapsed * 1.1f);
  MatrixRotationX(rot_x, elapsed * 0.7f);
  MatrixMultiply(model, rot_y, rot_x);

  float view[16];
  MatrixTranslation(view, 0.f, 0.f, -2.5f);

  float proj[16];
  const float aspect = height ? float(width) / float(height) : 1.f;
  MatrixPerspective(proj, 0.78539816339f, aspect, 0.1f, 100.f);

  float view_model[16];
  float mvp[16];
  MatrixMultiply(view_model, view, model);
  MatrixMultiply(mvp, proj, view_model);

  TestCubeUniforms uniforms;
  std::memcpy(uniforms.mvp, mvp, sizeof(mvp));

  MTL::RenderPassDescriptor* mtl3_rpd = MTL::RenderPassDescriptor::alloc()->init();
  auto* color_attachment = mtl3_rpd->colorAttachments()->object(0);
  color_attachment->setTexture(dst);
  color_attachment->setLoadAction(MTL::LoadActionClear);
  color_attachment->setClearColor(MTL::ClearColor(0.05, 0.05, 0.08, 1));
  color_attachment->setStoreAction(MTL::StoreActionStore);

  MTL::RenderPassDepthAttachmentDescriptor* depth_desc =
      MTL::RenderPassDepthAttachmentDescriptor::alloc()->init();
  depth_desc->setTexture(test_cube_depth_texture_);
  depth_desc->setLoadAction(MTL::LoadActionClear);
  depth_desc->setClearDepth(1.0);
  depth_desc->setStoreAction(MTL::StoreActionDontCare);
  mtl3_rpd->setDepthAttachment(depth_desc);

  MTL4::RenderPassDescriptor* rpd = mtl4_->CreateRenderPassDescriptor(mtl3_rpd);
  mtl3_rpd->release();
  depth_desc->release();
  if (!rpd) {
    return;
  }

  MTL4::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
  rpd->release();
  if (!enc) {
    return;
  }

  enc->setRenderPipelineState(test_cube_pipe_);
  enc->setDepthStencilState(test_cube_depth_state_);
  enc->setCullMode(MTL::CullModeBack);
  enc->setFrontFacingWinding(MTL::WindingCounterClockwise);

  MTL::Viewport viewport = {0.0, 0.0, double(width), double(height), 0.0, 1.0};
  enc->setViewport(viewport);

  mtl4_->SetVertexAddress(mtl4_->AllocInlineConstant(&uniforms, sizeof(uniforms)), 0);
  mtl4_->SetVertexAddress(test_cube_vertex_buffer_->gpuAddress(), 1);
  mtl4_->FlushRenderBindings(enc);

  enc->drawIndexedPrimitives(
      MTL::PrimitiveTypeTriangle, NS::UInteger(std::size(kTestCubeIndices)),
      MTL::IndexTypeUInt16, test_cube_index_buffer_->gpuAddress(),
      NS::UInteger(sizeof(kTestCubeIndices)));
  enc->endEncoding();
}

#if REX_PLATFORM_MAC
MTL::RenderPipelineState* MetalPresenter::GetOrCreateDebugTriPipeline(
    MTL::PixelFormat fmt) {
  if (debug_tri_pipe_ && debug_tri_pipe_fmt_ == fmt) {
    return debug_tri_pipe_;
  }
  if (!debug_tri_lib_) {
    return nullptr;
  }
  if (debug_tri_pipe_) {
    debug_tri_pipe_->release();
    debug_tri_pipe_ = nullptr;
  }
  auto* vf = debug_tri_lib_->newFunction(
      NS::String::string("debug_tri_vs", NS::UTF8StringEncoding));
  auto* ff = debug_tri_lib_->newFunction(
      NS::String::string("debug_tri_fs", NS::UTF8StringEncoding));
  auto* desc = MTL::RenderPipelineDescriptor::alloc()->init();
  desc->setVertexFunction(vf);
  desc->setFragmentFunction(ff);
  desc->colorAttachments()->object(0)->setPixelFormat(fmt);
  NS::Error* err = nullptr;
  debug_tri_pipe_ =
      device_->newRenderPipelineState(desc, MTL::PipelineOptionNone, nullptr, &err);
  if (debug_tri_pipe_) {
    debug_tri_pipe_fmt_ = fmt;
    REXLOG_INFO("MetalPresenter: Debug tri pipeline created fmt={}", (int)fmt);
  } else {
    REXLOG_ERROR("MetalPresenter: Debug tri pipeline failed: {}",
                 err ? err->localizedDescription()->utf8String() : "null");
  }
  desc->release();
  if (vf) vf->release();
  if (ff) ff->release();
  return debug_tri_pipe_;
}

Presenter::PaintResult MetalPresenter::PaintAndPresentViaMTL3(
    CA::MetalDrawable* drawable, MTL::Texture* dst) {
  if (!drawable || !dst || !ui_present_queue_) {
    return PaintResult::kNotPresented;
  }

  MTL::RenderPipelineState* pipe = GetOrCreateDebugTriPipeline(dst->pixelFormat());
  if (!pipe) {
    static uint32_t pipe_probe = 0;
    if (pipe_probe++ < 8) {
      std::fprintf(stderr, "[metal-present] MTL3: no debug_tri pipeline\n");
    }
    return PaintResult::kNotPresented;
  }

  MTL::CommandBuffer* cmd = ui_present_queue_->commandBuffer();
  if (!cmd) {
    return PaintResult::kNotPresented;
  }

  MTL::RenderPassDescriptor* rpd = MTL::RenderPassDescriptor::alloc()->init();
  auto* ca = rpd->colorAttachments()->object(0);
  ca->setTexture(dst);
  ca->setLoadAction(MTL::LoadActionClear);
  ca->setClearColor(MTL::ClearColor(0.08, 0.08, 0.12, 1));
  ca->setStoreAction(MTL::StoreActionStore);

  MTL::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
  rpd->release();
  if (!enc) {
    cmd->release();
    static uint32_t enc_probe = 0;
    if (enc_probe++ < 8) {
      std::fprintf(stderr, "[metal-present] MTL3: render encoder null\n");
    }
    return PaintResult::kNotPresented;
  }

  enc->setRenderPipelineState(pipe);
  enc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));
  enc->endEncoding();

  cmd->presentDrawable(reinterpret_cast<MTL::Drawable*>(drawable));
  cmd->commit();
  cmd->release();

  static uint32_t ok_probe = 0;
  if (ok_probe++ < 4) {
    std::fprintf(stderr,
                 "[metal-present] MTL3: presented debug tri %ux%u\n",
                 uint32_t(dst->width()), uint32_t(dst->height()));
  }

  return PaintResult::kPresented;
}
#endif

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
  layer->setDisplaySyncEnabled(true);
  layer->setFramebufferOnly(true);

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
  if (REXCVAR_GET(metal_present_probe)) {
    static uint32_t refresh_probe_count = 0;
    if (refresh_probe_count < 16) {
      ++refresh_probe_count;
      REXLOG_WARN(
          "[refresh-probe] mailbox={} success=1 guest_tex={} guest={}x{} submission={}",
          mailbox_index, fmt::ptr(guest_output_texture), frontbuffer_width,
          frontbuffer_height, context.submission_id());
    }
  }
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
