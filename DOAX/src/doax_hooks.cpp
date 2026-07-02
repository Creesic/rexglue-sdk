#include "doax_hooks.h"

#include "generated/default/doax_init.h"

#include <cstdint>
#include <utility>

#include <rex/gpu_sync_diag.h>  // TEMP_DIAG: lock-free worker sync trace
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/guest_pc_fiber.h>

// ============================================================================
// DOAX hooks — CLEAN BASELINE + Group 1 (guest-PC fiber reentry) ONLY.
//
// Restart from a from-scratch baseline. The complete prior hook set is archived
// at DOAX/archive/full-hooks-2026-06-27/ (see its README for the 6 groups +
// restore workflow). This file restores ONLY the load-bearing fiber reentry, in
// MINIMAL form:
//   - DOAX_FiberContextSwitch (0x82785670) routed through the OS-fiber-backed
//     rex::ppc::RunFiberSwap. The static recomp needs this because the guest
//     context switch rewrites LR + the stack (KeSetCurrentStackPointers), which
//     a straight-line recompile cannot represent.
//   - Guest-region config registration + interpreter install.
//
// DELIBERATELY OMITTED vs the archive (suspected cruft, not core fiber):
//   * The DOAX_FiberYield hook. Guest 0x82783210 is a thunk -> the context
//     switch, so the generated yield already routes through the hook below.
//   * The callee-saved GPR (r14-r31) preserve band-aid that was gated to 3
//     specific menu/scheduler yield LRs (NeedsFiberCalleeSavePreserve). If a
//     menu fiber yield corrupts r14-r31 across a swap-back, that is a real bug
//     in the recompiled context switch / RunFiberSwap to fix UNIFORMLY in the
//     runtime — not three per-LR patches here.
//   * All press-start / menu / scheduler state-forcing and the ~40 g_* resets.
// ============================================================================

