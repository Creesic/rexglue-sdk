# DOAX guest fiber / coroutine swap (2026-06-22)

## Symptom

Access violation at host `0x100000000 + 0x34F8` in `sub_8258CC00` (`doax_recomp.14.cpp`
~57156):

```cpp
ctx.r11.u64 = REX_LOAD_U32(ctx.r30.u32 + 13560);  // expects r30 = 0x830A0000
```

`r30` was `0` after a guest fiber round-trip.

## Root cause

`sub_82785670` is a guest fiber context switch (save all GPR/VMX to TLS context,
restore from `r3`, branch to resumed LR). Recompiled code calls it as a normal C++
function. Guest `blr` becomes C++ `return`, which unwinds the **host** stack while
`r1` may point at a different guest stack — callee-saved registers (`r30`, etc.)
are wrong on resume.

Same class of bug as FH1 `sub_830ED910` (see `ReXGlue080FH1/FH1/patcher/fiber-reentry.md`).

## Fix (FH1 pattern)

1. `DOAX/src/doax_hooks.cpp` defines a **strong** `sub_82785670` that calls
   `rex::ppc::RunFiberSwap(ctx, base, __imp__sub_82785670, ...)`.
2. `InstallDoaxGuestPcFiber()` in `OnPostSetup` registers config and activates
   the guest-PC fiber subsystem.

**Do not** rely on `FunctionDispatcher::SetFunction` alone — direct codegen calls
never hit the dispatcher table.

## Finding fiber-swap candidates (semi-automated)

In generated `*_recomp.*.cpp`, search for the save/restore fingerprint:

```powershell
rg "std r30,280\(r5\)" DOAX/generated -l
rg "ld r30,280\(r3\)" DOAX/generated -l
```

A function containing both (plus `lwz r4,256(r13)` / TLS setup) is almost certainly
a fiber swap. DOAX has one: `0x82785670`.

FH1 marks these in manifest as `{ fiber_swap_host_boundary = true }` (documentation;
the link-time override is still required in title code).

## Follow-up: `sub_824C08B8` / `sub_8258CE60` (2026-06-22)

After the `DOAX_FiberContextSwitch` hook, crash at `doax_recomp.7.cpp` ~49140:

```cpp
REX_STORE_U8(ctx.r31.u32 + 0, ctx.r11.u8);  // r31 was 0 -> host 0x100000000
```

Call chain: `sub_82785660` → `sub_82783220` → … → `sub_824C08B8` → `sub_8258CE60`.

`sub_824C08B8` holds a **global scheduler flag** in `r31` (`lis -31940` / `addi -29192`
→ guest `0x833B8DF8`). After a virtual `bctrl` and nested fiber work inside
`sub_8258CE60` (`sub_827831B0`), guest `ld r31,-16(r1)` can restore `0` instead of
the flag pointer.

**Fix** (`DOAX/src/doax_hooks.cpp`):

1. Strong `sub_8258CE60` — save caller `r31` on the host C++ stack, restore after
   `__imp__sub_8258CE60`.
2. Strong `sub_824C08B8` — reload `kDoaxSchedulerFlagAddr` after `bctrl` and after
   `sub_8258CE60` before the final `stb`.

## Follow-up: `sub_824C05B8` work-queue loop (2026-06-22)

Crash at `doax_recomp.7.cpp` ~48682 after the prior fix:

```cpp
ctx.r11.u64 = REX_LOAD_U32(ctx.r30.u32 + 13560);  // r30 was 0 -> host 0x10000034F8
```

`sub_824C05B8` is the worker dispatch loop (`sub_82785660` → `sub_82783220` → …).
It materializes `r29`/`r30`/`r31` from `lis -31926` once, then each iteration calls
`sub_82783210` (fiber yield) and `sub_824C0928`. Fiber resume can leave `r30 == 0`.

**Fix:** strong `sub_824C05B8` that reloads `r28`–`r31` globals at the top of every
loop iteration (`0x834A0000` segment base, `0x834A3258` table base).

## Work-queue hang probe (2026-06-22)

`sub_824C05B8` logs `DOAX wq-probe` lines (`REXKRNL_WARN`, grep the log):

| `reason` | When |
|----------|------|
| `boot` | First 8 dispatcher iterations |
| `interval` | Every 10 000 iterations |
| `slow_call` | `sub_82783210` or `sub_824C0928` took ≥1 ms |
| `stuck_same_state` | Same `slot` / `queue_head` / `work_item` for 50 000 iterations |

Fields: `slot`, `queue_head`, `work_item`, `yield_us`, `drain_us`, `r29`/`r30`/`r31`, `lr`.
Capped at 150 lines per run (`kWorkQueueProbeLogCap`). `slow_call` only fires on `drain_us` ≥1 ms
(yield time is excluded — it is always ~30 ms).

## Work-queue drain probe (0x824C0928) (2026-06-22)

Strong `sub_824C0928` wraps the generated body, logs `DOAX drain-probe`, and restores
`r29`–`r31`/`lr` for the `sub_824C05B8` caller (`lr=0x824C0604`).

Scheduler global at `0x833B8DF8` (`kDoaxSchedulerFlagAddr`): **byte at +0 must be non-zero**
or the drain returns immediately (`beq` → `0x824C0B40`).

| `reason` | When |
|----------|------|
| `boot` | First 12 drain calls |
| `active` | `before.flag0 != 0` |
| `slow` | Drain took ≥1 ms |
| `reg_clobber` | Fiber path zeroed callee-saves before restore |
| `flag_transition` | Scheduler `flag0` changed |
| `noop_steady` | Every 500th call while `flag0==0` |

Fields: `before/after[f0-7]`, `w8`, `w12`, `caller_lr`, elapsed `us`.
Capped at 120 lines (`kDrainProbeLogCap`).

## Scheduler memory preservation + flag0 safety net (2026-06-22)

Call #85 in `doax-clean.log` showed `flag0` dropping `1→0` after `sub_824C08B8`
inside drain, leaving `sub_824C0928` in permanent early-exit while
`sub_824C05B8` keeps spinning.

**Fix experiments** (`DOAX/src/doax_hooks.cpp`):

