#include "doax_hooks.h"

#include "generated/default/doax_init.h"

#include <cstdint>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/guest_pc_fiber.h>

namespace {

constexpr uint32_t kGuestFiberSwapAddress = 0x82785670u;

constexpr uint32_t kDoaxSchedulerFlagAddr =
    static_cast<uint32_t>(static_cast<int32_t>(-2093219840 + -29192));

constexpr int64_t kDoaxWorkQueueSegBase = -2092302336;
constexpr int64_t kDoaxWorkQueueTableBase = kDoaxWorkQueueSegBase + 12888;

constexpr uint32_t kSchedulerHeaderBytes = 16;

constexpr uint32_t kDoaxBootPresentByteAddr = 0x833BB763u;
constexpr uint8_t kDoaxMainMenuReadyPresent = 5u;

constexpr uint32_t kDoaxMenuFiberDeaAddr = 0x833B8DEAu;
constexpr uint32_t kDoaxMenuFiberDebAddr = 0x833B8DEBu;
constexpr uint32_t kDoaxMenuFiberDefAddr = 0x833B8DEFu;
constexpr uint32_t kDoaxIslandOverlayFlagAddr = 0x833B8514u;
constexpr uint32_t kDoaxPresentStateIndexAddr = 0x833B84C8u;

constexpr uint32_t kDispatcherFiberYieldLr = 0x824C0600u;
constexpr uint32_t kCdf8FiberYieldLr = 0x8258CE4Cu;
constexpr uint32_t kMenuWorkFiberYieldLr = 0x824C15F4u;
constexpr uint32_t kDrainInnerFiberYieldLr = 0x824C0C3Cu;
constexpr uint32_t kAltMenuWorkFiberYieldLr = 0x825A25E0u;
constexpr uint32_t kIslandBringUpCef0Lr = 0x8258E03Cu;
constexpr uint32_t kMenuTravelConfirmLr = 0x824C16A4u;
constexpr uint32_t kMenuPostConfirmCleanupLr = 0x824C17FCu;
constexpr uint32_t kMenuSceneTransitionLabel12Lr = 0x824C1770u;
constexpr uint32_t kMenuSceneTransitionLabel34Lr = 0x824C191Cu;
constexpr uint32_t kDoaxTravelMenuItemId = 15u;
constexpr uint32_t kMenuItemConfirmCase0Arg = 30u;
constexpr uint32_t kMenuIdleSchedulerMode4 = 5u;
constexpr uint32_t kMenuIdleSchedulerMode5 = 1u;
constexpr uint32_t kDoaxIslandGameplayPresentIndex = 3u;
constexpr uint32_t kTravelFadeMilestoneTimeline = 30u;
constexpr uint32_t kTravelFastCompleteDf0Threshold = 40u;
constexpr uint32_t kDoaxMenuItemIdByteAddr = 0x833B8DECu;
constexpr uint32_t kDoaxMenuTransitionDf0Addr = 0x833B8DF0u;
constexpr uint32_t kDoaxMenuTransitionDf4Addr = 0x833B8DF4u;
constexpr uint32_t kMenuWorkFiberCase3 = 3u;
constexpr uint32_t kTravelCase3ProbeLogCap = 64;
constexpr uint32_t kTravelOverlayGuardLogCap = 24;
constexpr uint32_t kTravelCompleteLogCap = 16;
constexpr uint32_t kMenuSceneTransitionLogCap = 8;
constexpr uint32_t kIslandSceneLoadLogCap = 8;

// ---------------------------------------------------------------------------
// Boot present-state globals (written by DOAX_BootPresentStateUpdate at
// 0x8250BEB0 and DOAX_BootWorkFiberBody at 0x8250A568).
//
// The teardown path inside DOAX_BootPresentStateUpdate sets these fields to
// force a state transition back to boot/loading (state 2). When we prevent
// the teardown function (DOAX_BootMovieReplayTeardown) from running, these
// fields still get written. We save/restore them to keep the boot fiber
// running the current state instead of resetting.
// ---------------------------------------------------------------------------
constexpr uint32_t kDoaxBootTargetStateAddr = 0x8341F9F6u;       // byte_8341F9F6 — target boot-present state (set to 2 by teardown)
constexpr uint32_t kDoaxBootStateChangeFlagAddr = 0x8342629Cu;    // dword_8342629C — non-zero = state change pending
constexpr uint32_t kDoaxBootTeardownNeededAddr = 0x8341F9F5u;    // byte_8341F9F5 — "movie replay teardown needed" flag
constexpr uint32_t kDoaxBootPresentHandlingAddr = 0x83984924u;   // dword_83984924 — "present state handler busy" latch

constexpr uint32_t kMenuFiberProbeLogCap = 16;
constexpr uint32_t kMenuFiberYieldLogCap = 48;
constexpr uint32_t kCef0ProbeLogCap = 8;
constexpr uint32_t kDrainMenuProbeLogCap = 32;
constexpr uint32_t kDrainStuckProbeInterval = 200;
constexpr uint32_t kSessionOutcomeLogCap = 32;
constexpr uint32_t kPlayMovieOutcomeLogCap = 16;

uint32_t g_session_run_id = 0;
uint32_t g_session_outcome_logs = 0;
uint32_t g_play_movie_outcome_logs = 0;
bool g_prevent_scene_teardown = false;
bool g_suppress_menu_kick = false;
bool g_suppress_boot_work_fiber = false;
bool g_menu_fiber_entered_this_run = false;
bool g_opening_played_this_run = false;
uint32_t g_boot_fiber_suppress_logs = 0;
uint32_t g_boot_fiber_run_logs = 0;
uint32_t g_boot_present_run_logs = 0;
uint32_t g_teardown_logs = 0;
constexpr uint32_t kBootFiberSuppressLogCap = 8;
constexpr uint32_t kBootFiberRunLogCap = 128;
constexpr uint32_t kBootPresentRunLogCap = 128;
constexpr uint32_t kTeardownLogCap = 16;
constexpr uint32_t kMovieProbeLogCap = 4;
constexpr const char* kDoaxHooksBuildTag = "doax-hooks-2026-06-24-travel-teardown-log";

struct SessionOutcomeState {
  bool menu_fiber_enter = false;
  bool menu_fiber_exit = false;
  bool opening_play = false;
  bool travel_confirm = false;
  uint32_t menu_kick_count = 0;
  uint32_t r28_zero_yields = 0;
};
SessionOutcomeState g_session_outcome;

bool g_travel_overlay_guard = false;
bool g_travel_fade_complete = false;

uint32_t g_travel_case3_probe_logs = 0;
uint32_t g_travel_overlay_guard_logs = 0;
uint32_t g_travel_complete_logs = 0;
uint32_t g_menu_scene_transition_logs = 0;
uint32_t g_island_scene_load_logs = 0;

// Suppress the boot work fiber during normal menu operation (non-Travel).
// Once Travel is confirmed (g_prevent_scene_teardown), the fiber must run
// to drive rendering and island scene initialization — only the teardown
// is blocked. After the fade completes, suppression stays off so the boot
// fiber can handle the gameplay state transition.
bool ShouldSuppressBootWorkFiber() {
  return g_suppress_boot_work_fiber && !g_travel_fade_complete &&
         !g_prevent_scene_teardown;
}

struct SchedulerSnapshot {
  uint8_t flag0 = 0;
  uint8_t flag1 = 0;
  uint8_t flag2 = 0;
  uint8_t flag3 = 0;
  uint8_t flag4 = 0;
  uint8_t flag5 = 0;
  uint8_t flag6 = 0;
  uint8_t flag7 = 0;
  uint32_t word8 = 0;
  uint32_t word12 = 0;
};

struct MenuFiberGlobals {
  uint8_t dea = 0;
  uint8_t deb = 0;
  uint8_t def = 0;
  uint8_t overlay = 0;
  uint8_t present = 0;
};

struct MenuFiberProbeState {
  uint32_t enter_count = 0;
  uint32_t log_count = 0;
};

struct MenuFiberYieldProbeState {
  uint32_t log_count = 0;
};

struct Cef0ProbeState {
  uint32_t call_count = 0;
  uint32_t log_count = 0;
};

struct DrainMenuProbeState {
  uint64_t call_count = 0;
  uint32_t log_count = 0;
  uint64_t stuck_calls_since_log = 0;
};

MenuFiberProbeState g_menu_fiber_probe;
MenuFiberYieldProbeState g_menu_fiber_yield_probe;
Cef0ProbeState g_cef0_probe;
DrainMenuProbeState g_drain_menu_probe;

SchedulerSnapshot ReadSchedulerSnapshot(uint8_t* base) {
  SchedulerSnapshot snap{};
  const uint32_t sched = kDoaxSchedulerFlagAddr;
  snap.flag0 = REX_LOAD_U8(sched + 0);
  snap.flag1 = REX_LOAD_U8(sched + 1);
  snap.flag2 = REX_LOAD_U8(sched + 2);
  snap.flag3 = REX_LOAD_U8(sched + 3);
  snap.flag4 = REX_LOAD_U8(sched + 4);
  snap.flag5 = REX_LOAD_U8(sched + 5);
  snap.flag6 = REX_LOAD_U8(sched + 6);
  snap.flag7 = REX_LOAD_U8(sched + 7);
  snap.word8 = REX_LOAD_U32(sched + 8);
  snap.word12 = REX_LOAD_U32(sched + 12);
  return snap;
}

void PreserveSchedulerExceptBytes45(uint8_t* base, const SchedulerSnapshot& before) {
  SchedulerSnapshot after = ReadSchedulerSnapshot(base);
  const uint32_t sched = kDoaxSchedulerFlagAddr;
  if (after.flag0 != before.flag0) {
    REX_STORE_U8(sched + 0, before.flag0);
  }
  if (after.flag1 != before.flag1) {
    REX_STORE_U8(sched + 1, before.flag1);
  }
  if (after.flag2 != before.flag2) {
    REX_STORE_U8(sched + 2, before.flag2);
  }
  if (after.flag3 != before.flag3) {
    REX_STORE_U8(sched + 3, before.flag3);
  }
  if (after.flag6 != before.flag6) {
    REX_STORE_U8(sched + 6, before.flag6);
  }
  if (after.flag7 != before.flag7) {
    REX_STORE_U8(sched + 7, before.flag7);
  }
  if (after.word8 != before.word8) {
    REX_STORE_U32(sched + 8, before.word8);
  }
  if (after.word12 != before.word12) {
    REX_STORE_U32(sched + 12, before.word12);
  }
}

bool IsIdleMainMenuSchedulerMode(const SchedulerSnapshot& snap) {
  return snap.flag4 == 5 && snap.flag5 == 1;
}

bool IsMenuBringUpSchedulerMode(const SchedulerSnapshot& snap) {
  return (snap.flag4 == 2 && snap.flag5 == 2) || (snap.flag4 == 3 && snap.flag5 == 3) ||
         (snap.flag4 == 2 && snap.flag5 == 0);
}

void RestoreSchedulerFlag0IfClobbered(uint8_t* base, const SchedulerSnapshot& before,
                                      SchedulerSnapshot& after) {
  if (before.flag0 == 0 || after.flag0 != 0) {
    return;
  }
  REX_STORE_U8(kDoaxSchedulerFlagAddr + 0, before.flag0);
  after.flag0 = before.flag0;
}

void MaybeLogSessionOutcome(uint8_t* base, const char* event, const char* detail = "") {
  if (g_session_outcome_logs >= kSessionOutcomeLogCap) {
    return;
  }
  ++g_session_outcome_logs;
  const SchedulerSnapshot sched = ReadSchedulerSnapshot(base);
  REXKRNL_WARN(
      "DOAX session-outcome run={} event={} detail={} menu_kick={} menu_fiber={} "
      "opening={} travel_confirm={} r28_zero_yields={} sched f2={} f4/5={}/{} boot_present={}",
      g_session_run_id, event, detail, g_session_outcome.menu_kick_count,
      g_session_outcome.menu_fiber_enter ? 1 : 0, g_session_outcome.opening_play ? 1 : 0,
      g_session_outcome.travel_confirm ? 1 : 0, g_session_outcome.r28_zero_yields,
      sched.flag2, sched.flag4, sched.flag5, REX_LOAD_U8(kDoaxBootPresentByteAddr));
}

uint8_t* g_doax_hook_guest_base = nullptr;

bool IsIslandHubMenuReady(uint8_t* base) {
  if (REX_LOAD_U8(kDoaxBootPresentByteAddr) != kDoaxMainMenuReadyPresent) {
    return false;
  }
  if (REX_LOAD_U8(kDoaxIslandOverlayFlagAddr) != 1) {
    return false;
  }
  return IsIdleMainMenuSchedulerMode(ReadSchedulerSnapshot(base));
}

// Arm flag2 only once island hub overlay is up (after Press Start). boot_present==5 is set
// at promotion-complete, before Press Start — do not use it alone.
void TryArmMenuDispatchAtCef0(uint8_t* base, uint32_t caller_lr) {
  if (g_suppress_menu_kick) {
    return;
  }
  if (caller_lr != kIslandBringUpCef0Lr) {
    return;
  }
  if (!IsIslandHubMenuReady(base)) {
    return;
  }
  const SchedulerSnapshot snap = ReadSchedulerSnapshot(base);
  if (snap.flag0 == 0 || snap.flag2 != 0) {
    return;
  }
  REX_STORE_U8(kDoaxSchedulerFlagAddr + 2, 1);
  ++g_session_outcome.menu_kick_count;
  MaybeLogSessionOutcome(base, "menu-kick", "cef0-idle");
  REXKRNL_WARN(
      "DOAX menu-kick: armed flag2 run={} lr=0x{:08X} sched={}/{} overlay=1",
      g_session_run_id, caller_lr, snap.flag4, snap.flag5);
}

void TryRearmMenuDispatchAfterDrain(uint8_t* base, const SchedulerSnapshot& after) {
  if (g_suppress_menu_kick) {
    return;
  }
  if (!IsIslandHubMenuReady(base)) {
    return;
  }
  if (after.flag0 == 0 || after.flag2 != 0) {
    return;
  }
  REX_STORE_U8(kDoaxSchedulerFlagAddr + 2, 1);
  ++g_session_outcome.menu_kick_count;
  MaybeLogSessionOutcome(base, "menu-kick", "drain-idle");
}

void MaybeLogBootFiberSuppress(uint32_t caller_lr, const char* site) {
  if (g_boot_fiber_suppress_logs >= kBootFiberSuppressLogCap) {
    return;
  }
  ++g_boot_fiber_suppress_logs;
  REXKRNL_WARN("DOAX boot-fiber-suppress: skip {} lr=0x{:08X} run={}", site, caller_lr,
               g_session_run_id);
}

struct MenuTravelProbeState {
  uint8_t deb = 0;
  uint8_t def = 0;
  uint8_t dea = 0;
  uint8_t overlay = 0;
  uint8_t menu_item = 0;
  uint32_t present_idx = 0;
  uint32_t df0 = 0;
  uint32_t df4 = 0;
};

MenuTravelProbeState ReadMenuTravelProbeState(uint8_t* base) {
  (void)base;
  MenuTravelProbeState state;
  state.deb = REX_LOAD_U8(kDoaxMenuFiberDebAddr);
  state.def = REX_LOAD_U8(kDoaxMenuFiberDefAddr);
  state.dea = REX_LOAD_U8(kDoaxMenuFiberDeaAddr);
  state.overlay = REX_LOAD_U8(kDoaxIslandOverlayFlagAddr);
  state.menu_item = REX_LOAD_U8(kDoaxMenuItemIdByteAddr);
  state.present_idx = REX_LOAD_U32(kDoaxPresentStateIndexAddr);
  state.df0 = REX_LOAD_U32(kDoaxMenuTransitionDf0Addr);
  state.df4 = REX_LOAD_U32(kDoaxMenuTransitionDf4Addr);
  return state;
}

bool ShouldHoldTravelMenuPresentState(uint8_t* base) {
  if (g_travel_fade_complete) {
    return false;
  }
  const uint32_t timeline = REX_LOAD_U32(kDoaxMenuTransitionDf4Addr);
  if (timeline != 0xFFFFFFFFu && timeline <= kTravelFadeMilestoneTimeline) {
    return false;
  }
  return true;
}

void ApplyTravelTransitionGuards(uint8_t* base, const char* site) {
  g_travel_overlay_guard = true;
  const uint32_t before_present = REX_LOAD_U32(kDoaxPresentStateIndexAddr);
  const uint8_t before_overlay = REX_LOAD_U8(kDoaxIslandOverlayFlagAddr);
  if (ShouldHoldTravelMenuPresentState(base) && before_present != 1) {
    REX_STORE_U32(kDoaxPresentStateIndexAddr, 1);
  }
  if (before_overlay == 0) {
    REX_STORE_U8(kDoaxIslandOverlayFlagAddr, 1);
  }
  if (g_travel_overlay_guard_logs < kTravelOverlayGuardLogCap) {
    ++g_travel_overlay_guard_logs;
    REXKRNL_WARN(
        "DOAX travel-guard: arm site={} present {}->{} overlay {}->1",
        site, before_present,
        ShouldHoldTravelMenuPresentState(base) ? 1u : before_present, before_overlay);
  }
}

void EnforceTravelOverlayGuard(uint8_t* base, const char* site) {
  if (!g_travel_overlay_guard || g_travel_fade_complete) {
    return;
  }
  bool restored = false;
  if (ShouldHoldTravelMenuPresentState(base) &&
      REX_LOAD_U32(kDoaxPresentStateIndexAddr) != 1) {
    REX_STORE_U32(kDoaxPresentStateIndexAddr, 1);
    restored = true;
  }
  if (REX_LOAD_U8(kDoaxIslandOverlayFlagAddr) == 0) {
    REX_STORE_U8(kDoaxIslandOverlayFlagAddr, 1);
    restored = true;
  }
  if (restored && g_travel_overlay_guard_logs < kTravelOverlayGuardLogCap) {
    ++g_travel_overlay_guard_logs;
    const MenuTravelProbeState state = ReadMenuTravelProbeState(base);
    REXKRNL_WARN(
        "DOAX travel-guard: restore site={} deb={} def={} dea={} overlay={} present={} "
        "df0={} df4={}",
        site, state.deb, state.def, state.dea, state.overlay, state.present_idx, state.df0,
        state.df4);
  }
}

void ArmTravelFadeMilestone(uint8_t* base, int32_t timeline, const char* site) {
  if (!g_travel_overlay_guard || g_travel_fade_complete) {
    return;
  }
  if (REX_LOAD_U8(kDoaxMenuFiberDeaAddr) != kMenuWorkFiberCase3) {
    return;
  }
  if (timeline < 0) {
    return;
  }
  if (REX_LOAD_U8(kDoaxMenuFiberDefAddr) != 0) {
    return;
  }
  REX_STORE_U8(kDoaxMenuFiberDefAddr, 1);
  if (g_travel_complete_logs < kTravelCompleteLogCap) {
    ++g_travel_complete_logs;
    REXKRNL_WARN("DOAX travel-complete: milestone site={} timeline={} def=1", site, timeline);
  }
}

void CompleteTravelFade(uint8_t* base, int32_t timeline, const char* site) {
  if (!g_travel_overlay_guard || g_travel_fade_complete) {
    return;
  }
  if (timeline > 0) {
    return;
  }
  g_travel_fade_complete = true;
  g_travel_overlay_guard = false;
  g_prevent_scene_teardown = false;
  const uint32_t before_present = REX_LOAD_U32(kDoaxPresentStateIndexAddr);
  if (REX_LOAD_U8(kDoaxMenuFiberDefAddr) == 0) {
    REX_STORE_U8(kDoaxMenuFiberDefAddr, 1);
  }
  REX_STORE_U32(kDoaxPresentStateIndexAddr, kDoaxIslandGameplayPresentIndex);
  if (g_travel_complete_logs < kTravelCompleteLogCap) {
    ++g_travel_complete_logs;
    const MenuTravelProbeState state = ReadMenuTravelProbeState(base);
    const uint32_t gamma_mul_raw = REX_LOAD_U32(0x834A2C20u);
    const uint32_t gamma_bak_raw = REX_LOAD_U32(0x83961314u);
    float gamma_mul, gamma_bak;
    static_assert(sizeof(float) == sizeof(uint32_t), "");
    memcpy(&gamma_mul, &gamma_mul_raw, sizeof(float));
    memcpy(&gamma_bak, &gamma_bak_raw, sizeof(float));
    REXKRNL_WARN(
        "DOAX travel-complete: fade-done site={} timeline={} present {}->{} deb={} def={} "
        "dea={} overlay={} df0={} df4={} gamma={:.4f} gamma_bak={:.4f}",
        site, timeline, before_present, kDoaxIslandGameplayPresentIndex, state.deb, state.def,
        state.dea, state.overlay, state.df0, state.df4, gamma_mul, gamma_bak);
  }
}

void MaybeFastCompleteTravelFade(uint8_t* base, const char* site) {
  if (!g_travel_overlay_guard || g_travel_fade_complete) {
    return;
  }
  if (REX_LOAD_U8(kDoaxMenuFiberDeaAddr) != kMenuWorkFiberCase3) {
    return;
  }
  const uint32_t df0 = REX_LOAD_U32(kDoaxMenuTransitionDf0Addr);
  if (df0 < kTravelFastCompleteDf0Threshold) {
    return;
  }
  CompleteTravelFade(base, 0, site);
}

void MaybeLogTravelCase3Probe(uint8_t* base, const char* site, int32_t ready, int32_t fade,
                              int32_t timeline, uint32_t caller_lr) {
  if (!g_travel_overlay_guard) {
    return;
  }
  const MenuTravelProbeState state = ReadMenuTravelProbeState(base);
  if (state.dea != kMenuWorkFiberCase3) {
    return;
  }
  if (g_travel_case3_probe_logs >= kTravelCase3ProbeLogCap) {
    return;
  }
  ++g_travel_case3_probe_logs;
  REXKRNL_WARN(
      "DOAX travel-case3-probe site={} deb={} def={} dea={} overlay={} item={} present={} "
      "df0={} df4={} ready={} fade={} timeline={} lr=0x{:08X}",
      site, state.deb, state.def, state.dea, state.overlay, state.menu_item, state.present_idx,
      state.df0, state.df4, ready, fade, timeline, caller_lr);
}

void ArmBootFiberSuppress(uint8_t* base, const char* site, uint32_t lr, uint32_t detail) {
  if (g_suppress_boot_work_fiber) {
    ApplyTravelTransitionGuards(base, site);
    return;
  }
  g_suppress_boot_work_fiber = true;
  g_session_outcome.travel_confirm = true;
  ApplyTravelTransitionGuards(base, site);
  REXKRNL_WARN(
      "DOAX travel-transition: arm suppress site={} detail={} lr=0x{:08X} "
      "boot_present={} prevent_teardown={} sched={}/{}",
      site, detail, lr, REX_LOAD_U8(kDoaxBootPresentByteAddr),
      g_prevent_scene_teardown ? 1 : 0,
      ReadSchedulerSnapshot(base).flag4, ReadSchedulerSnapshot(base).flag5);
}

bool IsMenuWorkFiberConfirm(uint32_t caller_lr) {
  return caller_lr == kMenuTravelConfirmLr || caller_lr == kMenuPostConfirmCleanupLr;
}

void MaybeArmBootFiberForTravel(uint8_t* base, const char* site, uint32_t lr, uint32_t detail,
                                uint32_t item_id, uint32_t arg2) {
  if (g_suppress_boot_work_fiber) {
    return;
  }
  if (arg2 == 0) {
    ArmBootFiberSuppress(base, site, lr, detail);
    return;
  }
  if (item_id != kDoaxTravelMenuItemId || arg2 != kMenuItemConfirmCase0Arg ||
      lr != kMenuTravelConfirmLr) {
    return;
  }
  const SchedulerSnapshot snap = ReadSchedulerSnapshot(base);
  const bool idle_menu =
      snap.flag4 == kMenuIdleSchedulerMode4 && snap.flag5 == kMenuIdleSchedulerMode5;
  const bool confirm_frame = snap.flag4 == 2 && snap.flag5 == 2;
  if (idle_menu || confirm_frame) {
    // Only arm teardown prevention here — NOT in ArmBootFiberSuppress itself,
    // because ArmBootFiberSuppress is also called from DOAX_MenuSceneTransition
    // during normal menu bring-up (Press Start → 4-menu), where teardown must
    // be allowed to fire.
    g_prevent_scene_teardown = true;
    ArmBootFiberSuppress(base, site, lr, item_id);
  }
}

MenuFiberGlobals ReadMenuFiberGlobals(uint8_t* base) {
  (void)base;
  MenuFiberGlobals g{};
  g.dea = REX_LOAD_U8(kDoaxMenuFiberDeaAddr);
  g.deb = REX_LOAD_U8(kDoaxMenuFiberDebAddr);
  g.def = REX_LOAD_U8(kDoaxMenuFiberDefAddr);
  g.overlay = REX_LOAD_U8(kDoaxIslandOverlayFlagAddr);
  g.present = REX_LOAD_U8(kDoaxPresentStateIndexAddr);
  return g;
}

void LogSchedulerFlags(const char* prefix, const SchedulerSnapshot& snap) {
  REXKRNL_WARN(
      "{} f0-7={:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X} w8={} w12={}", prefix,
      snap.flag0, snap.flag1, snap.flag2, snap.flag3, snap.flag4, snap.flag5, snap.flag6,
      snap.flag7, snap.word8, snap.word12);
}

void LogMenuFiberGlobals(const char* prefix, const MenuFiberGlobals& g) {
  REXKRNL_WARN("{} dea={} deb={} def={} overlay={} present={}", prefix, g.dea, g.deb, g.def,
               g.overlay, g.present);
}

void MaybeLogMenuFiberSite(const char* site, uint8_t* base, const PPCContext& ctx) {
  if (g_menu_fiber_probe.log_count >= kMenuFiberProbeLogCap) {
    return;
  }
  ++g_menu_fiber_probe.log_count;
  const SchedulerSnapshot sched = ReadSchedulerSnapshot(base);
  const MenuFiberGlobals globals = ReadMenuFiberGlobals(base);
  REXKRNL_WARN(
      "DOAX menu-fiber-probe site={} enter={} r28=0x{:08X} r30=0x{:08X} r31=0x{:08X} "
      "boot_present={}",
      site, g_menu_fiber_probe.enter_count, ctx.r28.u32, ctx.r30.u32, ctx.r31.u32,
      REX_LOAD_U8(kDoaxBootPresentByteAddr));
  LogSchedulerFlags("DOAX menu-fiber-probe sched", sched);
  LogMenuFiberGlobals("DOAX menu-fiber-probe", globals);
}

void MaybeLogMenuFiberYield(uint8_t* base, uint32_t yield_index, uint64_t r28_before,
                            uint64_t r28_after, bool preserved_gprs) {
  if (g_menu_fiber_yield_probe.log_count >= kMenuFiberYieldLogCap) {
    return;
  }
  ++g_menu_fiber_yield_probe.log_count;
  const SchedulerSnapshot sched = ReadSchedulerSnapshot(base);
  const MenuFiberGlobals globals = ReadMenuFiberGlobals(base);
  REXKRNL_WARN(
      "DOAX menu-fiber-yield n={} r28 0x{:08X}->0x{:08X} gpr_preserve={} boot_present={}",
      yield_index, static_cast<uint32_t>(r28_before), static_cast<uint32_t>(r28_after),
      preserved_gprs ? 1 : 0, REX_LOAD_U8(kDoaxBootPresentByteAddr));
  LogSchedulerFlags("DOAX menu-fiber-yield sched", sched);
  LogMenuFiberGlobals("DOAX menu-fiber-yield", globals);
}

void MaybeLogCef0Probe(uint8_t* base, uint32_t caller_lr, const char* phase) {
  if (g_cef0_probe.log_count >= kCef0ProbeLogCap) {
    return;
  }
  ++g_cef0_probe.log_count;
  const SchedulerSnapshot sched = ReadSchedulerSnapshot(base);
  REXKRNL_WARN("DOAX cef0-probe phase={} call={} lr=0x{:08X} boot_present={}", phase,
               g_cef0_probe.call_count, caller_lr, REX_LOAD_U8(kDoaxBootPresentByteAddr));
  LogSchedulerFlags("DOAX cef0-probe sched", sched);
  LogMenuFiberGlobals("DOAX cef0-probe", ReadMenuFiberGlobals(base));
}

bool IsStuckMainMenuDrain(const SchedulerSnapshot& snap) {
  return snap.flag0 != 0 && snap.flag2 == 0 && snap.flag4 == 5 && snap.flag5 == 1;
}

bool IsIslandHubStuckDrain(uint8_t* base, const SchedulerSnapshot& snap) {
  return REX_LOAD_U8(kDoaxIslandOverlayFlagAddr) == 1 && IsStuckMainMenuDrain(snap);
}

void MaybeLogDrainMenuProbe(uint8_t* base, const SchedulerSnapshot& before,
                            const SchedulerSnapshot& after, uint32_t caller_lr,
                            const char* reason) {
  if (g_drain_menu_probe.log_count >= kDrainMenuProbeLogCap) {
    return;
  }
  ++g_drain_menu_probe.log_count;
  REXKRNL_WARN(
      "DOAX drain-menu-probe call={} reason={} caller_lr=0x{:08X} boot_present={}",
      g_drain_menu_probe.call_count, reason, caller_lr,
      REX_LOAD_U8(kDoaxBootPresentByteAddr));
  LogSchedulerFlags("DOAX drain-menu-probe before", before);
  LogSchedulerFlags("DOAX drain-menu-probe after ", after);
  LogMenuFiberGlobals("DOAX drain-menu-probe", ReadMenuFiberGlobals(base));
}

void ProbeDrainMenuDispatch(uint8_t* base, const SchedulerSnapshot& before,
                            const SchedulerSnapshot& after, uint32_t caller_lr) {
  auto& probe = g_drain_menu_probe;
  ++probe.call_count;

  const bool boot_present =
      REX_LOAD_U8(kDoaxBootPresentByteAddr) == kDoaxMainMenuReadyPresent;
  const bool menu_armed = before.flag2 != 0 || after.flag2 != 0;
  const bool flag2_cleared = before.flag2 != 0 && after.flag2 == 0;
  const bool stuck = boot_present && IsIslandHubStuckDrain(base, after);

  if (menu_armed) {
    MaybeLogDrainMenuProbe(base, before, after, caller_lr,
                           flag2_cleared ? "flag2_cleared" : "flag2_active");
    probe.stuck_calls_since_log = 0;
    return;
  }

  if (stuck) {
    ++probe.stuck_calls_since_log;
    if (probe.call_count <= 8 || probe.stuck_calls_since_log >= kDrainStuckProbeInterval) {
      probe.stuck_calls_since_log = 0;
      MaybeLogDrainMenuProbe(base, before, after, caller_lr, "stuck_flag2_zero");
    }
  }
}

struct SchedulerFiberGprs {
  uint64_t r14 = 0;
  uint64_t r15 = 0;
  uint64_t r16 = 0;
  uint64_t r17 = 0;
  uint64_t r18 = 0;
  uint64_t r19 = 0;
  uint64_t r20 = 0;
  uint64_t r21 = 0;
  uint64_t r22 = 0;
  uint64_t r23 = 0;
  uint64_t r24 = 0;
  uint64_t r25 = 0;
  uint64_t r26 = 0;
  uint64_t r27 = 0;
  uint64_t r28 = 0;
  uint64_t r29 = 0;
  uint64_t r30 = 0;
  uint64_t r31 = 0;
};

// Only menu work-fiber loop yields need r14-r31 preserved across guest-PC swap.
// Universal restore (or restore on boot/press-start yields) wedges input handling.
bool NeedsFiberCalleeSavePreserve(uint32_t caller_lr) {
  switch (caller_lr) {
    case kMenuWorkFiberYieldLr:
    case kDrainInnerFiberYieldLr:
    case kAltMenuWorkFiberYieldLr:
      return true;
    default:
      return false;
  }
}

void SaveSchedulerFiberGprs(PPCContext& ctx, SchedulerFiberGprs& saved) {
  saved.r14 = ctx.r14.u64;
  saved.r15 = ctx.r15.u64;
  saved.r16 = ctx.r16.u64;
  saved.r17 = ctx.r17.u64;
  saved.r18 = ctx.r18.u64;
  saved.r19 = ctx.r19.u64;
  saved.r20 = ctx.r20.u64;
  saved.r21 = ctx.r21.u64;
  saved.r22 = ctx.r22.u64;
  saved.r23 = ctx.r23.u64;
  saved.r24 = ctx.r24.u64;
  saved.r25 = ctx.r25.u64;
  saved.r26 = ctx.r26.u64;
  saved.r27 = ctx.r27.u64;
  saved.r28 = ctx.r28.u64;
  saved.r29 = ctx.r29.u64;
  saved.r30 = ctx.r30.u64;
  saved.r31 = ctx.r31.u64;
}

void RestoreSchedulerFiberGprs(PPCContext& ctx, const SchedulerFiberGprs& saved) {
  ctx.r14.u64 = saved.r14;
  ctx.r15.u64 = saved.r15;
  ctx.r16.u64 = saved.r16;
  ctx.r17.u64 = saved.r17;
  ctx.r18.u64 = saved.r18;
  ctx.r19.u64 = saved.r19;
  ctx.r20.u64 = saved.r20;
  ctx.r21.u64 = saved.r21;
  ctx.r22.u64 = saved.r22;
  ctx.r23.u64 = saved.r23;
  ctx.r24.u64 = saved.r24;
  ctx.r25.u64 = saved.r25;
  ctx.r26.u64 = saved.r26;
  ctx.r27.u64 = saved.r27;
  ctx.r28.u64 = saved.r28;
  ctx.r29.u64 = saved.r29;
  ctx.r30.u64 = saved.r30;
  ctx.r31.u64 = saved.r31;
}

void SaveSchedulerHeaderBytes(uint8_t* base, uint8_t* out) {
  for (uint32_t i = 0; i < kSchedulerHeaderBytes; ++i) {
    out[i] = REX_LOAD_U8(kDoaxSchedulerFlagAddr + i);
  }
}

void WriteSchedulerHeaderBytes(uint8_t* base, const uint8_t* in) {
  for (uint32_t i = 0; i < kSchedulerHeaderBytes; ++i) {
    REX_STORE_U8(kDoaxSchedulerFlagAddr + i, in[i]);
  }
}

void SchedulerGuardedIndirectCall(PPCContext& ctx, uint8_t* base, uint32_t target,
                                  uint64_t return_lr) {
  uint8_t sched_before[kSchedulerHeaderBytes];
  SaveSchedulerHeaderBytes(base, sched_before);
  const uint64_t saved_r30 = ctx.r30.u64;
  const uint64_t saved_r31 = ctx.r31.u64;
  ctx.lr = return_lr;
  REX_CALL_INDIRECT_FUNC(target);
  bool reg_fix = false;
  if (saved_r31 != 0 && ctx.r31.u64 != saved_r31) {
    reg_fix = true;
    ctx.r31.u64 = saved_r31;
  }
  if (saved_r30 != 0 && ctx.r30.u64 == 0) {
    reg_fix = true;
    ctx.r30.u64 = saved_r30;
  }
  if (reg_fix) {
    WriteSchedulerHeaderBytes(base, sched_before);
  }
}

void RestoreDrainCallerRegs(PPCContext& ctx, uint64_t caller_lr, uint64_t caller_r29,
                            uint64_t caller_r30, uint64_t caller_r31) {
  if (static_cast<uint32_t>(caller_lr) == 0x824C0604) {
    ctx.r29.u64 = kDoaxWorkQueueSegBase;
    ctx.r30.u64 = kDoaxWorkQueueSegBase;
    ctx.r31.u64 = kDoaxWorkQueueTableBase;
  } else {
    if (caller_r29 != 0) {
      ctx.r29.u64 = caller_r29;
    }
    if (caller_r30 != 0) {
      ctx.r30.u64 = caller_r30;
    }
    if (caller_r31 != 0) {
      ctx.r31.u64 = caller_r31;
    }
  }
  if (caller_lr != 0) {
    ctx.lr = caller_lr;
  }
}

void FiberSwapImpl82785670(PPCContext& ctx, uint8_t* base) {
  __imp__DOAX_FiberContextSwitch(ctx, base);
}

void RegisterDoaxGuestPcFiberConfig(const rex::PPCImageInfo& image_info) {
  rex::ppc::GuestPcFiberConfig config;
  config.guest_regions.push_back(
      {image_info.image_base, image_info.image_base + image_info.image_size});
  rex::ppc::RegisterGuestPcFiberConfig(std::move(config));
}

}  // namespace

