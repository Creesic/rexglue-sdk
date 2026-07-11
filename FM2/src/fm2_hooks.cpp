/**
 * @file        fm2_hooks.cpp
 * @brief       FM2 mid-asm hooks: crash guards + FMOD pump wait-timeout override
 *
 * Ported from ReXGlue080plume's FM2/src/fm2_hooks.cpp (3645 lines). That file
 * mixed these with ~100 diagnostic-only instrumentation hooks and 18 hooks
 * tied to the plume/native-renderer integration; only the 9 verified
 * crash-guard hooks, the shared guest-memory-safety helpers they need, and
 * one deliberately-kept non-crash hook (the FMOD pump wait-timeout override)
 * are ported here. See docs/migration-from-plume.md for the full audit.
 */

#include "generated/default/fm2_init.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <mutex>

#include <rex/memory/utils.h>
#include <rex/ppc.h>
#include <rex/system/kernel_state.h>

namespace {

uint8_t* GuestBase() {
  auto* kernel_state = rex::system::kernel_state();
  if (!kernel_state || !kernel_state->memory()) {
    return nullptr;
  }
  return kernel_state->memory()->virtual_membase();
}

bool GuestReadableByte(uint8_t* base, uint32_t guest_address) {
  (void)base;
  constexpr uint32_t kPageMask = ~uint32_t(0xFFFu);
  constexpr uint32_t kPageCacheSlots = 32u;
  struct PageCacheEntry {
    uint32_t page = 0;
    uint8_t readable = 0;
    uint8_t valid = 0;
  };
  thread_local PageCacheEntry cache[kPageCacheSlots];

  const uint32_t page = guest_address & kPageMask;
  const uint32_t slot = (page >> 12) & (kPageCacheSlots - 1);
  PageCacheEntry& e = cache[slot];
  if (e.valid && e.page == page) {
    return e.readable != 0;
  }

  size_t length = 1;
  rex::memory::PageAccess access = rex::memory::PageAccess::kNoAccess;
  if (!rex::memory::QueryProtect(REX_RAW_ADDR(page), length, access)) {
    e.page = page;
    e.readable = 0;
    e.valid = 1;
    return false;
  }
  const bool readable = access != rex::memory::PageAccess::kNoAccess;
  e.page = page;
  e.readable = readable ? 1 : 0;
  e.valid = 1;
  return readable;
}

bool GuestReadableRange(uint8_t* base, uint32_t guest_address, uint32_t byte_count) {
  if (byte_count == 0) {
    return true;
  }

  const uint64_t last = uint64_t(guest_address) + byte_count - 1;
  if (last > std::numeric_limits<uint32_t>::max()) {
    return false;
  }

  return GuestReadableByte(base, guest_address) &&
         GuestReadableByte(base, static_cast<uint32_t>(last));
}

bool HasCallableVtableSlot(uint8_t* base, uint32_t object, uint32_t slot_offset) {
  if (object == 0 || (object & 3) != 0 || !GuestReadableRange(base, object, 4)) {
    return false;
  }

  const uint32_t vtable = REX_LOAD_U32(object);
  const uint64_t slot = uint64_t(vtable) + slot_offset;
  if ((vtable & 3) != 0 || slot > std::numeric_limits<uint32_t>::max() ||
      !GuestReadableRange(base, static_cast<uint32_t>(slot), 4)) {
    return false;
  }

  const uint32_t target = REX_LOAD_U32(static_cast<uint32_t>(slot));
  constexpr uint64_t code_end = uint64_t(REX_CODE_BASE) + REX_CODE_SIZE;
  return (target & 3) == 0 && target >= REX_CODE_BASE && target < code_end;
}

}  // namespace

// --- Racetrack-loading / race start-finish crash guards ---------------------
// Each guard validates a pointer or vtable slot the generated code is about
// to dereference/call unconditionally; on failure it redirects control flow
// (via the manifest's jump_address_on_true) instead of faulting. See
// docs/migration-from-plume.md for the fault this fixes and the exact
// [[entrypoint.midasm_hook]] wiring in fm2_manifest.toml.

bool FM2SkipBadChildSlot(PPCRegister& r11, PPCRegister& r31) {
  uint8_t* base = GuestBase();
  if (!base || HasCallableVtableSlot(base, r11.u32, 12)) {
    return false;
  }
  REX_STORE_U32(r31.u32, 0);
  return true;
}

bool FM2ReturnZeroOnBadNestedVcall(PPCRegister& r3) {
  uint8_t* base = GuestBase();
  return base && !HasCallableVtableSlot(base, r3.u32, 44);
}

