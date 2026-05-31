/**
 ******************************************************************************
 * ReXGlue FM2 Native Codec Helpers                                           *
 ******************************************************************************
 */

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>

namespace rex::audio::fm2_native {

enum class DecodeBackend : uint8_t {
  kProbe = 0,
  kPassthrough = 1,
  kNative = 2,
};

struct ProbeState {
  uint32_t blocks_remaining = 0;
  uint64_t compared_bytes = 0;
  uint64_t matched_bytes = 0;
  double squared_error_sum = 0.0;
  bool started = false;
  std::chrono::steady_clock::time_point start_time{};
  uint64_t passthrough_bytes = 0;
};

struct BackendDecision {
  DecodeBackend backend = DecodeBackend::kProbe;
  bool decided = false;
  double match_ratio = 0.0;
  double rms_error = 0.0;
  double passthrough_bytes_per_sec = 0.0;
};

BackendDecision UpdateProbeAndMaybeDecide(ProbeState& probe, const uint8_t* passthrough,
                                          const uint8_t* native_candidate, uint32_t byte_count,
                                          uint32_t probe_block_count,
                                          uint32_t target_bytes_per_second);

void BuildNativeOutput(std::deque<uint8_t>& fifo, std::array<uint8_t, 4>& hold_sample,
                       const uint8_t* source_block, uint32_t source_size,
                       uint32_t output_size, uint32_t prefill_limit_bytes,
                       uint8_t* out_block, uint64_t& underrun_bytes);

const char* BackendName(DecodeBackend backend);

}  // namespace rex::audio::fm2_native

