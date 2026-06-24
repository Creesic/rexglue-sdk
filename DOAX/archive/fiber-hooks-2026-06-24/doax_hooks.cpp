#include "doax_hooks.h"

#include "generated/default/doax_init.h"

#include <chrono>
#include <cstdint>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/guest_pc_fiber.h>

namespace {

constexpr uint32_t kGuestFiberSwapAddress = 0x82785670u;

// Global scheduler flag in .bss (lis -31940 / addi -29192 in DOAX_SchedulerDrainWake).
constexpr uint32_t kDoaxSchedulerFlagAddr =
    static_cast<uint32_t>(static_cast<int32_t>(-2093219840 + -29192));

// Boot skip toggles.
constexpr bool kSkipLicenseWarning = true;
constexpr bool kSkipNinjaViHdMovie = true;

// Work-queue globals (lis -31926 / addi 12888 in DOAX_WorkQueueDispatchLoop).
constexpr int64_t kDoaxWorkQueueSegBase = -2092302336;
constexpr int64_t kDoaxWorkQueueTableBase = kDoaxWorkQueueSegBase + 12888;

// Work-queue dispatcher (DOAX_WorkQueueDispatchLoop) hang probe — grep log for "DOAX wq-probe".
constexpr uint32_t kWorkQueueProbeLogInterval = 10'000;
constexpr uint32_t kWorkQueueProbeLogCap = 150;
constexpr uint32_t kWorkQueueProbeBootLines = 8;
constexpr uint64_t kWorkQueueStuckIterationThreshold = 50'000;
constexpr uint64_t kWorkQueueSlowCallUs = 1'000;
constexpr uint32_t kDrainProbeLogCap = 120;
constexpr uint32_t kDrainProbeBootLines = 12;
constexpr uint32_t kDrainProbeNoopInterval = 500;
constexpr uint32_t kSchedulerHeaderBytes = 16;
constexpr uint32_t kFlag0SafetyNetLogCap = 24;

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

struct DrainProbeState {
  uint64_t call_count = 0;
  uint32_t log_count = 0;
  uint8_t last_flag0 = 0xFF;
  uint64_t noop_since_log = 0;
};

DrainProbeState g_drain_probe;
uint32_t g_flag0_safety_net_logs = 0;
uint32_t g_cdf8_wake_logs = 0;
constexpr uint32_t kCdf8WakeLogCap = 16;

// Input / menu probes — grep log for "DOAX input-probe" / "DOAX menu-probe".
constexpr uint32_t kInputProbeLogCap = 48;
constexpr uint32_t kInputProbeBootLines = 16;
constexpr uint32_t kInputProbeInterval = 500;
constexpr uint32_t kMenuProbeLogCap = 8;
constexpr uint32_t kMenuSceneProbeLogCap = 4;
constexpr uint32_t kMovieProbeLogCap = 8;
constexpr uint32_t kFiberYieldDispatcherLogCap = 48;
constexpr uint32_t kFiberYieldWorkQueueLogCap = 8;

struct InputProbeState {
  uint64_t get_state_calls = 0;
  uint64_t poll_calls = 0;
  uint64_t indirect_calls = 0;
  uint32_t log_count = 0;
  uint32_t last_buttons = 0xFFFF;
  uint32_t last_user = 0xFFFFFFFF;
};

struct MenuProbeState {
  uint32_t cef0_calls = 0;
  uint32_t log_count = 0;
};

struct FiberYieldProbeState {
  uint32_t dispatcher_log_count = 0;
  uint32_t work_queue_log_count = 0;
};

InputProbeState g_input_probe;
MenuProbeState g_menu_probe;
FiberYieldProbeState g_fiber_yield_probe;
uint32_t g_menu_scene_probe_logs = 0;
uint32_t g_movie_probe_logs = 0;
bool g_suppress_menu_kick = false;
bool g_suppress_boot_work_fiber = false;
uint32_t g_menu_select_logs = 0;
uint32_t g_boot_fiber_suppress_logs = 0;
uint32_t g_menu_scene_transition_logs = 0;
uint32_t g_island_scene_load_logs = 0;
constexpr uint32_t kMenuSelectLogCap = 8;
constexpr uint32_t kBootFiberSuppressLogCap = 8;
constexpr uint32_t kMenuSceneTransitionLogCap = 16;
constexpr uint32_t kIslandSceneLoadLogCap = 8;
constexpr uint32_t kTravelCase3ProbeLogCap = 128;
constexpr uint32_t kTravelOverlayGuardLogCap = 48;
constexpr uint32_t kTravelCompleteLogCap = 16;
constexpr uint32_t kDoaxIslandGameplayPresentIndex = 3u;
constexpr uint32_t kTravelFadeMilestoneTimeline = 30u;
constexpr uint32_t kTravelFastCompleteDf0Threshold = 40u;

// Menu work-fiber / travel transition globals (IDA: DOAX_MenuWorkFiberLoop case 3).
constexpr uint32_t kDoaxMenuFiberDebAddr = 0x833B8DEBu;
constexpr uint32_t kDoaxMenuFiberDefAddr = 0x833B8DEFu;
constexpr uint32_t kDoaxMenuFiberDeaAddr = 0x833B8DEAu;
constexpr uint32_t kDoaxMenuItemIdByteAddr = 0x833B8DECu;
constexpr uint32_t kDoaxIslandOverlayFlagAddr = 0x833B8514u;
constexpr uint32_t kDoaxPresentStateIndexAddr = 0x833B84C8u;
constexpr uint32_t kDoaxMenuTransitionDf0Addr = 0x833B8DF0u;
constexpr uint32_t kDoaxMenuTransitionDf4Addr = 0x833B8DF4u;

constexpr uint32_t kMenuSceneTransitionLabel12Lr = 0x824C1770u;
constexpr uint32_t kMenuSceneTransitionLabel34Lr = 0x824C191Cu;
constexpr uint32_t kMenuWorkFiberCase3 = 3u;

bool g_travel_overlay_guard = false;
bool g_travel_fade_complete = false;
uint32_t g_travel_case3_probe_logs = 0;
uint32_t g_travel_overlay_guard_logs = 0;
uint32_t g_travel_complete_logs = 0;
uint8_t* g_doax_hook_guest_base = nullptr;

SchedulerSnapshot ReadSchedulerSnapshot(uint8_t* base);

void MaybeLogInputProbe(const char* site, uint32_t user, uint32_t result, uint32_t buttons,
                        uint32_t caller_lr, bool non_zero_buttons) {
  auto& probe = g_input_probe;
  ++probe.get_state_calls;
  const bool boot_line = probe.get_state_calls <= kInputProbeBootLines;
  const bool button_change = buttons != probe.last_buttons;
  const bool interval_line = probe.get_state_calls % kInputProbeInterval == 0;
  if (probe.log_count >= kInputProbeLogCap) {
    return;
  }
  if (!boot_line && !button_change && !non_zero_buttons && !interval_line) {
    return;
  }
  ++probe.log_count;
  probe.last_buttons = buttons;
  probe.last_user = user;
  REXKRNL_WARN(
      "DOAX input-probe site={} call={} user={} result=0x{:08X} buttons=0x{:04X} lr=0x{:08X}",
      site, probe.get_state_calls, user, result, buttons, caller_lr);
}

constexpr uint32_t kSchedulerFiberYieldLr = 0x824C15F4u;
constexpr uint32_t kMenuWorkFiberYieldLr = 0x825A25E0u;
constexpr uint32_t kDrainInnerFiberYieldLr = 0x824C0C3Cu;
constexpr uint32_t kDispatcherFiberYieldLr = 0x824C0600u;
constexpr uint32_t kCdf8FiberYieldLr = 0x8258CE4Cu;

// Fiber yields from the hooked dispatcher/cdf8 paths reload their own callee-saves
// after return. Restoring r14-r31 there undoes legitimate post-swap state and
// leaves the scheduler wedged (black screen, flag2 never dispatches menu work).
bool NeedsFiberCalleeSavePreserve(uint32_t caller_lr) {
  switch (caller_lr) {
    case kDispatcherFiberYieldLr:
    case kCdf8FiberYieldLr:
      return false;
    default:
      return true;
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

void MaybeLogSchedulerSnapshot(const char* site, const SchedulerSnapshot& snap) {
  REXKRNL_WARN(
      "DOAX sched-probe site={} f0-7={:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X} w8={} w12={}",
      site, snap.flag0, snap.flag1, snap.flag2, snap.flag3, snap.flag4, snap.flag5, snap.flag6,
      snap.flag7, snap.word8, snap.word12);
}

// DOAX_SchedulerDrainDispatch only dispatches menu work through loc_824C0A5C when flag2!=0.
// Arm flag2 after CEF0 whenever the drain left it cleared. Scene-bring-up modes
// (2/2, 3/3, 2/0) need this during initial boot — restricting to menu-idle 5/1
// alone prevented the four-option screen. After the player confirms a menu item,
// g_suppress_menu_kick stops re-arming so the post-selection transition can finish.
void ArmSchedulerDispatchAfterMenuInit(uint8_t* base, uint32_t caller_lr) {
  if (g_suppress_menu_kick) {
    return;
  }
  const SchedulerSnapshot before = ReadSchedulerSnapshot(base);
  if (before.flag0 == 0 || before.flag2 != 0) {
    return;
  }
  REX_STORE_U8(kDoaxSchedulerFlagAddr + 2, 1);
  const SchedulerSnapshot after = ReadSchedulerSnapshot(base);
  REXKRNL_WARN(
      "DOAX menu-kick: armed flag2 after CEF0 lr=0x{:08X} mode={}/{} -> {}/{}",
      caller_lr, before.flag4, before.flag5, after.flag4, after.flag5);
}

bool IsMenuWorkFiberConfirm(uint32_t caller_lr) {
  switch (caller_lr) {
    case 0x824C16A4u:
    case 0x824C17FCu:
      return true;
    default:
      return false;
  }
}

bool IsPostMenuConfirmSceneTransition(uint32_t caller_lr) {
  // LABEL_12 in DOAX_MenuWorkFiberLoop (case 1/2) and LABEL_34 else path.
  // Do not use DOAX_MenuItemConfirm (0x824C16A4): case 0 calls it during menu bring-up.
  switch (caller_lr) {
    case 0x824C176Cu:
    case kMenuSceneTransitionLabel12Lr:
    case 0x824C1918u:
    case kMenuSceneTransitionLabel34Lr:
      return true;
    default:
      return false;
  }
}

bool IsLabel34SceneTransition(uint32_t caller_lr) {
  return caller_lr == 0x824C1918u || caller_lr == kMenuSceneTransitionLabel34Lr;
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
  // dword_833B8DF4 holds the fade timeline; near completion allow present index 3.
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
  const uint32_t before_present = REX_LOAD_U32(kDoaxPresentStateIndexAddr);
  if (REX_LOAD_U8(kDoaxMenuFiberDefAddr) == 0) {
    REX_STORE_U8(kDoaxMenuFiberDefAddr, 1);
  }
  REX_STORE_U32(kDoaxPresentStateIndexAddr, kDoaxIslandGameplayPresentIndex);
  if (g_travel_complete_logs < kTravelCompleteLogCap) {
    ++g_travel_complete_logs;
    const MenuTravelProbeState state = ReadMenuTravelProbeState(base);
    REXKRNL_WARN(
        "DOAX travel-complete: fade-done site={} timeline={} present {}->{} deb={} def={} "
        "dea={} overlay={} df0={} df4={}",
        site, timeline, before_present, kDoaxIslandGameplayPresentIndex, state.deb, state.def,
        state.dea, state.overlay, state.df0, state.df4);
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
  if (state.dea != kMenuWorkFiberCase3 && caller_lr != kSchedulerFiberYieldLr) {
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

constexpr uint32_t kMenuPostConfirmCleanupLr = 0x824C17FCu;
constexpr uint32_t kDoaxTravelMenuItemId = 15u;
constexpr uint32_t kMenuItemConfirmCase0Arg = 30u;
constexpr uint32_t kMenuIdleSchedulerMode4 = 5u;
constexpr uint32_t kMenuIdleSchedulerMode5 = 1u;

void ArmBootFiberSuppress(uint8_t* base, const char* site, uint32_t lr, uint32_t detail) {
  if (g_suppress_boot_work_fiber) {
    ApplyTravelTransitionGuards(base, site);
    return;
  }
  g_suppress_boot_work_fiber = true;
  ApplyTravelTransitionGuards(base, site);
  REXKRNL_WARN("DOAX travel-transition: arm boot-fiber-suppress site={} detail={} lr=0x{:08X}",
               site, detail, lr);
}

// Arm boot-fiber suppress for Travel without firing during CEF0 bring-up (modes 2/2,
// 3/3, 2/0). Case 0 calls DOAX_MenuItemConfirm(item, 30) at lr=0x824C16A4; Travel is
// item 15. Post-confirm cleanup passes arg 0 (lr=0x824C17FC when byte_833B8DEB>1).
void MaybeArmBootFiberForTravel(uint8_t* base, const char* site, uint32_t lr,
                                uint32_t detail, uint32_t item_id, uint32_t arg2) {
  if (g_suppress_boot_work_fiber) {
    return;
  }
  if (arg2 == 0) {
    ArmBootFiberSuppress(base, site, lr, detail);
    return;
  }
  if (item_id != kDoaxTravelMenuItemId || arg2 != kMenuItemConfirmCase0Arg ||
      lr != 0x824C16A4u) {
    return;
  }
  const SchedulerSnapshot snap = ReadSchedulerSnapshot(base);
  const bool idle_menu =
      snap.flag4 == kMenuIdleSchedulerMode4 && snap.flag5 == kMenuIdleSchedulerMode5;
  const bool confirm_frame = snap.flag4 == 2 && snap.flag5 == 2;
  if (idle_menu || confirm_frame) {
    ArmBootFiberSuppress(base, site, lr, item_id);
  }
}

void MaybeLogMenuSelection(uint32_t caller_lr, uint32_t item_id, uint32_t cursor) {
  if (g_menu_select_logs >= kMenuSelectLogCap) {
    return;
  }
  ++g_menu_select_logs;
  REXKRNL_WARN("DOAX menu-select: confirm lr=0x{:08X} cursor={} item_id={}", caller_lr,
               cursor, item_id);
}

void MaybeLogMenuProbe(const char* site, uint32_t caller_lr) {
  auto& probe = g_menu_probe;
  if (probe.log_count >= kMenuProbeLogCap) {
    return;
  }
  ++probe.log_count;
  REXKRNL_WARN("DOAX menu-probe site={} call={} lr=0x{:08X}", site, ++probe.cef0_calls,
               caller_lr);
}

void MaybeLogFiberYield(uint32_t caller_lr, uint32_t target, uint32_t resume_lr) {
  auto& probe = g_fiber_yield_probe;
  if (caller_lr == 0x824C0600 || caller_lr == 0x8258CE4C || caller_lr == kSchedulerFiberYieldLr) {
    if (probe.dispatcher_log_count >= kFiberYieldDispatcherLogCap) {
      return;
    }
    ++probe.dispatcher_log_count;
    REXKRNL_WARN("DOAX fiber-yield caller_lr=0x{:08X} target=0x{:08X} resume_lr=0x{:08X}",
                 caller_lr, target, resume_lr);
    return;
  }
  if (caller_lr != 0x8258CC88) {
    return;
  }
  if (probe.work_queue_log_count >= kFiberYieldWorkQueueLogCap) {
    return;
  }
  ++probe.work_queue_log_count;
  REXKRNL_WARN("DOAX fiber-yield caller_lr=0x{:08X} target=0x{:08X} resume_lr=0x{:08X}",
               caller_lr, target, resume_lr);
}

void SaveSchedulerHeaderBytes(uint8_t* base, uint8_t* out) {
  const uint32_t sched = kDoaxSchedulerFlagAddr;
  for (uint32_t i = 0; i < kSchedulerHeaderBytes; ++i) {
    out[i] = REX_LOAD_U8(sched + i);
  }
}

void WriteSchedulerHeaderBytes(uint8_t* base, const uint8_t* in) {
  const uint32_t sched = kDoaxSchedulerFlagAddr;
  for (uint32_t i = 0; i < kSchedulerHeaderBytes; ++i) {
    REX_STORE_U8(sched + i, in[i]);
  }
}

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

// Virtual bctrl paths pass r3=scheduler and may fiber-swap. If callee-saves are
// wrong after resume, guest stores can hit host address 0 and trash .bss.
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

// Temporary experiment: DOAX_SchedulerDrainWake clears flag0 but wake path may not re-arm it.
void ApplyFlag0SafetyNet(uint8_t* base, const SchedulerSnapshot& before,
                         SchedulerSnapshot& after) {
  // Post-Travel scene transition may legitimately clear flag0. Restoring it keeps
  // the drain dispatching stale boot-movie work (lr=0x8250A104).
  if (g_suppress_boot_work_fiber) {
    return;
  }
  if (before.flag0 == 0 || after.flag0 != 0) {
    return;
  }
  const uint32_t sched = kDoaxSchedulerFlagAddr;
  REX_STORE_U8(sched + 0, before.flag0);
  after.flag0 = before.flag0;
  // After DOAX_SchedulerDrainWake wake (flag1==2), flag2 must be non-zero for the
  // loc_824C0A5C virtual-dispatch path; wake may not re-arm it after fiber bugs.
  if (after.flag1 == 2 && after.flag2 == 0) {
    REX_STORE_U8(sched + 2, 1);
    after.flag2 = 1;
  }
  if (g_flag0_safety_net_logs < kFlag0SafetyNetLogCap) {
    ++g_flag0_safety_net_logs;
    REXKRNL_WARN(
        "DOAX scheduler-safety: restored flag0={} flag2={} (flag1={} after wake)",
        before.flag0, after.flag2, after.flag1);
  }
}

void MaybeLogDrainProbe(PPCContext& ctx, const SchedulerSnapshot& before,
                        const SchedulerSnapshot& after, uint64_t elapsed_us,
                        uint32_t caller_lr, const char* reason) {
  auto& probe = g_drain_probe;
  if (probe.log_count >= kDrainProbeLogCap) {
    return;
  }
  ++probe.log_count;
  REXKRNL_WARN(
      "DOAX drain-probe call={} reason={} us={} caller_lr=0x{:08X} "
      "before[f0-7]={:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X} w8={} w12={} "
      "after[f0-7]={:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X} w8={} w12={} "
      "r29=0x{:08X} r30=0x{:08X} r31=0x{:08X} lr=0x{:08X}",
      probe.call_count, reason, elapsed_us, caller_lr, before.flag0, before.flag1,
      before.flag2, before.flag3, before.flag4, before.flag5, before.flag6, before.flag7,
      before.word8, before.word12, after.flag0, after.flag1, after.flag2, after.flag3,
      after.flag4, after.flag5, after.flag6, after.flag7, after.word8, after.word12,
      ctx.r29.u32, ctx.r30.u32, ctx.r31.u32, static_cast<uint32_t>(ctx.lr));
}

void ProbeDrainCall(PPCContext& ctx, const SchedulerSnapshot& before,
                    const SchedulerSnapshot& after, uint64_t elapsed_us,
                    uint32_t caller_lr, bool reg_clobber) {
  auto& probe = g_drain_probe;
  ++probe.call_count;

  const bool boot_line = probe.call_count <= kDrainProbeBootLines;
  const bool active = before.flag0 != 0;
  const bool slow = elapsed_us >= kWorkQueueSlowCallUs;
  const bool flag_transition = before.flag0 != probe.last_flag0;

  if (flag_transition) {
    probe.last_flag0 = before.flag0;
  }

  if (boot_line) {
    MaybeLogDrainProbe(ctx, before, after, elapsed_us, caller_lr, "boot");
  } else if (reg_clobber) {
    MaybeLogDrainProbe(ctx, before, after, elapsed_us, caller_lr, "reg_clobber");
  } else if (active) {
    MaybeLogDrainProbe(ctx, before, after, elapsed_us, caller_lr, "active");
  } else if (slow) {
    MaybeLogDrainProbe(ctx, before, after, elapsed_us, caller_lr, "slow");
  } else if (flag_transition) {
    MaybeLogDrainProbe(ctx, before, after, elapsed_us, caller_lr, "flag_transition");
  } else if (!active) {
    ++probe.noop_since_log;
    if (probe.noop_since_log >= kDrainProbeNoopInterval) {
      probe.noop_since_log = 0;
      MaybeLogDrainProbe(ctx, before, after, elapsed_us, caller_lr, "noop_steady");
    }
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

struct WorkQueueProbeState {
  uint64_t iteration = 0;
  uint32_t log_count = 0;
  uint32_t last_slot = 0;
  uint32_t last_queue_head = 0;
  uint32_t last_work_item = 0;
  uint64_t stuck_same_state_since = 0;
  bool stuck_logged = false;
};

WorkQueueProbeState g_work_queue_probe;

using ProbeClock = std::chrono::steady_clock;

uint64_t ProbeElapsedUs(ProbeClock::time_point start) {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(ProbeClock::now() - start)
          .count());
}

void MaybeLogWorkQueueProbe(PPCContext& ctx, uint32_t slot_idx, uint32_t queue_head,
                            uint32_t work_item, uint64_t yield_us, uint64_t drain_us,
                            const char* reason) {
  auto& probe = g_work_queue_probe;
  if (probe.log_count >= kWorkQueueProbeLogCap) {
    return;
  }
  ++probe.log_count;
  REXKRNL_WARN(
      "DOAX wq-probe iter={} reason={} slot={} queue_head=0x{:08X} work_item=0x{:08X} "
      "yield_us={} drain_us={} r29=0x{:08X} r30=0x{:08X} r31=0x{:08X} lr=0x{:08X}",
      probe.iteration, reason, slot_idx, queue_head, work_item, yield_us, drain_us,
      ctx.r29.u32, ctx.r30.u32, ctx.r31.u32, static_cast<uint32_t>(ctx.lr));
}

void ProbeWorkQueueIteration(PPCContext& ctx, uint32_t slot_idx, uint32_t queue_head,
                             uint32_t work_item, uint64_t yield_us, uint64_t drain_us) {
  auto& probe = g_work_queue_probe;
  ++probe.iteration;

  const bool boot_line = probe.iteration <= kWorkQueueProbeBootLines;
  const bool interval_line =
      probe.iteration % kWorkQueueProbeLogInterval == 0;
  const bool slow_call = drain_us >= kWorkQueueSlowCallUs;

  if (slot_idx == probe.last_slot && queue_head == probe.last_queue_head &&
      work_item == probe.last_work_item) {
    if (probe.stuck_same_state_since == 0) {
      probe.stuck_same_state_since = probe.iteration;
    } else if (!probe.stuck_logged &&
               probe.iteration - probe.stuck_same_state_since >=
                   kWorkQueueStuckIterationThreshold) {
      probe.stuck_logged = true;
      MaybeLogWorkQueueProbe(ctx, slot_idx, queue_head, work_item, yield_us, drain_us,
                             "stuck_same_state");
      return;
    }
  } else {
    probe.stuck_same_state_since = 0;
    probe.stuck_logged = false;
    probe.last_slot = slot_idx;
    probe.last_queue_head = queue_head;
    probe.last_work_item = work_item;
  }

  if (boot_line) {
    MaybeLogWorkQueueProbe(ctx, slot_idx, queue_head, work_item, yield_us, drain_us, "boot");
  } else if (slow_call) {
    MaybeLogWorkQueueProbe(ctx, slot_idx, queue_head, work_item, yield_us, drain_us,
                           "slow_call");
  } else if (interval_line) {
    MaybeLogWorkQueueProbe(ctx, slot_idx, queue_head, work_item, yield_us, drain_us,
                           "interval");
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
  REXLOG_INFO("DOAX guest-PC fiber: OS-fiber swap override active for 0x{:08X}",
              kGuestFiberSwapAddress);
}

//=============================================================================
// Strong override of the generated guest fiber-swap routine.
//
// Codegen emits `DOAX_FiberContextSwitch` as a weak alias of
// `__imp__DOAX_FiberContextSwitch`. Callers invoke it as a direct C++ call,
// which bypasses the runtime function table. This strong definition routes
// every direct/indirect caller through RunFiberSwap; the real guest register
// save/restore still runs inside __imp__DOAX_FiberContextSwitch via
// FiberSwapImpl82785670.
//=============================================================================
extern "C" REX_FUNC(DOAX_FiberContextSwitch) {
  rex::ppc::RunFiberSwap(ctx, base, &FiberSwapImpl82785670, static_cast<uint32_t>(ctx.r3.u64), 0);
}

//=============================================================================
// Work-queue wake helper (0x8258CE60). Called from DOAX_SchedulerDrainWake with r31
// holding a global flag pointer; the function may trigger nested fiber swaps via
// sub_827831B0. Guest epilogue `ld r31,-16(r1)` can load 0 when r1/stack state
// does not match the saved slot after a fiber round-trip, so preserve the
// caller's r31 on the host stack and restore it after the generated body runs.
//=============================================================================
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

//=============================================================================
// Active-slot wake (0x8258CDF8). Loads queue_head from 0x834A3254, ORs bit 8 on
// the work entry, then fiber-swaps via DOAX_FiberYield(r3=queue_head). This is the
// dispatch step inside DOAX_SchedulerDrainWake's DOAX_WorkQueueSlotWake(1) wake.
//=============================================================================
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
  const uint32_t queue_head = ctx.r3.u32;
  ctx.r11.u64 = ctx.r10.u64 | 8;
  REX_STORE_U32(ctx.r31.u32 + 0, ctx.r11.u32);
  ctx.lr = 0x8258CE4C;
  DOAX_FiberYield(ctx, base);
  ctx.r31.u64 = work_entry;
  if (g_cdf8_wake_logs < kCdf8WakeLogCap) {
    ++g_cdf8_wake_logs;
    REXKRNL_WARN("DOAX cdf8-wake queue_head=0x{:08X} work_entry=0x{:08X}", queue_head,
                 static_cast<uint32_t>(work_entry));
  }
  ctx.r1.s64 = ctx.r1.s64 + 96;
  ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
  ctx.lr = ctx.r12.u64;
  ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
}

//=============================================================================
// Scheduler flag helper (0x824C08B8). After the virtual bctrl, reload the global
// flag pointer before waking the work queue. Indirect callees and nested fiber
// swaps can leave r31 clobbered even though the original game relied on ABI
// callee-save across the call.
//=============================================================================
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
  if (!ctx.cr6.eq) goto loc_824C0910_hook;
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

//=============================================================================
// Scheduler vtable index helper (0x824C06D8). Only bytes +4/+5 are updated;
// fiber paths that clobber r8 can corrupt the rest of the scheduler header.
//=============================================================================
extern "C" REX_FUNC(DOAX_SchedulerFiberSwap) {
  const SchedulerSnapshot before = ReadSchedulerSnapshot(base);
  __imp__DOAX_SchedulerFiberSwap(ctx, base);
  PreserveSchedulerExceptBytes45(base, before);
}

//=============================================================================
// Work-queue drain (0x824C0928). Returns immediately when scheduler flag[0]==0.
// Virtual bctrl paths and DOAX_SchedulerDrainWake can clobber callee-saves across fiber
// swaps; preserve the dispatcher's r29-r31 when called from DOAX_WorkQueueDispatchLoop.
//=============================================================================
extern "C" REX_FUNC(DOAX_SchedulerDrainDispatch) {
  const uint64_t caller_lr = ctx.lr;
  const uint64_t caller_r29 = ctx.r29.u64;
  const uint64_t caller_r30 = ctx.r30.u64;
  const uint64_t caller_r31 = ctx.r31.u64;
  const SchedulerSnapshot before = ReadSchedulerSnapshot(base);
  const auto drain_start = ProbeClock::now();
  __imp__DOAX_SchedulerDrainDispatch(ctx, base);
  const uint64_t drain_us = ProbeElapsedUs(drain_start);
  const bool reg_clobber = (caller_r29 != 0 && ctx.r29.u64 == 0) ||
                           (caller_r30 != 0 && ctx.r30.u64 == 0) ||
                           (caller_r31 != 0 && ctx.r31.u64 == 0) ||
                           (caller_lr != 0 && ctx.lr == 0);
  RestoreDrainCallerRegs(ctx, caller_lr, caller_r29, caller_r30, caller_r31);
  SchedulerSnapshot after = ReadSchedulerSnapshot(base);
  ApplyFlag0SafetyNet(base, before, after);
  ProbeDrainCall(ctx, before, after, drain_us, static_cast<uint32_t>(caller_lr), reg_clobber);
}

//=============================================================================
// Work-queue dispatcher loop (0x824C05B8). r29/r30/r31 point at globals
// (lis -31926); each iteration yields via DOAX_FiberYield then drains work in
// DOAX_SchedulerDrainDispatch. Fiber round-trips can zero those registers before the loop
// head reloads, so re-materialize them every iteration.
//=============================================================================
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
  const uint32_t probe_slot = ctx.r11.u32;
  ctx.r3.u64 = REX_LOAD_U32(ctx.r29.u32 + 12884);
  const uint32_t probe_queue_head = ctx.r3.u32;
  ctx.r11.s64 = static_cast<int64_t>(ctx.r11.u64 * static_cast<uint64_t>(28));
  ctx.r11.u64 = ctx.r11.u64 + ctx.r31.u64;
  ctx.r10.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
  const uint32_t probe_work_item = ctx.r10.u32;
  REX_STORE_U32(ctx.r11.u32 + 4, ctx.r28.u32);
  ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 0) & 0xFFFFFFFFFFFFFFFB;
  REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
  {
    const auto yield_start = ProbeClock::now();
    ctx.lr = 0x824C0600;
    DOAX_FiberYield(ctx, base);
    const uint64_t yield_us = ProbeElapsedUs(yield_start);
    const auto drain_start = ProbeClock::now();
    ctx.lr = 0x824C0604;
    DOAX_SchedulerDrainDispatch(ctx, base);
    const uint64_t drain_us = ProbeElapsedUs(drain_start);
    ProbeWorkQueueIteration(ctx, probe_slot, probe_queue_head, probe_work_item, yield_us,
                            drain_us);
  }
  goto loc_824C05DC_hook;
}

//=============================================================================
// Fiber yield shim (0x82783210). Log dispatcher/cdf8/work-queue yields.
//=============================================================================
extern "C" REX_FUNC(DOAX_FiberYield) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t target = static_cast<uint32_t>(ctx.r3.u64);
  // Guest-PC fiber swap does not restore PPC callee-saves (r14-r31). Work-fiber
  // loops (sub_824C1548 @ lr=0x824C15F4, sub_825A2560 @ lr=0x825A25E0,
  // DOAX_SchedulerDrainDispatch inner loop @ lr=0x824C0C3C, ...) branch back after yield
  // without re-running their register setup prologues. Do not restore on the
  // hooked dispatcher/cdf8 yields — those paths re-materialize globals after.
  SchedulerFiberGprs saved_gprs{};
  const bool preserve_gprs = NeedsFiberCalleeSavePreserve(caller_lr);
  if (preserve_gprs) {
    SaveSchedulerFiberGprs(ctx, saved_gprs);
  }
  DOAX_FiberContextSwitch(ctx, base);
  if (preserve_gprs) {
    RestoreSchedulerFiberGprs(ctx, saved_gprs);
  }
  if (caller_lr == kSchedulerFiberYieldLr) {
    EnforceTravelOverlayGuard(base, "menu-fiber-yield");
    MaybeFastCompleteTravelFade(base, "menu-fiber-yield");
    MaybeLogTravelCase3Probe(base, "menu-fiber-yield", -1, -1, -1, caller_lr);
  }
  MaybeLogFiberYield(caller_lr, target, static_cast<uint32_t>(ctx.lr));
}

//=============================================================================
// XInput wrapper (0x82782BF0) -> XamInputGetState.
//=============================================================================
extern "C" REX_FUNC(sub_82782BF0) {
  const uint32_t user = static_cast<uint32_t>(ctx.r3.u64);
  const uint32_t state_ptr = ctx.r4.u32;
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  ctx.r5.u64 = ctx.r4.u64;
  ctx.r4.s64 = 0;
  __imp__XamInputGetState(ctx, base);
  uint32_t buttons = 0;
  if (state_ptr != 0) {
    buttons = REX_LOAD_U16(state_ptr + 4);
  }
  MaybeLogInputProbe("GetState", user, static_cast<uint32_t>(ctx.r3.u64), buttons, caller_lr,
                     buttons != 0);
}

//=============================================================================
// Controller poll loop (0x8274B650). Init-time + per-controller input setup.
//=============================================================================
extern "C" REX_FUNC(sub_8274B650) {
  auto& probe = g_input_probe;
  ++probe.poll_calls;
  if (probe.log_count < kInputProbeLogCap) {
    REXKRNL_WARN("DOAX input-probe site=PollEnter call={} r3=0x{:08X} lr=0x{:08X}",
                 probe.poll_calls, static_cast<uint32_t>(ctx.r3.u64),
                 static_cast<uint32_t>(ctx.lr));
    ++probe.log_count;
  }
  __imp__sub_8274B650(ctx, base);
  if (probe.log_count < kInputProbeLogCap) {
    REXKRNL_WARN("DOAX input-probe site=PollReturn call={} lr=0x{:08X}", probe.poll_calls,
                 static_cast<uint32_t>(ctx.lr));
    ++probe.log_count;
  }
}

//=============================================================================
// Menu item confirm (0x82671308). Work-fiber menu cases call this before scene
// transition; suppress flag2 re-arm until the transition can finish.
//=============================================================================
extern "C" REX_FUNC(DOAX_MenuItemConfirm) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t item_id = static_cast<uint32_t>(ctx.r3.u64);
  const uint32_t arg2 = static_cast<uint32_t>(ctx.r4.u64);
  if (caller_lr == kMenuPostConfirmCleanupLr) {
    uint32_t cursor = 0xFFFFFFFFu;
    if (ctx.r28.u32 != 0) {
      cursor = REX_LOAD_U8(ctx.r28.u32 + 16147);
    }
    MaybeArmBootFiberForTravel(base, "menu-post-confirm", caller_lr, item_id, item_id,
                               arg2);
    g_suppress_menu_kick = true;
    MaybeLogMenuSelection(caller_lr, item_id, cursor);
    MaybeLogSchedulerSnapshot("menu-post-confirm", ReadSchedulerSnapshot(base));
  } else if (IsMenuWorkFiberConfirm(caller_lr)) {
    uint32_t cursor = 0xFFFFFFFFu;
    if (ctx.r28.u32 != 0) {
      cursor = REX_LOAD_U8(ctx.r28.u32 + 16147);
    }
    g_suppress_menu_kick = true;
    MaybeArmBootFiberForTravel(base, "menu-travel-confirm", caller_lr, item_id, item_id,
                               arg2);
    MaybeLogMenuSelection(caller_lr, item_id, cursor);
  }
  __imp__DOAX_MenuItemConfirm(ctx, base);
}

//=============================================================================
// Menu work-fiber loop (0x824C1548). On exit clears byte_833B8514 then cdf8.
//=============================================================================
extern "C" REX_FUNC(DOAX_MenuWorkFiberLoop) {
  g_doax_hook_guest_base = base;
  __imp__DOAX_MenuWorkFiberLoop(ctx, base);
  if (g_travel_overlay_guard) {
    EnforceTravelOverlayGuard(base, "menu-fiber-exit");
    if (g_travel_overlay_guard_logs < kTravelOverlayGuardLogCap) {
      ++g_travel_overlay_guard_logs;
      const MenuTravelProbeState state = ReadMenuTravelProbeState(base);
      REXKRNL_WARN(
          "DOAX travel-guard: menu-fiber-return deb={} def={} dea={} overlay={} present={}",
          state.deb, state.def, state.dea, state.overlay, state.present_idx);
    }
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

//=============================================================================
// Menu scene transition (0x824C1958). Called from LABEL_12 after the menu
// work-fiber validates a selection — this is the real Travel handoff, not the
// case-0 idle calls to DOAX_MenuItemConfirm during CEF0 bring-up.
//=============================================================================
extern "C" REX_FUNC(DOAX_MenuSceneTransition) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t scene_id = static_cast<uint32_t>(ctx.r3.u64);
  if (g_menu_scene_transition_logs < kMenuSceneTransitionLogCap) {
    ++g_menu_scene_transition_logs;
    REXKRNL_WARN("DOAX scene-transition: enter scene_id={} lr=0x{:08X} expected={} label34={}",
                 scene_id, caller_lr, IsPostMenuConfirmSceneTransition(caller_lr),
                 IsLabel34SceneTransition(caller_lr));
  }
  ArmBootFiberSuppress(base, "scene-transition", caller_lr, scene_id);
  MaybeLogSchedulerSnapshot("scene-transition", ReadSchedulerSnapshot(base));
  __imp__DOAX_MenuSceneTransition(ctx, base);
  if (g_travel_fade_complete) {
    return;
  }
  EnforceTravelOverlayGuard(base, "scene-transition-return");
}

//=============================================================================
// Island scene load (0x82538048). Menu work-fiber case 3; fallback arm if
// scene-transition hook never logged (observed when lr-gated suppress missed).
//=============================================================================
extern "C" REX_FUNC(DOAX_IslandSceneLoad) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  if (g_suppress_menu_kick) {
    ArmBootFiberSuppress(base, "island-scene-load", caller_lr, 0);
  }
  if (g_island_scene_load_logs < kIslandSceneLoadLogCap) {
    ++g_island_scene_load_logs;
    REXKRNL_WARN("DOAX island-scene-load: lr=0x{:08X} menu_kick={} boot_suppress={}",
                 caller_lr, g_suppress_menu_kick, g_suppress_boot_work_fiber);
  }
  __imp__DOAX_IslandSceneLoad(ctx, base);
  EnforceTravelOverlayGuard(base, "island-scene-load-return");
}

//=============================================================================
// Menu transition fade / ready probes (case 3 gate, IDA 0x824C1814).
//=============================================================================
extern "C" REX_FUNC(DOAX_MenuTransitionReadyCheck) {
  __imp__DOAX_MenuTransitionReadyCheck(ctx, base);
  const int32_t ready = static_cast<int32_t>(ctx.r3.u64);
  MaybeLogTravelCase3Probe(base, "ready-check", ready, -1, -1,
                           static_cast<uint32_t>(ctx.lr));
}

extern "C" REX_FUNC(DOAX_MenuTransitionFadeAlpha) {
  __imp__DOAX_MenuTransitionFadeAlpha(ctx, base);
  const int32_t fade = static_cast<int32_t>(ctx.r3.u64);
  MaybeLogTravelCase3Probe(base, "fade-alpha", -1, fade, -1, static_cast<uint32_t>(ctx.lr));
}

extern "C" REX_FUNC(DOAX_MenuTransitionTimeline) {
  __imp__DOAX_MenuTransitionTimeline(ctx, base);
  const int32_t timeline = static_cast<int32_t>(ctx.r3.u64);
  MaybeLogTravelCase3Probe(base, "timeline", -1, -1, timeline, static_cast<uint32_t>(ctx.lr));
  ArmTravelFadeMilestone(base, timeline, "timeline");
  MaybeFastCompleteTravelFade(base, "timeline-df0");
  CompleteTravelFade(base, timeline, "timeline");
}

//=============================================================================
// Boot intro work-fiber loop (0x8250A0B8). Resume lr=0x8250A104.
//=============================================================================
extern "C" REX_FUNC(DOAX_BootWorkFiberLoop) {
  if (g_suppress_boot_work_fiber) {
    if (g_boot_fiber_suppress_logs < kBootFiberSuppressLogCap) {
      ++g_boot_fiber_suppress_logs;
      REXKRNL_WARN("DOAX boot-fiber-suppress: skip DOAX_BootWorkFiberLoop lr=0x{:08X}",
                   static_cast<uint32_t>(ctx.lr));
    }
    return;
  }
  __imp__DOAX_BootWorkFiberLoop(ctx, base);
}

//=============================================================================
// Boot intro work-fiber body (0x8250A568). Fiber resumes at 0x8250A104 directly
// into this function — loop-level suppress never runs. The body vtable dispatch
// at 0x8250A6BC tears down island even when present-state update is blocked.
//=============================================================================
extern "C" REX_FUNC(DOAX_BootWorkFiberBody) {
  if (g_suppress_boot_work_fiber && !g_travel_fade_complete) {
    if (g_boot_fiber_suppress_logs < kBootFiberSuppressLogCap) {
      ++g_boot_fiber_suppress_logs;
      REXKRNL_WARN("DOAX boot-fiber-suppress: skip DOAX_BootWorkFiberBody lr=0x{:08X}",
                   static_cast<uint32_t>(ctx.lr));
    }
    EnforceTravelOverlayGuard(base, "boot-fiber-body-skip");
    MaybeFastCompleteTravelFade(base, "boot-fiber-body-skip");
    return;
  }
  __imp__DOAX_BootWorkFiberBody(ctx, base);
  if (g_travel_overlay_guard) {
    EnforceTravelOverlayGuard(base, "boot-fiber-body-return");
  }
}

//=============================================================================
// Boot present-state helper (0x8250BEB0). DOAX_BootWorkFiberBody may call DOAX_BootMovieReplayTeardown
// when byte_833BB763 != 5, replaying boot movie / scheduler drain.
//=============================================================================
extern "C" REX_FUNC(DOAX_BootPresentStateUpdate) {
  if (g_suppress_boot_work_fiber && !g_travel_fade_complete) {
    if (g_boot_fiber_suppress_logs < kBootFiberSuppressLogCap) {
      ++g_boot_fiber_suppress_logs;
      REXKRNL_WARN("DOAX boot-fiber-suppress: skip DOAX_BootPresentStateUpdate lr=0x{:08X}",
                   static_cast<uint32_t>(ctx.lr));
    }
    return;
  }
  __imp__DOAX_BootPresentStateUpdate(ctx, base);
}

//=============================================================================
// Boot movie replay teardown (0x8250A728). Calls DOAX_SchedulerDrainWake and resets scene
// slots — observed to collapse island (draws~1300 -> 37) after Travel confirm.
//=============================================================================
extern "C" REX_FUNC(DOAX_BootMovieReplayTeardown) {
  if (g_suppress_boot_work_fiber) {
    if (g_boot_fiber_suppress_logs < kBootFiberSuppressLogCap) {
      ++g_boot_fiber_suppress_logs;
      REXKRNL_WARN("DOAX boot-fiber-suppress: skip DOAX_BootMovieReplayTeardown lr=0x{:08X}",
                   static_cast<uint32_t>(ctx.lr));
    }
    return;
  }
  __imp__DOAX_BootMovieReplayTeardown(ctx, base);
}

//=============================================================================
// Main-menu state setup (0x8258CEF0). Four menu slots (li r7,4).
//=============================================================================
extern "C" REX_FUNC(DOAX_MainMenuRegisterSlots) {
  const uint32_t caller_lr = static_cast<uint32_t>(ctx.lr);
  MaybeLogMenuProbe("CEF0-enter", caller_lr);
  __imp__DOAX_MainMenuRegisterSlots(ctx, base);
  ArmSchedulerDispatchAfterMenuInit(base, caller_lr);
  MaybeLogMenuProbe("CEF0-return", caller_lr);
}

//=============================================================================
// Main-menu island scene bring-up (0x8258E000). Calls CEF0 then scene setup.
//=============================================================================
extern "C" REX_FUNC(DOAX_IslandSceneBringUp) {
  if (g_menu_scene_probe_logs < kMenuSceneProbeLogCap) {
    ++g_menu_scene_probe_logs;
    MaybeLogSchedulerSnapshot("E000-enter", ReadSchedulerSnapshot(base));
  }
  __imp__DOAX_IslandSceneBringUp(ctx, base);
  if (g_menu_scene_probe_logs < kMenuSceneProbeLogCap) {
    MaybeLogSchedulerSnapshot("E000-return", ReadSchedulerSnapshot(base));
  }
}

//=============================================================================
// Movie playback probes — grep log for "DOAX movie-probe".
//=============================================================================
extern "C" REX_FUNC(DOAX_PlayMovie) {
  if (g_movie_probe_logs < kMovieProbeLogCap) {
    ++g_movie_probe_logs;
    REXKRNL_WARN("DOAX movie-probe PlayMovie r3={} r4={} lr=0x{:08X}",
                 static_cast<uint32_t>(ctx.r3.u64), static_cast<uint32_t>(ctx.r4.u64),
                 static_cast<uint32_t>(ctx.lr));
  }
  __imp__DOAX_PlayMovie(ctx, base);
}

//=============================================================================
// Indirect input dispatch (0x82782B58).
//=============================================================================
extern "C" REX_FUNC(sub_82782B58) {
  auto& probe = g_input_probe;
  ++probe.indirect_calls;
  if (probe.log_count < kInputProbeLogCap) {
    REXKRNL_WARN("DOAX input-probe site=IndirectEnter call={} r3=0x{:08X} lr=0x{:08X}",
                 probe.indirect_calls, static_cast<uint32_t>(ctx.r3.u64),
                 static_cast<uint32_t>(ctx.lr));
    ++probe.log_count;
  }
  __imp__sub_82782B58(ctx, base);
  if (probe.log_count < kInputProbeLogCap) {
    REXKRNL_WARN("DOAX input-probe site=IndirectReturn call={} r3=0x{:08X} lr=0x{:08X}",
                 probe.indirect_calls, static_cast<uint32_t>(ctx.r3.u64),
                 static_cast<uint32_t>(ctx.lr));
    ++probe.log_count;
  }
}

//=============================================================================
// Skip cutscene movies (midasm hooks).
//=============================================================================

bool DOAX_SkipNinjaViHdMovie() {
  if (!kSkipNinjaViHdMovie) {
    return false;
  }
  static uint32_t skip_logs = 0;
  if (skip_logs < kMovieProbeLogCap) {
    ++skip_logs;
    REXKRNL_WARN("DOAX movie-probe SkipNinjaViHd (midasm) log={}", skip_logs);
  }
  return true;
}

bool DOAX_SkipLicenseWarningIntro() {
  if (!kSkipLicenseWarning) {
    return false;
  }
  static bool logged = false;
  if (!logged) {
    REXKRNL_WARN(
        "DOAX: skipping license warning intro block (0x8250AAA8 -> 0x8250AAFC)");
    logged = true;
  }
  return true;
}

//=============================================================================
// Midasm: LABEL_34 byte_833B8DEB decrement — keep menu fiber alive during Travel.
//=============================================================================
void DOAX_GuardMenuFiberDebAfterDecrement(PPCRegister& /*r31*/) {
  if (!g_travel_overlay_guard || g_doax_hook_guest_base == nullptr) {
    return;
  }
  uint8_t* base = g_doax_hook_guest_base;
  if (REX_LOAD_U8(kDoaxMenuFiberDebAddr) != 0) {
    return;
  }
  REX_STORE_U8(kDoaxMenuFiberDebAddr, 1);
  if (g_travel_overlay_guard_logs < kTravelOverlayGuardLogCap) {
    ++g_travel_overlay_guard_logs;
    const MenuTravelProbeState state = ReadMenuTravelProbeState(base);
    REXKRNL_WARN(
        "DOAX travel-guard: prevent deb=0 deb={} def={} dea={} overlay={}",
        state.deb, state.def, state.dea, state.overlay);
  }
}

//=============================================================================
// Midasm: menu fiber exit overlay clear (0x824C1948 stb byte_833B8514).
//=============================================================================
bool DOAX_SkipIslandOverlayClearOnTravel() {
  if (!g_travel_overlay_guard) {
    return false;
  }
  if (g_travel_overlay_guard_logs < kTravelOverlayGuardLogCap) {
    ++g_travel_overlay_guard_logs;
    const MenuTravelProbeState state =
        g_doax_hook_guest_base != nullptr ? ReadMenuTravelProbeState(g_doax_hook_guest_base)
                                          : MenuTravelProbeState{};
    REXKRNL_WARN(
        "DOAX travel-guard: skip overlay-clear deb={} def={} dea={} overlay={}",
        state.deb, state.def, state.dea, state.overlay);
  }
  return true;
}
