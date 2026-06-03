#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include <Metal/Metal.hpp>
#include <rex/cvar.h>
#include <rex/graphics/flags.h>
#include <rex/graphics/register_file.h>
#include <rex/graphics/trace_writer.h>
#include <rex/graphics/xenos.h>
#include <rex/graphics/pipeline/render_target/cache.h>
#define IR_RUNTIME_METALCPP
#include <metal_irconverter_runtime.h>

namespace rex::memory {
class Memory;
}  // namespace rex::memory

namespace rex::graphics::metal {

class MetalCommandProcessor;
class MetalHeapPool;

class MetalRenderTargetCache : public RenderTargetCache {
 public:
  class MetalRenderTarget : public RenderTarget {
   public:
    MetalRenderTarget(RenderTargetKey key) : RenderTarget(key) {}
    virtual ~MetalRenderTarget();

    MTL::Texture* texture() const { return texture_; }
    MTL::Texture* msaa_texture() const { return msaa_texture_; }
    MTL::Texture* draw_texture() const { return draw_texture_; }
    MTL::Texture* transfer_texture() const { return transfer_texture_; }
    MTL::Texture* msaa_draw_texture() const { return msaa_draw_texture_; }
    MTL::Texture* msaa_transfer_texture() const { return msaa_transfer_texture_; }
    MTL::Texture* stencil_view() const { return stencil_view_; }

    void SetTexture(MTL::Texture* tex) { texture_ = tex; }
    void SetDrawTexture(MTL::Texture* tex) { draw_texture_ = tex; }
    void SetTransferTexture(MTL::Texture* tex) { transfer_texture_ = tex; }
    void SetMsaaDrawTexture(MTL::Texture* tex) { msaa_draw_texture_ = tex; }
    void SetMsaaTransferTexture(MTL::Texture* tex) { msaa_transfer_texture_ = tex; }
    void SetStencilView(MTL::Texture* view) { stencil_view_ = view; }
    void SetNeedsInitialClear(bool v) { needs_initial_clear_ = v; }
    bool needs_initial_clear() const { return needs_initial_clear_; }

    uint32_t temporary_sort_index() const { return temporary_sort_index_; }
    void SetTemporarySortIndex(uint32_t index) { temporary_sort_index_ = index; }

   private:
    MTL::Texture* texture_ = nullptr;
    MTL::Texture* msaa_texture_ = nullptr;
    MTL::Texture* draw_texture_ = nullptr;
    MTL::Texture* transfer_texture_ = nullptr;
    MTL::Texture* msaa_draw_texture_ = nullptr;
    MTL::Texture* msaa_transfer_texture_ = nullptr;
    MTL::Texture* stencil_view_ = nullptr;
    bool needs_initial_clear_ = false;
    uint32_t temporary_sort_index_ = 0;
  };

  MetalRenderTargetCache(const RegisterFile& register_file,
                         const rex::memory::Memory& memory, TraceWriter* trace_writer,
                         uint32_t draw_resolution_scale_x,
                         uint32_t draw_resolution_scale_y,
                         MetalCommandProcessor& command_processor);
  ~MetalRenderTargetCache();

  bool Initialize();
  void Shutdown(bool from_destructor = false);
  void ClearCache();
  void BeginFrame();

  Path GetPath() const override;
  bool IsGammaFormatHostStorageSeparate() const override;
  uint32_t GetMaxRenderTargetWidth() const override;
  uint32_t GetMaxRenderTargetHeight() const override;
  RenderTarget* CreateRenderTarget(RenderTargetKey key) override;
  bool IsHostDepthEncodingDifferent(xenos::DepthRenderTargetFormat format) const override;

  bool Update(bool is_rasterization_done,
              reg::RB_DEPTHCONTROL normalized_depth_control,
              uint32_t normalized_color_mask, const Shader& vertex_shader);

  void RestoreEdramSnapshot(const void* snapshot);

  bool Resolve(rex::memory::Memory& memory, uint32_t& written_address,
               uint32_t& written_length, MTL4::CommandBuffer* command_buffer);

  MTL::Texture* GetColorTarget(uint32_t index) const;
  MTL::Texture* GetDepthTarget() const;
  MTL::Texture* GetDummyColorTarget() const;
  MTL::Texture* GetColorTargetForDraw(uint32_t index) const;
  MTL::Texture* GetDummyColorTargetForDraw() const;
  MTL::Texture* GetDepthTargetForDraw() const;

  MTL::RenderPassDescriptor* GetRenderPassDescriptor(uint32_t sample_count);

