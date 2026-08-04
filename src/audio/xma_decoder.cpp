/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/audio/xma/context.h>
#include <rex/audio/xma/decoder.h>
#include <algorithm>
#include <chrono>
#include <limits>
#include <rex/cvar.h>
#include <rex/dbg.h>
#include <rex/logging.h>
#include <rex/perf/counter.h>
#include <rex/math.h>
#include <rex/memory/ring_buffer.h>
#include <rex/string/buffer.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/thread_state.h>
#include <rex/system/xthread.h>
#include <unordered_map>
#include <vector>

extern "C" {
#include "libavutil/log.h"
}  // extern "C"

REXCVAR_DEFINE_BOOL(ffmpeg_verbose, false, "Audio", "Verbose FFmpeg output (debug and above)");
REXCVAR_DEFINE_BOOL(audio_xma_mix_diag, false, "Audio",
                    "Log top active XMA contexts by RMS level once per interval");
REXCVAR_DEFINE_INT32(audio_xma_mix_diag_top, 8, "Audio",
                     "Number of XMA contexts to include in each mix diagnostics sample");
REXCVAR_DEFINE_INT32(audio_xma_mix_diag_interval_ms, 1000, "Audio",
                     "Interval in milliseconds for XMA mix diagnostics logging");
REXCVAR_DEFINE_BOOL(audio_xma_player_trace, false, "Audio",
                    "Track likely player-engine XMA contexts by stable signature and activity");
REXCVAR_DEFINE_INT32(audio_xma_player_trace_top, 4, "Audio",
                     "Number of likely player-engine candidates to log per trace sample");
REXCVAR_DEFINE_INT32(audio_xma_player_trace_interval_ms, 1000, "Audio",
                     "Interval in milliseconds for likely player-engine trace logging");

// As with normal Microsoft, there are like twelve different ways to access
// the audio APIs. Early games use XMA*() methods almost exclusively to touch
// decoders. Later games use XAudio*() and direct memory writes to the XMA
// structures (as opposed to the XMA* calls), meaning that we have to support
// both.
//
// The XMA*() functions just manipulate the audio system in the guest context
// and let the normal XmaDecoder handling take it, to prevent duplicate
// implementations. They can be found in xboxkrnl_audio_xma.cc
//
// XMA details:
// https://devel.nuclex.org/external/svn/directx/trunk/include/xma2defs.h
// https://github.com/gdawg/fsbext/blob/master/src/xma_header.h
//
// XAudio2 uses XMA under the covers, and seems to map with the same
// restrictions of frame/subframe/etc:
// https://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.xaudio2.xaudio2_buffer(v=vs.85).aspx
//
// XMA contexts are 64b in size and tight bitfields. They are in physical
// memory not usually available to games. Games will use MmMapIoSpace to get
// the 64b pointer in user memory so they can party on it. If the game doesn't
// do this, it's likely they are either passing the context to XAudio or
// using the XMA* functions.