void InstallDoaxGuestPcFiber(const rex::PPCImageInfo& image_info) {
  RegisterDoaxGuestPcFiberConfig(image_info);
  rex::ppc::InstallGuestPcFiberInterpreter();
  ++g_session_run_id;
  g_suppress_menu_kick = false;
  g_suppress_boot_work_fiber = false;
  g_travel_overlay_guard = false;
  g_travel_fade_complete = false;
  g_prevent_scene_teardown = false;
  g_travel_case3_probe_logs = 0;
  g_travel_overlay_guard_logs = 0;
  g_travel_complete_logs = 0;
  g_menu_scene_transition_logs = 0;
  g_island_scene_load_logs = 0;
  g_boot_fiber_suppress_logs = 0;
  g_boot_fiber_run_logs = 0;
  g_boot_present_run_logs = 0;
  g_teardown_logs = 0;
  g_menu_fiber_entered_this_run = false;
  g_opening_played_this_run = false;
  g_session_outcome = {};
  g_session_outcome_logs = 0;
  REXLOG_INFO("DOAX guest-PC fiber: swap override active for 0x{:08X} session_run={}",
              kGuestFiberSwapAddress, g_session_run_id);
  REXKRNL_WARN("DOAX session-start run={} hooks={}", g_session_run_id, kDoaxHooksBuildTag);
}

