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
