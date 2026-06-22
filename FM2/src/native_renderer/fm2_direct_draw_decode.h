#pragma once

#include "native_renderer/fm2_native_state.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

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
inline constexpr uint32_t kD3DResourceByteSizeMask = 0x0FFFFFFFu;

inline constexpr uint32_t kDirectDrawStateHandleResolvedObjectOffset = 0x48u;
inline constexpr uint32_t kDirectDrawVertexShaderTypeTag = 0x00100006u;
inline constexpr uint32_t kDirectDrawPixelShaderTypeTag = 0x00100007u;
inline constexpr uint32_t kDirectDrawPixelShaderPayloadGpuBaseOffset = 0x18u;
inline constexpr uint32_t kDirectDrawVertexShaderPayloadGpuBaseOffset = 0x20u;
inline constexpr uint32_t kDirectDrawPixelShaderPayloadUcodeOffset = 0x00u;
inline constexpr uint32_t kDirectDrawVertexShaderPayloadUcodeOffset = 0x30u;
inline constexpr uint32_t kDirectDrawPixelShaderPayloadByteCountOffset = 0x30u;
inline constexpr uint32_t kDirectDrawVertexShaderPayloadByteCountCandidateOffset = 0x30u;
inline constexpr uint32_t kDirectDrawStateByteDumpMax = 1024u;
inline constexpr uint32_t kDirectDrawShaderByteDumpMax = 4096u;
inline constexpr uint32_t kXenosUcodeInstructionByteStride = 12u;
inline constexpr uint32_t kXenosUcodeControlFlowPairDwordCount = 3u;
inline constexpr uint32_t kDirectDrawSlot28StateTableBaseOffset = 0x28u;
inline constexpr uint32_t kDirectDrawSlot28StateTableOffsetField = 0x3Cu;
inline constexpr uint32_t kDirectDrawVertexShaderTableBaseOffset = 0x368u;
inline constexpr uint32_t kDirectDrawVertexShaderTableOffsetField = 0x37Cu;
inline constexpr uint32_t kDirectDrawVertexShaderTablePayloadByteCountOffset = 0x2Cu;
inline constexpr uint32_t kDirectDrawCompiledStateHeaderSize = 0x14u;
inline constexpr uint32_t kDirectDrawCompiledStateMaxEntries = 16u;
inline constexpr uint32_t kDirectDrawFetchConstantRegisterBase = 0x4800u;
inline constexpr uint32_t kDirectDrawFetchConstantGroupDwordCount = 6u;
inline constexpr uint32_t kDirectDrawReplayFetchBaseMask = 0x1FFFFFFCu;
inline constexpr uint32_t kD3DCommandContextStateShadowOffset = 0x400u;
inline constexpr uint32_t kD3DCommandContextStateShadowByteCount = 0x300u;
inline constexpr uint32_t kD3DCommandContextFetchGroupDwordCount = 6u;
inline constexpr uint32_t kD3DCommandContextFetchGroupByteCount =
    kD3DCommandContextFetchGroupDwordCount * sizeof(uint32_t);
inline constexpr uint32_t kD3DCommandContextFetchGroupCount =
    kD3DCommandContextStateShadowByteCount /
    kD3DCommandContextFetchGroupByteCount;
inline constexpr uint32_t kDirectDrawDebugReplayWorldTransformFirstConstant = 28u;
inline constexpr uint32_t kDirectDrawDebugReplayTransformRowCount = 4u;
inline constexpr uint32_t kDirectDrawDebugReplayTransformColumnCount = 4u;
inline constexpr uint32_t kDirectDrawReplayAnyRecordIndex = 0xFFFFFFFFu;

enum class D3DCommandContextFetchGroupKind : uint8_t {
  kTexture,
  kVertexTriplet,
};

struct D3DCommandContextTextureFetchSummary {
  uint32_t type = 0;
  uint32_t format = 0;
  uint32_t endian = 0;
  uint32_t base_address = 0;
  uint32_t mip_address = 0;
  uint32_t dimension = 0;
  uint32_t pitch = 0;
  uint32_t tiled = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t depth_or_stack = 0;
};

struct D3DCommandContextVertexFetchSummary {
  uint32_t type = 0;
  uint32_t address_words = 0;
  uint32_t address_bytes = 0;
  uint32_t endian = 0;
  uint32_t size_words = 0;
  uint32_t size_bytes = 0;
};

struct D3DCommandContextFetchGroupSummary {
  bool valid = false;
  uint32_t group_index = 0;
  uint32_t register_base = 0;
  D3DCommandContextFetchGroupKind kind =
      D3DCommandContextFetchGroupKind::kTexture;
  uint32_t dwords[kD3DCommandContextFetchGroupDwordCount] = {};
  D3DCommandContextTextureFetchSummary texture;
  D3DCommandContextVertexFetchSummary vertex[3] = {};
};

constexpr bool ShouldTraceD3DCommandContextSample(uint64_t sample_ix,
                                                  uint32_t sample_skip,
                                                  uint32_t sample_limit) {
  if (sample_limit == 0 || sample_ix < sample_skip) {
    return false;
  }
  return sample_ix - sample_skip < sample_limit;
}

constexpr bool ShouldTraceDirectDrawSample(uint64_t sample_ix,
                                           uint32_t sample_skip,
                                           uint32_t sample_limit) {
  return ShouldTraceD3DCommandContextSample(sample_ix, sample_skip,
                                            sample_limit);
}

constexpr bool ShouldSubmitDirectDebugReplayRecord(uint32_t record_index,
                                                   uint32_t selected_index) {
  return selected_index == kDirectDrawReplayAnyRecordIndex ||
         record_index == selected_index;
}

constexpr bool ShouldDecodeDirectDrawForCompareReplay(bool trace_enabled,
                                                      bool compare_enabled) {
  return trace_enabled || compare_enabled;
}

constexpr uint32_t DirectCompareReplayRecordScanCount(uint32_t record_count,
                                                      uint32_t record_limit) {
  if (record_limit == 0 || record_limit > record_count) {
    return record_count;
  }
  return record_limit;
}

constexpr bool ShouldDecodeDirectDrawForPlumeSubmission(
    bool trace_enabled, bool compare_enabled, bool native_direct_draw_enabled) {
  return trace_enabled || compare_enabled || native_direct_draw_enabled;
}

constexpr uint32_t DirectPlumeSubmissionRecordScanCount(
    uint32_t record_count, uint32_t record_limit, bool compare_enabled,
    bool native_direct_draw_enabled) {
  if (compare_enabled || native_direct_draw_enabled) {
    return record_count;
  }
  return DirectCompareReplayRecordScanCount(record_count, record_limit);
}

constexpr bool ShouldStopDirectPlumeRecordScanAfterNativeAttempt(
    bool native_direct_draw_enabled, bool compare_enabled,
    bool native_live_batch_enabled) {
  return native_direct_draw_enabled && !compare_enabled &&
         !native_live_batch_enabled;
}

constexpr uint32_t NativeDirectDrawLiveBatchSize(uint32_t configured_size) {
  return configured_size == 0 ? 1u : configured_size;
}

constexpr bool ShouldFlushNativeDirectDrawLiveBatch(
    uint32_t queued_count, uint32_t configured_size) {
  return queued_count != 0 &&
         queued_count >= NativeDirectDrawLiveBatchSize(configured_size);
}

constexpr bool ShouldPromoteDirectReplayToNativeLayout(
    bool compare_enabled, bool native_direct_draw_enabled,
    bool native_live_batch_enabled) {
  (void)native_live_batch_enabled;
  return compare_enabled || native_direct_draw_enabled;
}

constexpr bool DirectDebugReplaySubmitLimitReached(uint64_t attempt,
                                                   uint32_t limit,
                                                   bool compare_enabled) {
  return !compare_enabled && limit != 0 && attempt > limit;
}

constexpr bool ShouldArmD3DCommandContextAfterDirectTrace(uint32_t sample_limit,
                                                          bool already_armed) {
  return sample_limit != 0 && !already_armed;
}

constexpr bool ShouldTraceD3DCommandContextAfterDirectSample(uint32_t samples_remaining) {
  return samples_remaining != 0;
}

enum class DirectDrawCompiledStateSection : uint8_t {
  kSkip,
  kCopy,
  kMaskValue,
};

struct DirectDrawCompiledStateEntry {
  DirectDrawCompiledStateSection section = DirectDrawCompiledStateSection::kSkip;
  uint32_t table_offset = 0;
  uint32_t target_offset = 0;
  uint32_t dword_count = 0;
  uint32_t payload_offset = 0;
  uint32_t payload_bytes = 0;
  bool truncated = false;
};

struct DirectDrawCompiledStateMaskValuePair {
  bool valid = false;
  uint32_t pair_index = 0;
  uint32_t payload_offset = 0;
  uint32_t state_offset = 0;
  uint32_t state_dword_index = 0;
  uint32_t fetch_register = 0;
  uint32_t fetch_group = 0;
  uint32_t fetch_group_dword = 0;
  uint32_t mask = 0;
  uint32_t value = 0;
};

struct DirectDrawCompiledStateTableSummary {
  bool valid = false;
  bool truncated = false;
  bool entries_truncated = false;
  uint32_t header_w00 = 0;
  uint32_t header_w04 = 0;
  uint32_t header_w08 = 0;
  uint32_t header_w0c = 0;
  uint32_t declared_payload_bytes = 0;
  uint32_t entry_count = 0;
  DirectDrawCompiledStateEntry entries[kDirectDrawCompiledStateMaxEntries] = {};
};

struct XenosControlFlowInstructionBits {
  uint32_t dword_0 = 0;
  uint32_t dword_1 = 0;

  constexpr uint32_t opcode() const { return (dword_1 >> 12) & 0xFu; }
  constexpr uint32_t exec_address() const { return dword_0 & 0xFFFu; }
  constexpr uint32_t exec_count() const { return (dword_0 >> 12) & 0x7u; }
};

struct DirectDrawShaderUcodeBounds {
  bool valid = false;
  bool saw_exec_end = false;
  bool truncated = false;
  uint32_t scanned_bytes = 0;
  uint32_t scanned_dwords = 0;
  uint32_t cf_pair_count_available = 0;
  uint32_t cf_pair_index_bound = 0;
  uint32_t cf_byte_count = 0;
  uint32_t first_exec_cf_index = 0;
  uint32_t first_exec_opcode = 0;
  uint32_t first_exec_address = 0;
  uint32_t exec_instruction_count = 0;
  uint32_t exec_high_water_instruction = 0;
  uint32_t exec_high_water_bytes = 0;
  uint32_t total_used_bytes = 0;
};

struct DirectDrawShaderUcodeCandidate {
  bool valid = false;
  uint32_t dword_offset = 0;
  uint32_t byte_offset = 0;
  DirectDrawShaderUcodeBounds bounds;
};

constexpr bool IsXenosControlFlowExecOpcode(uint32_t opcode) {
  return opcode == 1u || opcode == 2u || opcode == 3u || opcode == 4u || opcode == 5u ||
         opcode == 6u || opcode == 13u || opcode == 14u;
}

constexpr bool IsXenosControlFlowEndShaderOpcode(uint32_t opcode) {
  return opcode == 2u || opcode == 4u || opcode == 6u || opcode == 14u;
}