1. `SchedulerGuardedIndirectCall` — save 16-byte scheduler header at
   `0x833B8DF8`, run virtual `bctrl`, restore `r30`/`r31` and header if fiber
   resume clobbered callee-saves.
2. Strong `sub_824C0770` — guarded `bctrl` at `0x824C07C8`, reload `r31` after
   nested calls.
3. Strong `sub_824C06D8` — `PreserveSchedulerExceptBytes45` (only bytes `+4`/`+5`
   may change).
4. `sub_824C08B8` — uses guarded `bctrl` instead of raw `REX_CALL_INDIRECT_FUNC`.
5. `sub_824C0928` wrapper — `ApplyFlag0SafetyNet`: if `before.flag0!=0` and
   `after.flag0==0`, restore `flag0` (grep `DOAX scheduler-safety`). On
   `reg_clobber`, restore full 16-byte header from pre-drain snapshot.

Remove the flag0 safety net once the wake path correctly re-arms the scheduler.

## Input / menu probes (2026-06-23)

Strong hooks log guest input and menu paths (grep `DOAX input-probe`, `DOAX menu-probe`,
`DOAX fiber-yield`):

| Hook | Address | Role |
|------|---------|------|
| `sub_82782BF0` | 0x82782BF0 | `XamInputGetState` wrapper |
| `sub_8274B650` | 0x8274B650 | Controller poll (init, 4 slots) |
| `sub_8258CEF0` | 0x8258CEF0 | Main-menu state setup (`li r7,4`) |
| `sub_82782B58` | 0x82782B58 | Indirect input dispatch |
| `sub_82783210` | 0x82783210 | Fiber yield from dispatcher/cdf8 |

**Note:** `XamInputGetState` is only reached via `sub_82782BF0` → `sub_8274B650` during
`sub_8258CEF0` init. If `input-probe` shows no `GetState` calls at the menu, ongoing input
uses a different path (memory tables / indirect dispatch).

## cdf8 scheduler preservation fix (2026-06-23)

`PreserveSchedulerExceptBytes45` after `sub_8258CDF8` / `sub_8258CE60` was undoing legitimate
scheduler updates made during the nested fiber round-trip (e.g. `flag6` countdown). Removed
post-fiber preservation on those two functions; keep callee-save `r31`/`r30` restore only.

## Scheduler fiber loop codegen bug (2026-06-23)

**Symptom:** `BootstrapOrFatal` / `Unresolved branch from 0x824C193C to 0x824C15D0` inside
`sub_824C15D0` (scheduler work-fiber main loop at `queue_head`).

**Cause:** Listing a mid-loop manifest entry such as `0x824C1714` (or `0x824C193C`)
splits the scheduler fiber loop out of `sub_824C15D0`. The backward `bne` at
`0x824C193C` then classifies as a tail-call to another function; if the graph
has no tail-call edge, codegen emits `BootstrapOrFatal` instead of looping back
to `0x824C15D0`.

**Durable fix:**

1. Do **not** list `0x824C1714` or `0x824C193C` under `[entrypoint.functions]`
   (split entries break backward-branch classification).
2. SDK codegen (`emit_conditional_branch`): when the branch target equals the
   function entry currently being emitted, emit `goto loc_<target>` instead of
   `BootstrapOrFatal`.
3. SDK fallback: unresolved conditional branch to a known function entry calls
   that function and returns (loop restart) instead of bootstrap.

Re-run `doax_codegen` after SDK/manifest changes. Do not hand-edit generated `doax_recomp.*.cpp`.

## Main-menu wrong-state (2026-06-23)

**Observed:** Island background renders, D-pad moves the **camera** (island view), no four-option
menu UI, then `promotion_video.sfd` / boot-movie path fires again within ~1s of scene load.

**Log evidence (`doax-clean.log`):**

| Signal | Finding |
|--------|---------|
| `menu-probe` / `CEF0` | Runs 4× per scene load from `sub_8258E000` (`lr=0x8258E03C`) — menu slot registration happens |
| `input-probe GetState` | Only during CEF0 bursts (~42 total); no ongoing menu input polling |
| `drain-probe` | Scheduler stuck `flag0=1 flag2=0 flag4/5=5/1`; `w8`/`w12` counters increment forever |
| Boot movie replay | ~1s after island load, fiber resumes at `0x8250A104` → license/ninja skip → `promotion_video.sfd` |

**Interpretation:** `sub_8258E000` brings up the **island scene** (camera + 3D) and calls
`sub_8258CEF0` to register four menu work-queue slots, but `sub_824C0928` never enters the
`loc_824C0A5C` dispatch path because **`flag2` stays 0**. The game is in a hybrid state: island
exploration camera works, menu UI work never runs, and a stale boot-movie work item still fires.

**Experiment (2026-06-23):** `ArmSchedulerDispatchAfterMenuInit` in the `sub_8258CEF0` hook sets
`flag2=1` when `flag0!=0 && flag2==0` after menu init. Grep `DOAX menu-kick`. Also added
`sched-probe` on `sub_8258E000` and `movie-probe` on `DOAX_PlayMovie` / ninja skip.

**Regression (2026-06-23):** Unconditional menu-kick on every CEF0 call (whenever `flag2==0`)
re-arms menu dispatch during scene-bring-up modes `2/2`, `3/3`, `2/0`, trapping
`sub_8258E000` in a `5/1→2/2→3/3→2/0→5/1` loop after confirming a menu item (black screen,
`PlayMovie` replay, `824C15F4` fiber spin). Log showed 800+ `menu-kick` lines per session.

**Fix:** Re-arm `flag2` after CEF0 for all scene-bring-up modes when `flag2==0` (boot needs
modes `2/2`/`3/3`/`2/0`, not just menu-idle `5/1`). On any work-fiber menu confirm
(`sub_82671308`, LR `0x824C16A4`/`0x824C17FC`), set `g_suppress_menu_kick`. Travel uses
table item id **15**, not highlight index 0. Grep `DOAX menu-select`.

## Travel black screen after menu confirm (2026-06-23)

**Observed:** Confirming Travel (`item_id=15`) starts island load (`ups.dat`, GPU
`draws≈1300`), then within ~2s collapses to `draws=37` black screen. A on the black
screen plays UI confirm sound but B cannot return to menu.

**Log evidence (`doax-clean.log`, session 14:04:08):**

