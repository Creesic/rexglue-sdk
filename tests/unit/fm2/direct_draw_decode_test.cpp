#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "native_renderer/fm2_direct_draw_decode.h"
#include "native_renderer/fm2_native_state.h"
#include "native_renderer/fm2_shader_analysis.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <rex/graphics/xenos.h>

namespace {

constexpr uint32_t MakeCfExecDword0(uint32_t address, uint32_t count,
                                    uint32_t sequence = 0) {
  return (address & 0xFFFu) | ((count & 0x7u) << 12) |
         ((sequence & 0xFFFu) << 16);
}

constexpr uint32_t MakeCfDword1(uint32_t opcode) {
  return (opcode & 0xFu) << 12;
}

template <std::size_t N>
void StoreCfPair(std::array<uint32_t, N>& ucode, uint32_t pair_index,
                 uint32_t a_dword0, uint32_t a_dword1, uint32_t b_dword0,
                 uint32_t b_dword1) {
  const uint32_t offset = pair_index * 3u;
  ucode[offset + 0] = a_dword0;
  ucode[offset + 1] = (a_dword1 & 0xFFFFu) | (b_dword0 << 16);
  ucode[offset + 2] = (b_dword0 >> 16) | ((b_dword1 & 0xFFFFu) << 16);
}

template <std::size_t N>
void StoreCfPairAt(std::array<uint32_t, N>& ucode, uint32_t dword_offset,
                   uint32_t pair_index, uint32_t a_dword0, uint32_t a_dword1,
                   uint32_t b_dword0, uint32_t b_dword1) {
  const uint32_t offset = dword_offset + pair_index * 3u;
  ucode[offset + 0] = a_dword0;
  ucode[offset + 1] = (a_dword1 & 0xFFFFu) | (b_dword0 << 16);
  ucode[offset + 2] = (b_dword0 >> 16) | ((b_dword1 & 0xFFFFu) << 16);
}

template <std::size_t N>
void StoreValidUcodeAt(std::array<uint32_t, N>& payload, uint32_t dword_offset) {
  StoreCfPairAt(payload, dword_offset, 0, 0u, MakeCfDword1(0u),
                MakeCfExecDword0(4u, 2u), MakeCfDword1(1u));
  StoreCfPairAt(payload, dword_offset, 2, MakeCfExecDword0(7u, 3u),
                MakeCfDword1(2u), 0u, MakeCfDword1(0u));
}

template <std::size_t N>
void StoreTinyValidUcodeAt(std::array<uint32_t, N>& payload, uint32_t dword_offset) {
  StoreCfPairAt(payload, dword_offset, 0, MakeCfExecDword0(2u, 0u),
                MakeCfDword1(1u), MakeCfExecDword0(2u, 0u),
                MakeCfDword1(2u));
}

constexpr uint32_t MakeFetchDestSwizzle(uint32_t x, uint32_t y, uint32_t z,
                                        uint32_t w) {
  return (x & 0x7u) | ((y & 0x7u) << 3) | ((z & 0x7u) << 6) |
         ((w & 0x7u) << 9);
}

constexpr void StoreVertexFetchInstruction(
    uint32_t* out, uint32_t fetch_constant, uint32_t source_register,
    uint32_t destination_register, rex::graphics::xenos::VertexFormat format,
    uint32_t stride_words, int32_t offset_words) {
  out[0] = ((source_register & 0x3Fu) << 5) |
           ((destination_register & 0x3Fu) << 12) | (1u << 19) |
           (((fetch_constant / 3u) & 0x1Fu) << 20) |
           (((fetch_constant % 3u) & 0x3u) << 25);
  out[1] = MakeFetchDestSwizzle(0u, 1u, 2u, 3u) |
           ((static_cast<uint32_t>(format) & 0x3Fu) << 16);
  out[2] = (stride_words & 0xFFu) |
           ((static_cast<uint32_t>(offset_words) & 0x7FFFFFu) << 8);
}

template <std::size_t N>
constexpr void StoreBE16(std::array<uint8_t, N>& bytes, uint32_t offset,
                         uint16_t value) {
  bytes[offset + 0u] = static_cast<uint8_t>(value >> 8);
  bytes[offset + 1u] = static_cast<uint8_t>(value);
}

template <std::size_t N>
constexpr void StoreBE32(std::array<uint8_t, N>& bytes, uint32_t offset,
                         uint32_t value) {
  bytes[offset + 0u] = static_cast<uint8_t>(value >> 24);
  bytes[offset + 1u] = static_cast<uint8_t>(value >> 16);
  bytes[offset + 2u] = static_cast<uint8_t>(value >> 8);
  bytes[offset + 3u] = static_cast<uint8_t>(value);
}

}  // namespace

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
  CHECK(decode::kD3DResourceByteSizeMask == 0x0FFFFFFFu);
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
  CHECK(decode::kDirectDrawShaderByteDumpMax == 4096u);
  CHECK(decode::kDirectDrawSlot28StateTableBaseOffset == 0x28u);
  CHECK(decode::kDirectDrawSlot28StateTableOffsetField == 0x3Cu);
  CHECK(decode::kDirectDrawVertexShaderTableBaseOffset == 0x368u);
  CHECK(decode::kDirectDrawVertexShaderTableOffsetField == 0x37Cu);
  CHECK(decode::kDirectDrawVertexShaderTablePayloadByteCountOffset == 0x2Cu);
  CHECK(decode::kDirectDrawCompiledStateHeaderSize == 0x14u);
  CHECK(decode::kDirectDrawStateByteDumpMax == 1024u);
  CHECK(decode::kDirectDrawCompiledStateMaxEntries == 16u);
  CHECK(decode::kD3DCommandContextStateShadowOffset == 0x400u);
  CHECK(decode::kD3DCommandContextStateShadowByteCount == 0x300u);
  CHECK(decode::DirectDrawCompiledStateTrailingEntryOffset(0x14u) == 0x28u);
  CHECK(decode::DirectDrawCompiledStateTrailingEntryOffset(0u) == 0u);
}

TEST_CASE("FM2 D3D context trace window can skip noisy startup samples",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK_FALSE(decode::ShouldTraceD3DCommandContextSample(0u, 0u, 0u));
  CHECK(decode::ShouldTraceD3DCommandContextSample(0u, 0u, 1u));
  CHECK_FALSE(decode::ShouldTraceD3DCommandContextSample(1u, 0u, 1u));

  CHECK_FALSE(decode::ShouldTraceD3DCommandContextSample(99u, 100u, 2u));
  CHECK(decode::ShouldTraceD3DCommandContextSample(100u, 100u, 2u));
  CHECK(decode::ShouldTraceD3DCommandContextSample(101u, 100u, 2u));
  CHECK_FALSE(decode::ShouldTraceD3DCommandContextSample(102u, 100u, 2u));
}

TEST_CASE("FM2 direct draw trace window can skip startup samples",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK_FALSE(decode::ShouldTraceDirectDrawSample(0u, 0u, 0u));
  CHECK(decode::ShouldTraceDirectDrawSample(0u, 0u, 1u));
  CHECK_FALSE(decode::ShouldTraceDirectDrawSample(1u, 0u, 1u));

  CHECK_FALSE(decode::ShouldTraceDirectDrawSample(7u, 8u, 3u));
  CHECK(decode::ShouldTraceDirectDrawSample(8u, 8u, 3u));
  CHECK(decode::ShouldTraceDirectDrawSample(10u, 8u, 3u));
  CHECK_FALSE(decode::ShouldTraceDirectDrawSample(11u, 8u, 3u));
}

TEST_CASE("FM2 direct debug replay can filter records", "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK(decode::ShouldSubmitDirectDebugReplayRecord(
      3u, decode::kDirectDrawReplayAnyRecordIndex));
  CHECK(decode::ShouldSubmitDirectDebugReplayRecord(3u, 3u));
  CHECK_FALSE(decode::ShouldSubmitDirectDebugReplayRecord(2u, 3u));
}

TEST_CASE("FM2 compare replay policy bypasses trace-only debug limits",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK(decode::ShouldDecodeDirectDrawForCompareReplay(false, true));
  CHECK(decode::ShouldDecodeDirectDrawForCompareReplay(true, false));
  CHECK_FALSE(decode::ShouldDecodeDirectDrawForCompareReplay(false, false));

  CHECK(decode::DirectCompareReplayRecordScanCount(31u, 0u) == 31u);
  CHECK(decode::DirectCompareReplayRecordScanCount(31u, 8u) == 8u);
  CHECK(decode::DirectCompareReplayRecordScanCount(4u, 8u) == 4u);

  CHECK_FALSE(decode::DirectDebugReplaySubmitLimitReached(2u, 1u, true));
  CHECK(decode::DirectDebugReplaySubmitLimitReached(2u, 1u, false));
  CHECK_FALSE(decode::DirectDebugReplaySubmitLimitReached(99u, 0u, false));
}

