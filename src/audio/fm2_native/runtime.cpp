/**
 ******************************************************************************
 * ReXGlue FM2 Native Runtime                                                 *
 ******************************************************************************
 */

#include <rex/audio/fm2_native/runtime.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <rex/audio/fm2_native/codec.h>
#include <rex/audio/fm2_native/diag.h>
#include <rex/audio/fm2_native/scheduler.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>

REXCVAR_DEFINE_BOOL(fm2_native_audio_enabled, true, "Audio",
                    "Enable FM2-specific native FMOD codec replacement hooks.");
REXCVAR_DEFINE_INT32(
    fm2_native_force_backend, 2, "Audio",
    "FM2 native backend override: 0=auto probe, 1=passthrough, 2=native scheduler.");
REXCVAR_DEFINE_INT32(fm2_native_probe_blocks, 200, "Audio",
                     "Number of codec-read blocks used before backend selection.");
REXCVAR_DEFINE_INT32(fm2_native_target_sample_rate, 48000, "Audio",
                     "FM2 native scheduler target sample rate in Hz.");
REXCVAR_DEFINE_INT32(fm2_native_target_channels, 1, "Audio",
                     "FM2 native scheduler target channel count (mono=1, stereo=2).");
REXCVAR_DEFINE_INT32(fm2_native_prefill_limit, 8192, "Audio",
                     "FM2 native scheduler FIFO cap in bytes.");
REXCVAR_DEFINE_INT32(fm2_native_ring_capacity, 2048, "Audio",
                     "FM2 codec PCM ring capacity in bytes.");
REXCVAR_DEFINE_INT32(fm2_native_copy_prefill_target, 4096, "Audio",
                     "Target host FIFO fill before copy-window output.");
REXCVAR_DEFINE_INT32(fm2_native_max_extra_passes, 8, "Audio",
                     "Maximum extra guest decode passes per copy window.");
REXCVAR_DEFINE_INT32(fm2_native_copy_window_boost, 2, "Audio",
                     "Multiplier applied to codec copy-window target bytes.");
REXCVAR_DEFINE_INT32(fm2_native_copy_window_max_bytes, 4096, "Audio",
                     "Hard cap for copy-window output bytes per codec read.");
REXCVAR_DEFINE_INT32(fm2_native_samples_per_quantum, 256, "Audio",
                     "Host scheduler quantum size in PCM samples.");

namespace rex::audio::fm2_native {

namespace {

constexpr uint32_t kCodecStreamFieldOffset = 212;

bool IsHardNativeMode() { return REXCVAR_GET(fm2_native_force_backend) == 2; }

uint32_t BytesPerQuantum(uint32_t channels) {
  const uint32_t samples =
      static_cast<uint32_t>(std::max(REXCVAR_GET(fm2_native_samples_per_quantum), 1));
  return samples * channels * static_cast<uint32_t>(sizeof(int16_t));
}

uint32_t AlignUpToQuantum(uint32_t bytes, uint32_t channels) {
  const uint32_t quantum = std::max(BytesPerQuantum(channels), 1u);
  if (bytes == 0) {
    return 0;
  }
  return ((bytes + quantum - 1) / quantum) * quantum;
}

struct StreamState {
  uint32_t flags = 0;
  DecodeBackend backend = DecodeBackend::kProbe;
  bool backend_decided = false;
  bool backend_logged = false;
  uint32_t consecutive_silent_native_blocks = 0;
  uint64_t read_calls = 0;
  ProbeState probe{};
  HostTimedScheduler scheduler{};
  std::deque<uint8_t> native_fifo{};
  std::deque<uint8_t> output_fifo{};
  std::array<uint8_t, 4> hold_sample{};
  uint32_t last_planned_bytes = 0;
};

class RuntimeState {
 public:
  void RegisterCodecInstance(uint32_t codec_instance_ptr, uint32_t flags) {
    std::lock_guard<std::mutex> lock(lock_);
    StreamState& stream = streams_[codec_instance_ptr];
    stream.flags = flags;
    if (!stream.backend_decided) {
      stream.backend = DecodeBackend::kProbe;
    }
    const uint32_t channels =
        static_cast<uint32_t>(std::clamp(REXCVAR_GET(fm2_native_target_channels), 1, 2));
    stream.scheduler.Reset(static_cast<double>(AlignUpToQuantum(2048, channels)));
    stream.output_fifo.clear();
    metrics_.codec_open_count++;
    metrics_.active_codec_instances = static_cast<uint32_t>(streams_.size());
    REXAPU_ERROR("FM2_NATIVE_OPEN codec={:08X} flags={:08X} active={}", codec_instance_ptr,
                 flags, metrics_.active_codec_instances);
  }