| Signal | Finding |
|--------|---------|
| `menu-select` | `cursor=0 item_id=15` at 14:04:08.947 |
| `gpf` fiber resume | `lr=0x8250A104` (boot work-fiber loop in `sub_8250A0B8`) interleaved with confirm |
| GPU | `draws=1310` at 14:04:10, back to `draws=37` at 14:04:11 |
| `ups.dat` / `rds.dat` | Open immediately after confirm — Travel transition does start |

**Interpretation:** Menu work-fiber (`sub_824C1548`) enters case 1→2→3 scene transition
(`sub_824C1958`, `sub_82538048`), but the **boot intro work-fiber** (`sub_8250A0B8` /
`sub_8250A568`, resume `lr=0x8250A104`) is still queued and keeps running during/after
confirm. That replays boot movie / present state and tears down the island scene back to
the idle `draws=37` splash.

**Fix experiment (`DOAX/src/doax_hooks.cpp`):**

1. On **scene transition** (`sub_824C1958`, caller `lr=0x824C176C` or `0x824C1918`) and
   **post-confirm cleanup** (`sub_82671308`, caller `lr=0x824C17FC`, arg 0), set
   `g_suppress_boot_work_fiber`. Do **not** arm from case-0 `sub_82671308` @
   `0x824C16A4` (menu bring-up).
2. Block boot replay chain when suppressed: `sub_8250A0B8`, `sub_8250A568`,
   `sub_8250BEB0`, **`sub_8250A728`** (IDA: calls `sub_824C08B8`, resets scene —
   matches `draws~1300 → 37` cliff).
3. Skip `ApplyFlag0SafetyNet` while `g_suppress_boot_work_fiber` (grep
   `DOAX travel-transition`).

**IDA teardown chain (2026-06-23):** `sub_8250A568` → `sub_8250BEB0` (when
`byte_833BB763 != 5`) → `sub_8250A728` → `sub_824C08B8` / `sub_8258CE60` work-queue
wake. That replays boot movie state and tears down the Travel island scene.

**Regression (2026-06-23):** Arming `g_suppress_boot_work_fiber` from `sub_82671308`
at `lr=0x824C16A4` fired during case-0 menu bring-up (before user confirm), suppressed
`sub_8250A568` too early, and caused **black after press start** (never reached four-option
menu). Moved suppress arm to `sub_824C1958` @ `lr=0x824C176C` only.

**Log follow-up (2026-06-23, `doax-clean.log` 14:39/14:45):** `DOAX menu-select`
`item_id=15` and `ups.dat` open, but **`DOAX travel-transition` never logged** — boot-fiber
suppress never armed; island `draws≈1316` still collapses to `draws=37` ~2s later.
`DOAX_MenuSceneTransition` / `lr=0x824C17FC` hooks did not fire (or LR gate missed).

**Fix (2026-06-23):** Arm boot-fiber suppress on:

1. Any `DOAX_MenuSceneTransition` entry (only two xrefs in menu fiber).
2. `DOAX_MenuItemConfirm` Travel row at menu-idle scheduler `5/1`: `item_id=15`, `arg2=30`,
   `lr=0x824C16A4` (case 0 confirm frame — not CEF0 bring-up modes `2/2`/`3/3`/`2/0`).
3. `DOAX_MenuItemConfirm` with `arg2=0` (post-confirm cleanup when reached).
4. Fallback: `DOAX_IslandSceneLoad` when `g_suppress_menu_kick` is already set.

Grep `DOAX scene-transition`, `DOAX island-scene-load`, `DOAX boot-fiber-suppress`,
`DOAX travel-case3-probe`, `DOAX travel-guard`.

**Log follow-up (2026-06-23, session 15:11):** Boot-fiber suppress arms correctly;
case-3 fade runs full ~900-tick countdown (`overlay=1`, `present=1`, `def=0` throughout).
Island GPU peaks `draws≈1287` then decays during fade. At timeline completion
(`lr=0x824C191C` LABEL_34) `draws` cliff to `37`. Root cause: `byte_833B8DEF` never
reaches 1 (timeline can skip exactly 30), so LABEL_34 takes the
`DOAX_MenuSceneTransition(item,1)` else path instead of the deb/case-0 completion path.
Forcing `dword_833B84C8==1` via travel-guard also blocks
`DOAX_BootPresentStateUpdate`'s `dword_833B84C8==3` island-gameplay handoff.

**Fix (2026-06-23):** grep `DOAX travel-complete`. When timeline is in `(0,30]`, set
`byte_833B8DEF=1`. When timeline hits 0, set `dword_833B84C8=3`, clear travel guard,
allow `DOAX_BootPresentStateUpdate` to run. Remove LABEL_34 scene-transition skip.
Stop forcing `present=1` once timeline `<=30`.

**Log follow-up (2026-06-23, session 15:21):** Prior fix never logged
`DOAX travel-complete`. Cliff at `df0≈48` / `timeline≈853` (~2s after confirm), not at
fade end. `DOAX_BootWorkFiberBody` still ran on fiber resume (`lr=0x8250A104`) — loop-level
suppress at `0x8250A0B8` never re-enters. Body vtable dispatch at `0x8250A6BC` tears down
island without hitting `DOAX_BootMovieReplayTeardown` hook.

**Fix (2026-06-23, session 15:21+):** Skip entire `DOAX_BootWorkFiberBody` while
`g_suppress_boot_work_fiber && !g_travel_fade_complete`. Fast-complete travel at
`df0>=40` (`DOAX travel-complete: fade-done site=timeline-df0` or `menu-fiber-yield`).
Arm `byte_833B8DEF=1` on first case-3 timeline tick (any `timeline>=0`). Grep
`skip DOAX_BootWorkFiberBody`, `travel-complete`.

## Scheduler fiber GPR clobber (2026-06-23)

**Symptom:** AV reading `0x100003F13` in `sub_824C1548` at `lbz r11,16147(r28)` once menu
dispatch runs (`flag2` kick). `r28` was `0` after fiber resume.

**Cause:** `sub_824C1548` sets `r14`–`r31` in its prologue, then yields via `sub_82783210`
with `lr=0x824C15F4`. Guest-PC fiber swap does not preserve those callee-saves across the
round-trip.