extern "C" REX_FUNC(DOAX_FiberContextSwitch) {
  rex::ppc::RunFiberSwap(ctx, base, &FiberSwapImpl82785670, static_cast<uint32_t>(ctx.r3.u64),
                         0);
}

extern "C" REX_FUNC(DOAX_FiberYield) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  const bool is_menu_fiber_yield = caller_lr == kMenuWorkFiberYieldLr;
  const uint64_t r28_before = ctx.r28.u64;
  static uint32_t menu_fiber_yield_index = 0;

  SchedulerFiberGprs saved_gprs{};
  const bool preserve_gprs = NeedsFiberCalleeSavePreserve(caller_lr);
  if (preserve_gprs) {
    SaveSchedulerFiberGprs(ctx, saved_gprs);
  }
  DOAX_FiberContextSwitch(ctx, base);
  if (preserve_gprs) {
    RestoreSchedulerFiberGprs(ctx, saved_gprs);
  }

  if (is_menu_fiber_yield) {
    ++menu_fiber_yield_index;
    if (r28_before != 0 && ctx.r28.u64 == 0) {
      ++g_session_outcome.r28_zero_yields;
      MaybeLogSessionOutcome(base, "r28-clobber", "menu-fiber-yield");
    }
    MaybeLogMenuFiberYield(base, menu_fiber_yield_index, r28_before, ctx.r28.u64,
                           preserve_gprs);
    EnforceTravelOverlayGuard(base, "menu-fiber-yield");
    MaybeFastCompleteTravelFade(base, "menu-fiber-yield");
    MaybeLogTravelCase3Probe(base, "menu-fiber-yield", -1, -1, -1, caller_lr);
  }
}