  void ReleaseCodecInstance(uint32_t codec_instance_ptr) {
    std::lock_guard<std::mutex> lock(lock_);
    const bool had_stream = streams_.find(codec_instance_ptr) != streams_.end();
    streams_.erase(codec_instance_ptr);
    metrics_.codec_close_count++;
    metrics_.active_codec_instances = static_cast<uint32_t>(streams_.size());
    REXAPU_ERROR("FM2_NATIVE_CLOSE codec={:08X} had_stream={} active={}", codec_instance_ptr,
                 had_stream ? 1 : 0, metrics_.active_codec_instances);
  }

  void MarkCodecRead(uint32_t codec_instance_ptr, uint32_t requested_bytes) {
    std::lock_guard<std::mutex> lock(lock_);
    auto [it, inserted] = streams_.try_emplace(codec_instance_ptr, StreamState{});
    StreamState& stream = it->second;
    stream.read_calls++;
    metrics_.codec_read_count++;
    metrics_.requested_bytes += requested_bytes;
    metrics_.active_codec_instances = static_cast<uint32_t>(streams_.size());
    if (inserted || stream.read_calls == 1 || (stream.read_calls % 512) == 0) {
      REXAPU_ERROR("FM2_NATIVE_READ codec={:08X} reads={} req={} inserted={} active={}",
                   codec_instance_ptr, stream.read_calls, requested_bytes, inserted ? 1 : 0,
                   metrics_.active_codec_instances);
    }
  }

  uint32_t PlanReadBytes(uint32_t codec_instance_ptr, uint32_t requested_bytes,
                         uint32_t source_ring_offset, uint32_t source_ring_capacity) {
    std::lock_guard<std::mutex> lock(lock_);
    if (!REXCVAR_GET(fm2_native_audio_enabled)) {
      return requested_bytes;
    }

    auto it = streams_.find(codec_instance_ptr);
    if (it == streams_.end()) {
      it = streams_.emplace(codec_instance_ptr, StreamState{}).first;
    }
    return PlanReadBytesUnlocked(it->second, requested_bytes, source_ring_offset,
                                 source_ring_capacity);
  }

  uint32_t PlanReadBytesUnlocked(StreamState& stream, uint32_t requested_bytes,
                                 uint32_t source_ring_offset, uint32_t source_ring_capacity) {
    const uint32_t max_capacity = std::max(source_ring_capacity, 1u);
    uint32_t max_contiguous = max_capacity;
    if (source_ring_offset < max_capacity) {
      max_contiguous = max_capacity - source_ring_offset;
      if (max_contiguous == 0) {
        max_contiguous = max_capacity;
      }
    }

    const uint32_t sample_rate =
        static_cast<uint32_t>(std::max(REXCVAR_GET(fm2_native_target_sample_rate), 1));
    const uint32_t channels =
        static_cast<uint32_t>(std::clamp(REXCVAR_GET(fm2_native_target_channels), 1, 2));
    const uint32_t bytes_per_frame = channels * sizeof(int16_t);
    const uint32_t target_bytes_per_second = sample_rate * bytes_per_frame;
    const uint32_t prefill_limit =
        static_cast<uint32_t>(std::max(REXCVAR_GET(fm2_native_prefill_limit), 1024));

    stream.scheduler.Advance(target_bytes_per_second, prefill_limit * 2);
    uint32_t desired_bytes = requested_bytes;

    if (stream.backend != DecodeBackend::kPassthrough) {
      const uint32_t budget = stream.scheduler.CurrentBudget(std::min(max_capacity, max_contiguous));
      if (budget > desired_bytes) {
        desired_bytes = budget;
      }
    }

    const uint32_t aligned_desired = AlignUpToQuantum(desired_bytes, channels);
    desired_bytes =
        std::clamp(aligned_desired, requested_bytes, std::min(max_capacity, max_contiguous));
    stream.scheduler.Consume(desired_bytes);
    stream.last_planned_bytes = desired_bytes;

    if (desired_bytes > requested_bytes) {
      metrics_.inflated_request_bytes += desired_bytes - requested_bytes;
    }

    return desired_bytes;
  }