TEST_CASE("FM2 native direct draw policy bypasses trace-only debug limits",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK(decode::ShouldDecodeDirectDrawForPlumeSubmission(false, false, true));
  CHECK(decode::ShouldDecodeDirectDrawForPlumeSubmission(false, true, false));
  CHECK(decode::ShouldDecodeDirectDrawForPlumeSubmission(true, false, false));
  CHECK_FALSE(
      decode::ShouldDecodeDirectDrawForPlumeSubmission(false, false, false));

  CHECK(decode::DirectPlumeSubmissionRecordScanCount(31u, 4u, false, true) ==
        31u);
  CHECK(decode::DirectPlumeSubmissionRecordScanCount(31u, 4u, true, false) ==
        31u);
  CHECK(decode::DirectPlumeSubmissionRecordScanCount(31u, 4u, false, false) ==
        4u);
  CHECK(decode::DirectPlumeSubmissionRecordScanCount(3u, 4u, false, false) ==
        3u);

  CHECK(decode::ShouldStopDirectPlumeRecordScanAfterNativeAttempt(true, false,
                                                                  false));
  CHECK_FALSE(
      decode::ShouldStopDirectPlumeRecordScanAfterNativeAttempt(false, false,
                                                                false));
  CHECK_FALSE(
      decode::ShouldStopDirectPlumeRecordScanAfterNativeAttempt(true, true,
                                                                false));
  CHECK_FALSE(
      decode::ShouldStopDirectPlumeRecordScanAfterNativeAttempt(true, false,
                                                                true));

  CHECK(decode::NativeDirectDrawLiveBatchSize(0u) == 1u);
  CHECK(decode::NativeDirectDrawLiveBatchSize(8u) == 8u);
  CHECK_FALSE(decode::ShouldFlushNativeDirectDrawLiveBatch(0u, 8u));
  CHECK_FALSE(decode::ShouldFlushNativeDirectDrawLiveBatch(7u, 8u));
  CHECK(decode::ShouldFlushNativeDirectDrawLiveBatch(8u, 8u));
  CHECK(decode::ShouldFlushNativeDirectDrawLiveBatch(1u, 0u));

  CHECK(decode::ShouldPromoteDirectReplayToNativeLayout(true, false, false));
  CHECK(decode::ShouldPromoteDirectReplayToNativeLayout(false, true, false));
  CHECK(decode::ShouldPromoteDirectReplayToNativeLayout(false, true, true));
  CHECK(decode::ShouldPromoteDirectReplayToNativeLayout(true, true, true));
}

TEST_CASE("FM2 direct draw interface filter selects the bound record",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  const decode::DirectDrawLiveDrawFilter filter =
      decode::BuildDirectDrawLiveDrawFilter(4u, 0u, 1354u, 0x2E0F1400u,
                                            0x2E0F14E0u);

  REQUIRE(filter.enabled);
  CHECK(filter.topology == decode::DirectDrawReplayTopology::kTriangleList);
  CHECK(filter.start_index == 0u);
  CHECK(filter.index_count == 4062u);
  CHECK(filter.stream0_resource == 0x2E0F1400u);
  CHECK(filter.index_resource == 0x2E0F14E0u);

  decode::DirectDrawSegmentSummary segment;
  segment.valid = true;
  segment.start_index = 0u;
  segment.index_count = 4062u;

  CHECK(decode::DirectDrawLiveDrawFilterMatchesRecord(
      filter, 0x2E0F1400u, 0x2E0F14E0u, segment));
  CHECK_FALSE(decode::DirectDrawLiveDrawFilterMatchesRecord(
      filter, 0x2E0F16A0u, 0x2E0F14E0u, segment));
  CHECK_FALSE(decode::DirectDrawLiveDrawFilterMatchesRecord(
      filter, 0x2E0F1400u, 0x2E0F1900u, segment));

  segment.start_index = 6u;
  CHECK_FALSE(decode::DirectDrawLiveDrawFilterMatchesRecord(
      filter, 0x2E0F1400u, 0x2E0F14E0u, segment));
}

TEST_CASE("FM2 direct debug replay topology names parse diagnostics",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK(decode::ParseDirectDebugReplayPipelineTopology("triangle_list") ==
        decode::DirectDebugReplayPipelineTopology::kTriangleList);
  CHECK(decode::ParseDirectDebugReplayPipelineTopology("triangle_strip") ==
        decode::DirectDebugReplayPipelineTopology::kTriangleStrip);
  CHECK(decode::ParseDirectDebugReplayPipelineTopology("auto") ==
        decode::DirectDebugReplayPipelineTopology::kAuto);
  CHECK(decode::ParseDirectDebugReplayPipelineTopology("bad") ==
        decode::DirectDebugReplayPipelineTopology::kAuto);
}

TEST_CASE("FM2 direct draw segment summary preserves raw header fields",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint8_t, decode::kDirectDrawSegmentStride> bytes = {
      0x12, 0x34, 0xAB, 0xCD, 0x00, 0x10, 0x01, 0x20};
  const auto summary = decode::DecodeDirectDrawSegmentSummary(bytes.data(),
                                                              bytes.size());

  REQUIRE(summary.valid);
  CHECK(summary.raw_w0 == 0x1234u);
  CHECK(summary.raw_w2 == 0xABCDu);
  CHECK(summary.start_index == 0x10u);
  CHECK(summary.index_count == 0x120u);

  CHECK(decode::DirectDrawReplayTopologyFromSegmentHeader(0x8210u, 0x94E8u) ==
        decode::DirectDrawReplayTopology::kTriangleStrip);
  CHECK(decode::DirectDrawReplayTopologyFromSegmentSummary(summary) ==
        decode::DirectDrawReplayTopology::kTriangleList);
  CHECK(decode::ResolveDirectDrawReplayTopology(
            decode::DirectDrawReplayTopology::kUnknown, 0u) ==
        decode::DirectDrawReplayTopology::kUnknown);
  CHECK(decode::ResolveDirectDrawReplayTopology(
            decode::DirectDrawReplayTopology::kUnknown, 6u) ==
        decode::DirectDrawReplayTopology::kTriangleList);
  CHECK(decode::ResolveDirectDrawReplayTopology(
            decode::DirectDrawReplayTopology::kTriangleStrip, 6u) ==
        decode::DirectDrawReplayTopology::kTriangleStrip);
}

TEST_CASE("FM2 D3D context post-direct trace arms once with a nonzero budget",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK_FALSE(decode::ShouldArmD3DCommandContextAfterDirectTrace(0u, false));
  CHECK(decode::ShouldArmD3DCommandContextAfterDirectTrace(4u, false));
  CHECK_FALSE(decode::ShouldArmD3DCommandContextAfterDirectTrace(4u, true));

  CHECK_FALSE(decode::ShouldTraceD3DCommandContextAfterDirectSample(0u));
  CHECK(decode::ShouldTraceD3DCommandContextAfterDirectSample(1u));
}

TEST_CASE("FM2 D3D context state shadow decodes observed texture fetch groups",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint8_t, 64> shadow = {};
  StoreBE32(shadow, 0x00u, 0x84024802u);
  StoreBE32(shadow, 0x04u, 0x10E78052u);
  StoreBE32(shadow, 0x08u, 0x003FE1FFu);
  StoreBE32(shadow, 0x0Cu, 0x00A80D10u);
  StoreBE32(shadow, 0x10u, 0x00000243u);
  StoreBE32(shadow, 0x14u, 0x10E98A18u);
  StoreBE32(shadow, 0x18u, 0x82024802u);
  StoreBE32(shadow, 0x1Cu, 0x09DD6096u);
  StoreBE32(shadow, 0x20u, 0x001FE0FFu);
  StoreBE32(shadow, 0x24u, 0x00A81690u);
  StoreBE32(shadow, 0x28u, 0x00000003u);
  StoreBE32(shadow, 0x2Cu, 0x00000218u);

  const auto group0 = decode::DecodeD3DCommandContextFetchGroup(
      shadow.data(), static_cast<uint32_t>(shadow.size()), 0u);
  REQUIRE(group0.valid);
  CHECK(group0.group_index == 0u);
  CHECK(group0.register_base == 0x4800u);
  CHECK(group0.kind == decode::D3DCommandContextFetchGroupKind::kTexture);
  CHECK(group0.dwords[0] == 0x84024802u);
  CHECK(group0.dwords[5] == 0x10E98A18u);
  CHECK(group0.texture.type == 2u);
  CHECK(group0.texture.format == 0x12u);
  CHECK(group0.texture.endian == 1u);
  CHECK(group0.texture.base_address == 0x10E78000u);
  CHECK(group0.texture.mip_address == 0x10E98000u);
  CHECK(group0.texture.dimension == 1u);

  const auto group1 = decode::DecodeD3DCommandContextFetchGroup(
      shadow.data(), static_cast<uint32_t>(shadow.size()), 1u);
  REQUIRE(group1.valid);
  CHECK(group1.group_index == 1u);
  CHECK(group1.register_base == 0x4806u);
  CHECK(group1.kind == decode::D3DCommandContextFetchGroupKind::kTexture);
  CHECK(group1.texture.base_address == 0x09DD6000u);

  const auto truncated_group2 = decode::DecodeD3DCommandContextFetchGroup(
      shadow.data(), static_cast<uint32_t>(shadow.size()), 2u);
  CHECK_FALSE(truncated_group2.valid);
}

TEST_CASE("FM2 D3D context state shadow decodes vertex fetch triplets",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint8_t, decode::kD3DCommandContextFetchGroupByteCount> shadow = {};
  StoreBE32(shadow, 0x00u, (0x12345u << 2) | 3u);
  StoreBE32(shadow, 0x04u, (0x56u << 2) | 1u);
  StoreBE32(shadow, 0x08u, (0x23456u << 2) | 1u);
  StoreBE32(shadow, 0x0Cu, (0x78u << 2) | 2u);
  StoreBE32(shadow, 0x10u, (0x34567u << 2) | 3u);
  StoreBE32(shadow, 0x14u, (0x9Au << 2) | 3u);

  const auto group = decode::DecodeD3DCommandContextFetchGroup(
      shadow.data(), static_cast<uint32_t>(shadow.size()), 0u);
  REQUIRE(group.valid);
  CHECK(group.kind == decode::D3DCommandContextFetchGroupKind::kVertexTriplet);
  CHECK(group.vertex[0].type == 3u);
  CHECK(group.vertex[0].address_words == 0x12345u);
  CHECK(group.vertex[0].address_bytes == 0x48D14u);
  CHECK(group.vertex[0].endian == 1u);
  CHECK(group.vertex[0].size_words == 0x56u);
  CHECK(group.vertex[0].size_bytes == 0x158u);
  CHECK(decode::D3DCommandContextVertexFetchIsValid(group.vertex[0]));
  CHECK(group.vertex[1].type == 1u);
  CHECK(group.vertex[1].endian == 2u);
  CHECK_FALSE(decode::D3DCommandContextVertexFetchIsValid(group.vertex[1]));
  CHECK(group.vertex[2].type == 3u);
  CHECK(group.vertex[2].endian == 3u);
  CHECK(decode::D3DCommandContextVertexFetchConstantIndex(0u, 2u) == 2u);
  CHECK(decode::D3DCommandContextVertexFetchConstantIndex(31u, 1u) == 94u);
}

