#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <deque>
#include <vector>

#include <rex/graphics/command_processor.h>
#include <rex/graphics/metal/texture_cache.h>
#include <rex/graphics/metal/pipeline_cache.h>
#include <rex/graphics/pipeline/shader/dxbc_translator.h>
#include <rex/graphics/pipeline/texture/util.h>
#include <rex/graphics/primitive_processor.h>
#include <rex/graphics/util/draw.h>
#include <rex/math.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>
#define IR_RUNTIME_METALCPP
#include <metal_irconverter_runtime.h>

namespace rex::ui::metal {
class MetalProvider;
}  // namespace rex::ui::metal

namespace rex::graphics::metal {

class DxbcToDxilConverter;
class Metal4Context;
class MetalGraphicsSystem;
class MetalPipelineCache;
class MetalPrimitiveProcessor;
class MetalRenderTargetCache;
class MetalSharedMemory;
class MetalShader;
class MetalShaderConverter;
class MetalTextureCache;
class MetalUploadBufferPool;

class MetalCommandProcessor : public CommandProcessor {
 public:
  MetalCommandProcessor(MetalGraphicsSystem* graphics_system,
                        system::KernelState* kernel_state);
  ~MetalCommandProcessor() override;

  void TracePlaybackWroteMemory(uint32_t base_ptr, uint32_t length) override;
  void RestoreEdramSnapshot(const void* snapshot) override;
  void ClearCaches() override;
  void InvalidateGpuMemory() override;
  void ClearReadbackBuffers();

  rex::ui::metal::MetalProvider& GetMetalProvider() const;
  uint64_t GetCurrentSubmission() const;
  uint64_t GetCompletedSubmission() const;

  MTL::Device* GetMetalDevice() const { return device_; }
  Metal4Context* GetMetal4Context() const { return mtl4_; }

  bool HasActiveSubmission() const;
  bool CanJoinActiveSubmissionForTransfer() const;
  uint64_t GetLatestSubmissionStarted() const;

  MTL4::CommandBuffer* CreateStandaloneTransferCommandBuffer(
      const char* label);
  void CommitStandaloneAsync(MTL4::CommandBuffer* cmd);
  void CommitStandaloneAsyncWithCallback(
      MTL4::CommandBuffer* cmd,
      const std::function<void(MTL4::CommandBuffer*)>& callback);
  void CommitStandaloneAndWait(MTL4::CommandBuffer* cmd);

  void UseRenderEncoderResource(MTL::Resource* resource,
                                MTL::ResourceUsage usage);

  uint32_t AllocateViewBindlessIndex();
  void ReleaseViewBindlessIndex(uint32_t index);
  void RetireViewBindlessIndex(uint32_t index);
  uint32_t GetViewBindlessHeapAvailableCount() const;
  uint32_t AllocateSamplerBindlessIndex();
  void ReleaseSamplerBindlessIndex(uint32_t index);
  uint64_t GetBindlessDescriptorRetirementSubmission() const;

  void FreeViewBindlessIndexNow(uint32_t index);
  void FreeSamplerBindlessIndexNow(uint32_t index);

  MetalSharedMemory* shared_memory() const { return shared_memory_.get(); }
  MetalTextureCache* texture_cache() const { return texture_cache_.get(); }

  void SetSwapDestSwap(uint32_t dest_base, bool swap);
  void EndRenderEncoder();
  void ForceIssueSwap();

  // IRDescriptorTableEntry accessor helpers
  IRDescriptorTableEntry* GetViewBindlessHeapEntry(uint32_t index);
  IRDescriptorTableEntry* GetSamplerBindlessHeapEntry(uint32_t index);

 protected:
  bool SetupContext() override;
  void ShutdownContext() override;
  void PrepareForWait() override;

  Shader* LoadShader(xenos::ShaderType shader_type, uint32_t guest_address,
                     const uint32_t* host_address,
                     uint32_t dword_count) override;

  bool IssueDraw(xenos::PrimitiveType primitive_type, uint32_t index_count,
                 IndexBufferInfo* index_buffer_info,
                 bool major_mode_explicit) override;
  bool IssueCopy() override;
  void IssueSwap(uint32_t frontbuffer_ptr, uint32_t frontbuffer_width,
                 uint32_t frontbuffer_height) override;
  void OnPrimaryBufferEnd() override;
  bool CanEndSubmissionImmediately();
  void InitializeShaderStorage(const std::filesystem::path& cache_root,
                               uint32_t title_id, bool blocking) override;

