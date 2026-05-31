# FH1 guest-PC fiber interpreter

Date: 2026-05-29

## Problem

FH1 loading uses a cooperative fiber scheduler built on `KeSetCurrentStackPointers`.
After `sub_830ED910` saves thread context and restores job context, the guest **blr**s
to a resume address (often `0x830ED900`, not a function entry). Static recomp emits
`return` for `blr`, so the host call stack cannot represent fiber continuations.

Prior fixes in `fh1_workqueue_dispatch.cpp` simulated resume LRs by hand after each
KeSet. That worked for debugging but did not scale to nested fiber polls
(`sub_82C0BEC8` → `sub_82C0BC88` → `sub_830ED910` inside an outer job).

## Approach

`FH1/src/fh1_guest_pc_fiber.cpp` implements a **guest-PC driver** for fiber sites:

1. **KeSet hook** records swap-to-fiber vs swap-back (via `SetFiberStackSwitchHandler`).
2. **`RunFiberSwap`** wraps `__imp__sub_830ED910`; after swap-to-fiber, calls
   **`RunGuestPc(ctx, base, ctx.lr)`** instead of returning through generated C++.
3. **`RunGuestPc` loop** at each PC:
   - **Native resume sites** (hand-coded semantics, e.g. `0x830ED900` → EBEA0 job dispatch + swap-back tail)
   - **Registered function entries** via `REX_LOOKUP_FUNC` / `FunctionDispatcher`
   - **Minimal PPC stepper** for in-region fall-through (BC88 epilogue at `0x82C0BDB4`, etc.)
4. **Frame stack** (`fiber_slot_at_entry`, `job_ctx`) tracks the **outermost** fiber
   swap only. When `RunGuestPc` is already active, inner `BC88→ED910` calls reuse
   `frames.back()` without push/pop. Nested swap-to-fiber dispatches via native
   `sub_830EBEA0` inline from `RunFiberSwap` (no longjmp back to outer `RunGuestPc`).

## Fiber address regions

| Region | Range | Contents |
|--------|-------|----------|
| Work queue | `0x82C0B800`–`0x82C10000` | `sub_82C0BC88`, `sub_82C0BDC8`, `sub_82C0BEC8` |
| Fiber swap | `0x830EBE00`–`0x830EF000` | `sub_830EBE90`, `sub_830ED910`, resume labels |

## Known native resume sites

| PC | Semantics |
|----|-----------|
| `0x830ED900` | `mr r3,r31`; `bl sub_830EBEA0` — handled by FH1 `native_site_handler` |
| `0x82C0BDB4` | BC88 post-swap epilogue (stepped when on thread stack). **Do not** rewrite to `0x830ED900` — that retriggers poll-job dispatch and thread-stack overflow. |

After `sub_830EBEA0` returns, only run the swap-back tail if `r1` is still on a fiber stack (`0x70xxxxxx`). Nested swaps inside the poll job may already have restored the thread stack — **do not drain interpreter frames while `GetFiberJobDispatchDepth() > 0`** (BDC8 reconcile used to clear frames mid-BEC8 when `run_depth` was 0).

Add new rows here when logs show `unimplemented insn` or `outside fiber regions`.

## Integration

- `sub_830ED910` REX hook → `fh1::fiber::RunFiberSwap`
- Work-queue ring commit / queue sanitize remain in `fh1_workqueue_dispatch.cpp`
- KeSet stack-span sync remains in SDK `xboxkrnl_threading.cpp`

## Upstream path (main ReXGlue repo)

If this proves stable on FH1:

1. Move core loop to `include/rex/ppc/guest_pc_fiber.h` + `src/system/guest_pc_fiber.cpp`
2. Codegen: emit `GuestPcFiberResume(ctx, lr)` instead of `return` after KeSet+blr
3. Title layer keeps only queue/track-loader policy guards

## Resume PC after host-boundary KeSet (2026-05-29)

Codegen `fiber_swap_host_boundary` calls `GuestPcFiberResume(ctx, base, ctx.lr)` when
KeSet longjmps to the fiber stack. At that point guest `mtlr` has **not** run yet, so
`ctx.lr` is still the fiber swap entry (`0x830ED910`) or the BC88 caller return
(`0x82C0BDB4`) — not the post-KeSet blr stub (`0x830ED900`).