  bool IsKey64bpp(RenderTargetKey key) const;
  static uint32_t GetMetalEdramDumpFormat(RenderTargetKey key);
  bool gamma_render_target_as_unorm16() const { return gamma_render_target_as_unorm16_; }
  bool msaa_2x_supported() const { return msaa_2x_supported_; }

  bool IsFixedRG16TruncatedToMinus1To1() const {
    return !REXCVAR_GET(snorm16_render_target_full_range);
  }
  bool IsFixedRGBA16TruncatedToMinus1To1() const {
    return !REXCVAR_GET(snorm16_render_target_full_range);
  }

  bool WriteEdramUintPow2BindlessDescriptor(IRDescriptorTableEntry* entry,
                                            uint32_t element_size_bytes_pow2) const;
  void UseBindlessResources(MetalCommandProcessor& command_processor,
                            MTL::ResourceUsage usage) const;

 protected:
  MTL::Texture* CreateColorTexture(uint32_t width, uint32_t height,
                                    xenos::ColorRenderTargetFormat format,
                                    uint32_t samples,
                                    bool transient_render_target_only = false,
                                    bool allow_unpooled_fallback = false);
  MTL::Texture* CreateDepthTexture(uint32_t width, uint32_t height,
                                    xenos::DepthRenderTargetFormat format,
                                    uint32_t samples);

 private:
  bool InitializeEdramBufferViews();
  void ReleaseEdramBufferViews();
  MTL::Texture* GetEdramUintPow2BufferView(
      uint32_t element_size_bytes_pow2) const;

  bool InitializeEdramComputeShaders();
  void ShutdownEdramComputeShaders();

  MTL::PixelFormat GetColorResourcePixelFormat(
      xenos::ColorRenderTargetFormat format) const;
  MTL::PixelFormat GetColorDrawPixelFormat(
      xenos::ColorRenderTargetFormat format) const;
  MTL::PixelFormat GetColorOwnershipTransferPixelFormat(
      xenos::ColorRenderTargetFormat format,
      bool* is_integer_out = nullptr) const;
  MTL::PixelFormat GetDepthPixelFormat(
      xenos::DepthRenderTargetFormat format) const;
  MTL::Texture* GetStencilTextureView(MetalRenderTarget* render_target);

  MetalRenderTarget* GetColorRenderTarget(uint32_t index) const;
  MTL::Texture* GetLastRealColorTarget(uint32_t index) const;
  MTL::Texture* GetLastRealDepthTarget() const;
  MTL::Texture* GetRenderTargetTexture(RenderTargetKey key) const;
  MTL::Texture* GetColorRenderTargetTexture(
      uint32_t pitch, xenos::MsaaSamples samples, uint32_t base,
      xenos::ColorRenderTargetFormat format) const;

  void StoreTiledData(MTL4::CommandBuffer* command_buffer,
                      MTL::Texture* texture, uint32_t edram_base,
                      uint32_t pitch_tiles, uint32_t height_tiles,
                      bool is_depth);

  void DumpRenderTargets(uint32_t dump_base, uint32_t dump_row_length_used,
                          uint32_t dump_rows, uint32_t dump_pitch,
                          MTL4::CommandBuffer* command_buffer = nullptr);

  void PerformTransfersAndResolveClears(
      uint32_t render_target_count, RenderTarget* const* render_targets,
      const std::vector<Transfer>* render_target_transfers,
      const uint64_t* render_target_resolve_clear_values = nullptr,
      const Transfer::Rectangle* resolve_clear_rectangle = nullptr,
      MTL4::CommandBuffer* command_buffer = nullptr);

  enum class TransferOutput : uint32_t {
    kColor,
    kDepth,
    kStencilBit,
  };

  enum class TransferMode : uint32_t {
    kColorToColor,
    kColorToDepth,
    kDepthToColor,
    kDepthToDepth,
    kColorToStencilBit,
    kDepthToStencilBit,
    kColorAndHostDepthToDepth,
    kDepthAndHostDepthToDepth,
    kCount,
  };

  struct TransferModeInfo {
    TransferOutput output;
    bool source_is_color;
    bool uses_host_depth;
  };

  static const TransferModeInfo kTransferModeInfos[size_t(TransferMode::kCount)];