  void BeginResolveOrdering();
  void EndResolveOrdering();
  void OnGammaRamp256EntryTableValueWritten() override;
  void OnGammaRampPWLValueWritten() override;
  void WriteRegister(uint32_t index, uint32_t value) override;
  MTL4::CommandBuffer* EnsureMTL4CommandBuffer();
  void EnsureCommandBufferAutoreleasePool();

 private:
  struct UniformBufferInfo {
    MTL::Buffer* vs_buf = nullptr;
    NS::UInteger vs_off = 0;
    uint64_t vs_gpu = 0;
    MTL::Buffer* ps_buf = nullptr;
    NS::UInteger ps_off = 0;
    uint64_t ps_gpu = 0;
  };

  struct DepthStencilStateKey {
    uint32_t depth_control = 0;
    uint32_t stencil_ref_mask_front = 0;
    uint32_t stencil_ref_mask_back = 0;
    uint32_t polygonal_and_backface = 0;
    bool operator==(const DepthStencilStateKey& other) const {
      return depth_control == other.depth_control &&
             stencil_ref_mask_front == other.stencil_ref_mask_front &&
             stencil_ref_mask_back == other.stencil_ref_mask_back &&
             polygonal_and_backface == other.polygonal_and_backface;
    }
  };
  struct DepthStencilStateKeyHasher {
    size_t operator()(const DepthStencilStateKey& k) const {
      size_t h = std::hash<uint32_t>{}(k.depth_control);
      h ^= std::hash<uint32_t>{}(k.stencil_ref_mask_front) << 1;
      h ^= std::hash<uint32_t>{}(k.stencil_ref_mask_back) << 2;
      h ^= std::hash<uint32_t>{}(k.polygonal_and_backface) << 3;
      return h;
    }
  };

  struct TraceResolveGuard {
    struct ResolvedRange {
      uint32_t base = 0;
      uint32_t length = 0;
    };
    void Mark(uint32_t base_ptr, uint32_t length);
    bool IsResolved(uint32_t base_ptr, uint32_t length) const;
    void Clear();
    std::vector<ResolvedRange> ranges_;
  };

  bool ConsumeSwapDestSwap(uint32_t dest_base, bool* swap_out);

  void FlushCommandBufferAndWait(uint64_t timeout_ns, const char* context);
  void WaitForPendingCompletionHandlers();
  void ProcessCompletedSubmissions();

  void BeginCommandBuffer();
  void EndCommandBuffer();
  void DrainCommandBufferAutoreleasePool();

  bool UploadConstants(
      const RegisterFile& regs, Shader* vertex_shader, Shader* pixel_shader,
      MetalShader* metal_vertex_shader, MetalShader* metal_pixel_shader,
      bool shared_memory_is_uav,
      const ::rex::graphics::PrimitiveProcessor::ProcessingResult& primitive_processing_result,
      uint32_t used_texture_mask, uint32_t normalized_color_mask,
      UniformBufferInfo& uniforms_out);
  bool PopulateBindlessTables(
      MetalShader* metal_vertex_shader, MetalShader* metal_pixel_shader,
      bool shared_memory_is_uav, MTL::ResourceUsage shared_memory_usage,
      bool use_geometry_emulation, bool use_tessellation_emulation,
      const UniformBufferInfo& uniforms);

  struct VertexBindingRange {
    uint32_t binding_index;
    uint32_t offset;
    uint32_t length;
    uint32_t stride;
  };

  bool DispatchDraw(
      const RegisterFile& regs,
      const ::rex::graphics::PrimitiveProcessor::ProcessingResult& primitive_processing_result,
      bool use_tessellation_emulation,
      MetalPipelineCache::TessellationPipelineState* tessellation_pipeline_state,
      bool use_geometry_emulation,
      MetalPipelineCache::GeometryPipelineState* geometry_pipeline_state,
      bool shared_memory_is_uav, MTL::ResourceUsage shared_memory_usage,
      bool memexport_used, bool uses_vertex_fetch,
      const std::vector<Shader::VertexBinding>& vb_bindings,
      const VertexBindingRange* vertex_ranges, uint32_t vertex_range_count,
      IndexBufferInfo* index_buffer_info);
  void ApplyRasterizerState(bool prim_polygonal);
  void ApplyDepthStencilState(bool prim_polygonal,
                               reg::RB_DEPTHCONTROL depth_control);
  void UpdateSystemConstantValues(
      bool shared_memory_is_uav, bool primitive_polygonal,
      uint32_t line_loop_closing_index, xenos::Endian index_endian,
      const ::rex::graphics::draw_util::ViewportInfo& viewport_info,
      uint32_t used_texture_mask,
      reg::RB_DEPTHCONTROL normalized_depth_control,
      uint32_t normalized_color_mask);