If `RunGuestPc` starts at `0x830ED910`, it re-enters the full generated swap function
and exits immediately instead of spinning the BEC8 poll loop.

**Fix (SDK + FH1):** `ResolveSwapToFiberResumePc()` applies configured `resume_rewrites`
(including `{0x830ED910 → 0x830ED900, only_on_fiber_stack=false}` because guest `r1` may
not reflect the fiber stack yet when `GuestPcFiberResume` runs). If still not a
`native_sites` entry, rewrite when on the fiber stack **or** when `resume_pc` is a
function entry inside a registered fiber region (swap-entry re-entry).

Log: `Guest-PC fiber: rewrite swap-to-fiber resume 0x830ED910 -> 0x830ED900`

**Symptom when broken:** `823ED888` runs on fiber stack (`sp=0x7001FFB0`) after KeSet
from nested `82C013E0` → BC88 inside the world-load worker; no
`native resume job_fn=0x...` log; worker AV at `lwz 600(r31)`; spinner forever.

**SEH blocker (2026-05-31):** Do not wrap `__imp__sub_823ED888` in `__try/__except`.
The worker nests BC88→ED910 KeSet longjmp; an outer SEH frame prevents
`GuestPcFiberResume` from running even when `ResolveSwapToFiberResumePc` rewrites
`0x830ED910` → `0x830ED900`. Preflight `object+600` readability instead.

## Log strings

- `FH1 guest-PC fiber: KeSet limit=...`
- `FH1 guest-PC fiber: swap-to-fiber complete, resume pc=...`
- `FH1 guest-PC fiber: resume 0x830ED900 job_fn=...`
- `FH1 guest-PC fiber: KeSet limit=... frames=... sdk_depth=...`
- `FH1 guest-PC fiber: nested swap (reuse frame)` → inner ED910 during BEC8 poll
- `FH1 guest-PC fiber: nested swap-to-fiber inline resume` → longjmp to RunGuestPc
- `FH1 guest-PC fiber: inline resume pc=...` → landed in guest-PC loop after longjmp
- `FH1 guest-PC fiber: pop frame (RunFiberSwap exit)` → outer swap balance
- `FH1 guest-PC fiber: drained N stale frame(s)` → reconcile on idle thread stack
- `FH1 guest-PC fiber: unimplemented insn ...` → extend stepper or add native site

## Test

Rebuild SDK + FH1, run to loading spinner, inspect `C:\temp\fh1-test.log` for the
strings above. Success looks like car zip VFS reads after swap-back, without
`InvalidFunctionTrap` on `0x830ED900`.

## Loading screen UI observation (2026-05-30)

**Observed behavior:**

| Mode | Spinner icon | Tip text rotation | Background load work |
|------|--------------|-------------------|----------------------|
| `sub_82C0BC88` stubbed (`stub_return = 1`) | Yes | Yes | No — poll loop fakes success, jobs never run |
| Real BC88 + guest-PC fiber interpreter (current) | No | No | Partial — car zip reads, but poll stalls before world-load worker |

When BC88 was stubbed, the loading screen **looked** alive (animated spinner + tips)
but could not finish because no job callbacks ran. After removing the stub and driving
the real work-queue / fiber path, that cosmetic UI stopped updating even though logs
show more real work (fiber swaps, `VIP_*.zip` reads).

**Likely explanation:** the loading-screen UI tick (spinner + tip strings) is driven by
a main-thread or render-side state machine that expects the worker poll to yield in a
particular cadence. The stub returned success instantly every poll, so the UI loop kept
advancing cosmetically while load never completed. The current path spends worker time
inside nested `BEC8 → BC88 → ED910` fiber polls (frame depth growth, `8247D7A8`
clearing pending jobs, BDC8 AV retries), which may block or starve the code path that
updates spinner/tip state.

**Expectation:** once the work-queue poll completes jobs reliably (pending `8247D7A8`
jobs preserved, stable `frames=`, BDC8 poll without stack-guard AV, world-load worker
`8255AE10` / `823ED888` reached), the loading screen should either resume spinner/tip
updates or transition past loading entirely. Absence of spinner/tips alone is not
regression vs stub — it may indicate we are stuck earlier or on a worker-blocked path
while doing more real I/O than the stub ever did.
