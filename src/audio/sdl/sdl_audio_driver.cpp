/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#include <rex/assert.h>
#include <rex/audio/conversion.h>
#include <rex/audio/flags.h>
#include <rex/audio/sdl/sdl_audio_driver.h>
#include <rex/cvar.h>
#include <rex/dbg.h>
#include <rex/logging.h>
#include <rex/perf/counter.h>
#include <SDL3/SDL.h>

// TEMP_DIAG: Frame content analysis for half-speed audio investigation
namespace {
// Simple FNV-1a hash for frame content
uint32_t fnv1a_hash(const uint8_t* data, size_t len) {
  uint32_t h = 0x811C9DC5u;
  for (size_t i = 0; i < len; i++) {
    h ^= data[i];
    h *= 0x01000193u;
  }
  return h;
}

// Byte-swap a float (BE guest -> LE host) for reading
float be_to_host_float(uint32_t be_bits) {
  uint32_t le_bits = (be_bits >> 24) | ((be_bits >> 8) & 0xFF00) |
                     ((be_bits << 8) & 0xFF0000) | (be_bits << 24);
  float result;
  std::memcpy(&result, &le_bits, sizeof(float));
  return result;
}

struct ChannelStats {
  float min_val;
  float max_val;
  float mean_abs;
  float rms;
  int zero_count;    // |sample| < 1e-7f
  int nan_inf_count;
  uint32_t hash;
  int same_as_prev;  // count of samples identical to prev frame
};

ChannelStats compute_channel_stats(const uint32_t* samples_be, size_t count,
                                   const uint32_t* prev_samples_be) {
  ChannelStats s = {1e30f, -1e30f, 0, 0, 0, 0, 0, 0};
  s.hash = fnv1a_hash(reinterpret_cast<const uint8_t*>(samples_be), count * sizeof(float));
  double sum_abs = 0, sum_sq = 0;
  for (size_t i = 0; i < count; i++) {
    float v = be_to_host_float(samples_be[i]);
    if (std::isnan(v) || std::isinf(v)) { s.nan_inf_count++; continue; }
    if (v < s.min_val) s.min_val = v;
    if (v > s.max_val) s.max_val = v;
    float av = std::fabs(v);
    sum_abs += av;
    sum_sq += (double)v * v;
    if (av < 1e-7f) s.zero_count++;
    if (prev_samples_be && samples_be[i] == prev_samples_be[i]) s.same_as_prev++;
  }
  s.mean_abs = (float)(sum_abs / count);
  s.rms = (float)std::sqrt(sum_sq / count);
  return s;
}

// Count zero crossings in a channel (after byte-swap)
int count_zero_crossings(const uint32_t* samples_be, size_t count) {
  int crossings = 0;
  float prev = be_to_host_float(samples_be[0]);
  for (size_t i = 1; i < count; i++) {
    float cur = be_to_host_float(samples_be[i]);
    if ((prev < 0 && cur >= 0) || (prev >= 0 && cur < 0)) crossings++;
    prev = cur;
  }
  return crossings;
}

// Count adjacent duplicate samples (after byte-swap)
int count_adjacent_duplicates(const uint32_t* samples_be, size_t count) {
  int dups = 0;
  for (size_t i = 1; i < count; i++) {
    if (samples_be[i] == samples_be[i - 1]) dups++;
  }
  return dups;
}

// Compare two halves of a channel block (samples 0..127 vs 128..255)
float half_correlation(const uint32_t* samples_be, size_t count) {
  size_t half = count / 2;
  double sum = 0;
  int matching = 0;
  for (size_t i = 0; i < half; i++) {
    if (samples_be[i] == samples_be[half + i]) matching++;
    float a = be_to_host_float(samples_be[i]);
    float b = be_to_host_float(samples_be[half + i]);
    sum += (double)a * b;
  }
  // Return fraction of identical samples between halves
  return (float)matching / (float)half;
}
}  // anonymous namespace
// END TEMP_DIAG

REXCVAR_DEFINE_BOOL(audio_mute, false, "Audio", "Mute audio output");

