/**
 * TEMP_DIAG: worker-pool sync-coordination diagnostics.
 *
 * Investigating the intermittent (~1 in 50) "Press Start -> 4-option menu"
 * black screen in DOAX.
 *
 * RULED OUT by the doax_139(bad) vs doax_140(good) diff: GPU interrupt /
 * frame-sync (swaps + frame-done KeSetEvent identical in both, 32fps in both),
 * the transition-animation timeline (counts 900->.. identically), the game-
 * state logic (same menu-confirm/LABEL_34/scene_id markers), and file loading
 * (same 19 files incl. ups.dat/rds.dat). The ONLY divergence is the render:
 * GOOD tears the island scene down (draws 1316->685->37, destination reached)
 * while BAD plateaus at ~1170 draws and stays black. That teardown/destination
 * load is driven by the ~40 worker threads coordinating via SignalAndWait /
 * single waits / event signals — so THIS traces that coordination.
 *
 * Strategy: log each worker-sync primitive with the calling guest LR (work
 * type) and the wait/signal handles, always logging when a thread's wait/signal
 * *target changes* (a coordination state transition — where a lost wakeup would
 * strand a worker) and throttling the steady cadence. Capture a GOOD run next
 * to a BAD run; the diff at the teardown window pins which worker/handle stalls.
 *
 * All logging uses REXKRNL_ERROR to bypass log_level filters. Grep "SYNCDIAG".
 *
 * Remove this file and every `#include <rex/gpu_sync_diag.h>` / gpu_sync_diag::
 * call when done.
 */
#pragma once

#include <atomic>
#include <cstdint>

#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/xthread.h>

namespace gpu_sync_diag {

inline std::atomic<uint64_t> g_int_count{0};       // all interrupt dispatches
inline std::atomic<uint64_t> g_int_swap_count{0};  // source == 1 (swap/EOP)

// True while the current thread is executing a guest interrupt handler. Set by
// the InterruptScope guard in DispatchInterruptCallback, so any KeSetEvent /
// KeReleaseSemaphore the handler issues is attributable to the interrupt.
inline thread_local bool g_in_interrupt = false;

inline uint32_t NativeTid() { return rex::system::XThread::GetCurrentThreadId(); }

inline uint64_t GuestLr() {
  auto* ctx = rex::runtime::current_ppc_context();
  return ctx ? ctx->lr : 0;
}

// Global line budget so a long (30s+) repro can't fill the disk.
inline std::atomic<uint64_t> g_sync_lines{0};
inline constexpr uint64_t kSyncLineCap = 500000;
inline bool SyncBudget() {
  return g_sync_lines.load(std::memory_order_relaxed) < kSyncLineCap &&
         g_sync_lines.fetch_add(1, std::memory_order_relaxed) < kSyncLineCap;
}

// Called at the top of GraphicsSystem::DispatchInterruptCallback. The GPU
// interrupt/frame-sync path was RULED OUT by the 139(bad)-vs-140(good) diff, so
// this is count-only now — the counters still feed OnSwap's frame markers.
inline void OnDispatch(uint32_t source, uint32_t cpu, uint32_t callback, bool dropped) {
  g_int_count.fetch_add(1, std::memory_order_relaxed);
  if (source == 1) {
    g_int_swap_count.fetch_add(1, std::memory_order_relaxed);
  }
  (void)cpu;
  (void)callback;
  (void)dropped;
}

// RAII guard around the guest interrupt-handler execution.
struct InterruptScope {
  bool prev;
  InterruptScope() : prev(g_in_interrupt) { g_in_interrupt = true; }
  ~InterruptScope() { g_in_interrupt = prev; }
};

// Called from the CP's XE_SWAP packet handler — the actual frame buffer swap.
// Kept as a per-frame time/frame marker to correlate worker activity with the
// draw-teardown (the bad case plateaus at ~1170 draws).
// TEMP_DIAG logging neutralized: the per-sync-op REXKRNL_ERROR+flush across ~40
// worker threads was itself a heavy serialization point (contends on the spdlog
// mutex), perturbing the very timing race it was meant to observe. Bodies are
// no-ops; the call sites remain so re-enabling is a one-file change.
inline std::atomic<uint64_t> g_swap_count{0};
inline void OnSwap(uint32_t frontbuffer_ptr) { (void)frontbuffer_ptr; }
inline void OnSignal(const char* api, uint32_t obj_id) { (void)api; (void)obj_id; }
inline void OnSignalAndWait(uint32_t sig_h, uint32_t wait_h) { (void)sig_h; (void)wait_h; }
inline void OnWaitSingle(const char* api, uint32_t obj_id) { (void)api; (void)obj_id; }
inline void OnWaitMultiple(const char* api, uint32_t count, uint32_t wait_type) {
  (void)api;
  (void)count;
  (void)wait_type;
}

}  // namespace gpu_sync_diag