TEST_CASE("FM2 D3D vertex fetch constants match normalized replay streams",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  const auto stream1 = decode::BuildDirectDrawBufferViewSummary(
      0xB09BF463u, 0x1012574Au, 0x18746u, 0x0Cu, false, true,
      0x891376055F6CD53Aull);
  const auto replay_stream1 =
      decode::BuildDirectDrawReplayVertexStream(1u, stream1);

  const decode::D3DCommandContextVertexFetchSummary fetch94 = {
      3u, 0x426FD18u, 0x109BF460u, 2u, 300498u, 0x125748u};

  const auto match = decode::MatchDirectDrawReplayStreamToVertexFetch(
      replay_stream1, 31u, 1u, fetch94);
  REQUIRE(match.valid);
  CHECK(match.stream_slot == 1u);
  CHECK(match.fetch_group == 31u);
  CHECK(match.fetch_group_slot == 1u);
  CHECK(match.fetch_constant == 94u);
  CHECK(match.stream_guest_base == 0xB09BF463u);
  CHECK(match.stream_fetch_base == 0x109BF460u);
  CHECK(match.fetch_base == 0x109BF460u);
  CHECK(match.stream_bytes == 0x125748u);
  CHECK(match.fetch_bytes == 0x125748u);

  const auto stream0 = decode::BuildDirectDrawBufferViewSummary(
      0xB0BBF697u, 0x1000FD62u, 0x7EBu, 0x20u, false, true,
      0x088CCFB61631658Cull);
  const auto replay_stream0 =
      decode::BuildDirectDrawReplayVertexStream(0u, stream0);
  CHECK_FALSE(decode::MatchDirectDrawReplayStreamToVertexFetch(
                  replay_stream0, 31u, 1u, fetch94)
                  .valid);

  const decode::D3DCommandContextVertexFetchSummary wrong_size_fetch = {
      3u, 0x426FD18u, 0x109BF460u, 2u, 384u, 0x600u};
  CHECK_FALSE(decode::MatchDirectDrawReplayStreamToVertexFetch(
                  replay_stream1, 31u, 1u, wrong_size_fetch)
                  .valid);
}

TEST_CASE("FM2 direct draw compiled state table scan follows shader setter",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint8_t, 0x12C> table = {};
  StoreBE32(table, 0x04u, 1u);
  StoreBE32(table, 0x10u, 0x14u);
  StoreBE16(table, 0x14u, 0x00FCu);
  StoreBE16(table, 0x16u, 0x0010u);
  StoreBE32(table, 0x18u, 0u);
  StoreBE16(table, 0x1Cu, 0u);
  StoreBE16(table, 0x1Eu, 0u);
  StoreBE16(table, 0x20u, 0u);
  StoreBE16(table, 0x22u, 0u);
  StoreBE16(table, 0x28u, 0u);
  StoreBE16(table, 0x2Au, 0x0040u);
  StoreBE32(table, 0x2Cu, 0x000004ECu);
  StoreBE32(table, 0x30u, 0x0011000Cu);

  const auto summary = decode::AnalyzeDirectDrawCompiledStateTable(
      table.data(), static_cast<uint32_t>(table.size()));

  REQUIRE(summary.valid);
  CHECK(summary.header_w04 == 1u);
  CHECK(summary.declared_payload_bytes == 0x14u);
  CHECK(summary.entry_count == 2u);
  CHECK_FALSE(summary.truncated);

  CHECK(summary.entries[0].section ==
        decode::DirectDrawCompiledStateSection::kSkip);
  CHECK(summary.entries[0].table_offset == 0x14u);
  CHECK(summary.entries[0].target_offset == 0xFCu);
  CHECK(summary.entries[0].dword_count == 0x10u);
  CHECK(summary.entries[0].payload_offset == 0x18u);
  CHECK(summary.entries[0].payload_bytes == 4u);
  CHECK_FALSE(summary.entries[0].truncated);

  CHECK(summary.entries[1].section ==
        decode::DirectDrawCompiledStateSection::kMaskValue);
  CHECK(summary.entries[1].table_offset == 0x28u);
  CHECK(summary.entries[1].target_offset == 0u);
  CHECK(summary.entries[1].dword_count == 0x40u);
  CHECK(summary.entries[1].payload_offset == 0x2Cu);
  CHECK(summary.entries[1].payload_bytes == 0x100u);
  CHECK_FALSE(summary.entries[1].truncated);
}

TEST_CASE("FM2 direct draw compiled state mask/value payload maps fetch registers",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint8_t, 0x80> table = {};
  StoreBE32(table, 0x04u, 1u);
  StoreBE32(table, 0x10u, 0x14u);
  StoreBE16(table, 0x14u, 0x00FCu);
  StoreBE16(table, 0x16u, 0x0010u);
  StoreBE16(table, 0x1Cu, 0u);
  StoreBE16(table, 0x1Eu, 0u);
  StoreBE16(table, 0x20u, 0u);
  StoreBE16(table, 0x22u, 0u);
  StoreBE16(table, 0x28u, 0u);
  StoreBE16(table, 0x2Au, 0x000Eu);
  StoreBE32(table, 0x2Cu, 0x000004ECu);
  StoreBE32(table, 0x30u, 0x0011000Cu);
  StoreBE32(table, 0x54u, 0x0000300Du);
  StoreBE32(table, 0x58u, 0x0000500Eu);
  StoreBE32(table, 0x5Cu, 0x0030100Fu);
  StoreBE32(table, 0x60u, 0x00415011u);

  const auto summary = decode::AnalyzeDirectDrawCompiledStateTable(
      table.data(), static_cast<uint32_t>(table.size()));

  REQUIRE(summary.valid);
  REQUIRE(summary.entry_count == 2u);
  const auto& entry = summary.entries[1];
  REQUIRE(entry.section == decode::DirectDrawCompiledStateSection::kMaskValue);
  CHECK(decode::DirectDrawCompiledStateMaskValuePairCount(entry) == 7u);

  const auto pair0 = decode::DirectDrawReadCompiledStateMaskValuePair(
      table.data(), static_cast<uint32_t>(table.size()), entry, 0u);
  REQUIRE(pair0.valid);
  CHECK(pair0.pair_index == 0u);
  CHECK(pair0.payload_offset == 0x2Cu);
  CHECK(pair0.state_offset == 0u);
  CHECK(pair0.state_dword_index == 0u);
  CHECK(pair0.fetch_register == 0x4800u);
  CHECK(pair0.fetch_group == 0u);
  CHECK(pair0.fetch_group_dword == 0u);
  CHECK(pair0.mask == 0x000004ECu);
  CHECK(pair0.value == 0x0011000Cu);

  const auto pair5 = decode::DirectDrawReadCompiledStateMaskValuePair(
      table.data(), static_cast<uint32_t>(table.size()), entry, 5u);
  REQUIRE(pair5.valid);
  CHECK(pair5.payload_offset == 0x54u);
  CHECK(pair5.state_offset == 0x14u);
  CHECK(pair5.state_dword_index == 5u);
  CHECK(pair5.fetch_register == 0x4805u);
  CHECK(pair5.fetch_group == 0u);
  CHECK(pair5.fetch_group_dword == 5u);
  CHECK(pair5.mask == 0x0000300Du);
  CHECK(pair5.value == 0x0000500Eu);

  const auto pair6 = decode::DirectDrawReadCompiledStateMaskValuePair(
      table.data(), static_cast<uint32_t>(table.size()), entry, 6u);
  REQUIRE(pair6.valid);
  CHECK(pair6.payload_offset == 0x5Cu);
  CHECK(pair6.state_offset == 0x18u);
  CHECK(pair6.state_dword_index == 6u);
  CHECK(pair6.fetch_register == 0x4806u);
  CHECK(pair6.fetch_group == 1u);
  CHECK(pair6.fetch_group_dword == 0u);
  CHECK(pair6.mask == 0x0030100Fu);
  CHECK(pair6.value == 0x00415011u);

  const auto out_of_bounds = decode::DirectDrawReadCompiledStateMaskValuePair(
      table.data(), static_cast<uint32_t>(table.size()), entry, 7u);
  CHECK_FALSE(out_of_bounds.valid);
}

TEST_CASE("FM2 direct draw compiled state table scan reports capped dumps",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint8_t, 64> table = {};
  StoreBE32(table, 0x04u, 1u);
  StoreBE32(table, 0x10u, 0x14u);
  StoreBE16(table, 0x14u, 0x00FCu);
  StoreBE16(table, 0x16u, 0x0010u);
  StoreBE16(table, 0x20u, 0u);
  StoreBE16(table, 0x22u, 0u);
  StoreBE16(table, 0x28u, 0u);
  StoreBE16(table, 0x2Au, 0x0040u);

  const auto summary = decode::AnalyzeDirectDrawCompiledStateTable(
      table.data(), static_cast<uint32_t>(table.size()));

  REQUIRE(summary.valid);
  REQUIRE(summary.entry_count == 2u);
  CHECK(summary.truncated);
  CHECK_FALSE(summary.entries[0].truncated);
  CHECK(summary.entries[1].section ==
        decode::DirectDrawCompiledStateSection::kMaskValue);
  CHECK(summary.entries[1].payload_offset == 0x2Cu);
  CHECK(summary.entries[1].payload_bytes == 0x100u);
  CHECK(summary.entries[1].truncated);
}

