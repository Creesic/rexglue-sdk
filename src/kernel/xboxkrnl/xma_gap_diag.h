/**
 * TEMP_DIAG: FMOD stream event wait/signal diagnostics
 *
 * Replaces previous single-thread PC sampler with multi-thread tracking.
 *
 * TASK 1: Multi-thread tracking (replaces single g_xma_native_tid)
 * TASK 2: NtWaitForSingleObject instrumentation for FMOD stream events
 * TASK 3: NtSetEvent instrumentation for signal site
 * TASK 4: codec_read throughput tracking
 * TASK 5: Root-cause determination
 *
 * All logging uses REXKRNL_ERROR to bypass log_level filters.
 * Remove this entire file and all #include "xma_gap_diag.h" when done.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>

#include <rex/logging.h>
#include <rex/system/xthread.h>
#include <rex/runtime.h>

// Forward declaration from audio_system.cpp
extern uint32_t audio_diag_get_render_frame();

// TEMP_DIAG V2: Per-second diagnostics + write-watch
#include "xma_gap_diag_v2.h"
// END TEMP_DIAG V2

namespace xma_gap_diag {

// ============================================================================
// TASK 1: Multi-thread tracking
// ============================================================================

// Set of native thread IDs that have called XMA APIs (Enable/Disable/SetOutput etc.)
inline std::mutex g_tracked_threads_mutex;
inline std::unordered_set<uint32_t> g_tracked_native_tids;
inline std::unordered_set<uint32_t> g_tracked_guest_tids;

// Track a thread as an XMA/FMOD stream thread. Called from XMA API entry points.
inline void TrackXmaThread() {
  uint32_t native_tid = rex::system::XThread::GetCurrentThreadId();
  uint32_t guest_tid = 0;
  auto* thread = rex::system::XThread::GetCurrentThread();
  if (thread) guest_tid = thread->thread_id();

  bool is_new = false;
  {
    std::lock_guard<std::mutex> lock(g_tracked_threads_mutex);
    is_new = g_tracked_native_tids.insert(native_tid).second;
    if (guest_tid) g_tracked_guest_tids.insert(guest_tid);
  }
  if (is_new) {
    REXKRNL_ERROR("FMOD_DIAG: new XMA thread registered native_tid={} guest_tid={}",
                  native_tid, guest_tid);
  }
}

// Check if the current thread is a tracked XMA/FMOD stream thread
inline bool IsXmaThread() {
  uint32_t native_tid = rex::system::XThread::GetCurrentThreadId();
  std::lock_guard<std::mutex> lock(g_tracked_threads_mutex);
  return g_tracked_native_tids.count(native_tid) > 0;
}

// Check if a specific native TID is a tracked XMA thread
inline bool IsXmaThreadByNativeTid(uint32_t native_tid) {
  std::lock_guard<std::mutex> lock(g_tracked_threads_mutex);
  return g_tracked_native_tids.count(native_tid) > 0;
}

// ============================================================================
// Utility
// ============================================================================

inline double WallMs() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::chrono::milliseconds::period>(
             now.time_since_epoch())
      .count();
}

inline uint64_t GetGuestLR() {
  auto* ctx = rex::runtime::current_ppc_context();
  return ctx ? ctx->lr : 0;
}

inline uint32_t GetGuestTID() {
  auto* thread = rex::system::XThread::GetCurrentThread();
  return thread ? thread->thread_id() : 0;
}

inline uint32_t GetNativeTID() {
  return rex::system::XThread::GetCurrentThreadId();
}

// ============================================================================
// TASK 2: FMOD stream event wait instrumentation
// ============================================================================

// Tracked FMOD stream event handles (set when we see NtWaitForSingleObjectEx
// from a tracked XMA thread)
inline std::mutex g_stream_events_mutex;
inline std::unordered_set<uint32_t> g_stream_event_handles;

// Cumulative wait statistics
inline std::atomic<uint32_t> g_wait_total{0};
inline std::atomic<uint32_t> g_wait_signal{0};    // STATUS_SUCCESS (0x00000000)
inline std::atomic<uint32_t> g_wait_timeout{0};   // STATUS_TIMEOUT (0x00000102)
inline std::atomic<uint32_t> g_wait_other{0};
inline std::atomic<double> g_wait_signal_sum_ms{0.0};
inline std::atomic<double> g_wait_timeout_sum_ms{0.0};

// Per-wait logging (every wait for first 5 seconds, then sample)
inline std::atomic<double> g_diag_start_ms{0.0};
inline constexpr double kDiagDurationSec = 5.0;  // Log every event for 5 seconds

inline bool ShouldLogVerbose() {
  return false;
}

// Called from NtWaitForSingleObjectEx_entry when a tracked XMA thread waits
inline void LogStreamWaitEnter(uint32_t handle, uint32_t wait_mode, uint32_t alertable,
                                int64_t timeout_ticks) {
  // Register this handle as a stream event
  {
    std::lock_guard<std::mutex> lock(g_stream_events_mutex);
    g_stream_event_handles.insert(handle);
  }

  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  uint32_t ntid = GetNativeTID();
  uint64_t lr = GetGuestLR();

  // Convert timeout ticks to ms for display
  int32_t timeout_ms = 0;
  bool is_infinite = (timeout_ticks == 0);
  if (timeout_ticks < 0) {
    timeout_ms = static_cast<int32_t>(-timeout_ticks / 10000);
  }

  if (ShouldLogVerbose()) {
    REXKRNL_ERROR("FMOD_WAIT ENTER rf={} nt{} gt{} handle={:08X} mode={} alertable={} "
                  "timeout_ms={} infinite={} lr={:08X}",
                  rf, ntid, gtid, handle, wait_mode, alertable,
                  timeout_ms, is_infinite, (uint32_t)lr);
  }
}

// Called from NtWaitForSingleObjectEx_entry after wait returns
inline void LogStreamWaitExit(uint32_t handle, uint32_t result, double elapsed_ms) {
  g_wait_total.fetch_add(1, std::memory_order_relaxed);

  // Categorize result
  bool is_signal = (result == 0x00000000);     // STATUS_SUCCESS
  bool is_timeout = (result == 0x00000102);     // STATUS_TIMEOUT

  if (is_signal) {
    g_wait_signal.fetch_add(1, std::memory_order_relaxed);
    double old_sum = g_wait_signal_sum_ms.load(std::memory_order_relaxed);
    g_wait_signal_sum_ms.store(old_sum + elapsed_ms, std::memory_order_relaxed);
  } else if (is_timeout) {
    g_wait_timeout.fetch_add(1, std::memory_order_relaxed);
    double old_sum = g_wait_timeout_sum_ms.load(std::memory_order_relaxed);
    g_wait_timeout_sum_ms.store(old_sum + elapsed_ms, std::memory_order_relaxed);
  } else {
    g_wait_other.fetch_add(1, std::memory_order_relaxed);
  }

  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  uint32_t ntid = GetNativeTID();

  if (ShouldLogVerbose()) {
    const char* wake_type = is_signal ? "SIGNAL" : (is_timeout ? "TIMEOUT" : "OTHER");
    REXKRNL_ERROR("FMOD_WAIT EXIT  rf={} nt{} gt{} handle={:08X} result={:08X} "
                  "elapsed_ms={:.2f} wake={}",
                  rf, ntid, gtid, handle, result, elapsed_ms, wake_type);
  }

  // Periodic summary (every 100 waits)
  uint32_t total = g_wait_total.load(std::memory_order_relaxed);
  if (total % 100 == 0) {
    uint32_t signals = g_wait_signal.load(std::memory_order_relaxed);
    uint32_t timeouts = g_wait_timeout.load(std::memory_order_relaxed);
    double sig_sum = g_wait_signal_sum_ms.load(std::memory_order_relaxed);
    double to_sum = g_wait_timeout_sum_ms.load(std::memory_order_relaxed);
    double avg_sig = signals > 0 ? sig_sum / signals : 0.0;
    double avg_to = timeouts > 0 ? to_sum / timeouts : 0.0;
    REXKRNL_ERROR("FMOD_WAIT SUMMARY total={} signal={} timeout={} other={} "
                  "avg_signal_ms={:.2f} avg_timeout_ms={:.2f}",
                  total, signals, timeouts,
                  g_wait_other.load(std::memory_order_relaxed),
                  avg_sig, avg_to);
  }
}

// Check if a handle is a known FMOD stream event
inline bool IsStreamEventHandle(uint32_t handle) {
  std::lock_guard<std::mutex> lock(g_stream_events_mutex);
  return g_stream_event_handles.count(handle) > 0;
}

// ============================================================================
// TASK 3: Signal site instrumentation (NtSetEvent)
// ============================================================================

// Cumulative signal statistics
inline std::atomic<uint32_t> g_signal_total{0};

inline void LogStreamSignal(uint32_t handle) {
  // Only log known stream-event handles. Treating all XMA-thread signals as
  // stream activity creates massive non-stream noise and can distort timing.
  bool is_stream_event = IsStreamEventHandle(handle);
  if (!is_stream_event) return;

  g_signal_total.fetch_add(1, std::memory_order_relaxed);

  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  uint32_t ntid = GetNativeTID();
  uint64_t lr = GetGuestLR();

  if (ShouldLogVerbose()) {
    REXKRNL_ERROR("FMOD_SIGNAL rf={} nt{} gt{} handle={:08X} lr={:08X} "
                  "is_stream_event={} is_xma_thread={}",
                  rf, ntid, gtid, handle, (uint32_t)lr,
                  is_stream_event, false);
  }

  // Periodic signal summary (every 100 signals)
  uint32_t total = g_signal_total.load(std::memory_order_relaxed);
  if (total % 100 == 0) {
    REXKRNL_ERROR("FMOD_SIGNAL SUMMARY total_signals={}", total);
  }
}

// ============================================================================
// TASK 4: codec_read throughput tracking
// ============================================================================

// codec_read is sub_826938E8 in the XEX. It's called via wrapper sub_82693B18.
// The generated code calls __imp__sub_826938E8 which maps to a generated function.
// We instrument at the XMA level: XMADisableContext marks the start of a decode,
// and the codec_read wrapper returns decoded data.
//
// Since we can't directly instrument the generated codec_read function from C++,
// we track throughput by measuring:
// 1. XMA context state at Enable/Disable (output buffer offsets)
// 2. Time between consecutive XMADisableContext calls per context
// 3. Output bytes produced per decode cycle

// ============================================================================
// TASK 5: Final root-cause determination — printed at shutdown
// ============================================================================

// Forward declaration — defined later in this file
inline void PrintTask5Diagnosis();

inline void PrintFinalDiagnosis() {
  // Delegate to the comprehensive TASK 5 diagnosis
  PrintTask5Diagnosis();
}

// ============================================================================
// Legacy gap boundary markers (kept for continuity, simplified)
// ============================================================================

// Gap state for boundary tracking
inline std::atomic<bool> g_in_gap{false};
inline std::atomic<uint32_t> g_gap_counter{0};

inline void LogXmaEnter(const char* api, uint32_t ctx_addr) {
  TrackXmaThread();

  // If we were in a gap, this entry ends it
  bool was_in_gap = g_in_gap.exchange(false, std::memory_order_relaxed);

  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  uint32_t ntid = GetNativeTID();
  uint64_t lr = GetGuestLR();
  double ms = WallMs();

  if (was_in_gap) {
    uint32_t gap_id = g_gap_counter.load(std::memory_order_relaxed);
    REXKRNL_ERROR("GAP_BOUNDARY === GAP END #{} rf={} nt{} gt{} {} ctx={:08X} "
                  "lr={:08X} ms={:.3f} ===",
                  gap_id, rf, ntid, gtid, api, ctx_addr, (uint32_t)lr, ms);
  }

  REXKRNL_ERROR("GAP_XMA ENTER rf={} nt{} gt{} {} ctx={:08X} lr={:08X} ms={:.3f}",
                rf, ntid, gtid, api, ctx_addr, (uint32_t)lr, ms);
}

inline void LogXmaExit(const char* api, uint32_t ctx_addr) {
  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  uint32_t ntid = GetNativeTID();
  double ms = WallMs();

  REXKRNL_ERROR("GAP_XMA EXIT  rf={} nt{} gt{} {} ctx={:08X} ms={:.3f}",
                rf, ntid, gtid, api, ctx_addr, ms);

  // If this is EnableContext exiting, we're entering the gap
  if (strcmp(api, "XMAEnableContext") == 0) {
    uint32_t gap_id = g_gap_counter.fetch_add(1, std::memory_order_relaxed) + 1;
    REXKRNL_ERROR("GAP_BOUNDARY === GAP START #{} rf={} ms={:.3f} ===",
                  gap_id, rf, ms);
    g_in_gap.store(true, std::memory_order_relaxed);
  }
}

// ============================================================================
// Bridge accessors for clock.cpp
// ============================================================================

inline bool gap_active_impl() {
  return g_in_gap.load(std::memory_order_relaxed);
}

inline uint32_t get_native_tid_impl() {
  // Return first tracked thread (for compatibility with clock.cpp bridge)
  std::lock_guard<std::mutex> lock(g_tracked_threads_mutex);
  if (!g_tracked_native_tids.empty()) return *g_tracked_native_tids.begin();
  return 0;
}

inline void log_timebase_impl(uint64_t value) {
  // Minimal — no longer the primary diagnostic path
}

// Legacy compatibility — no-ops for old code paths
inline double LogWaitEnter(const char* api, uint32_t obj_guest_addr,
                           uint32_t alertable, bool is_infinite, int64_t timeout_val) {
  return 0.0;  // Handled by new TASK 2 instrumentation
}
inline void LogWaitExit(const char* api, uint32_t obj_guest_addr,
                        uint32_t result, double enter_ms) {
  // Handled by new TASK 2 instrumentation
}
inline void LogSignal(const char* api, uint32_t obj_guest_addr, uint32_t value = 0) {
  // Handled by new TASK 3 instrumentation
}
inline double LogMultiWaitEnter(const char* api, uint32_t count,
                                uint32_t* obj_addrs, uint32_t alertable,
                                bool is_infinite) {
  if (!IsXmaThread()) return 0.0;
  REXKRNL_ERROR("FMOD_MWAIT ENTER count={}", count);
  return WallMs();
}
inline void LogMultiWaitExit(const char* api, uint32_t count,
                             uint32_t* obj_addrs, uint32_t result,
                             double enter_ms) {
  if (!IsXmaThread()) return;
  double elapsed = enter_ms > 0 ? (WallMs() - enter_ms) : 0.0;
  REXKRNL_ERROR("FMOD_MWAIT EXIT  result={:08X} elapsed_ms={:.1f}", result, elapsed);
}
inline double LogDelayEnter(const char* api, uint64_t interval) {
  if (!IsXmaThread()) return 0.0;
  if (ShouldLogVerbose()) {
    REXKRNL_ERROR("FMOD_DELAY ENTER {} interval={} lr={:08X} nt{} gt{}",
                  api, (int64_t)interval, (uint32_t)GetGuestLR(),
                  GetNativeTID(), GetGuestTID());
  }
  return WallMs();
}
inline void LogDelayExit(const char* api, uint32_t result, double enter_ms) {
  if (!IsXmaThread()) return;
  double elapsed = enter_ms > 0 ? (WallMs() - enter_ms) : 0.0;
  if (ShouldLogVerbose()) {
    REXKRNL_ERROR("FMOD_DELAY EXIT  {} result={:08X} elapsed_ms={:.1f}", api, result, elapsed);
  }
}
inline void LogTimerRead(const char* api, uint64_t returned_value) {
  // No longer primary diagnostic — kept for compatibility
}

// ============================================================================
// TASK 1: FMOD Configuration Logging
// ============================================================================

struct FmodConfig {
  // XMA context params
  uint32_t sample_rate_raw = 0;     // raw 2-bit enum from XMA_CONTEXT_INIT
  uint32_t sample_rate_hz = 0;      // decoded Hz
  uint32_t channel_count = 0;       // 0=mono, 1=stereo
  uint32_t subframe_decode_count = 0;
  uint32_t output_buffer_block_count = 0;
  uint32_t output_buffer_ptr = 0;
  uint32_t input_buffer_0_ptr = 0;
  uint32_t input_buffer_1_ptr = 0;
  uint32_t input_buffer_0_packet_count = 0;
  uint32_t input_buffer_1_packet_count = 0;
  // Derived
  uint32_t dsp_buffer_samples = 0;  // FMOD DSP buffer in samples
  uint32_t dsp_buffer_count = 0;    // number of DSP buffers
  uint32_t stream_buffer_size = 0;  // FMOD stream buffer in bytes
  uint32_t decode_buffer_size = 0;  // XMA decode buffer in bytes
  // Timing
  double expected_decode_hz = 0.0;  // expected decode rate based on config
};

inline FmodConfig g_fmod_config;
inline std::atomic<bool> g_fmod_config_logged{false};

// Called from XMAInitializeContext to capture FMOD codec configuration
inline void LogFmodConfig(uint32_t sample_rate_raw, uint32_t channel_count,
                          uint32_t subframe_decode_count, uint32_t output_buffer_block_count,
                          uint32_t output_buffer_ptr,
                          uint32_t input_buffer_0_ptr, uint32_t input_buffer_0_packet_count,
                          uint32_t input_buffer_1_ptr, uint32_t input_buffer_1_packet_count) {
  auto& c = g_fmod_config;
  c.sample_rate_raw = sample_rate_raw;
  c.channel_count = channel_count;
  c.subframe_decode_count = subframe_decode_count;
  c.output_buffer_block_count = output_buffer_block_count;
  c.output_buffer_ptr = output_buffer_ptr;
  c.input_buffer_0_ptr = input_buffer_0_ptr;
  c.input_buffer_0_packet_count = input_buffer_0_packet_count;
  c.input_buffer_1_ptr = input_buffer_1_ptr;
  c.input_buffer_1_packet_count = input_buffer_1_packet_count;

  // Decode sample rate from XMA enum
  // XMA_CONTEXT_DATA sample_rate field: 0=24000, 1=32000, 2=44100, 3=48000
  switch (sample_rate_raw & 0x3) {
    case 0: c.sample_rate_hz = 24000; break;
    case 1: c.sample_rate_hz = 32000; break;
    case 2: c.sample_rate_hz = 44100; break;
    case 3: c.sample_rate_hz = 48000; break;
    default: c.sample_rate_hz = 0; break;
  }

  // XMA ring buffer: output_buffer_block_count * 256 bytes per block
  c.decode_buffer_size = output_buffer_block_count * 256;

  // Expected decode rate: sample_rate / samples_per_decode
  // XMA2 produces 512 samples per decode frame
  if (c.sample_rate_hz > 0) {
    c.expected_decode_hz = (double)c.sample_rate_hz / 512.0;
  }

  REXKRNL_ERROR("FMOD_CONFIG: sr_raw={} sr_hz={} ch={} sdc={} obc={} "
                "decode_buf_bytes={} expected_decode_hz={:.2f} "
                "obuf={:08X} ib0={:08X}[{}] ib1={:08X}[{}]",
                c.sample_rate_raw, c.sample_rate_hz, c.channel_count,
                c.subframe_decode_count, c.output_buffer_block_count,
                c.decode_buffer_size, c.expected_decode_hz,
                c.output_buffer_ptr, c.input_buffer_0_ptr, c.input_buffer_0_packet_count,
                c.input_buffer_1_ptr, c.input_buffer_1_packet_count);

  // Key math for 30 Hz investigation
  if (c.sample_rate_hz > 0) {
    REXKRNL_ERROR("FMOD_CONFIG_MATH: 48000/512={:.2f} 48000/1024={:.2f} "
                  "48000/1536={:.2f} 48000/1600={:.2f} 48000/2048={:.2f}",
                  48000.0/512.0, 48000.0/1024.0, 48000.0/1536.0,
                  48000.0/1600.0, 48000.0/2048.0);
    REXKRNL_ERROR("FMOD_CONFIG_MATH: sr_hz/512={:.2f} sr_hz/1024={:.2f} "
                  "sr_hz/1536={:.2f} sr_hz/1600={:.2f}",
                  (double)c.sample_rate_hz/512.0, (double)c.sample_rate_hz/1024.0,
                  (double)c.sample_rate_hz/1536.0, (double)c.sample_rate_hz/1600.0);
  }

  g_fmod_config_logged.store(true, std::memory_order_relaxed);
}

// ============================================================================
// TASK 2: codec_read Unit Tracking
// ============================================================================

struct CodecReadStats {
  std::atomic<uint32_t> total_calls{0};
  std::atomic<uint32_t> total_pcm_bytes{0};
  std::atomic<uint32_t> total_pcm_frames{0};
  std::atomic<uint32_t> total_compressed_bytes{0};
  std::atomic<double> first_call_ms{0.0};
  std::atomic<double> last_call_ms{0.0};

  // Per-context tracking
  struct PerCtx {
    std::atomic<uint32_t> last_write_offset{0};
    std::atomic<uint32_t> last_read_offset{0};
    std::atomic<uint32_t> last_input_read_offset{0};
    std::atomic<double> last_call_ms{0.0};
    std::atomic<uint32_t> call_count{0};
    std::atomic<uint32_t> total_bytes_produced{0};
  };
  inline static std::unordered_map<uint32_t, PerCtx> g_per_ctx;
  inline static std::mutex g_per_ctx_mutex;
};

inline CodecReadStats g_codec_read_stats;

// Called at XMADisableContext (start of decode cycle)
// Logs the pre-decode state and computes what codec_read will process
inline void LogCodecReadStart(uint32_t ctx_addr, uint32_t write_offset,
                               uint32_t read_offset, uint32_t input_read_offset,
                               uint32_t input_buffer_0_valid, uint32_t input_buffer_1_valid,
                               uint32_t input_buffer_0_packet_count,
                               uint32_t input_buffer_1_packet_count) {
  uint32_t ntid = GetNativeTID();
  uint32_t gtid = GetGuestTID();
  uint32_t rf = audio_diag_get_render_frame();
  double now = WallMs();

  CodecReadStats::PerCtx* per_ctx = nullptr;
  {
    std::lock_guard<std::mutex> lock(CodecReadStats::g_per_ctx_mutex);
    per_ctx = &CodecReadStats::g_per_ctx[ctx_addr];
  }

  uint32_t prev_write = per_ctx->last_write_offset.exchange(write_offset, std::memory_order_relaxed);
  uint32_t prev_read = per_ctx->last_read_offset.exchange(read_offset, std::memory_order_relaxed);
  uint32_t prev_inp_read = per_ctx->last_input_read_offset.exchange(input_read_offset, std::memory_order_relaxed);
  double prev_ms = per_ctx->last_call_ms.exchange(now, std::memory_order_relaxed);
  uint32_t call_num = per_ctx->call_count.fetch_add(1, std::memory_order_relaxed) + 1;

  // Compute output bytes produced (ring buffer wrap)
  uint32_t output_bytes = 0;
  if (write_offset >= prev_write) {
    output_bytes = write_offset - prev_write;
  } else {
    output_bytes = (2048 - prev_write) + write_offset;
  }

  // Compute compressed bytes consumed
  uint32_t compressed_bytes = 0;
  if (input_read_offset >= prev_inp_read) {
    compressed_bytes = input_read_offset - prev_inp_read;
  } else {
    // Input ring buffer wrap (typically 0x800 bytes)
    compressed_bytes = (0x800 - prev_inp_read) + input_read_offset;
  }

  double delta_ms = now - prev_ms;

  // PCM frames = output_bytes / (channels * 2 bytes per sample)
  // For stereo 16-bit: 4 bytes per frame
  uint32_t pcm_frames = output_bytes / 4;  // assumes stereo 16-bit

  if (prev_ms > 0.0 && output_bytes > 0) {
    g_codec_read_stats.total_calls.fetch_add(1, std::memory_order_relaxed);
    g_codec_read_stats.total_pcm_bytes.fetch_add(output_bytes, std::memory_order_relaxed);
    g_codec_read_stats.total_pcm_frames.fetch_add(pcm_frames, std::memory_order_relaxed);
    g_codec_read_stats.total_compressed_bytes.fetch_add(compressed_bytes, std::memory_order_relaxed);
    g_codec_read_stats.last_call_ms.store(now, std::memory_order_relaxed);
    per_ctx->total_bytes_produced.fetch_add(output_bytes, std::memory_order_relaxed);
  } else if (g_codec_read_stats.first_call_ms.load() == 0.0) {
    g_codec_read_stats.first_call_ms.store(now, std::memory_order_relaxed);
  }

  // Verbose per-decode logging (first 5s, then sample)
  if (ShouldLogVerbose()) {
    REXKRNL_ERROR("FMOD_CODEC_READ START rf={} nt{} gt{} ctx={:08X} "
                  "woff={} (prev={}) roff={} (prev={}) "
                  "inp_off={} (prev={}) "
                  "out_bytes={} pcm_frames={} comp_bytes={} "
                  "ib0v={} ib1v={} ib0pc={} ib1pc={} "
                  "delta_ms={:.2f} call_num={}",
                  rf, ntid, gtid, ctx_addr,
                  write_offset, prev_write, read_offset, prev_read,
                  input_read_offset, prev_inp_read,
                  output_bytes, pcm_frames, compressed_bytes,
                  input_buffer_0_valid, input_buffer_1_valid,
                  input_buffer_0_packet_count, input_buffer_1_packet_count,
                  delta_ms, call_num);
  }

  // Periodic summary (every 100 calls)
  uint32_t total = g_codec_read_stats.total_calls.load(std::memory_order_relaxed);
  if (total > 0 && total % 100 == 0) {
    double first_ms = g_codec_read_stats.first_call_ms.load(std::memory_order_relaxed);
    double last_ms = g_codec_read_stats.last_call_ms.load(std::memory_order_relaxed);
    double elapsed_sec = (last_ms - first_ms) / 1000.0;
    uint64_t total_bytes = g_codec_read_stats.total_pcm_bytes.load(std::memory_order_relaxed);
    uint64_t total_frames = g_codec_read_stats.total_pcm_frames.load(std::memory_order_relaxed);
    uint64_t total_comp = g_codec_read_stats.total_compressed_bytes.load(std::memory_order_relaxed);

    double calls_per_sec = elapsed_sec > 0 ? total / elapsed_sec : 0;
    double frames_per_sec = elapsed_sec > 0 ? (double)total_frames / elapsed_sec : 0;
    double bytes_per_call = total > 0 ? (double)total_bytes / total : 0;
    double avg_delta_ms = total > 0 ? (elapsed_sec * 1000.0) / total : 0;

    REXKRNL_ERROR("FMOD_CODEC_READ SUMMARY calls={} bytes={} frames={} comp_bytes={} "
                  "avg_bytes/call={:.0f} avg_delta_ms={:.2f} "
                  "calls/sec={:.1f} frames/sec={:.0f} "
                  "expected_frames/sec={:.0f} ratio={:.2f}",
                  total, total_bytes, total_frames, total_comp,
                  bytes_per_call, avg_delta_ms,
                  calls_per_sec, frames_per_sec,
                  g_fmod_config.sample_rate_hz > 0 ? (double)g_fmod_config.sample_rate_hz : 48000.0,
                  frames_per_sec / (g_fmod_config.sample_rate_hz > 0 ? (double)g_fmod_config.sample_rate_hz : 48000.0));
  }
}

inline void LogCodecReadFinish(uint32_t ctx_addr, uint32_t pre_write_offset,
                               uint32_t pre_read_offset,
                               uint32_t pre_input_read_offset,
                               uint32_t post_write_offset,
                               uint32_t post_read_offset,
                               uint32_t post_input_read_offset,
                               uint32_t post_input_buffer_0_valid,
                               uint32_t post_input_buffer_1_valid,
                               bool block_success, uint32_t wait_arg,
                               uint32_t produced_bytes) {
  uint32_t ntid = GetNativeTID();
  uint32_t gtid = GetGuestTID();
  uint32_t rf = audio_diag_get_render_frame();

  uint32_t compressed_bytes = 0;
  if (post_input_read_offset >= pre_input_read_offset) {
    compressed_bytes = post_input_read_offset - pre_input_read_offset;
  } else {
    compressed_bytes = (0x800 - pre_input_read_offset) + post_input_read_offset;
  }

  uint32_t pcm_frames = produced_bytes / 4;

  if (block_success) {
    xma_gap_diag_v2::IncXmaDecode();
    if (pcm_frames > 0) {
      xma_gap_diag_v2::AddPcmFrames(pcm_frames);
    }
  }

  if (ShouldLogVerbose()) {
    REXKRNL_ERROR(
        "FMOD_CODEC_READ FINISH rf={} nt{} gt{} ctx={:08X} wait={} "
        "ok={} woff {}->{} roff {}->{} inp {}->{} out_bytes={} "
        "pcm_frames={} comp_bytes={} ib0v={} ib1v={}",
        rf, ntid, gtid, ctx_addr, wait_arg, block_success, pre_write_offset,
        post_write_offset, pre_read_offset, post_read_offset,
        pre_input_read_offset, post_input_read_offset, produced_bytes, pcm_frames,
        compressed_bytes, post_input_buffer_0_valid, post_input_buffer_1_valid);
  }
}

// ============================================================================
// TASK 3: Queue-Work Decision Tracing
// ============================================================================

// Called from NtSetEvent to identify stream event signals and their callers
inline void LogQueueWorkSignal(uint32_t handle, uint64_t guest_lr, uint32_t thread_id) {
  // Only log known stream-event handles. Non-stream XMA-thread signaling can be
  // extremely high frequency and is not useful for stream cadence diagnosis.
  bool is_stream_event = IsStreamEventHandle(handle);
  if (!is_stream_event) return;

  g_signal_total.fetch_add(1, std::memory_order_relaxed);

  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  uint32_t ntid = GetNativeTID();
  double now = WallMs();

  // Track per-handle signal rate
  static std::mutex g_handle_rate_mutex;
  static std::unordered_map<uint32_t, double> g_handle_last_signal_ms;
  double last_sig_ms = 0.0;
  {
    std::lock_guard<std::mutex> lock(g_handle_rate_mutex);
    last_sig_ms = g_handle_last_signal_ms[handle];
    g_handle_last_signal_ms[handle] = now;
  }
  double sig_delta_ms = last_sig_ms > 0 ? now - last_sig_ms : 0.0;

  if (ShouldLogVerbose()) {
    REXKRNL_ERROR("FMOD_QUEUE_SIGNAL rf={} nt{} gt{} handle={:08X} "
                  "lr={:08X} is_stream={} is_xma={} "
                  "delta_ms={:.2f} total_signals={}",
                  rf, ntid, gtid, handle,
                  (uint32_t)guest_lr, is_stream_event, false,
                  sig_delta_ms, g_signal_total.load(std::memory_order_relaxed));
  }

  // Periodic summary
  uint32_t total = g_signal_total.load(std::memory_order_relaxed);
  if (total % 100 == 0) {
    REXKRNL_ERROR("FMOD_QUEUE_SIGNAL SUMMARY total={}", total);
  }
}

// ============================================================================
// TASK 4: Timebase Read Tracking
// ============================================================================

struct TimebaseStats {
  std::atomic<uint32_t> total_reads{0};
  std::atomic<double> first_read_ms{0.0};
  std::atomic<double> last_read_ms{0.0};
  std::atomic<uint64_t> last_value{0};
  std::atomic<double> last_read_wall_ms{0.0};
};

inline TimebaseStats g_timebase_stats;

// Called from clock.cpp when QueryGuestTickCount is called during a gap
inline void LogTimebaseRead(uint64_t guest_tick_value) {
  double now = WallMs();
  uint64_t prev_value = g_timebase_stats.last_value.exchange(guest_tick_value, std::memory_order_relaxed);
  double prev_wall = g_timebase_stats.last_read_wall_ms.exchange(now, std::memory_order_relaxed);
  uint32_t count = g_timebase_stats.total_reads.fetch_add(1, std::memory_order_relaxed) + 1;

  if (g_timebase_stats.first_read_ms.load() == 0.0) {
    g_timebase_stats.first_read_ms.store(now, std::memory_order_relaxed);
  }
  g_timebase_stats.last_read_ms.store(now, std::memory_order_relaxed);

  // Compute deltas
  uint64_t guest_delta = guest_tick_value - prev_value;
  double wall_delta_ms = now - prev_wall;

  // Convert guest ticks to time (assuming 10 MHz Xbox 360 timebase)
  double guest_delta_ms = guest_delta / 10000.0;  // 10 MHz = 10000 ticks per ms

  // Only log during gaps (when g_in_gap is true) to avoid spam
  if (g_in_gap.load(std::memory_order_relaxed)) {
    if (count <= 50 || count % 50 == 0) {
      REXKRNL_ERROR("FMOD_TIMEBASE gap_read #{} guest_tick={} guest_delta={} "
                    "guest_delta_ms={:.2f} wall_delta_ms={:.2f} "
                    "ratio={:.2f}",
                    count, guest_tick_value, guest_delta,
                    guest_delta_ms, wall_delta_ms,
                    wall_delta_ms > 0 ? guest_delta_ms / wall_delta_ms : 0.0);
    }
  } else {
    // Outside gap: only log periodically
    if (count % 500 == 0) {
      REXKRNL_ERROR("FMOD_TIMEBASE read #{} guest_tick={} guest_delta={} "
                    "guest_delta_ms={:.2f} wall_delta_ms={:.2f}",
                    count, guest_tick_value, guest_delta,
                    guest_delta_ms, wall_delta_ms);
    }
  }
}

// ============================================================================
// TASK 5: Enhanced Final Diagnosis
// ============================================================================

inline void PrintTask5Diagnosis() {
  REXKRNL_ERROR("========================================");
  REXKRNL_ERROR("FMOD TASK 5: ROOT CAUSE ANALYSIS");
  REXKRNL_ERROR("========================================");

  // A. FMOD config table
  auto& c = g_fmod_config;
  REXKRNL_ERROR("A. FMOD Config Table:");
  REXKRNL_ERROR("   software_rate_raw={} decoded_hz={}", c.sample_rate_raw, c.sample_rate_hz);
  REXKRNL_ERROR("   channel_count={} (0=mono, 1=stereo)", c.channel_count);
  REXKRNL_ERROR("   subframe_decode_count={}", c.subframe_decode_count);
  REXKRNL_ERROR("   output_buffer_block_count={} decode_buffer_bytes={}",
                c.output_buffer_block_count, c.decode_buffer_size);
  REXKRNL_ERROR("   expected_decode_hz={:.2f} (sr/512)", c.expected_decode_hz);
  REXKRNL_ERROR("   input_buf0={:08X}[{}] input_buf1={:08X}[{}]",
                c.input_buffer_0_ptr, c.input_buffer_0_packet_count,
                c.input_buffer_1_ptr, c.input_buffer_1_packet_count);

  // B. codec_read unit table
  uint32_t codec_calls = g_codec_read_stats.total_calls.load(std::memory_order_relaxed);
  uint64_t codec_bytes = g_codec_read_stats.total_pcm_bytes.load(std::memory_order_relaxed);
  uint64_t codec_frames = g_codec_read_stats.total_pcm_frames.load(std::memory_order_relaxed);
  uint64_t codec_comp = g_codec_read_stats.total_compressed_bytes.load(std::memory_order_relaxed);
  double codec_first = g_codec_read_stats.first_call_ms.load(std::memory_order_relaxed);
  double codec_last = g_codec_read_stats.last_call_ms.load(std::memory_order_relaxed);
  double codec_elapsed = (codec_last - codec_first) / 1000.0;

  REXKRNL_ERROR("B. codec_read Unit Table:");
  REXKRNL_ERROR("   total_calls={} total_pcm_bytes={} total_pcm_frames={}",
                codec_calls, codec_bytes, codec_frames);
  REXKRNL_ERROR("   total_compressed_bytes={}", codec_comp);
  REXKRNL_ERROR("   avg_bytes/call={:.0f} avg_frames/call={:.0f}",
                codec_calls > 0 ? (double)codec_bytes / codec_calls : 0,
                codec_calls > 0 ? (double)codec_frames / codec_calls : 0);
  REXKRNL_ERROR("   calls/sec={:.1f} frames/sec={:.0f}",
                codec_elapsed > 0 ? codec_calls / codec_elapsed : 0,
                codec_elapsed > 0 ? (double)codec_frames / codec_elapsed : 0);
  REXKRNL_ERROR("   expected_frames/sec={:.0f} ratio={:.2f}",
                c.sample_rate_hz > 0 ? (double)c.sample_rate_hz : 48000.0,
                codec_elapsed > 0 ? ((double)codec_frames / codec_elapsed) / (c.sample_rate_hz > 0 ? (double)c.sample_rate_hz : 48000.0) : 0);

  // C. Queue-work decision table
  uint32_t total_signals = g_signal_total.load(std::memory_order_relaxed);
  uint32_t total_waits = g_wait_total.load(std::memory_order_relaxed);
  uint32_t signal_wakes = g_wait_signal.load(std::memory_order_relaxed);
  uint32_t timeout_wakes = g_wait_timeout.load(std::memory_order_relaxed);

  REXKRNL_ERROR("C. Queue-Work Decision Table:");
  REXKRNL_ERROR("   total_signals={} total_waits={}", total_signals, total_waits);
  REXKRNL_ERROR("   signal_wakes={} timeout_wakes={}", signal_wakes, timeout_wakes);
  REXKRNL_ERROR("   signals/sec={:.1f} waits/sec={:.1f}",
                codec_elapsed > 0 ? total_signals / codec_elapsed : 0,
                codec_elapsed > 0 ? total_waits / codec_elapsed : 0);
  REXKRNL_ERROR("   signal/wait ratio={:.2f}",
                total_waits > 0 ? (double)total_signals / total_waits : 0);

  // D. Scheduler timing evidence
  uint32_t tb_reads = g_timebase_stats.total_reads.load(std::memory_order_relaxed);
  double tb_first = g_timebase_stats.first_read_ms.load(std::memory_order_relaxed);
  double tb_last = g_timebase_stats.last_read_ms.load(std::memory_order_relaxed);
  uint64_t tb_last_val = g_timebase_stats.last_value.load(std::memory_order_relaxed);

  REXKRNL_ERROR("D. Scheduler Timing Evidence:");
  REXKRNL_ERROR("   timebase_reads={} elapsed_sec={:.1f}",
                tb_reads, (tb_last - tb_first) / 1000.0);
  REXKRNL_ERROR("   last_guest_tick={} reads/sec={:.1f}",
                tb_last_val, (tb_last - tb_first) > 0 ? tb_reads / ((tb_last - tb_first) / 1000.0) : 0);

  // E. Root cause candidate
  REXKRNL_ERROR("E. Root Cause Candidate:");

  double actual_decode_hz = codec_elapsed > 0 ? codec_calls / codec_elapsed : 0;
  double actual_frames_sec = codec_elapsed > 0 ? (double)codec_frames / codec_elapsed : 0;
  double expected_hz = c.expected_decode_hz > 0 ? c.expected_decode_hz : 93.75;
  double actual_signal_hz = codec_elapsed > 0 ? total_signals / codec_elapsed : 0;

  // Check if decode rate matches signal rate (confirms signal-driven bottleneck)
  bool decode_matches_signal = fabs(actual_decode_hz - actual_signal_hz) < 5.0;

  // Check if the rate matches any known buffer quantum
  bool matches_30hz = fabs(actual_signal_hz - 30.0) < 3.0;
  bool matches_60hz = fabs(actual_signal_hz - 60.0) < 3.0;
  bool matches_94hz = fabs(actual_signal_hz - 93.75) < 5.0;
  bool matches_47hz = fabs(actual_signal_hz - 46.875) < 3.0;
  bool matches_31hz = fabs(actual_signal_hz - 31.25) < 3.0;
  bool matches_23hz = fabs(actual_signal_hz - 23.44) < 3.0;

  REXKRNL_ERROR("   actual_decode_hz={:.1f} actual_signal_hz={:.1f} expected_hz={:.2f}",
                actual_decode_hz, actual_signal_hz, expected_hz);
  REXKRNL_ERROR("   decode_matches_signal={} timeout_pct={:.1f}%",
                decode_matches_signal,
                total_waits > 0 ? (double)timeout_wakes / total_waits * 100.0 : 0);

  if (matches_30hz) {
    REXKRNL_ERROR("   => SIGNAL RATE ~30 Hz matches 48000/1600=30");
    REXKRNL_ERROR("      FMOD DSP buffer quantum is likely 1600 samples");
    REXKRNL_ERROR("      Root cause: FMOD DSP buffer size implies ~30 Hz scheduling");
  } else if (matches_31hz) {
    REXKRNL_ERROR("   => SIGNAL RATE ~31.25 Hz matches 48000/1536=31.25");
    REXKRNL_ERROR("      FMOD DSP buffer quantum is likely 1536 samples");
    REXKRNL_ERROR("      Root cause: FMOD DSP buffer size implies ~30 Hz scheduling");
  } else if (matches_47hz) {
    REXKRNL_ERROR("   => SIGNAL RATE ~46.875 Hz matches 48000/1024=46.875");
    REXKRNL_ERROR("      FMOD DSP buffer quantum is likely 1024 samples");
  } else if (matches_94hz) {
    REXKRNL_ERROR("   => SIGNAL RATE ~93.75 Hz matches 48000/512=93.75");
    REXKRNL_ERROR("      Decode rate matches XMA frame rate — scheduling is correct");
  } else if (matches_60hz) {
    REXKRNL_ERROR("   => SIGNAL RATE ~60 Hz — possibly tied to 60 fps graphics");
    REXKRNL_ERROR("      Root cause: FMOD update tied to graphics frame rate");
  } else if (matches_23hz) {
    REXKRNL_ERROR("   => SIGNAL RATE ~23.44 Hz matches 48000/2048=23.44");
    REXKRNL_ERROR("      FMOD DSP buffer quantum is likely 2048 samples");
  } else {
    REXKRNL_ERROR("   => SIGNAL RATE {:.1f} Hz does not match common buffer quanta", actual_signal_hz);
    REXKRNL_ERROR("      Need deeper investigation of mixer decision logic");
  }

  if (decode_matches_signal && timeout_wakes == 0) {
    REXKRNL_ERROR("   Confirmed: decode rate == signal rate, 0% timeout");
    REXKRNL_ERROR("   The mixer is the bottleneck — it decides when to queue work");
  }

  // Check if codec_read returns less data than expected
  double avg_frames_per_call = codec_calls > 0 ? (double)codec_frames / codec_calls : 0;
  if (avg_frames_per_call > 0 && fabs(avg_frames_per_call - 512.0) > 50.0) {
    REXKRNL_ERROR("   WARNING: avg_frames/call={:.0f} differs from expected 512", avg_frames_per_call);
    REXKRNL_ERROR("   codec_read may be returning less PCM than FMOD expects");
  }

  REXKRNL_ERROR("========================================");
}

}  // namespace xma_gap_diag

// Free-function bridge symbols for clock.cpp to call
namespace xma_gap_diag_bridge {
  inline bool gap_active() { return xma_gap_diag::gap_active_impl(); }
  inline uint32_t get_native_tid() { return xma_gap_diag::get_native_tid_impl(); }
  inline void log_timebase(uint64_t v) { return xma_gap_diag::log_timebase_impl(v); }
}