  union TransferShaderKey {
    uint32_t key;
    struct {
      xenos::MsaaSamples dest_msaa_samples : xenos::kMsaaSamplesBits;
      uint32_t dest_resource_format : xenos::kRenderTargetFormatBits;
      xenos::MsaaSamples source_msaa_samples : xenos::kMsaaSamplesBits;
      xenos::MsaaSamples host_depth_source_msaa_samples : xenos::kMsaaSamplesBits;
      uint32_t source_resource_format : xenos::kRenderTargetFormatBits;
      static_assert(size_t(TransferMode::kCount) <= (size_t(1) << 4));
      TransferMode mode : 4;
      uint32_t host_depth_source_is_copy : 1;
      uint32_t dest_sample_id_from_sample : 1;
    };

    TransferShaderKey() : key(0) { static_assert_size(*this, sizeof(key)); }

    struct Hasher {
      size_t operator()(const TransferShaderKey& key) const {
        return std::hash<uint32_t>{}(key.key);
      }
    };
    bool operator==(const TransferShaderKey& other_key) const { return key == other_key.key; }
    bool operator!=(const TransferShaderKey& other_key) const { return !(*this == other_key); }
    bool operator<(const TransferShaderKey& other_key) const { return key < other_key.key; }
  };

  struct TransferInvocation {
    Transfer transfer;
    TransferShaderKey shader_key;
    TransferInvocation(const Transfer& transfer, const TransferShaderKey& shader_key)
        : transfer(transfer), shader_key(shader_key) {}
    bool operator<(const TransferInvocation& other_invocation) const {
      if (shader_key != other_invocation.shader_key) {
        return shader_key < other_invocation.shader_key;
      }
      assert_not_null(transfer.source);
      assert_not_null(other_invocation.transfer.source);
      uint32_t source_index =
          static_cast<const MetalRenderTarget*>(transfer.source)->temporary_sort_index();
      uint32_t other_source_index =
          static_cast<const MetalRenderTarget*>(other_invocation.transfer.source)
              ->temporary_sort_index();
      if (source_index != other_source_index) {
        return source_index < other_source_index;
      }
      return transfer.start_tiles < other_invocation.transfer.start_tiles;
    }
    bool CanBeMergedIntoOneDraw(const TransferInvocation& other_invocation) const {
      return shader_key == other_invocation.shader_key &&
             transfer.AreSourcesSame(other_invocation.transfer);
    }
  };

  MTL::Library* GetOrCreateEdramLoadLibrary(bool msaa);
  MTL::RenderPipelineState* GetOrCreateEdramLoadPipeline(
      MTL::PixelFormat dest_format, uint32_t sample_count);

  MTL::RenderPipelineState* GetOrCreateTransferPipelines(
      const TransferShaderKey& key, MTL::PixelFormat dest_format,
      bool dest_is_uint, bool tile_instanced);
  MTL::Library* GetOrCreateTransferLibrary();
  MTL::RenderPipelineState* GetOrCreateTransferClearPipeline(
      MTL::PixelFormat dest_format, bool dest_is_uint, bool is_depth,
      uint32_t sample_count);
  MTL::Texture* GetTransferDummyTexture(MTL::PixelFormat format,
                                        uint32_t sample_count);
  MTL::Texture* GetTransferDummyColorFloatTexture(uint32_t sample_count);
  MTL::Texture* GetTransferDummyColorUintTexture(uint32_t sample_count);
  MTL::Texture* GetTransferDummyDepthTexture(uint32_t sample_count);
  MTL::Texture* GetTransferDummyStencilTexture(uint32_t sample_count);
  MTL::Buffer* GetTransferDummyBuffer();
  MTL::DepthStencilState* GetTransferDepthStencilState(bool depth_write);
  MTL::DepthStencilState* GetTransferNoDepthStencilState();
  MTL::DepthStencilState* GetTransferDepthClearState();
  MTL::DepthStencilState* GetTransferStencilClearState();
  MTL::DepthStencilState* GetTransferStencilBitState(uint32_t bit);

  MetalCommandProcessor& command_processor_;
  TraceWriter* trace_writer_;
  MTL::Device* device_ = nullptr;

  bool msaa_2x_supported_ = false;
  bool gamma_render_target_as_unorm16_ = false;

  // EDRAM
  MTL::Buffer* edram_buffer_ = nullptr;
  MTL::Texture* edram_r32_uint_buffer_view_ = nullptr;
  MTL::Texture* edram_r32g32_uint_buffer_view_ = nullptr;
  MTL::Texture* edram_r32g32b32a32_uint_buffer_view_ = nullptr;

