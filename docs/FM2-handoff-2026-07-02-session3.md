# FM2 plume_native handoff — 2026-07-02 session 3 (overlay-gate hunt CLOSED)

Continuation of `FM2-handoff-2026-07-02-session2.md`. That doc's central theory —
"nothing invokes the overlay renderer; find the frontend gate that skips
PlayCommandBuffer" — is now **disproven by live evidence**. There is no gate.
The menu 2D pipeline never uses `COverlayRenderer::PlayCommandBuffer`; it draws
through **CRenderAdapterLink**, and the real remaining bug is the already-known
**deferred-pool drop latch** (pool+145) that intermittently discards the
CRenderAdapterLink CParams stream. This doc records the proof chain, the full
frontend render architecture recovered along the way (with live addresses), and
the next steps.

## TL;DR

1. The 2D screen system runs COMPLETELY on the game side in plume: screens
   tick, render passes get registered (count reached 8+ per tick, live-read),
   pass element lists are populated (47 elements in pass[0], live-read), and
   the per-element draw dispatch executes.
2. All of that drawing goes through `*(container+16)` =
   **CRenderAdapterLink** (RTTI-confirmed, object `0x2E048440`, vtbl
   `0x8200EEBC`), whose recorders enqueue CParams into deferred pool
   `0x4001CA20` at ~600+/s (see `FM2_ENQ_HIST`: `8227AC88:243/s`,
   `8227A8F8:36/s`, ...).
3. `COverlayRenderer` / `COverlayRendererDeferred` receive exactly ONE call in
   this path: **PreRender** (deferred vtbl slot 8, `+0x20`), broadcast once per
   screen tick. `RecordBegin/RecordEnd/SetGlobalOffset/PlayCommandBuffer` = 0
   calls in 26 minutes of runtime (log-verified). That is apparently NORMAL
   for menu screens — the overlay renderer is not the menu 2D path.
4. The visible symptom (jumbled/invisible 2D, colors+zeros in regs 0-3) is the
   **drop latch eating the CRenderAdapterLink stream in bursts**: live
   `FM2_POOLSTATE` shows `p145` toggling 0↔1 with backlog crossing thresh=3.
   When latched, state/matrix CParams are dropped ⇒ draws execute with stale
   or missing per-element state. This is memory-note fix #20-21 territory:
   drain catch-up (`kDrainCatchUp`) fixes the drops but is DISABLED because
   executing state on the drain timeline vs draws on the hook timeline tears.
5. **Next big rock (unchanged from the rewire notes, now with proof it's the
   only rock): move draw execution onto the drain timeline, re-enable drain
   catch-up, and the full 2D stream (state + matrices + draws) executes in
   order with zero drops.**

## The proof chain (how the gate theory died)

Each step was verified live in x64dbg against `fm2.exe` (base
`0x7ff7136f0000`, membase `0x100000000`) + IDA (`ida37`, FM2.xex.i64):

1. HW read-watchpoints on both overlay objects' vtable fields
   (`0x12E005660` immediate / `0x12E048400` deferred), log-only, 12 s of live
   menu: exactly TWO reader instructions in the whole process.
   - deferred: `fm2.exe+0x228BC65` = `__imp__FM2_ReleaseOwnedChildObjects` —
     the per-tick broadcast (see below), NOT a refcount walk.
   - immediate: `fm2.exe+0x6DDB85` = `sub_82278FF0` — the shared deferred
     CParams execute thunk `return vtbl[8](obj)` ⇒ **PreRender executes**.
2. vtbl slot maps (read live): `COverlayRenderer` vtbl `0x82108F28`,
   `COverlayRendererDeferred` vtbl `0x8200EC5C`. Slot 8 = PreRender
   (immediate impl `0x825B6EA8` just does `*(this+0x9C)=0`), slot 20 (+0x50) =
   PlayCommandBuffer (`0x825B8A60` / recorder `0x82279210`). Both classes'
   slot 3 (+12) = `0x82278010` = `return 0` stub — the child-walk calls in
   `FM2_ReleaseOwnedChildObjects` are no-ops for renderers.
   The full IOverlayRenderer method map is recoverable from mangled
   `COverlayRendererDeferred::CParams*` vftable names at `0x8200ECB4..0x8200ED04`
   (SetViewport, SetGlobalRotation, SetGlobalOffset, SetupCommandBuffer,
   ReleaseCommandBuffer, RecordBegin, RecordEnd, PlayCommandBuffer, PreRender,
   BeginRender, EndRender + RenderLine/Quad/Circle/RoundedLine/LineStrip).
