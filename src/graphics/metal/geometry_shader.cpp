#include <rex/graphics/metal/geometry_shader.h>

#include <rex/logging/macros.h>

namespace rex {
namespace graphics {
namespace metal {

bool GetGeometryShaderKey(
    PipelineGeometryShader geometry_shader_type,
    DxbcShaderTranslator::Modification vertex_shader_modification,
    DxbcShaderTranslator::Modification pixel_shader_modification,
    GeometryShaderKey& key_out) {
  key_out.key = 0;
  key_out.type = geometry_shader_type;
  return geometry_shader_type != PipelineGeometryShader::kNone;
}

const std::vector<uint32_t>& GetGeometryShader(GeometryShaderKey key) {
  static std::vector<uint32_t> empty;
  REXLOG_WARN("MetalGeometryShader: GetGeometryShader stub called");
  return empty;
}

}  // namespace metal
}  // namespace graphics
}  // namespace rex
