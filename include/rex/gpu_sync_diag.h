/**
 * TEMP_DIAG: worker-pool sync-coordination diagnostics (lock-free trace).
 *
 * Investigating the intermittent (~9 in 10) "Press Start -> 4-option menu"
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
 * single waits / event signals -- so a lost wakeup strands a worker.
 *
 * Prior approach (per-op REXKRNL_ERROR+flush) FAILED: the spdlog mutex
 * serialized all ~40 workers and perturbed the very race. This version records
 * each sync op into a LOCK-FREE in-memory ring (atomic fetch_add slot, no mutex,
 * no I/O) and dumps the recent window to logs/synctrace_<reason>.txt ONCE at the
 * terminal state (present->2 = BLACK, or the good menu coming up), driven from
 * LogBootGate. A stranded worker shows as a WAIT_ENTER with no matching WAIT_EXIT
 * for its tid; diff the good vs bad dump at the teardown window to pin worker+handle.
 *
 * CROSS-MODULE: the ring + Record/Dump/Arm are DEFINED ONCE in the runtime DLL
 * (the TU that does `#define GPU_SYNC_DIAG_IMPL` before including this --
 * xboxkrnl_threading.cpp) and exported via CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS; every
 * other TU only sees declarations. Header `inline` variables get a SEPARATE copy
 * per module on Windows, which silently split the ring (DLL recorded into its copy,
 * doax.exe's LogBootGate dumped its own empty copy -> total_ops=1). Hence functions.
 *
 * Grep dumps for "SYNCDIAG". Remove this file and every include / call when done.
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
inline std::atomic<uint64_t> g_swap_count{0};      // CP frame swaps
inline std::atomic<bool> g_armed{false};           // confirm window opened (smode==2)
inline std::atomic<uint64_t> g_arm_swap{0};        // g_swap_count at arm (GOOD baseline)
inline constexpr uint64_t kGoodSwaps = 120;        // swaps since arm w/o black => GOOD

// True while the current thread is executing a guest interrupt handler. Set by
// the InterruptScope guard in DispatchInterruptCallback.
inline thread_local bool g_in_interrupt = false;

inline uint32_t NativeTid() { return rex::system::XThread::GetCurrentThreadId(); }

inline uint64_t GuestLr() {
  auto* ctx = rex::runtime::current_ppc_context();
  return ctx ? ctx->lr : 0;
}

enum SyncOp : uint16_t {
  OP_NONE = 0,
  OP_SET_EVENT,             // KeSetEvent
  OP_NT_SET_EVENT,          // NtSetEvent
  OP_RELEASE_SEM,           // KeReleaseSemaphore
  OP_WAIT1_ENTER,           // KeWaitForSingleObject
  OP_WAIT1_EXIT,
  OP_NTWAIT1_ENTER,         // NtWaitForSingleObjectEx
  OP_NTWAIT1_EXIT,
  OP_WAITM_ENTER,           // KeWaitForMultipleObjects
  OP_WAITM_EXIT,
  OP_NTWAITM_ENTER,         // NtWaitForMultipleObjectsEx
  OP_NTWAITM_EXIT,
  OP_SIGNALWAIT_ENTER,      // NtSignalAndWaitForSingleObjectEx
  OP_SIGNALWAIT_EXIT,
  OP_SWAP,                  // CP frame buffer swap
  OP_MARK,                  // explicit marker (arm locator)
  OP_FIBERSWAP,             // guest fiber swap (handle=src block, handle2=tgt block,
                            // result bit0=will_switch bit1=brand_new)
  OP_FUNC,                  // guest function ENTRY (handle = func addr, handle2 = arg/state)
};

// --- Shared across the DLL/EXE boundary (see CROSS-MODULE note above) ---------
// Defined once in the IMPL TU (rexruntimerd.dll), exported, called from both
// modules. NOT inline -> exactly one ring instance.
void Record(uint16_t op, uint32_t handle, uint32_t handle2, uint16_t result);
void Dump(const char* reason);  // one-shot; dumps logs/synctrace_<reason>.txt
void Arm(const char* tag);      // drop a locator MARK at the confirm window

// --- Sync-op hooks (inline; call the exported Record) -------------------------
// Signatures preserved from the prior count-only version so the threading / CP /
// graphics call sites are unchanged.

inline void OnSwap(uint32_t frontbuffer_ptr) {
  const uint64_t s = g_swap_count.fetch_add(1, std::memory_order_relaxed) + 1;
  Record(OP_SWAP, frontbuffer_ptr, 0, 0);
  // GOOD trigger: the render keeps swapping past the danger window. A black run
  // FREEZES the swap thread, so it never reaches kGoodSwaps. Driven here, NOT from
  // LogBootGate, because in a good run the boot scheduler parks (countdown freezes
  // at 1) and LogBootGate stops polling. fl85094 stays 0 even in good (doax_053.log).
  if (g_armed.load(std::memory_order_relaxed) &&
      s - g_arm_swap.load(std::memory_order_relaxed) >= kGoodSwaps) {
    Dump("kept-swapping");  // trigger description, NOT a verdict (one-shot; no-op if already dumped)
  }
}

inline uint16_t SignalOp(const char* api) {
  // "KeSetEvent" / "NtSetEvent" / "KeReleaseSemaphore"
  if (api[0] == 'N') return OP_NT_SET_EVENT;
  return api[2] == 'R' ? OP_RELEASE_SEM : OP_SET_EVENT;
}
inline void OnSignal(const char* api, uint32_t obj_id) { Record(SignalOp(api), obj_id, 0, 0); }

inline void OnWaitSingle(const char* api, uint32_t obj_id) {
  Record(api[0] == 'N' ? OP_NTWAIT1_ENTER : OP_WAIT1_ENTER, obj_id, 0, 0);
}
inline void OnWaitSingleExit(const char* api, uint32_t obj_id, uint32_t result) {
  Record(api[0] == 'N' ? OP_NTWAIT1_EXIT : OP_WAIT1_EXIT, obj_id, 0,
         static_cast<uint16_t>(result));
}

// h0/h1 = first two wait objects (the count is in result). For the stuck worker
// multi-waits (count=1) h0 IS the handle whose wakeup is lost.
inline void OnWaitMultiple(const char* api, uint32_t count, uint32_t h0, uint32_t h1) {
  Record(api[0] == 'N' ? OP_NTWAITM_ENTER : OP_WAITM_ENTER, h0, h1,
         static_cast<uint16_t>(count));
}
inline void OnWaitMultipleExit(const char* api, uint32_t count, uint32_t result, uint32_t h0) {
  Record(api[0] == 'N' ? OP_NTWAITM_EXIT : OP_WAITM_EXIT, h0, count,
         static_cast<uint16_t>(result));
}

inline void OnSignalAndWait(uint32_t sig_h, uint32_t wait_h) {
  Record(OP_SIGNALWAIT_ENTER, sig_h, wait_h, 0);
}
inline void OnSignalAndWaitExit(uint32_t sig_h, uint32_t wait_h, uint32_t result) {
  Record(OP_SIGNALWAIT_EXIT, sig_h, wait_h, static_cast<uint16_t>(result));
}

// Called at the top of GraphicsSystem::DispatchInterruptCallback. The GPU
// interrupt/frame-sync path was RULED OUT by the 139/140 diff, so count-only.
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

}  // namespace gpu_sync_diag

// ============================================================================
// Single definition (exactly one TU defines GPU_SYNC_DIAG_IMPL: the runtime's
// xboxkrnl_threading.cpp). Lives in rexruntimerd.dll; auto-exported.
// ============================================================================
#ifdef GPU_SYNC_DIAG_IMPL
#include <chrono>
#include <cstdio>
#include <string>

namespace gpu_sync_diag {

struct SyncRec {
  uint64_t ts;       // steady_clock ns
  uint64_t seq;      // global order (fetch_add index)
  uint32_t tid;      // thread id
  uint32_t lr;       // guest LR (work type / call site)
  uint32_t handle;   // primary guest object addr/handle
  uint32_t handle2;  // wait handle / count
  uint16_t op;       // SyncOp
  uint16_t result;   // X_STATUS low bits / wait_type
};

constexpr uint32_t kRingShift = 20;  // 1,048,576 entries (~36 MB)
constexpr uint32_t kRingSize = 1u << kRingShift;
constexpr uint32_t kRingMask = kRingSize - 1;
constexpr uint64_t kDumpWindow = 50000;  // last N records (spans confirm->terminal); kept modest now that
                                         // the trace goes into the main doax_NNN.log instead of its own file

static SyncRec g_ring[kRingSize];
static std::atomic<uint64_t> g_ring_head{0};
static std::atomic<bool> g_trace_enabled{true};
static std::atomic<bool> g_dumped{false};

static uint64_t NowNs() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

static const char* OpName(uint16_t op) {
  switch (op) {
    case OP_SET_EVENT: return "SetEvent";
    case OP_NT_SET_EVENT: return "NtSetEvent";
    case OP_RELEASE_SEM: return "RelSem";
    case OP_WAIT1_ENTER: return "Wait1>";
    case OP_WAIT1_EXIT: return "Wait1<";
    case OP_NTWAIT1_ENTER: return "NtWait1>";
    case OP_NTWAIT1_EXIT: return "NtWait1<";
    case OP_WAITM_ENTER: return "WaitM>";
    case OP_WAITM_EXIT: return "WaitM<";
    case OP_NTWAITM_ENTER: return "NtWaitM>";
    case OP_NTWAITM_EXIT: return "NtWaitM<";
    case OP_SIGNALWAIT_ENTER: return "SigWait>";
    case OP_SIGNALWAIT_EXIT: return "SigWait<";
    case OP_SWAP: return "Swap";
    case OP_MARK: return "MARK";
    case OP_FIBERSWAP: return "FiberSw";
    case OP_FUNC: return "Func";
    default: return "?";
  }
}

// Hot path: one atomic fetch_add + one struct store. No lock, no I/O.
void Record(uint16_t op, uint32_t handle, uint32_t handle2, uint16_t result) {
  if (!g_trace_enabled.load(std::memory_order_relaxed)) {
    return;
  }
  const uint64_t i = g_ring_head.fetch_add(1, std::memory_order_relaxed);
  SyncRec& r = g_ring[i & kRingMask];
  r.ts = NowNs();
  r.seq = i;
  r.tid = NativeTid();
  r.lr = static_cast<uint32_t>(GuestLr());
  r.handle = handle;
  r.handle2 = handle2;
  r.op = op;
  r.result = result;
}

void Arm(const char* tag) {
  if (g_dumped.load(std::memory_order_relaxed)) {
    return;
  }
  g_arm_swap.store(g_swap_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
  g_armed.store(true, std::memory_order_relaxed);
  Record(OP_MARK, 0, 0, 0);
  REXKRNL_WARN("SYNCDIAG armed at {} (head={} swap={})", tag,
               static_cast<unsigned long long>(g_ring_head.load()),
               static_cast<unsigned long long>(g_arm_swap.load()));
}

void Dump(const char* reason) {
  bool expected = false;
  if (!g_dumped.compare_exchange_strong(expected, true)) {
    return;  // already dumped (one-shot)
  }
  g_trace_enabled.store(false, std::memory_order_seq_cst);

  const uint64_t head = g_ring_head.load(std::memory_order_seq_cst);
  const uint64_t total = head;
  const uint64_t count = total < kDumpWindow ? total : kDumpWindow;
  const uint64_t start = head - count;

  // CONSOLIDATED: emit the trace into the MAIN doax_NNN.log (one spot, auto per-run) instead of a
  // separate logs/synctrace_*.txt. `reason` is the TRIGGER CONDITION (not a GOOD/BLACK verdict -- that was
  // misleading once the present-hold gates kept present=1). Each op line is prefixed "ST " (grep that).
  // Lines are batched into multi-line log entries to avoid thousands of spdlog calls.
  REXKRNL_WARN(
      "SYNCTRACE BEGIN reason={} total_ops={} window={} cols=[seq dt_us tid op handle handle2 lr result]",
      reason, static_cast<unsigned long long>(total), static_cast<unsigned long long>(count));
  std::string buf;
  buf.reserve(1u << 16);
  char line[160];
  uint64_t t0 = 0;
  bool have_t0 = false;
  uint64_t printed = 0;
  int inbuf = 0;
  for (uint64_t s = start; s < head; ++s) {
    const SyncRec& r = g_ring[s & kRingMask];
    if (r.seq != s) {
      continue;  // slot overwritten between snapshot and read
    }
    if (!have_t0) {
      t0 = r.ts;
      have_t0 = true;
    }
    const int n = std::snprintf(line, sizeof(line), "ST %llu %.1f %u %s 0x%08X 0x%08X 0x%08X %u\n",
                                static_cast<unsigned long long>(r.seq), (r.ts - t0) / 1000.0, r.tid,
                                OpName(r.op), r.handle, r.handle2, r.lr, r.result);
    if (n > 0) {
      buf.append(line, static_cast<size_t>(n));
    }
    ++printed;
    if (++inbuf >= 500) {
      REXKRNL_WARN("SYNCTRACE\n{}", buf);
      buf.clear();
      inbuf = 0;
    }
  }
  if (!buf.empty()) {
    REXKRNL_WARN("SYNCTRACE\n{}", buf);
  }
  REXKRNL_WARN("SYNCTRACE END reason={} printed={}", reason, static_cast<unsigned long long>(printed));
}

}  // namespace gpu_sync_diag
#endif  // GPU_SYNC_DIAG_IMPL