**Fix:** `sub_82783210` hook saves/restores `r14`–`r31` around fiber swap for **work-fiber
loop yields** (e.g. `lr=0x824C15F4`, `0x825A25E0`, `0x824C0C3C`). **Do not** restore on
hooked infrastructure yields (`lr=0x824C0600` dispatcher, `0x8258CE4C` cdf8 wake) — those
paths reload globals after return; universal restore wedged the scheduler again (black
screen, no menu draws).

**Press Start regression (2026-06-24):** Menu-kick on scheduler bring-up modes `2/2`/`3/3`/`2/0`
also matches **boot warning** (`sub_824C1350(2)`). Arming `flag2` before
`boot_present==5` or during Press Start (`boot_present==5`, `overlay==0`) steals the
boot work-fiber and wedges A/Start. Gate with `IsPreIslandMenuKickPhase` in
`DOAX/src/doax_hooks.cpp` — only kick when `boot_present>=5` and not
`(boot_present==5 && overlay==0)`, or after `menu-fiber` has run.

**Press Start regression (2026-06-24):** Inverting the policy to “restore on every yield
except dispatcher/cdf8” broke Press Start — boot work-fiber yields (resume `lr=0x8250A104`)
must **not** get GPR restore. Whitelist-only preserve fixes input on Press Start while
keeping menu work-fiber `r28` alive.

**Four-menu black (2026-06-24, `doax_037`/`doax_038`):** `menu_kick=0` entire session.
`IsPreIslandMenuKickPhase` used `boot_present < 5`, but Press Start dismiss sets
`boot_present==1` (not 5). Fix: pre-island is only `boot_present==0` or
`(boot_present==5 && overlay==0)`. Spurious Travel at `0x824C16A4` on first menu-fiber
bring-up (`sched 2/2`, `overlay==0`, `item_id=15`) set `g_suppress_menu_kick` — fix:
`IsUserTravelMenuConfirm` requires idle hub `5/1`, `overlay==1`, `item==15`, `arg2==30`.
Build tag `doax-hooks-2026-06-24-menu-kick-fix`. Grep `menu-kick`, `travel-confirm`.

## Promotion replay block crash (2026-06-24, `doax_005`)

**Symptom:** Odd boot behavior then crash ~9s in. Log build tag
`doax-hooks-2026-06-24-promotion-replay-block`.

**Cause:** Skipping `PlayMovie(1)`, `MenuTransitionPlayMovie`, and faking handler-done bytes
(`byte_833B8DFA`, `dword_82B85498`) left the scheduler promotion handler chain inconsistent.
`IsMovieFinished` marked promotion finished before poll. Menu-kick on `2/2` still fired during
active promotion (`menu-kick site=drain-rearm` before `promotion-finished`).

**Press Start state (log-proven):** After promotion, island preview uses `boot_present==1`
`overlay==0` on scheduler `5/1` — not `boot_present==5`. Forcing `boot_present=5` blackens
Press Start.

**Fix (build `doax-hooks-2026-06-24-promotion-menu-kick-guard`):**
- Revert all PlayMovie / handler-skip / fake-byte hooks.
- Block menu-kick while `g_boot_promotion_started && !g_boot_promotion_finished`.
- Extend `IsPreIslandMenuKickPhase`: also `boot_present==1 && overlay==0 && sched 5/1`.
- Mark promotion finished only from `DOAX_MenuTransitionMoviePoll`.
- Narrow teardown guard: `IsPressStartIslandIdle` blocks `BootMovieReplayTeardown` only on
  sched `5/1` with `overlay==0` after promotion finished.

**Expected log sequence:** one `movie-play idx=1 attempt=1`, then `promotion-finished
site=promotion-poll`, no `menu-kick` on `2/2` before that, Press Start draws >> 37.

## Four-menu black regression (2026-06-24, `doax_006`)

**Symptom:** Promotion/Press Start OK again; four-option menu black. `menu_kick=0` entire
session.

**Cause (dual):**
1. `g_boot_promotion_finished` never set (`promotion-finished` absent from log) so
   `g_boot_promotion_started && !finished` blocked all menu-kick forever.
2. `DOAX_MenuSceneTransition` at `lr=0x824C1770` (LABEL_12 Press Start → four-menu) called
   `ArmBootFiberSuppress`, setting `travel_confirm=1` and `g_suppress_boot_work_fiber` on
   bring-up.

**Fix (build `doax-hooks-2026-06-24-four-menu-fix`):**
- Mark promotion finished from `PostPromotionCleanup`, `IsMovieFinished` (passive), and poll.
- Skip travel suppress on scene-transition when `lr==0x824C1770` (LABEL_12 bring-up).
- Only `EnforceTravelOverlayGuard` when `g_travel_overlay_guard` is already armed.

**Expected:** `promotion-finished` in log, `menu-kick: armed` on `cef0` mode `2/2`, scene-transition
`travel_arm=0`.

## Instant promotion replay (2026-06-24, `doax_007`)

**Symptom:** Promotion replays immediately after first cycle (`movie-play attempt=2`).

**Cause:** `post-promotion-cleanup` marks promotion finished, then `drain-rearm` menu-kicks
through `2/2→3/3→2/0` while `overlay==0` (Press Start still showing). Working path
(`doax_003`) only menu-kicks from `cef0` after player dismisses.

**Fix (build `doax-hooks-2026-06-24-drain-rearm-guard`):** Block `drain-rearm` menu-kick while
`g_boot_promotion_started && overlay==0`. Keep `cef0` / `menu-fiber-return` kicks for
Press Start → four-menu. Remove passive `IsMovieFinished` promotion-finished mark.

## Promotion replay midasm (2026-06-24)

**Symptom:** Scheduler/menu-kick hooks still fight promotion — instant replay after cleanup.

**Root fix:** Midasm at `0x824C12B8` (`bl DOAX_PlayMovie` in `DOAX_MenuTransitionPlayMovie`
`0x824C1208`). When `r3==1` and `g_boot_promotion_play_attempts>0`, jump to `0x824C12BC`
(skip the call). First play runs normally; handler poll sees prior `IsMovieFinished` state.
Same pattern as ninja `0x8250AB1C`→`0x8250ABA4`. Build tag
`doax-hooks-2026-06-24-promotion-replay-midasm`. Keep drain-rearm + scene-transition guards
as belt-and-suspenders for menu-kick.

