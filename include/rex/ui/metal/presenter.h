#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <rex/ui/presenter.h>

namespace rex::graphics::metal {
class Metal4Context;
}  // namespace rex::graphics::metal

namespace rex::ui::metal {

class MetalProvider;

class MetalGuestOutputRefreshContext : public Presenter::GuestOutputRefreshContext {
 public:
  MetalGuestOutputRefreshContext(bool& is_8bpc_out_ref,
                                MTL::Texture* resource)
      : GuestOutputRefreshContext(is_8bpc_out_ref), resource_(resource) {}

  MTL::Texture* resource_uav_capable() const { return resource_; }

  void SetSubmissionId(uint64_t id) { submission_id_ = id; }
  uint64_t submission_id() const { return submission_id_; }

 private:
  MTL::Texture* resource_;
  uint64_t submission_id_ = 0;
};

class MetalPresenter : public Presenter {
 public:
  MetalPresenter(MetalProvider* provider,
                 HostGpuLossCallback host_gpu_loss_callback);
  ~MetalPresenter() override;

  bool Initialize();
  void Shutdown();

  Surface::TypeFlags GetSupportedSurfaceTypes() const override;

  bool CaptureGuestOutput(RawImage& image_out) override;

  bool CopyTextureToGuestOutput(MTL::Texture* source_texture,
                                MTL::Texture* dest_texture,
                                uint32_t source_width, uint32_t source_height,
                                bool force_swap_rb, bool use_pwl_gamma_ramp,
                                uint64_t* submission_out);

  bool UpdateGammaRamp(const void* table_data, size_t table_bytes,
                       const void* pwl_data, size_t pwl_bytes);

 protected:
  SurfacePaintConnectResult ConnectOrReconnectPaintingToSurfaceFromUIThread(
      Surface& new_surface, uint32_t new_surface_width,
      uint32_t new_surface_height, bool was_paintable,
      bool& is_vsync_implicit_out) override;

  void DisconnectPaintingFromSurfaceFromUIThreadImpl() override;

  bool RefreshGuestOutputImpl(
      uint32_t mailbox_index, uint32_t frontbuffer_width,
      uint32_t frontbuffer_height,
      std::function<bool(GuestOutputRefreshContext& context)> refresher,
      bool& is_8bpc_out_ref) override;

  PaintResult PaintAndPresentImpl(bool execute_ui_drawers) override;

  bool WantsContinuousUIPaintFromUIThread() const override;

 private:
  MTL::RenderPipelineState* GetOrCreateBlitPipeline(MTL::PixelFormat fmt);
  MTL::SamplerState* GetNearestSampler();
  bool EnsureTestCubeResources(MTL::PixelFormat color_format, uint32_t width,
                               uint32_t height);
  void ReleaseTestCubeResources();
  void PaintTestCube(MTL4::CommandBuffer* cmd, MTL::Texture* dst);
#if REX_PLATFORM_MAC
  MTL::RenderPipelineState* GetOrCreateDebugTriPipeline(MTL::PixelFormat fmt);
  Presenter::PaintResult PaintAndPresentViaMTL3(CA::MetalDrawable* drawable,
                                                MTL::Texture* dst);
#endif

  MetalProvider* provider_ = nullptr;
  MTL::Device* device_ = nullptr;
  graphics::metal::Metal4Context* mtl4_ = nullptr;

  void* metal_layer_ = nullptr;

  MTL::Library* blit_lib_ = nullptr;
  MTL::RenderPipelineState* blit_pipe_ = nullptr;
  MTL::PixelFormat blit_pipe_fmt_ = MTL::PixelFormatInvalid;
  MTL::SamplerState* nearest_sampler_ = nullptr;

#if REX_PLATFORM_MAC
  MTL::CommandQueue* ui_present_queue_ = nullptr;
  MTL::Library* debug_tri_lib_ = nullptr;
  MTL::RenderPipelineState* debug_tri_pipe_ = nullptr;
  MTL::PixelFormat debug_tri_pipe_fmt_ = MTL::PixelFormatInvalid;
#endif

  MTL::Library* test_cube_lib_ = nullptr;
  MTL::RenderPipelineState* test_cube_pipe_ = nullptr;
  MTL::DepthStencilState* test_cube_depth_state_ = nullptr;
  MTL::Buffer* test_cube_vertex_buffer_ = nullptr;
  MTL::Buffer* test_cube_index_buffer_ = nullptr;
  MTL::Texture* test_cube_depth_texture_ = nullptr;
  MTL::PixelFormat test_cube_pipe_fmt_ = MTL::PixelFormatInvalid;
  uint32_t test_cube_depth_width_ = 0;
  uint32_t test_cube_depth_height_ = 0;

  std::array<MTL::Texture*, kGuestOutputMailboxSize> guest_output_textures_;
  std::array<uint64_t, kGuestOutputMailboxSize> guest_output_submissions_;
  std::atomic<uint32_t> last_guest_output_mailbox_index_{0};
  std::atomic<uint64_t> guest_output_submission_counter_{0};
  MTL::SharedEvent* guest_output_shared_event_ = nullptr;

  MTL::Buffer* gamma_ramp_buffer_ = nullptr;
  uint32_t gamma_ramp_buffer_size_ = 0;
  MTL::Texture* gamma_ramp_table_texture_ = nullptr;
  MTL::Texture* gamma_ramp_pwl_texture_ = nullptr;
  bool gamma_ramp_table_valid_ = false;
  bool gamma_ramp_pwl_valid_ = false;
};

}  // namespace rex::ui::metal