namespace rex::audio::sdl {

SDLAudioDriver::SDLAudioDriver(memory::Memory* memory, rex::thread::Semaphore* semaphore)
    : AudioDriver(memory), semaphore_(semaphore) {}

SDLAudioDriver::~SDLAudioDriver() {
  assert_true(frames_queued_.empty());
  assert_true(frames_unused_.empty());
}

bool SDLAudioDriver::Initialize() {
  // Prevent SDL from interfering with timer resolution (causes FPS drops)
  SDL_SetHintWithPriority(SDL_HINT_TIMER_RESOLUTION, "0", SDL_HINT_OVERRIDE);

  // Set audio category for proper OS audio handling
  SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "playback");

  // Set app name for audio device identification
  SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "rexglue");

  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    REXAPU_ERROR("SDL_InitSubSystem(SDL_INIT_AUDIO) failed: {}", SDL_GetError());
    return false;
  }
  sdl_initialized_ = true;

  SDL_AudioSpec desired_spec = {};
  SDL_AudioSpec obtained_spec = {};
  desired_spec.freq = frame_frequency_;
  desired_spec.format = SDL_AUDIO_F32LE;
  desired_spec.channels = frame_channels_;
  sdl_device_channels_ = frame_channels_;
  sdl_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired_spec,
                                          SDLCallback, this);
  if (!sdl_stream_) {
    REXAPU_ERROR("SDL_OpenAudioDeviceStream() failed: {}", SDL_GetError());
    return false;
  }

  SDL_AudioDeviceID sdl_device = SDL_GetAudioStreamDevice(sdl_stream_);
  if (!sdl_device) {
    REXAPU_ERROR("SDL_GetAudioStreamDevice() failed: {}", SDL_GetError());
    return false;
  }

  if (!SDL_GetAudioDeviceFormat(sdl_device, &obtained_spec, NULL)) {
    REXAPU_WARN("SDL_GetAudioDeviceFormat() failed: {}", SDL_GetError());
    obtained_spec = desired_spec;
  }

  if (obtained_spec.channels == 2) {
    SDL_DestroyAudioStream(sdl_stream_);
    sdl_stream_ = nullptr;
    desired_spec.channels = 2;
    sdl_device_channels_ = 2;
    sdl_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired_spec,
                                            SDLCallback, this);
    if (!sdl_stream_) {
      REXAPU_ERROR("SDL_OpenAudioDeviceStream() stereo fallback failed: {}", SDL_GetError());
      return false;
    }
    sdl_device = SDL_GetAudioStreamDevice(sdl_stream_);
    if (!sdl_device) {
      REXAPU_ERROR("SDL_GetAudioStreamDevice() failed after stereo fallback: {}", SDL_GetError());
      return false;
    }
  }

  if (!SDL_ResumeAudioDevice(sdl_device)) {
    REXAPU_ERROR("SDL_ResumeAudioDevice() failed: {}", SDL_GetError());
    return false;
  }

  return true;
}