namespace rex::audio {

namespace {

uint64_t HashCombine64(uint64_t hash, uint64_t value) {
  hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
  return hash;
}

uint64_t BuildContextSignature(const XMA_CONTEXT_DATA& ctx) {
  if (ctx.output_buffer_ptr == 0 && ctx.input_buffer_0_ptr == 0 && ctx.input_buffer_1_ptr == 0) {
    return 0;
  }
  uint64_t hash = 0xcbf29ce484222325ULL;
  hash = HashCombine64(hash, static_cast<uint64_t>(ctx.output_buffer_ptr));
  hash = HashCombine64(hash, static_cast<uint64_t>(ctx.input_buffer_0_ptr));
  hash = HashCombine64(hash, static_cast<uint64_t>(ctx.input_buffer_1_ptr));
  hash = HashCombine64(hash, static_cast<uint64_t>(ctx.sample_rate));
  hash = HashCombine64(hash, ctx.is_stereo ? 1ULL : 0ULL);
  return hash;
}

struct MixDiagContext {
  uint32_t index = 0;
  uint64_t signature = 0;
  float rms = 0.0f;
  float peak = 0.0f;
  float rms_ch0 = 0.0f;
  float rms_ch1 = 0.0f;
  float audible_rms = 0.0f;
  float audible_peak = 0.0f;
  float audible_rms_ch0 = 0.0f;
  float audible_rms_ch1 = 0.0f;
  float volume = 1.0f;
  bool muted = false;
  bool enabled = false;
  bool input0_valid = false;
  bool input1_valid = false;
  bool output_valid = false;
  bool stereo = false;
  uint8_t sample_rate_id = 0;
  uint32_t input_buffer_0_ptr = 0;
  uint32_t input_buffer_1_ptr = 0;
  uint32_t output_buffer_ptr = 0;
};

struct PlayerTraceSigState {
  uint64_t seen_samples = 0;
  uint64_t active_samples = 0;
  uint64_t last_seen_sample = 0;
  float ema_audible_rms = 0.0f;
  float ema_audible_peak = 0.0f;
  float ema_activity_delta = 0.0f;
  uint8_t sample_rate_id = 0;
  bool stereo = false;
  uint32_t last_ctx = 0;
};

struct PlayerTraceState {
  uint64_t sample_counter = 0;
  std::unordered_map<uint64_t, PlayerTraceSigState> by_signature;
};

PlayerTraceState& GetPlayerTraceState() {
  static PlayerTraceState state;
  return state;
}

void EmitLikelyPlayerTrace(const std::vector<MixDiagContext>& rows) {
  auto& trace_state = GetPlayerTraceState();
  trace_state.sample_counter += 1;
  const uint64_t sample_index = trace_state.sample_counter;

  std::unordered_map<uint64_t, const MixDiagContext*> rows_by_sig;
  rows_by_sig.reserve(rows.size());
  for (const auto& row : rows) {
    if (row.signature == 0) {
      continue;
    }
    rows_by_sig[row.signature] = &row;
    auto& sig = trace_state.by_signature[row.signature];
    const float prev_rms = sig.ema_audible_rms;
    const float prev_peak = sig.ema_audible_peak;
    const bool first = (sig.seen_samples == 0);
    sig.seen_samples += 1;
    sig.last_seen_sample = sample_index;
    sig.sample_rate_id = row.sample_rate_id;
    sig.stereo = row.stereo;
    sig.last_ctx = row.index;
    if (row.audible_rms > 0.01f || row.audible_peak > 0.02f || row.output_valid || row.enabled) {
      sig.active_samples += 1;
    }
    if (first) {
      sig.ema_audible_rms = row.audible_rms;
      sig.ema_audible_peak = row.audible_peak;
      sig.ema_activity_delta = 0.0f;
    } else {
      constexpr float kEmaAlpha = 0.20f;
      constexpr float kDeltaAlpha = 0.20f;
      sig.ema_audible_rms = prev_rms + (row.audible_rms - prev_rms) * kEmaAlpha;
      sig.ema_audible_peak = prev_peak + (row.audible_peak - prev_peak) * kEmaAlpha;
      const float instant_delta = std::abs(row.audible_rms - prev_rms);
      sig.ema_activity_delta =
          sig.ema_activity_delta + (instant_delta - sig.ema_activity_delta) * kDeltaAlpha;
    }
  }

  for (auto it = trace_state.by_signature.begin(); it != trace_state.by_signature.end();) {
    if (sample_index - it->second.last_seen_sample > 120) {
      it = trace_state.by_signature.erase(it);
    } else {
      ++it;
    }
  }

  struct Candidate {
    uint64_t signature = 0;
    float score = 0.0f;
    float persistence = 0.0f;
    const PlayerTraceSigState* sig = nullptr;
    const MixDiagContext* row = nullptr;
  };

  std::vector<Candidate> candidates;
  candidates.reserve(trace_state.by_signature.size());
  for (const auto& [signature, sig] : trace_state.by_signature) {
    const auto row_it = rows_by_sig.find(signature);
    if (row_it == rows_by_sig.end()) {
      continue;
    }
    const uint64_t age = sample_index - sig.last_seen_sample;
    if (age > 4 || sig.seen_samples < 3 || sig.active_samples < 2) {
      continue;
    }
    const float persistence =
        static_cast<float>(sig.active_samples) / static_cast<float>(sig.seen_samples);
    const float stability = std::clamp(1.0f - sig.ema_activity_delta * 20.0f, 0.0f, 1.0f);
    float score = sig.ema_audible_rms * 0.50f + persistence * 0.35f + stability * 0.15f;
    if (sig.sample_rate_id == 3) {
      score += 0.05f;
    }
    if (score < 0.20f) {
      continue;
    }
    candidates.push_back(Candidate{signature, score, persistence, &sig, row_it->second});
  }

  std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
    if (a.score == b.score) {
      return a.sig->ema_audible_rms > b.sig->ema_audible_rms;
    }
    return a.score > b.score;
  });

