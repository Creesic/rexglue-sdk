#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <rex/graphics/xenos.h>

struct IRVersionedInputLayoutDescriptor;

namespace rex::graphics::metal {

enum class MetalShaderStage : uint32_t {
  kVertex = 0,
  kFragment,
  kGeometry,
  kCompute,
  kHull,
  kDomain,
};

struct MetalShaderFunctionConstant {
  std::string name;
  uint32_t type = 0;
};

struct MetalShaderReflectionInput {
  std::string name;
  uint32_t attribute_index = 0;
};

struct MetalShaderReflectionInfo {
  std::vector<MetalShaderReflectionInput> vertex_inputs;
  std::vector<MetalShaderFunctionConstant> function_constants;
  uint32_t vertex_output_size_in_bytes = 0;
  uint32_t vertex_input_count = 0;
  uint32_t gs_max_input_primitives_per_mesh_threadgroup = 0;
  bool has_hull_info = false;
  uint32_t hs_max_patches_per_object_threadgroup = 0;
  uint32_t hs_max_object_threads_per_patch = 0;
  uint32_t hs_patch_constants_size = 0;
  uint32_t hs_input_control_point_count = 0;
  uint32_t hs_output_control_point_count = 0;
  uint32_t hs_output_control_point_size = 0;
  uint32_t hs_tessellator_domain = 0;
  uint32_t hs_tessellator_partitioning = 0;
  uint32_t hs_tessellator_output_primitive = 0;
  bool hs_tessellation_type_half = false;
  float hs_max_tessellation_factor = 0.0f;
  bool has_domain_info = false;
  uint32_t ds_max_input_prims_per_mesh_threadgroup = 0;
  uint32_t ds_input_control_point_count = 0;
  uint32_t ds_input_control_point_size = 0;
  uint32_t ds_patch_constants_size = 0;
  uint32_t ds_tessellator_domain = 0;
  bool ds_tessellation_type_half = false;
};

struct MetalShaderConversionResult {
  bool success = false;
  std::string error_message;
  std::string function_name;
  std::vector<uint8_t> metallib_data;
  bool has_mesh_stage = false;
  bool has_geometry_stage = false;
};

class MetalShaderConverter {
 public:
  MetalShaderConverter();
  ~MetalShaderConverter();

  bool Initialize();

  void SetMinimumTarget(uint32_t gpu_family, uint32_t os,
                        const std::string& version);

  bool Convert(xenos::ShaderType shader_type,
               const std::vector<uint8_t>& dxil_data,
               MetalShaderConversionResult& result);

  bool ConvertWithStage(MetalShaderStage stage,
                        const std::vector<uint8_t>& dxil_data,
                        MetalShaderConversionResult& result);

  bool ConvertWithStageEx(MetalShaderStage stage,
                          const std::vector<uint8_t>& dxil_data,
                          MetalShaderConversionResult& result,
                          MetalShaderReflectionInfo* reflection = nullptr,
                          const IRVersionedInputLayoutDescriptor* input_layout = nullptr,
                          std::vector<uint8_t>* stage_in_metallib = nullptr,
                          bool enable_geometry_emulation = false,
                          int input_topology = 0);

  void* CreateXbox360RootSignature(MetalShaderStage stage,
                                    bool force_all_visibility,
                                    bool bindless_resources_used);

 private:
  bool is_available_ = false;
  bool has_minimum_target_ = false;
  uint32_t minimum_gpu_family_ = 0;
  uint32_t minimum_os_ = 0;
  std::string minimum_os_version_;
};

}  // namespace rex::graphics::metal