## Press Start 3D island missing (2026-06-24, `doax_009`)

**Symptom:** Promotion replay fixed (midasm), but Press Start shows UI text with no 3D island
backdrop. GPU `draws` stuck ~35–39; `menu_kick` climbs rapidly (cef0 `2/2→3/3→2/0` loop).

**Cause:** `drain-rearm` menu-kick stayed blocked after `promotion-finished` while `cef0`
still armed bring-up kicks — asymmetric cycle never completed back to idle `5/1`. Also
`IsPreIslandMenuKickPhase` blocked idle `5/1` kicks on `boot_present==1 overlay==0`, so
`sub_8258E000` island dispatch never ran on the Press Start preview drain path.

**Fix (build `doax-hooks-2026-06-24-press-start-island-guard`):**
- Block `drain-rearm` only during **active** promotion (`!g_boot_promotion_finished`); allow
  after cleanup (midasm blocks PlayMovie replay).
- Remove `boot_present==1 && overlay==0 && sched 5/1` from `IsPreIslandMenuKickPhase`.
- Add `IsPressStartIslandPreview`: allow idle `5/1` menu-kick when promotion finished,
  `overlay==0`, `boot_present==1`.

**Expected:** one brief bring-up cycle after `promotion-finished`, then stable `sched=5/1` with
GPU draws well above UI-only (~35), island visible behind Press Start text.

## Press Start promotion flicker (2026-06-24)

**Symptom:** Press Start flickers; no stable 3D island. Feels like instant promotion replay
then midasm skip.

**Cause:** After `press-start-island-guard` unblocked `drain-rearm` post-promotion, menu-kick
on `2/2→3/3→2/0` and idle `5/1` re-entered `DOAX_MenuTransitionPlayMovie`. Midasm at
`0x824C12B8` skipped only the `PlayMovie` bl — handler body still toggled present/gamma
(flicker). `doax_037` showed island can render at high `draws` with `menu_kick=0` on Press Start.

**Fix (build `doax-hooks-2026-06-24-press-start-promotion-loop-fix`):**
- Block **all** menu-kick (cef0, drain-rearm, flag0-safety-net) during Press Start preview
  (`promotion finished`, `overlay==0`, `boot_present==1`) until `menu-fiber` enters (player
  pressed Start).
- Hook `DOAX_MenuTransitionPlayMovie`: early-return on promotion replay during preview.
- Keep midasm `SkipPromotionReplay` as belt-and-suspenders.

**Expected:** stable Press Start, no `movie-play idx=1 attempt=2`, `menu-kick-skip
reason=press_start_preview` during idle, first `menu-kick` after `menu-fiber-enter`.

**Fix v2 (build `doax-hooks-2026-06-24-press-start-hold-v2`):** v1 guards required
`g_boot_promotion_finished` and unblocked when spurious `menu-fiber` entered during replay
(`doax_037`), so replay/flicker continued. v2 uses guest-state hold instead:

- `IsPromotionPressStartHold`: `plays>0`, `boot_present==1`, `overlay==0`, not user-dismissed.
- `g_press_start_user_dismissed` set on `DOAX_MenuSceneTransition` `lr=0x824C1770` only.
- Skip entire `DOAX_PlayMovie(1)` and `DOAX_MenuTransitionPlayMovie` during hold (not just
  midasm `bl`).
- `ShouldUndoBootPresentTeardown` + `BootMovieReplayTeardown` block during hold even when sched
  is `2/2`/`3/3` (not only idle `5/1`).

Grep: `press-start-hold-v2`, `press-start-dismiss`, `promotion-replay-block: skip PlayMovie`.

**Fix v3 (build `doax-hooks-2026-06-24-press-start-drain-skip`):** Per-frame flicker was
`DOAX_SchedulerDrainDispatch` re-entering the promotion handler state machine (`5/1→2/2→3/3→2/0`)
every frame. Skipping only `PlayMovie` / `MenuTransitionPlayMovie` left overlay fade, gamma, and
present-state toggles running — looks like a hook firing every frame.

- Skip guest **drain dispatch** during hold when sched is idle `5/1` or bring-up `2/2`/`3/3`/`2/0`;
  pin sched header to latched `5/1` snapshot; clear `byte_833B8DFE` fade counter.
- Skip `BootPresentStateUpdate` during hold (no run+undo fight each frame).
- Latch handler-done bytes (`byte_833B8DFA`, `dword_82B85498`) at `promotion-finished`.
- Skip `MenuTransitionOverlaySetup` / fade / timeline / ready-check during hold.
- Hold still defers until `promotion-finished` (or `plays>1`) so first promotion poll works.

Grep: `press-start-drain-skip`, `drain-skip: press-start-hold`, `press-start-latch`.

**Fix v4 (build `doax-hooks-2026-06-24-press-start-stable`):** v3 skipped drain as soon as
`promotion-finished` — black screen after A-skip because post-promotion bring-up never completed.
v4 is two-phase:

1. **Bring-up phase** (`!g_press_start_seen_stable`): drain, overlay, boot-present all run
   normally until idle `5/1` for 4 frames with `boot_present==1 overlay==0`.
2. **Stable phase** (`g_press_start_seen_stable`): block menu-kick on idle `5/1`, skip
   `PlayMovie`/`MenuTransitionPlayMovie`/`OverlaySetup` replay, snap sched back from bring-up
   `2/2`/`3/3` to pinned `5/1` after drain (not skip drain entirely).

Grep: `press-start-stable`, `drain-snapback`.

**Fix v5 (build `doax-hooks-2026-06-24-press-start-drain-freeze`):** v4 snapback ran
*after* guest drain each frame — one frame of promotion handler (overlay/gamma) still ran
causing per-frame flicker. v5:

- Wait for `PostPromotionCleanup` before arming stable/freeze (A-skip bring-up can finish).
- After 2 idle `5/1` frames: latch handler-done bytes + pin sched.
- **Skip guest drain entirely** when stable on idle `5/1` or bring-up modes (boot fiber keeps
  rendering). No post-hoc snapback.