  bool ReadCodecData(uint32_t codec_instance_ptr, uint32_t source_ring_ptr,
                     uint32_t source_ring_offset, uint32_t source_ring_capacity,
                     uint32_t bytes_to_write) {
    return ReadCodecData(codec_instance_ptr, source_ring_ptr, source_ring_offset,
                         source_ring_capacity, bytes_to_write, true);
  }

  bool ReadCodecData(uint32_t codec_instance_ptr, uint32_t source_ring_ptr,
                     uint32_t source_ring_offset, uint32_t source_ring_capacity,
                     uint32_t bytes_to_write, bool count_produced_bytes) {
    if (!REXCVAR_GET(fm2_native_audio_enabled)) {
      return false;
    }
    if (!source_ring_ptr || !source_ring_capacity || !bytes_to_write) {
      return false;
    }

    auto* kernel_state = rex::system::kernel_state();
    if (!kernel_state || !kernel_state->memory()) {
      return false;
    }

    auto* source_ring_base = kernel_state->memory()->TranslateVirtual<uint8_t*>(source_ring_ptr);
    if (!source_ring_base) {
      return false;
    }

    std::lock_guard<std::mutex> lock(lock_);
    auto it = streams_.find(codec_instance_ptr);
    if (it == streams_.end()) {
      it = streams_.emplace(codec_instance_ptr, StreamState{}).first;
    }
    return ReadCodecDataUnlocked(it->second, codec_instance_ptr, source_ring_base,
                                 source_ring_offset, source_ring_capacity, bytes_to_write,
                                 count_produced_bytes);
  }