TEST_CASE("FM2 direct draw shader payload layout matches runtime evidence",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK(decode::kDirectDrawVertexShaderPayloadUcodeOffset == 0x30u);
  CHECK(decode::kDirectDrawPixelShaderPayloadUcodeOffset == 0x00u);
  CHECK(decode::kDirectDrawPixelShaderPayloadByteCountOffset == 0x30u);
  CHECK(decode::kDirectDrawVertexShaderPayloadByteCountCandidateOffset == 0x30u);
  CHECK(decode::kDirectDrawShaderByteDumpMax == 4096u);
  CHECK(decode::kXenosUcodeInstructionByteStride == 12u);
  CHECK(decode::kXenosUcodeControlFlowPairDwordCount == 3u);

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
  CHECK(decode::BoundedShaderPayloadDumpByteCount(8192u, 0u) ==
        decode::kDirectDrawShaderByteDumpMax);
  CHECK(decode::BoundedShaderPayloadDumpByteCount(256u, 0x90u) == 0x90u);
  CHECK(decode::BoundedShaderPayloadDumpByteCount(0x40u, 0x90u) == 0x40u);

  CHECK(decode::BoundedShaderUcodeDumpByteCount(8192u, 0u, 0x30u) ==
        decode::kDirectDrawShaderByteDumpMax);
  CHECK(decode::BoundedShaderUcodeDumpByteCount(256u, 0x90u, 0x00u) == 0x90u);
  CHECK(decode::BoundedShaderUcodeDumpByteCount(256u, 0x90u, 0x30u) == 0x60u);
  CHECK(decode::BoundedShaderUcodeDumpByteCount(256u, 0x20u, 0x30u) == 0u);
}

TEST_CASE("FM2 direct draw resource view byte counts match trace evidence",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  CHECK(decode::DirectDrawResourceReadableByteCount(0x1000FD62u) == 0xFD62u);
  CHECK(decode::DirectDrawResourceReadableByteCount(0x00004704u) == 0x4704u);

  CHECK(decode::DirectDrawIndexElementByteCount(1u) == 2u);
  CHECK(decode::DirectDrawIndexElementByteCount(2u) == 4u);
  CHECK(decode::DirectDrawIndexElementByteCount(0u) == 0u);

  CHECK(decode::DirectDrawBufferViewByteCount(0x7EBu, 0x20u, false) == 0xFD60u);
  CHECK(decode::DirectDrawBufferViewByteCount(0x2382u, 1u, true) == 0x4704u);
  CHECK(decode::DirectDrawBufferViewByteCount(0xFFFFFFFFu, 2u, false) ==
        0xFFFFFFFFu);

  CHECK(decode::BoundedDirectDrawBufferHashByteCount(0xFD62u, 0xFD60u) ==
        0xFD60u);
  CHECK(decode::BoundedDirectDrawBufferHashByteCount(0x4704u, 0x9999u) ==
        0x4704u);
  CHECK(decode::BoundedDirectDrawBufferHashByteCount(0x12574Au, 0u) ==
        0x12574Au);
  CHECK(decode::DirectDrawReplayUploadGuestBase(0xB0BBF697u) ==
        0xB0BBF694u);
}

TEST_CASE("FM2 direct draw packet summary exposes replay readiness from trace keys",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  const auto stream1 = decode::BuildDirectDrawBufferViewSummary(
      0xB09BF463u, 0x1012574Au, 0x18746u, 0x0Cu, false, true,
      0x891376055F6CD53Aull);
  const auto stream0 = decode::BuildDirectDrawBufferViewSummary(
      0xB0BBF697u, 0x1000FD62u, 0x7EBu, 0x20u, false, true,
      0x088CCFB61631658Cull);
  const auto index = decode::BuildDirectDrawBufferViewSummary(
      0xB0BCF3F4u, 0x00004704u, 0x2382u, 1u, true, true,
      0x64270C4F8840C8FBull);
  const auto vertex_shader = decode::BuildDirectDrawShaderKeySummary(
      0xB0BBEC20u, 0x4ECu, true, 0x924D29737CD56BDCull, 0u, false, 0ull);
  const auto pixel_shader = decode::BuildDirectDrawShaderKeySummary(
      0xB0BBEB80u, 0x90u, true, 0x225917CA19FF2FA7ull, 0x84u, true,
      0xE4D8250D9CEA5612ull);

  CHECK(stream1.view_bytes == 0x125748u);
  CHECK(stream0.view_bytes == 0xFD60u);
  CHECK(index.view_bytes == 0x4704u);

  const auto packet = decode::BuildDirectDrawIndexedPacketSummary(
      0u, decode::DirectDrawReplayTopology::kTriangleStrip, 0u, 4062u,
      stream0, stream1, index, vertex_shader, pixel_shader);
  CHECK(packet.topology == decode::DirectDrawReplayTopology::kTriangleStrip);
  CHECK(packet.primitive_count == 4060u);
  CHECK(packet.HasReplayableBuffers());
  CHECK(packet.stream0.upload_guest_base == 0xB0BBF694u);
  CHECK(packet.stream1.upload_guest_base == 0xB09BF460u);
  CHECK(packet.index.upload_guest_base == 0xB0BCF3F4u);
  CHECK(packet.HasStableShaderPayloadKeys());
  CHECK(packet.HasPixelStructuralUcodeKey());
  CHECK_FALSE(packet.HasVertexStructuralUcodeKey());
  CHECK(packet.CanAttemptDebugReplay());
  CHECK_FALSE(packet.HasCompleteStructuralUcodeKeys());
}

TEST_CASE("FM2 direct draw debug replay plan maps packet to Plume draw contract",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  const auto stream1 = decode::BuildDirectDrawBufferViewSummary(
      0xB09BF463u, 0x1012574Au, 0x18746u, 0x0Cu, false, true,
      0x891376055F6CD53Aull);
  const auto stream0 = decode::BuildDirectDrawBufferViewSummary(
      0xB0BBF697u, 0x1000FD62u, 0x7EBu, 0x20u, false, true,
      0x088CCFB61631658Cull);
  const auto index = decode::BuildDirectDrawBufferViewSummary(
      0xB0BCF3F4u, 0x00004704u, 0x2382u, 1u, true, true,
      0x64270C4F8840C8FBull);
  const auto vertex_shader = decode::BuildDirectDrawShaderKeySummary(
      0xB0BBEC20u, 0x4ECu, true, 0x924D29737CD56BDCull, 0u, false, 0ull);
  const auto pixel_shader = decode::BuildDirectDrawShaderKeySummary(
      0xB0BBEB80u, 0x90u, true, 0x225917CA19FF2FA7ull, 0x84u, true,
      0xE4D8250D9CEA5612ull);
  const auto packet = decode::BuildDirectDrawIndexedPacketSummary(
      0u, decode::DirectDrawReplayTopology::kTriangleStrip, 0u, 4062u,
      stream0, stream1, index, vertex_shader, pixel_shader);

  const auto plan = decode::BuildDirectDrawDebugReplayPlan(packet);

  CHECK(plan.ready);
  CHECK(plan.topology == decode::DirectDrawReplayTopology::kTriangleStrip);
  CHECK(plan.index_format == decode::DirectDrawReplayIndexFormat::kUint16);
  CHECK(plan.stream_count == 2u);
  CHECK(plan.streams[0].slot == 0u);
  CHECK(plan.streams[0].stride == 0x20u);
  CHECK(plan.streams[0].upload_endian == decode::DirectDrawReplayUploadEndian::kSwap32);
  CHECK(plan.streams[0].guest_base == 0xB0BBF697u);
  CHECK(plan.streams[0].upload_guest_base == 0xB0BBF694u);
  CHECK(plan.streams[0].upload_bytes == 0xFD60u);
  CHECK(plan.streams[0].hash == 0x088CCFB61631658Cull);
  CHECK(plan.streams[1].slot == 1u);
  CHECK(plan.streams[1].stride == 0x0Cu);
  CHECK(plan.streams[1].upload_endian == decode::DirectDrawReplayUploadEndian::kSwap32);
  CHECK(plan.streams[1].guest_base == 0xB09BF463u);
  CHECK(plan.streams[1].upload_guest_base == 0xB09BF460u);
  CHECK(plan.streams[1].upload_bytes == 0x125748u);
  CHECK(plan.streams[1].hash == 0x891376055F6CD53Aull);
  CHECK(plan.index.guest_base == 0xB0BCF3F4u);
  CHECK(plan.index.upload_guest_base == 0xB0BCF3F4u);
  CHECK(plan.index.upload_bytes == 0x4704u);
  CHECK(plan.index.upload_endian == decode::DirectDrawReplayUploadEndian::kSwap16);
  CHECK(plan.index.hash == 0x64270C4F8840C8FBull);
  CHECK(plan.draw.index_count == 4062u);
  CHECK(plan.draw.instance_count == 1u);
  CHECK(plan.draw.start_index == 0u);
  CHECK(plan.draw.base_vertex == 0);
  CHECK(plan.draw.start_instance == 0u);
  CHECK(plan.vertex_payload_hash == 0x924D29737CD56BDCull);
  CHECK(plan.pixel_payload_hash == 0x225917CA19FF2FA7ull);
  CHECK(plan.pixel_structural_ucode_hash == 0xE4D8250D9CEA5612ull);
  CHECK_FALSE(plan.has_vertex_structural_ucode);
  CHECK(plan.has_pixel_structural_ucode);
}