  const int32_t top_count =
      std::max<int32_t>(1, std::min<int32_t>(REXCVAR_GET(audio_xma_player_trace_top), 12));
  const int32_t limit = std::min<int32_t>(top_count, static_cast<int32_t>(candidates.size()));
  REXAPU_ERROR("XMA_PLAYER_TRACE sample={} tracked={} candidates={}", sample_index,
               trace_state.by_signature.size(), candidates.size());
  for (int32_t i = 0; i < limit; ++i) {
    const auto& c = candidates[static_cast<size_t>(i)];
    const auto& s = *c.sig;
    const auto& r = *c.row;
    const uint64_t age = sample_index - s.last_seen_sample;
    REXAPU_ERROR(
        "XMA_PLAYER_TRACE #{:02d} sig={:016X} ctx={:03X} score={:.3f} ar={:.4f} ap={:.4f} "
        "persist={:.2f} delta={:.5f} active={}/{} age={} vol={:.3f} muted={} en={} out={} "
        "sr={} st={} out_ptr={:08X}",
        i, c.signature, r.index, c.score, s.ema_audible_rms, s.ema_audible_peak, c.persistence,
        s.ema_activity_delta, s.active_samples, s.seen_samples, age, r.volume, r.muted ? 1 : 0,
        r.enabled ? 1 : 0, r.output_valid ? 1 : 0, static_cast<uint32_t>(r.sample_rate_id),
        r.stereo ? 1 : 0, r.output_buffer_ptr);
  }
}

}  // namespace

XmaDecoder::XmaDecoder(runtime::FunctionDispatcher* function_dispatcher)
    : memory_(function_dispatcher->memory()), function_dispatcher_(function_dispatcher) {}

XmaDecoder::~XmaDecoder() = default;

void av_log_callback(void* avcl, int level, const char* fmt, va_list va) {
  if (!REXCVAR_GET(ffmpeg_verbose) && level > AV_LOG_WARNING) {
    return;
  }

  string::StringBuffer buff;
  buff.AppendVarargs(fmt, va);
  auto msg = buff.to_string_view();

  switch (level) {
    case AV_LOG_ERROR:
      REXAPU_ERROR("ffmpeg: {}", msg);
      break;
    case AV_LOG_WARNING:
      REXAPU_WARN("ffmpeg: {}", msg);
      break;
    case AV_LOG_INFO:
      REXAPU_INFO("ffmpeg: {}", msg);
      break;
    case AV_LOG_VERBOSE:
    case AV_LOG_DEBUG:
    default:
      REXAPU_DEBUG("ffmpeg: {}", msg);
      break;
  }
}