  bool ReadCodecDataUnlocked(StreamState& stream, uint32_t codec_instance_ptr,
                             uint8_t* source_ring_base, uint32_t source_ring_offset,
                             uint32_t source_ring_capacity, uint32_t bytes_to_write,
                             bool count_produced_bytes) {
    ApplyForceBackendIfAny(stream, codec_instance_ptr);

    const uint32_t target_capacity = std::max(source_ring_capacity, 1u);
    const uint32_t write_bytes = std::min(bytes_to_write, target_capacity);

    std::vector<uint8_t> passthrough_block(write_bytes);
    for (uint32_t i = 0; i < write_bytes; ++i) {
      passthrough_block[i] = source_ring_base[(source_ring_offset + i) % target_capacity];
    }

    std::vector<uint8_t> native_block(write_bytes);
    uint64_t native_underrun_bytes = 0;
    const uint32_t prefill_limit =
        static_cast<uint32_t>(std::max(REXCVAR_GET(fm2_native_prefill_limit), 1024));
    BuildNativeOutput(stream.native_fifo, stream.hold_sample, passthrough_block.data(),
                      write_bytes, write_bytes, prefill_limit, native_block.data(),
                      native_underrun_bytes);

    if (!stream.backend_decided) {
      const uint32_t sample_rate =
          static_cast<uint32_t>(std::max(REXCVAR_GET(fm2_native_target_sample_rate), 1));
      const uint32_t channels =
          static_cast<uint32_t>(std::clamp(REXCVAR_GET(fm2_native_target_channels), 1, 2));
      const uint32_t bytes_per_frame = channels * sizeof(int16_t);
      const uint32_t target_bytes_per_second = sample_rate * bytes_per_frame;
      const uint32_t probe_blocks = static_cast<uint32_t>(std::max(REXCVAR_GET(fm2_native_probe_blocks), 1));

      BackendDecision decision = UpdateProbeAndMaybeDecide(
          stream.probe, passthrough_block.data(), native_block.data(), write_bytes, probe_blocks,
          target_bytes_per_second);
      if (decision.decided) {
        stream.backend = decision.backend;
        stream.backend_decided = true;
        metrics_.backend_decided = true;
        metrics_.native_backend_active = stream.backend == DecodeBackend::kNative;
        LogBackendDecision(codec_instance_ptr, decision);
      }
    }

    const bool native_selected =
        IsHardNativeMode() || stream.backend == DecodeBackend::kNative;
    const std::vector<uint8_t>& output_block =
        native_selected ? native_block : passthrough_block;

    if (native_selected) {
      uint64_t native_energy = 0;
      uint64_t passthrough_energy = 0;
      for (uint32_t i = 0; i < write_bytes; ++i) {
        const int n = static_cast<int>(native_block[i]) - 128;
        const int p = static_cast<int>(passthrough_block[i]) - 128;
        native_energy += static_cast<uint64_t>(n < 0 ? -n : n);
        passthrough_energy += static_cast<uint64_t>(p < 0 ? -p : p);
      }

      // Safety rail: if native output collapses to near-silence while passthrough has signal,
      // force this stream back to passthrough to avoid persistent mute output.
      if (native_energy < (write_bytes / 16) && passthrough_energy > (write_bytes * 2)) {
        stream.consecutive_silent_native_blocks++;
      } else {
        stream.consecutive_silent_native_blocks = 0;
      }

      if (!IsHardNativeMode() && stream.consecutive_silent_native_blocks >= 8) {
        stream.backend = DecodeBackend::kPassthrough;
        stream.backend_decided = true;
        stream.consecutive_silent_native_blocks = 0;
        REXAPU_ERROR(
            "FM2_NATIVE_BACKEND_FALLBACK codec={:08X} reason=native_silent passthrough_energy={} write_bytes={}",
            codec_instance_ptr, passthrough_energy, write_bytes);
      }
    }

    for (uint32_t i = 0; i < write_bytes; ++i) {
      source_ring_base[(source_ring_offset + i) % target_capacity] = output_block[i];
    }

    if (count_produced_bytes) {
      metrics_.produced_bytes += write_bytes;
      UpdateOutputRateLocked();
      LogPerSecond(metrics_);
    }
    if (native_selected) {
      metrics_.native_underrun_bytes += native_underrun_bytes;
    }
    metrics_.native_backend_active = native_selected;
    metrics_.backend_decided = stream.backend_decided || metrics_.backend_decided;
    metrics_.active_codec_instances = static_cast<uint32_t>(streams_.size());
    return true;
  }

  RuntimeMetrics GetRuntimeMetrics() {
    std::lock_guard<std::mutex> lock(lock_);
    metrics_.active_codec_instances = static_cast<uint32_t>(streams_.size());
    return metrics_;
  }

  void InflateCodecReadRequest(uint32_t codec_instance_ptr, uint32_t request_ptr,
                               uint32_t ring_offset, uint32_t ring_capacity) {
    if (!request_ptr) {
      return;
    }

    auto* kernel_state = rex::system::kernel_state();
    if (!kernel_state || !kernel_state->memory()) {
      return;
    }

    auto* request_words =
        kernel_state->memory()->TranslateVirtual<uint32_t*>(request_ptr);
    if (!request_words) {
      return;
    }

    const uint32_t requested_bytes = request_words[0];
    const uint32_t planned =
        PlanReadBytes(codec_instance_ptr, requested_bytes, ring_offset, ring_capacity);
    if (planned > requested_bytes) {
      request_words[0] = planned;
    }
    MarkCodecRead(codec_instance_ptr, std::max(planned, requested_bytes));
  }