  struct RetiredBindlessIndex {
    uint32_t index;
    uint64_t submission_id;
  };

  // Bindless heap constants
  static constexpr uint32_t kViewBindlessHeapSize = 131072;
  static constexpr uint32_t kSamplerBindlessHeapSize = 65536;
  static constexpr uint32_t kNullBufferSize = 256;
  static constexpr uint32_t kSystemViewTableEntryCount = 8;
  static constexpr uint32_t kSystemViewTableSRVSharedMemory = 0;
  static constexpr uint32_t kSystemViewTableSRVNull = 1;
  static constexpr uint32_t kSystemViewTableUAVNullStart = 2;
  static constexpr uint32_t kSystemViewTableUAVSharedMemoryStart = 4;
  static constexpr size_t kCbvSizeBytes = 4096;

  static constexpr uint32_t kSystemViewTableSRVSharedMemory_ = kSystemViewTableSRVSharedMemory;
  static constexpr uint32_t kSystemViewTableSRVNull_ = kSystemViewTableSRVNull;
  static constexpr uint32_t kSystemViewTableUAVNullStart_ = kSystemViewTableUAVNullStart;
  static constexpr uint32_t kSystemViewTableUAVSharedMemoryStart_ = kSystemViewTableUAVSharedMemoryStart;

  MTL::Device* device_ = nullptr;
  Metal4Context* mtl4_ = nullptr;

  std::unique_ptr<MetalSharedMemory> shared_memory_;
  std::unique_ptr<MetalPrimitiveProcessor> primitive_processor_;
  std::unique_ptr<MetalTextureCache> texture_cache_;
  std::unique_ptr<MetalRenderTargetCache> render_target_cache_;
  std::unique_ptr<MetalPipelineCache> pipeline_cache_;
  std::unique_ptr<MetalUploadBufferPool> constant_buffer_pool_;

  TraceResolveGuard trace_resolve_guard_;
  bool saw_swap_ = false;
  uint32_t last_swap_ptr_ = 0;
  uint32_t last_swap_width_ = 0;
  uint32_t last_swap_height_ = 0;
  std::unordered_map<uint32_t, bool> swap_dest_swaps_by_base_;

  bool gamma_ramp_256_entry_table_up_to_date_ = false;
  bool gamma_ramp_pwl_up_to_date_ = false;

  bool frame_open_ = false;
  bool mesh_shader_supported_ = false;
  bool submission_has_draws_ = false;
  bool copy_resolve_writes_pending_ = false;

  uint64_t submission_current_ = 0;
  std::atomic<uint64_t> completed_command_buffers_{0};
  std::atomic<uint32_t> pending_completion_handlers_{0};

  MTL4::RenderCommandEncoder* current_render_encoder_ = nullptr;
  MTL::RenderPipelineState* current_render_pipeline_state_ = nullptr;
  MTL4::CommandBuffer* current_mtl4_command_buffer_ = nullptr;

  MTL::Buffer* view_bindless_heap_ = nullptr;
  uint32_t view_bindless_heap_next_ = 0;
  bool view_bindless_heap_exhausted_logged_ = false;
  std::vector<uint32_t> view_bindless_heap_free_;
  std::deque<RetiredBindlessIndex> retired_view_bindless_indices_;

  MTL::Buffer* sampler_bindless_heap_ = nullptr;
  uint32_t sampler_bindless_heap_next_ = 0;
  bool sampler_bindless_heap_exhausted_logged_ = false;
  std::vector<uint32_t> sampler_bindless_heap_free_;
  std::deque<RetiredBindlessIndex> retired_sampler_bindless_indices_;

  MTL::Buffer* system_view_tables_ = nullptr;
  MTL::Buffer* null_buffer_ = nullptr;
  MTL::Texture* null_texture_ = nullptr;
  MTL::SamplerState* null_sampler_ = nullptr;

  MTL::Buffer* tessellator_tables_buffer_ = nullptr;

  MTL::SharedEvent* wait_shared_event_ = nullptr;
  uint64_t wait_shared_event_value_ = 0;

  std::unordered_map<DepthStencilStateKey, MTL::DepthStencilState*,
                     DepthStencilStateKeyHasher>
      depth_stencil_state_cache_;