X_STATUS XmaDecoder::Setup(system::KernelState* kernel_state) {
  // Setup ffmpeg logging callback
  av_log_set_callback(av_log_callback);

  // Register APU/XMA MMIO handlers
  // XMA registers are at 0x7FEA0000-0x7FEAFFFF
  memory()->AddVirtualMappedRange(
      0x7FEA0000,  // base address
      0xFFFF0000,  // mask
      0x0000FFFF,  // size (64KB)
      this,        // context (XmaDecoder*)
      reinterpret_cast<runtime::MMIOReadCallback>(MMIOReadRegisterThunk),
      reinterpret_cast<runtime::MMIOWriteCallback>(MMIOWriteRegisterThunk));
  REXAPU_DEBUG("XMA: Registered MMIO handlers at 0x7FEA0000-0x7FEAFFFF");

  // Setup XMA context data.
  // The Xbox 360 kernel allocates the contexts with X_PAGE_NOCACHE |
  // X_PAGE_READWRITE and writes MmGetPhysicalAddress for the address to the
  // register.
  context_data_first_ptr_ = memory()->SystemHeapAlloc(sizeof(XMA_CONTEXT_DATA) * kContextCount, 256,
                                                      memory::kSystemHeapPhysical);
  context_data_last_ptr_ = context_data_first_ptr_ + (sizeof(XMA_CONTEXT_DATA) * kContextCount - 1);
  register_file_[XmaRegister::ContextArrayAddress] =
      memory()->GetPhysicalAddress(context_data_first_ptr_);

  // Setup XMA contexts.
  for (size_t i = 0; i < kContextCount; ++i) {
    uint32_t guest_ptr = context_data_first_ptr_ + i * sizeof(XMA_CONTEXT_DATA);
    XmaContext& context = contexts_[i];
    if (context.Setup(i, memory(), guest_ptr)) {
      assert_always();
    }
  }
  register_file_[XmaRegister::NextContextIndex] = 1;
  context_bitmap_.Resize(kContextCount);

  worker_running_ = true;
  work_event_ = rex::thread::Event::CreateAutoResetEvent(false);
  assert_not_null(work_event_);
  worker_thread_ = system::object_ref<system::XHostThread>(
      new system::XHostThread(kernel_state, 128 * 1024, 0, [this]() {
        WorkerThreadMain();
        return 0;
      }));
  worker_thread_->set_name("XMA Decoder");

  worker_thread_->Create();

  return X_STATUS_SUCCESS;
}

