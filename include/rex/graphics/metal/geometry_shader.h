#pragma once

#include <cstdint>
#include <vector>

#include <rex/graphics/pipeline/shader/dxbc_translator.h>

namespace rex::graphics::metal {

enum PipelineGeometryShader : int {
  kNone = 0,
  kPointList,
  kRectangleList,
  kQuadList,
};

struct GeometryShaderKey {
  union {
    uint64_t key = 0;
    struct {
      uint32_t type : 3;
      uint32_t interpolator_count : 5;
      uint32_t user_clip_plane_count : 3;
      uint32_t user_clip_plane_cull : 1;
      uint32_t has_vertex_kill_and : 1;
      uint32_t has_point_size : 1;
      uint32_t has_point_coordinates : 1;
    };
  };
  bool operator==(const GeometryShaderKey& other) const {
    return key == other.key;
  }
  struct Hasher {
    size_t operator()(const GeometryShaderKey& k) const {
      return std::hash<uint64_t>{}(k.key);
    }
  };
};

bool GetGeometryShaderKey(PipelineGeometryShader geometry_shader_type,
                          DxbcShaderTranslator::Modification vertex_shader_modification,
                          DxbcShaderTranslator::Modification pixel_shader_modification,
                          GeometryShaderKey& key_out);

const std::vector<uint32_t>& GetGeometryShader(GeometryShaderKey key);

}  // namespace rex::graphics::metal