namespace {

constexpr uint32_t kGuestFiberSwapAddress = 0x82785670u;

// Group 4 (partial): boot intro skips. Bounded logging so we can confirm each
// midasm hook fires and correlate with the on-screen sequence
// (legal/health warning -> Team NINJA movie -> Press Start).
uint32_t g_midasm_logs = 0;
constexpr uint32_t kMidasmLogCap = 32;

// The guest's own register save/restore (the recompiled context switch). Passed
// to RunFiberSwap, which uses it to move PPCContext between the current and the
// target fiber's guest context block on either side of the host-fiber switch.
void FiberSwapImpl82785670(PPCContext& ctx, uint8_t* base) {
  __imp__DOAX_FiberContextSwitch(ctx, base);
}

void RegisterDoaxGuestPcFiberConfig(const rex::PPCImageInfo& image_info) {
  rex::ppc::GuestPcFiberConfig config;
  config.guest_regions.push_back(
      {image_info.image_base, image_info.image_base + image_info.image_size});
  rex::ppc::RegisterGuestPcFiberConfig(std::move(config));
}

// --- Groups 2 + 3: scheduler / work-queue register handling --------------------
// The recompiled scheduler/work-queue code loses callee-saved registers (r29-r31)
// across its embedded fiber yields and ends up dispatching a worker fiber with a
// null "job" (guest r31 == 0), which faults writing through it in DrainWake.
// These reimplementations re-derive r29-r31 from the work-queue table constants
// rather than trusting them across the yield. (Combed from the archive: dropped
// the logging/diag, the SchedulerSnapshot state-forcing, and the GPR band-aid
// that the runtime RunFiberSwap non-volatile preserve now handles.)
constexpr uint32_t kDoaxSchedulerFlagAddr = 0x833B8DF8u;
constexpr int64_t kDoaxWorkQueueSegBase = -2092302336;
constexpr int64_t kDoaxWorkQueueTableBase = kDoaxWorkQueueSegBase + 12888;
constexpr uint32_t kSchedulerHeaderBytes = 16;

// EXPERIMENTAL suppress-boot-replay override. The press-start->menu black is the boot
// present machine flipping to stage 2 (boot-replay), whose teardown TEARS DOWN THE 3D
// ISLAND scene -> black. In a good run the island PERSISTS as the menu background and the
// 4-menu overlays on it (present stays at 1). So instead of forcing the menu (which also
// tears the island down), this HOLDS the present machine at stage 1 during the press-start
// transition (content gate ready) by canceling any sphase/target/present==2. Set false to
// disable. (Forcing the menu flags = WORSE: instant island black, confirmed.)
constexpr bool kForceMenuOnGate = false;  // COUNTDOWN GATE EXHAUSTED (doax_041 hold->sits, doax_043 release->black):
                                          // load is NOT the discriminator (done in good AND black); the real diff is
                                          // the scheduler PARK vs mode-ADVANCE (synctrace: DOAX_MenuTransitionSetup
                                          // 0x824C0FB0 fires in black only) - a fiber-timing race no symptom byte controls.
constexpr uint32_t kMaxCountdownRearms = 16;  // ~16 x 15-frame windows (~4-7s) for the load; then fall through (orig)

// EXPERIMENTAL fix: the menu-transition loop counter byte_833B8DEB gets clobbered to 255
// ("loop forever") by DOAX_MenuTransitionSetup (a scheduler mode-callback) when sphase
// (byte_833B8DF9) == 2 lands MID-TRANSITION. DOAX_MenuWorkFiberLoop's case-3 exit only
// decrements the counter when it's < 100, so 255 = the transition never terminates = BLACK.
// When we see 255 while the menu fiber is active (byte_833B8DE8==1) and in case 3
// (byte_833B8DEA==3), clamp it to 1 so the transition completes and the menu appears.
constexpr bool kClampMenuLoop = false;

// EXPERIMENTAL fix: DOAX_BootWarningDismiss commits the boot-replay (DOAX_PresentTargetState=2 =
// black) when the countdown expires (sphase byte_833B8DF9 == 2). The menu transition (timeline=900)
// is identical in good/black; the black is just present->2 landing DURING that transition. So when
// the menu fiber is active (byte_833B8DE8==1), suppress that target=2 commit so the in-progress
// menu finishes instead of being torn to black.
constexpr bool kSuppressBlackDuringMenu = false;  // OFF 2026-06-29: present held at 1 was STILL black

// EXPERIMENT (doax_039 good-run synctrace): the GOOD run's boot fiber just CYCLES present=1 forever
// (BootWorkFiberBody->...->BootPresentStateUpdate->CameraInterp), island+menu rendering, never reaching
// the teardown. BLACK reaches DOAX_BootPresentStateUpdate's teardown branch (present!=5 && !MenuModeFlag)
// which runs DOAX_BootMovieReplayTeardown (DESTROYS the island) + target=2. The journal only suppressed
// the present=2 STATE (after the teardown already ran -> still black). FIX: stop the teardown FUNCTION
// from running while the menu fiber is active + present is still at the good state (1), by pre-setting
// BootPresentStateUpdate's own re-entry guard dword_83984924 so __imp__ no-ops. Present holds at 1, the
// boot fiber keeps cycling = the good state. Paired with kSuppressBlackDuringMenu (BootWarningDismiss's
// target=2). If present still leaves 1, the gammadiag/bootgate log shows the remaining target=2 source.
constexpr bool kBlockTeardownDuringMenu = false;  // OFF 2026-06-29: present held at 1 was STILL black -> teardown is NOT the cause; black is render content

// === EXPERIMENTAL content-ready gamma gate + render-layer diagnostic (2026-06-28) ===========
// VERIFIED this session: the title->4-menu black is NOT a broken event primitive -- rex_SetEvent
// (the worker wake; "rex_PulseEvent" was a misnamed SetEvent), the auto-reset SynchronizationEvents,
// and the SignalObjectAndWait barrier are all faithful. So it's a genuine per-frame producer/consumer
// race: the main render loop (DOAX_FrontEndMainLoop) advances past worker-thread-loaded menu/scene
// content that isn't ready yet. The VISIBLE black is gamma (DOAX_GammaFadeTick lerps the ramp toward a
// target over DOAX_GammaFadeCounter frames) and/or the present machine tearing down the island
// (present 0x833BB763 -> 2). Earlier journal fixes already tried gating present=2 and the menu loop and
// FAILED; the GAMMA layer is the one never instrumented. This hook (runs every frame, main thread):
//   * DIAGNOSTIC (always): logs gamma cur/target floats + present/target + overlay + sprite-load state
//     on change, so a good-vs-black capture pins WHICH blacks the island and whether content is ready.
//   * GATE (kHoldFadeUntilContentReady): holds the fade one step short of its final snap while a menu
//     transition is active AND menu sprites are still loading, capped at kMaxHeldFrames (then falls
//     through). Touches only the main-thread gamma counter -> cannot deadlock the worker ring.
constexpr bool kHoldFadeUntilContentReady = false;  // gamma RULED OUT (doax_033: counter/lvl never move); diag kept

// EXPERIMENT (from doax_036 probe): the menu fiber auto-confirms the default item because
// DOAX_MenuValidateSelection reads "selection-highlight sprite LOADED" (slot state 255 + item set by the
// pump DOAX_MenuSelectionSpritePumpSlot) as "item CONFIRMED" -- kicking it from the navigable case 1 into
// the case-3 scene transition with NO input. Suppress that: force validate to return -1 so the fiber IDLES
// at case 1 = the navigable 4-menu. Trade-off while on: you also cannot CONFIRM an item (this is a "does the
// navigable menu appear" test); a real fresh-A-edge gate replaces the blanket suppress once that's confirmed.
constexpr bool kSuppressMenuAutoConfirm = false;  // RULED OUT (doax_037): the "confirm" is the NORMAL
                                                  // press-start->main-menu SCENE TRANSITION; suppressing it
                                                  // removes the island entirely. Keep the validate PROBE only.
constexpr uint32_t kMaxHeldFrames = 120;  // ~2s @60fps cap, then let the fade complete regardless

float GammaF(uint8_t* base, uint32_t addr) {
  const uint32_t u = REX_LOAD_U32(addr);
  float f;
  __builtin_memcpy(&f, &u, sizeof(f));
  return f;
}

// SCORCHED-EARTH function-entry trace: record each instrumented menu/fiber/scheduler
// function entry into the lock-free ring (op=Func, handle=func addr, handle2=present,
// result=menu-case byte_833B8DEA). Dumps with synctrace_*.txt; diff the Func sequence
// good vs black to see exactly what the transition does differently.
void RecF(uint8_t* base, uint32_t addr) {
  gpu_sync_diag::Record(gpu_sync_diag::OP_FUNC, addr, REX_LOAD_U8(0x833BB763u),
                        static_cast<uint16_t>(REX_LOAD_U8(0x833B8DEAu)));
}

// Menu state machine + transition state, logged on change (DOAX_MenuWorkFiberLoop case =
// byte_833B8DEA drives the in-menu transition; the press-start->menu fork lives here).
void MenuStateDiag(uint8_t* base) {
  const uint32_t mcase = REX_LOAD_U8(0x833B8DEAu);   // DOAX_MenuWorkFiberLoop case selector
  const uint32_t mloop = REX_LOAD_U8(0x833B8DEBu);   // loop counter
  const uint32_t mflag = REX_LOAD_U8(0x833B8DEFu);   // pre-transition flag
  const uint32_t mitem = REX_LOAD_U8(0x833B8DECu);   // selected item
  const uint32_t mact = REX_LOAD_U8(0x833B8DE8u);    // menu-fiber active flag
  const uint32_t tl = REX_LOAD_U32(0x833B8DF4u);     // transition timeline value
  static uint64_t s_prev = ~0ull;
  const uint64_t key = mcase | (uint64_t(mloop) << 8) | (uint64_t(mflag) << 16) |
                       (uint64_t(mitem) << 24) | (uint64_t(mact) << 32) | (uint64_t(tl) << 40);
  if (key == s_prev) {
    return;
  }
  s_prev = key;
  REXKRNL_WARN("DOAX menusm case={} loop={} flag={} item={} active={} timeline={}", mcase, mloop,
               mflag, mitem, mact, static_cast<int32_t>(tl));
}

void SchedulerGuardedIndirectCall(PPCContext& ctx, uint8_t* base, uint32_t target,
                                  uint64_t return_lr) {
  const uint64_t saved_r30 = ctx.r30.u64;
  const uint64_t saved_r31 = ctx.r31.u64;
  ctx.lr = return_lr;
  REX_CALL_INDIRECT_FUNC(target);
  // Re-derive callee-saved r30/r31 if the embedded fiber yield in the callee
  // dropped them (the null-deref crash guard). Register-only; the header is left
  // exactly as the callee wrote it.
  if (saved_r31 != 0 && ctx.r31.u64 != saved_r31) {
    ctx.r31.u64 = saved_r31;
  }
  if (saved_r30 != 0 && ctx.r30.u64 == 0) {
    ctx.r30.u64 = saved_r30;
  }
}

void RestoreDrainCallerRegs(PPCContext& ctx, uint64_t caller_lr, uint64_t caller_r29,
                            uint64_t caller_r30, uint64_t caller_r31) {
  if (static_cast<uint32_t>(caller_lr) == 0x824C0604u) {
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

// TEMP_DIAG: title->menu transition GATE probe (ida37 = real DOAX). The present
// state machine (DOAX_BootWorkFiberBody) only advances DOAX_PresentState
// (byte_833BB763) toward its target (byte_8341F9F6) when DOAX_BootMovieGateCheck
// (0x8250C468) returns 1 -- which requires a STORAGE/CONTENT subsystem to reach
// ready across ~5 slots (XamContent / overlapped-IO / semaphores). Black = one slot
// never reaches ready. Logs the gate determinants ON CHANGE (one per frame max via
// DrainDispatch) so a good-vs-bad capture shows exactly which slot/state stalls.
void LogBootGate(uint8_t* base) {
  const uint32_t present = REX_LOAD_U8(0x833BB763u);
  const uint32_t target = REX_LOAD_U8(0x8341F9F6u);
  const uint32_t presidx = REX_LOAD_U32(0x833B84C8u);
  const uint32_t dev = REX_LOAD_U32(0x82E1D360u);  // gate wants 7
  const uint32_t s1 = REX_LOAD_U32(0x82E1D870u), f1 = REX_LOAD_U8(0x82E1D86Cu);  // wants 6 / 0
  const uint32_t s2 = REX_LOAD_U32(0x82E1DD80u), f2 = REX_LOAD_U8(0x82E1DD7Cu);
  const uint32_t s3 = REX_LOAD_U32(0x82E1E290u), f3 = REX_LOAD_U8(0x82E1E28Cu);
  const uint32_t s4 = REX_LOAD_U32(0x82E1E7A0u), f4 = REX_LOAD_U8(0x82E1E79Cu);
  const uint32_t last = REX_LOAD_U32(0x82E2021Cu);  // wants 5
  const uint32_t fl_85094 = REX_LOAD_U32(0x83985094u);   // boot(0) vs menu(1) mode
  const uint32_t fl_204BC = REX_LOAD_U32(0x834204BCu);
  // present=1 = working menu (good); present=2 = boot-replay stall (BLACK). The bad
  // 1->2 fires in DOAX_BootPresentStateUpdate only when off_82E27BD8[1831] (a flag in
  // the storage/content context) is set + gate passes + guard924==0 + fl85094==0. The
  // storage gate cycles the same good/bad, so off_82E27BD8[1831] is the suspected
  // discriminator. Read it via the pointer at 0x82E27BD8 (+1831*4 = +0x1C9C), guarded.
  const uint32_t e_base = REX_LOAD_U32(0x82E27BD8u);
  const uint32_t e1831 =
      (e_base >= 0x82000000u && e_base < 0x84000000u) ? REX_LOAD_U32(e_base + 7324u) : 0xFFFFFFFFu;
  const uint32_t g924 = REX_LOAD_U32(0x83984924u);  // DOAX_BootPresentStateUpdate guard
  const uint32_t g920 = REX_LOAD_U32(0x83984920u);
  // Scheduler mode machine (DOAX_SchedulerDrainDispatch): DOAX_BootWarningDismiss forces
  // present=2 (BLACK) when sphase(0x833B8DF9)==2. sphase->2 when the mode countdown
  // sctdown(0x833B8DFE) expires with sff(0x833B8DFF)!=1. smode=0x833B8DFC, sactive=0x833B8DF8.
  const uint32_t sactive = REX_LOAD_U8(0x833B8DF8u);
  const uint32_t sphase = REX_LOAD_U8(0x833B8DF9u);
  const uint32_t smode = REX_LOAD_U8(0x833B8DFCu);
  const uint32_t sctdown = REX_LOAD_U8(0x833B8DFEu);
  const uint32_t sff = REX_LOAD_U8(0x833B8DFFu);

  // TEMP_DIAG: drive the lock-free worker sync trace. Arm a locator MARK when the
  // menu-confirm window opens (smode==2), then dump the recent sync window ONCE at
  // the terminal state: sphase/present==2 = BLACK (stranded worker), fl85094==1 =
  // GOOD (menu mode reached). One run is one or the other; g_dumped makes it a
  // one-shot. Diff synctrace_BLACK.txt vs synctrace_GOOD.txt at the teardown.
  static bool s_armed = false;
  if (!s_armed && smode == 2) {
    s_armed = true;
    gpu_sync_diag::Arm("confirm(smode=2)");
  }
  if (sphase == 2 || present == 2) {
    gpu_sync_diag::Dump("present2");  // trigger description, NOT a verdict
  }
  // GOOD is dumped from OnSwap (render keeps swapping past arm), NOT here: in a good
  // run the boot scheduler parks (sctdown freezes at 1) and this path stops being
  // called, and fl85094 stays 0 even in good (verified doax_053.log).

  static uint64_t s_prev = ~0ull;
  const uint64_t key = (uint64_t(present) << 56) ^ (uint64_t(target) << 48) ^
                       (uint64_t(presidx) << 40) ^ (uint64_t(dev) * 7) ^ (uint64_t(s1) * 11) ^
                       (uint64_t(s2) * 13) ^ (uint64_t(s3) * 17) ^ (uint64_t(s4) * 19) ^
                       (uint64_t(last) * 23) ^ (uint64_t(f1 + f2 + f3 + f4) << 8) ^
                       (uint64_t(fl_85094) * 29) ^ (uint64_t(fl_204BC) * 31) ^
                       (uint64_t(e1831) * 37) ^ (uint64_t(g924) * 41) ^ (uint64_t(g920) * 43) ^
                       (uint64_t(sactive) << 32) ^ (uint64_t(sphase) << 24) ^
                       (uint64_t(smode) * 53) ^ (uint64_t(sctdown) * 59) ^ (uint64_t(sff) * 61);
  if (key == s_prev) {
    return;
  }
  s_prev = key;
  // Full DOAX_BootMovieGateCheck condition (was only logging slot STATES, not Busy/Final):
  // gate = dev==7 && each slot {state==6, busy==0} && final==5. BootPresentStateUpdate:
  // when (e1831 && gate): fl85094!=0 -> menu path (target=5 needs presidx==3 && fl204BC);
  // fl85094==0 -> target=2 BLACK. So this line shows exactly which term blocks the menu.
  const bool gate = (dev == 7 && s1 == 6 && f1 == 0 && s2 == 6 && f2 == 0 && s3 == 6 && f3 == 0 &&
                     s4 == 6 && f4 == 0 && last == 5);
  REXKRNL_WARN(
      "DOAX bootgate present={} target={} presidx={} | GATE={} e1831={} g924={} fl85094={} "
      "fl204BC={} menu_ok={} | dev={} slots(st/busy)={}/{} {}/{} {}/{} {}/{} final={} | "
      "sphase={}(2=BLK) smode={} sctdown={} sff={} sactive={}",
      present, target, presidx, gate ? 1 : 0, e1831, g924, fl_85094, fl_204BC,
      (gate && e1831 && g924 == 0 && fl_85094 != 0 && presidx == 3 && fl_204BC != 0) ? 1 : 0, dev, s1,
      f1, s2, f2, s3, f3, s4, f4, last, sphase, smode, sctdown, sff, sactive);
}

// EXPERIMENTAL: clamp the menu-transition countdown so it can't take the final tick to 0.
// Called from the DrainDispatch hook BEFORE __imp__ decrements the countdown. The matched
// GOOD/BAD diff (doax_008 vs doax_007) is byte-identical until ONE extra DrainDispatch tick:
// GOOD freezes at present=1, smode=2, sff=0, sctdown=1, sactive=1 (countdown parks -> island
// stays, menu overlays); BAD ticks sctdown 1->0 -> sphase=2 -> present=2 -> black. So when we
// reach that exact frozen state, bump sctdown back to 2 so __imp__ decrements it to 1 (never
// 0), holding the present machine at 1 = the good state. (Distinct from the earlier seed-bump,
// which still let it expire later.)
// BLACK mechanism PINNED 2026-06-28 (doax_033 bootgate, render-layer): gamma never moves; the black
// is the boot present machine. The press-start->4-menu A press runs the mode-2 countdown with
// sff(byte_833B8DFF)==0; when it expires (sctdown->0) with sff!=1 the scheduler sets sphase=2 ->
// present=2 -> the 3D island is TORN DOWN = black. In the SAME log the earlier press-start-dismiss
// countdown expired with sff==1 -> sphase=1 (NO black). So sff-at-expiry IS the discriminator. The
// fatal (2nd) countdown is distinguished by the menu fiber being mid-transition: DOAX_MenuOverlayActive
// (byte_833B8514) == 1 (the 1st/good countdown has it == 0). FORCE sff=1 on the fatal expiry frame so
// __imp__ takes the continue path (sphase=1, present stays 1, island persists, the 4-menu overlays it).
// NOTE: the previous version of this fn HELD the countdown (sctdown 1->2) per a stale comment -- that
// just froze on press-start. Forcing sff (what the comment claimed) is the intended fix, never tested.
void ForceMenuTransition(uint8_t* base) {
  if (!kForceMenuOnGate) {
    return;
  }
  if (REX_LOAD_U8(0x833BB763u) != 1) {  // present==1 (island/menu good state)
    return;
  }
  if (REX_LOAD_U8(0x833B8DFCu) != 2) {  // smode==2 (menu-confirm mode)
    return;
  }
  if (REX_LOAD_U8(0x833B8DFFu) != 0) {  // sff==0 (the branch whose expiry forces sphase=2/black)
    return;
  }
  if (REX_LOAD_U8(0x833B8DFEu) != 1) {  // sctdown==1 (one frame from expiry inside __imp__)
    return;
  }
  // RELEASE-ON-LOAD countdown gate (good doax_039 vs gate-sits doax_041): the black is the menu LOAD vs
  // this 15-frame countdown. GOOD: load done by sctdown=1 -> the boot fiber PARKS at 1 (hands off to slot-4
  // DOAX_MenuWorkFiberLoop, scheduler stops, menu shows). BLACK: load not done -> sctdown 1->0 -> sphase=2 ->
  // present=2 -> teardown. doax_041 (re-arm UNCONDITIONALLY) prevented the teardown AND the load completed
  // (content_idx[0] 255->0) but it SAT on press-start: re-arming the instant sctdown==1 PREEMPTS the park
  // (bumps to 15 before the fiber can park at 1). FIX: re-arm ONLY WHILE the menu content slot 0 is still
  // loading (content_idx[0] dword_83426228 == 255). Once it loads (->0), STOP re-arming -> sctdown reaches 1
  // with the load DONE -> the fiber parks naturally = the good-run handoff. Capped so a stuck load can't hang.
  if (REX_LOAD_U32(0x83426228u) != 255u) {
    return;  // menu content slot 0 LOADED -> let sctdown reach 1 + park (do NOT re-arm)
  }
  static uint32_t s_rearms = 0;
  if (s_rearms >= kMaxCountdownRearms) {
    return;  // load still not done after the cap -> let it expire (original behavior; load is truly stuck)
  }
  ++s_rearms;
  REX_STORE_U8(0x833B8DFEu, 15u);  // load still in flight: fresh 15-frame window
  if (s_rearms <= 4 || (s_rearms % 8) == 0) {
    REXKRNL_WARN("DOAX CountdownGate: re-arm sctdown->15 (#{}/{}) - content_idx[0] still loading (255)",
                 s_rearms, kMaxCountdownRearms);
  }
}

// TEMP_DIAG: track the menu-loader = work-queue SLOT 2 (DOAX_SchedulerWorkerFiberLoop,
// spawned by DOAX_BootWarningFrame via sub_8258CD58). Work entries are 28 bytes at
// unk_834A3258 (0x834A3258); slot 2 is +56. Logs ONE-SHOT when slot 2 first becomes the
// dispatched index (dword_834A34F8==2) and on slot-2 state(+0)/func(+12) change. Good run:
// slot 2 spawns (func=DOAX_SchedulerWorkerFiberLoop 0x825A2560) + gets dispatched + cycles.
// Black run: we expect it never dispatched / never runs sub_825A1AA0 -> target stays at the
// 2 fallback. Called from the DispatchLoop reimpl each iteration (dedup keeps it bounded).
void WorkQueueDiag(uint8_t* base) {
  MenuStateDiag(base);  // menu state machine progression
  if (kClampMenuLoop && REX_LOAD_U8(0x833B8DEBu) == 255u && REX_LOAD_U8(0x833B8DE8u) == 1u &&
      REX_LOAD_U8(0x833B8DEAu) == 3u) {
    REX_STORE_U8(0x833B8DEBu, 1u);  // 255("forever") -> 1 so the menu transition can finish
    static bool s_clamped = false;
    if (!s_clamped) {
      s_clamped = true;
      REXKRNL_WARN("DOAX ClampMenuLoop: byte_833B8DEB 255->1 (active menu transition, case 3)");
    }
  }
  const uint32_t idx = REX_LOAD_U32(0x834A34F8u);
  static bool s_seen_idx2 = false;
  if (idx == 2 && !s_seen_idx2) {
    s_seen_idx2 = true;
    REXKRNL_WARN("DOAX workq: SLOT 2 (menu loader) DISPATCHED (dword_834A34F8==2)");
  }
  const uint32_t s2 = 0x834A3258u + 56u;  // slot 2 entry
  const uint32_t st = REX_LOAD_U32(s2 + 0u);
  const uint32_t func = REX_LOAD_U32(s2 + 12u);
  static uint32_t s_prev_st = 0xFFFFFFFFu;
  static uint32_t s_prev_func = 0xFFFFFFFFu;
  if (st != s_prev_st || func != s_prev_func) {
    s_prev_st = st;
    s_prev_func = func;
    REXKRNL_WARN("DOAX workq: slot2 state=0x{:X} func=0x{:08X} (curidx={})", st, func, idx);
  }
  // The menu-load completion the loader (sub_825A1AA0) actually polls: 4 content-slot
  // indices dword_83426228[0..3] (255 = unloaded; ==5 uses default dword_83DC8604) and
  // the loader's per-slot tracked state unk_82E2D94A[0..3] (8-byte stride). This is the
  // readiness the present machine waits on to advance 2->4 (menu) -- DISTINCT from the
  // storage GATE. In black one of these should be the slot that never reaches ready.
  const uint32_t ci0 = REX_LOAD_U32(0x83426228u), ci1 = REX_LOAD_U32(0x8342622Cu),
                 ci2 = REX_LOAD_U32(0x83426230u), ci3 = REX_LOAD_U32(0x83426234u);
  const uint32_t dflt = REX_LOAD_U32(0x83DC8604u);
  const uint32_t ls0 = REX_LOAD_U8(0x82E2D94Au), ls1 = REX_LOAD_U8(0x82E2D952u),
                 ls2 = REX_LOAD_U8(0x82E2D95Au), ls3 = REX_LOAD_U8(0x82E2D962u);
  static uint64_t s_prev_load = ~0ull;
  const uint64_t lk = (uint64_t(ci0 & 0xFF)) | (uint64_t(ci1 & 0xFF) << 8) |
                      (uint64_t(ci2 & 0xFF) << 16) | (uint64_t(ci3 & 0xFF) << 24) |
                      (uint64_t(ls0) << 32) | (uint64_t(ls1) << 40) | (uint64_t(ls2) << 48) |
                      (uint64_t(ls3) << 56);
  if (lk != s_prev_load) {
    s_prev_load = lk;
    REXKRNL_WARN("DOAX loadslots content_idx={}/{}/{}/{} dflt={} loaderstate=0x{:X}/0x{:X}/0x{:X}/0x{:X}",
                 ci0, ci1, ci2, ci3, dflt, ls0, ls1, ls2, ls3);
  }
  // All 24 work slots (unk_834A3258, 28B each): which are ACTIVE (state bit0). The menu
  // fiber is the slot that's active in GOOD but never in BLACK -- sub_8258CC00 only yields
  // to a slot's fiber when active(bit0) && !paused(bit1). Log the active-mask on change +
  // each newly-active slot's func(+12 = fiber entry).
  uint32_t mask = 0, newly = 0;
  static uint32_t s_prev_mask = 0xFFFFFFFFu;
  uint32_t funcs[24];
  for (uint32_t i = 0; i < 24; ++i) {
    const uint32_t e = 0x834A3258u + 28u * i;
    const uint32_t state = REX_LOAD_U32(e + 0u);
    funcs[i] = REX_LOAD_U32(e + 12u);
    if (state & 1u) {
      mask |= (1u << i);
    }
  }
  if (mask != s_prev_mask) {
    newly = mask & ~s_prev_mask;
    s_prev_mask = mask;
    for (uint32_t i = 0; i < 24; ++i) {
      if (newly & (1u << i)) {
        REXKRNL_WARN("DOAX slot{} ACTIVATED func=0x{:08X}", i, funcs[i]);
      }
    }
    REXKRNL_WARN("DOAX slotmask active=0x{:06X}", mask);
  }
  // Loader-slot gate. The loader fiber (func 0x825A2560) yields 66x in GOOD but 0x in BLACK
  // -> never dispatched. sub_8258CC00 dispatches a slot only when active(bit0) && !paused(bit1)
  // && its countdown(+4) decrements to 0. Log the gate of any slot running the loader, on
  // change, to see which term blocks it in black (paused? countdown stuck high? done bit3?).
  static uint32_t s_lst[24] = {};
  static uint32_t s_lcd[24] = {};
  static uint8_t s_lseen[24] = {};
  for (uint32_t i = 0; i < 24; ++i) {
    const uint32_t e = 0x834A3258u + 28u * i;
    if (REX_LOAD_U32(e + 12u) != 0x825A2560u) {
      continue;
    }
    const uint32_t st = REX_LOAD_U32(e + 0u);
    const uint32_t cd = REX_LOAD_U32(e + 4u);
    if (!s_lseen[i] || st != s_lst[i] || cd != s_lcd[i]) {
      s_lseen[i] = 1;
      s_lst[i] = st;
      s_lcd[i] = cd;
      REXKRNL_WARN("DOAX loaderslot{} state=0x{:X}(act={} paused={} done={}) countdown={} ctx=0x{:08X}",
                   i, st, st & 1u, (st >> 1) & 1u, (st >> 3) & 1u, cd, REX_LOAD_U32(e + 8u));
    }
  }
}

}  // namespace

void InstallDoaxGuestPcFiber(const rex::PPCImageInfo& image_info) {
  RegisterDoaxGuestPcFiberConfig(image_info);
  rex::ppc::InstallGuestPcFiberInterpreter();
  REXKRNL_WARN("DOAX Group-1 baseline: guest-PC fiber reentry only (swap override 0x{:08X})",
               kGuestFiberSwapAddress);
}

extern "C" REX_FUNC(DOAX_FiberContextSwitch) {
  rex::ppc::RunFiberSwap(ctx, base, &FiberSwapImpl82785670, static_cast<uint32_t>(ctx.r3.u64), 0);
}

// --- Group 4 (partial): boot intro skips (midasm hooks) ------------------------
// Each returns true so the recomp jumps to the hook's jump_address_on_true,
// skipping the intro's display/wait. Confirmed against the on-screen sequence.

bool DOAX_SkipLicenseWarningIntro() {
  if (g_midasm_logs < kMidasmLogCap) {
    ++g_midasm_logs;
    REXKRNL_WARN("DOAX midasm: skip legal/health warning");
  }
  return true;
}

bool DOAX_SkipNinjaViHdMovie() {
  if (g_midasm_logs < kMidasmLogCap) {
    ++g_midasm_logs;
    REXKRNL_WARN("DOAX midasm: skip Team NINJA movie");
  }
  return true;
}

// --- Groups 2 + 3: REVERTED to passthrough (2026-06-29) --------------------------
// Per FH1 learnings: don't reimplement the game's scheduler/work-queue. Each hook now
// just runs the original via __imp__; Group 1 fiber-reentry (DOAX_FiberContextSwitch /
// runtime RunFiberSwap) handles the cooperative fiber swaps faithfully. Manifest
// registrations + LogBootGate diag kept. (Journal: this passthrough = no crash.)

extern "C" REX_FUNC(DOAX_WorkQueueSlotWake) {
  // TARGETED REGISTER GUARD (kept after the 2026-06-29 Group 2/3 revert; FH1 #8/#9 pattern).
  // NOT a scheduler reimplementation - the scheduler dispatches a work-queue job whose fiber
  // context block carries r31=0 (null job, +288==0). DrainWake calls this right before
  // `stb r11,0(r31)` (doax_recomp.7.cpp:49140); without restoring r31 that store faults at
  // guest 0. Re-derive r31 from the caller value, or the scheduler header if it was dropped.
  const uint64_t caller_r31 = ctx.r31.u64;
  const uint64_t caller_r30 = ctx.r30.u64;
  __imp__DOAX_WorkQueueSlotWake(ctx, base);
  ctx.r31.u64 = caller_r31 != 0 ? caller_r31 : kDoaxSchedulerFlagAddr;
  if (caller_r30 != 0 && ctx.r30.u64 == 0) {
    ctx.r30.u64 = caller_r30;
  }
}

extern "C" REX_FUNC(sub_8258CDF8) {
  __imp__sub_8258CDF8(ctx, base);
}

extern "C" REX_FUNC(DOAX_SchedulerDrainWake) {
  __imp__DOAX_SchedulerDrainWake(ctx, base);
}

extern "C" REX_FUNC(DOAX_SchedulerDrainDispatch) {
  // TARGETED REGISTER GUARD: restore the dispatch loop's callee-saved r28-r31 across __imp__
  // (the deactivation branch clobbers them -> AV @ doax_recomp.7.cpp:48682). RunFiberSwap is clean.
  const uint64_t r28_in = ctx.r28.u64, r29_in = ctx.r29.u64, r30_in = ctx.r30.u64, r31_in = ctx.r31.u64;
  __imp__DOAX_SchedulerDrainDispatch(ctx, base);
  if ((r30_in & 0xFFFFFFFFu) == 0x834A0000u) {
    ctx.r28.u64 = r28_in;
    ctx.r29.u64 = r29_in;
    ctx.r30.u64 = r30_in;
    ctx.r31.u64 = r31_in;
  }

  WorkQueueDiag(base);
  LogBootGate(base);
}

extern "C" REX_FUNC(DOAX_WorkQueueDispatchLoop) {
  __imp__DOAX_WorkQueueDispatchLoop(ctx, base);
}

// --- SCORCHED-EARTH function-entry trace (each named in doax_manifest.toml) ------------
// Each records its entry (op=Func, present, menu-case) then runs the original via __imp__.
extern "C" REX_FUNC(DOAX_BootWorkFiberBody) {
  RecF(base, 0x8250A568u);
  __imp__DOAX_BootWorkFiberBody(ctx, base);
}
extern "C" REX_FUNC(DOAX_BootPresentStateUpdate) {
  RecF(base, 0x8250BEB0u);
  // Block the island teardown while the menu fiber is active and present is still at the good state (1):
  // pre-set this function's own re-entry guard (dword_83984924) so __imp__ returns BEFORE its teardown
  // branch (DOAX_BootMovieReplayTeardown + target=2). Present holds at 1 = the good cycling state.
  if (kBlockTeardownDuringMenu && REX_LOAD_U8(0x833B8DE8u) == 1u && REX_LOAD_U8(0x833BB763u) == 1u) {
    REX_STORE_U32(0x83984924u, 1u);
    static bool s_bt = false;
    if (!s_bt) {
      s_bt = true;
      REXKRNL_WARN("DOAX BlockTeardown: menu active + present=1 -> guard BootPresentStateUpdate (no island "
                   "teardown, hold present=1)");
    }
  }
  __imp__DOAX_BootPresentStateUpdate(ctx, base);
}
extern "C" REX_FUNC(DOAX_BootWarningDismiss) {
  RecF(base, 0x8250AC50u);
  const uint32_t tgt_before = REX_LOAD_U8(0x8341F9F6u);
  __imp__DOAX_BootWarningDismiss(ctx, base);
  // If it just committed target=2 (black) but the menu transition is active, undo it so the
  // in-progress menu can finish instead of going black.
  if (kSuppressBlackDuringMenu && REX_LOAD_U8(0x833B8DE8u) == 1u &&
      REX_LOAD_U8(0x8341F9F6u) == 2u && tgt_before != 2u) {
    REX_STORE_U8(0x8341F9F6u, static_cast<uint8_t>(tgt_before));
    static bool s_sb = false;
    if (!s_sb) {
      s_sb = true;
      REXKRNL_WARN("DOAX SuppressBlack: BootWarningDismiss target 2->{} (menu transition active)",
                   tgt_before);
    }
  }
}
extern "C" REX_FUNC(DOAX_WarningScreenUpdate) {
  RecF(base, 0x8250BB60u);
  __imp__DOAX_WarningScreenUpdate(ctx, base);
}
extern "C" REX_FUNC(DOAX_BootWarningFrame) {
  RecF(base, 0x8250AC98u);
  __imp__DOAX_BootWarningFrame(ctx, base);
}
extern "C" REX_FUNC(DOAX_BootMovieReplayTeardown) {
  RecF(base, 0x8250A728u);
  __imp__DOAX_BootMovieReplayTeardown(ctx, base);
}
extern "C" REX_FUNC(DOAX_MenuWorkFiberLoop) {
  RecF(base, 0x824C1548u);
  __imp__DOAX_MenuWorkFiberLoop(ctx, base);
}
extern "C" REX_FUNC(DOAX_MenuSceneTransition) {
  RecF(base, 0x824C1958u);
  __imp__DOAX_MenuSceneTransition(ctx, base);
}
extern "C" REX_FUNC(DOAX_IslandSceneLoad) {
  RecF(base, 0x82538048u);
  __imp__DOAX_IslandSceneLoad(ctx, base);
}
extern "C" REX_FUNC(DOAX_MenuPreTransitionHook) {
  RecF(base, 0x824C1460u);
  __imp__DOAX_MenuPreTransitionHook(ctx, base);
}
// The menu-transition setup: *(a1+1)==2 -> sub_824C14E8(0xFF) = loop-forever (BLACK);
// else -> deactivate menu fiber (GOOD). Log a1 + its first bytes to pin the deciding state.
extern "C" REX_FUNC(DOAX_MenuTransitionSetup) {
  const uint32_t a1 = ctx.r3.u32;
  RecF(base, 0x824C0FB0u);
  if (a1 >= 0x82000000u && a1 < 0x84000000u) {
    const uint32_t b0 = REX_LOAD_U8(a1 + 0u), b1 = REX_LOAD_U8(a1 + 1u),
                   b2 = REX_LOAD_U8(a1 + 2u), b3 = REX_LOAD_U8(a1 + 3u);
    static uint32_t s_prev = ~0u;
    const uint32_t key = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    if (key != s_prev) {
      s_prev = key;
      REXKRNL_WARN("DOAX MenuTransitionSetup a1=0x{:08X} bytes=[{} {} {} {}] (b1==2 -> loop-forever BLACK)",
                   a1, b0, b1, b2, b3);
    }
  }
  __imp__DOAX_MenuTransitionSetup(ctx, base);
}

extern "C" REX_FUNC(DOAX_FrontEndRenderTick) {
  __imp__DOAX_FrontEndRenderTick(ctx, base);
}

extern "C" REX_FUNC(DOAX_IslandSceneRender) {
  __imp__DOAX_IslandSceneRender(ctx, base);
}

// --- EXPERIMENT: content-ready gamma gate + render-layer diagnostic --------------------
// DOAX_GammaFadeTick (0x8258C070) is the per-frame gamma-fade interpolator, called from the
// front-end loop (DOAX_IslandSceneBringUp). See the kHoldFadeUntilContentReady block above.
extern "C" REX_FUNC(DOAX_GammaFadeTick) {
  const uint32_t overlay = REX_LOAD_U8(0x833B8514u);     // DOAX_MenuOverlayActive (transition active)
  const uint32_t counter = REX_LOAD_U32(0x834A2C24u);    // DOAX_GammaFadeCounter (pre-tick)
  const uint32_t sprite_busy = REX_LOAD_U32(0x83CAF8A4u);// menu sprite-cache load active (!=0 = loading)
  const bool transitioning = (overlay != 0u);
  const bool content_ready = (sprite_busy == 0u);

  // GATE: don't let the fade take its final snap (counter 1->0) while a transition is in
  // progress and content is still loading. Re-arm so __imp__ ticks 2->1 (no lerp, no snap).
  static uint32_t s_held = 0;
  if (kHoldFadeUntilContentReady && transitioning && !content_ready && counter == 1u &&
      s_held < kMaxHeldFrames) {
    REX_STORE_U32(0x834A2C24u, 2u);
    ++s_held;
    if (s_held == 1u) {
      REXKRNL_WARN("DOAX GammaGate HOLD: overlay=1 sprite_busy={} counter 1->held", sprite_busy);
    }
  } else {
    if (s_held != 0u) {
      REXKRNL_WARN("DOAX GammaGate RELEASE after {} frames (transitioning={} content_ready={})",
                   s_held, transitioning ? 1 : 0, content_ready ? 1 : 0);
    }
    s_held = 0;
  }

  __imp__DOAX_GammaFadeTick(ctx, base);

  // DIAGNOSTIC: render-layer state on change (dedup keeps it bounded). Captures the exact frame
  // the island blacks -> shows if it's gamma going dark (cur lvl/scale/bias -> ~0) or present
  // teardown (present 1->2), and whether content was ready (sprite_busy) at that instant.
  const uint32_t present = REX_LOAD_U8(0x833BB763u);     // DOAX_PresentState (1=menu/good, 2=black)
  const uint32_t target = REX_LOAD_U8(0x8341F9F6u);      // DOAX_PresentTargetState
  const float lvl = GammaF(base, 0x834A2C20u), scale = GammaF(base, 0x834A3230u),
              bias = GammaF(base, 0x834A3238u);
  const float tlvl = GammaF(base, 0x834A3228u), tscale = GammaF(base, 0x834A2C18u),
              tbias = GammaF(base, 0x834A3234u);
  const uint32_t cur_counter = REX_LOAD_U32(0x834A2C24u), total = REX_LOAD_U32(0x834A323Cu);
  const uint32_t mcase = REX_LOAD_U8(0x833B8DEAu);
  static uint64_t s_prev = ~0ull;
  const uint64_t key =
      uint64_t(present) | (uint64_t(target) << 4) | (uint64_t(overlay) << 8) |
      (uint64_t(sprite_busy != 0u) << 9) | (uint64_t(mcase) << 12) |
      (uint64_t(cur_counter & 0xFFFFu) << 16) | (uint64_t(uint32_t(lvl * 64.f) & 0xFFFFu) << 32) |
      (uint64_t(uint32_t(scale * 64.f) & 0xFFFFu) << 48);
  if (key != s_prev) {
    s_prev = key;
    REXKRNL_WARN("DOAX gammadiag present={} target={} overlay={} sprite_busy={} case={} | "
                 "cur(lvl/scl/bias)={:.3f}/{:.3f}/{:.3f} tgt={:.3f}/{:.3f}/{:.3f} counter={} total={}",
                 present, target, overlay, sprite_busy, mcase, lvl, scale, bias, tlvl, tscale, tbias,
                 cur_counter, total);
  }
}

// --- PROBE (log-only): why the press-start dismiss-A auto-confirms the default menu item ----------
// DOAX_MenuValidateSelection(item) scans byte_8394743C (stride 16, <=2 entries) for {state[0]==255,
// item[3]==a1} and returns its index (=CONFIRMED) or -1 (=idle). The menu fiber stays at case 1
// (navigable 4-menu) while this returns -1, and advances to the case-3 scene transition the moment it
// returns >=0. doax_035 shows it confirms default item 15 (Travel) with no navigation. Log a1 + return
// + both slot {state,item} pairs on change, so the capture shows WHEN slot0.state hits 255 (the confirm)
// relative to the menu opening and whether it climbs without input (the selection pump auto-completing).
extern "C" REX_FUNC(DOAX_MenuValidateSelection) {
  const uint32_t item = ctx.r3.u8;  // a1 = item being validated (DOAX_MenuSelectedItem)
  __imp__DOAX_MenuValidateSelection(ctx, base);
  const int32_t ret = static_cast<int32_t>(ctx.r3.u32);  // -1 = idle/navigable, 0/1 = CONFIRMED
  const uint32_t st0 = REX_LOAD_U8(0x8394743Cu), it0 = REX_LOAD_U8(0x8394743Cu + 3u);
  const uint32_t st1 = REX_LOAD_U8(0x8394743Cu + 16u), it1 = REX_LOAD_U8(0x8394743Cu + 19u);
  const uint32_t mcase = REX_LOAD_U8(0x833B8DEAu);  // DOAX_MenuCaseState
  static uint64_t s_prev = ~0ull;
  const uint64_t key = uint64_t(item) | (uint64_t(static_cast<uint8_t>(ret)) << 8) |
                       (uint64_t(st0) << 16) | (uint64_t(it0) << 24) | (uint64_t(st1) << 32) |
                       (uint64_t(it1) << 40) | (uint64_t(mcase) << 48);
  if (key != s_prev) {
    s_prev = key;
    REXKRNL_WARN("DOAX validate item={} ret={} | slot0(state/item)={}/{} slot1={}/{} case={}", item,
                 ret, st0, it0, st1, it1, mcase);
  }

  // GATE: suppress the sprite-load-as-confirm so the menu fiber idles at case 1 (navigable 4-menu)
  // instead of advancing to the case-3 scene transition. Return value is read as a (char), so ~0 = -1.
  if (kSuppressMenuAutoConfirm && ret >= 0) {
    ctx.r3.u64 = ~0ull;
    static uint32_t s_sup = 0;
    if (s_sup < 8) {
      ++s_sup;
      REXKRNL_WARN("DOAX AutoConfirmSuppress: validate {}->-1 (item={} case={}) idle@case1 (#{})", ret,
                   item, mcase, s_sup);
    }
  }
}

// --- PROBE: log FROM THE MENU FIBER (not the boot scheduler) -----------------------------------------
// DOAX_MenuWorkFiberLoop calls DOAX_MenuTransitionTimeline EVERY FRAME in case 3, so this hook runs in
// the menu fiber's own context and KEEPS logging even after the boot scheduler parks (which kills the
// DrainDispatch-driven menusm in the good run -- the blind spot we never instrumented). Logs the menu
// fiber's state + whether the 4-menu sprite (spMode/asset 170, via the menu sprite cache) and the option
// ready flags actually come up. Tests: in the black/held run do the options ever load/fade, and does the
// menu fiber reach the navigable state, after the boot scheduler stops?
extern "C" REX_FUNC(DOAX_MenuTransitionTimeline) {
  __imp__DOAX_MenuTransitionTimeline(ctx, base);
  const int32_t tl = static_cast<int32_t>(ctx.r3.u32);          // the timeline return (animation position)
  const uint32_t mcase = REX_LOAD_U8(0x833B8DEAu), loop = REX_LOAD_U8(0x833B8DEBu),
                 def = REX_LOAD_U8(0x833B8DEFu), present = REX_LOAD_U8(0x833BB763u);
  const uint32_t cached = REX_LOAD_U32(0x82C098F4u), target = REX_LOAD_U32(0x82BFC8C0u),  // menu sprite cache
                 busy = REX_LOAD_U32(0x83CAF8A4u);                                          // 1=load in flight
  const uint32_t o0 = REX_LOAD_U8(0x833B8510u), o1 = REX_LOAD_U8(0x833B8511u),  // option-sprite ready flags
                 o2 = REX_LOAD_U8(0x833B8512u), o3 = REX_LOAD_U8(0x833B8513u);
  static uint64_t s_prev = ~0ull;
  const uint64_t key = uint64_t(mcase) | (uint64_t(loop) << 8) | (uint64_t(def) << 16) |
                       (uint64_t(present) << 24) | (uint64_t(cached & 0xFF) << 32) |
                       (uint64_t(busy != 0) << 40) | (uint64_t(o0 & 3) << 41) | (uint64_t(o1 & 3) << 43) |
                       (uint64_t(o2 & 3) << 45) | (uint64_t(o3 & 3) << 47) |
                       (uint64_t(static_cast<uint32_t>(tl >= 0 ? tl : 0) / 64) << 49);
  if (key != s_prev) {
    s_prev = key;
    REXKRNL_WARN("DOAX menufiber case={} loop={} def={} present={} tl={} | modespr cached={} target={} "
                 "busy={} | opt_ready={}/{}/{}/{}", mcase, loop, def, present, tl, cached, target, busy, o0,
                 o1, o2, o3);
  }
}
