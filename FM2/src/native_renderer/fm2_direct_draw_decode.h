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
inline constexpr uint32_t kDirectDrawCtxRecordBeginOffset = 0x5A4u;
inline constexpr uint32_t kDirectDrawCtxRecordEndOffset = 0x5A8u;
inline constexpr uint32_t kDirectDrawCtxStream1Offset = 0x5B0u;

inline constexpr uint32_t kDirectDrawResourceDescriptorSize = 0x0Cu;

inline constexpr uint32_t kD3DResourceGpuBaseOffset = 0x18u;
inline constexpr uint32_t kD3DResourceSizeOffset = 0x1Cu;
inline constexpr uint32_t kD3DResourceDecodeSize = 0x20u;

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

}  // namespace fm2::native_renderer