TEST_CASE("FM2 direct draw replay plan carries paired native state provenance",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  const auto stream1 = decode::BuildDirectDrawBufferViewSummary(
      0xB09BF463u, 0x1012574Au, 0x18746u, 0x0Cu, false, true,
      0x891376055F6CD53Aull);
  const auto stream0 = decode::BuildDirectDrawBufferViewSummary(
      0xB0BBF697u, 0x1000FD62u, 0x7EBu, 0x20u, false, true,
      0x088CCFB61631658Cull);
  const auto index = decode::BuildDirectDrawBufferViewSummary(
      0xB0BCF3F4u, 0x00004704u, 0x2382u, 1u, true, true,
      0x64270C4F8840C8FBull);
  const auto vertex_shader = decode::BuildDirectDrawShaderKeySummary(
      0xB0BBEC20u, 0x4ECu, true, 0x924D29737CD56BDCull, 0u, false, 0ull);
  const auto pixel_shader = decode::BuildDirectDrawShaderKeySummary(
      0xB0BBEB80u, 0x90u, true, 0x225917CA19FF2FA7ull, 0x84u, true,
      0xE4D8250D9CEA5612ull);
  const auto packet = decode::BuildDirectDrawIndexedPacketSummary(
      0u, decode::DirectDrawReplayTopology::kTriangleStrip, 0u, 4062u,
      stream0, stream1, index, vertex_shader, pixel_shader);

  decode::NativeStateSnapshot snapshot;
  snapshot.valid = true;
  snapshot.sequence = 42u;
  snapshot.render_context = 0x4004D100u;
  snapshot.vertex_shader = {.valid = true,
                            .sequence = 1u,
                            .render_context = 0x4004D100u,
                            .shader = 0x4181A600u};
  snapshot.pixel_shader = {.valid = true,
                           .sequence = 2u,
                           .render_context = 0x4004D100u,
                           .shader = 0x2E8F24B0u};
  snapshot.streams[0] = {.valid = true,
                         .sequence = 3u,
                         .render_context = 0x4004D100u,
                         .slot = 0u,
                         .resource = 0xBACACA50u,
                         .byte_offset = 0x10u,
                         .stride_bytes = 0x10u,
                         .dirty_mask = 0x20u};
  snapshot.streams[1] = {.valid = true,
                         .sequence = 4u,
                         .render_context = 0x4004D100u,
                         .slot = 1u,
                         .resource = 0x2ECFFA80u,
                         .byte_offset = 0u,
                         .stride_bytes = 0x0Cu,
                         .dirty_mask = 0x40u};
  snapshot.index_buffer = {.valid = true,
                           .sequence = 5u,
                           .render_context = 0x4004D100u,
                           .resource = 0xBACACAE0u};
  snapshot.bound_surface = {.valid = true,
                            .sequence = 6u,
                            .render_context = 0x4004D100u,
                            .surface = 0x2E049240u,
                            .surface_arg = 2u};
  snapshot.viewport = {.valid = true,
                       .sequence = 7u,
                       .render_context = 0x4004D100u,
                       .viewport_mode = 3u};
  snapshot.texture_fetch = {.valid = true,
                            .sequence = 8u,
                            .render_context = 0x4004D100u,
                            .fetch_bits_low = 0x12u,
                            .fetch_bits_mid = 0x34u};
  snapshot.clear = {.valid = true,
                    .sequence = 9u,
                    .render_context = 0x4004D100u,
                    .clear_color_byte = 0x56u,
                    .clear_flags = 0x78u};
  snapshot.last_pass = {.valid = true,
                        .sequence = 10u,
                        .submit_object = 0x41001000u,
                        .tls_or_pass_context = 0x42002000u,
                        .pass_flags = 0x33u,
                        .drawable = 0x43003000u,
                        .draw_callback = 0x44004000u,
                        .wireframe = 1u,
                        .draw_mode = 2u,
                        .pass_marker = 3u};
  snapshot.last_direct_draw = {.valid = true,
                               .sequence = 11u,
                               .direct_render_context = 0x42950010u,
                               .draw_iface = 0x2E0162C0u};

  const auto plan = decode::BuildDirectDrawDebugReplayPlan(packet, snapshot);

  REQUIRE(plan.ready);
  CHECK(plan.native_state.valid);
  CHECK(plan.native_state.sequence == 42u);
  CHECK(plan.native_state.render_context == 0x4004D100u);
  CHECK(plan.native_state.direct_render_context == 0x42950010u);
  CHECK(plan.native_state.draw_iface == 0x2E0162C0u);
  CHECK(plan.native_state.vertex_shader == 0x4181A600u);
  CHECK(plan.native_state.pixel_shader == 0x2E8F24B0u);
  CHECK(plan.native_state.streams[0].valid);
  CHECK(plan.native_state.streams[0].resource == 0xBACACA50u);
  CHECK(plan.native_state.streams[0].byte_offset == 0x10u);
  CHECK(plan.native_state.streams[0].stride_bytes == 0x10u);
  CHECK(plan.native_state.streams[1].valid);
  CHECK(plan.native_state.streams[1].resource == 0x2ECFFA80u);
  CHECK(plan.native_state.index_resource == 0xBACACAE0u);
  CHECK(plan.native_state.bound_surface == 0x2E049240u);
  CHECK(plan.native_state.bound_surface_arg == 2u);
  CHECK(plan.native_state.viewport.valid);
  CHECK(plan.native_state.viewport.viewport_mode == 3u);
  CHECK(plan.native_state.texture_fetch.valid);
  CHECK(plan.native_state.texture_fetch.fetch_bits_low == 0x12u);
  CHECK(plan.native_state.texture_fetch.fetch_bits_mid == 0x34u);
  CHECK(plan.native_state.clear.valid);
  CHECK(plan.native_state.clear.clear_color_byte == 0x56u);
  CHECK(plan.native_state.clear.clear_flags == 0x78u);
  CHECK(plan.native_state.pass.valid);
  CHECK(plan.native_state.pass.submit_object == 0x41001000u);
  CHECK(plan.native_state.pass.tls_or_pass_context == 0x42002000u);
  CHECK(plan.native_state.pass.pass_flags == 0x33u);
  CHECK(plan.native_state.pass.drawable == 0x43003000u);
  CHECK(plan.native_state.pass.draw_callback == 0x44004000u);
  CHECK(plan.native_state.pass.wireframe == 1u);
  CHECK(plan.native_state.pass.draw_mode == 2u);
  CHECK(plan.native_state.pass.pass_marker == 3u);

  CHECK(plan.streams[0].guest_base == 0xB0BBF697u);
  CHECK(plan.streams[0].stride == 0x20u);
  CHECK(plan.streams[1].guest_base == 0xB09BF463u);
  CHECK(plan.index.guest_base == 0xB0BCF3F4u);
}

TEST_CASE("FM2 direct draw replay can promote observed native 28 byte stream layout",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  const auto direct_stream0 = decode::BuildDirectDrawBufferViewSummary(
      0xB0BBF697u, 0x1000FD62u, 0x7EBu, 0x20u, false, true,
      0x088CCFB61631658Cull);
  const auto direct_stream1 = decode::BuildDirectDrawBufferViewSummary(
      0xB09BF463u, 0x1012574Au, 0x18746u, 0x0Cu, false, true,
      0x891376055F6CD53Aull);
  const auto direct_index = decode::BuildDirectDrawBufferViewSummary(
      0xB0BCF3F4u, 0x00004704u, 0x2382u, 1u, true, true,
      0x64270C4F8840C8FBull);
  const auto vertex_shader = decode::BuildDirectDrawShaderKeySummary(
      0xB0BBEC20u, 0x4ECu, true, 0x924D29737CD56BDCull, 0u, false, 0ull);
  const auto pixel_shader = decode::BuildDirectDrawShaderKeySummary(
      0xB0BBEB80u, 0x90u, true, 0x225917CA19FF2FA7ull, 0x84u, true,
      0xE4D8250D9CEA5612ull);
  const auto packet = decode::BuildDirectDrawIndexedPacketSummary(
      0u, decode::DirectDrawReplayTopology::kTriangleStrip, 0u, 4062u,
      direct_stream0, direct_stream1, direct_index, vertex_shader,
      pixel_shader);

  decode::NativeStateSnapshot snapshot;
  snapshot.valid = true;
  snapshot.sequence = 223189951u;
  snapshot.render_context = 0x4004D100u;
  snapshot.streams[0] = {.valid = true,
                         .sequence = 3u,
                         .render_context = 0x4004D100u,
                         .slot = 0u,
                         .resource = 0x2EF2A900u,
                         .byte_offset = 0u,
                         .stride_bytes = 28u,
                         .dirty_mask = 1u};
  snapshot.streams[1] = {.valid = true,
                         .sequence = 4u,
                         .render_context = 0x4004D100u,
                         .slot = 1u,
                         .resource = 0x2E660E40u,
                         .byte_offset = 0u,
                         .stride_bytes = 12u,
                         .dirty_mask = 1u};
  snapshot.index_buffer = {.valid = true,
                           .sequence = 5u,
                           .render_context = 0x4004D100u,
                           .resource = 0x2E2867E0u};

  const auto direct_plan =
      decode::BuildDirectDrawDebugReplayPlan(packet, snapshot);
  CHECK(decode::DirectDrawReplayPipelineLayoutForPlan(direct_plan) ==
        decode::DirectDrawReplayPipelineLayout::kDebugRaw32Side12);
  CHECK(decode::DirectDrawReplayNativeLayoutFromState(
            direct_plan.native_state) ==
        decode::DirectDrawReplayPipelineLayout::kNativePosition28Side12);

  const auto native_stream0 = decode::BuildDirectDrawBufferViewSummary(
      0xBA000010u, 0x00007000u, 0x400u, 28u, false, true,
      0x1111222233334444ull);
  const auto native_stream1 = decode::BuildDirectDrawBufferViewSummary(
      0xBB000000u, 0x00003000u, 0x400u, 12u, false, true,
      0x5555666677778888ull);
  const auto native_index = decode::BuildDirectDrawBufferViewSummary(
      0xBC000000u, 0x00002000u, 0x1000u, 1u, true, true,
      0x9999AAAABBBBCCCCull);

  const auto native_plan = decode::BuildDirectDrawNativeLayoutReplayPlan(
      direct_plan, native_stream0, native_stream1, native_index);

  REQUIRE(native_plan.ready);
  CHECK(decode::DirectDrawReplayPipelineLayoutForPlan(native_plan) ==
        decode::DirectDrawReplayPipelineLayout::kNativePosition28Side12);
  CHECK(native_plan.streams[0].slot == 0u);
  CHECK(native_plan.streams[0].stride == 28u);
  CHECK(native_plan.streams[0].guest_base == 0xBA000010u);
  CHECK(native_plan.streams[0].upload_guest_base == 0xBA000010u);
  CHECK(native_plan.streams[0].upload_bytes == 0x7000u);
  CHECK(native_plan.streams[1].slot == 1u);
  CHECK(native_plan.streams[1].stride == 12u);
  CHECK(native_plan.streams[1].hash == 0x5555666677778888ull);
  CHECK(native_plan.index.guest_base == 0xBC000000u);
  CHECK(native_plan.index.upload_endian ==
        decode::DirectDrawReplayUploadEndian::kSwap16);
}