extern "C" REX_FUNC(DOAX_WorkQueueSlotWake) {
  const uint64_t caller_r31 = ctx.r31.u64;
  const uint64_t caller_r30 = ctx.r30.u64;
  __imp__DOAX_WorkQueueSlotWake(ctx, base);
  if (caller_r31 != 0) {
    ctx.r31.u64 = caller_r31;
  } else {
    ctx.r31.u64 = kDoaxSchedulerFlagAddr;
  }
  if (caller_r30 != 0 && ctx.r30.u64 == 0) {
    ctx.r30.u64 = caller_r30;
  }
}

extern "C" REX_FUNC(DOAX_SchedulerDrainWake) {
  REX_FUNC_PROLOGUE(DOAX_SchedulerDrainWake);
  uint32_t ea{};
  ctx.r12.u64 = ctx.lr;
  REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
  REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
  ea = -96 + ctx.r1.u32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;
  ctx.r11.s64 = -2093219840;
  ctx.r31.s64 = ctx.r11.s64 + -29192;
  ctx.r11.u64 = REX_LOAD_U8(ctx.r31.u32 + 0);
  ctx.cr6.compare<uint32_t>(ctx.r11.u32, 1, ctx.xer);
  if (!ctx.cr6.eq) {
    goto loc_824C0910_hook;
  }
  ctx.r11.s64 = -2101149696;
  ctx.r10.u64 = REX_LOAD_U8(ctx.r31.u32 + 4);
  ctx.r3.u64 = ctx.r31.u64;
  ctx.r11.s64 = ctx.r11.s64 + -28360;
  ctx.r10.u64 = __builtin_rotateleft32(ctx.r10.u32, 4);
  ctx.r11.s64 = ctx.r11.s64 + 12;
  ctx.r11.u64 = REX_LOAD_U32(ctx.r10.u32 + ctx.r11.u32);
  ctx.ctr.u64 = ctx.r11.u64;
  ctx.lr = 0x824C0900;
  SchedulerGuardedIndirectCall(ctx, base, ctx.ctr.u32, 0x824C0900);
  ctx.r31.u64 = kDoaxSchedulerFlagAddr;
  ctx.r3.s64 = 1;
  ctx.lr = 0x824C0908;
  DOAX_WorkQueueSlotWake(ctx, base);
  ctx.r31.u64 = kDoaxSchedulerFlagAddr;
  ctx.r11.s64 = 0;
  REX_STORE_U8(ctx.r31.u32 + 0, ctx.r11.u8);
loc_824C0910_hook:
  ctx.r1.s64 = ctx.r1.s64 + 96;
  ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
  ctx.lr = ctx.r12.u64;
  ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
}

