/**
 ******************************************************************************
 * ReXGlue FM2 Native Diagnostics                                             *
 ******************************************************************************
 */

#include <rex/audio/fm2_native/diag.h>

#include <chrono>

#include <rex/audio/fm2_native/runtime.h>
#include <rex/logging.h>

namespace rex::audio::fm2_native {

void LogBackendDecision(uint32_t codec_instance_ptr, const BackendDecision& decision) {
  REXAPU_ERROR(
      "FM2_NATIVE_BACKEND codec={:08X} backend={} parity={:.4f} rms={:.6f} "
      "passthrough_bps={:.0f}",
      codec_instance_ptr, BackendName(decision.backend), decision.match_ratio,
      decision.rms_error, decision.passthrough_bytes_per_sec);
}

void LogPerSecond(const RuntimeMetrics& metrics) {
  static auto last_log = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  const double elapsed =
      std::chrono::duration_cast<std::chrono::duration<double>>(now - last_log).count();
  if (elapsed < 1.0) {
    return;
  }
  last_log = now;

  REXAPU_ERROR(
      "FM2_NATIVE_PERSEC reads={} req_bytes={} out_bytes={} infl_req={} "
      "underrun_bytes={} cw_calls={} cw_extra={} cw_starve={} "
      "replay_attempts={} replay_ret_nz={} replay_zero={} replay_oor={} "
      "replay_ok_bytes={} active={} "
      "backend_decided={} native={} out_fps={:.1f}",
      metrics.codec_read_count, metrics.requested_bytes, metrics.produced_bytes,
      metrics.inflated_request_bytes, metrics.native_underrun_bytes,
      metrics.copy_window_calls, metrics.copy_window_extra_passes,
      metrics.copy_window_starve_bytes, metrics.replay_attempts,
      metrics.replay_ret_nonzero, metrics.replay_next_bytes_zero,
      metrics.replay_next_bytes_oor, metrics.replay_success_bytes,
      metrics.active_codec_instances,
      metrics.backend_decided ? 1 : 0, metrics.native_backend_active ? 1 : 0,
      metrics.output_frames_per_second);
}

}  // namespace rex::audio::fm2_native
