#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif
#include <rex/graphics/pipeline/shader/shader.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#include <rex/string/buffer.h>

namespace fm2::native_renderer {

inline constexpr uint32_t kDirectDrawShaderAnalysisMaxAttributes = 16u;

struct DirectDrawVertexFetchAttributeSummary {
  uint32_t binding_index = 0;
  uint32_t attribute_index = 0;
  uint32_t fetch_constant = 0;
  uint32_t stride_words = 0;
  int32_t offset_words = 0;
  uint32_t data_format = 0;
  uint32_t source_register = 0;
  uint32_t destination_register = 0;
  uint32_t write_mask = 0;
  bool is_mini_fetch = false;
  bool is_signed = false;
  bool is_integer = false;
};

struct DirectDrawVertexShaderAnalysisSummary {
  bool valid = false;
  bool attributes_truncated = false;
  uint32_t ucode_dword_count = 0;
  uint32_t cf_pair_index_bound = 0;
  uint32_t binding_count = 0;
  uint32_t attribute_count = 0;
  uint32_t vertex_fetch_bitmap[3] = {};
  DirectDrawVertexFetchAttributeSummary
      attributes[kDirectDrawShaderAnalysisMaxAttributes] = {};
};

inline DirectDrawVertexShaderAnalysisSummary AnalyzeDirectDrawVertexShaderUcode(
    const uint32_t* host_order_ucode_dwords, uint32_t ucode_dword_count) {
  DirectDrawVertexShaderAnalysisSummary summary;
  summary.ucode_dword_count = ucode_dword_count;
  if (!host_order_ucode_dwords || ucode_dword_count == 0) {
    return summary;
  }

  rex::graphics::Shader shader(
      rex::graphics::xenos::ShaderType::kVertex, 0, host_order_ucode_dwords,
      ucode_dword_count, std::endian::native);
  rex::string::StringBuffer disassembly_buffer;
  shader.AnalyzeUcode(disassembly_buffer);

  summary.valid = shader.is_ucode_analyzed();
  summary.cf_pair_index_bound = shader.cf_pair_index_bound();
  const auto& constant_map = shader.constant_register_map();
  summary.vertex_fetch_bitmap[0] = constant_map.vertex_fetch_bitmap[0];
  summary.vertex_fetch_bitmap[1] = constant_map.vertex_fetch_bitmap[1];
  summary.vertex_fetch_bitmap[2] = constant_map.vertex_fetch_bitmap[2];

  const auto& vertex_bindings = shader.vertex_bindings();
  summary.binding_count = static_cast<uint32_t>(
      std::min<size_t>(vertex_bindings.size(), UINT32_MAX));

  for (const auto& binding : vertex_bindings) {
    for (size_t attribute_index = 0; attribute_index < binding.attributes.size();
         ++attribute_index) {
      const uint32_t output_index = summary.attribute_count++;
      if (output_index >= kDirectDrawShaderAnalysisMaxAttributes) {
        summary.attributes_truncated = true;
        continue;
      }

      const auto& attribute = binding.attributes[attribute_index];
      const auto& fetch = attribute.fetch_instr;
      auto& out = summary.attributes[output_index];
      out.binding_index = static_cast<uint32_t>(binding.binding_index);
      out.attribute_index = static_cast<uint32_t>(attribute_index);
      out.fetch_constant = binding.fetch_constant;
      out.stride_words = binding.stride_words;
      out.offset_words = fetch.attributes.offset;
      out.data_format = static_cast<uint32_t>(fetch.attributes.data_format);
      out.source_register = fetch.operands[0].storage_index;
      out.destination_register = fetch.result.storage_index;
      out.write_mask = fetch.result.GetUsedWriteMask();
      out.is_mini_fetch = fetch.is_mini_fetch;
      out.is_signed = fetch.attributes.is_signed;
      out.is_integer = fetch.attributes.is_integer;
    }
  }

  return summary;
}

}  // namespace fm2::native_renderer