extern "C" REX_FUNC(DOAX_SchedulerDrainDispatch) {
  const uint64_t caller_lr = ctx.lr;
  const uint64_t caller_r29 = ctx.r29.u64;
  const uint64_t caller_r30 = ctx.r30.u64;
  const uint64_t caller_r31 = ctx.r31.u64;
  const SchedulerSnapshot before = ReadSchedulerSnapshot(base);
  __imp__DOAX_SchedulerDrainDispatch(ctx, base);
  SchedulerSnapshot after = ReadSchedulerSnapshot(base);
  ProbeDrainMenuDispatch(base, before, after, static_cast<uint32_t>(caller_lr));
  RestoreDrainCallerRegs(ctx, caller_lr, caller_r29, caller_r30, caller_r31);
}

extern "C" REX_FUNC(DOAX_SchedulerFiberSwap) {
  __imp__DOAX_SchedulerFiberSwap(ctx, base);
}

extern "C" REX_FUNC(sub_8258CDF8) {
  REX_FUNC_PROLOGUE(sub_8258CDF8);
  uint32_t ea{};
  ctx.r12.u64 = ctx.lr;
  REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
  REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
  ea = -96 + ctx.r1.u32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;
  ctx.r10.s64 = kDoaxWorkQueueSegBase;
  ctx.r11.s64 = kDoaxWorkQueueTableBase;
  ctx.r10.u64 = REX_LOAD_U32(ctx.r10.u32 + 13560);
  ctx.r10.s64 = static_cast<int64_t>(ctx.r10.u64 * static_cast<uint64_t>(28));
  ctx.r31.u64 = ctx.r10.u64 + ctx.r11.u64;
  const uint64_t work_entry = ctx.r31.u64;
  ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 24);
  if (ctx.r11.u32 != 0) {
    SchedulerGuardedIndirectCall(ctx, base, ctx.r11.u32, 0x8258CE34);
  }
  ctx.r31.u64 = work_entry;
  ctx.r11.s64 = kDoaxWorkQueueSegBase;
  ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
  ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 12884);
  ctx.r11.u64 = ctx.r10.u64 | 8;
  REX_STORE_U32(ctx.r31.u32 + 0, ctx.r11.u32);
  ctx.lr = 0x8258CE4C;
  DOAX_FiberYield(ctx, base);
  ctx.r31.u64 = work_entry;
  ctx.r1.s64 = ctx.r1.s64 + 96;
  ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
  ctx.lr = ctx.r12.u64;
  ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
}

