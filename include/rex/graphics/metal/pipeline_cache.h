#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <rex/string/buffer.h>

#include <Metal/Metal.hpp>
#include <rex/graphics/register_file.h>
#include <rex/graphics/xenos.h>
#include <rex/graphics/metal/dxbc_to_dxil_converter.h>
#include <rex/graphics/metal/geometry_shader.h>
#include <rex/graphics/metal/shader.h>
#include <rex/graphics/metal/shader_converter.h>
#include <rex/graphics/pipeline/shader/dxbc_translator.h>
#define IR_RUNTIME_METALCPP
#include <metal_irconverter_runtime.h>

#include <rex/graphics/primitive_processor.h>

namespace rex::graphics::metal {

class DxbcToDxilConverter;
class MetalShaderCache;

struct PipelineAttachmentFormats {
  uint32_t sample_count = 1;
  MTL::PixelFormat color_formats[4] = {};
  MTL::PixelFormat depth_format = MTL::PixelFormatInvalid;
  MTL::PixelFormat stencil_format = MTL::PixelFormatInvalid;
};

struct PipelineRenderingKey {
  uint32_t normalized_color_mask = 0;
  uint32_t alpha_to_mask_enable = 0;
  uint32_t blendcontrol[4] = {};
};

class MetalPipelineCache {
 public:
  struct PipelineHandle {
    std::atomic<MTL::RenderPipelineState*> state{nullptr};
    MetalShader::MetalTranslation* pending_vertex_translation = nullptr;
    MetalShader::MetalTranslation* pending_pixel_translation = nullptr;
    PipelineAttachmentFormats pending_formats = {};
    uint32_t pending_normalized_color_mask = 0;
    uint32_t pending_blendcontrol[4] = {};
    bool pending_alpha_to_mask = false;
  };

  struct TessellationPipelineState {
    MTL::RenderPipelineState* pipeline = nullptr;
    IRRuntimeTessellationPipelineConfig config = {};
    IRRuntimePrimitiveType primitive = IRRuntimePrimitiveTypeTriangle;
  };

  struct GeometryPipelineState {
    MTL::RenderPipelineState* pipeline = nullptr;
    uint32_t gs_vertex_size_in_bytes = 0;
    uint32_t gs_max_input_primitives_per_mesh_threadgroup = 0;
  };

  struct GeometryVertexStageState {
    MTL::Library* library = nullptr;
    MTL::Library* stage_in_library = nullptr;
    std::string function_name;
    uint32_t vertex_output_size_in_bytes = 0;
  };

  struct GeometryShaderStageState {
    MTL::Library* library = nullptr;
    std::string function_name;
    uint32_t max_input_primitives_per_mesh_threadgroup = 0;
    std::vector<MetalShaderFunctionConstant> function_constants;
  };

  struct TessellationVertexStageState {
    MTL::Library* library = nullptr;
    MTL::Library* stage_in_library = nullptr;
    std::string function_name;
    uint32_t vertex_output_size_in_bytes = 0;
  };

  struct TessellationHullStageState {
    MTL::Library* library = nullptr;
    std::string function_name;
    MetalShaderReflectionInfo reflection = {};
  };

  struct TessellationDomainStageState {
    MTL::Library* library = nullptr;
    std::string function_name;
    MetalShaderReflectionInfo reflection = {};
  };

  struct PipelineDiskCacheVertexAttribute {
    uint32_t attribute_index = 0;
    uint32_t format = 0;
    uint32_t offset = 0;
    uint32_t buffer_index = 0;
  };

  struct PipelineDiskCacheVertexLayout {
    uint32_t buffer_index = 0;
    uint32_t stride = 0;
    uint32_t step_function = 0;
    uint32_t step_rate = 0;
  };

  struct PipelineDiskCacheEntry {
    uint64_t pipeline_key = 0;
    uint64_t vertex_shader_cache_key = 0;
    uint64_t pixel_shader_cache_key = 0;
    uint32_t sample_count = 0;
    uint32_t depth_format = 0;
    uint32_t stencil_format = 0;
    uint32_t color_formats[4] = {};
    uint32_t normalized_color_mask = 0;
    uint32_t alpha_to_mask_enable = 0;
    uint32_t blendcontrol[4] = {};
    std::vector<PipelineDiskCacheVertexAttribute> vertex_attributes;
    std::vector<PipelineDiskCacheVertexLayout> vertex_layouts;
  };

  MetalPipelineCache(MTL::Device* device, const RegisterFile& register_file);
  ~MetalPipelineCache();

  bool InitializeShaderTranslation(bool gamma_render_target_as_unorm8,
                                   bool msaa_2x_supported,
                                   uint32_t draw_resolution_scale_x,
                                   uint32_t draw_resolution_scale_y);
  void InitializeShaderStorage(const std::filesystem::path& cache_root,
                               uint32_t title_id, bool blocking);
  bool InitializeShaderStorageInternal(const std::filesystem::path& cache_root,
                                       uint32_t title_id, bool blocking);

  Shader* LoadShader(xenos::ShaderType shader_type,
                     uint32_t guest_address, const uint32_t* host_address,
                     uint32_t dword_count);

  PipelineHandle* GetOrCreatePipelineState(
      MetalShader::MetalTranslation* vertex_translation,
      MetalShader::MetalTranslation* pixel_translation,
      const PipelineAttachmentFormats& attachment_formats,
      const PipelineRenderingKey& rendering_key);