- Skip `BootPresentStateUpdate` + `MenuTransitionMoviePoll` during stable replay block.

Grep: `press-start-drain-freeze`, `drain-freeze`, `press-start-stable`.

**Fix v6 (build `doax-hooks-2026-06-24-press-start-hold-visible`):** Press Start appeared then
instantly faded after A-skip. Promotion cleanup had started `MenuTransitionOverlaySetup` fade
(`byte_833B8DFE` / `df4` timeline); handler-done latch (`833B8DFA`, `82B85498`) advanced the
transition away. v6:

- `ShouldHoldPressStartVisible`: after `PostPromotionCleanup`, `boot_present==1`, `overlay==0`.
- `CancelPressStartOverlayFade`: clear `DFE`, `df0`, set `df4=0xFFFFFFFF`.
- Skip overlay/fade/timeline/pre-transition hooks during hold; cancel fade on boot/menu fiber.
- Re-enable `BootPresentStateUpdate` with teardown undo (was fully skipped).
- Remove handler-done byte latch from stable freeze.

Grep: `press-start-hold-visible`, `cancelled overlay fade`.

**Fix v7 (build `doax-hooks-2026-06-24-press-start-replay-latch`):** v6 still faded on
A-skip because `ShouldHoldPressStartVisible` / `ShouldBlockPromotionVideoReplay` required
`g_press_start_post_cleanup`, which is only set in handler 10. Boot fiber can call
`PlayMovie(1)` again between movie end and cleanup.

- Hold when `boot_present==1`, `overlay==0`, `plays>0`, not dismissed (no cleanup latch).
- Block replay on the same guest state immediately, not after cleanup.
- `LatchPressStartPreviewIfNeeded` from drain, movie poll, boot fiber; pre-enforce in
  `PostPromotionCleanup` before guest body runs.

Grep: `press-start-replay-latch`, `press-start-latch`, `promotion-replay-block`.

**Regression v7 (`press-start-replay-latch`):** Holding on `boot_present==1 overlay==0
plays>0` matched **active promotion** too — movie poll returned early, `IsMovieFinished`
spoofed done, promotion went black/unresponsive.

**Fix v8 (build `doax-hooks-2026-06-24-press-start-hold-v7`):** Revert early hold. Replay
block uses `plays>1`, `ShouldHoldPressStartVisible` (needs cleanup), or
`poll_saw_done && preview && !finished` for the A-skip gap only.

Grep: `press-start-hold-v7`, `promotion-poll-done`, `post-promotion-cleanup`.

**Fix v9 (build `doax-hooks-2026-06-24-press-start-phase-hold`):** Delayed promotion return
after Press Start — guest `boot_present`/`overlay` flicker during sched `5/1→2/2` re-entry
dropped v8 guards mid-hold.

- `IsPressStartHoldPhase`: latched at cleanup until dismiss (not per-frame guest bytes).
- `SealPromotionSchedulerAfterCleanup`: `833B8DFA=1`, `82B85498=1`, clear fade after cleanup.
- `drain-promo-skip`: skip guest drain on promo handler sched modes during hold bring-up;
  keep idle `5/1` drain for island render.

Grep: `press-start-phase-hold`, `drain-promo-skip`, `post-promotion-cleanup`.

**Regression v9:** `drain-promo-skip` + `SealPromotionScheduler` blocked post-cleanup
bring-up drain on sched `2/2`/`3/3` — never reached Press Start.

**Fix v10 (build `press-start-phase-hold-v10`):** Remove drain suppress and scheduler seal.
Keep `IsPressStartHoldPhase` for replay/menu-kick block only; overlay/fade hook skips still
need live preview guest bytes. Cancel fade each drain frame during hold phase.

Grep: `press-start-phase-hold-v10`, `post-promotion-cleanup`.

**Fix v11 (build `press-start-handler-advance`):** Press Start flashed then promotion looped
because blocking `MenuTransitionPlayMovie`/`PlayMovie` without advancing scheduler handler
6 (`833B8DFA`, `82B85498`) made drain re-enter the promotion handler every frame.

- `CompleteBlockedPromotionHandler` on replay block paths + stable latch.
- Skip overlay/fade hooks for full `IsPressStartHoldPhase`.
- Immediate stable latch when cleanup lands on idle `5/1` preview.

Grep: `press-start-handler-advance`, `promotion-replay-block`, `press-start-stable`.

**Regression v11:** Handler advance at cleanup/stable blocked bring-up to Press Start.

**Fix v12 (build `press-start-preview-gated`):** Only block/advance promotion replay after
`g_press_start_preview_seen` (Press Start actually visible once). Bring-up after cleanup
runs normally; replay loop broken only after preview latch.

Grep: `press-start-preview-gated`, `press-start-preview-seen`, `promotion-replay-block`.

## Why Press Start is hard (2026-06-24)

One scheduler handler (`5/1 → 2/2 → 3/3 → 2/0 → 5/1`) drives **three** unrelated jobs:

1. Island 3D bring-up (`cef0` menu-kicks)
2. Promotion handler 6 fade/poll (`MenuTransitionMoviePoll`, `PlayMovie(1)`)
3. Press Start dismiss → four-menu (`MenuSceneTransition` LABEL_12 at `lr=0x824C1770`)

Fixes are coupled: blocking menu-kicks stops replay but also blocks island load; running guest
poll enables A/Start path but auto-advances promotion fade; accepting LABEL_12 after bring-up
without input dismisses on timer (~1.4s at sched `2/2` in `doax_033`).

**Fix v13 (build `press-start-stable-input`):** Three explicit phases after
`press-start-bringup-done`:

1. **Bring-up** (`!bringup_done`): allow one `cef0` kick cycle; block spurious LABEL_12 at
   non-idle sched.
2. **Stable hold** (`bringup_done && !dismissed`): block all menu-kicks; **skip guest**
   `MenuTransitionMoviePoll` until A/Start (`sub_82782BF0` sets `g_press_start_button_seen`);
   enforce display each frame.
3. **Dismiss**: accept LABEL_12 only when `g_press_start_button_seen`; block `PlayMovie(1)`
   replay until `boot_present==5 overlay==1` hub idle (not just during hold phase).

Grep: `press-start-stable-input`, `press-start-bringup-done`, `block LABEL_12`, `user=1`.

