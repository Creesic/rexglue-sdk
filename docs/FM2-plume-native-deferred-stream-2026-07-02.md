# FM2 plume_native — the deferred render-command stream (2026-07-02)

The single biggest correctness unlock so far: FM2 routes essentially all of its
per-draw render state (and the entire UI/overlay pipeline) through a deferred
command system that plume_native had been **silently discarding since the
beginning**. Everything rendered before this date was drawn with that state
stream frozen at defaults.

## The architecture (IDA-verified, RTTI names)

- Game threads record operations as `CParams*` objects (e.g.
  `CRenderAdapterLink::CParams2IFixedFunctionRendererSetRenderState_Inline`,
  `...SetPredication`, `...SetWorldTransform`,
  `COverlayRendererDeferred::CParams1IOverlayRendererSetGlobalOffset`),
  bump-allocated from the pool returned by `FM2_Render_GetDeferredCommandPool`
  (global `0x82A028DC`) and enqueued by pointer via `sub_8245CED8` into the
  queue object at guest `0x4001CA20`.
- The render thread's RunFrame consumes ONE submitted buffer per frame:
  `sub_8245D448` (swap next buffer in) → `sub_8245D048` (dispatch its nodes).
  Draw execution already happens on this thread, in this order (verified by
  hook thread IDs) — state and draws are correctly sequenced.
- Backpressure: the per-frame submit/retire function (`0x8245D5B8`) latches
  `pool+145` whenever the submitted-buffer backlog (`+36`, snapshotted to
  `+128`) exceeds the threshold (`+132`, = 3). While latched, any enqueue with
  `a5==0` and scope credit (`+140`) ≤ 0 spins up to 1000 yields, then **frees
  the command without executing it**. Organic volume is ~100-300k commands/sec
  (SetRenderState alone hit 107k/s), so on plume — where the drain never
  caught up past a ~7-buffer boot backlog — the latch stayed set forever and
  everything died at the gate. This same mechanism was the session-6P car-node
  loss (`kForceCarNodeEnqueue` bypassed it for one function).

## Fixes landed (FM2/src/render)

1. **Drain catch-up** (`d3d_hooks.cpp`, `sub_8245D048` hook, `kDrainCatchUp`):
   after the game's own drain, keep swapping+draining (via imported
   `sub_8245D448` + the original drain) until the backlog is ≤ 1 (cap 16).
   Draining merely to == threshold re-latches at the next submit. Result:
   drops → 0, the full state stream executes.
2. **Guest-upload guard** (`render_state.cpp`, `UploadGuestVertexData`):
   validates the source range against BOTH host windows — virtual
   (`virtual_membase`, via `HostToGuestVirtual`+`LookupHeap`) and physical
   (`physical_membase`, 512MB, via `GetPhysicalHeap`) — PM4 geometry arrives
   via `TranslatePhysical`. (A first version only accepted the virtual window
   and null-bound every PM4 vertex upload = full black screen.)
3. **Bind-time EDRAM tile identification** (`render_state.cpp`,
   `SetFramebuffer`): with live state, the game renders whole frames into a
   frame-wide × 256 tile surface (recreated per menu screen) under a
   1280x256 tile viewport/scissor, and resolves once full-surface — the old
   band-resolve detection never fires. A 1280x256 color RT on FM2's 720p
   frame is unambiguous: grow it to 1280x720 at first bind
   (`ResizeTileSurface`) and expand the game's tile-window viewport/scissor
   to the surface in `FlushViewport`. User-verified: full-screen rendering
   at boot and menus.

## What the un-dropped stream is NOT

It is not a retry loop and not a flood bug — the 107k/s SetRenderState rate is
the game's normal per-draw traffic. Force-enqueueing *dropped* (already-freed)
commands is NOT safe (their bump-pool params get recycled → deterministic AVs
in `UploadGuestVertexData`/`Fm2BindIndexBuffer`); keeping the latch open so
commands never drop IS safe (prompt execution, live params).

## Open fronts

- **2D/overlay placement matrices** (jumbled 2D meshes, no text):
  `FM2_Render_UiOrScreenDrawListSubmit` (= `COverlayRenderer` vtable slot 20,
  stages per-element ModelView; Xenia ground truth = small scale + NDC
  translation per element) never fires even at ZERO drops — its Render
  command is never recorded. Second gate upstream, unfound. See
  `docs/FM2-ida-renames-2026-07-02.md` for the renderer-class map.
- **Textures** — untriaged post-unlock; texture state also rides the deferred
  stream (`D3DCommandBuffer_SetTexture` fixups), so re-capture before
  assuming the old analysis holds.
- Temp diagnostics to remove when stable: `FM2_UIPM4*`, `FM2_FNUP`,
  `FM2_DRAWSEQ`, `FM2_ENQ_HIST`, `FM2_POOLSTATE`, `FM2_UIENQ`,
  `FM2_ENQ_DROP`, `FM2_UISUBMIT`, `FM2_DRAIN_CATCHUP` logs.

## Addendum (same day, fresh captures fm2pressstart/fm2mainmenu.rdc)

- `FM2_Render_UiOrScreenDrawListSubmit` (0x825B8A60) is
  **`IOverlayRenderer::PlayCommandBuffer`** — interface slot +0x50 on both
  `COverlayRenderer` and `COverlayRendererDeferred`. The deferred recorder is
  `FM2_COverlayRendererDeferred_RecordPlayCommandBuffer` (0x82279210, RTTI
  `CParams3IOverlayRendererPlayCommandBuffer`); its execute thunk
  (~0x822791F0) never appears in the enqueue stream even with zero drops:
  the UI never records the command. Lead theory: the compiled command-buffer
  handle it would play is null — probe `FM2_D3D_BeginCommandBufferBatch`
  (0x82375A40), `FM2_D3D_FinalizeCommandBufferBatch` (0x82376A58),
  `D3DCommandBuffer_CreateClone` (0x823759A8).
- Texture front: game draws bind ZERO pixel-shader SRVs (menu capture
  ev1818, readonly list empty) while our own present blit binds its source
  fine — the per-draw texture descriptor delivery
  (`ApplyLiveTexturesFromContext` onward) is the gap to trace.
