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
  CHECK(decode::kDirectDrawVertexShaderTypeTag == 0x00100006u);
  CHECK(decode::kDirectDrawPixelShaderTypeTag == 0x00100007u);
  CHECK(decode::kDirectDrawPixelShaderPayloadGpuBaseOffset == 0x18u);
  CHECK(decode::kDirectDrawVertexShaderPayloadGpuBaseOffset == 0x20u);
  CHECK(decode::kDirectDrawShaderByteDumpMax == 1024u);
  CHECK(decode::kDirectDrawSlot28StateTableBaseOffset == 0x28u);
  CHECK(decode::kDirectDrawSlot28StateTableOffsetField == 0x3Cu);
  CHECK(decode::kDirectDrawVertexShaderTableBaseOffset == 0x368u);
  CHECK(decode::kDirectDrawVertexShaderTableOffsetField == 0x37Cu);
  CHECK(decode::kDirectDrawCompiledStateHeaderSize == 0x14u);
}

TEST_CASE("FM2 direct draw shader payload layout matches runtime evidence",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK(decode::kDirectDrawVertexShaderPayloadUcodeOffset == 0x30u);
  CHECK(decode::kDirectDrawPixelShaderPayloadUcodeOffset == 0x00u);
  CHECK(decode::kDirectDrawPixelShaderPayloadByteCountOffset == 0x30u);
  CHECK(decode::kDirectDrawVertexShaderPayloadByteCountCandidateOffset == 0x30u);
  CHECK(decode::kDirectDrawShaderByteDumpMax == 1024u);

  CHECK(decode::DirectDrawShaderPayloadGpuBaseOffsetForType(
            decode::kDirectDrawVertexShaderTypeTag) ==
        decode::kDirectDrawVertexShaderPayloadGpuBaseOffset);
  CHECK(decode::DirectDrawShaderPayloadGpuBaseOffsetForType(
            decode::kDirectDrawPixelShaderTypeTag) ==
        decode::kDirectDrawPixelShaderPayloadGpuBaseOffset);
  CHECK(decode::DirectDrawShaderPayloadGpuBaseOffsetForType(0xDEADBEEFu) == 0u);

  CHECK(decode::DirectDrawShaderPayloadUcodeOffsetForType(
            decode::kDirectDrawVertexShaderTypeTag) ==
        decode::kDirectDrawVertexShaderPayloadUcodeOffset);
  CHECK(decode::DirectDrawShaderPayloadUcodeOffsetForType(
            decode::kDirectDrawPixelShaderTypeTag) ==
        decode::kDirectDrawPixelShaderPayloadUcodeOffset);
  CHECK(decode::DirectDrawShaderPayloadUcodeOffsetForType(0xDEADBEEFu) == 0u);

  CHECK(decode::DirectDrawLittleEndianValueFromGuestDword(0x08030000u) ==
        0x00000308u);
  CHECK(decode::DirectDrawLittleEndianValueFromGuestDword(0x00000000u) ==
        0x00000000u);
}

TEST_CASE("FM2 direct draw shader byte count bounds respect known payload sizes",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK(decode::BoundedShaderPayloadDumpByteCount(256u, 0u) == 256u);
  CHECK(decode::BoundedShaderPayloadDumpByteCount(2048u, 0u) ==
        decode::kDirectDrawShaderByteDumpMax);
  CHECK(decode::BoundedShaderPayloadDumpByteCount(256u, 0x90u) == 0x90u);
  CHECK(decode::BoundedShaderPayloadDumpByteCount(0x40u, 0x90u) == 0x40u);

  CHECK(decode::BoundedShaderUcodeDumpByteCount(2048u, 0u, 0x30u) ==
        decode::kDirectDrawShaderByteDumpMax);
  CHECK(decode::BoundedShaderUcodeDumpByteCount(256u, 0x90u, 0x00u) == 0x90u);
  CHECK(decode::BoundedShaderUcodeDumpByteCount(256u, 0x90u, 0x30u) == 0x60u);
  CHECK(decode::BoundedShaderUcodeDumpByteCount(256u, 0x20u, 0x30u) == 0u);
}

TEST_CASE("FM2 direct draw triangle segment count maps to primitive count", "[fm2][plume]") {
  using fm2::native_renderer::TriangleListPrimitiveCountFromIndexCount;

  CHECK(TriangleListPrimitiveCountFromIndexCount(0u) == 0u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(2u) == 0u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(3u) == 1u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(4062u) == 1354u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(4158u) == 1386u);
}