void SDLAudioDriver::SubmitFrame(uint32_t frame_ptr) {
  const auto input_frame = memory_->TranslateVirtual<float*>(frame_ptr);
  float* output_frame;
  {
    std::unique_lock<std::mutex> guard(frames_mutex_);
    if (frames_unused_.empty()) {
      output_frame = new float[frame_samples_];
    } else {
      output_frame = frames_unused_.top();
      frames_unused_.pop();
    }
  }

  std::memcpy(output_frame, input_frame, frame_samples_ * sizeof(float));

  // TEMP_DIAG: Frame content analysis — sample every ~0.5s (every 94th frame at 188/s)
  {
    static uint32_t diag_frame_idx = 0;
    static uint32_t diag_prev_raw[6 * 256];  // 6ch * 256 samples, raw BE uint32
    static bool diag_has_prev = false;
    static bool diag_capture_window = true;  // capture first 5 seconds
    static auto diag_start = std::chrono::steady_clock::now();

    diag_frame_idx++;

    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - diag_start).count();
    // Only log during first 5 seconds, every ~0.5s
    if (elapsed < 5.0f && diag_frame_idx % 94 == 0) {
      const uint32_t* raw = reinterpret_cast<const uint32_t*>(input_frame);
      uint32_t full_hash = fnv1a_hash(reinterpret_cast<const uint8_t*>(raw), frame_size_);

      // First 16 raw bytes
      REXAPU_ERROR("PCM_DIAG[{}] t={:.2f}s ptr={:08X} hash={:08X} "
                   "raw16={:02X}{:02X}{:02X}{:02X} {:02X}{:02X}{:02X}{:02X} "
                   "{:02X}{:02X}{:02X}{:02X} {:02X}{:02X}{:02X}{:02X}",
                   diag_frame_idx, elapsed, frame_ptr, full_hash,
                   raw[0] & 0xFF, (raw[0] >> 8) & 0xFF, (raw[0] >> 16) & 0xFF, (raw[0] >> 24) & 0xFF,
                   raw[1] & 0xFF, (raw[1] >> 8) & 0xFF, (raw[1] >> 16) & 0xFF, (raw[1] >> 24) & 0xFF,
                   raw[2] & 0xFF, (raw[2] >> 8) & 0xFF, (raw[2] >> 16) & 0xFF, (raw[2] >> 24) & 0xFF,
                   raw[3] & 0xFF, (raw[3] >> 8) & 0xFF, (raw[3] >> 16) & 0xFF, (raw[3] >> 24) & 0xFF);

      // Same as previous full frame?
      if (diag_has_prev) {
        uint32_t prev_hash = fnv1a_hash(reinterpret_cast<const uint8_t*>(diag_prev_raw), frame_size_);
        REXAPU_ERROR("PCM_DIAG[{}] prev_hash={:08X} same_prev={} same_raw_bytes={}",
                     diag_frame_idx, prev_hash, full_hash == prev_hash ? 1 : 0,
                     std::memcmp(raw, diag_prev_raw, frame_size_) == 0 ? 1 : 0);
      }

      // Per-channel stats (6 channels, 256 samples each, sequential layout)
      const char* ch_names[] = {"FL", "FR", "FC", "LF", "BL", "BR"};
      for (int ch = 0; ch < 6; ch++) {
        const uint32_t* ch_data = raw + ch * 256;
        const uint32_t* prev_ch = diag_has_prev ? (diag_prev_raw + ch * 256) : nullptr;
        ChannelStats st = compute_channel_stats(ch_data, 256, prev_ch);
        int zc = count_zero_crossings(ch_data, 256);
        int adj_dup = count_adjacent_duplicates(ch_data, 256);
        float half_corr = half_correlation(ch_data, 256);
        REXAPU_ERROR("PCM_DIAG[{}] {} min={:+.5f} max={:+.5f} meanAbs={:.5f} rms={:.5f} "
                     "zero={} nan={} chash={:08X} samePrev={} zcross={} adjDup={} halfMatch={:.2f}",
                     diag_frame_idx, ch_names[ch], st.min_val, st.max_val, st.mean_abs, st.rms,
                     st.zero_count, st.nan_inf_count, st.hash, st.same_as_prev,
                     zc, adj_dup, half_corr);
      }
    }

    // Always save for next comparison (only during capture window)
    if (elapsed < 5.0f) {
      const uint32_t* save_raw = reinterpret_cast<const uint32_t*>(input_frame);
      std::memcpy(diag_prev_raw, save_raw, frame_size_);
      diag_has_prev = true;
    }
  }
  // END TEMP_DIAG

  static uint32_t sdl_submit_count = 0;
  if (sdl_submit_count < 10) {
    REXAPU_DEBUG("SDLAudioDriver::SubmitFrame: frame_ptr={:08X} queued_count={}", frame_ptr,
                 frames_queued_.size() + 1);
    sdl_submit_count++;
  }

  {
    std::unique_lock<std::mutex> guard(frames_mutex_);
    frames_queued_.push(output_frame);
    PROFILE_BUFFER_QUEUE_DEPTH(static_cast<int64_t>(frames_queued_.size()));
  }
}