3. Breakpoint on the PreRender recorder body (found by byte-searching .text
   for the CParams vftable constant `0x8200ECF4` → single hit
   `0x7FF713DCFC20`; symbol-table publics addresses are unreliable in the
   merged recompiled code). Host stack at hit, symbolized:
   `sub_822172E8` (main loop) → `sub_82277CB0` (mgr thunk: calls
   `uiScreen->vtbl[5]`) → `sub_825DEFC0` (screen tick, slot 5) →
   `FM2_ReleaseOwnedChildObjects` (tick tail: **misnamed** — line 2 is a
   direct `(*(this+20))->vtbl[8]()` = PreRender on the cached deferred
   overlay; it also releases +16 and no-op walks 10 children) → recorder.
4. The screen RENDER method is vtbl slot 6 = `0x825DCE08` (not even a
   function in IDA yet): `if(*(this+744)==0)return; if(vector B empty)return;
   tail-call sub_82609AB8(*(this+524))`. HW watch on vector-B begin field:
   it RUNS every tick (58 hits/10 s) and its gates PASS (vector B live-read =
   2 entries).
5. `sub_82609AB8` = the render-pass loop over the CONTAINER (`*(screen+524)`).
   Live container read: pass count (+92) written up to 8+ per tick by
   `sub_8260A868` (pass registrar) and cleared by the loop; pass[0] element
   vector = **47 × 20-byte element records**. So per-pass rendering
   (`sub_826092F0`) and per-element dispatch (`sub_826091C8` etc.) all run.
6. The renderer interface those draws target = `*(container+16)` =
   `0x2E048440`, RTTI `.?AVCRenderAdapterLink@@`, vtbl `0x8200EEBC` (34+
   slots; slot 20 `sub_8227A500` is the CB-play analog on this class; +32
   SetRenderState; +144/148/152 matrix setters; +92 element draw). Its
   recorders are exactly the exec callbacks flooding `FM2_ENQ_HIST`.