**Fix v14 (build `press-start-midasm-input`):** stable-input regressed to no A/Start because
full hooks short-circuited the guest dismiss path:

1. `MenuPreTransitionHook` returned early during hold (never ran guest pre-transition).
2. `MenuTransitionMoviePoll` was frozen until a synthetic XInput latch.
3. `MenuSceneTransition` returned early blocking all LABEL_12 until button flag.

**Midasm + passthrough approach:**

- Promotion replay: midasm `0x824C12B8` only (`MenuTransitionPlayMovie` hook passthrough).
- Spurious LABEL_12 during island bring-up: midasm `0x824C176C` → `0x824C1770` skips
  `MenuSceneTransition` bl when hold + preview + `!bringup_done` + sched != `5/1`.
- `MenuPreTransitionHook` / `FadeAlpha` / `Timeline`: call guest, cancel overlay fade after.
- `MoviePoll`: always call guest; post-enforce display during hold.

Grep: `press-start-midasm-input`, `midasm-skip-LABEL_12`.

**Fix v15 (build `press-start-handler-hold`, from `doax_036` flash probe):** After
`press-start-bringup-done`, handler 6 still called `MenuTransitionPlayMovie`; midasm skipped
the `bl` but guest advanced sched `5/1`→`2/2` and `PostPromotionCleanup` spun every ~120ms.

1. Skip full `MenuTransitionPlayMovie` when replay blocked; `OnPromotionReplayBlocked` +
   `EnforcePressStartDisplayState`.
2. Skip guest `PostPromotionCleanup` on re-entry after bring-up (`skipped-reentry`).
3. Pin sched at bring-up; `drain-snapback` when `bringup_done` + hold + sched != `5/1` (not
   only after `press-start-stable`).

Grep: `press-start-handler-hold`, `play-movie-hook blocked`, `skipped-reentry`, `drain-snapback`.

**Regression (`press-start-handler-hold`):** Black after promotion — full `MenuTransitionPlayMovie`
skip fired during `2/2→5/1` bring-up (not only after island latched), and/or bring-up latched
on first post-cleanup `5/1` before the kick cycle ran.

**Fix v16 (build `press-start-bringup-cycle`):**
- `g_press_start_saw_bringup_cycle` set when drain sees non-idle sched during hold.
- `bringup_done` only after saw cycle + idle `5/1`.
- Full `MenuTransitionPlayMovie` skip only when `bringup_done` (midasm handles earlier).
- Snapback / cleanup re-entry skip unchanged (still after bringup).

Grep: `press-start-bringup-cycle`, `press-start-bringup-done`, `saw_bringup`.

**Fix v17 (build `press-start-overlay-hold`, from `doax_044`):** Scheduler logic
correct (`bringup-done`, `press-start-stable`, `drain-freeze`, `plays=1`) but user
still saw promotion fade. Log showed `deb=1` on bring-up kicks at `3/3`/`2/0` while
overlay guest hooks only skipped **after** `bringup_done`; island scene load lagged
~1.1s after stable (`island-scene-load` at 22.217 vs `bringup-done` at 21.069).

1. Skip `MenuTransitionOverlaySetup` / `Timeline` / `FadeAlpha` for entire hold
   (`ShouldHoldPressStartVisible`), not only post-bringup — bring-up uses cef0, not overlay.
2. `SuppressPressStartPromotionOverlay`: clear `byte_833B8DEB`, cancel fade bytes every hold frame.
3. `MaybeKickPressStartIslandSceneLoad`: call `DOAX_IslandSceneLoad` immediately after
   `bringup_done` (boot fiber + drain) instead of waiting for natural late load.

Grep: `press-start-overlay-hold`, `press-start-island-load`, `skip MenuTransitionOverlaySetup`.

**Fix v18 (build `press-start-guest-drain`, from `doax_045`):** Overlay-hold had
perfect scheduler bytes (`dfa=0`, `deb=0`, `drain-freeze` x186) but user still saw
promotion fade. Stable `doax_031` had **no drain-freeze**, `deb=1` throughout, and
handler-6 drain kept running at idle `5/1`.

Root cause: `EnforcePressStartIdleHoldState` ran **every drain frame** when hold was
active (not only on sched drift), clearing `deb` and skipping handler-6 via
`drain-freeze` — fighting guest Press Start presentation.

1. Disable `drain-freeze` — handler-6 must dispatch at idle `5/1`.
2. Restore `deb` — only clamp sched/`dfa`/`dfe` on actual drift (`drain-restore`).
3. Un-skip `MenuTransitionMoviePoll` after bringup (input + guest state).
4. Drop per-frame `EnforcePressStartDisplayState` hammering; latch gate once at cleanup.
5. Keep overlay-guest skip during hold (blocks new promotion overlay fade setup).

Grep: `press-start-guest-drain`, `drain-restore`, no `drain-freeze`.

**Fix v19 (build `press-start-allow-2-2`, from doax_031 comparison):** Stable Press Start
requires guest handler-6 at sched `2/2` with `deb=1` after bring-up — not forced idle `5/1`.
Remove `drain-restore` / `drain-clamp` snap-back, overlay-guest skip, and sched pin/seal on
bring-up latch. Keep menu-kick block + midasm PlayMovie replay skip.

Grep: `press-start-allow-2-2`, `allow sched 2/2`, no `drain-restore`.

**Fix v20 (build `four-menu-bringup-kick`):** Press Start stable again (`allow-2-2`) but
four-option menu black after dismiss — same root as `doax_006`: `menu_kick=0` because
`MenuKickBlockReason` / `ShouldArmMenuDispatch` required `overlay==1` at idle `5/1`, and
`IsPreIslandMenuKickPhase` blocked `boot_present==5 overlay==0` during bring-up.

- `IsPressStartFourMenuBringUp`: `post_cleanup && user_dismissed && overlay!=1`.
- Allow cef0/drain menu-kick on idle `5/1` and bring-up sched during that window.

Grep: `four-menu-bringup-kick`, `press-start-dismiss`, `menu-kick` after dismiss.

**Fiber log (build `fiber-log`, 2026-06-24):** Four-menu black treated as fiber/register
issue — no further menu-kick hook experiments. Reverted `four-menu-bringup-kick` behavior.
`fiber-probe` window arms at `post-promotion-cleanup` until Travel confirm.

