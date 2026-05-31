/**
 ******************************************************************************
 * ReXGlue FM2 Native Codec Helpers                                           *
 ******************************************************************************
 */

#include <algorithm>
#include <cmath>

#include <rex/audio/fm2_native/codec.h>

namespace rex::audio::fm2_native {

namespace {

double ComputeRmsError(double squared_error_sum, uint64_t samples) {
  if (samples == 0) {
    return 0.0;
  }
  const double mean_square = squared_error_sum / static_cast<double>(samples);
  return std::sqrt(std::max(0.0, mean_square)) / 255.0;
}

}  // namespace

BackendDecision UpdateProbeAndMaybeDecide(ProbeState& probe, const uint8_t* passthrough,
                                          const uint8_t* native_candidate, uint32_t byte_count,
                                          uint32_t probe_block_count,
                                          uint32_t target_bytes_per_second) {
  BackendDecision decision;

  if (!probe.started) {
    probe.started = true;
    probe.start_time = std::chrono::steady_clock::now();
    probe.blocks_remaining = std::max<uint32_t>(probe_block_count, 1);
  }

  if (probe.blocks_remaining == 0 || byte_count == 0) {
    return decision;
  }

  for (uint32_t i = 0; i < byte_count; ++i) {
    const int diff = static_cast<int>(passthrough[i]) - static_cast<int>(native_candidate[i]);
    probe.compared_bytes++;
    if (diff == 0) {
      probe.matched_bytes++;
    }
    probe.squared_error_sum += static_cast<double>(diff * diff);
  }
  probe.passthrough_bytes += byte_count;

  if (probe.blocks_remaining > 0) {
    probe.blocks_remaining--;
  }

  if (probe.blocks_remaining > 0) {
    return decision;
  }

  const auto now = std::chrono::steady_clock::now();
  const double elapsed =
      std::chrono::duration_cast<std::chrono::duration<double>>(now - probe.start_time).count();

  decision.match_ratio = probe.compared_bytes
                             ? static_cast<double>(probe.matched_bytes) /
                                   static_cast<double>(probe.compared_bytes)
                             : 1.0;
  decision.rms_error = ComputeRmsError(probe.squared_error_sum, probe.compared_bytes);
  decision.passthrough_bytes_per_sec =
      elapsed > 0.0 ? static_cast<double>(probe.passthrough_bytes) / elapsed : 0.0;
  decision.decided = true;

  // Decision policy:
  // 1) If passthrough bandwidth itself is below target, prefer native scheduler path.
  // 2) Otherwise keep passthrough when outputs are already near-identical.
  const double throughput_floor = static_cast<double>(target_bytes_per_second) * 0.95;
  if (decision.passthrough_bytes_per_sec < throughput_floor) {
    decision.backend = DecodeBackend::kNative;
  } else if (decision.match_ratio >= 0.98 || decision.rms_error <= 1.0e-4) {
    decision.backend = DecodeBackend::kPassthrough;
  } else {
    decision.backend = DecodeBackend::kNative;
  }

  return decision;
}

void BuildNativeOutput(std::deque<uint8_t>& fifo, std::array<uint8_t, 4>& hold_sample,
                       const uint8_t* source_block, uint32_t source_size,
                       uint32_t output_size, uint32_t prefill_limit_bytes,
                       uint8_t* out_block, uint64_t& underrun_bytes) {
  for (uint32_t i = 0; i < source_size; ++i) {
    fifo.push_back(source_block[i]);
  }

  while (fifo.size() > prefill_limit_bytes) {
    fifo.pop_front();
  }

  for (uint32_t i = 0; i < output_size; ++i) {
    uint8_t value = hold_sample[i & 3];
    if (!fifo.empty()) {
      value = fifo.front();
      fifo.pop_front();
      hold_sample[i & 3] = value;
    } else {
      underrun_bytes++;
    }
    out_block[i] = value;
  }
}

const char* BackendName(DecodeBackend backend) {
  switch (backend) {
    case DecodeBackend::kProbe:
      return "probe";
    case DecodeBackend::kPassthrough:
      return "passthrough";
    case DecodeBackend::kNative:
      return "native";
    default:
      return "unknown";
  }
}

}  // namespace rex::audio::fm2_native