  // EDRAM compute shaders
  MTL::ComputePipelineState* edram_store_pipeline_ = nullptr;
  MTL::ComputePipelineState* edram_dump_color_32bpp_1xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* edram_dump_color_32bpp_2xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* edram_dump_color_32bpp_4xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* edram_dump_color_64bpp_1xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* edram_dump_color_64bpp_2xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* edram_dump_color_64bpp_4xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* edram_dump_depth_32bpp_1xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* edram_dump_depth_32bpp_2xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* edram_dump_depth_32bpp_4xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_full_8bpp_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_full_16bpp_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_full_32bpp_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_full_64bpp_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_full_128bpp_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_fast_32bpp_1x2xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_fast_32bpp_4xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_fast_64bpp_1x2xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_fast_64bpp_4xmsaa_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_full_8bpp_scaled_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_full_16bpp_scaled_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_full_32bpp_scaled_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_full_64bpp_scaled_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_full_128bpp_scaled_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_fast_32bpp_1x2xmsaa_scaled_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_fast_32bpp_4xmsaa_scaled_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_fast_64bpp_1x2xmsaa_scaled_pipeline_ = nullptr;
  MTL::ComputePipelineState* resolve_fast_64bpp_4xmsaa_scaled_pipeline_ = nullptr;
  MTL::ComputePipelineState* host_depth_store_pipelines_[3] = {};

  // Transfer pipelines
  std::unordered_map<TransferShaderKey, MTL::RenderPipelineState*, TransferShaderKey::Hasher> transfer_pipelines_;
  std::unordered_map<TransferShaderKey, MTL::RenderPipelineState*, TransferShaderKey::Hasher> transfer_tile_pipelines_;
  std::unordered_map<uint64_t, MTL::RenderPipelineState*> edram_load_pipelines_;
  std::unordered_map<uint64_t, MTL::RenderPipelineState*> transfer_clear_pipelines_;
  MTL::Library* transfer_library_ = nullptr;
  MTL::Library* edram_load_library_ = nullptr;
  MTL::Library* edram_load_library_msaa_ = nullptr;
  MTL::DepthStencilState* transfer_depth_state_ = nullptr;
  MTL::DepthStencilState* transfer_depth_state_none_ = nullptr;
  MTL::DepthStencilState* transfer_depth_clear_state_ = nullptr;
  MTL::DepthStencilState* transfer_stencil_clear_state_ = nullptr;
  MTL::DepthStencilState* transfer_stencil_bit_states_[8] = {};
  MTL::Buffer* transfer_dummy_buffer_ = nullptr;
  MTL::Buffer* transfer_tile_instance_buffers_[3] = {};
  std::vector<MTL::Buffer*> transfer_tile_instance_retired_buffers_[3];
  std::array<size_t, 3> transfer_tile_instance_buffer_sizes_ = {};
  uint32_t transfer_tile_instance_buffer_offset_ = 0;
  MTL::Texture* transfer_dummy_color_float_[4] = {};
  MTL::Texture* transfer_dummy_color_uint_[4] = {};
  MTL::Texture* transfer_dummy_depth_[4] = {};
  MTL::Texture* transfer_dummy_stencil_[4] = {};

  // Render targets
  MetalRenderTarget* current_color_targets_[4] = {};
  MetalRenderTarget* current_depth_target_ = nullptr;
  MetalRenderTarget* dummy_color_target_ = nullptr;
  MetalRenderTarget* last_real_color_targets_[4] = {};
  MetalRenderTarget* last_real_depth_target_ = nullptr;
  struct DummyColorTargetEntry {
    std::unique_ptr<MetalRenderTarget> target;
    uint64_t last_used_frame = 0;
    uint64_t last_cleared_frame = 0;
  };
  std::unordered_map<uint64_t, DummyColorTargetEntry> dummy_color_targets_;
  std::unordered_map<uint64_t, MetalRenderTarget*> render_target_map_;
  std::vector<uint64_t> cleared_render_targets_this_frame_;
  uint64_t frame_id_ = 0;
  bool render_pass_descriptor_dirty_ = true;
  MTL::RenderPassDescriptor* cached_render_pass_descriptor_ = nullptr;
  uint32_t cached_render_pass_descriptor_sample_count_ = 0;

  std::unique_ptr<MetalHeapPool> render_target_heap_pool_;

  static constexpr uint32_t kTransferInstanceBufferCount = 3;
  uint64_t transfer_tile_instance_buffer_frame_id_ = 0;

  std::vector<TransferInvocation> transfer_invocations_;
};

}  // namespace rex::graphics::metal
