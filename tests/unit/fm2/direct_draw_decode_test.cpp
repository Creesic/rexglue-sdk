#include <catch2/catch_test_macros.hpp>

#include "native_renderer/fm2_direct_draw_decode.h"

TEST_CASE("FM2 direct draw vector count is bounded and stride-aware", "[fm2][plume]") {
  using fm2::native_renderer::BoundedVectorCount;

  CHECK(BoundedVectorCount(0x1000u, 0x1034u, 0x34u, 16u) == 1u);
  CHECK(BoundedVectorCount(0x1000u, 0x1000u, 0x34u, 16u) == 0u);
  CHECK(BoundedVectorCount(0x1034u, 0x1000u, 0x34u, 16u) == 0u);
  CHECK(BoundedVectorCount(0x1000u, 0x2000u, 0x34u, 2u) == 2u);
  CHECK(BoundedVectorCount(0xFFFFfff0u, 0x10u, 0x34u, 16u) == 0u);
}

TEST_CASE("FM2 direct draw descriptor offsets match IDA evidence", "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK(decode::kDirectDrawRecordStride == 0x34u);
  CHECK(decode::kDirectDrawSegmentStride == 0x08u);
  CHECK(decode::kDirectDrawRecordHolderOffset == 0x28u);
  CHECK(decode::kDirectDrawRecordStream0Offset == 0x2Cu);
  CHECK(decode::kDirectDrawRecordIndexResourceOffset == 0x30u);
  CHECK(decode::kDirectDrawHolderStream0Offset == 0x48u);
  CHECK(decode::kDirectDrawHolderIndexResourceOffset == 0x54u);
  CHECK(decode::kDirectDrawResourceDescriptorSize == 0x0Cu);
  CHECK(decode::kDirectDrawCtxStream1Offset == 0x5B0u);
  CHECK(decode::kD3DResourceGpuBaseOffset == 0x18u);
  CHECK(decode::kD3DResourceSizeOffset == 0x1Cu);
  CHECK(decode::kD3DResourceDecodeSize == 0x20u);
}

TEST_CASE("FM2 direct draw shader state offsets match IDA evidence", "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK(decode::kDirectDrawCtxVertexShaderHandleOffset == 0x4Cu);
  CHECK(decode::kDirectDrawCtxSlot28StateHandleOffset == 0x6Cu);
  CHECK(decode::kDirectDrawStateHandleResolvedObjectOffset == 0x48u);
  CHECK(decode::kDirectDrawSlot28StateTableBaseOffset == 0x28u);
  CHECK(decode::kDirectDrawSlot28StateTableOffsetField == 0x3Cu);
  CHECK(decode::kDirectDrawVertexShaderTableBaseOffset == 0x368u);
  CHECK(decode::kDirectDrawVertexShaderTableOffsetField == 0x37Cu);
  CHECK(decode::kDirectDrawCompiledStateHeaderSize == 0x14u);
}

TEST_CASE("FM2 direct draw triangle segment count maps to primitive count", "[fm2][plume]") {
  using fm2::native_renderer::TriangleListPrimitiveCountFromIndexCount;

  CHECK(TriangleListPrimitiveCountFromIndexCount(0u) == 0u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(2u) == 0u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(3u) == 1u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(4062u) == 1354u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(4158u) == 1386u);
}