extern "C" REX_FUNC(DOAX_MainMenuRegisterSlots) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  ++g_cef0_probe.call_count;
  MaybeLogCef0Probe(base, caller_lr, "before");
  __imp__DOAX_MainMenuRegisterSlots(ctx, base);
  MaybeLogCef0Probe(base, caller_lr, "after");
}

extern "C" REX_FUNC(DOAX_MenuItemConfirm) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t item_id = static_cast<uint32_t>(ctx.r3.u64);
  const uint32_t arg2 = static_cast<uint32_t>(ctx.r4.u64);
  if (caller_lr == kMenuPostConfirmCleanupLr) {
    MaybeArmBootFiberForTravel(base, "menu-post-confirm", caller_lr, item_id, item_id, arg2);
    g_suppress_menu_kick = true;
    REXKRNL_WARN("DOAX menu-select: post-confirm run={} lr=0x{:08X} item_id={}", g_session_run_id,
                 caller_lr, item_id);
  } else if (IsMenuWorkFiberConfirm(caller_lr)) {
    g_suppress_menu_kick = true;
    MaybeArmBootFiberForTravel(base, "menu-travel-confirm", caller_lr, item_id, item_id, arg2);
    if (item_id == kDoaxTravelMenuItemId && arg2 == kMenuItemConfirmCase0Arg) {
      MaybeLogSessionOutcome(base, "travel-confirm", "16a4-travel");
      REXKRNL_WARN("DOAX menu-select: travel run={} lr=0x{:08X} item_id={}", g_session_run_id,
                   caller_lr, item_id);
    }
  }
  __imp__DOAX_MenuItemConfirm(ctx, base);
}