void XmaDecoder::WorkerThreadMain() {
  auto next_mix_diag_time = std::chrono::steady_clock::now();
  auto next_player_trace_time = std::chrono::steady_clock::now();
  auto next_debug_rate_time = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  uint32_t sweeps_this_second = 0;
  uint32_t decode_iterations_this_second = 0;
  while (worker_running_) {
    const auto sweep_start_time = std::chrono::steady_clock::now();
    // Okay, let's loop through XMA contexts to find ones we need to decode!
    bool did_work = false;
    uint32_t worked_contexts = 0;
    for (uint32_t n = 0; n < kContextCount && worker_running_; n++) {
      XmaContext& context = contexts_[n];
      bool worked = context.Work();
      if (worked) {
        context.SignalWorkDone();
        PROFILE_XMA_FRAME_DECODED();
        worked_contexts += 1;
      }
      did_work = did_work || worked;
    }
    const auto sweep_end_time = std::chrono::steady_clock::now();
    const uint32_t sweep_time_us = static_cast<uint32_t>(std::chrono::duration_cast<
                                                          std::chrono::microseconds>(
                                                              sweep_end_time - sweep_start_time)
                                                              .count());
    debug_vp_total_worker_time_us_.store(sweep_time_us, std::memory_order_relaxed);
    debug_vp_worker_voices_.store(worked_contexts, std::memory_order_relaxed);
    sweeps_this_second += 1;
    decode_iterations_this_second += worked_contexts;
    if (sweep_end_time >= next_debug_rate_time) {
      debug_vp_sweeps_per_second_.store(sweeps_this_second, std::memory_order_relaxed);
      debug_vp_decode_iterations_per_second_.store(decode_iterations_this_second,
                                                   std::memory_order_relaxed);
      const uint64_t gp_cycles_estimate = static_cast<uint64_t>(sweep_time_us) * 160ULL;
      debug_gp_cycles_estimate_.store(static_cast<uint32_t>(std::min<uint64_t>(
                                          gp_cycles_estimate, std::numeric_limits<uint32_t>::max())),
                                      std::memory_order_relaxed);
      debug_ep_cycles_estimate_.store(0, std::memory_order_relaxed);
      sweeps_this_second = 0;
      decode_iterations_this_second = 0;
      next_debug_rate_time = sweep_end_time + std::chrono::seconds(1);
    }

    if (paused_) {
      pause_fence_.Signal();
      resume_fence_.Wait();
    }

    if (did_work) {
      const bool mix_diag_enabled = REXCVAR_GET(audio_xma_mix_diag);
      const bool player_trace_enabled = REXCVAR_GET(audio_xma_player_trace);
      if (mix_diag_enabled || player_trace_enabled) {
        const auto now = std::chrono::steady_clock::now();
        const bool emit_mix_diag = mix_diag_enabled && now >= next_mix_diag_time;
        const bool emit_player_trace = player_trace_enabled && now >= next_player_trace_time;
        if (emit_mix_diag || emit_player_trace) {
          std::vector<MixDiagContext> rows;
          rows.reserve(kContextCount);
          for (uint32_t i = 0; i < kContextCount; ++i) {
            auto& context = contexts_[i];
            if (!context.is_allocated() || context.guest_ptr() == 0) {
              continue;
            }
            auto* context_ptr = memory()->TranslateVirtual(context.guest_ptr());
            if (!context_ptr) {
              continue;
            }
            XMA_CONTEXT_DATA data(context_ptr);
            MixDiagContext row;
            row.index = i;
            row.signature = BuildContextSignature(data);
            row.rms = context.last_rms_level();
            row.peak = context.last_peak_level();
            row.rms_ch0 = context.last_rms_ch0_level();
            row.rms_ch1 = context.last_rms_ch1_level();
            row.audible_rms = context.last_audible_rms_level();
            row.audible_peak = context.last_audible_peak_level();
            row.audible_rms_ch0 = context.last_audible_rms_ch0_level();
            row.audible_rms_ch1 = context.last_audible_rms_ch1_level();
            row.volume = context.volume();
            row.muted = context.is_muted();
            row.enabled = context.is_enabled();
            row.input0_valid = data.input_buffer_0_valid != 0;
            row.input1_valid = data.input_buffer_1_valid != 0;
            row.output_valid = data.output_buffer_valid != 0;
            row.stereo = data.is_stereo != 0;
            row.sample_rate_id = static_cast<uint8_t>(data.sample_rate);
            row.input_buffer_0_ptr = data.input_buffer_0_ptr;
            row.input_buffer_1_ptr = data.input_buffer_1_ptr;
            row.output_buffer_ptr = data.output_buffer_ptr;
            if (row.audible_rms > 0.0005f || row.audible_peak > 0.001f || row.rms > 0.0005f ||
                row.peak > 0.001f || row.enabled || row.output_valid) {
              rows.push_back(row);
            }
          }

          if (emit_mix_diag) {
            std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
              if (a.audible_rms == b.audible_rms) {
                if (a.rms == b.rms) {
                  return a.peak > b.peak;
                }
                return a.rms > b.rms;
              }
              return a.audible_rms > b.audible_rms;
            });

            const int32_t top_count =
                std::max<int32_t>(1, std::min<int32_t>(REXCVAR_GET(audio_xma_mix_diag_top), 24));
            REXAPU_ERROR("XMA_MIX_DIAG active={} top={}", rows.size(), top_count);
            const int32_t limit = std::min<int32_t>(top_count, static_cast<int32_t>(rows.size()));
            for (int32_t row_index = 0; row_index < limit; ++row_index) {
              const auto& row = rows[static_cast<size_t>(row_index)];
              REXAPU_ERROR(
                  "XMA_MIX_DIAG #{:02d} ctx={:03X} sig={:016X} rms={:.4f}[ch0={:.4f} ch1={:.4f}] "
                  "peak={:.4f} audible_rms={:.4f}[ch0={:.4f} ch1={:.4f}] audible_peak={:.4f} "
                  "vol={:.3f} muted={} "
                  "en={} in0={} in1={} out={} st={} sr={} out_ptr={:08X} in0_ptr={:08X} "
                  "in1_ptr={:08X}",
                  row_index, row.index, row.signature, row.rms, row.rms_ch0, row.rms_ch1,
                  row.peak, row.audible_rms, row.audible_rms_ch0, row.audible_rms_ch1,
                  row.audible_peak, row.volume, row.muted ? 1 : 0,
                  row.enabled ? 1 : 0, row.input0_valid ? 1 : 0, row.input1_valid ? 1 : 0,
                  row.output_valid ? 1 : 0, row.stereo ? 1 : 0,
                  static_cast<uint32_t>(row.sample_rate_id), row.output_buffer_ptr,
                  row.input_buffer_0_ptr, row.input_buffer_1_ptr);
            }

            int32_t interval_ms = std::max<int32_t>(100, REXCVAR_GET(audio_xma_mix_diag_interval_ms));
            next_mix_diag_time = now + std::chrono::milliseconds(interval_ms);
          }

          if (emit_player_trace) {
            EmitLikelyPlayerTrace(rows);
            int32_t interval_ms =
                std::max<int32_t>(100, REXCVAR_GET(audio_xma_player_trace_interval_ms));
            next_player_trace_time = now + std::chrono::milliseconds(interval_ms);
          }
        }
      } else {
        next_mix_diag_time = std::chrono::steady_clock::now();
        next_player_trace_time = std::chrono::steady_clock::now();
      }
      continue;
    }
    // No work done this iteration, block until signaled.
    rex::thread::Wait(work_event_.get(), false);
  }
}