  CopyWindowResult ProcessReadCopyWindow(const GuestMemoryOps& mem, GuestDecodeFn decode_fn,
                                         void* decode_ctx, uint32_t codec_instance_ptr,
                                         uint32_t dest_ptr, uint32_t byte_count_ptr,
                                         uint32_t ring_ptr, uint32_t ring_cursor,
                                         uint32_t target_bytes,
                                         uint32_t stack_output_offset_ptr) {
    CopyWindowResult result;
    result.ring_cursor_out = ring_cursor;

    if (!REXCVAR_GET(fm2_native_audio_enabled) || !mem.guest_base || !mem.load_u32 ||
        !mem.store_u32 || target_bytes == 0 || !dest_ptr) {
      return result;
    }

    const uint32_t ring_capacity = static_cast<uint32_t>(
        std::max(REXCVAR_GET(fm2_native_ring_capacity), 256));
    const uint32_t ring_offset =
        (ring_capacity > 0) ? (ring_cursor % ring_capacity) : 0;
    result.ring_cursor_out = ring_offset;

    std::lock_guard<std::mutex> lock(lock_);
    auto it = streams_.find(codec_instance_ptr);
    if (it == streams_.end()) {
      it = streams_.emplace(codec_instance_ptr, StreamState{}).first;
    }
    metrics_.copy_window_calls++;
    StreamState& stream = it->second;
    ApplyForceBackendIfAny(stream, codec_instance_ptr);

    const uint32_t planned_bytes =
        PlanReadBytesUnlocked(stream, target_bytes, ring_offset, ring_capacity);
    const bool use_native_output =
        IsHardNativeMode() || (stream.backend_decided && stream.backend == DecodeBackend::kNative);

    uint32_t copy_bytes = std::max(planned_bytes, target_bytes);
    if (use_native_output) {
      // Preserve guest stream accounting cadence in hard-native mode.
      // Reporting more bytes than FMOD requested can make it reach EOF/idle
      // too early and stop issuing codec reads.
      copy_bytes = target_bytes;
    }
    if (!use_native_output) {
      const uint32_t boost =
          static_cast<uint32_t>(std::max(REXCVAR_GET(fm2_native_copy_window_boost), 1));
      const uint32_t max_copy = static_cast<uint32_t>(
          std::max(REXCVAR_GET(fm2_native_copy_window_max_bytes), 1024));
      copy_bytes = std::min(copy_bytes * boost, max_copy);
    }

    auto append_ring_bytes = [&](uint32_t start_off, uint32_t count) {
      for (uint32_t i = 0; i < count; ++i) {
        const uint32_t off = (start_off + i) % ring_capacity;
        stream.output_fifo.push_back(mem.guest_base[ring_ptr + off]);
      }
    };

    if (!use_native_output) {
      append_ring_bytes(ring_offset, target_bytes);

      const uint32_t prefill_target = std::max(
          static_cast<uint32_t>(std::max(REXCVAR_GET(fm2_native_copy_prefill_target), 0)),
          copy_bytes);
      const uint32_t max_extra_passes = static_cast<uint32_t>(
          std::max(REXCVAR_GET(fm2_native_max_extra_passes), 0));

      if (decode_fn && stack_output_offset_ptr && byte_count_ptr) {
        uint32_t extra_passes = 0;
        const uint32_t replay_request_bytes = std::min(target_bytes, ring_capacity);
        while (stream.output_fifo.size() < prefill_target && extra_passes < max_extra_passes) {
          metrics_.replay_attempts++;
          // sub_82692CC8 treats *byte_count as in/out. Re-seed requested bytes before
          // each replay pass so follow-up decodes are not starved by a prior 0 result.
          mem.store_u32(mem.guest_base, byte_count_ptr, replay_request_bytes);
          const uint32_t stream_ptr =
              mem.load_u32(mem.guest_base, codec_instance_ptr + kCodecStreamFieldOffset);
          const uint32_t ret = decode_fn(decode_ctx, codec_instance_ptr, stack_output_offset_ptr,
                                         byte_count_ptr, stream_ptr);
          if (ret != 0) {
            metrics_.replay_ret_nonzero++;
            break;
          }

          const uint32_t next_off = mem.load_u32(mem.guest_base, stack_output_offset_ptr);
          const uint32_t next_bytes = mem.load_u32(mem.guest_base, byte_count_ptr);
          if (next_bytes == 0) {
            metrics_.replay_next_bytes_zero++;
            break;
          }
          if (next_bytes > ring_capacity) {
            metrics_.replay_next_bytes_oor++;
            break;
          }

          append_ring_bytes(next_off, next_bytes);
          extra_passes++;
          metrics_.replay_success_bytes += next_bytes;
        }
        metrics_.copy_window_extra_passes += extra_passes;
      }

      // Keep replacement output bounded to a controlled 2x expansion window.
      // This targets near-real-time cadence when FMOD only feeds ~1-2KB/read.
      copy_bytes = std::min<uint32_t>(copy_bytes, target_bytes * 2u);
    } else {
      auto* kernel_state = rex::system::kernel_state();
      uint8_t* ring_base = nullptr;
      if (kernel_state && kernel_state->memory()) {
        ring_base = kernel_state->memory()->TranslateVirtual<uint8_t*>(ring_ptr);
      }
      if (ring_base) {
        ReadCodecDataUnlocked(stream, codec_instance_ptr, ring_base, ring_offset, ring_capacity,
                              copy_bytes, false);
      }
      append_ring_bytes(ring_offset, copy_bytes);
    }

    for (uint32_t i = 0; i < copy_bytes; ++i) {
      uint8_t value = stream.hold_sample[i & 3];
      if (!stream.output_fifo.empty()) {
        value = stream.output_fifo.front();
        stream.output_fifo.pop_front();
        stream.hold_sample[i & 3] = value;
      } else if (use_native_output) {
        metrics_.native_underrun_bytes++;
        metrics_.copy_window_starve_bytes++;
      } else {
        metrics_.copy_window_starve_bytes++;
      }
      mem.guest_base[dest_ptr + i] = value;
    }

    if (byte_count_ptr && mem.readable_range &&
        mem.readable_range(mem.guest_base, byte_count_ptr, 4)) {
      mem.store_u32(mem.guest_base, byte_count_ptr, copy_bytes);
    }

    if (!use_native_output) {
      auto* kernel_state = rex::system::kernel_state();
      uint8_t* ring_base = nullptr;
      if (kernel_state && kernel_state->memory()) {
        ring_base = kernel_state->memory()->TranslateVirtual<uint8_t*>(ring_ptr);
      }
      if (ring_base) {
        ReadCodecDataUnlocked(stream, codec_instance_ptr, ring_base, ring_offset, ring_capacity,
                              copy_bytes, false);
      }
    }

    metrics_.produced_bytes += copy_bytes;
    UpdateOutputRateLocked();
    LogPerSecond(metrics_);

    result.suppress_guest_memcpy = true;
    return result;
  }