extern "C" REX_FUNC(DOAX_PlayMovie) {
  const uint32_t movie_idx = static_cast<uint32_t>(ctx.r3.u64);
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  if (movie_idx == 0) {
    g_session_outcome.opening_play = true;
    g_opening_played_this_run = true;
    MaybeLogSessionOutcome(base, "opening-play", "PlayMovie(0)");
    REXKRNL_WARN("DOAX opening-play: run={} lr=0x{:08X} boot_present={}", g_session_run_id,
                 caller_lr, REX_LOAD_U8(kDoaxBootPresentByteAddr));
  } else if (g_play_movie_outcome_logs < kPlayMovieOutcomeLogCap) {
    ++g_play_movie_outcome_logs;
    REXKRNL_WARN("DOAX movie-play: run={} idx={} lr=0x{:08X}", g_session_run_id, movie_idx,
                 caller_lr);
  }
  __imp__DOAX_PlayMovie(ctx, base);
}

extern "C" REX_FUNC(DOAX_MenuWorkFiberLoop) {
  g_doax_hook_guest_base = base;
  if (!g_session_outcome.menu_fiber_enter) {
    g_session_outcome.menu_fiber_enter = true;
    g_menu_fiber_entered_this_run = true;
    MaybeLogSessionOutcome(base, "menu-fiber-enter", "");
  }
  ++g_menu_fiber_probe.enter_count;
  MaybeLogMenuFiberSite("enter", base, ctx);
  __imp__DOAX_MenuWorkFiberLoop(ctx, base);
  g_session_outcome.menu_fiber_exit = true;
  MaybeLogSessionOutcome(base, "menu-fiber-exit", "");
  MaybeLogMenuFiberSite("exit", base, ctx);
  if (g_travel_overlay_guard) {
    EnforceTravelOverlayGuard(base, "menu-fiber-exit");
  }
}

extern "C" REX_FUNC(DOAX_MenuPreTransitionHook) {
  const uint32_t arg2 = static_cast<uint32_t>(ctx.r4.u64);
  __imp__DOAX_MenuPreTransitionHook(ctx, base);
  if (g_travel_overlay_guard && arg2 == kTravelFadeMilestoneTimeline) {
    REX_STORE_U8(kDoaxMenuFiberDefAddr, 1);
    if (g_travel_complete_logs < kTravelCompleteLogCap) {
      ++g_travel_complete_logs;
      REXKRNL_WARN("DOAX travel-complete: MenuPreTransitionHook(0, 30) def=1");
    }
  }
}

extern "C" REX_FUNC(DOAX_MenuSceneTransition) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t scene_id = static_cast<uint32_t>(ctx.r3.u64);
  if (g_menu_scene_transition_logs < kMenuSceneTransitionLogCap) {
    ++g_menu_scene_transition_logs;
    REXKRNL_WARN("DOAX scene-transition: scene_id={} lr=0x{:08X}", scene_id, caller_lr);
  }
  ArmBootFiberSuppress(base, "scene-transition", caller_lr, scene_id);
  __imp__DOAX_MenuSceneTransition(ctx, base);
  if (!g_travel_fade_complete) {
    EnforceTravelOverlayGuard(base, "scene-transition-return");
  }
}

extern "C" REX_FUNC(DOAX_IslandSceneLoad) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  if (g_suppress_menu_kick) {
    ArmBootFiberSuppress(base, "island-scene-load", caller_lr, 0);
  }
  if (g_island_scene_load_logs < kIslandSceneLoadLogCap) {
    ++g_island_scene_load_logs;
    REXKRNL_WARN("DOAX island-scene-load: lr=0x{:08X} boot_suppress={}", caller_lr,
                 g_suppress_boot_work_fiber);
  }
  __imp__DOAX_IslandSceneLoad(ctx, base);
  EnforceTravelOverlayGuard(base, "island-scene-load-return");
}

extern "C" REX_FUNC(DOAX_MenuTransitionReadyCheck) {
  __imp__DOAX_MenuTransitionReadyCheck(ctx, base);
  MaybeLogTravelCase3Probe(base, "ready-check", static_cast<int32_t>(ctx.r3.u64), -1, -1,
                           static_cast<uint32_t>(ctx.lr));
}

extern "C" REX_FUNC(DOAX_MenuTransitionFadeAlpha) {
  __imp__DOAX_MenuTransitionFadeAlpha(ctx, base);
  MaybeLogTravelCase3Probe(base, "fade-alpha", -1, static_cast<int32_t>(ctx.r3.u64), -1,
                           static_cast<uint32_t>(ctx.lr));
}

extern "C" REX_FUNC(DOAX_MenuTransitionTimeline) {
  __imp__DOAX_MenuTransitionTimeline(ctx, base);
  const int32_t timeline = static_cast<int32_t>(ctx.r3.u64);
  MaybeLogTravelCase3Probe(base, "timeline", -1, -1, timeline, static_cast<uint32_t>(ctx.lr));
  ArmTravelFadeMilestone(base, timeline, "timeline");
  MaybeFastCompleteTravelFade(base, "timeline-df0");
  CompleteTravelFade(base, timeline, "timeline");
}

