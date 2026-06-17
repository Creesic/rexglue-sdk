#pragma once

#include <cstdint>

namespace fm2::native_renderer {

inline constexpr uint32_t kDirectDrawRecordStride = 0x34u;
inline constexpr uint32_t kDirectDrawSegmentStride = 0x08u;

inline constexpr uint32_t kDirectDrawRecordHolderOffset = 0x28u;
inline constexpr uint32_t kDirectDrawRecordStream0Offset = 0x2Cu;
inline constexpr uint32_t kDirectDrawRecordIndexResourceOffset = 0x30u;

inline constexpr uint32_t kDirectDrawHolderSegmentBeginOffset = 0x10u;
inline constexpr uint32_t kDirectDrawHolderSegmentEndOffset = 0x14u;
inline constexpr uint32_t kDirectDrawHolderStream0Offset = 0x48u;
inline constexpr uint32_t kDirectDrawHolderIndexResourceOffset = 0x54u;

inline constexpr uint32_t kDirectDrawSegmentStartOffset = 0x04u;
inline constexpr uint32_t kDirectDrawSegmentIndexCountOffset = 0x06u;

inline constexpr uint32_t kDirectDrawCtxBuiltOffset = 0x48u;
inline constexpr uint32_t kDirectDrawCtxVertexShaderHandleOffset = 0x4Cu;
inline constexpr uint32_t kDirectDrawCtxSlot28StateHandleOffset = 0x6Cu;
inline constexpr uint32_t kDirectDrawCtxRecordBeginOffset = 0x5A4u;
inline constexpr uint32_t kDirectDrawCtxRecordEndOffset = 0x5A8u;
inline constexpr uint32_t kDirectDrawCtxStream1Offset = 0x5B0u;

inline constexpr uint32_t kDirectDrawResourceDescriptorSize = 0x0Cu;

inline constexpr uint32_t kD3DResourceGpuBaseOffset = 0x18u;
inline constexpr uint32_t kD3DResourceSizeOffset = 0x1Cu;
inline constexpr uint32_t kD3DResourceDecodeSize = 0x20u;

inline constexpr uint32_t kDirectDrawStateHandleResolvedObjectOffset = 0x48u;
inline constexpr uint32_t kDirectDrawVertexShaderTypeTag = 0x00100006u;
inline constexpr uint32_t kDirectDrawPixelShaderTypeTag = 0x00100007u;
inline constexpr uint32_t kDirectDrawPixelShaderPayloadGpuBaseOffset = 0x18u;
inline constexpr uint32_t kDirectDrawVertexShaderPayloadGpuBaseOffset = 0x20u;
inline constexpr uint32_t kDirectDrawPixelShaderPayloadUcodeOffset = 0x00u;
inline constexpr uint32_t kDirectDrawVertexShaderPayloadUcodeOffset = 0x30u;
inline constexpr uint32_t kDirectDrawPixelShaderPayloadByteCountOffset = 0x30u;
inline constexpr uint32_t kDirectDrawShaderByteDumpMax = 256u;
inline constexpr uint32_t kDirectDrawSlot28StateTableBaseOffset = 0x28u;
inline constexpr uint32_t kDirectDrawSlot28StateTableOffsetField = 0x3Cu;
inline constexpr uint32_t kDirectDrawVertexShaderTableBaseOffset = 0x368u;
inline constexpr uint32_t kDirectDrawVertexShaderTableOffsetField = 0x37Cu;
inline constexpr uint32_t kDirectDrawCompiledStateHeaderSize = 0x14u;

constexpr uint32_t BoundedVectorCount(uint32_t begin, uint32_t end, uint32_t stride,
                                      uint32_t cap) {
  if (begin == 0 || end <= begin || stride == 0) {
    return 0;
  }
  const uint32_t count = (end - begin) / stride;
  return count > cap ? cap : count;
}

constexpr uint32_t TriangleListPrimitiveCountFromIndexCount(uint32_t index_count) {
  return index_count / 3u;
}

constexpr uint32_t DirectDrawShaderPayloadGpuBaseOffsetForType(uint32_t shader_type) {
  switch (shader_type) {
    case kDirectDrawVertexShaderTypeTag:
      return kDirectDrawVertexShaderPayloadGpuBaseOffset;
    case kDirectDrawPixelShaderTypeTag:
      return kDirectDrawPixelShaderPayloadGpuBaseOffset;
    default:
      return 0;
  }
}

constexpr uint32_t DirectDrawShaderPayloadUcodeOffsetForType(uint32_t shader_type) {
  switch (shader_type) {
    case kDirectDrawVertexShaderTypeTag:
      return kDirectDrawVertexShaderPayloadUcodeOffset;
    case kDirectDrawPixelShaderTypeTag:
      return kDirectDrawPixelShaderPayloadUcodeOffset;
    default:
      return 0;
  }
}

constexpr uint32_t BoundedShaderPayloadDumpByteCount(uint32_t requested_byte_count,
                                                     uint32_t known_payload_byte_count) {
  uint32_t byte_count = requested_byte_count;
  if (byte_count > kDirectDrawShaderByteDumpMax) {
    byte_count = kDirectDrawShaderByteDumpMax;
  }
  if (known_payload_byte_count != 0 && byte_count > known_payload_byte_count) {
    byte_count = known_payload_byte_count;
  }
  return byte_count;
}

constexpr uint32_t BoundedShaderUcodeDumpByteCount(uint32_t requested_byte_count,
                                                   uint32_t known_payload_byte_count,
                                                   uint32_t ucode_payload_offset) {
  uint32_t byte_count = requested_byte_count;
  if (byte_count > kDirectDrawShaderByteDumpMax) {
    byte_count = kDirectDrawShaderByteDumpMax;
  }
  if (known_payload_byte_count == 0) {
    return byte_count;
  }
  if (known_payload_byte_count <= ucode_payload_offset) {
    return 0;
  }
  const uint32_t known_ucode_byte_count = known_payload_byte_count - ucode_payload_offset;
  return byte_count > known_ucode_byte_count ? known_ucode_byte_count : byte_count;
}

}  // namespace fm2::native_renderer
