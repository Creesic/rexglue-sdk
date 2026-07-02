/**
 * TEMP_DIAG: DOAX lossy-PulseEvent worker-wake race.
 *
 * Root-cause hypothesis (2026-06-30): the random, action-triggered ~2s gameplay
 * black (e.g. fall-in-water) is a lost-wakeup race. DOAX_FrontEndRenderTick
 * (guest sub_8258E500) wakes the 5 scene-worker fibers every frame with
 * KePulseEvent. KePulseEvent -> XEvent::Pulse -> Win32 PulseEvent is LOSSY: it
 * releases only threads ALREADY blocked in the wait at that instant. A scene
 * worker that isn't parked yet when the render thread pulses MISSES the wake,
 * skips its scene-build that cycle -> zero 3D scene draws -> the EDRAM color
 * tile resolved to the frontbuffer is black. It self-recovers (~2s) once timing
 * re-aligns and the worker is parked when a pulse lands.
 *
 * This validates that hypothesis: it tracks, per guest event (keyed by the
 * KEVENT guest pointer = XObject::guest_object()), how many threads are CURRENTLY
 * parked in a host wait on it, and flags any Pulse() that fires with zero parked
 * waiters as a provably-lost wake.
 *
 *   DOAXLOSTPULSE evt=BBBBBBBB lr=LLLLLLLL nForEvt=N total=T
 *
 * evt = guest KEVENT address (matches the events pulsed in the render tick:
 *       dword_83C43A84[], unk_83C43A5C[], 83C43A88/8C). lr = guest return addr
 *       of the Pulse call site (confirms the render-tick wake vs an unrelated
 *       pulse). Correlate the log-line wall-clock timestamp with DOAXSWAP /
 *       BLANK-FRAME lines: a burst of DOAXLOSTPULSE on a worker event lining up
 *       with a black run = the race, confirmed.
 *
 * Only events that have been WAITED on at least once are tracked, so ordinary
 * pulse-with-no-waiter no-ops on unrelated events are not logged. The waiter
 * count brackets the real host wait (rex::thread::Wait / SignalAndWait /
 * WaitAny / WaitAll) in XObject, so it is a slight over-count of "parked"
 * (a pulse in the tiny gap before the OS wait is entered reads as delivered) =>
 * detection is CONSERVATIVE: every DOAXLOSTPULSE reported is genuinely lost.
 *
 * Single rex::system module (xevent.cpp + xobject.cpp) => the inline table is
 * one instance (the DLL/EXE inline split that bit gpu_sync_diag does not apply;
 * the table is only touched from these two same-module TUs). Grep "DOAXLOSTPULSE".
 * Remove this file and its 4 call sites (xevent.cpp Pulse, xobject.cpp
 * Wait/SignalAndWait/WaitMultiple) when the race is fixed.
 */
#pragma once

#include <atomic>
#include <cstdint>

#include <rex/logging.h>
#include <rex/runtime.h>

namespace doax_pulse_diag {

constexpr uint32_t kSlots = 1024;  // power of two; >> 22 yields a 10-bit index
constexpr uint32_t kMask = kSlots - 1;
constexpr uint32_t kProbe = 32;

struct Slot {
  std::atomic<uint32_t> key{0};      // guest KEVENT addr; 0 = empty
  std::atomic<int32_t> waiters{0};   // threads currently parked in a host wait
  std::atomic<uint32_t> lost{0};     // lost-pulse count for this event
};

inline Slot g_slots[kSlots];
inline std::atomic<uint64_t> g_lost_total{0};

inline uint32_t HashIdx(uint32_t k) { return (k * 2654435761u) >> 22; }

// create=true: claim an empty slot for `key` if not present.
inline Slot* FindSlot(uint32_t key, bool create) {
  const uint32_t base = HashIdx(key);
  for (uint32_t i = 0; i < kProbe; ++i) {
    Slot& s = g_slots[(base + i) & kMask];
    uint32_t k = s.key.load(std::memory_order_acquire);
    if (k == key) return &s;
    if (k == 0) {
      if (!create) return nullptr;
      uint32_t expected = 0;
      if (s.key.compare_exchange_strong(expected, key, std::memory_order_acq_rel)) return &s;
      if (s.key.load(std::memory_order_acquire) == key) return &s;  // lost the race, same key
    }
  }
  return nullptr;  // table full / not found (diagnostics only -> just drop)
}

inline void OnWaitEnter(uint32_t event_key) {
  if (!event_key) return;
  Slot* s = FindSlot(event_key, true);
  if (s) s->waiters.fetch_add(1, std::memory_order_acq_rel);
}

inline void OnWaitExit(uint32_t event_key) {
  if (!event_key) return;
  Slot* s = FindSlot(event_key, false);
  if (s) s->waiters.fetch_sub(1, std::memory_order_acq_rel);
}

// Called from XEvent::Pulse with the event's guest_object(). Logs only events
// that have been waited on (in the table) and have no parked waiter right now.
inline void OnPulse(uint32_t event_key) {
  if (!event_key) return;
  Slot* s = FindSlot(event_key, false);
  if (!s) return;  // never waited on -> not a worker sync event; ignore
  if (s->waiters.load(std::memory_order_acquire) > 0) return;  // delivered

  const uint32_t n = s->lost.fetch_add(1, std::memory_order_relaxed) + 1;
  const uint64_t total = g_lost_total.fetch_add(1, std::memory_order_relaxed) + 1;
  // Bound volume: dense early (1,2,...,16) then every 16th. Counts stay exact.
  if (n == 1 || (n % 16) == 0) {
    auto* ctx = rex::runtime::current_ppc_context();
    const uint32_t lr = ctx ? static_cast<uint32_t>(ctx->lr) : 0;
    REXKRNL_WARN("DOAXLOSTPULSE evt={:08X} lr={:08X} nForEvt={} total={}", event_key, lr, n,
                 static_cast<unsigned long long>(total));
  }
}

}  // namespace doax_pulse_diag