void XmaDecoder::Shutdown() {
  if (!worker_thread_) {
    return;
  }

  worker_running_ = false;

  if (work_event_) {
    work_event_->Set();
  }

  if (paused_) {
    Resume();
  }

  // Wait up to 2 seconds for worker thread to exit gracefully.
  auto result = rex::thread::Wait(worker_thread_->thread(), false, std::chrono::milliseconds(2000));
  if (result == rex::thread::WaitResult::kTimeout) {
    REXAPU_WARN("XMA: Worker thread did not exit within 2s, abandoning");
  }
  worker_thread_.reset();

  if (context_data_first_ptr_) {
    memory()->SystemHeapFree(context_data_first_ptr_);
  }

  context_data_first_ptr_ = 0;
  context_data_last_ptr_ = 0;
}

int XmaDecoder::GetContextId(uint32_t guest_ptr) {
  static_assert_size(XMA_CONTEXT_DATA, 64);
  if (guest_ptr < context_data_first_ptr_ || guest_ptr > context_data_last_ptr_) {
    return -1;
  }
  assert_zero(guest_ptr & 0x3F);
  return (guest_ptr - context_data_first_ptr_) >> 6;
}

uint32_t XmaDecoder::AllocateContext() {
  size_t index = context_bitmap_.Acquire();
  if (index == -1) {
    // Out of contexts.
    return 0;
  }

  XmaContext& context = contexts_[index];
  assert_false(context.is_allocated());
  context.set_is_allocated(true);
  return context.guest_ptr();
}

void XmaDecoder::ReleaseContext(uint32_t guest_ptr) {
  auto context_id = GetContextId(guest_ptr);
  assert_true(context_id >= 0);

  XmaContext& context = contexts_[context_id];
  assert_true(context.is_allocated());
  context.Release();
  context_bitmap_.Release(context_id);
}

bool XmaDecoder::BlockOnContext(uint32_t guest_ptr, bool poll) {
  auto context_id = GetContextId(guest_ptr);
  assert_true(context_id >= 0);

  XmaContext& context = contexts_[context_id];
  return context.Block(poll);
}

uint32_t XmaDecoder::ReadRegister(uint32_t addr) {
  auto r = (addr & 0xFFFF) / 4;

  assert_true(r < XmaRegisterFile::kRegisterCount);

  switch (r) {
    case XmaRegister::ContextArrayAddress:
      break;
    case XmaRegister::CurrentContextIndex: {
      // 0606h (1818h) is rotating context processing # set to hardware ID of
      // context being processed.
      // If bit 200h is set, the locking code will possibly collide on hardware
      // IDs and error out, so we should never set it (I think?).
      uint32_t& current_context_index = register_file_[XmaRegister::CurrentContextIndex];
      uint32_t& next_context_index = register_file_[XmaRegister::NextContextIndex];
      // To prevent games from seeing a stuck XMA context, return a rotating
      // number.
      current_context_index = next_context_index;
      next_context_index = (next_context_index + 1) % kContextCount;
      break;
    }
    default:
      const auto register_info = register_file_.GetRegisterInfo(r);
      if (register_info) {
        REXAPU_DEBUG("XMA: Read from unhandled register ({:04X}, {})", r, register_info->name);
      } else {
        REXAPU_DEBUG("XMA: Read from unknown register ({:04X})", r);
      }
      break;
  }

  return rex::byte_swap(register_file_[r]);
}

