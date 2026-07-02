# FM2 plume_native handoff — 2026-07-02 session 2 (deferred-stream unlock day)

Where things stand at handoff, what changed, and the exact next actions.
Companion docs: `FM2-plume-native-deferred-stream-2026-07-02.md` (architecture +
fixes), `FM2-ida-renames-2026-07-02.md` (renderer-class map, 28 renames).
Memory: `project_plume_native_state_rewire.md` sections #20-#28.

## Headline state

- **Rendering is LIVE**: animated menus, 3D racetrack outline, stopwatch model,
  full-screen fill at boot and menus (user-verified). This session unlocked the
  deferred render-command stream that plume had silently discarded since day
  one, fixed the vertex-upload guard, and added bind-time EDRAM tile
  identification. All fixes are in the working tree (uncommitted), build green
  via `cmake --build --preset win-amd64-relwithdebinfo --target fm2` (remember
  the LIB env prepend from `AGENTS.md`/memory).
- **ONE bug remains for menu visuals** (two symptoms, one cause): 2D elements
  (menu backgrounds, sprites, glyph text) are jumbled/invisible because their
  per-element placement matrices are staged by `COverlayRenderer` /
  `IOverlayRenderer::PlayCommandBuffer` — and **nothing in the frontend ever
  invokes the overlay renderer in plume**. Texture decode/binding is PROVEN
  perfect (capture texture 941 = pixel-perfect orange Forza menu art;
  SharedConstants carries real descriptor indices; 68k translate-ok/sec).
  Fix the overlay invocation gate → placement matrices + text + sprites +
  visible textures all arrive together.

## Live x64dbg hunt (interrupted mid-session — resume here)

Goal: find the frontend function that SHOULD call the overlay renderer and the
condition that skips it.

- Setup that works: launch plume bat → x64dbg attach fm2.exe → `plugload
  mcpbridge` if needed. Guest allocations are DETERMINISTIC across runs:
  - graphics manager (render-thread object) = guest `0x4001C170`
  - `COverlayRenderer` = guest `0x2E005660` (host `0x12E005660`, vtbl BE
    `82108F28`)
  - `COverlayRendererDeferred` = guest `0x2E048400` (host `0x12E048400`, vtbl
    BE `8200EC5C`)
  - host membase = `0x100000000`; symbolize hits via
    `llvm-symbolizer --obj=fm2.exe --relative-address (RIP - mod.base)`.
- **Result 1**: HW read-watchpoints on manager+0x870/+0x874 (the slots) got ZERO
  hits through full menu navigation → the frontend uses pointer copies cached
  at init (before attach), never the manager slots. No copies exist in guest
  .data globals (searched) — they're heap members (21 copies of the deferred
  ptr found via `findallmem 100000000, 2E048400, 100000000`).
- **Result 2**: HW read-watchpoint on the OBJECTS' vtable fields
  (`bph 12E048400, r, 4` + `bph 12E005660, r, 4`) hits IMMEDIATELY, in
  `__imp__FM2_ReleaseOwnedChildObjects` (= guest `0x82603BE0`,
  fm2_recomp.34.cpp:75188), with guest addrs `0x825DF510`/`0x825DF544` live in
  r14/r12. Interpretation NOT yet settled: this is a child-object
  release/refcount walk touching the deferred overlay object — either frame
  noise (an owner releasing a ref each frame; continue past it with a
  log-and-continue loop to find real method calls) or itself the clue (the UI
  tears down/never-acquires the overlay each frame; 0x825DF5xx is worth
  decompiling — near FM2_Render_* / scene-view vtable region 0x825E8BF8).
- **Next concrete steps**:
  1. Decompile `0x82603BE0` (FM2_ReleaseOwnedChildObjects) + the functions at
     `0x825DF510`/`0x825DF544` in ida37 (FM2.xex.i64 — do NOT `idb_save`).
  2. Resume the watchpoint session; on each hit, symbolize RIP and log; skip
     Release-walk hits until a non-release consumer appears (or prove only the
     release path ever touches it → the gate is "overlay never acquired by any
     screen": hunt whoever POPULATES the 21 heap copies — watch one of those
     copy addresses for READS instead).
  3. When the frontend caller is identified, decompile it, find the branch
     that skips PlayCommandBuffer/SetGlobalOffset, and force/emulate its
     condition (same pattern as the 6P car-node and drain fixes).