TEST_CASE("FM2 direct draw replay upload conversion swaps guest endian data",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  const std::array<uint8_t, 8> guest_index = {
      0x00, 0x00, 0x00, 0x01, 0x12, 0x34, 0xAB, 0xCD};
  std::array<uint8_t, guest_index.size()> host_index = {};
  REQUIRE(decode::ConvertDirectDrawReplayUploadBytes(
      guest_index.data(), static_cast<uint32_t>(guest_index.size()),
      decode::DirectDrawReplayUploadEndian::kSwap16, host_index.data()));
  CHECK(host_index == std::array<uint8_t, 8>{
                          0x00, 0x00, 0x01, 0x00, 0x34, 0x12, 0xCD, 0xAB});

  const std::array<uint8_t, 8> guest_stream = {
      0x18, 0x3E, 0xF9, 0xDA, 0xBE, 0xBF, 0x00, 0x1C};
  std::array<uint8_t, guest_stream.size()> host_stream = {};
  REQUIRE(decode::ConvertDirectDrawReplayUploadBytes(
      guest_stream.data(), static_cast<uint32_t>(guest_stream.size()),
      decode::DirectDrawReplayUploadEndian::kSwap32, host_stream.data()));
  CHECK(host_stream == std::array<uint8_t, 8>{
                           0xDA, 0xF9, 0x3E, 0x18, 0x1C, 0x00, 0xBF, 0xBE});

  std::array<uint8_t, 3> invalid = {};
  CHECK_FALSE(decode::ConvertDirectDrawReplayUploadBytes(
      guest_index.data(), static_cast<uint32_t>(invalid.size()),
      decode::DirectDrawReplayUploadEndian::kSwap16, invalid.data()));
  CHECK_FALSE(decode::ConvertDirectDrawReplayUploadBytes(
      guest_index.data(), static_cast<uint32_t>(invalid.size()),
      decode::DirectDrawReplayUploadEndian::kSwap32, invalid.data()));
}

TEST_CASE("FM2 direct draw stream0 float3 stats decode guest-endian positions",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint8_t, 0x60> stream = {};
  StoreBE32(stream, 0x00u, 0x3F800000u);  // 1.0
  StoreBE32(stream, 0x04u, 0xC0000000u);  // -2.0
  StoreBE32(stream, 0x08u, 0x40400000u);  // 3.0
  StoreBE32(stream, 0x20u, 0xC0800000u);  // -4.0
  StoreBE32(stream, 0x24u, 0x40A00000u);  // 5.0
  StoreBE32(stream, 0x28u, 0xC0C00000u);  // -6.0
  StoreBE32(stream, 0x40u, 0x7FC00000u);  // NaN
  StoreBE32(stream, 0x44u, 0x3F800000u);
  StoreBE32(stream, 0x48u, 0x3F800000u);

  const auto stats = decode::AnalyzeDirectDrawReplayFloat3PositionStats(
      stream.data(), static_cast<uint32_t>(stream.size()), 0x20u, 0u, 0u);
  REQUIRE(stats.valid);
  CHECK(stats.stride == 0x20u);
  CHECK(stats.position_offset == 0u);
  CHECK(stats.vertex_count == 3u);
  CHECK(stats.sampled_vertices == 3u);
  CHECK(stats.finite_vertices == 2u);
  CHECK(stats.has_finite_bounds);
  CHECK(stats.min_x == -4.0f);
  CHECK(stats.min_y == -2.0f);
  CHECK(stats.min_z == -6.0f);
  CHECK(stats.max_x == 1.0f);
  CHECK(stats.max_y == 5.0f);
  CHECK(stats.max_z == 3.0f);

  const auto capped = decode::AnalyzeDirectDrawReplayFloat3PositionStats(
      stream.data(), static_cast<uint32_t>(stream.size()), 0x20u, 0u, 1u);
  REQUIRE(capped.valid);
  CHECK(capped.vertex_count == 3u);
  CHECK(capped.sampled_vertices == 1u);
  CHECK(capped.finite_vertices == 1u);
  CHECK(capped.min_x == 1.0f);
  CHECK(capped.max_z == 3.0f);

  CHECK_FALSE(decode::AnalyzeDirectDrawReplayFloat3PositionStats(
                  stream.data(), static_cast<uint32_t>(stream.size()), 8u, 0u,
                  0u)
                  .valid);
}

TEST_CASE("FM2 direct draw VS float constant stats summarize host-order registers",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 256u * 4u> constants = {};
  auto store_float = [&](uint32_t constant_index, uint32_t component,
                         float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    constants[constant_index * 4u + component] = bits;
  };

  store_float(4u, 0u, 1.0f);
  store_float(4u, 1u, -2.0f);
  store_float(4u, 2u, 3.0f);
  store_float(4u, 3u, 4.0f);
  store_float(9u, 0u, 0.0f);
  store_float(9u, 1u, -0.0f);
  store_float(9u, 2u, 0.0f);
  store_float(9u, 3u, 0.0f);
  store_float(14u, 0u, std::numeric_limits<float>::infinity());
  store_float(14u, 1u, 1.0f);
  store_float(14u, 2u, 2.0f);
  store_float(14u, 3u, 3.0f);

  const auto stats = decode::AnalyzeDirectDrawFloat4ConstantStats(
      constants.data(), 256u, 4u);
  REQUIRE(stats.valid);
  CHECK(stats.constant_count == 256u);
  CHECK(stats.finite_constants == 255u);
  CHECK(stats.nonzero_constants == 2u);
  CHECK(stats.first_nonzero_constant == 4u);
  CHECK(stats.last_nonzero_constant == 14u);
  REQUIRE(stats.sample_count == 2u);
  CHECK(stats.samples[0].constant_index == 4u);
  CHECK(stats.samples[0].values[0] == 1.0f);
  CHECK(stats.samples[0].values[1] == -2.0f);
  CHECK(stats.samples[0].values[2] == 3.0f);
  CHECK(stats.samples[0].values[3] == 4.0f);
  CHECK(stats.samples[1].constant_index == 14u);
  CHECK(std::isinf(stats.samples[1].values[0]));

  const auto invalid =
      decode::AnalyzeDirectDrawFloat4ConstantStats(nullptr, 256u, 4u);
  CHECK_FALSE(invalid.valid);
}

TEST_CASE("FM2 direct draw debug replay extracts c28 world transform candidate",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 256u * 4u> constants = {};
  auto store_float = [&](uint32_t constant_index, uint32_t component,
                         float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    constants[constant_index * 4u + component] = bits;
  };

  const uint32_t first_constant =
      decode::kDirectDrawDebugReplayWorldTransformFirstConstant;
  for (uint32_t row = 0; row < 4u; ++row) {
    for (uint32_t component = 0; component < 4u; ++component) {
      store_float(first_constant + row, component,
                  float(row * 10u + component + 1u));
    }
  }

  const auto transform = decode::BuildDirectDrawDebugReplayTransformFromConstants(
      constants.data(), 256u, first_constant);
  REQUIRE(transform.valid);
  CHECK(transform.first_constant == first_constant);
  CHECK(transform.rows[0][0] == 1.0f);
  CHECK(transform.rows[0][3] == 4.0f);
  CHECK(transform.rows[1][0] == 11.0f);
  CHECK(transform.rows[2][2] == 23.0f);
  CHECK(transform.rows[3][3] == 34.0f);

  const auto truncated = decode::BuildDirectDrawDebugReplayTransformFromConstants(
      constants.data(), first_constant + 3u, first_constant);
  CHECK_FALSE(truncated.valid);

  const auto missing = decode::BuildDirectDrawDebugReplayTransformFromConstants(
      nullptr, 256u, first_constant);
  CHECK_FALSE(missing.valid);
}

TEST_CASE("FM2 direct draw transform candidate scoring samples projected positions",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint8_t, 0x20> stream = {};
  StoreBE32(stream, 0x00u, 0x3F800000u);  // 1.0
  StoreBE32(stream, 0x04u, 0x40000000u);  // 2.0
  StoreBE32(stream, 0x08u, 0x40400000u);  // 3.0

  decode::DirectDrawDebugReplayTransform transform;
  transform.valid = true;
  transform.first_constant = 40u;
  transform.rows[0][0] = 1.0f;
  transform.rows[0][3] = 1.0f;
  transform.rows[1][1] = 1.0f;
  transform.rows[1][3] = 2.0f;
  transform.rows[2][2] = 1.0f;
  transform.rows[2][3] = 3.0f;
  transform.rows[3][3] = 1.0f;

  const auto row_major = decode::AnalyzeDirectDrawReplayClipPositionStats(
      stream.data(), static_cast<uint32_t>(stream.size()), 0x20u, 0u,
      transform, decode::DirectDrawReplayTransformInterpretation::kRowMajorClip,
      0u);
  REQUIRE(row_major.valid);
  CHECK(row_major.sampled_vertices == 1u);
  CHECK(row_major.finite_vertices == 1u);
  CHECK(row_major.projectable_vertices == 1u);
  CHECK(row_major.xy_inside_vertices == 0u);
  CHECK(row_major.min_ndc_x == 2.0f);
  CHECK(row_major.max_ndc_y == 4.0f);
  CHECK(row_major.min_ndc_z == 6.0f);
  CHECK(row_major.min_w == 1.0f);

  const auto column_major = decode::AnalyzeDirectDrawReplayClipPositionStats(
      stream.data(), static_cast<uint32_t>(stream.size()), 0x20u, 0u,
      transform,
      decode::DirectDrawReplayTransformInterpretation::kColumnMajorClip, 0u);
  REQUIRE(column_major.valid);
  CHECK(column_major.projectable_vertices == 1u);
  CHECK(column_major.xy_inside_vertices == 1u);
  CHECK(column_major.d3d_xyz_inside_vertices == 1u);
  CHECK(column_major.min_w == 15.0f);
  CHECK(column_major.min_ndc_x == Catch::Approx(1.0f / 15.0f));
  CHECK(column_major.min_ndc_y == Catch::Approx(2.0f / 15.0f));
  CHECK(column_major.min_ndc_z == Catch::Approx(3.0f / 15.0f));

  const auto missing = decode::AnalyzeDirectDrawReplayClipPositionStats(
      stream.data(), static_cast<uint32_t>(stream.size()), 0x20u, 0u,
      {}, decode::DirectDrawReplayTransformInterpretation::kRowMajorClip, 0u);
  CHECK_FALSE(missing.valid);
}