void XmaDecoder::WriteRegister(uint32_t addr, uint32_t value) {
  SCOPE_profile_cpu_f("apu");

  uint32_t r = (addr & 0xFFFF) / 4;
  value = rex::byte_swap(value);

  assert_true(r < XmaRegisterFile::kRegisterCount);
  register_file_[r] = value;

  if (r >= XmaRegister::Context0Kick && r <= XmaRegister::Context9Kick) {
    // Context kick command.
    // This will kick off the given hardware contexts.
    // Basically, this kicks the SPU and says "hey, decode that audio!"
    // XMAEnableContext

    // The context ID is a bit in the range of the entire context array.
    uint32_t base_context_id = (r - XmaRegister::Context0Kick) * 32;
    uint32_t kicked_value = value;
    for (int i = 0; value && i < 32; ++i, value >>= 1) {
      if (value & 1) {
        uint32_t context_id = base_context_id + i;
        auto& context = contexts_[context_id];
        context.Enable();
      }
    }
    // Decode inline. waiting on the worker sweep stalls the realtime
    // audio thread 50-100ms during kick bursts.
    for (int i = 0; kicked_value && i < 32; ++i, kicked_value >>= 1) {
      if (kicked_value & 1) {
        uint32_t context_id = base_context_id + i;
        auto& context = contexts_[context_id];
        if (context.Work()) {
          context.SignalWorkDone();
        }
      }
    }
    work_event_->Set();
  } else if (r >= XmaRegister::Context0Lock && r <= XmaRegister::Context9Lock) {
    // Context lock command.
    // This requests a lock by flagging the context.
    // XMADisableContext
    uint32_t base_context_id = (r - XmaRegister::Context0Lock) * 32;
    for (int i = 0; value && i < 32; ++i, value >>= 1) {
      if (value & 1) {
        uint32_t context_id = base_context_id + i;
        auto& context = contexts_[context_id];
        context.Disable();
        // [XMA fix] Added Block(false) after Disable(). Without this, the game
        // could call XMADisableContext and start modifying the context struct
        // while a decode was still in progress on the worker thread. Block()
        // waits for the context mutex to be free (poll=false means wait, not spin).
        context.Block(false);
      }
    }
    // Signal the decoder thread to start processing.
    // work_event_->Set();
  } else if (r >= XmaRegister::Context0Clear && r <= XmaRegister::Context9Clear) {
    // Context clear command.
    // This will reset the given hardware contexts.
    uint32_t base_context_id = (r - XmaRegister::Context0Clear) * 32;
    for (int i = 0; value && i < 32; ++i, value >>= 1) {
      if (value & 1) {
        uint32_t context_id = base_context_id + i;
        XmaContext& context = contexts_[context_id];
        context.Clear();
      }
    }
  } else {
    // 0601h (1804h) is written to with 0x02000000 and 0x03000000 around a lock
    // operation
    switch (r) {
      default: {
        const auto register_info = register_file_.GetRegisterInfo(r);
        if (register_info) {
          REXAPU_DEBUG("XMA: Write to unhandled register ({:04X}, {}): {:08X}", r,
                       register_info->name, value);
        } else {
          REXAPU_DEBUG("XMA: Write to unknown register ({:04X}): {:08X}", r, value);
        }
        break;
      }
#pragma warning(suppress : 4065)
    }
  }
}

void XmaDecoder::Pause() {
  if (paused_) {
    return;
  }
  paused_ = true;

  if (work_event_) {
    work_event_->Set();
  }
  pause_fence_.Wait();
}

void XmaDecoder::Resume() {
  if (!paused_) {
    return;
  }
  paused_ = false;

  resume_fence_.Signal();
}

