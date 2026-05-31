// FH1 loading harness — loader epoch gate + audio consumer spin.

#include "fh1_loader_epoch.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

#include <rex/logging.h>
#include <rex/types.h>

namespace {

std::atomic<int> g_load_gate_depth{0};

// Per-thread nesting witness. Only the outermost enter/leave pair touches the
// global counter so nested loader calls (e.g. init → load track) stay gated.
thread_local int t_load_depth = 0;

}  // namespace

extern "C" void fh1_load_gate_enter(void) {
  const int td = ++t_load_depth;
  if (td == 1) {
    const int new_depth = g_load_gate_depth.fetch_add(1, std::memory_order_release) + 1;
    REXSYS_INFO("[fh1_loader_epoch] ENTER (global={}, t_depth=1)", new_depth);
  }
}

extern "C" void fh1_load_gate_leave(void) {
  const int td = --t_load_depth;
  if (td == 0) {
    const int new_depth = g_load_gate_depth.fetch_sub(1, std::memory_order_release) - 1;
    REXSYS_INFO("[fh1_loader_epoch] LEAVE (global={}, t_depth=0)", new_depth);
  } else if (td < 0) {
    REXSYS_WARN(
        "[fh1_loader_epoch] LEAVE called with no matching enter (t_depth={}) — clamping",
        td);
    t_load_depth = 0;
  }
}

extern "C" void fh1_load_gate_dispatch_wait(uint32_t target) {
  if (t_load_depth > 0) {
    return;  // loader thread bypasses its own gate
  }
  if (g_load_gate_depth.load(std::memory_order_acquire) == 0) {
    return;
  }

  REXSYS_INFO("[fh1_loader_epoch] WAIT begin target=0x{:08X} (depth={})", target,
              g_load_gate_depth.load());

  // 30s safety timeout — fall through to preserve AV observability vs deadlock.
  for (int spins = 0; spins < 3000; ++spins) {
    if (g_load_gate_depth.load(std::memory_order_acquire) == 0) {
      REXSYS_INFO(
          "[fh1_loader_epoch] WAIT end target=0x{:08X} after {} spins ({}ms)", target,
          spins, spins * 10);
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REXSYS_WARN(
      "[fh1_loader_epoch] WAIT TIMED OUT target=0x{:08X} after 30s — gate fall-through",
      target);
}

namespace {

inline uint32_t guest_load_u32_be_volatile(uint8_t* base, uint32_t addr) {
  const uint32_t be = *reinterpret_cast<volatile uint32_t*>(base + addr);
  return rex::byte_swap(be);
}

inline bool looks_like_torn_ptr(uint32_t v) {
  if (v == 0u) {
    return true;
  }
  if (v == 9u) {
    return true;  // codec_type sentinel leak
  }
  if (v < 0x10000u) {
    return true;
  }
  return false;
}

}  // namespace

extern "C" void fh1_audio_consumer_acquire(void* guest_base, uint32_t this_ptr) {
  if (!guest_base || !this_ptr) {
    return;
  }
  auto* base = static_cast<uint8_t*>(guest_base);

  constexpr int kMaxSpins = 1000;
  constexpr auto kSleep = std::chrono::microseconds(1000);

  std::atomic_thread_fence(std::memory_order_acquire);

  bool warned = false;
  for (int spin = 0; spin < kMaxSpins; ++spin) {
    const uint32_t sound = guest_load_u32_be_volatile(base, this_ptr + 24);
    if (!looks_like_torn_ptr(sound)) {
      const uint32_t inner = guest_load_u32_be_volatile(base, sound + 68);
      const bool inner_ok = (inner == 0u) || !looks_like_torn_ptr(inner);
      if (inner_ok) {
        std::atomic_thread_fence(std::memory_order_acquire);
        if (warned) {
          REXSYS_INFO(
              "[fh1_loader_epoch] audio_consumer_acquire OK this={:08X} after {} spins",
              this_ptr, spin);
        }
        return;
      }
    }

    if (!warned) {
      REXSYS_WARN(
          "[fh1_loader_epoch] audio_consumer_acquire SPIN this={:08X} (sound={:08X}) — "
          "retrying",
          this_ptr, sound);
      warned = true;
    }
    std::atomic_thread_fence(std::memory_order_acquire);
    std::this_thread::sleep_for(kSleep);
  }

  REXSYS_WARN(
      "[fh1_loader_epoch] audio_consumer_acquire TIMED OUT this={:08X} after 1s — "
      "fall-through",
      this_ptr);
}