TEST_CASE("FM2 direct draw row-major transform product composes constants",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  decode::DirectDrawDebugReplayTransform left;
  left.valid = true;
  left.first_constant = 36u;
  left.rows[0][0] = 1.0f;
  left.rows[0][3] = 10.0f;
  left.rows[1][1] = 1.0f;
  left.rows[1][3] = 20.0f;
  left.rows[2][2] = 1.0f;
  left.rows[2][3] = 30.0f;
  left.rows[3][3] = 1.0f;

  decode::DirectDrawDebugReplayTransform right;
  right.valid = true;
  right.first_constant = 28u;
  right.rows[0][0] = 2.0f;
  right.rows[1][1] = 3.0f;
  right.rows[2][2] = 4.0f;
  right.rows[3][3] = 1.0f;

  const auto product = decode::MultiplyDirectDrawReplayRowMajorTransforms(
      left, right, 36u);
  REQUIRE(product.valid);
  CHECK(product.first_constant == 36u);
  CHECK(product.rows[0][0] == 2.0f);
  CHECK(product.rows[1][1] == 3.0f);
  CHECK(product.rows[2][2] == 4.0f);
  CHECK(product.rows[0][3] == 10.0f);
  CHECK(product.rows[1][3] == 20.0f);
  CHECK(product.rows[2][3] == 30.0f);
  CHECK(product.rows[3][3] == 1.0f);

  CHECK_FALSE(decode::MultiplyDirectDrawReplayRowMajorTransforms(
                  {}, right, 0u)
                  .valid);
}

TEST_CASE("FM2 direct draw replay transform source selects diagnostic matrices",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 256u * 4u> constants = {};
  auto store_float = [&](uint32_t constant_index, uint32_t component,
                         float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    constants[constant_index * 4u + component] = bits;
  };

  for (uint32_t row = 0; row < 4u; ++row) {
    store_float(0u + row, row, 10.0f + float(row));
    store_float(28u + row, row, 2.0f + float(row));
    store_float(36u + row, row, 3.0f + float(row));
  }

  const auto c0 = decode::BuildDirectDrawDebugReplayTransformFromSource(
      constants.data(), 256u, "c0");
  REQUIRE(c0.valid);
  CHECK(c0.first_constant == 0u);
  CHECK(c0.rows[0][0] == 10.0f);
  CHECK(c0.rows[3][3] == 13.0f);

  const auto product = decode::BuildDirectDrawDebugReplayTransformFromSource(
      constants.data(), 256u, "c36_mul_c28");
  REQUIRE(product.valid);
  CHECK(product.first_constant == 36u);
  CHECK(product.rows[0][0] == 6.0f);
  CHECK(product.rows[3][3] == 30.0f);

  const auto fallback = decode::BuildDirectDrawDebugReplayTransformFromSource(
      constants.data(), 256u, "unknown");
  REQUIRE(fallback.valid);
  CHECK(fallback.first_constant ==
        decode::kDirectDrawDebugReplayWorldTransformFirstConstant);
  CHECK(fallback.rows[2][2] == 4.0f);
}

TEST_CASE("FM2 direct draw auto replay transform selects visible candidate",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint8_t, 0x60> stream = {};
  StoreBE32(stream, 0x00u, 0x00000000u);  // 0.0
  StoreBE32(stream, 0x04u, 0x00000000u);  // 0.0
  StoreBE32(stream, 0x08u, 0x00000000u);  // 0.0
  StoreBE32(stream, 0x20u, 0x3E800000u);  // 0.25
  StoreBE32(stream, 0x24u, 0x3F000000u);  // 0.5
  StoreBE32(stream, 0x28u, 0x3DCCCCCDu);  // 0.1
  StoreBE32(stream, 0x40u, 0xBE800000u);  // -0.25
  StoreBE32(stream, 0x44u, 0xBE800000u);  // -0.25
  StoreBE32(stream, 0x48u, 0x3E4CCCCDu);  // 0.2

  std::array<uint32_t, 256u * 4u> constants = {};
  auto store_float = [&](uint32_t constant_index, uint32_t component,
                         float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    constants[constant_index * 4u + component] = bits;
  };

  store_float(0u, 0u, 1.0f);
  store_float(0u, 3u, 8.0f);
  store_float(1u, 1u, 1.0f);
  store_float(2u, 2u, 1.0f);
  store_float(3u, 3u, 1.0f);

  store_float(28u, 0u, 1.0f);
  store_float(29u, 1u, 1.0f);
  store_float(30u, 2u, 1.0f);
  store_float(31u, 3u, 1.0f);

  const auto selection = decode::SelectDirectDrawDebugReplayTransformCandidate(
      stream.data(), static_cast<uint32_t>(stream.size()), 0x20u,
      constants.data(), 256u,
      decode::DirectDrawReplayTransformInterpretation::kColumnMajorClip, 0u);

  REQUIRE(selection.valid);
  CHECK(selection.source_name == "c28");
  CHECK(selection.transform.first_constant == 28u);
  CHECK(selection.stats.d3d_xyz_inside_vertices == 3u);
}

TEST_CASE("FM2 direct draw auto replay transform prefers tighter projected bounds",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint8_t, 0x80> stream = {};
  StoreBE32(stream, 0x00u, 0xBE4CCCCDu);  // -0.2
  StoreBE32(stream, 0x04u, 0xBE4CCCCDu);  // -0.2
  StoreBE32(stream, 0x08u, 0x3F000000u);  // 0.5
  StoreBE32(stream, 0x20u, 0x00000000u);  // 0.0
  StoreBE32(stream, 0x24u, 0x00000000u);  // 0.0
  StoreBE32(stream, 0x28u, 0x3F000000u);  // 0.5
  StoreBE32(stream, 0x40u, 0x3E4CCCCDu);  // 0.2
  StoreBE32(stream, 0x44u, 0x3E4CCCCDu);  // 0.2
  StoreBE32(stream, 0x48u, 0x3F000000u);  // 0.5
  StoreBE32(stream, 0x60u, 0x3F99999Au);  // 1.2
  StoreBE32(stream, 0x64u, 0x3E4CCCCDu);  // 0.2
  StoreBE32(stream, 0x68u, 0x3F000000u);  // 0.5

  std::array<uint32_t, 256u * 4u> constants = {};
  auto store_float = [&](uint32_t constant_index, uint32_t component,
                         float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    constants[constant_index * 4u + component] = bits;
  };

  store_float(0u, 0u, 3.0f);
  store_float(1u, 1u, 1.0f);
  store_float(2u, 2u, 1.0f);
  store_float(3u, 3u, 1.0f);

  store_float(28u, 0u, 1.0f);
  store_float(29u, 1u, 1.0f);
  store_float(30u, 2u, 1.0f);
  store_float(31u, 3u, 1.0f);

  const auto selection = decode::SelectDirectDrawDebugReplayTransformCandidate(
      stream.data(), static_cast<uint32_t>(stream.size()), 0x20u,
      constants.data(), 256u,
      decode::DirectDrawReplayTransformInterpretation::kColumnMajorClip, 0u);

  REQUIRE(selection.valid);
  CHECK(selection.source_name == "c28");
  CHECK(selection.stats.d3d_xyz_inside_vertices == 3u);
  CHECK(selection.stats.xy_inside_vertices == 3u);
}

TEST_CASE("FM2 Xenos ucode bounds follow shader analyzer control-flow scan",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 36> ucode = {};
  StoreCfPair(ucode, 0, 0u, MakeCfDword1(0u), MakeCfExecDword0(4u, 2u),
              MakeCfDword1(1u));
  StoreCfPair(ucode, 2, MakeCfExecDword0(7u, 3u), MakeCfDword1(2u), 0u,
              MakeCfDword1(0u));
  StoreCfPair(ucode, 8, MakeCfExecDword0(0u, 0u), MakeCfDword1(1u), 0u,
              MakeCfDword1(0u));

  const auto bounds = decode::AnalyzeXenosUcodeBounds(
      ucode.data(), static_cast<uint32_t>(ucode.size()));
  CHECK(bounds.valid);
  CHECK(bounds.cf_pair_count_available == 12u);
  CHECK(bounds.cf_pair_index_bound == 4u);
  CHECK(bounds.cf_byte_count == 48u);
  CHECK(bounds.first_exec_cf_index == 1u);
  CHECK(bounds.first_exec_opcode == 1u);
  CHECK(bounds.first_exec_address == 4u);
  CHECK(bounds.exec_instruction_count == 2u);
  CHECK(bounds.saw_exec_end);
  CHECK(bounds.exec_high_water_instruction == 10u);
  CHECK(bounds.exec_high_water_bytes == 120u);
  CHECK(bounds.total_used_bytes == 120u);
  CHECK_FALSE(bounds.truncated);

  const auto truncated_bounds = decode::AnalyzeXenosUcodeBounds(ucode.data(), 27u);
  CHECK(truncated_bounds.valid);
  CHECK(truncated_bounds.total_used_bytes == 120u);
  CHECK(truncated_bounds.scanned_bytes == 108u);
  CHECK(truncated_bounds.truncated);
}