## Fixes landed this session (all in tree, uncommitted)

| Fix | Where | Status |
|---|---|---|
| Drain catch-up (deferred queue drops → 0; drain to backlog ≤ 1, cap 16) | `d3d_hooks.cpp` sub_8245D048 hook, `kDrainCatchUp` + `g_cbQueueSwapNext` import | user-verified live rendering |
| Guest vertex-upload guard (both host windows: virtual + physical) | `render_state.cpp` `UploadGuestVertexData` | verified (fixed self-inflicted black screen) |
| Bind-time EDRAM tile grow (1280x256 color RT on 720p frame → grow + viewport/scissor expansion) | `render_state.cpp` `SetFramebuffer` + `FlushViewport`/scissor | user-verified full-screen |
| Force-all-enqueue experiment | REVERTED (`kForceAllNodeEnqueue=false`) — recycled CParams crash | closed |

## Diagnostics inventory (all TEMP — remove when the overlay gate is fixed)

`FM2_UIPM4*` (+chain walk), `FM2_FNUP`, `FM2_DRAWSEQ`, `FM2_ENQ_HIST`,
`FM2_POOLSTATE`, `FM2_ENQ_DROP`, `FM2_UIENQ`, `FM2_UISUBMIT`,
`FM2_OVL_SETGOFF`, `FM2_OVL_PLAYCB`, `FM2_CBFINALIZE`, `FM2_CBCLONE`,
`FM2_TEXBIND`, `FM2_UPLOAD_GUARD` (the guard itself is permanent; the log line
can stay), `FM2_DRAIN_CATCHUP` log, CbBatch*/OvlPlayCb counters.

## Known issues parked

- **Memory leak (user is monitoring)**: render-target/depth pair recreated per
  menu frame and retired-but-never-freed (`g_retiredTile*` vectors +
  per-screen surface churn; capture shows RT ids 839→17198 live). Killed one
  x64dbg session via OOM. Cleanup after the overlay fix.
- Boot-time press-A 2D elements still garbage until the overlay fix (same
  root cause).
- The wider `FM2_Audio*` misname cluster in IDA (graphics code) — rename pass
  pending, see renames doc caveats.
- `FM2_Render_TestPassVisibilityVMXCore` (0x8251E410) is misnamed — it's the
  sorted-path pass CB compiler; rename when touching that area.

## Key facts cheat-sheet

- Deferred command queue = guest `0x4001CA20` (manager+0x8B0); drop latch
  p145 = backlog(+36/+128) > threshold(+132=3); scope credit +140.
- `FM2_Render_UiOrScreenDrawListSubmit` (0x825B8A60) ==
  `IOverlayRenderer::PlayCommandBuffer` == vtable slot +0x50 (both impls).
  Deferred recorder 0x82279210; generic CParams thunk 0x82278FF0
  (`FM2_Deferred_CParamsExecuteThunk_Shared`).
- CB compile chain (Begin 0x82375A40 / Finalize 0x82376A58 returns HRESULT,
  0 = S_OK / Clone 0x823759A8) is fully healthy, ~1/frame.
- Xenia ground truth glyph ModelView (arcade A/B captures): rows
  `[0.033,0,0,0][0,0.059,0,0][-0.594,-0.756,0.118,1]` = per-element NDC
  placement; our regs 0-3 get colors+zeros without the overlay staging.
- Launch: `scripts\fm2\launch-fm2-plume-native.bat` ONLY (bare exe = dead
  xenos path). Keyboard: Space=A, Escape=Start, arrows=D-pad.
- Log: `C:\temp\fm2-clean.log`, append-mode — always tail from a recorded
  byte offset.