bool FM2ReturnOnBadListHead(PPCRegister& r10) {
  uint8_t* base = GuestBase();
  return base && !GuestReadableRange(base, r10.u32, 4);
}

bool FM2ReturnOnBadD5A8ListHead(PPCRegister& r11) {
  uint8_t* base = GuestBase();
  return base && !GuestReadableRange(base, r11.u32, 4);
}

bool FM2ReturnOnBadD4F8ListHead(PPCRegister& r10) {
  uint8_t* base = GuestBase();
  return base && !GuestReadableRange(base, r10.u32, 4);
}

bool FM2ReturnOnBad75A40Object(PPCRegister& r30) {
  uint8_t* base = GuestBase();
  return base && !GuestReadableRange(base, r30.u32, 172);
}

bool FM2ReturnZeroOnBad76A58Object(PPCRegister& r30, PPCRegister& r29) {
  uint8_t* base = GuestBase();
  if (!base || GuestReadableRange(base, r30.u32, 156)) {
    return false;
  }

  r29.u64 = 0;
  return true;
}

bool FM2ReturnOnBad75ED0Mask(PPCRegister& r25) {
  uint8_t* base = GuestBase();
  return base && !GuestReadableRange(base, r25.u32, 124);
}

bool FM2ReturnOnBad40160PrimaryResult(PPCRegister& r25) {
  uint8_t* base = GuestBase();
  return base && (!HasCallableVtableSlot(base, r25.u32, 20) ||
                  !HasCallableVtableSlot(base, r25.u32, 8));
}

// --- FMOD pump wait-timeout override -----------------------------------------
// Deliberately kept (not a crash guard) -- overrides the guest FMOD pump's
// wait timeout to a short, fixed duration by default. Ported as-is from
// ReXGlue080plume/FM2/src/fm2_hooks.cpp, including its env-var configuration:
// set REX_FM2_PUMP_WAIT_MS=<n> (1-32) to change the timeout, <=0 to disable
// the override and let the guest's own computed timeout stand.
// See docs/migration-from-plume.md for why this needed an explicit decision
// before porting -- it was undocumented in the source repo's PATCHES.md.

namespace {

struct FmodPumpWaitConfig {
  bool override_enabled = true;
  int32_t override_ms = 8;
};

FmodPumpWaitConfig& FmodPumpWait() {
  static FmodPumpWaitConfig config;
  static std::once_flag init_once;
  std::call_once(init_once, [] {
    if (const char* wait_env = std::getenv("REX_FM2_PUMP_WAIT_MS"); wait_env && *wait_env) {
      const long v = std::strtol(wait_env, nullptr, 10);
      if (v <= 0) {
        config.override_enabled = false;
      } else if (v <= 32) {
        config.override_enabled = true;
        config.override_ms = static_cast<int32_t>(v);
      }
    }
  });
  return config;
}

}  // namespace

void FM2FmodPumpWaitPrep82381DE4(PPCRegister& r29) {
  const auto& config = FmodPumpWait();
  if (!config.override_enabled || r29.u32 == 0) {
    return;
  }

  int32_t ms = config.override_ms;
  if (ms < 1) {
    ms = 1;
  } else if (ms > 32) {
    ms = 32;
  }
  const int64_t wait_100ns = -static_cast<int64_t>(ms) * 10000ll;

  uint8_t* base = GuestBase();
  if (base && GuestReadableRange(base, r29.u32, 8u)) {
    REX_STORE_U64(r29.u32, static_cast<uint64_t>(wait_100ns));
  }
}

// --- Startup intro / splash QoL skips ----------------------------------------
// Not crash guards -- cosmetic skips, ported as-is from
// ReXGlue080plume/FM2/src/fm2_hooks.cpp. See docs/migration-from-plume.md.

bool FM2SkipStartupIntroWait(PPCRegister& r3, PPCRegister& r11, PPCRegister& r30) {
  (void)r3;
  return r11.u32 == r30.u32;
}

bool FM2FastForwardSplashTiming(PPCRegister& f1, PPCRegister& f2, PPCRegister& r7,
                                PPCRegister& r31) {
  (void)r7;
  (void)r31;

  constexpr double kFastForwardDurationSec = 0.05;
  const double in_f1 = f1.f64;
  const double in_f2 = f2.f64;

  if (std::isfinite(in_f1) && std::isfinite(in_f2) && in_f2 >= 0.0 &&
      in_f2 > (in_f1 + kFastForwardDurationSec)) {
    f2.f64 = in_f1 + kFastForwardDurationSec;
  }

  return false;
}