 private:
  void ApplyForceBackendIfAny(StreamState& stream, uint32_t codec_instance_ptr) {
    const int force_backend = REXCVAR_GET(fm2_native_force_backend);
    if (force_backend <= 0) {
      return;
    }

    DecodeBackend forced = DecodeBackend::kPassthrough;
    if (force_backend == 2) {
      forced = DecodeBackend::kNative;
    }

    if (stream.backend != forced || !stream.backend_decided) {
      stream.backend = forced;
      stream.backend_decided = true;

      BackendDecision forced_decision;
      forced_decision.decided = true;
      forced_decision.backend = forced;
      forced_decision.match_ratio = 1.0;
      forced_decision.rms_error = 0.0;
      forced_decision.passthrough_bytes_per_sec = 0.0;
      LogBackendDecision(codec_instance_ptr, forced_decision);
    }
  }

  void UpdateOutputRateLocked() {
    const uint32_t channels =
        static_cast<uint32_t>(std::clamp(REXCVAR_GET(fm2_native_target_channels), 1, 2));
    const uint32_t bytes_per_frame = channels * sizeof(int16_t);
    if (bytes_per_frame == 0) {
      metrics_.output_frames_per_second = 0.0;
      return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!rate_initialized_) {
      rate_initialized_ = true;
      rate_last_time_ = now;
      rate_last_produced_bytes_ = metrics_.produced_bytes;
      metrics_.output_frames_per_second = 0.0;
      return;
    }

    const double elapsed =
        std::chrono::duration_cast<std::chrono::duration<double>>(now - rate_last_time_).count();
    if (elapsed < 0.25) {
      return;
    }

    const uint64_t produced_delta = metrics_.produced_bytes - rate_last_produced_bytes_;
    metrics_.output_frames_per_second =
        (static_cast<double>(produced_delta) / static_cast<double>(bytes_per_frame)) / elapsed;
    rate_last_time_ = now;
    rate_last_produced_bytes_ = metrics_.produced_bytes;
  }

