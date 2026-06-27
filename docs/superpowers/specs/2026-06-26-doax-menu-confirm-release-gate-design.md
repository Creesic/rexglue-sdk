# DOAX Menu-Confirm Release-Gate — Design

Date: 2026-06-26
Status: Approved (design); pending implementation
Target: `DOAX/src/doax_hooks.cpp` (recompiled DOAX2 / ReXGlue native title)

## Context

On the title "Press Start" screen, pressing A should dismiss the overlay and bring
up the navigable four-option main menu (Travel to New Zack Island / Xbox Live /
Precious Memories / Options); the player then navigates and confirms a choice.

In the recomp, pressing A on Press Start instead fades the overlay and cuts to a
permanent black screen, with input dying shortly after. A long investigation
(see `.debug-journal.md`, 2026-06-26) traced this end-to-end.

## Root cause

The four-option menu is navigable normally:
- The selection "highlight" (cursor on an item) auto-commits via the sprite pump
  (`sub_8266E618` → `sub_82670AE8`), gated only on async sprite-load readiness —
  not input. This is correct.
- The actual **confirm** is input-driven: `DOAX_MainMenuRegisterSlots`
  (`0x8258CEF0`) polls the input manager (`sub_8274B650`) and maps raw buttons
  through mask tables into per-context action words `dword_834A3510[11*ctx]`.
  `sub_8250BB60` (the mode-2 handler) reads bit `0x110` of those as
  "confirm/select" and, when set, runs the menu→scene hand-off.

The bug: **the single A press that dismisses Press Start is mapped into the menu's
confirm action**, so the just-opened menu immediately confirms the highlighted
default (item 15, Travel). That confirm triggers the scheduler mode-2 hand-off
(`dff=0` → `DrainWake` deactivates the scheduler, `byte_833B8DF8=0`), which hands
off to load Travel/New Zack Island; that load never completes (the menu fiber is
left in its `deb=-1` re-transition loop), so nothing takes over → black + stuck +
dead input (the deactivated scheduler stops ticking the input-manager work item).

On real hardware the dismiss-A is a single consumed input edge; by the time the
menu is input-active the button is merely *held* (no fresh edge), so the menu
waits. The recomp lets that press read as a fresh menu confirm.

## Design

Require a fresh confirm edge after the menu opens: the menu must not act on a
confirm until the confirm button (A) has been released at least once since the
menu became active. This is the behavior the old, now-removed
`g_press_start_waiting_for_release` machinery was reaching for, isolated to a
minimal gate.

Two touch-points, both in functions already hooked in `doax_hooks.cpp`:

1. **Arm** — `DOAX_MenuWorkFiberLoop` hook entry (menu fiber start = "menu just
   opened"): if the confirm button is currently held (`g_last_buttons &
   kXboxButtonA`), set `g_menu_confirm_wait_release = true`.

2. **Apply** — `DOAX_MainMenuRegisterSlots` hook, after `__imp__` maps the input:
   if `g_menu_confirm_wait_release`:
   - if A is no longer held → clear the flag (confirms allowed again);
   - else → clear bit `0x110` in `dword_834A3510[11*ctx]` for `ctx` in 0..3,
     suppressing the mapped confirm for this frame.

   This runs in the per-frame input update, before `sub_8250BB60` reads the action
   word in the same frame, so the suppression takes effect.

Net behavior:
- Open menu with A held → confirm suppressed → navigable four-option menu shows →
  release A, press A again → normal confirm proceeds.
- A not held at open → gate never arms → zero effect on normal navigation/confirm.

State: two file-scope globals, `bool g_menu_confirm_wait_release` and (if needed
to detect the open edge cleanly) `bool g_menu_was_active`; reset in
`InstallDoaxGuestPcFiber`.

### Addresses / facts used
- `DOAX_MainMenuRegisterSlots` = `0x8258CEF0` (hooked).
- `DOAX_MenuWorkFiberLoop` = `0x824C1548` (hooked).
- Confirm action word per context: `dword_834A3510 + 44*ctx` (11 dwords/ctx, 4
  contexts); confirm bits `0x10 | 0x100` (`sub_8250BB60` reads `*v2 & 0x100` and
  `*v2 & 0x10`).
- `g_last_buttons` already holds raw `wButtons` from the `sub_82782BF0`
  (`XamInputGetState`) input hook; `kXboxButtonA = 0x1000`.

## Staged rollout (one change at a time)

1. **Fix:** with the press-start state-forcing left disabled (already off under the
   current diagnostic standdown), add the release-gate. Build the `doax` target.
   Manual run to verify (below).
2. **Cleanup (only after step 1 is verified working):** remove the dead
   press-start state-forcing code and diagnostic probes/flags, leaving
   `doax_hooks.cpp` = recomp infrastructure + the release-gate.

### Keep vs remove
- **Keep:** guest-PC fiber install + `DOAX_FiberContextSwitch`/`DOAX_FiberYield`
  (+ GPR preservation), the scheduler/work-queue reimplementation glue
  (`DOAX_SchedulerDrainDispatch`, `DOAX_WorkQueueDispatchLoop`, `sub_8258CDF8`,
  `DOAX_SchedulerDrainWake`, `DOAX_WorkQueueSlotWake`, `DOAX_SchedulerFiberSwap`),
  the input hook `sub_82782BF0`, and the boot-movie midasm skips (license, ninja,
  promotion-replay).
- **Remove (step 2):** the `g_press_start_*` gate state machine and
  `NoteControllerButtons` press-start logic; `PrimePressStartDismissCompletion`,
  `RestorePostPressStartMenuState`, `MarkPressStartDismissUnwound`,
  `EnterPostPressStartMenuState`, `ArmPressStartGate`, `ArmPressStartDismissKick`;
  the press-start forcing inside `DOAX_MenuSceneTransition`,
  `DOAX_MenuTransitionMoviePoll`, `DOAX_MenuTransitionFadeAlpha`,
  `DOAX_MenuTransitionTimeline`, `DOAX_PostPromotionCleanup`; the press-start
  midasm hooks (`DOAX_SkipAutoPressStartSceneTransition`,
  `DOAX_SkipPostReadyAutoScenePrelude`, `DOAX_ForcePressStartLabel34Exit`,
  `DOAX_KeepPressStartCleanupActive`, `DOAX_ParkPressStartMenuFiberAfterCleanup`)
  and their `doax_manifest.toml` entries; the diagnostic flags/probes
  (`kDoaxDiagTimelineStanddown`, accelerator, sched-probe, `LogTimelineSlot`,
  `LogSchedulerAdvance`).

## Verification

Manual run of `DOAX/out/build/win-amd64-relwithdebinfo/doax.exe`:
1. Reach Press Start, press A.
2. Expect: the overlay dismisses and the **four-option menu renders and stays**
   (no cut to black, no stuck/deadlock, input stays alive).
3. D-pad navigates between the four options.
4. A fresh A press on a chosen option proceeds (e.g. Travel begins loading).

Regression check: a held A at menu open does not auto-confirm; releasing and
pressing A confirms. Confirm input is not permanently swallowed (gate clears on
release).

## Risks / notes
- Behavioral gate, not a fix of the deeper input-edge timing (if a lingering edge
  is the true mechanism). The release-gate is robust to either mechanism
  (lingering edge or same-frame double-read) because it waits for an explicit
  release.
- If `DOAX_MainMenuRegisterSlots` is not called on a given frame, the suppression
  is skipped that frame; acceptable because the gate persists until release and
  the confirm hand-off reads the same action word the mapper writes.