void SDLAudioDriver::Shutdown() {
  if (sdl_stream_) {
    SDL_DestroyAudioStream(sdl_stream_);
    sdl_stream_ = nullptr;
  }
  if (sdl_initialized_) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    sdl_initialized_ = false;
  }
  std::unique_lock<std::mutex> guard(frames_mutex_);
  while (!frames_unused_.empty()) {
    delete[] frames_unused_.top();
    frames_unused_.pop();
  }
  while (!frames_queued_.empty()) {
    delete[] frames_queued_.front();
    frames_queued_.pop();
  }
}

void SDLAudioDriver::SDLCallback(void* userdata, SDL_AudioStream* stream, int additional_amount,
                                 [[maybe_unused]] int total_amount) {
  SCOPE_profile_cpu_f("apu");
  if (!userdata || !stream) {
    REXAPU_ERROR("SDLAudioDriver::SDLCallback called with nullptr.");
    return;
  }
  const auto driver = static_cast<SDLAudioDriver*>(userdata);
  const int sample_count =
      static_cast<int>(channel_samples_ * std::max<uint8_t>(driver->sdl_device_channels_, 1));
  const int len = static_cast<int>(sizeof(float) * sample_count);
  float* data = SDL_stack_alloc(float, sample_count);
  if (!data) {
    REXAPU_ERROR("SDLAudioDriver::SDLCallback failed to allocate {} samples", sample_count);
    return;
  }
  static uint32_t sdl_callback_count = 0;
  static uint32_t sdl_real_frames = 0;
  static uint32_t sdl_silence_frames = 0;
  static auto sdl_stats_start = std::chrono::steady_clock::now();
  static bool sdl_was_draining = false;  // TEMP_DIAG: track queue drain transitions
  while (additional_amount > 0) {
    std::unique_lock<std::mutex> guard(driver->frames_mutex_);
    auto queue_depth = driver->frames_queued_.size();
    // TEMP_DIAG: Log queue drain entry/exit
    if (queue_depth <= 2 && !sdl_was_draining) {
      REXAPU_ERROR("AUDIO_DIAG: queue DRAINING depth={} count={}", queue_depth, sdl_callback_count);
      sdl_was_draining = true;
    } else if (queue_depth > 3 && sdl_was_draining) {
      REXAPU_ERROR("AUDIO_DIAG: queue RECOVERED depth={} count={}", queue_depth, sdl_callback_count);
      sdl_was_draining = false;
    }
    if (driver->frames_queued_.empty()) {
      // TEMP_DIAG: Log silence underruns — uses ERROR to bypass log_level filter
      if (sdl_silence_frames < 50 || sdl_silence_frames % 100 == 0) {
        REXAPU_ERROR("AUDIO_DIAG: SILENCE underrun #{} queue=0", sdl_silence_frames);
      }
      sdl_silence_frames++;
      std::memset(data, 0, len);
      if (!SDL_PutAudioStreamData(stream, data, len)) {
        REXAPU_ERROR("SDL_PutAudioStreamData() failed while filling silence: {}", SDL_GetError());
        break;
      }
      // Release semaphore even during silence so the worker thread can
      // produce new frames and refill the queue. Without this, the worker
      // blocks forever once the queue empties (death spiral).
      auto ret = driver->semaphore_->Release(1, nullptr);
      assert_true(ret);
      additional_amount -= len;
    } else {
      auto buffer = driver->frames_queued_.front();
      driver->frames_queued_.pop();
      if (REXCVAR_GET(audio_mute)) {
        std::memset(data, 0, len);
      } else {
        switch (driver->sdl_device_channels_) {
          case 2:
            conversion::sequential_6_BE_to_interleaved_2_LE(data, buffer, channel_samples_);
            break;
          case 6:
            conversion::sequential_6_BE_to_interleaved_6_LE(data, buffer, channel_samples_);
            break;
          default:
            assert_unhandled_case(driver->sdl_device_channels_);
            break;
        }
      }
      // TEMP_DIAG: Analyze conversion output — match with SubmitFrame cadence
      {
        static uint32_t conv_diag_idx = 0;
        static auto conv_diag_start = std::chrono::steady_clock::now();
        conv_diag_idx++;
        float conv_elapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - conv_diag_start).count();
        if (conv_elapsed < 5.0f && conv_diag_idx % 94 == 0) {
          const uint32_t* out_raw = reinterpret_cast<const uint32_t*>(data);
          int out_samples = sample_count;  // 256*2 for stereo, 256*6 for 5.1
          uint32_t out_hash = fnv1a_hash(reinterpret_cast<const uint8_t*>(data),
                                         out_samples * sizeof(float));
          // Compute RMS for first two output channels (L/R)
          double sum_sq_lr = 0;
          int lr_count = 0;
          int out_zero = 0;
          int out_adj_dup = 0;
          int ch_stride = driver->sdl_device_channels_;
          for (int i = 0; i < channel_samples_; i++) {
            for (int c = 0; c < (ch_stride < 3 ? ch_stride : 2); c++) {
              float v = data[i * ch_stride + c];
              if (std::fabs(v) < 1e-7f) out_zero++;
              sum_sq_lr += (double)v * v;
              lr_count++;
            }
            if (i > 0) {
              for (int c = 0; c < ch_stride; c++) {
                if (out_raw[i * ch_stride + c] == out_raw[(i - 1) * ch_stride + c])
                  out_adj_dup++;
              }
            }
          }
          float out_rms = (float)std::sqrt(sum_sq_lr / lr_count);
          // Check if first 128 output samples == second 128 (per-channel interleaved)
          int half_match = 0;
          int half_total = channel_samples_ / 2 * ch_stride;
          for (int i = 0; i < half_total; i++) {
            if (out_raw[i] == out_raw[half_total + i]) half_match++;
          }
          REXAPU_ERROR("PCM_CONV[{}] t={:.2f}s ch={} outHash={:08X} rms={:.5f} zero={} "
                       "adjDup={} halfMatch={}/{} ({:.2f})",
                       conv_diag_idx, conv_elapsed, driver->sdl_device_channels_,
                       out_hash, out_rms, out_zero,
                       out_adj_dup, half_match, half_total,
                       (float)half_match / half_total);
          // Log first 8 output floats (LE, directly readable)
          REXAPU_ERROR("PCM_CONV[{}] first8f={:+.5f} {:+.5f} {:+.5f} {:+.5f} "
                       "{:+.5f} {:+.5f} {:+.5f} {:+.5f}",
                       conv_diag_idx,
                       data[0], data[1], data[2], data[3],
                       data[4], data[5], data[6], data[7]);
        }
      }
      // END TEMP_DIAG
      if (!SDL_PutAudioStreamData(stream, data, len)) {
        REXAPU_ERROR("SDL_PutAudioStreamData() failed: {}", SDL_GetError());
        driver->frames_unused_.push(buffer);
        break;
      }
      driver->frames_unused_.push(buffer);

      auto ret = driver->semaphore_->Release(1, nullptr);
      assert_true(ret);
      sdl_real_frames++;
      additional_amount -= len;
    }
    // Log stats every 2 seconds
    sdl_callback_count++;
    if (sdl_callback_count % 375 == 0) {  // ~2s at 188 callbacks/sec
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration<float>(now - sdl_stats_start).count();
      REXAPU_ERROR("SDL_AUDIO_STATS: real={} silence={} total={} elapsed={:.1f}s "
                  "real_rate={:.0f}/s expected=188/s fill={:.0f}%",
                  sdl_real_frames, sdl_silence_frames,
                  sdl_real_frames + sdl_silence_frames, elapsed,
                  sdl_real_frames / elapsed,
                  100.0f * sdl_real_frames / (sdl_real_frames + sdl_silence_frames + 0.001f));
    }
  }
  SDL_stack_free(data);
}

}  // namespace rex::audio::sdl