  TessellationPipelineState* GetOrCreateTessellationPipelineState(
      MetalShader::MetalTranslation* domain_translation,
      MetalShader::MetalTranslation* pixel_translation,
      const ::rex::graphics::PrimitiveProcessor::ProcessingResult& primitive_processing_result,
      const PipelineAttachmentFormats& attachment_formats,
      const PipelineRenderingKey& rendering_key);
  GeometryPipelineState* GetOrCreateGeometryPipelineState(
      MetalShader::MetalTranslation* vertex_translation,
      MetalShader::MetalTranslation* pixel_translation,
      GeometryShaderKey geometry_shader_key,
      const PipelineAttachmentFormats& attachment_formats,
      const PipelineRenderingKey& rendering_key);

  DxbcShaderTranslator* shader_translator() const { return shader_translator_.get(); }
  DxbcToDxilConverter* dxbc_to_dxil_converter() const { return dxbc_to_dxil_converter_.get(); }
  MetalShaderConverter* metal_shader_converter() const { return metal_shader_converter_.get(); }

  DxbcShaderTranslator::Modification GetCurrentVertexShaderModification(
      const Shader& shader, Shader::HostVertexShaderType host_vertex_shader_type,
      uint32_t interpolator_mask) const;
  DxbcShaderTranslator::Modification GetCurrentPixelShaderModification(
      const Shader& shader, uint32_t interpolator_mask, uint32_t param_gen_pos,
      reg::RB_DEPTHCONTROL normalized_depth_control) const;

  bool IsCreatingPipelines();

  struct UcodeDisasmBuffer {};
  string::StringBuffer& ucode_disasm_buffer() { return ucode_disasm_buffer_; }

 private:
  bool EnsureDepthOnlyPixelShader();
  MTL::RenderPipelineState* CreatePipelineFromHandle(const PipelineHandle* handle);
  void CreationThread(size_t thread_index);

  void ShutdownShaderStorage();
  bool LoadPipelineDiskCache(const std::filesystem::path& path,
                             std::vector<PipelineDiskCacheEntry>* entries);
  bool AppendPipelineDiskCacheEntry(const PipelineDiskCacheEntry& entry);
  bool InitializePipelineBinaryArchive(const std::filesystem::path& archive_path);
  void SerializePipelineBinaryArchive();
  void PrewarmPipelineBinaryArchive(
      const std::vector<PipelineDiskCacheEntry>& entries);

  std::string GetShaderStorageDeviceTag() const;
  std::string GetShaderStorageAbiTag() const;

  MTL::Device* device_;
  const RegisterFile& register_file_;

  std::unique_ptr<DxbcShaderTranslator> shader_translator_;
  std::unique_ptr<DxbcToDxilConverter> dxbc_to_dxil_converter_;
  std::unique_ptr<MetalShaderConverter> metal_shader_converter_;

  std::unordered_map<uint64_t, std::unique_ptr<PipelineHandle>> pipeline_cache_;
  std::unordered_map<uint64_t, GeometryPipelineState> geometry_pipeline_cache_;
  std::unordered_map<const void*, GeometryVertexStageState> geometry_vertex_stage_cache_;
  std::unordered_map<GeometryShaderKey, GeometryShaderStageState,
                     GeometryShaderKey::Hasher> geometry_shader_stage_cache_;
  std::unordered_map<uint64_t, TessellationPipelineState> tessellation_pipeline_cache_;
  std::unordered_map<uint64_t, TessellationVertexStageState> tessellation_vertex_stage_cache_;
  std::unordered_map<uint64_t, TessellationHullStageState> tessellation_hull_stage_cache_;
  std::unordered_map<uint64_t, TessellationDomainStageState> tessellation_domain_stage_cache_;
  std::unordered_map<uint64_t, std::unique_ptr<MetalShader>> shader_cache_;

  MTL::Library* depth_only_pixel_library_ = nullptr;
  std::string depth_only_pixel_function_name_;

  // Async pipeline creation
  struct PipelineCreationRequest {
    PipelineHandle* handle;
    bool operator<(const PipelineCreationRequest& other) const {
      return handle < other.handle;
    }
  };
  std::vector<std::thread> creation_threads_;
  std::mutex creation_request_lock_;
  std::condition_variable creation_request_cond_;
  std::priority_queue<PipelineCreationRequest> creation_queue_;
  uint32_t creation_threads_busy_ = 0;
  bool creation_threads_shutdown_ = false;

  // Shader storage
  std::filesystem::path shader_storage_root_;
  std::filesystem::path shader_storage_local_root_;
  std::filesystem::path shader_storage_title_root_;
  std::filesystem::path metallib_cache_dir_;

  // Pipeline disk cache
  FILE* pipeline_disk_cache_file_ = nullptr;
  std::filesystem::path pipeline_disk_cache_path_;
  std::unordered_set<uint64_t> pipeline_disk_cache_keys_;
  std::vector<PipelineDiskCacheEntry> pipeline_disk_cache_entries_;

  // Pipeline binary archive
  MTL::BinaryArchive* pipeline_binary_archive_ = nullptr;
  std::mutex pipeline_binary_archive_mutex_;
  bool pipeline_binary_archive_dirty_ = false;
  std::filesystem::path pipeline_binary_archive_path_;

  string::StringBuffer ucode_disasm_buffer_;
};

PipelineRenderingKey ResolvePipelineRenderingKey(
    const RegisterFile& regs,
    const MetalShader::MetalTranslation* pixel_translation,
    bool use_fallback_pixel_shader);

const std::vector<uint32_t>& GetGeometryShader(GeometryShaderKey key);
bool GetGeometryShaderKey(PipelineGeometryShader type,
                          DxbcShaderTranslator::Modification vs_mod,
                          DxbcShaderTranslator::Modification ps_mod,
                          GeometryShaderKey& key_out);

}  // namespace rex::graphics::metal