constexpr void UnpackXenosControlFlowPair(const uint32_t* dwords,
                                          XenosControlFlowInstructionBits (&out)[2]) {
  out[0].dword_0 = dwords[0];
  out[0].dword_1 = dwords[1] & 0xFFFFu;
  out[1].dword_0 = (dwords[1] >> 16) | (dwords[2] << 16);
  out[1].dword_1 = dwords[2] >> 16;
}

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

constexpr uint32_t TriangleStripPrimitiveCountFromIndexCount(uint32_t index_count) {
  return index_count >= 3u ? index_count - 2u : 0u;
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

constexpr uint32_t DirectDrawLittleEndianValueFromGuestDword(uint32_t guest_dword) {
  return ((guest_dword & 0x000000FFu) << 24) | ((guest_dword & 0x0000FF00u) << 8) |
         ((guest_dword & 0x00FF0000u) >> 8) | ((guest_dword & 0xFF000000u) >> 24);
}

constexpr uint16_t DirectDrawLoadBE16(const uint8_t* bytes, uint32_t offset) {
  return static_cast<uint16_t>((static_cast<uint16_t>(bytes[offset]) << 8) |
                               static_cast<uint16_t>(bytes[offset + 1u]));
}

constexpr uint32_t DirectDrawLoadBE32(const uint8_t* bytes, uint32_t offset) {
  return (static_cast<uint32_t>(bytes[offset + 0u]) << 24) |
         (static_cast<uint32_t>(bytes[offset + 1u]) << 16) |
         (static_cast<uint32_t>(bytes[offset + 2u]) << 8) |
         static_cast<uint32_t>(bytes[offset + 3u]);
}

constexpr const char* D3DCommandContextFetchGroupKindName(
    D3DCommandContextFetchGroupKind kind) {
  switch (kind) {
    case D3DCommandContextFetchGroupKind::kTexture:
      return "texture";
    case D3DCommandContextFetchGroupKind::kVertexTriplet:
      return "vertex_triplet";
  }
  return "unknown";
}

constexpr bool D3DCommandContextFetchGroupTypeIsTexture(uint32_t fetch_type) {
  return fetch_type == 0u || fetch_type == 2u;
}

constexpr D3DCommandContextVertexFetchSummary
D3DCommandContextDecodeVertexFetch(uint32_t dword_0, uint32_t dword_1) {
  D3DCommandContextVertexFetchSummary summary;
  summary.type = dword_0 & 0x3u;
  summary.address_words = dword_0 >> 2;
  summary.address_bytes = summary.address_words << 2;
  summary.endian = dword_1 & 0x3u;
  summary.size_words = (dword_1 >> 2) & 0x00FFFFFFu;
  summary.size_bytes = summary.size_words << 2;
  return summary;
}

constexpr bool D3DCommandContextVertexFetchIsValid(
    const D3DCommandContextVertexFetchSummary& vertex_fetch) {
  return vertex_fetch.type == 3u && vertex_fetch.size_words != 0;
}

constexpr uint32_t D3DCommandContextVertexFetchConstantIndex(
    uint32_t group_index, uint32_t vertex_index_in_group) {
  return group_index * 3u + vertex_index_in_group;
}

constexpr D3DCommandContextTextureFetchSummary
D3DCommandContextDecodeTextureFetch(const uint32_t (&dwords)[6]) {
  D3DCommandContextTextureFetchSummary summary;
  summary.type = dwords[0] & 0x3u;
  summary.pitch = (dwords[0] >> 22) & 0x1FFu;
  summary.tiled = (dwords[0] >> 31) & 0x1u;
  summary.format = dwords[1] & 0x3Fu;
  summary.endian = (dwords[1] >> 6) & 0x3u;
  summary.base_address = ((dwords[1] >> 12) & 0xFFFFFu) << 12;
  summary.width = (dwords[2] & 0x1FFFu) + 1u;
  summary.height = ((dwords[2] >> 13) & 0x1FFFu) + 1u;
  summary.depth_or_stack = ((dwords[2] >> 26) & 0x3Fu) + 1u;
  summary.dimension = (dwords[5] >> 9) & 0x3u;
  summary.mip_address = ((dwords[5] >> 12) & 0xFFFFFu) << 12;
  return summary;
}

constexpr D3DCommandContextFetchGroupSummary DecodeD3DCommandContextFetchGroup(
    const uint8_t* state_shadow_bytes, uint32_t state_shadow_byte_count,
    uint32_t group_index) {
  D3DCommandContextFetchGroupSummary summary;
  summary.group_index = group_index;
  summary.register_base =
      kDirectDrawFetchConstantRegisterBase +
      group_index * kD3DCommandContextFetchGroupDwordCount;

  const uint64_t group_offset =
      uint64_t(group_index) * kD3DCommandContextFetchGroupByteCount;
  if (!state_shadow_bytes ||
      group_index >= kD3DCommandContextFetchGroupCount ||
      group_offset + kD3DCommandContextFetchGroupByteCount >
          state_shadow_byte_count) {
    return summary;
  }

  summary.valid = true;
  for (uint32_t i = 0; i < kD3DCommandContextFetchGroupDwordCount; ++i) {
    summary.dwords[i] = DirectDrawLoadBE32(
        state_shadow_bytes,
        static_cast<uint32_t>(group_offset) + i * sizeof(uint32_t));
  }

  const uint32_t fetch_type = summary.dwords[0] & 0x3u;
  if (D3DCommandContextFetchGroupTypeIsTexture(fetch_type)) {
    summary.kind = D3DCommandContextFetchGroupKind::kTexture;
    summary.texture = D3DCommandContextDecodeTextureFetch(summary.dwords);
  } else {
    summary.kind = D3DCommandContextFetchGroupKind::kVertexTriplet;
    summary.vertex[0] =
        D3DCommandContextDecodeVertexFetch(summary.dwords[0], summary.dwords[1]);
    summary.vertex[1] =
        D3DCommandContextDecodeVertexFetch(summary.dwords[2], summary.dwords[3]);
    summary.vertex[2] =
        D3DCommandContextDecodeVertexFetch(summary.dwords[4], summary.dwords[5]);
  }

  return summary;
}

constexpr uint32_t DirectDrawCompiledStateMaskValuePairCount(
    const DirectDrawCompiledStateEntry& entry) {
  if (entry.section != DirectDrawCompiledStateSection::kMaskValue ||
      (entry.dword_count & 1u) != 0) {
    return 0;
  }
  return entry.dword_count / 2u;
}

constexpr uint32_t DirectDrawCompiledStateFetchRegisterFromStateOffset(
    uint32_t state_offset) {
  if ((state_offset & 3u) != 0) {
    return 0;
  }
  return kDirectDrawFetchConstantRegisterBase + (state_offset / sizeof(uint32_t));
}

constexpr uint32_t DirectDrawCompiledStateFetchGroupFromStateOffset(
    uint32_t state_offset) {
  if ((state_offset & 3u) != 0) {
    return 0;
  }
  return (state_offset / sizeof(uint32_t)) / kDirectDrawFetchConstantGroupDwordCount;
}

constexpr uint32_t DirectDrawCompiledStateFetchGroupDwordFromStateOffset(
    uint32_t state_offset) {
  if ((state_offset & 3u) != 0) {
    return 0;
  }
  return (state_offset / sizeof(uint32_t)) % kDirectDrawFetchConstantGroupDwordCount;
}

constexpr DirectDrawCompiledStateMaskValuePair
DirectDrawReadCompiledStateMaskValuePair(
    const uint8_t* table_bytes, uint32_t table_byte_count,
    const DirectDrawCompiledStateEntry& entry, uint32_t pair_index) {
  DirectDrawCompiledStateMaskValuePair pair;
  const uint32_t pair_count = DirectDrawCompiledStateMaskValuePairCount(entry);
  if (!table_bytes || pair_index >= pair_count) {
    return pair;
  }

  const uint64_t payload_offset =
      uint64_t(entry.payload_offset) + uint64_t(pair_index) * 2u * sizeof(uint32_t);
  const uint64_t payload_end = payload_offset + 2u * sizeof(uint32_t);
  const uint64_t state_offset =
      uint64_t(entry.target_offset) + uint64_t(pair_index) * sizeof(uint32_t);
  if (payload_end > table_byte_count || state_offset > 0xFFFFFFFFu) {
    return pair;
  }

  pair.valid = true;
  pair.pair_index = pair_index;
  pair.payload_offset = static_cast<uint32_t>(payload_offset);
  pair.state_offset = static_cast<uint32_t>(state_offset);
  pair.state_dword_index = pair.state_offset / sizeof(uint32_t);
  pair.fetch_register =
      DirectDrawCompiledStateFetchRegisterFromStateOffset(pair.state_offset);
  pair.fetch_group =
      DirectDrawCompiledStateFetchGroupFromStateOffset(pair.state_offset);
  pair.fetch_group_dword =
      DirectDrawCompiledStateFetchGroupDwordFromStateOffset(pair.state_offset);
  pair.mask = DirectDrawLoadBE32(table_bytes, pair.payload_offset);
  pair.value =
      DirectDrawLoadBE32(table_bytes, pair.payload_offset + sizeof(uint32_t));
  return pair;
}

constexpr const char* DirectDrawCompiledStateSectionName(
    DirectDrawCompiledStateSection section) {
  switch (section) {
    case DirectDrawCompiledStateSection::kSkip:
      return "skip";
    case DirectDrawCompiledStateSection::kCopy:
      return "copy";
    case DirectDrawCompiledStateSection::kMaskValue:
      return "mask_value";
  }
  return "unknown";
}

constexpr uint32_t DirectDrawResourceReadableByteCount(uint32_t raw_byte_size) {
  return raw_byte_size & kD3DResourceByteSizeMask;
}

constexpr uint32_t DirectDrawIndexElementByteCount(uint32_t descriptor_format) {
  switch (descriptor_format) {
    case 1u:
      return 2u;
    case 2u:
      return 4u;
    default:
      return 0u;
  }
}

constexpr uint32_t SaturatingMulU32(uint32_t a, uint32_t b) {
  if (a == 0 || b == 0) {
    return 0;
  }
  if (a > 0xFFFFFFFFu / b) {
    return 0xFFFFFFFFu;
  }
  return a * b;
}

constexpr uint32_t DirectDrawBufferViewByteCount(uint32_t descriptor_count,
                                                 uint32_t descriptor_stride_or_format,
                                                 bool index_buffer) {
  const uint32_t element_bytes =
      index_buffer ? DirectDrawIndexElementByteCount(descriptor_stride_or_format)
                   : descriptor_stride_or_format;
  return SaturatingMulU32(descriptor_count, element_bytes);
}

constexpr uint32_t BoundedDirectDrawBufferHashByteCount(uint32_t resource_readable_bytes,
                                                       uint32_t descriptor_view_bytes) {
  if (resource_readable_bytes == 0) {
    return 0;
  }
  if (descriptor_view_bytes == 0 || descriptor_view_bytes > resource_readable_bytes) {
    return resource_readable_bytes;
  }
  return descriptor_view_bytes;
}

constexpr uint32_t DirectDrawReplayUploadGuestBase(uint32_t guest_base) {
  return guest_base & ~uint32_t(3u);
}

constexpr bool DirectDrawAddCompiledStateEntry(
    DirectDrawCompiledStateTableSummary& summary,
    DirectDrawCompiledStateSection section, uint32_t table_offset,
    uint32_t target_offset, uint32_t dword_count, uint32_t payload_offset,
    uint32_t payload_bytes, uint32_t available_bytes) {
  const uint64_t payload_end = uint64_t(payload_offset) + payload_bytes;
  const bool truncated = payload_end > available_bytes;
  summary.truncated = summary.truncated || truncated;
  if (summary.entry_count >= kDirectDrawCompiledStateMaxEntries) {
    summary.entries_truncated = true;
    return truncated;
  }

  DirectDrawCompiledStateEntry& entry = summary.entries[summary.entry_count++];
  entry.section = section;
  entry.table_offset = table_offset;
  entry.target_offset = target_offset;
  entry.dword_count = dword_count;
  entry.payload_offset = payload_offset;
  entry.payload_bytes = payload_bytes;
  entry.truncated = truncated;
  return truncated;
}

constexpr bool DirectDrawCompiledStateCursorBeforeDeclaredEnd(
    uint32_t cursor, uint32_t declared_payload_bytes) {
  return declared_payload_bytes == 0 ||
         uint64_t(cursor) < uint64_t(kDirectDrawCompiledStateHeaderSize) +
                                declared_payload_bytes;
}

constexpr uint32_t DirectDrawCompiledStateTrailingEntryOffset(
    uint32_t declared_payload_bytes) {
  if (declared_payload_bytes == 0) {
    return 0;
  }
  return kDirectDrawCompiledStateHeaderSize + declared_payload_bytes;
}

constexpr bool DirectDrawCompiledStateHasEntryAt(
    const DirectDrawCompiledStateTableSummary& summary, uint32_t table_offset) {
  for (uint32_t i = 0; i < summary.entry_count; ++i) {
    if (summary.entries[i].table_offset == table_offset) {
      return true;
    }
  }
  return false;
}

constexpr DirectDrawCompiledStateTableSummary AnalyzeDirectDrawCompiledStateTable(
    const uint8_t* table_bytes, uint32_t table_byte_count) {
  DirectDrawCompiledStateTableSummary summary;
  if (!table_bytes || table_byte_count < kDirectDrawCompiledStateHeaderSize) {
    return summary;
  }

  summary.valid = true;
  summary.header_w00 = DirectDrawLoadBE32(table_bytes, 0x00u);
  summary.header_w04 = DirectDrawLoadBE32(table_bytes, 0x04u);
  summary.header_w08 = DirectDrawLoadBE32(table_bytes, 0x08u);
  summary.header_w0c = DirectDrawLoadBE32(table_bytes, 0x0Cu);
  summary.declared_payload_bytes = DirectDrawLoadBE32(table_bytes, 0x10u);

  uint32_t cursor = kDirectDrawCompiledStateHeaderSize;

  while (cursor + 4u <= table_byte_count &&
         DirectDrawCompiledStateCursorBeforeDeclaredEnd(
             cursor, summary.declared_payload_bytes)) {
    const uint32_t table_offset = cursor;
    const uint32_t target_offset = DirectDrawLoadBE16(table_bytes, cursor);
    const uint32_t dword_count = DirectDrawLoadBE16(table_bytes, cursor + 2u);
    cursor += 4u;
    if (dword_count == 0) {
      break;
    }

    DirectDrawAddCompiledStateEntry(summary, DirectDrawCompiledStateSection::kSkip,
                                    table_offset, target_offset, dword_count, cursor,
                                    sizeof(uint32_t), table_byte_count);
    cursor += sizeof(uint32_t);
  }

  if (DirectDrawCompiledStateCursorBeforeDeclaredEnd(
          cursor, summary.declared_payload_bytes)) {
    while (cursor + 4u <= table_byte_count &&
           DirectDrawCompiledStateCursorBeforeDeclaredEnd(
               cursor, summary.declared_payload_bytes)) {
      const uint32_t table_offset = cursor;
      const uint32_t target_offset = DirectDrawLoadBE16(table_bytes, cursor);
      const uint32_t dword_count = DirectDrawLoadBE16(table_bytes, cursor + 2u);
      cursor += 4u;
      if (dword_count == 0) {
        break;
      }

      const uint32_t payload_bytes = SaturatingMulU32(dword_count, sizeof(uint32_t));
      DirectDrawAddCompiledStateEntry(summary, DirectDrawCompiledStateSection::kCopy,
                                      table_offset, target_offset, dword_count,
                                      cursor, payload_bytes, table_byte_count);
      const uint64_t next = uint64_t(cursor) + payload_bytes;
      cursor = next > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(next);
    }
  }

  if (DirectDrawCompiledStateCursorBeforeDeclaredEnd(
          cursor, summary.declared_payload_bytes)) {
    while (cursor + 4u <= table_byte_count &&
           DirectDrawCompiledStateCursorBeforeDeclaredEnd(
               cursor, summary.declared_payload_bytes)) {
      const uint32_t table_offset = cursor;
      const uint32_t target_offset = DirectDrawLoadBE16(table_bytes, cursor);
      const uint32_t dword_count = DirectDrawLoadBE16(table_bytes, cursor + 2u);
      cursor += 4u;
      if (dword_count == 0) {
        break;
      }

      const uint32_t payload_bytes = SaturatingMulU32(dword_count, sizeof(uint32_t));
      DirectDrawAddCompiledStateEntry(
          summary, DirectDrawCompiledStateSection::kMaskValue, table_offset,
          target_offset, dword_count, cursor, payload_bytes, table_byte_count);
      const uint64_t next = uint64_t(cursor) + payload_bytes;
      cursor = next > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(next);
    }
  }

  if (summary.declared_payload_bytes >= sizeof(uint32_t)) {
    const uint32_t trailing_table_offset =
        DirectDrawCompiledStateTrailingEntryOffset(summary.declared_payload_bytes);
    if (trailing_table_offset + 4u <= table_byte_count &&
        !DirectDrawCompiledStateHasEntryAt(summary, trailing_table_offset)) {
      const uint32_t target_offset =
          DirectDrawLoadBE16(table_bytes, trailing_table_offset);
      const uint32_t dword_count =
          DirectDrawLoadBE16(table_bytes, trailing_table_offset + 2u);
      if (dword_count != 0) {
        DirectDrawAddCompiledStateEntry(
            summary, DirectDrawCompiledStateSection::kMaskValue,
            trailing_table_offset, target_offset, dword_count,
            trailing_table_offset + sizeof(uint32_t),
            SaturatingMulU32(dword_count, sizeof(uint32_t)), table_byte_count);
      }
    }
  }

  if (cursor + 4u > table_byte_count &&
      DirectDrawCompiledStateCursorBeforeDeclaredEnd(
          cursor, summary.declared_payload_bytes)) {
    summary.truncated = true;
  }
  return summary;
}

struct DirectDrawBufferViewSummary {
  uint32_t gpu_base = 0;
  uint32_t upload_guest_base = 0;
  uint32_t raw_byte_size = 0;
  uint32_t readable_bytes = 0;
  uint32_t descriptor_count = 0;
  uint32_t descriptor_stride_or_format = 0;
  uint32_t element_bytes = 0;
  uint32_t view_bytes = 0;
  uint32_t hash_bytes = 0;
  uint64_t hash = 0;
  bool index_buffer = false;
  bool hash_ok = false;

  constexpr bool HasReplayableView() const {
    return gpu_base != 0 && upload_guest_base != 0 && hash_ok &&
           hash_bytes != 0 && view_bytes != 0;
  }
};

struct DirectDrawShaderKeySummary {
  uint32_t payload_base = 0;
  uint32_t payload_bytes = 0;
  uint64_t payload_hash = 0;
  uint32_t structural_ucode_bytes = 0;
  uint64_t structural_ucode_hash = 0;
  bool payload_hash_ok = false;
  bool structural_ucode_hash_ok = false;

  constexpr bool HasPayloadKey() const {
    return payload_base != 0 && payload_bytes != 0 && payload_hash_ok;
  }

  constexpr bool HasStructuralUcodeKey() const {
    return structural_ucode_bytes != 0 && structural_ucode_hash_ok;
  }
};

struct DirectDrawSegmentSummary {
  bool valid = false;
  uint16_t raw_w0 = 0;
  uint16_t raw_w2 = 0;
  uint16_t start_index = 0;
  uint16_t index_count = 0;
};

enum class DirectDrawReplayTopology : uint8_t {
  kUnknown,
  kTriangleList,
  kTriangleStrip,
};

struct DirectDrawLiveDrawFilter {
  bool enabled = false;
  DirectDrawReplayTopology topology = DirectDrawReplayTopology::kUnknown;
  uint32_t start_index = 0;
  uint32_t index_count = 0;
  uint32_t stream0_resource = 0;
  uint32_t index_resource = 0;
};

constexpr DirectDrawReplayTopology
DirectDrawReplayTopologyFromDirectIfacePrimitiveType(uint32_t primitive_type) {
  if (primitive_type == 4u) {
    return DirectDrawReplayTopology::kTriangleList;
  }
  return DirectDrawReplayTopology::kUnknown;
}

constexpr uint32_t DirectDrawReplayIndexCountFromPrimitiveCount(
    DirectDrawReplayTopology topology, uint32_t primitive_count) {
  switch (topology) {
    case DirectDrawReplayTopology::kTriangleList:
      return primitive_count * 3u;
    case DirectDrawReplayTopology::kTriangleStrip:
      return primitive_count + 2u;
    case DirectDrawReplayTopology::kUnknown:
      break;
  }
  return 0;
}

constexpr DirectDrawLiveDrawFilter BuildDirectDrawLiveDrawFilter(
    uint32_t primitive_type, uint32_t start_index, uint32_t primitive_count,
    uint32_t stream0_resource, uint32_t index_resource) {
  DirectDrawLiveDrawFilter out;
  out.topology = DirectDrawReplayTopologyFromDirectIfacePrimitiveType(
      primitive_type);
  out.start_index = start_index;
  out.index_count =
      DirectDrawReplayIndexCountFromPrimitiveCount(out.topology,
                                                   primitive_count);
  out.stream0_resource = stream0_resource;
  out.index_resource = index_resource;
  out.enabled = out.topology != DirectDrawReplayTopology::kUnknown &&
                out.index_count != 0 && stream0_resource != 0 &&
                index_resource != 0;
  return out;
}

constexpr bool DirectDrawLiveDrawFilterMatchesRecord(
    const DirectDrawLiveDrawFilter& filter, uint32_t stream0_resource,
    uint32_t index_resource, const DirectDrawSegmentSummary& segment) {
  if (!filter.enabled) {
    return true;
  }
  return segment.valid && stream0_resource == filter.stream0_resource &&
         index_resource == filter.index_resource &&
         segment.start_index == filter.start_index &&
         segment.index_count == filter.index_count;
}

struct DirectDrawIndexedPacketSummary {
  DirectDrawReplayTopology topology = DirectDrawReplayTopology::kUnknown;
  uint32_t record_index = 0;
  uint32_t first_index = 0;
  uint32_t index_count = 0;
  uint32_t primitive_count = 0;
  DirectDrawBufferViewSummary stream0;
  DirectDrawBufferViewSummary stream1;
  DirectDrawBufferViewSummary index;
  DirectDrawShaderKeySummary vertex_shader;
  DirectDrawShaderKeySummary pixel_shader;

  constexpr bool HasReplayableBuffers() const {
    return stream0.HasReplayableView() && stream1.HasReplayableView() &&
           index.HasReplayableView();
  }

  constexpr bool HasStableShaderPayloadKeys() const {
    return vertex_shader.HasPayloadKey() && pixel_shader.HasPayloadKey();
  }

  constexpr bool HasVertexStructuralUcodeKey() const {
    return vertex_shader.HasStructuralUcodeKey();
  }

  constexpr bool HasPixelStructuralUcodeKey() const {
    return pixel_shader.HasStructuralUcodeKey();
  }

  constexpr bool HasCompleteStructuralUcodeKeys() const {
    return HasVertexStructuralUcodeKey() && HasPixelStructuralUcodeKey();
  }

  constexpr bool CanAttemptDebugReplay() const {
    return topology != DirectDrawReplayTopology::kUnknown &&
           index_count != 0 && primitive_count != 0 && HasReplayableBuffers();
  }
};

enum class DirectDrawReplayIndexFormat : uint8_t {
  kUnknown,
  kUint16,
  kUint32,
};

enum class DirectDrawReplayUploadEndian : uint8_t {
  kRaw,
  kSwap16,
  kSwap32,
};

enum class DirectDrawReplayTransformInterpretation : uint8_t {
  kRowMajorClip,
  kColumnMajorClip,
};

enum class DirectDebugReplayPipelineTopology : uint8_t {
  kAuto,
  kTriangleList,
  kTriangleStrip,
};

enum class DirectDrawReplayPipelineLayout : uint8_t {
  kUnsupported,
  kDebugRaw32Side12,
  kNativePosition28Side12,
};

constexpr DirectDebugReplayPipelineTopology
ParseDirectDebugReplayPipelineTopology(std::string_view name) {
  if (name == "auto") {
    return DirectDebugReplayPipelineTopology::kAuto;
  }
  if (name == "triangle_strip") {
    return DirectDebugReplayPipelineTopology::kTriangleStrip;
  }
  if (name == "triangle_list") {
    return DirectDebugReplayPipelineTopology::kTriangleList;
  }
  return DirectDebugReplayPipelineTopology::kAuto;
}

struct DirectDrawReplayVertexStream {
  uint32_t slot = 0;
  uint32_t stride = 0;
  uint32_t guest_base = 0;
  uint32_t upload_guest_base = 0;
  uint32_t upload_bytes = 0;
  uint64_t hash = 0;
  DirectDrawReplayUploadEndian upload_endian = DirectDrawReplayUploadEndian::kRaw;
};

struct DirectDrawReplayFloat3PositionStats {
  bool valid = false;
  bool has_finite_bounds = false;
  uint32_t stride = 0;
  uint32_t position_offset = 0;
  uint32_t vertex_count = 0;
  uint32_t sampled_vertices = 0;
  uint32_t finite_vertices = 0;
  float min_x = 0.0f;
  float min_y = 0.0f;
  float min_z = 0.0f;
  float max_x = 0.0f;
  float max_y = 0.0f;
  float max_z = 0.0f;
};

struct DirectDrawFloat4ConstantSample {
  uint32_t constant_index = 0;
  float values[4] = {};
};

struct DirectDrawFloat4ConstantStats {
  static constexpr uint32_t kMaxSamples = 128u;

  bool valid = false;
  uint32_t constant_count = 0;
  uint32_t finite_constants = 0;
  uint32_t nonzero_constants = 0;
  uint32_t first_nonzero_constant = 0;
  uint32_t last_nonzero_constant = 0;
  uint32_t sample_count = 0;
  DirectDrawFloat4ConstantSample samples[kMaxSamples] = {};
};

struct DirectDrawDebugReplayTransform {
  bool valid = false;
  uint32_t first_constant = 0;
  float rows[kDirectDrawDebugReplayTransformRowCount]
            [kDirectDrawDebugReplayTransformColumnCount] = {};
};

struct DirectDrawReplayClipPositionStats {
  bool valid = false;
  bool transform_valid = false;
  bool has_clip_bounds = false;
  bool has_ndc_bounds = false;
  uint32_t stride = 0;
  uint32_t position_offset = 0;
  uint32_t vertex_count = 0;
  uint32_t sampled_vertices = 0;
  uint32_t finite_vertices = 0;
  uint32_t clip_finite_vertices = 0;
  uint32_t projectable_vertices = 0;
  uint32_t xy_inside_vertices = 0;
  uint32_t d3d_xyz_inside_vertices = 0;
  uint32_t gl_xyz_inside_vertices = 0;
  float min_clip_x = 0.0f;
  float min_clip_y = 0.0f;
  float min_clip_z = 0.0f;
  float min_w = 0.0f;
  float max_clip_x = 0.0f;
  float max_clip_y = 0.0f;
  float max_clip_z = 0.0f;
  float max_w = 0.0f;
  float min_ndc_x = 0.0f;
  float min_ndc_y = 0.0f;
  float min_ndc_z = 0.0f;
  float max_ndc_x = 0.0f;
  float max_ndc_y = 0.0f;
  float max_ndc_z = 0.0f;
};

struct DirectDrawDebugReplayTransformSelection {
  bool valid = false;
  std::string_view source_name;
  DirectDrawDebugReplayTransform transform;
  DirectDrawReplayTransformInterpretation interpretation =
      DirectDrawReplayTransformInterpretation::kColumnMajorClip;
  DirectDrawReplayClipPositionStats stats;
};

struct DirectDrawReplayVertexFetchMatch {
  bool valid = false;
  uint32_t stream_slot = 0;
  uint32_t fetch_group = 0;
  uint32_t fetch_group_slot = 0;
  uint32_t fetch_constant = 0;
  uint32_t stream_guest_base = 0;
  uint32_t stream_fetch_base = 0;
  uint32_t stream_bytes = 0;
  uint32_t fetch_base = 0;
  uint32_t fetch_bytes = 0;
};

struct DirectDrawReplayIndexBuffer {
  uint32_t guest_base = 0;
  uint32_t upload_guest_base = 0;
  uint32_t upload_bytes = 0;
  uint64_t hash = 0;
  DirectDrawReplayUploadEndian upload_endian = DirectDrawReplayUploadEndian::kRaw;
};

struct DirectDrawReplayIndexedDraw {
  uint32_t index_count = 0;
  uint32_t instance_count = 0;
  uint32_t start_index = 0;
  int32_t base_vertex = 0;
  uint32_t start_instance = 0;
};

struct DirectDrawReplayNativeStreamBinding {
  bool valid = false;
  uint32_t slot = 0;
  uint32_t resource = 0;
  uint32_t byte_offset = 0;
  uint32_t stride_bytes = 0;
  uint32_t dirty_mask = 0;
};

struct DirectDrawReplayNativeViewportSummary {
  bool valid = false;
  uint32_t viewport_mode = 0;
};

struct DirectDrawReplayNativeTextureFetchSummary {
  bool valid = false;
  uint8_t fetch_bits_low = 0;
  uint8_t fetch_bits_mid = 0;
};

struct DirectDrawReplayNativeClearSummary {
  bool valid = false;
  uint8_t clear_color_byte = 0;
  uint8_t clear_flags = 0;
};

struct DirectDrawReplayNativePassSummary {
  bool valid = false;
  uint32_t submit_object = 0;
  uint32_t tls_or_pass_context = 0;
  uint32_t pass_flags = 0;
  uint32_t drawable = 0;
  uint32_t draw_callback = 0;
  uint32_t wireframe = 0;
  uint32_t draw_mode = 0;
  uint32_t pass_marker = 0;
};

struct DirectDrawReplayNativeStateSummary {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t render_context = 0;
  uint32_t direct_render_context = 0;
  uint32_t draw_iface = 0;
  uint32_t vertex_shader = 0;
  uint32_t pixel_shader = 0;
  DirectDrawReplayNativeStreamBinding streams[2];
  uint32_t index_resource = 0;
  uint32_t bound_surface = 0;
  uint32_t bound_surface_arg = 0;
  DirectDrawReplayNativeViewportSummary viewport;
  DirectDrawReplayNativeTextureFetchSummary texture_fetch;
  DirectDrawReplayNativeClearSummary clear;
  DirectDrawReplayNativePassSummary pass;
};

constexpr DirectDrawReplayPipelineLayout DirectDrawReplayNativeLayoutFromState(
    const DirectDrawReplayNativeStateSummary& state) {
  if (!state.valid || !state.streams[0].valid || !state.streams[1].valid) {
    return DirectDrawReplayPipelineLayout::kUnsupported;
  }
  if (state.streams[0].slot == 0u && state.streams[0].stride_bytes == 28u &&
      state.streams[1].slot == 1u && state.streams[1].stride_bytes == 12u) {
    return DirectDrawReplayPipelineLayout::kNativePosition28Side12;
  }
  return DirectDrawReplayPipelineLayout::kUnsupported;
}

struct DirectDrawDebugReplayPlan {
  bool ready = false;
  DirectDrawReplayTopology topology = DirectDrawReplayTopology::kUnknown;
  DirectDrawReplayIndexFormat index_format = DirectDrawReplayIndexFormat::kUnknown;
  uint32_t stream_count = 0;
  DirectDrawReplayVertexStream streams[2];
  DirectDrawReplayIndexBuffer index;
  DirectDrawReplayIndexedDraw draw;
  DirectDrawDebugReplayTransform transform;
  uint64_t vertex_payload_hash = 0;
  uint64_t pixel_payload_hash = 0;
  uint64_t vertex_structural_ucode_hash = 0;
  uint64_t pixel_structural_ucode_hash = 0;
  bool has_vertex_structural_ucode = false;
  bool has_pixel_structural_ucode = false;
  DirectDrawReplayNativeStateSummary native_state;
};

constexpr const char* DirectDrawReplayPipelineLayoutName(
    DirectDrawReplayPipelineLayout layout) {
  switch (layout) {
    case DirectDrawReplayPipelineLayout::kDebugRaw32Side12:
      return "debug_raw32_side12";
    case DirectDrawReplayPipelineLayout::kNativePosition28Side12:
      return "native_position28_side12";
    case DirectDrawReplayPipelineLayout::kUnsupported:
      break;
  }
  return "unsupported";
}

constexpr DirectDrawReplayPipelineLayout DirectDrawReplayPipelineLayoutForPlan(
    const DirectDrawDebugReplayPlan& plan) {
  if (!plan.ready || plan.stream_count != 2u || plan.streams[0].slot != 0u ||
      plan.streams[1].slot != 1u) {
    return DirectDrawReplayPipelineLayout::kUnsupported;
  }
  if (plan.streams[0].stride == 0x20u && plan.streams[1].stride == 0x0Cu) {
    return DirectDrawReplayPipelineLayout::kDebugRaw32Side12;
  }
  if (plan.streams[0].stride == 28u && plan.streams[1].stride == 12u &&
      DirectDrawReplayNativeLayoutFromState(plan.native_state) ==
          DirectDrawReplayPipelineLayout::kNativePosition28Side12) {
    return DirectDrawReplayPipelineLayout::kNativePosition28Side12;
  }
  return DirectDrawReplayPipelineLayout::kUnsupported;
}

constexpr DirectDrawBufferViewSummary BuildDirectDrawBufferViewSummary(
    uint32_t gpu_base, uint32_t raw_byte_size, uint32_t descriptor_count,
    uint32_t descriptor_stride_or_format, bool index_buffer, bool hash_ok, uint64_t hash) {
  DirectDrawBufferViewSummary out;
  out.gpu_base = gpu_base;
  out.upload_guest_base = DirectDrawReplayUploadGuestBase(gpu_base);
  out.raw_byte_size = raw_byte_size;
  out.readable_bytes = DirectDrawResourceReadableByteCount(raw_byte_size);
  out.descriptor_count = descriptor_count;
  out.descriptor_stride_or_format = descriptor_stride_or_format;
  out.index_buffer = index_buffer;
  out.element_bytes =
      index_buffer ? DirectDrawIndexElementByteCount(descriptor_stride_or_format)
                   : descriptor_stride_or_format;
  out.view_bytes =
      DirectDrawBufferViewByteCount(descriptor_count, descriptor_stride_or_format,
                                    index_buffer);
  out.hash_bytes =
      BoundedDirectDrawBufferHashByteCount(out.readable_bytes, out.view_bytes);
  out.hash_ok = hash_ok;
  out.hash = hash;
  return out;
}

inline DirectDrawSegmentSummary DecodeDirectDrawSegmentSummary(
    const uint8_t* bytes, uint32_t byte_count) {
  DirectDrawSegmentSummary out;
  if (!bytes || byte_count < kDirectDrawSegmentStride) {
    return out;
  }

  out.raw_w0 = DirectDrawLoadBE16(bytes, 0u);
  out.raw_w2 = DirectDrawLoadBE16(bytes, 2u);
  out.start_index =
      DirectDrawLoadBE16(bytes, kDirectDrawSegmentStartOffset);
  out.index_count =
      DirectDrawLoadBE16(bytes, kDirectDrawSegmentIndexCountOffset);
  out.valid = true;
  return out;
}

constexpr DirectDrawReplayTopology DirectDrawReplayTopologyFromSegmentHeader(
    uint16_t raw_w0, uint16_t raw_w2) {
  if (raw_w0 == 0x8210u && raw_w2 == 0x94E8u) {
    return DirectDrawReplayTopology::kTriangleStrip;
  }
  return DirectDrawReplayTopology::kUnknown;
}

constexpr DirectDrawReplayTopology ResolveDirectDrawReplayTopology(
    DirectDrawReplayTopology from_header, uint32_t index_count) {
  if (from_header != DirectDrawReplayTopology::kUnknown) {
    return from_header;
  }
  if (index_count >= 3u && (index_count % 3u) == 0u) {
    return DirectDrawReplayTopology::kTriangleList;
  }
  return DirectDrawReplayTopology::kUnknown;
}

constexpr DirectDrawReplayTopology DirectDrawReplayTopologyFromSegmentSummary(
    const DirectDrawSegmentSummary& segment) {
  if (!segment.valid) {
    return DirectDrawReplayTopology::kUnknown;
  }
  return ResolveDirectDrawReplayTopology(
      DirectDrawReplayTopologyFromSegmentHeader(segment.raw_w0,
                                                segment.raw_w2),
      segment.index_count);
}

constexpr uint32_t DirectDrawReplayPrimitiveCountFromIndexCount(
    DirectDrawReplayTopology topology, uint32_t index_count) {
  switch (topology) {
    case DirectDrawReplayTopology::kTriangleList:
      return TriangleListPrimitiveCountFromIndexCount(index_count);
    case DirectDrawReplayTopology::kTriangleStrip:
      return TriangleStripPrimitiveCountFromIndexCount(index_count);
    case DirectDrawReplayTopology::kUnknown:
      break;
  }
  return 0;
}

constexpr DirectDrawShaderKeySummary BuildDirectDrawShaderKeySummary(
    uint32_t payload_base, uint32_t payload_bytes, bool payload_hash_ok,
    uint64_t payload_hash, uint32_t structural_ucode_bytes,
    bool structural_ucode_hash_ok, uint64_t structural_ucode_hash) {
  DirectDrawShaderKeySummary out;
  out.payload_base = payload_base;
  out.payload_bytes = payload_bytes;
  out.payload_hash_ok = payload_hash_ok;
  out.payload_hash = payload_hash;
  out.structural_ucode_bytes = structural_ucode_bytes;
  out.structural_ucode_hash_ok = structural_ucode_hash_ok;
  out.structural_ucode_hash = structural_ucode_hash;
  return out;
}

constexpr DirectDrawIndexedPacketSummary BuildDirectDrawIndexedPacketSummary(
    uint32_t record_index, DirectDrawReplayTopology topology,
    uint32_t first_index, uint32_t index_count,
    DirectDrawBufferViewSummary stream0, DirectDrawBufferViewSummary stream1,
    DirectDrawBufferViewSummary index, DirectDrawShaderKeySummary vertex_shader,
    DirectDrawShaderKeySummary pixel_shader) {
  DirectDrawIndexedPacketSummary out;
  out.topology = topology;
  out.record_index = record_index;
  out.first_index = first_index;
  out.index_count = index_count;
  out.primitive_count =
      DirectDrawReplayPrimitiveCountFromIndexCount(topology, index_count);
  out.stream0 = stream0;
  out.stream1 = stream1;
  out.index = index;
  out.vertex_shader = vertex_shader;
  out.pixel_shader = pixel_shader;
  return out;
}

constexpr DirectDrawReplayIndexFormat DirectDrawReplayIndexFormatFromElementBytes(
    uint32_t element_bytes) {
  switch (element_bytes) {
    case 2u:
      return DirectDrawReplayIndexFormat::kUint16;
    case 4u:
      return DirectDrawReplayIndexFormat::kUint32;
    default:
      return DirectDrawReplayIndexFormat::kUnknown;
  }
}

constexpr DirectDrawReplayUploadEndian DirectDrawReplayIndexUploadEndianFromElementBytes(
    uint32_t element_bytes) {
  switch (element_bytes) {
    case 2u:
      return DirectDrawReplayUploadEndian::kSwap16;
    case 4u:
      return DirectDrawReplayUploadEndian::kSwap32;
    default:
      return DirectDrawReplayUploadEndian::kRaw;
  }
}

constexpr DirectDrawReplayVertexStream BuildDirectDrawReplayVertexStream(
    uint32_t slot, const DirectDrawBufferViewSummary& view) {
  DirectDrawReplayVertexStream out;
  out.slot = slot;
  out.stride = view.element_bytes;
  out.guest_base = view.gpu_base;
  out.upload_guest_base = view.upload_guest_base;
  out.upload_bytes = view.hash_bytes;
  out.hash = view.hash;
  out.upload_endian = DirectDrawReplayUploadEndian::kSwap32;
  return out;
}

inline float DirectDrawReplayFloatFromGuestBE32(uint32_t bits) {
  float value = 0.0f;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

inline float DirectDrawFloatFromHostBits(uint32_t bits) {
  float value = 0.0f;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

inline DirectDrawDebugReplayTransform BuildDirectDrawDebugReplayTransformFromConstants(
    const uint32_t* constant_registers, uint32_t constant_count,
    uint32_t first_constant) {
  DirectDrawDebugReplayTransform out;
  out.first_constant = first_constant;
  if (!constant_registers ||
      first_constant > constant_count ||
      constant_count - first_constant < kDirectDrawDebugReplayTransformRowCount) {
    return out;
  }

  for (uint32_t row = 0; row < kDirectDrawDebugReplayTransformRowCount; ++row) {
    const uint32_t constant_index = first_constant + row;
    for (uint32_t component = 0;
         component < kDirectDrawDebugReplayTransformColumnCount; ++component) {
      const float value = DirectDrawFloatFromHostBits(
          constant_registers[constant_index * 4u + component]);
      if (!std::isfinite(value)) {
        return out;
      }
      out.rows[row][component] = value;
    }
  }
  out.valid = true;
  return out;
}

inline DirectDrawDebugReplayTransform MultiplyDirectDrawReplayRowMajorTransforms(
    const DirectDrawDebugReplayTransform& left,
    const DirectDrawDebugReplayTransform& right, uint32_t first_constant) {
  DirectDrawDebugReplayTransform out;
  out.first_constant = first_constant;
  if (!left.valid || !right.valid) {
    return out;
  }

  for (uint32_t row = 0; row < kDirectDrawDebugReplayTransformRowCount; ++row) {
    for (uint32_t column = 0;
         column < kDirectDrawDebugReplayTransformColumnCount; ++column) {
      float value = 0.0f;
      for (uint32_t k = 0; k < kDirectDrawDebugReplayTransformColumnCount; ++k) {
        value += left.rows[row][k] * right.rows[k][column];
      }
      out.rows[row][column] = value;
    }
  }
  out.valid = true;
  return out;
}

inline DirectDrawDebugReplayTransform BuildDirectDrawDebugReplayTransformFromSource(
    const uint32_t* constant_registers, uint32_t constant_count,
    std::string_view source) {
  const DirectDrawDebugReplayTransform c28 =
      BuildDirectDrawDebugReplayTransformFromConstants(
          constant_registers, constant_count,
          kDirectDrawDebugReplayWorldTransformFirstConstant);
  if (source == "c0") {
    return BuildDirectDrawDebugReplayTransformFromConstants(
        constant_registers, constant_count, 0u);
  }
  if (source == "c36_mul_c28") {
    const DirectDrawDebugReplayTransform c36 =
        BuildDirectDrawDebugReplayTransformFromConstants(
            constant_registers, constant_count, 36u);
    return MultiplyDirectDrawReplayRowMajorTransforms(c36, c28, 36u);
  }
  return c28;
}

inline DirectDrawFloat4ConstantStats AnalyzeDirectDrawFloat4ConstantStats(
    const uint32_t* constant_registers, uint32_t constant_count,
    uint32_t max_sample_constants) {
  DirectDrawFloat4ConstantStats stats;
  if (!constant_registers || constant_count == 0) {
    return stats;
  }

  stats.valid = true;
  stats.constant_count = constant_count;
  const uint32_t sample_limit =
      max_sample_constants < DirectDrawFloat4ConstantStats::kMaxSamples
          ? max_sample_constants
          : DirectDrawFloat4ConstantStats::kMaxSamples;

  for (uint32_t i = 0; i < constant_count; ++i) {
    const uint32_t* raw = constant_registers + i * 4u;
    float values[4] = {
        DirectDrawFloatFromHostBits(raw[0]),
        DirectDrawFloatFromHostBits(raw[1]),
        DirectDrawFloatFromHostBits(raw[2]),
        DirectDrawFloatFromHostBits(raw[3]),
    };

    const bool finite = std::isfinite(values[0]) && std::isfinite(values[1]) &&
                        std::isfinite(values[2]) && std::isfinite(values[3]);
    const bool nonzero = values[0] != 0.0f || values[1] != 0.0f ||
                         values[2] != 0.0f || values[3] != 0.0f;
    if (finite) {
      ++stats.finite_constants;
    }
    if (!nonzero) {
      continue;
    }

    if (stats.nonzero_constants == 0) {
      stats.first_nonzero_constant = i;
    }
    stats.last_nonzero_constant = i;
    ++stats.nonzero_constants;

    if (stats.sample_count < sample_limit) {
      DirectDrawFloat4ConstantSample& sample = stats.samples[stats.sample_count++];
      sample.constant_index = i;
      sample.values[0] = values[0];
      sample.values[1] = values[1];
      sample.values[2] = values[2];
      sample.values[3] = values[3];
    }
  }

  return stats;
}

inline DirectDrawReplayFloat3PositionStats
AnalyzeDirectDrawReplayFloat3PositionStats(const uint8_t* upload_bytes,
                                           uint32_t byte_count, uint32_t stride,
                                           uint32_t position_offset,
                                           uint32_t max_sample_vertices) {
  DirectDrawReplayFloat3PositionStats stats;
  stats.stride = stride;
  stats.position_offset = position_offset;
  if (!upload_bytes || stride == 0 || position_offset > stride ||
      stride - position_offset < 12u || byte_count < position_offset + 12u) {
    return stats;
  }

  stats.vertex_count = byte_count / stride;
  if (stats.vertex_count == 0) {
    return stats;
  }

  stats.valid = true;
  stats.sampled_vertices = stats.vertex_count;
  if (max_sample_vertices != 0 &&
      stats.sampled_vertices > max_sample_vertices) {
    stats.sampled_vertices = max_sample_vertices;
  }

  for (uint32_t vertex_index = 0; vertex_index < stats.sampled_vertices;
       ++vertex_index) {
    const uint32_t vertex_offset = vertex_index * stride + position_offset;
    const float x = DirectDrawReplayFloatFromGuestBE32(
        DirectDrawLoadBE32(upload_bytes, vertex_offset + 0u));
    const float y = DirectDrawReplayFloatFromGuestBE32(
        DirectDrawLoadBE32(upload_bytes, vertex_offset + 4u));
    const float z = DirectDrawReplayFloatFromGuestBE32(
        DirectDrawLoadBE32(upload_bytes, vertex_offset + 8u));
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
      continue;
    }

    if (!stats.has_finite_bounds) {
      stats.min_x = stats.max_x = x;
      stats.min_y = stats.max_y = y;
      stats.min_z = stats.max_z = z;
      stats.has_finite_bounds = true;
    } else {
      if (x < stats.min_x) {
        stats.min_x = x;
      }
      if (y < stats.min_y) {
        stats.min_y = y;
      }
      if (z < stats.min_z) {
        stats.min_z = z;
      }
      if (x > stats.max_x) {
        stats.max_x = x;
      }
      if (y > stats.max_y) {
        stats.max_y = y;
      }
      if (z > stats.max_z) {
        stats.max_z = z;
      }
    }
    ++stats.finite_vertices;
  }

  return stats;
}

inline void DirectDrawReplayTransformPosition(
    const DirectDrawDebugReplayTransform& transform,
    DirectDrawReplayTransformInterpretation interpretation, const float* local,
    float* clip) {
  if (interpretation == DirectDrawReplayTransformInterpretation::kRowMajorClip) {
    for (uint32_t row = 0; row < kDirectDrawDebugReplayTransformRowCount; ++row) {
      clip[row] = transform.rows[row][0] * local[0] +
                  transform.rows[row][1] * local[1] +
                  transform.rows[row][2] * local[2] +
                  transform.rows[row][3] * local[3];
    }
    return;
  }

  for (uint32_t component = 0;
       component < kDirectDrawDebugReplayTransformColumnCount; ++component) {
    clip[component] = transform.rows[0][component] * local[0] +
                      transform.rows[1][component] * local[1] +
                      transform.rows[2][component] * local[2] +
                      transform.rows[3][component] * local[3];
  }
}

inline void DirectDrawUpdateFloatBounds(bool& has_bounds, float value,
                                        float& min_value, float& max_value) {
  if (!has_bounds) {
    min_value = max_value = value;
    return;
  }
  if (value < min_value) {
    min_value = value;
  }
  if (value > max_value) {
    max_value = value;
  }
}

inline DirectDrawReplayClipPositionStats AnalyzeDirectDrawReplayClipPositionStats(
    const uint8_t* upload_bytes, uint32_t byte_count, uint32_t stride,
    uint32_t position_offset, const DirectDrawDebugReplayTransform& transform,
    DirectDrawReplayTransformInterpretation interpretation,
    uint32_t max_sample_vertices) {
  DirectDrawReplayClipPositionStats stats;
  stats.transform_valid = transform.valid;
  stats.stride = stride;
  stats.position_offset = position_offset;
  if (!transform.valid || !upload_bytes || stride == 0 ||
      position_offset > stride || stride - position_offset < 12u ||
      byte_count < position_offset + 12u) {
    return stats;
  }

  stats.vertex_count = byte_count / stride;
  if (stats.vertex_count == 0) {
    return stats;
  }

  stats.valid = true;
  stats.sampled_vertices = stats.vertex_count;
  if (max_sample_vertices != 0 &&
      stats.sampled_vertices > max_sample_vertices) {
    stats.sampled_vertices = max_sample_vertices;
  }

  for (uint32_t vertex_index = 0; vertex_index < stats.sampled_vertices;
       ++vertex_index) {
    const uint32_t vertex_offset = vertex_index * stride + position_offset;
    const float local[4] = {
        DirectDrawReplayFloatFromGuestBE32(
            DirectDrawLoadBE32(upload_bytes, vertex_offset + 0u)),
        DirectDrawReplayFloatFromGuestBE32(
            DirectDrawLoadBE32(upload_bytes, vertex_offset + 4u)),
        DirectDrawReplayFloatFromGuestBE32(
            DirectDrawLoadBE32(upload_bytes, vertex_offset + 8u)),
        1.0f,
    };
    if (!std::isfinite(local[0]) || !std::isfinite(local[1]) ||
        !std::isfinite(local[2])) {
      continue;
    }
    ++stats.finite_vertices;

    float clip[4] = {};
    DirectDrawReplayTransformPosition(transform, interpretation, local, clip);
    if (!std::isfinite(clip[0]) || !std::isfinite(clip[1]) ||
        !std::isfinite(clip[2]) || !std::isfinite(clip[3])) {
      continue;
    }
    ++stats.clip_finite_vertices;

    bool has_clip_bounds = stats.has_clip_bounds;
    DirectDrawUpdateFloatBounds(has_clip_bounds, clip[0], stats.min_clip_x,
                                stats.max_clip_x);
    DirectDrawUpdateFloatBounds(has_clip_bounds, clip[1], stats.min_clip_y,
                                stats.max_clip_y);
    DirectDrawUpdateFloatBounds(has_clip_bounds, clip[2], stats.min_clip_z,
                                stats.max_clip_z);
    DirectDrawUpdateFloatBounds(has_clip_bounds, clip[3], stats.min_w,
                                stats.max_w);
    stats.has_clip_bounds = true;

    if (std::fabs(clip[3]) <= 1.0e-6f) {
      continue;
    }

    const float ndc_x = clip[0] / clip[3];
    const float ndc_y = clip[1] / clip[3];
    const float ndc_z = clip[2] / clip[3];
    if (!std::isfinite(ndc_x) || !std::isfinite(ndc_y) ||
        !std::isfinite(ndc_z)) {
      continue;
    }
    ++stats.projectable_vertices;

    bool has_ndc_bounds = stats.has_ndc_bounds;
    DirectDrawUpdateFloatBounds(has_ndc_bounds, ndc_x, stats.min_ndc_x,
                                stats.max_ndc_x);
    DirectDrawUpdateFloatBounds(has_ndc_bounds, ndc_y, stats.min_ndc_y,
                                stats.max_ndc_y);
    DirectDrawUpdateFloatBounds(has_ndc_bounds, ndc_z, stats.min_ndc_z,
                                stats.max_ndc_z);
    stats.has_ndc_bounds = true;

    const bool xy_inside =
        std::fabs(ndc_x) <= 1.0f && std::fabs(ndc_y) <= 1.0f;
    if (xy_inside) {
      ++stats.xy_inside_vertices;
    }
    if (xy_inside && ndc_z >= 0.0f && ndc_z <= 1.0f) {
      ++stats.d3d_xyz_inside_vertices;
    }
    if (xy_inside && ndc_z >= -1.0f && ndc_z <= 1.0f) {
      ++stats.gl_xyz_inside_vertices;
    }
  }

  return stats;
}

inline float DirectDrawReplayNdcSpanX(
    const DirectDrawReplayClipPositionStats& stats) {
  return std::fabs(stats.max_ndc_x - stats.min_ndc_x);
}

inline float DirectDrawReplayNdcSpanY(
    const DirectDrawReplayClipPositionStats& stats) {
  return std::fabs(stats.max_ndc_y - stats.min_ndc_y);
}

inline bool DirectDrawReplayTransformStatsAreUsable(
    const DirectDrawReplayClipPositionStats& stats) {
  if (!stats.valid || !stats.transform_valid || !stats.has_ndc_bounds ||
      stats.sampled_vertices == 0 || stats.projectable_vertices == 0) {
    return false;
  }

  constexpr float kMinVisibleNdcSpan = 1.0e-5f;
  constexpr float kMaxReasonableNdcXY = 2.0f;
  if (stats.min_ndc_x < -kMaxReasonableNdcXY ||
      stats.max_ndc_x > kMaxReasonableNdcXY ||
      stats.min_ndc_y < -kMaxReasonableNdcXY ||
      stats.max_ndc_y > kMaxReasonableNdcXY) {
    return false;
  }

  return DirectDrawReplayNdcSpanX(stats) > kMinVisibleNdcSpan ||
         DirectDrawReplayNdcSpanY(stats) > kMinVisibleNdcSpan;
}

inline bool DirectDrawReplayTransformStatsAreBetter(
    const DirectDrawReplayClipPositionStats& candidate,
    const DirectDrawReplayClipPositionStats& current) {
  if (!DirectDrawReplayTransformStatsAreUsable(candidate)) {
    return false;
  }
  if (!DirectDrawReplayTransformStatsAreUsable(current)) {
    return true;
  }
  if (candidate.d3d_xyz_inside_vertices != current.d3d_xyz_inside_vertices) {
    return candidate.d3d_xyz_inside_vertices > current.d3d_xyz_inside_vertices;
  }
  if (candidate.xy_inside_vertices != current.xy_inside_vertices) {
    return candidate.xy_inside_vertices > current.xy_inside_vertices;
  }
  if (candidate.projectable_vertices != current.projectable_vertices) {
    return candidate.projectable_vertices > current.projectable_vertices;
  }
  if (candidate.clip_finite_vertices != current.clip_finite_vertices) {
    return candidate.clip_finite_vertices > current.clip_finite_vertices;
  }

  const float candidate_area =
      DirectDrawReplayNdcSpanX(candidate) * DirectDrawReplayNdcSpanY(candidate);
  const float current_area =
      DirectDrawReplayNdcSpanX(current) * DirectDrawReplayNdcSpanY(current);
  return candidate_area > current_area;
}

inline void ConsiderDirectDrawDebugReplayTransformCandidate(
    DirectDrawDebugReplayTransformSelection& best, std::string_view source_name,
    const DirectDrawDebugReplayTransform& transform,
    DirectDrawReplayTransformInterpretation interpretation,
    const uint8_t* upload_bytes, uint32_t byte_count, uint32_t stride,
    uint32_t max_sample_vertices) {
  const DirectDrawReplayClipPositionStats stats =
      AnalyzeDirectDrawReplayClipPositionStats(
          upload_bytes, byte_count, stride, 0u, transform, interpretation,
          max_sample_vertices);
  if (!DirectDrawReplayTransformStatsAreBetter(stats, best.stats)) {
    return;
  }

  best.valid = true;
  best.source_name = source_name;
  best.transform = transform;
  best.interpretation = interpretation;
  best.stats = stats;
}

inline DirectDrawDebugReplayTransformSelection
SelectDirectDrawDebugReplayTransformCandidate(
    const uint8_t* upload_bytes, uint32_t byte_count, uint32_t stride,
    const uint32_t* constant_registers, uint32_t constant_count,
    DirectDrawReplayTransformInterpretation interpretation,
    uint32_t max_sample_vertices) {
  DirectDrawDebugReplayTransformSelection best;
  if (!upload_bytes || byte_count == 0 || stride == 0 ||
      !constant_registers || constant_count == 0) {
    return best;
  }

  struct CandidateRange {
    std::string_view name;
    uint32_t first_constant;
  };
  constexpr CandidateRange kCandidateRanges[] = {
      {"c0", 0u},   {"c23", 23u}, {"c28", 28u}, {"c36", 36u},
      {"c60", 60u}, {"c68", 68u}, {"c76", 76u}, {"c252", 252u},
  };
  DirectDrawDebugReplayTransform transforms[sizeof(kCandidateRanges) /
                                            sizeof(kCandidateRanges[0])] = {};
  for (uint32_t i = 0;
       i < sizeof(kCandidateRanges) / sizeof(kCandidateRanges[0]); ++i) {
    transforms[i] = BuildDirectDrawDebugReplayTransformFromConstants(
        constant_registers, constant_count, kCandidateRanges[i].first_constant);
    ConsiderDirectDrawDebugReplayTransformCandidate(
        best, kCandidateRanges[i].name, transforms[i], interpretation,
        upload_bytes, byte_count, stride, max_sample_vertices);
  }

  if (interpretation == DirectDrawReplayTransformInterpretation::kRowMajorClip) {
    const DirectDrawDebugReplayTransform c0_c28 =
        MultiplyDirectDrawReplayRowMajorTransforms(transforms[0], transforms[2],
                                                   0u);
    const DirectDrawDebugReplayTransform c36_c28 =
        MultiplyDirectDrawReplayRowMajorTransforms(transforms[3], transforms[2],
                                                   36u);
    const DirectDrawDebugReplayTransform c36_c60 =
        MultiplyDirectDrawReplayRowMajorTransforms(transforms[3], transforms[4],
                                                   36u);
    const DirectDrawDebugReplayTransform c36_c76 =
        MultiplyDirectDrawReplayRowMajorTransforms(transforms[3], transforms[6],
                                                   36u);
    ConsiderDirectDrawDebugReplayTransformCandidate(
        best, "c0_mul_c28", c0_c28, interpretation, upload_bytes, byte_count,
        stride, max_sample_vertices);
    ConsiderDirectDrawDebugReplayTransformCandidate(
        best, "c36_mul_c28", c36_c28, interpretation, upload_bytes, byte_count,
        stride, max_sample_vertices);
    ConsiderDirectDrawDebugReplayTransformCandidate(
        best, "c36_mul_c60", c36_c60, interpretation, upload_bytes, byte_count,
        stride, max_sample_vertices);
    ConsiderDirectDrawDebugReplayTransformCandidate(
        best, "c36_mul_c76", c36_c76, interpretation, upload_bytes, byte_count,
        stride, max_sample_vertices);
  }

  return best;
}

constexpr uint32_t DirectDrawReplayNormalizeGuestBaseForFetch(uint32_t guest_base) {
  return DirectDrawReplayUploadGuestBase(guest_base) &
         kDirectDrawReplayFetchBaseMask;
}

constexpr DirectDrawReplayVertexFetchMatch MatchDirectDrawReplayStreamToVertexFetch(
    const DirectDrawReplayVertexStream& stream, uint32_t fetch_group,
    uint32_t fetch_group_slot, const D3DCommandContextVertexFetchSummary& fetch) {
  DirectDrawReplayVertexFetchMatch out;
  out.stream_slot = stream.slot;
  out.fetch_group = fetch_group;
  out.fetch_group_slot = fetch_group_slot;
  out.fetch_constant =
      D3DCommandContextVertexFetchConstantIndex(fetch_group, fetch_group_slot);
  out.stream_guest_base = stream.guest_base;
  out.stream_fetch_base = DirectDrawReplayNormalizeGuestBaseForFetch(stream.guest_base);
  out.stream_bytes = stream.upload_bytes;
  out.fetch_base = fetch.address_bytes;
  out.fetch_bytes = fetch.size_bytes;

  if (fetch_group >= kD3DCommandContextFetchGroupCount || fetch_group_slot >= 3u ||
      stream.guest_base == 0 || stream.upload_bytes == 0 ||
      !D3DCommandContextVertexFetchIsValid(fetch)) {
    return out;
  }

  out.valid = out.stream_fetch_base == out.fetch_base &&
              out.stream_bytes == out.fetch_bytes;
  return out;
}

constexpr DirectDrawReplayIndexBuffer BuildDirectDrawReplayIndexBuffer(
    const DirectDrawBufferViewSummary& view) {
  DirectDrawReplayIndexBuffer out;
  out.guest_base = view.gpu_base;
  out.upload_guest_base = view.upload_guest_base;
  out.upload_bytes = view.hash_bytes;
  out.hash = view.hash;
  out.upload_endian = DirectDrawReplayIndexUploadEndianFromElementBytes(view.element_bytes);
  return out;
}

constexpr DirectDrawReplayNativeStreamBinding
BuildDirectDrawReplayNativeStreamBinding(
    const NativeStateVertexStreamBinding& binding) {
  DirectDrawReplayNativeStreamBinding out;
  if (!binding.valid || binding.slot >= 2u) {
    return out;
  }

  out.valid = true;
  out.slot = binding.slot;
  out.resource = binding.resource;
  out.byte_offset = binding.byte_offset;
  out.stride_bytes = binding.stride_bytes;
  out.dirty_mask = binding.dirty_mask;
  return out;
}

constexpr DirectDrawReplayNativeViewportSummary
BuildDirectDrawReplayNativeViewportSummary(
    const NativeStateViewportBinding& binding) {
  DirectDrawReplayNativeViewportSummary out;
  if (!binding.valid) {
    return out;
  }

  out.valid = true;
  out.viewport_mode = binding.viewport_mode;
  return out;
}

constexpr DirectDrawReplayNativeTextureFetchSummary
BuildDirectDrawReplayNativeTextureFetchSummary(
    const NativeStateTextureFetchBinding& binding) {
  DirectDrawReplayNativeTextureFetchSummary out;
  if (!binding.valid) {
    return out;
  }

  out.valid = true;
  out.fetch_bits_low = binding.fetch_bits_low;
  out.fetch_bits_mid = binding.fetch_bits_mid;
  return out;
}

constexpr DirectDrawReplayNativeClearSummary
BuildDirectDrawReplayNativeClearSummary(
    const NativeStateClearBinding& binding) {
  DirectDrawReplayNativeClearSummary out;
  if (!binding.valid) {
    return out;
  }

  out.valid = true;
  out.clear_color_byte = binding.clear_color_byte;
  out.clear_flags = binding.clear_flags;
  return out;
}

constexpr DirectDrawReplayNativePassSummary
BuildDirectDrawReplayNativePassSummary(
    const NativeStatePassDrawBoundary& boundary) {
  DirectDrawReplayNativePassSummary out;
  if (!boundary.valid) {
    return out;
  }

  out.valid = true;
  out.submit_object = boundary.submit_object;
  out.tls_or_pass_context = boundary.tls_or_pass_context;
  out.pass_flags = boundary.pass_flags;
  out.drawable = boundary.drawable;
  out.draw_callback = boundary.draw_callback;
  out.wireframe = boundary.wireframe;
  out.draw_mode = boundary.draw_mode;
  out.pass_marker = boundary.pass_marker;
  return out;
}

constexpr DirectDrawReplayNativeStateSummary
BuildDirectDrawReplayNativeStateSummary(
    const NativeStateSnapshot& snapshot) {
  DirectDrawReplayNativeStateSummary out;
  if (!snapshot.valid) {
    return out;
  }

  out.valid = true;
  out.sequence = snapshot.sequence;
  out.render_context = snapshot.render_context;
  if (snapshot.last_direct_draw.valid) {
    out.direct_render_context =
        snapshot.last_direct_draw.direct_render_context;
    out.draw_iface = snapshot.last_direct_draw.draw_iface;
  }
  if (snapshot.vertex_shader.valid) {
    out.vertex_shader = snapshot.vertex_shader.shader;
  }
  if (snapshot.pixel_shader.valid) {
    out.pixel_shader = snapshot.pixel_shader.shader;
  }
  for (const NativeStateVertexStreamBinding& stream : snapshot.streams) {
    if (stream.valid && stream.slot < 2u) {
      out.streams[stream.slot] =
          BuildDirectDrawReplayNativeStreamBinding(stream);
    }
  }
  if (snapshot.index_buffer.valid) {
    out.index_resource = snapshot.index_buffer.resource;
  }
  if (snapshot.bound_surface.valid) {
    out.bound_surface = snapshot.bound_surface.surface;
    out.bound_surface_arg = snapshot.bound_surface.surface_arg;
  }
  out.viewport = BuildDirectDrawReplayNativeViewportSummary(snapshot.viewport);
  out.texture_fetch =
      BuildDirectDrawReplayNativeTextureFetchSummary(snapshot.texture_fetch);
  out.clear = BuildDirectDrawReplayNativeClearSummary(snapshot.clear);
  out.pass = BuildDirectDrawReplayNativePassSummary(snapshot.last_pass);
  return out;
}

constexpr DirectDrawDebugReplayPlan BuildDirectDrawDebugReplayPlan(
    const DirectDrawIndexedPacketSummary& packet) {
  DirectDrawDebugReplayPlan out;
  out.topology = packet.topology;
  out.index_format =
      DirectDrawReplayIndexFormatFromElementBytes(packet.index.element_bytes);
  out.stream_count = 2u;
  out.streams[0] = BuildDirectDrawReplayVertexStream(0u, packet.stream0);
  out.streams[1] = BuildDirectDrawReplayVertexStream(1u, packet.stream1);
  out.index = BuildDirectDrawReplayIndexBuffer(packet.index);
  out.draw.index_count = packet.index_count;
  out.draw.instance_count = 1u;
  out.draw.start_index = packet.first_index;
  out.draw.base_vertex = 0;
  out.draw.start_instance = 0u;
  out.vertex_payload_hash = packet.vertex_shader.payload_hash;
  out.pixel_payload_hash = packet.pixel_shader.payload_hash;
  out.vertex_structural_ucode_hash = packet.vertex_shader.structural_ucode_hash;
  out.pixel_structural_ucode_hash = packet.pixel_shader.structural_ucode_hash;
  out.has_vertex_structural_ucode = packet.HasVertexStructuralUcodeKey();
  out.has_pixel_structural_ucode = packet.HasPixelStructuralUcodeKey();
  out.ready = packet.CanAttemptDebugReplay() &&
              out.index_format != DirectDrawReplayIndexFormat::kUnknown &&
              out.streams[0].stride != 0 && out.streams[1].stride != 0 &&
              out.index.upload_bytes != 0;
  return out;
}

constexpr DirectDrawDebugReplayPlan BuildDirectDrawDebugReplayPlan(
    const DirectDrawIndexedPacketSummary& packet,
    const NativeStateSnapshot& snapshot) {
  DirectDrawDebugReplayPlan out = BuildDirectDrawDebugReplayPlan(packet);
  out.native_state = BuildDirectDrawReplayNativeStateSummary(snapshot);
  return out;
}

constexpr DirectDrawDebugReplayPlan BuildDirectDrawNativeLayoutReplayPlan(
    const DirectDrawDebugReplayPlan& base_plan,
    const DirectDrawBufferViewSummary& native_stream0,
    const DirectDrawBufferViewSummary& native_stream1,
    const DirectDrawBufferViewSummary& native_index) {
  DirectDrawDebugReplayPlan out = base_plan;
  if (DirectDrawReplayNativeLayoutFromState(out.native_state) !=
      DirectDrawReplayPipelineLayout::kNativePosition28Side12) {
    out.ready = false;
    return out;
  }

  out.stream_count = 2u;
  out.streams[0] =
      BuildDirectDrawReplayVertexStream(out.native_state.streams[0].slot,
                                        native_stream0);
  out.streams[1] =
      BuildDirectDrawReplayVertexStream(out.native_state.streams[1].slot,
                                        native_stream1);
  out.index = BuildDirectDrawReplayIndexBuffer(native_index);
  out.index_format =
      DirectDrawReplayIndexFormatFromElementBytes(native_index.element_bytes);
  out.ready = base_plan.ready && native_stream0.HasReplayableView() &&
              native_stream1.HasReplayableView() &&
              native_index.HasReplayableView() &&
              out.index_format != DirectDrawReplayIndexFormat::kUnknown &&
              DirectDrawReplayPipelineLayoutForPlan(out) ==
                  DirectDrawReplayPipelineLayout::kNativePosition28Side12;
  return out;
}

inline bool ConvertDirectDrawReplayUploadBytes(
    const uint8_t* src, uint32_t byte_count, DirectDrawReplayUploadEndian endian,
    uint8_t* dst) {
  if (!src || !dst) {
    return false;
  }

  switch (endian) {
    case DirectDrawReplayUploadEndian::kRaw:
      for (uint32_t i = 0; i < byte_count; ++i) {
        dst[i] = src[i];
      }
      return true;
    case DirectDrawReplayUploadEndian::kSwap16:
      if ((byte_count & 1u) != 0) {
        return false;
      }
      for (uint32_t i = 0; i < byte_count; i += 2u) {
        const uint8_t b0 = src[i + 0u];
        const uint8_t b1 = src[i + 1u];
        dst[i + 0u] = b1;
        dst[i + 1u] = b0;
      }
      return true;
    case DirectDrawReplayUploadEndian::kSwap32:
      if ((byte_count & 3u) != 0) {
        return false;
      }
      for (uint32_t i = 0; i < byte_count; i += 4u) {
        const uint8_t b0 = src[i + 0u];
        const uint8_t b1 = src[i + 1u];
        const uint8_t b2 = src[i + 2u];
        const uint8_t b3 = src[i + 3u];
        dst[i + 0u] = b3;
        dst[i + 1u] = b2;
        dst[i + 2u] = b1;
        dst[i + 3u] = b0;
      }
      return true;
  }

  return false;
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

inline DirectDrawShaderUcodeBounds AnalyzeXenosUcodeBounds(const uint32_t* ucode_dwords,
                                                           uint32_t ucode_dword_count) {
  DirectDrawShaderUcodeBounds bounds;
  bounds.scanned_dwords = ucode_dword_count;
  bounds.scanned_bytes = ucode_dword_count * sizeof(uint32_t);
  bounds.cf_pair_count_available =
      ucode_dword_count / kXenosUcodeControlFlowPairDwordCount;
  bounds.cf_pair_index_bound = bounds.cf_pair_count_available;
  if (!ucode_dwords || bounds.cf_pair_count_available == 0) {
    return bounds;
  }

  bool saw_exec = false;
  for (uint32_t i = 0; i < bounds.cf_pair_index_bound; ++i) {
    XenosControlFlowInstructionBits cf[2];
    UnpackXenosControlFlowPair(
        ucode_dwords + i * kXenosUcodeControlFlowPairDwordCount, cf);
    for (uint32_t j = 0; j < 2; ++j) {
      const uint32_t opcode = cf[j].opcode();
      if (!IsXenosControlFlowExecOpcode(opcode)) {
        continue;
      }
      const uint32_t exec_address = cf[j].exec_address();
      if (!saw_exec) {
        saw_exec = true;
        bounds.first_exec_cf_index = i * 2u + j;
        bounds.first_exec_opcode = opcode;
        bounds.first_exec_address = exec_address;
      }
      if (exec_address < bounds.cf_pair_index_bound) {
        bounds.cf_pair_index_bound = exec_address;
      }
    }
  }

  if (!saw_exec) {
    bounds.cf_pair_index_bound = 0;
    return bounds;
  }
  if (bounds.first_exec_address >= bounds.cf_pair_count_available) {
    bounds.cf_pair_index_bound = 0;
    bounds.truncated = true;
    return bounds;
  }

  bounds.valid = true;
  bounds.cf_byte_count = bounds.cf_pair_index_bound * kXenosUcodeInstructionByteStride;

  for (uint32_t i = 0; i < bounds.cf_pair_index_bound; ++i) {
    XenosControlFlowInstructionBits cf[2];
    UnpackXenosControlFlowPair(
        ucode_dwords + i * kXenosUcodeControlFlowPairDwordCount, cf);
    for (uint32_t j = 0; j < 2; ++j) {
      const uint32_t opcode = cf[j].opcode();
      if (!IsXenosControlFlowExecOpcode(opcode)) {
        continue;
      }
      ++bounds.exec_instruction_count;
      const uint32_t instruction_end = cf[j].exec_address() + cf[j].exec_count();
      if (instruction_end > bounds.exec_high_water_instruction) {
        bounds.exec_high_water_instruction = instruction_end;
      }
      if (IsXenosControlFlowEndShaderOpcode(opcode)) {
        bounds.saw_exec_end = true;
      }
    }
  }

  bounds.exec_high_water_bytes =
      bounds.exec_high_water_instruction * kXenosUcodeInstructionByteStride;
  bounds.total_used_bytes =
      bounds.cf_byte_count > bounds.exec_high_water_bytes ? bounds.cf_byte_count
                                                          : bounds.exec_high_water_bytes;
  bounds.truncated = bounds.total_used_bytes > bounds.scanned_bytes;
  return bounds;
}

constexpr bool IsCompleteXenosUcodeCandidate(const DirectDrawShaderUcodeBounds& bounds) {
  return bounds.valid && bounds.saw_exec_end && !bounds.truncated && bounds.total_used_bytes != 0;
}

constexpr bool IsZeroXenosControlFlowPair(const uint32_t* dwords) {
  return dwords[0] == 0 && dwords[1] == 0 && dwords[2] == 0;
}

constexpr bool IsBetterXenosUcodeCandidate(
    const DirectDrawShaderUcodeCandidate& candidate,
    const DirectDrawShaderUcodeCandidate& current_best) {
  if (!candidate.valid) {
    return false;
  }
  if (!current_best.valid) {
    return true;
  }
  if (candidate.bounds.total_used_bytes != current_best.bounds.total_used_bytes) {
    return candidate.bounds.total_used_bytes > current_best.bounds.total_used_bytes;
  }
  return candidate.dword_offset < current_best.dword_offset;
}

inline DirectDrawShaderUcodeCandidate FindXenosUcodeCandidate(
    const uint32_t* payload_dwords, uint32_t payload_dword_count) {
  DirectDrawShaderUcodeCandidate best_candidate;
  if (!payload_dwords ||
      payload_dword_count < kXenosUcodeControlFlowPairDwordCount) {
    return best_candidate;
  }

  const uint32_t last_dword_offset =
      payload_dword_count - kXenosUcodeControlFlowPairDwordCount;
  for (uint32_t dword_offset = 0; dword_offset <= last_dword_offset; ++dword_offset) {
    if (IsZeroXenosControlFlowPair(payload_dwords + dword_offset)) {
      continue;
    }
    const auto bounds = AnalyzeXenosUcodeBounds(
        payload_dwords + dword_offset, payload_dword_count - dword_offset);
    if (!IsCompleteXenosUcodeCandidate(bounds)) {
      continue;
    }
    DirectDrawShaderUcodeCandidate candidate;
    candidate.valid = true;
    candidate.dword_offset = dword_offset;
    candidate.byte_offset = dword_offset * sizeof(uint32_t);
    candidate.bounds = bounds;
    if (IsBetterXenosUcodeCandidate(candidate, best_candidate)) {
      best_candidate = candidate;
    }
  }

  return best_candidate;
}

inline uint32_t FindTopXenosUcodeCandidates(
    const uint32_t* payload_dwords, uint32_t payload_dword_count,
    DirectDrawShaderUcodeCandidate* out_candidates, uint32_t out_candidate_capacity) {
  if (!payload_dwords || !out_candidates || out_candidate_capacity == 0 ||
      payload_dword_count < kXenosUcodeControlFlowPairDwordCount) {
    return 0;
  }

  uint32_t candidate_count = 0;
  const uint32_t last_dword_offset =
      payload_dword_count - kXenosUcodeControlFlowPairDwordCount;
  for (uint32_t dword_offset = 0; dword_offset <= last_dword_offset; ++dword_offset) {
    if (IsZeroXenosControlFlowPair(payload_dwords + dword_offset)) {
      continue;
    }
    const auto bounds = AnalyzeXenosUcodeBounds(
        payload_dwords + dword_offset, payload_dword_count - dword_offset);
    if (!IsCompleteXenosUcodeCandidate(bounds)) {
      continue;
    }

    DirectDrawShaderUcodeCandidate candidate;
    candidate.valid = true;
    candidate.dword_offset = dword_offset;
    candidate.byte_offset = dword_offset * sizeof(uint32_t);
    candidate.bounds = bounds;

    if (candidate_count < out_candidate_capacity) {
      out_candidates[candidate_count++] = candidate;
    } else if (IsBetterXenosUcodeCandidate(candidate,
                                          out_candidates[candidate_count - 1u])) {
      out_candidates[candidate_count - 1u] = candidate;
    } else {
      continue;
    }

    for (uint32_t i = candidate_count - 1u; i > 0; --i) {
      if (!IsBetterXenosUcodeCandidate(out_candidates[i], out_candidates[i - 1u])) {
        break;
      }
      const DirectDrawShaderUcodeCandidate tmp = out_candidates[i - 1u];
      out_candidates[i - 1u] = out_candidates[i];
      out_candidates[i] = tmp;
    }
  }

  return candidate_count;
}

}  // namespace fm2::native_renderer