TEST_CASE("FM2 Xenos ucode bounds reject exec targets outside the scan window",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 12> ucode = {};
  StoreCfPair(ucode, 0, 0u, MakeCfDword1(0u), MakeCfExecDword0(3840u, 2u),
              MakeCfDword1(3u));

  const auto bounds = decode::AnalyzeXenosUcodeBounds(
      ucode.data(), static_cast<uint32_t>(ucode.size()));
  CHECK_FALSE(bounds.valid);
  CHECK(bounds.truncated);
  CHECK(bounds.first_exec_cf_index == 1u);
  CHECK(bounds.first_exec_opcode == 3u);
  CHECK(bounds.first_exec_address == 3840u);
  CHECK(bounds.cf_pair_index_bound == 0u);
  CHECK(bounds.total_used_bytes == 0u);
}

TEST_CASE("FM2 Xenos ucode bounds reject data without exec control flow",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 3> ucode = {};
  const auto bounds = decode::AnalyzeXenosUcodeBounds(
      ucode.data(), static_cast<uint32_t>(ucode.size()));
  CHECK_FALSE(bounds.valid);
  CHECK(bounds.cf_pair_count_available == 1u);
  CHECK(bounds.cf_pair_index_bound == 0u);
  CHECK(bounds.total_used_bytes == 0u);
}

TEST_CASE("FM2 Xenos ucode candidate scan accepts ucode at payload start",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 36> payload = {};
  StoreValidUcodeAt(payload, 0u);

  const auto candidate = decode::FindXenosUcodeCandidate(
      payload.data(), static_cast<uint32_t>(payload.size()));
  REQUIRE(candidate.valid);
  CHECK(candidate.dword_offset == 0u);
  CHECK(candidate.byte_offset == 0u);
  CHECK(candidate.bounds.valid);
  CHECK(candidate.bounds.saw_exec_end);
  CHECK(candidate.bounds.total_used_bytes == 120u);
}

TEST_CASE("FM2 Xenos ucode candidate scan skips payload wrapper dwords",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 48> payload = {};
  payload[0] = 0xAAAAAAAAu;
  payload[1] = 0x55555555u;
  payload[2] = 0xDEADBEEFu;
  payload[3] = 0x01020304u;
  constexpr uint32_t kCandidateDwordOffset = 8u;
  StoreValidUcodeAt(payload, kCandidateDwordOffset);

  const auto candidate = decode::FindXenosUcodeCandidate(
      payload.data(), static_cast<uint32_t>(payload.size()));
  REQUIRE(candidate.valid);
  CHECK(candidate.dword_offset == kCandidateDwordOffset);
  CHECK(candidate.byte_offset == kCandidateDwordOffset * sizeof(uint32_t));
  CHECK(candidate.bounds.first_exec_opcode == 1u);
  CHECK(candidate.bounds.first_exec_address == 4u);
}

TEST_CASE("FM2 Xenos ucode candidate scan skips an invalid first candidate",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 48> payload = {};
  StoreCfPairAt(payload, 0u, 0, 0u, MakeCfDword1(0u),
                MakeCfExecDword0(3840u, 2u), MakeCfDword1(3u));
  constexpr uint32_t kCandidateDwordOffset = 12u;
  StoreValidUcodeAt(payload, kCandidateDwordOffset);

  const auto candidate = decode::FindXenosUcodeCandidate(
      payload.data(), static_cast<uint32_t>(payload.size()));
  REQUIRE(candidate.valid);
  CHECK(candidate.dword_offset == kCandidateDwordOffset);
  CHECK(candidate.byte_offset == kCandidateDwordOffset * sizeof(uint32_t));
  CHECK_FALSE(candidate.bounds.truncated);
}

TEST_CASE("FM2 Xenos ucode candidate scan prefers larger complete candidates",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 60> payload = {};
  StoreTinyValidUcodeAt(payload, 0u);
  constexpr uint32_t kCandidateDwordOffset = 12u;
  StoreValidUcodeAt(payload, kCandidateDwordOffset);

  const auto candidate = decode::FindXenosUcodeCandidate(
      payload.data(), static_cast<uint32_t>(payload.size()));
  REQUIRE(candidate.valid);
  CHECK(candidate.dword_offset == kCandidateDwordOffset);
  CHECK(candidate.byte_offset == kCandidateDwordOffset * sizeof(uint32_t));
  CHECK(candidate.bounds.total_used_bytes == 120u);
}

TEST_CASE("FM2 Xenos ucode candidate scan returns top candidates in score order",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 72> payload = {};
  StoreTinyValidUcodeAt(payload, 0u);
  constexpr uint32_t kFirstLargeCandidateDwordOffset = 12u;
  constexpr uint32_t kSecondLargeCandidateDwordOffset = 24u;
  StoreValidUcodeAt(payload, kSecondLargeCandidateDwordOffset);
  StoreValidUcodeAt(payload, kFirstLargeCandidateDwordOffset);

  std::array<decode::DirectDrawShaderUcodeCandidate, 4> candidates = {};
  const uint32_t candidate_count = decode::FindTopXenosUcodeCandidates(
      payload.data(), static_cast<uint32_t>(payload.size()), candidates.data(),
      static_cast<uint32_t>(candidates.size()));

  REQUIRE(candidate_count == static_cast<uint32_t>(candidates.size()));
  CHECK(candidates[0].valid);
  CHECK(candidates[0].dword_offset == kFirstLargeCandidateDwordOffset);
  CHECK(candidates[0].bounds.total_used_bytes == 120u);
  CHECK(candidates[1].valid);
  CHECK(candidates[1].dword_offset > candidates[0].dword_offset);
  CHECK(candidates[1].bounds.total_used_bytes == 120u);
  CHECK(candidates[2].valid);
  CHECK(candidates[2].dword_offset == kSecondLargeCandidateDwordOffset);
  CHECK(candidates[2].bounds.total_used_bytes == 120u);
  CHECK(candidates[3].valid);
  CHECK(candidates[3].dword_offset > candidates[2].dword_offset);
  CHECK(candidates[3].bounds.total_used_bytes == 120u);
  for (uint32_t i = 1; i < candidate_count; ++i) {
    CHECK_FALSE(decode::IsBetterXenosUcodeCandidate(candidates[i],
                                                    candidates[i - 1u]));
  }
}

TEST_CASE("FM2 Xenos ucode candidate scan rejects payloads without exec control flow",
          "[fm2][plume]") {
  namespace decode = fm2::native_renderer;

  std::array<uint32_t, 12> payload = {};
  const auto candidate = decode::FindXenosUcodeCandidate(
      payload.data(), static_cast<uint32_t>(payload.size()));
  CHECK_FALSE(candidate.valid);
  CHECK(candidate.dword_offset == 0u);
  CHECK(candidate.byte_offset == 0u);
  CHECK_FALSE(candidate.bounds.valid);
}

TEST_CASE("FM2 shader analysis summarizes vertex fetch bindings", "[fm2][plume]") {
  namespace decode = fm2::native_renderer;
  namespace xenos = rex::graphics::xenos;

  std::array<uint32_t, 6> ucode = {};
  StoreCfPair(ucode, 0, MakeCfExecDword0(1u, 1u, 1u), MakeCfDword1(2u), 0u,
              MakeCfDword1(0u));
  StoreVertexFetchInstruction(ucode.data() + 3, 5u, 0u, 3u,
                              xenos::VertexFormat::k_32_32_32_FLOAT, 8u, 2);

  const auto summary = decode::AnalyzeDirectDrawVertexShaderUcode(
      ucode.data(), static_cast<uint32_t>(ucode.size()));
  REQUIRE(summary.valid);
  CHECK(summary.ucode_dword_count == 6u);
  CHECK(summary.cf_pair_index_bound == 1u);
  CHECK(summary.binding_count == 1u);
  CHECK(summary.attribute_count == 1u);
  CHECK(summary.vertex_fetch_bitmap[0] == (1u << 5));
  CHECK(summary.attributes[0].binding_index == 0u);
  CHECK(summary.attributes[0].attribute_index == 0u);
  CHECK(summary.attributes[0].fetch_constant == 5u);
  CHECK(summary.attributes[0].stride_words == 8u);
  CHECK(summary.attributes[0].offset_words == 2);
  CHECK(summary.attributes[0].data_format ==
        static_cast<uint32_t>(xenos::VertexFormat::k_32_32_32_FLOAT));
  CHECK(summary.attributes[0].source_register == 0u);
  CHECK(summary.attributes[0].destination_register == 3u);
  CHECK(summary.attributes[0].write_mask == 0b1111u);
}

TEST_CASE("FM2 direct draw triangle segment count maps to primitive count", "[fm2][plume]") {
  using fm2::native_renderer::TriangleListPrimitiveCountFromIndexCount;
  using fm2::native_renderer::TriangleStripPrimitiveCountFromIndexCount;

  CHECK(TriangleListPrimitiveCountFromIndexCount(0u) == 0u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(2u) == 0u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(3u) == 1u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(4062u) == 1354u);
  CHECK(TriangleListPrimitiveCountFromIndexCount(4158u) == 1386u);

  CHECK(TriangleStripPrimitiveCountFromIndexCount(0u) == 0u);
  CHECK(TriangleStripPrimitiveCountFromIndexCount(2u) == 0u);
  CHECK(TriangleStripPrimitiveCountFromIndexCount(3u) == 1u);
  CHECK(TriangleStripPrimitiveCountFromIndexCount(4062u) == 4060u);
  CHECK(TriangleStripPrimitiveCountFromIndexCount(4158u) == 4156u);
}
