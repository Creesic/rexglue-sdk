/**
 ******************************************************************************
 * ReXGlue FM2 Native Runtime                                                 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>

namespace rex::audio::fm2_native {

// Guest-side codec decode invoked from FM2 hooks (sub_82692CC8).
using GuestDecodeFn = uint32_t (*)(void* ctx, uint32_t codec_instance_ptr,
                                   uint32_t output_offset_ptr, uint32_t byte_count_ptr,
                                   uint32_t stream_ptr);

struct GuestMemoryOps {
  uint8_t* guest_base = nullptr;
  uint32_t (*load_u32)(uint8_t* base, uint32_t guest_addr) = nullptr;
  void (*store_u32)(uint8_t* base, uint32_t guest_addr, uint32_t value) = nullptr;
  bool (*readable_range)(uint8_t* base, uint32_t guest_addr, uint32_t byte_count) = nullptr;
};

struct CopyWindowResult {
  uint32_t ring_cursor_out = 0;
  bool suppress_guest_memcpy = false;
};

struct RuntimeMetrics {
  uint64_t codec_open_count = 0;
  uint64_t codec_close_count = 0;
  uint64_t codec_read_count = 0;
  uint64_t requested_bytes = 0;
  uint64_t produced_bytes = 0;
  uint64_t inflated_request_bytes = 0;
  uint64_t native_underrun_bytes = 0;
  uint64_t copy_window_calls = 0;
  uint64_t copy_window_extra_passes = 0;
  uint64_t copy_window_starve_bytes = 0;
  uint64_t replay_attempts = 0;
  uint64_t replay_ret_nonzero = 0;
  uint64_t replay_next_bytes_zero = 0;
  uint64_t replay_next_bytes_oor = 0;
  uint64_t replay_success_bytes = 0;
  uint32_t active_codec_instances = 0;
  bool backend_decided = false;
  bool native_backend_active = false;
  double output_frames_per_second = 0.0;
};

void RegisterCodecInstance(uint32_t codec_instance_ptr, uint32_t flags);
void ReleaseCodecInstance(uint32_t codec_instance_ptr);
void MarkCodecRead(uint32_t codec_instance_ptr, uint32_t request_ptr, uint32_t requested_bytes);

uint32_t PlanReadBytes(uint32_t codec_instance_ptr, uint32_t requested_bytes,
                       uint32_t source_ring_offset, uint32_t source_ring_capacity);

bool ReadCodecData(uint32_t codec_instance_ptr, uint32_t source_ring_ptr,
                   uint32_t source_ring_offset, uint32_t source_ring_capacity,
                   uint32_t bytes_to_write);

// Inflates *request_ptr when scheduler credit allows (FM2 codec read entry).
void InflateCodecReadRequest(uint32_t codec_instance_ptr, uint32_t request_ptr,
                             uint32_t ring_offset, uint32_t ring_capacity);

// Host-timed copy pipeline: prefill via guest decode, copy to dest, probe/metrics.
CopyWindowResult ProcessReadCopyWindow(const GuestMemoryOps& mem, GuestDecodeFn decode_fn,
                                       void* decode_ctx, uint32_t codec_instance_ptr,
                                       uint32_t dest_ptr, uint32_t byte_count_ptr,
                                       uint32_t ring_ptr, uint32_t ring_cursor,
                                       uint32_t target_bytes, uint32_t stack_output_offset_ptr);

RuntimeMetrics GetRuntimeMetrics();

}  // namespace rex::audio::fm2_native
