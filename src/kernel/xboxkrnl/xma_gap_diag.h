/**
 * TEMP_DIAG: XMA 30ms gap diagnostic — PC Sampler + Timer Tracing
 *
 * Purpose: Identify exactly what code the FMOD XMA thread runs during the
 * ~30ms gap between XMAEnableContext returning and the next XMADisableContext.
 *
 * Architecture:
 * - Gap state flag: set when codec_read (EnableContext) exits, cleared on next XMA API entry
 * - PC sampler: Windows host thread that samples FMOD guest thread's host RIP during gap
 * - Timer tracing: logs KeQuerySystemTime and REX_QUERY_TIMEBASE calls during gap
 * - Gap boundary tracking: logs gap start/end with wall timestamps
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
#include <vector>

#include <rex/logging.h>
#include <rex/system/xthread.h>
#include <rex/runtime.h>

// TEMP_DIAG: Windows headers for SuspendThread/GetThreadContext sampler
#if REX_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

// Forward declaration from audio_system.cpp
extern uint32_t audio_diag_get_render_frame();

namespace xma_gap_diag {

// ============================================================================
// Core state
// ============================================================================

// Captured native thread ID of the XMA guest thread (set on first XMA API call)
inline std::atomic<uint32_t> g_xma_native_tid{0};

// Gap state: true = FMOD thread is in the gap (between codec_read exit and next XMA API enter)
inline std::atomic<bool> g_in_gap{false};

// Gap counter (increments each time we enter+exit a gap)
inline std::atomic<uint32_t> g_gap_counter{0};

// Track up to 8 objects that the XMA thread is currently waiting on
struct WaitedObject {
  std::atomic<uint32_t> guest_addr{0};
  std::atomic<uint32_t> ref_count{0};
};
inline constexpr int kMaxTrackedObjects = 8;
inline WaitedObject g_waited_objects[kMaxTrackedObjects];

// Per-thread state: are we inside an XMA API call?
thread_local extern bool t_in_xma_api;

// ============================================================================
// Utility
// ============================================================================

inline bool IsXmaThread() {
  auto tid = g_xma_native_tid.load(std::memory_order_relaxed);
  return tid != 0 && tid == rex::system::XThread::GetCurrentThreadId();
}

// Set to true when sampler should start (set by RegisterXmaThread)
inline std::atomic<bool> g_sampler_initialized{false};

inline void RegisterXmaThread() {
  auto expected = 0u;
  if (g_xma_native_tid.compare_exchange_strong(expected, rex::system::XThread::GetCurrentThreadId(),
                                             std::memory_order_relaxed)) {
    // First XMA API call — mark initialized so sampler thread starts sampling
    g_sampler_initialized.store(true, std::memory_order_relaxed);
    // Note: StartPcSampler is called below at file scope init, or lazily.
    // The sampler thread checks g_sampler_initialized before sampling.
  }
}

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

// ============================================================================
// PC Sampler — samples host RIP of FMOD guest thread during gap
// ============================================================================

struct RipSample {
  uint64_t rip;
  uint32_t count;
};
inline constexpr int kMaxRipSamples = 256;
inline RipSample g_rip_samples[kMaxRipSamples];
inline std::atomic<int> g_rip_sample_count{0};
inline std::atomic<uint32_t> g_total_samples{0};
inline std::atomic<bool> g_sampler_running{false};

// Accumulated samples per gap (reset each gap)
inline constexpr int kMaxGapSamples = 128;
inline uint64_t g_gap_rip_list[kMaxGapSamples];
inline std::atomic<int> g_gap_sample_idx{0};

// Record one RIP sample (called from sampler thread)
inline void RecordRipSample(uint64_t rip) {
  g_total_samples.fetch_add(1, std::memory_order_relaxed);

  // Add to per-gap list (first kMaxGapSamples samples of this gap)
  int idx = g_gap_sample_idx.fetch_add(1, std::memory_order_relaxed);
  if (idx < kMaxGapSamples) {
    g_gap_rip_list[idx] = rip;
  }

  // Accumulate into histogram
  for (int i = 0; i < g_rip_sample_count.load(std::memory_order_relaxed); i++) {
    if (g_rip_samples[i].rip == rip) {
      g_rip_samples[i].count++;
      return;
    }
  }
  int slot = g_rip_sample_count.fetch_add(1, std::memory_order_relaxed);
  if (slot < kMaxRipSamples) {
    g_rip_samples[slot].rip = rip;
    g_rip_samples[slot].count = 1;
  }
}

// Start the PC sampler thread (auto-starts when g_sampler_initialized is set)
inline void StartPcSampler() {
  if (g_sampler_running.exchange(true)) return;  // already running

  std::thread([]() {
    REXKRNL_ERROR("GAP_SAMPLER: sampler thread started");

    while (g_sampler_running.load(std::memory_order_relaxed)) {
      // Wait until the XMA thread has been registered
      if (!g_sampler_initialized.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      }
      // Wait until we're in a gap
      if (!g_in_gap.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      uint32_t native_tid = g_xma_native_tid.load(std::memory_order_relaxed);
      if (native_tid == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

#if REX_PLATFORM_WIN32
      // Open handle to the FMOD guest thread
      HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT,
                                  FALSE, native_tid);
      if (!hThread) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      // Suspend, read RIP, resume
      DWORD suspend_result = SuspendThread(hThread);
      if (suspend_result != (DWORD)-1) {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_CONTROL;
        if (GetThreadContext(hThread, &ctx)) {
          RecordRipSample(ctx.Rip);
        }
        ResumeThread(hThread);
      }
      CloseHandle(hThread);
#endif

      // Sample every ~0.5ms during gap
      std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

    REXKRNL_ERROR("GAP_SAMPLER: sampler thread stopped");
  }).detach();
}

// Auto-start the sampler on first include (runs once via inline flag)
inline int _sampler_auto_init = (StartPcSampler(), 0);

// Dump the RIP histogram only (no timer stats)
inline void DumpRipHistogram() {
  int count = g_rip_sample_count.load(std::memory_order_relaxed);
  if (count == 0) return;

  // Sort by count descending (simple bubble sort — small N)
  for (int i = 0; i < count && i < kMaxRipSamples; i++) {
    for (int j = i + 1; j < count && j < kMaxRipSamples; j++) {
      if (g_rip_samples[j].count > g_rip_samples[i].count) {
        RipSample tmp = g_rip_samples[i];
        g_rip_samples[i] = g_rip_samples[j];
        g_rip_samples[j] = tmp;
      }
    }
  }

  uint32_t total = g_total_samples.load(std::memory_order_relaxed);
  REXKRNL_ERROR("GAP_SAMPLER: === RIP HISTOGRAM (top 30, total_samples={}) ===", total);

  for (int i = 0; i < count && i < 30 && i < kMaxRipSamples; i++) {
    double pct = 100.0 * g_rip_samples[i].count / total;
    REXKRNL_ERROR("GAP_SAMPLER: [{:3d}] rip={:016X} count={:5d} ({:5.1f}%)",
                  i, g_rip_samples[i].rip, g_rip_samples[i].count, pct);
  }
}

// ============================================================================
// Timer read tracing (TASK 2)
// ============================================================================

struct TimerRead {
  uint64_t guest_lr;
  uint64_t returned_value;
  double wall_ms;
  uint32_t gap_id;
};

inline constexpr int kMaxTimerReads = 512;
inline TimerRead g_timer_reads[kMaxTimerReads];
inline std::atomic<int> g_timer_read_count{0};
inline std::atomic<uint64_t> g_prev_timer_value{0};
inline std::atomic<double> g_prev_timer_wall_ms{0.0};

// Log a timer/timebase read from the FMOD thread during gap
// Called from KeQuerySystemTime_entry and from QueryGuestTickCount wrapper
inline void LogTimerRead(const char* api, uint64_t returned_value) {
  if (!g_in_gap.load(std::memory_order_relaxed)) return;
  if (!IsXmaThread()) return;

  double now_ms = WallMs();
  uint64_t lr = GetGuestLR();
  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gap_id = g_gap_counter.load(std::memory_order_relaxed);

  uint64_t prev_val = g_prev_timer_value.exchange(returned_value, std::memory_order_relaxed);
  double prev_ms = g_prev_timer_wall_ms.exchange(now_ms, std::memory_order_relaxed);
  double wall_delta = now_ms - prev_ms;
  uint64_t val_delta = returned_value > prev_val ? returned_value - prev_val : 0;

  // Compute guest ticks/sec if wall_delta > 0
  double guest_ticks_per_sec = 0.0;
  if (wall_delta > 0.01) {
    guest_ticks_per_sec = val_delta / (wall_delta / 1000.0);
  }

  REXKRNL_ERROR("GAP_TIMER rf={} {} val={:016X} delta_val={} delta_wall_ms={:.3f} "
                "guest_hz={:.0f} lr={:08X} gap={}",
                rf, api, returned_value, val_delta, wall_delta,
                guest_ticks_per_sec, (uint32_t)lr, gap_id);

  int slot = g_timer_read_count.fetch_add(1, std::memory_order_relaxed);
  if (slot < kMaxTimerReads) {
    g_timer_reads[slot] = {lr, returned_value, now_ms, gap_id};
  }
}

// Dump cumulative histogram + timer stats (defined here after timer section)
inline void DumpSamplerResults() {
  DumpRipHistogram();
  int timer_count = g_timer_read_count.load(std::memory_order_relaxed);
  REXKRNL_ERROR("GAP_SAMPLER: total timer reads during gaps: {}", timer_count);
}

// ============================================================================
// Wait/Signal/Delay diagnostics (preserved from before)
// ============================================================================

inline void TrackWaitObject(uint32_t guest_addr) {
  if (!IsXmaThread()) return;
  for (int i = 0; i < kMaxTrackedObjects; i++) {
    uint32_t expected = 0;
    if (g_waited_objects[i].guest_addr.compare_exchange_strong(
            expected, guest_addr, std::memory_order_relaxed)) {
      g_waited_objects[i].ref_count.store(1, std::memory_order_relaxed);
      return;
    }
  }
}

inline void UntrackWaitObject(uint32_t guest_addr) {
  for (int i = 0; i < kMaxTrackedObjects; i++) {
    if (g_waited_objects[i].guest_addr.load(std::memory_order_relaxed) == guest_addr) {
      g_waited_objects[i].guest_addr.store(0, std::memory_order_relaxed);
      return;
    }
  }
}

inline bool IsWaitedObject(uint32_t guest_addr) {
  for (int i = 0; i < kMaxTrackedObjects; i++) {
    if (g_waited_objects[i].guest_addr.load(std::memory_order_relaxed) == guest_addr) {
      return true;
    }
  }
  return false;
}

inline double LogWaitEnter(const char* api, uint32_t obj_guest_addr,
                           uint32_t alertable, bool is_infinite, int64_t timeout_val) {
  if (!IsXmaThread()) return 0.0;
  uint32_t rf = audio_diag_get_render_frame();
  uint64_t lr = GetGuestLR();
  uint32_t gtid = GetGuestTID();
  TrackWaitObject(obj_guest_addr);
  REXKRNL_ERROR("GAP_WAIT ENTER rf={} t{} {} obj={:08X} lr={:08X} alertable={} "
                "infinite={} timeout={}",
                rf, gtid, api, obj_guest_addr, (uint32_t)lr, alertable,
                is_infinite, timeout_val);
  return WallMs();
}

inline void LogWaitExit(const char* api, uint32_t obj_guest_addr,
                        uint32_t result, double enter_ms) {
  if (!IsXmaThread()) return;
  double exit_ms = WallMs();
  double elapsed = enter_ms > 0 ? (exit_ms - enter_ms) : 0.0;
  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  UntrackWaitObject(obj_guest_addr);
  REXKRNL_ERROR("GAP_WAIT EXIT  rf={} t{} {} obj={:08X} result={:08X} "
                "elapsed_ms={:.1f}",
                rf, gtid, api, obj_guest_addr, result, elapsed);
}

inline double LogMultiWaitEnter(const char* api, uint32_t count,
                                uint32_t* obj_addrs, uint32_t alertable,
                                bool is_infinite) {
  if (!IsXmaThread()) return 0.0;
  uint32_t rf = audio_diag_get_render_frame();
  uint64_t lr = GetGuestLR();
  uint32_t gtid = GetGuestTID();
  for (uint32_t i = 0; i < count && i < 4; i++) {
    TrackWaitObject(obj_addrs[i]);
  }
  REXKRNL_ERROR("GAP_WAIT ENTER rf={} t{} {} count={} obj=[{:08X},{:08X}] lr={:08X} "
                "infinite={}",
                rf, gtid, api, count,
                count > 0 ? obj_addrs[0] : 0,
                count > 1 ? obj_addrs[1] : 0,
                (uint32_t)lr, is_infinite);
  return WallMs();
}

inline void LogMultiWaitExit(const char* api, uint32_t count,
                             uint32_t* obj_addrs, uint32_t result,
                             double enter_ms) {
  if (!IsXmaThread()) return;
  double exit_ms = WallMs();
  double elapsed = enter_ms > 0 ? (exit_ms - enter_ms) : 0.0;
  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  for (uint32_t i = 0; i < count && i < 4; i++) {
    UntrackWaitObject(obj_addrs[i]);
  }
  REXKRNL_ERROR("GAP_WAIT EXIT  rf={} t{} {} result={:08X} elapsed_ms={:.1f}",
                rf, gtid, api, result, elapsed);
}

inline void LogSignal(const char* api, uint32_t obj_guest_addr, uint32_t value = 0) {
  bool is_xma_thread = IsXmaThread();
  bool is_waited = IsWaitedObject(obj_guest_addr);
  if (!is_xma_thread && !is_waited) return;
  uint32_t rf = audio_diag_get_render_frame();
  uint32_t native_tid = rex::system::XThread::GetCurrentThreadId();
  uint32_t gtid = GetGuestTID();
  uint64_t lr = GetGuestLR();
  REXKRNL_ERROR("GAP_SIGNAL    rf={} t{}(n{}) {} obj={:08X} lr={:08X} val={}",
                rf, gtid, native_tid, api, obj_guest_addr, (uint32_t)lr, value);
}

inline double LogDelayEnter(const char* api, uint64_t interval) {
  if (!IsXmaThread()) return 0.0;
  uint32_t rf = audio_diag_get_render_frame();
  uint64_t lr = GetGuestLR();
  uint32_t gtid = GetGuestTID();
  REXKRNL_ERROR("GAP_DELAY ENTER rf={} t{} {} interval={} lr={:08X}",
                rf, gtid, api, (int64_t)interval, (uint32_t)lr);
  return WallMs();
}

inline void LogDelayExit(const char* api, uint32_t result, double enter_ms) {
  if (!IsXmaThread()) return;
  double exit_ms = WallMs();
  double elapsed = enter_ms > 0 ? (exit_ms - enter_ms) : 0.0;
  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  REXKRNL_ERROR("GAP_DELAY EXIT  rf={} t{} {} result={:08X} elapsed_ms={:.1f}",
                rf, gtid, api, result, elapsed);
}

// ============================================================================
// XMA boundary markers — with gap state management
// ============================================================================

inline void LogXmaEnter(const char* api, uint32_t ctx_addr) {
  RegisterXmaThread();

  // If we were in a gap, this entry ends it
  bool was_in_gap = g_in_gap.exchange(false, std::memory_order_relaxed);

  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  uint64_t lr = GetGuestLR();
  double ms = WallMs();

  if (was_in_gap) {
    uint32_t gap_id = g_gap_counter.load(std::memory_order_relaxed);
    int gap_samples = g_gap_sample_idx.load(std::memory_order_relaxed);

    REXKRNL_ERROR("GAP_BOUNDARY === GAP END #{} rf={} t{} {} ctx={:08X} lr={:08X} ms={:.3f} "
                  "gap_samples={} ===",
                  gap_id, rf, gtid, api, ctx_addr, (uint32_t)lr, ms, gap_samples);

    // Dump per-gap RIP list (raw samples for this gap)
    if (gap_samples > 0) {
      // Count unique RIPs in this gap
      std::unordered_map<uint64_t, int> gap_counts;
      int n = gap_samples < kMaxGapSamples ? gap_samples : kMaxGapSamples;
      for (int i = 0; i < n; i++) {
        gap_counts[g_gap_rip_list[i]]++;
      }
      // Sort by count
      std::vector<std::pair<uint64_t, int>> sorted(gap_counts.begin(), gap_counts.end());
      std::sort(sorted.begin(), sorted.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });
      REXKRNL_ERROR("GAP_GAPRIP #{} top entries (of {} unique, {} total):",
                    gap_id, sorted.size(), n);
      int shown = 0;
      for (auto& [rip, cnt] : sorted) {
        if (shown++ >= 20) break;
        double pct = 100.0 * cnt / n;
        REXKRNL_ERROR("GAP_GAPRIP #{} [{:2d}] rip={:016X} count={:4d} ({:5.1f}%)",
                      gap_id, shown - 1, rip, cnt, pct);
      }
    }

    // Dump cumulative histogram every 50 gaps
    if (gap_id % 50 == 0) {
      DumpSamplerResults();
    }
  }

  REXKRNL_ERROR("GAP_XMA ENTER rf={} t{} {} ctx={:08X} lr={:08X} ms={:.3f}",
                rf, gtid, api, ctx_addr, (uint32_t)lr, ms);
}

inline void LogXmaExit(const char* api, uint32_t ctx_addr) {
  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gtid = GetGuestTID();
  double ms = WallMs();

  REXKRNL_ERROR("GAP_XMA EXIT  rf={} t{} {} ctx={:08X} ms={:.3f}",
                rf, gtid, api, ctx_addr, ms);

  // If this is EnableContext exiting, we're entering the gap
  // (the FMOD thread will now run PPC code until the next XMA API call)
  if (strcmp(api, "XMAEnableContext") == 0) {
    uint32_t gap_id = g_gap_counter.fetch_add(1, std::memory_order_relaxed) + 1;
    g_gap_sample_idx.store(0, std::memory_order_relaxed);

    REXKRNL_ERROR("GAP_BOUNDARY === GAP START #{} rf={} ms={:.3f} ===",
                  gap_id, rf, ms);

    g_in_gap.store(true, std::memory_order_relaxed);
  }
}

// ============================================================================
// Bridge accessors for clock.cpp (avoids pulling Windows headers into core/)
// These are declared extern in clock.cpp and defined here (header-only).
// ============================================================================

// Returns true if the gap flag is set
inline bool gap_active_impl() {
  return g_in_gap.load(std::memory_order_relaxed);
}

// Returns the captured XMA native thread ID
inline uint32_t get_native_tid_impl() {
  return g_xma_native_tid.load(std::memory_order_relaxed);
}

// Logs a timebase read from generated code (REX_QUERY_TIMEBASE)
inline void log_timebase_impl(uint64_t value) {
  if (!g_in_gap.load(std::memory_order_relaxed)) return;

  double now_ms = WallMs();
  uint64_t lr = GetGuestLR();
  uint32_t rf = audio_diag_get_render_frame();
  uint32_t gap_id = g_gap_counter.load(std::memory_order_relaxed);

  uint64_t prev_val = g_prev_timer_value.exchange(value, std::memory_order_relaxed);
  double prev_ms = g_prev_timer_wall_ms.exchange(now_ms, std::memory_order_relaxed);
  double wall_delta = now_ms - prev_ms;
  uint64_t val_delta = value > prev_val ? value - prev_val : 0;

  double guest_ticks_per_sec = 0.0;
  if (wall_delta > 0.01) {
    guest_ticks_per_sec = val_delta / (wall_delta / 1000.0);
  }

  REXKRNL_ERROR("GAP_TIMER rf={} REX_QUERY_TIMEBASE val={:016X} delta_val={} "
                "delta_wall_ms={:.3f} guest_hz={:.0f} lr={:08X} gap={}",
                rf, value, val_delta, wall_delta,
                guest_ticks_per_sec, (uint32_t)lr, gap_id);
}

}  // namespace xma_gap_diag

// Free-function bridge symbols for clock.cpp to call
// These are inline (header-only) but we need them at file scope.
// Using a namespace to avoid collisions.
namespace xma_gap_diag_bridge {
  inline bool gap_active() { return xma_gap_diag::gap_active_impl(); }
  inline uint32_t get_native_tid() { return xma_gap_diag::get_native_tid_impl(); }
  inline void log_timebase(uint64_t v) { xma_gap_diag::log_timebase_impl(v); }
}

// These extern declarations are what clock.cpp sees.
// They're satisfied by the inline definitions above in any TU that includes this header.
// Since clock.cpp does NOT include this header, we need a different mechanism.
// Solution: define them as weak/exported functions from xboxkrnl_audio_xma.cpp.

// Thread-local definition
inline thread_local bool xma_gap_diag::t_in_xma_api{false};