  bool viewport_dirty_ = true;
  bool scissor_dirty_ = true;
  MTL::Viewport cached_viewport_ = {};
  MTL::ScissorRect cached_scissor_ = {};

  bool ff_blend_factor_valid_ = false;
  float ff_blend_factor_[4] = {};

  uint32_t current_float_constant_map_vertex_[4] = {};
  uint32_t current_float_constant_map_pixel_[4] = {};

  struct CBufferBinding {
    MTL::Buffer* buffer = nullptr;
    size_t offset = 0;
    uint64_t gpu_address = 0;
    bool up_to_date = false;
  };
  CBufferBinding cbuffer_binding_float_vertex_;
  CBufferBinding cbuffer_binding_float_pixel_;
  bool cbuffer_binding_system_up_to_date_ = false;
  CBufferBinding cbuffer_binding_bool_loop_;
  CBufferBinding cbuffer_binding_fetch_;

  size_t current_sampler_layout_uid_vertex_ = 0;
  size_t current_sampler_layout_uid_pixel_ = 0;

  bool cbuffer_binding_descriptor_indices_vertex_up_to_date_ = false;
  bool cbuffer_binding_descriptor_indices_pixel_up_to_date_ = false;
  size_t current_texture_layout_uid_vertex_ = 0;
  size_t current_texture_layout_uid_pixel_ = 0;
  std::vector<uint32_t> current_texture_bindless_indices_vertex_;
  std::vector<uint32_t> current_sampler_bindless_indices_vertex_;
  std::vector<uint32_t> current_texture_bindless_indices_pixel_;
  std::vector<uint32_t> current_sampler_bindless_indices_pixel_;
  std::vector<MetalTextureCache::SamplerParameters> current_samplers_vertex_;
  std::vector<MetalTextureCache::SamplerParameters> current_samplers_pixel_;
  std::vector<MetalTextureCache::TextureSRVKey> current_texture_srv_keys_vertex_;
  std::vector<MetalTextureCache::TextureSRVKey> current_texture_srv_keys_pixel_;

  bool heap_binds_set_on_encoder_ = false;
  MTL4::RenderPassDescriptor* current_render_pass_descriptor_ = nullptr;
  MTL::DepthStencilState* current_depth_stencil_state_ = nullptr;
  bool rasterizer_state_valid_ = false;
  bool stencil_reference_valid_ = false;
  uint32_t current_stencil_reference_ = 0;
  MTL::CullMode current_cull_mode_ = MTL::CullModeNone;
  MTL::Winding current_front_facing_winding_ = MTL::WindingCounterClockwise;
  MTL::TriangleFillMode current_triangle_fill_mode_ = MTL::TriangleFillModeFill;
  float current_depth_bias_values_[3] = {};
  MTL::DepthClipMode current_depth_clip_mode_ = MTL::DepthClipModeClip;
  uint64_t submission_completed_processed_ = 0;
  NS::AutoreleasePool* command_buffer_autorelease_pool_ = nullptr;
  enum class ResolveOrderingPolicy { kSubmissionBoundary };
  ResolveOrderingPolicy resolve_ordering_policy_ = ResolveOrderingPolicy::kSubmissionBoundary;

  static constexpr size_t kUniformsBytesPerTable = 5 * kCbvSizeBytes;
  static constexpr size_t kStageCount = 2;
  static constexpr size_t kCbvHeapSlotsPerTable = 16;
  static constexpr size_t kTopLevelABBytesPerTable = 14 * sizeof(uint64_t);

  uint64_t current_bindless_vs_uniforms_gpu_ = 0;
  uint64_t current_bindless_ps_uniforms_gpu_ = 0;
  bool current_bindless_shared_memory_is_uav_ = false;
  bool current_bindless_uses_mesh_stages_ = false;

  DxbcShaderTranslator::SystemConstants system_constants_ = {};

  std::vector<draw_util::MemExportRange> memexport_ranges_;

  bool current_bindless_table_valid_ = false;
  MTL::Buffer* current_bindless_top_level_buffer_ = nullptr;
  uint32_t current_bindless_top_level_offset_ = 0;
  uint64_t current_bindless_top_level_gpu_address_ = 0;
  MTL::Buffer* current_bindless_cbv_buffer_ = nullptr;
  uint32_t current_bindless_cbv_offset_ = 0;
  uint64_t current_bindless_cbv_gpu_address_ = 0;
};

}  // namespace rex::graphics::metal