Grep patterns in `DOAX/out/build/win-amd64-relwithdebinfo/logs/doax_*.log`:

| Pattern | What it tells you |
|---------|-------------------|
| `fiber-probe kind=yield` | Every fiber yield in window: site, lr, r28-r31 before/after, gpr_preserve |
| `fiber-probe kind=swap` | Guest-PC fiber swap target + r30/r31 clobber |
| `fiber-probe kind=sched-swap` | Scheduler fiber swap sched before/after |
| `fiber-probe kind=clobber` | r30/r31 zeroed after swap (compare good vs bad runs) |
| `fiber-probe kind=menu-fiber` | Menu work fiber enter/exit |
| `fiber-probe kind=boot-fiber` | Boot work fiber during menu phase |
| `fiber-probe kind=drain` | Drain dispatch during window (incl. `post_dismiss_trace`) |
| `four-menu-probe` | Phase snapshot on dismiss / clobber / boot-fiber |
| `menu-snapshot` | One-shot when `boot_present==1` first seen |

**Input log (build `input-log2`, 2026-06-24):** SDK-level `InputSystem::GetState` probe
(`DOAX sdk-input-probe`) catches all XAM input, not only guest `0x82782BF0`. Guest scheduler
block at `0x833B8DF8` logged via `DOAX guest-input-probe`. `movie-poll` now logs post-call
`r3` result (`done`/`pending`), not the pre-call pointer in `r3`.

Grep patterns:

| Pattern | What it tells you |
|---------|-------------------|
| `DOAX sdk-input-probe` | Every merged host input sample (all callers) |
| `DOAX sdk-input-probe site=edge` | Button rising edge from SDK path |
| `DOAX sdk-input-probe site=Keystroke` | Keyboard/text input path |
| `DOAX guest-input-probe` | Scheduler `0x833B8DF8` header + menu bytes at transition |
| `DOAX input-probe site=GetState-wrap` | Guest `0x82782BF0` wrapper only (lr tagged) |
| `input-transition-probe site=button-edge` | Correlated A/Start with guest snapshot |
| `input-transition-probe site=movie-poll` | Post-call poll `r3` (`done` = skip/dismiss ready) |

Do not infer "no user input" from `user_dismiss=0` alone — check `sdk-input-probe` and
`guest-input-probe` at `LABEL_12-attempt` first.

**Fix v20 (build `press-start-dismiss`, from `doax_051`):** `movie-poll` `done` latched
`dismiss_f9` at `0x833B8DF9` without user A; midasm blocked LABEL_12 at stable sched `2/2`;
game fell through to LABEL_34 Travel (`scene_id=15`) → black/wrong screen.

1. `SanitizeAutoPressStartDismissLatch` clears `dismiss_f9` after `movie-poll-done` and at
   bring-up latch unless user A/Start confirm is armed.
2. `ArmPressStartUserConfirm` sets `dismiss_f9` only on real A/Start edge after bring-up.
3. `CanAcceptPressStartDismiss` accepts LABEL_12 at idle `5/1` or stable `2/2` with user confirm.
4. Block LABEL_34 auto-travel during Press Start hold until user dismiss.

Grep: `press-start-dismiss`, `cleared auto dismiss_f9`, `LABEL_34-blocked`, `stable-2/2-dismiss`.

**Fix v21 (build `press-start-label34`, from `doax_052`):** After island overlay latches
(`overlay=1`), user A dismiss runs through **LABEL_34** (`lr=0x824C191C`, scene 15) — not
LABEL_12. Blocking all LABEL_34 during hold caused infinite black once overlay appeared.

1. Block LABEL_34 only when `!HasPressStartUserConfirm()` (auto `dismiss_f9` 1/2).
2. On user confirm, accept LABEL_34 as dismiss path and call guest transition.
3. Do not arm travel suppress for LABEL_34 during Press Start hold.
4. `IsPressStartDismissEligibleState`: `boot_present` 1 or 2 (overlay may be 1).

Grep: `press-start-label34`, `LABEL_34-overlay-dismiss`, `blocked LABEL_34 auto-travel`.

**Fix v22 (build `press-start-fade-snap`, from `doax_053`):** Four-menu worked but black
flash during Press Start → 4-menu. `AcceptPressStartUserDismiss` set `user_dismissed` before
`ShouldArmTravelSuppress` ran, so `IsPressStartHoldPhase()` was false → `travel_arm=1`,
boot-fiber suppress, and full timeline=900 travel fade.

1. `g_press_start_overlay_dismiss` latched on LABEL_34 accept; exempt from travel suppress
   even after `user_dismissed`.
2. `SnapPressStartDismissFadeComplete` on timeline/fade-alpha/menu-fiber-yield during overlay
   dismiss — skip the long travel fade countdown.

Grep: `press-start-fade-snap`, `dismiss-snap`, `travel_arm=0` at LABEL_34 dismiss.

**Fix v23 (build `moviepoll-standdown`, from `doax_083`-`doax_085`):** After
Press Start dismiss reached four-menu idle, the title's no-input timeout path still fired
`LABEL_12` (`lr=0x824C1770`, scene 17). Blocking only the `MenuSceneTransition` call was too
late: `MenuPreTransitionHook` and setup had already started the timeout movie blend.

1. Add midasm `0x824C1714 -> 0x824C1770` (`DOAX_SkipPostReadyAutoScenePrelude`) so the
   post-ready timeout is blocked before pre-transition/setup calls when no fresh A/Start edge
   was seen.
2. Restore post-ready four-menu idle bytes on block: scheduler `5/1`, `overlay=1`,
   `dea=0`, `deb=0`, `def=1`, `df0=1`, `df4=0`.
3. After `g_press_start_dismiss_idle_finalized`, `DOAX_MenuTransitionMoviePoll` restores
   the same idle state and returns before `MaybeSealAndSnapBackPostDismissPromotion`; this
   prevents the old snapback from pushing the menu to `3/3` / item 17.

Grep: `timeout-prelude-standdown`, `moviepoll-standdown`, `blocked post-ready timeout prelude`,
`movie-poll-post-ready-restored`, `finalized-no-snapback`.