  std::mutex lock_;
  std::unordered_map<uint32_t, StreamState> streams_;
  RuntimeMetrics metrics_{};
  bool rate_initialized_ = false;
  std::chrono::steady_clock::time_point rate_last_time_{};
  uint64_t rate_last_produced_bytes_ = 0;
};

RuntimeState& GetRuntimeState() {
  static RuntimeState state;
  return state;
}

}  // namespace

void RegisterCodecInstance(uint32_t codec_instance_ptr, uint32_t flags) {
  GetRuntimeState().RegisterCodecInstance(codec_instance_ptr, flags);
}

void ReleaseCodecInstance(uint32_t codec_instance_ptr) {
  GetRuntimeState().ReleaseCodecInstance(codec_instance_ptr);
}

void MarkCodecRead(uint32_t codec_instance_ptr, uint32_t request_ptr,
                   uint32_t requested_bytes) {
  (void)request_ptr;
  GetRuntimeState().MarkCodecRead(codec_instance_ptr, requested_bytes);
}

uint32_t PlanReadBytes(uint32_t codec_instance_ptr, uint32_t requested_bytes,
                       uint32_t source_ring_offset, uint32_t source_ring_capacity) {
  return GetRuntimeState().PlanReadBytes(codec_instance_ptr, requested_bytes,
                                         source_ring_offset, source_ring_capacity);
}

bool ReadCodecData(uint32_t codec_instance_ptr, uint32_t source_ring_ptr,
                   uint32_t source_ring_offset, uint32_t source_ring_capacity,
                   uint32_t bytes_to_write) {
  return GetRuntimeState().ReadCodecData(codec_instance_ptr, source_ring_ptr,
                                         source_ring_offset, source_ring_capacity,
                                         bytes_to_write);
}

RuntimeMetrics GetRuntimeMetrics() { return GetRuntimeState().GetRuntimeMetrics(); }

void InflateCodecReadRequest(uint32_t codec_instance_ptr, uint32_t request_ptr,
                             uint32_t ring_offset, uint32_t ring_capacity) {
  GetRuntimeState().InflateCodecReadRequest(codec_instance_ptr, request_ptr, ring_offset,
                                           ring_capacity);
}

CopyWindowResult ProcessReadCopyWindow(const GuestMemoryOps& mem, GuestDecodeFn decode_fn,
                                       void* decode_ctx, uint32_t codec_instance_ptr,
                                       uint32_t dest_ptr, uint32_t byte_count_ptr,
                                       uint32_t ring_ptr, uint32_t ring_cursor,
                                       uint32_t target_bytes, uint32_t stack_output_offset_ptr) {
  return GetRuntimeState().ProcessReadCopyWindow(
      mem, decode_fn, decode_ctx, codec_instance_ptr, dest_ptr, byte_count_ptr, ring_ptr,
      ring_cursor, target_bytes, stack_output_offset_ptr);
}

}  // namespace rex::audio::fm2_native