7. Meanwhile `FM2_POOLSTATE` (live): `p145` drop latch toggling 0↔1,
   backlog 0..11 vs thresh 3, enq ~8k/s. Drops active in bursts ⇒ the
   CRenderAdapterLink stream loses state/matrix nodes exactly as described in
   the state-rewire notes (#20-21).

Also disproven en route: the `0x8210b6bc`-area "vtables" at `0x82194b70` /
`0x82196340` from session 2's step 1 are **.pdata unwind entries**, not
vtables; the `0x825DF510/0x825DF544` register values at the old watchpoint
hits were just code addresses inside `sub_825DEFC0` (the tick).

## Frontend 2D architecture map (live guest addresses, deterministic)

```
main loop sub_822172E8
  └─ sub_82277CB0                     ; calls *( *(gfxmgr+2392)+8 )->vtbl[5]
     └─ UI screen host  guest 0x413EF5D0   vtbl 0x8210B6A8 (sibling 0x8210B600)
        ├─ slot 5 sub_825DEFC0 = TICK (~6/s at boot screens)
        │    ├─ clears vectors +724/+740/+756
        │    ├─ FM2_UIScene_FireControlEventsOnPath(*(this+772)=screen 0x2E0A3E70,
        │    │        +724, +740, ...)   ; fills element/event vectors
        │    ├─ timed event dispatch (mftb budget 74812.5/n)
        │    └─ FM2_ReleaseOwnedChildObjects(*(this+524))   ; MISNAMED:
        │         (*(cont+16))->vtbl[0]()          ; CRenderAdapterLink
        │         (*(cont+20))->vtbl[8]()          ; overlay PreRender (the 3/s!)
        │         children[10]->vtbl[3]()          ; no-op stubs
        ├─ slot 6 sub_825DCE08 = RENDER
        │    gates: *(this+744) != 0 && vectorB non-empty
        │    └─ sub_82609AB8(container)
        └─ +524 CONTAINER guest 0x2E107120  vtbl 0x8210E390
             +16  CRenderAdapterLink 0x2E048440  vtbl 0x8200EEBC  ← ALL 2D draws
             +20  COverlayRendererDeferred 0x2E048400 (PreRender only)
             +80/+84 pass array 0x41258280 (304 B/pass), +92 count, +96 current
             +100 tick counter (a2), +104..+140 ten renderer children
             pass+228/+232: element vector (20 B records)
render-pass loop sub_82609AB8:
  children->vtbl[6] begin; per pass: sub_826092F0(pass, *(cont+16), flag)
    per element: sub_826091C8 / type-dispatch → CRenderAdapterLink virtuals
    (SetRenderState +32, matrices +144.., CB build sub_82603DA0 → +80 play)
  children->vtbl[7] end; count=0
pass registrar sub_8260A868: material elements →
  FM2_Render_DispatchMaterialPass0RecursiveOrInvalidate → *(cont+92)++ + pass init
```

Overlay create/store (session-verified): `FM2_GraphicsManager_InitRenderersAndTargets`
(0x822864D0): mgr+2160=COverlayRenderer, +2164=deferred (deferred+12=immediate),
+2156=CGraphicsStreamDeferred, +2168=CSimpleModelRenderer, +2392=binder(+8→ui host).

## Debugging gotchas learned this session

- **Symbol→host-address via PDB publics is unreliable** (addresses land
  mid-instruction inside merged recompiled bodies). Byte-search .text for a
  distinctive guest constant instead (`find fm2.exe:text, <LE bytes>`), or
  watchpoint a guest data field and symbolize the RIP.
- Guest `0x4001Cxxxx` (graphics-mgr/pool region) is NOT readable at
  `membase+guest` from the debugger this run (physical-alloc window?); find
  objects via `findallmem` on their vtable bytes instead. Other heap regions
  (0x2Exxxxxxx, 0x41xxxxxxx) read fine at `membase+guest`.
- x64dbg db-reload resurrects old HW watchpoints with broken single-step
  handling (last-chance EXCEPTION_SINGLE_STEP at the watched instr) — clear
  stale HW bps after re-attach.
- The x64dbg-MCP log capture redirects to a temp file per command; use
  `capture_log_ms` on the `run` command itself to harvest bp-log lines.
- fm2.exe crashed once while running unattended under the leak (session-2
  known issue). An `mm3.exe` debug session also appeared in the same x64dbg
  (not started by this session) — timeline unclear, user was at the machine.

## IDA rename candidates (evidence in this doc; not yet applied)

- `FM2_ReleaseOwnedChildObjects` (0x82603BE0) → screen-resources per-tick
  broadcast (releases +16 slot0, PreRenders +20 overlay, no-op walks children).
- `sub_825DEFC0` → UIScreenHost::Tick; `sub_825DCE08` → UIScreenHost::Render
  (needs func definition at 0x825DCE08 first); `sub_82609AB8` →
  container render-pass loop; `sub_8260A868` → pass registrar;
  `sub_826092F0` → pass element-draw dispatch; `sub_826091C8` /
  `sub_826075E0` → element render by type (uses osTIMER2, debug-text branch).
- `FM2_SceneCamera_CallVfunc20` (0x82277CB0) → misnamed; it's the UI-host
  tick thunk. `FM2_StartQueuedTask_VTable8200ECF4` (0x822792A0) →
  COverlayRendererDeferred::RecordPreRender (and `..8200F160` = the
  CSimpleModelRendererDeferred analog).
- `FM2_WaitScreen_FlushPendingEntries` (0x822884C8) is the movie/wait-screen
  playlist flusher (CWaitTexture/CWaitModel/CWaitMovie) — name is okay-ish.

## Next concrete steps

1. **Drain-timeline unification** (the real fix, per state-rewire #20-21 +
   this session's proof): make the hooked draw execution happen on the same
   timeline as drained state CParams (move draw execution to the drain), then
   re-enable `kDrainCatchUp` so backlog stays ≤1 and p145 never latches.
   Success criteria: `FM2_POOLSTATE p145=0` sustained, zero drops, menu 2D
   elements positioned/textured correctly.
2. If glyph placement is still wrong with zero drops + ordered execution,
   instrument `CRenderAdapterLink` slot 20 (`sub_8227A500`) and the matrix
   setters (+144/148/152 = slots 36-38) — THOSE carry the per-element
   ModelView data (not the overlay's SetGlobalOffset).
3. Clean up temp diagnostics (session-2 inventory) once verified.
4. Parked: memory leak (RT/depth churn per menu frame), `FM2_Audio*` misname
   cluster, boot-time press-A garbage (same root cause as #1).

## Part 2 (same session): drop latch + leak FIXED — stream fully executing

The "move draws to the drain timeline" step from the original plan turned out
to be ALREADY TRUE in-tree (kDrainCatchUp enabled; tid-verified draws execute
on the drain thread — the "tearing" memory note was stale). The real work was
three fixes, all landed and log-verified:

1. **Drop-threshold lift** (`d3d_hooks.cpp`, sub_8245CED8 hook,
   `kLiftDropThreshold`): pool+132 threshold 3 → 31, and unlatch p145 when
   backlog ≤ threshold. thresh=3 was flow control sized for a 60fps render
   thread; plume's ~10fps render thread let the backlog sit at 4-11 so p145
   stayed latched (bursts of dropped SetWorldTransform/SetRenderState = the
   jumbled 2D). First attempt used 15 — not enough: with drops gone the
   ORGANIC record rate (~80-160k cmds/s) fills bump buffers so fast that
   submits run ~100-130/s and the backlog rides ~30-63.
2. **Drain catch-up cap 16 → 64** (same file, sub_8245D048 hook): 16/call
   could never clear the ~31-buffer steady-state backlog at ~10 drain
   calls/s. 64 fully clears the ring every call.
3. **Tile-surface cache leak fix** (the parked "memory leak" — it became
   BLOCKING once the stream was unleashed; 23.6 GB WS and OOM death in ~60 s):
   `TranslateGuestSurface` cached the 1280x256 tile pair by guest address,
   `ResizeTileSurface` grew the same object to 1280x720, and the next frame's
   re-translation saw a dimension mismatch → erase + recreate + regrow +
   retire EVERY frame (desc ids climbing, retired vectors growing, orphaned
   GuestSurface structs). Fix: `GuestSurface::tileGrownFromHeight` records
   the pre-grow height (`render_state.cpp`), and the cache treats a lookup
   with the ORIGINAL dimensions as a hit (`d3d_resource_hooks.cpp`).

**Verified after the fixes** (fresh run, log offset 27358921): `FM2_POOLSTATE
p145=0` on every sample, backlog oscillating 0-30 under thresh 31, ZERO new
`TranslateGuestSurface` lines (was 4+/frame), WS stable ~1.1 GB (was 23.6 GB),
DrawSubmit ~22k/s (was ~2k), drain rate 130+/s (was ~10 — the leak churn was
strangling the render thread). The full CRenderAdapterLink stream (state +
world/view/proj transforms + predication) now records and executes in order.

**Still open:** visual confirmation of menu 2D placement (needs eyes on the
screen / pressing through to menus); the boot-screen PM4 draws still sample
c0=gray/c2=zeros in FM2_DRAWSEQ — if glyph placement is still wrong with the
stream now executing, instrument the IMMEDIATE fixed-function renderer's
SetWorldTransform execution (CRenderAdapterLink CParams → CFixedFunctionRendererX360)
to see where the matrix lands. ENQ_DROP=64 lines are all from early boot
(pre-lift); recent samples show none.
