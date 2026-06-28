# DOAX hooks — full archive (2026-06-27)

Snapshot of **every** DOAX hook taken right before the clean-slate strip. The live
`DOAX/src/doax_hooks.cpp` + `DOAX/doax_manifest.toml` were reduced to a bare baseline
(no behavior hooks, no fiber reentry) so we can re-add hooks **one group at a time**
and see exactly what each one is responsible for.

## Files here
| File | What |
|------|------|
| `doax_hooks.cpp` | 1411 lines — every `REX_FUNC` override + helpers (verbatim) |
| `doax_hooks.h`   | `InstallDoaxGuestPcFiber` decl |
| `doax_manifest.toml` | all named function hooks, all boundary regs, 8 midasm hooks |

## Restore workflow
1. Pick ONE group below. Restore its **manifest** entries (function `name=` and/or
   `[[entrypoint.midasm_hook]]` block) from `doax_manifest.toml` here, and its
   **code** (the `REX_FUNC` + helpers, grep this `doax_hooks.cpp`).
2. `cd DOAX && scripts/build-launch.ps1 -Codegen` (codegen regenerates the recomp
   from the manifest, then builds `doax.exe`). Codegen auto-adds any missing
   boundary stubs.
3. Test. Add the next group only after the current one is understood.

Notes:
- The bare manifest keeps the unnamed `0x... = {}` **boundary registrations** (NOT
  hooks): many are indirect-call targets the scanner can't resolve alone — removing
  them faults the recomp (`invalid or unregistered function 0x82A1B3F0`).
- Bare recomp (no fiber reentry) **crashes in early boot** (access violation in the
  nt=14 work loop) — so **Group 1 (fiber reentry) is required just to boot**.

---

## Group 1 — Guest-PC fiber reentry  *(LOAD-BEARING: boot crashes without it)*
OS-fiber-backed guest-PC reentry; original "first exception fix".
- Code: `InstallDoaxGuestPcFiber`, `RegisterDoaxGuestPcFiberConfig`,
  `REX_FUNC(DOAX_FiberContextSwitch)` → `rex::ppc::RunFiberSwap` + `FiberSwapImpl82785670`,
  `REX_FUNC(DOAX_FiberYield)` + GPR preserve (`SaveSchedulerFiberGprs`,
  `RestoreSchedulerFiberGprs`, `NeedsFiberCalleeSavePreserve`).
- Manifest: `0x82783210 = {name="DOAX_FiberYield"}`, `0x82785670 = {name="DOAX_FiberContextSwitch"}`.
- Also: `DOAX/src/doax_app.h` `OnPostSetup()` calls `InstallDoaxGuestPcFiber`.
- Runtime backend `src/ppc/guest_pc_fiber.cpp` is always compiled but only ACTIVE
  once `InstallGuestPcFiberInterpreter()` runs (from `InstallDoaxGuestPcFiber`).

## Group 2 — Scheduler  (0x824C0xxx)
- `REX_FUNC(DOAX_SchedulerDrainDispatch)` 0x824C0928 + `RestoreDrainCallerRegs`, `LogSchedulerAdvance`.
- `REX_FUNC(DOAX_SchedulerDrainWake)` 0x824C08B8 (reimplemented, embedded yield).
- `REX_FUNC(DOAX_SchedulerFiberSwap)` 0x824C06D8 + `PreserveSchedulerExceptBytes45`.
- Helpers: `SchedulerGuardedIndirectCall`, `ReadSchedulerSnapshot`, `LogSchedulerSnapshot`.

## Group 3 — Work queue
- `REX_FUNC(DOAX_WorkQueueDispatchLoop)` 0x824C05B8 (reimplemented).
- `REX_FUNC(DOAX_WorkQueueSlotWake)` 0x8258CE60 (reg restore).
- `REX_FUNC(sub_8258CDF8)` (reimplemented work-entry runner, embedded yield).

## Group 4 — Boot fiber + intro movies
- `DOAX_BootWorkFiberLoop` 0x8250A0B8, `DOAX_BootWorkFiberBody` 0x8250A568,
  `DOAX_BootMovieReplayTeardown` 0x8250A728, `DOAX_BootPresentStateUpdate` 0x8250BEB0,
  `DOAX_BootMovieGateCheck` 0x8250C468, `DOAX_PlayMovie` 0x826B0CD8, `DOAX_IsMovieFinished` 0x826B0E30.
- Midasm: `DOAX_SkipLicenseWarningIntro` 0x8250AAA8, `DOAX_SkipNinjaViHdMovie` 0x8250AB1C,
  `DOAX_SkipPromotionVideoReplay` 0x824C12B8.

## Group 5 — Press-start → four-menu transition  *(the bulk of the debug cruft)*
- Menu/transition `REX_FUNC`s: `DOAX_MenuWorkFiberLoop` 0x824C1548,
  `DOAX_MenuPreTransitionHook` 0x824C1460, `DOAX_MenuTransitionPlayMovie` 0x824C1208,
  `DOAX_MenuTransitionMoviePoll` 0x824C12D0, `DOAX_MenuTransitionOverlaySetup` 0x824C13D8,
  `DOAX_MenuTransitionFadeAlpha` 0x824DA790, `DOAX_MenuTransitionReadyCheck` 0x824DA868,
  `DOAX_MenuTransitionTimeline` 0x824195C8, `DOAX_MenuSceneTransition` 0x824C1958,
  `DOAX_PostPromotionCleanup` 0x824C1310, `DOAX_MenuItemConfirm` 0x82671308,
  `DOAX_MainMenuRegisterSlots` 0x8258CEF0, `DOAX_IslandSceneLoad` 0x82538048.
- Press-start state helpers: `ApplyMenuConfirmReleaseGate`, `ArmPressStartGate`,
  `ArmPressStartDismissKick`, `PrimePressStartDismissCompletion`, `MarkPressStartDismissUnwound`,
  `RestorePostPressStartMenuState`, `EnterPostPressStartMenuState`,
  `RestoreMenuWorkFiberLoopRegisters`, `ArmSchedulerDispatchAfterMenuInit`, `NoteControllerButtons`.
- Input fns: `sub_82670E10`, `sub_8266E618`, `sub_8258E000`, `sub_82782BF0`.
- Midasm: `DOAX_SkipAutoPressStartSceneTransition` 0x824C176C,
  `DOAX_SkipPostReadyAutoScenePrelude` 0x824C1714, `DOAX_ForcePressStartLabel34Exit` 0x824C1908,
  `DOAX_KeepPressStartCleanupActive` 0x824C1948, `DOAX_ParkPressStartMenuFiberAfterCleanup` 0x824C1950.
- Build flags: `kDoaxDiagTimelineStanddown`, `kDoaxDiagTimelineAccel`, `kDoaxDiagSchedProbe`.

## Group 6 — Session diagnostics (TEMP_DIAG, this session — logging only)
- doax_hooks: `REX_FUNC(DOAX_FM2EventDispatch)` 0x82768270 (FM2 tween-dispatch probe).
- Runtime (separate from doax_hooks, still live in `rexruntimerd.dll`):
  - SYNCDIAG worker-sync trace — `include/rex/gpu_sync_diag.h` + wiring in
    `src/kernel/xboxkrnl/xboxkrnl_threading.cpp`, `src/graphics/graphics_system.cpp`,
    `src/graphics/command_processor.cpp`.
  - `FIBERSW`/`FIBERRES` fiber-swap trace — `src/ppc/guest_pc_fiber.cpp`.