XmaDecoder::DebugSnapshot XmaDecoder::GetDebugSnapshot() {
  DebugSnapshot snapshot{};
  snapshot.paused = paused_.load(std::memory_order_acquire);
  snapshot.vp.total_worker_time_us =
      debug_vp_total_worker_time_us_.load(std::memory_order_relaxed);
  snapshot.vp.sweeps_per_second = debug_vp_sweeps_per_second_.load(std::memory_order_relaxed);
  snapshot.vp.decode_iterations_per_second =
      debug_vp_decode_iterations_per_second_.load(std::memory_order_relaxed);
  snapshot.vp.workers[0].num_voices = debug_vp_worker_voices_.load(std::memory_order_relaxed);
  snapshot.vp.workers[0].time_us = snapshot.vp.total_worker_time_us;
  snapshot.gp.cycles = debug_gp_cycles_estimate_.load(std::memory_order_relaxed);
  snapshot.ep.cycles = debug_ep_cycles_estimate_.load(std::memory_order_relaxed);

  for (uint32_t i = 0; i < kContextCount; ++i) {
    auto& out = snapshot.contexts[i];
    auto& context = contexts_[i];

    out.allocated = context.is_allocated();
    out.enabled = context.is_enabled();
    out.guest_ptr = context.guest_ptr();

    if (!out.allocated || out.guest_ptr == 0) {
      continue;
    }

    auto* context_ptr = memory()->TranslateVirtual(out.guest_ptr);
    if (!context_ptr) {
      continue;
    }

    XMA_CONTEXT_DATA data(context_ptr);
    out.input0_valid = data.input_buffer_0_valid != 0;
    out.input1_valid = data.input_buffer_1_valid != 0;
    out.output_valid = data.output_buffer_valid != 0;
    out.stop_when_done = data.stop_when_done != 0;
    out.interrupt_when_done = data.interrupt_when_done != 0;
    out.consume_only = data.IsConsumeOnlyContext();
    out.stereo = data.is_stereo != 0;
    out.muted = context.is_muted();
    out.volume = context.volume();
    out.peak_level = context.last_peak_level();
    out.rms_level = context.last_rms_level();
    out.rms_ch0_level = context.last_rms_ch0_level();
    out.rms_ch1_level = context.last_rms_ch1_level();
    out.audible_peak_level = context.last_audible_peak_level();
    out.audible_rms_level = context.last_audible_rms_level();
    out.audible_rms_ch0_level = context.last_audible_rms_ch0_level();
    out.audible_rms_ch1_level = context.last_audible_rms_ch1_level();
    out.current_buffer = static_cast<uint8_t>(data.current_buffer);
    out.subframe_decode_count = static_cast<uint8_t>(data.subframe_decode_count);
    out.output_buffer_block_count = static_cast<uint8_t>(data.output_buffer_block_count);
    out.output_buffer_write_offset = static_cast<uint8_t>(data.output_buffer_write_offset);
    out.output_buffer_read_offset = static_cast<uint8_t>(data.output_buffer_read_offset);
    out.sample_rate_id = static_cast<uint8_t>(data.sample_rate);
    out.loop_count = static_cast<uint8_t>(data.loop_count);
    out.output_buffer_padding = static_cast<uint8_t>(data.output_buffer_padding);
    out.loop_subframe_end = static_cast<uint8_t>(data.loop_subframe_end);
    out.loop_subframe_skip = static_cast<uint8_t>(data.loop_subframe_skip);
    out.packet_metadata = static_cast<uint8_t>(data.packet_metadata);
    out.input_buffer_0_packet_count = static_cast<uint16_t>(data.input_buffer_0_packet_count);
    out.input_buffer_1_packet_count = static_cast<uint16_t>(data.input_buffer_1_packet_count);
    out.input_buffer_read_offset = data.input_buffer_read_offset;
    out.input_buffer_0_ptr = data.input_buffer_0_ptr;
    out.input_buffer_1_ptr = data.input_buffer_1_ptr;
    out.output_buffer_ptr = data.output_buffer_ptr;
  }

  return snapshot;
}

void XmaDecoder::ToggleContextMute(uint32_t context_id) {
  if (context_id >= kContextCount) {
    return;
  }
  auto& context = contexts_[context_id];
  if (!context.is_allocated()) {
    return;
  }
  context.ToggleMuted();
}

void XmaDecoder::SetContextMuted(uint32_t context_id, bool muted) {
  if (context_id >= kContextCount) {
    return;
  }
  auto& context = contexts_[context_id];
  context.SetMuted(muted);
}

void XmaDecoder::SetContextVolume(uint32_t context_id, float volume) {
  if (context_id >= kContextCount) {
    return;
  }
  auto& context = contexts_[context_id];
  if (!context.is_allocated()) {
    return;
  }
  context.SetVolume(volume);
}

}  // namespace rex::audio