extern "C" REX_FUNC(DOAX_BootWorkFiberLoop) {
  if (ShouldSuppressBootWorkFiber()) {
    MaybeLogBootFiberSuppress(static_cast<uint32_t>(ctx.lr), "BootWorkFiberLoop");
    return;
  }
  __imp__DOAX_BootWorkFiberLoop(ctx, base);
}

extern "C" REX_FUNC(DOAX_BootWorkFiberBody) {
  if (ShouldSuppressBootWorkFiber()) {
    MaybeLogBootFiberSuppress(static_cast<uint32_t>(ctx.lr), "BootWorkFiberBody");
    EnforceTravelOverlayGuard(base, "boot-fiber-body-skip");
    MaybeFastCompleteTravelFade(base, "boot-fiber-body-skip");
    return;
  }
  if (g_boot_fiber_run_logs < kBootFiberRunLogCap) {
    ++g_boot_fiber_run_logs;
    REXKRNL_WARN(
        "DOAX boot-fiber-run: boot_present={} suppress={} prevent_td={} fade_done={} "
        "sched={}/{} lr=0x{:08X}",
        REX_LOAD_U8(kDoaxBootPresentByteAddr), g_suppress_boot_work_fiber ? 1 : 0,
        g_prevent_scene_teardown ? 1 : 0, g_travel_fade_complete ? 1 : 0,
        ReadSchedulerSnapshot(base).flag4, ReadSchedulerSnapshot(base).flag5,
        static_cast<uint32_t>(ctx.lr));
  }
  __imp__DOAX_BootWorkFiberBody(ctx, base);
  if (g_travel_overlay_guard) {
    EnforceTravelOverlayGuard(base, "boot-fiber-body-return");
  }
}

extern "C" REX_FUNC(DOAX_BootPresentStateUpdate) {
  if (ShouldSuppressBootWorkFiber()) {
    MaybeLogBootFiberSuppress(static_cast<uint32_t>(ctx.lr), "BootPresentStateUpdate");
    return;
  }
  if (g_boot_present_run_logs < kBootPresentRunLogCap) {
    ++g_boot_present_run_logs;
    const uint8_t boot_present = REX_LOAD_U8(kDoaxBootPresentByteAddr);
    const uint8_t target_state = REX_LOAD_U8(kDoaxBootTargetStateAddr);
    REXKRNL_WARN(
        "DOAX boot-present-run: boot_present={} target={} present_idx={} "
        "prevent_td={} fade_done={} gate_834204BC={}",
        boot_present, target_state, REX_LOAD_U32(kDoaxPresentStateIndexAddr),
        g_prevent_scene_teardown ? 1 : 0, g_travel_fade_complete ? 1 : 0,
        REX_LOAD_U32(0x834204BCu));
  }
  if (g_prevent_scene_teardown && !g_travel_fade_complete) {
    const uint8_t saved_target_state = REX_LOAD_U8(kDoaxBootTargetStateAddr);
    const uint32_t saved_state_change = REX_LOAD_U32(kDoaxBootStateChangeFlagAddr);
    const uint32_t saved_handling = REX_LOAD_U32(kDoaxBootPresentHandlingAddr);

    __imp__DOAX_BootPresentStateUpdate(ctx, base);

    if (REX_LOAD_U8(kDoaxBootTargetStateAddr) == 2 &&
        REX_LOAD_U8(kDoaxBootTargetStateAddr) != saved_target_state) {
      REXKRNL_WARN("DOAX boot-present-undo: teardown path fired, restoring state");
      REX_STORE_U8(kDoaxBootTargetStateAddr, saved_target_state);
      REX_STORE_U32(kDoaxBootStateChangeFlagAddr, saved_state_change);
      REX_STORE_U32(kDoaxBootPresentHandlingAddr, saved_handling);
      REX_STORE_U8(kDoaxBootTeardownNeededAddr, 0);
    }
  } else {
    __imp__DOAX_BootPresentStateUpdate(ctx, base);
  }
}

extern "C" REX_FUNC(DOAX_BootMovieReplayTeardown) {
  if (g_prevent_scene_teardown) {
    if (g_teardown_logs < kTeardownLogCap) {
      ++g_teardown_logs;
      REXKRNL_WARN("DOAX teardown: PREVENTED lr=0x{:08X} boot_present={}",
                   static_cast<uint32_t>(ctx.lr), REX_LOAD_U8(kDoaxBootPresentByteAddr));
    }
    REX_STORE_U8(kDoaxBootTeardownNeededAddr, 0);
    return;
  }
  if (g_teardown_logs < kTeardownLogCap) {
    ++g_teardown_logs;
    REXKRNL_WARN("DOAX teardown: RUNNING lr=0x{:08X} boot_present={} target={}",
                 static_cast<uint32_t>(ctx.lr), REX_LOAD_U8(kDoaxBootPresentByteAddr),
                 REX_LOAD_U8(kDoaxBootTargetStateAddr));
  }
  __imp__DOAX_BootMovieReplayTeardown(ctx, base);
}

// Midasm hook: skip ninja_vi_hd.sfd playback. Jumps from 0x8250AB1C (the
// DOAX_PlayMovie(4) call) to 0x8250ABA4 (after the entire ninja block).
bool DOAX_SkipNinjaViHdMovie() {
  static uint32_t skip_logs = 0;
  if (skip_logs < kMovieProbeLogCap) {
    ++skip_logs;
    REXKRNL_WARN("DOAX movie-probe SkipNinjaViHd (midasm) log={}", skip_logs);
  }
  return true;
}

extern "C" REX_FUNC(DOAX_WorkQueueDispatchLoop) {
  REX_FUNC_PROLOGUE(DOAX_WorkQueueDispatchLoop);
  uint32_t ea{};
  ctx.r12.u64 = ctx.lr;
  ctx.lr = 0x824C05C0;
  __savegprlr_28(ctx, base);
  ea = -128 + ctx.r1.u32;
  REX_STORE_U32(ea, ctx.r1.u32);
  ctx.r1.u32 = ea;
  ctx.lr = 0x824C05C8;
  sub_824C0608(ctx, base);
  ctx.r28.s64 = 1;
loc_824C05DC_hook:
  ctx.r29.s64 = kDoaxWorkQueueSegBase;
  ctx.r30.s64 = kDoaxWorkQueueSegBase;
  ctx.r31.s64 = kDoaxWorkQueueTableBase;
  ctx.r11.u64 = REX_LOAD_U32(ctx.r30.u32 + 13560);
  ctx.r3.u64 = REX_LOAD_U32(ctx.r29.u32 + 12884);
  ctx.r11.s64 = static_cast<int64_t>(ctx.r11.u64 * static_cast<uint64_t>(28));
  ctx.r11.u64 = ctx.r11.u64 + ctx.r31.u64;
  ctx.r10.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
  REX_STORE_U32(ctx.r11.u32 + 4, ctx.r28.u32);
  ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 0) &
                0xFFFFFFFFFFFFFFFB;
  REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
  ctx.lr = 0x824C0600;
  DOAX_FiberYield(ctx, base);
  ctx.lr = 0x824C0604;
  DOAX_SchedulerDrainDispatch(ctx, base);
  goto loc_824C05DC_hook;
}
