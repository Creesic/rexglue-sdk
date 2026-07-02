# FM2 IDA renames — 2026-07-02 session 3 (UI screen render pipeline cluster)

20 renames + 2 new function definitions in `default.xex.i64` (ida37), all
evidence-based from decompiles + the live x64dbg proof chain in
`docs/FM2-handoff-2026-07-02-session3.md`. Three names were already burned in
`FM2/fm2_manifest.toml` and were synced there in the same change (no
`FM2/src` hook references the old names; takes effect on next intentional
codegen only).

## Naming context

This cluster is the frontend 2D/UI screen render pipeline discovered while
closing the overlay-gate hunt: `UIScreenHost` (vtbls `0x8210B600`/`0x8210B6A8`,
no RTTI) ticks and renders UI screens; its member at +524 (vtbl `0x8210E390`,
no RTTI) is named `UIScreenPassList` here — it owns the render-pass array
(304-byte passes at +80, count +92), the `CRenderAdapterLink` at +16, the
`COverlayRendererDeferred` at +20, and 10 renderer plugin children at
+104..+140. `CRenderAdapterLink` (RTTI-confirmed) records
**IFixedFunctionRenderer** CParams into deferred pool `0x4001CA20` — every
recorder below is identified by the mangled CParams vftable constant its body
stores, which is as close to ground truth as this binary gets.

## Renames

| Address | Old | New | Evidence |
|---|---|---|---|
| 0x825DCE08 | (undefined code) | `FM2_UIScreenHost_RenderPassListIfPending` | New func. Slot 6 of both UIScreenHost vtbls. Gates: `*(this+744)!=0` && pending-element vector (+744/+748) non-empty, then `b` (tail-call) to the pass loop with `this=*(this+524)`. Live-verified running per tick. |
| 0x825DEFC0 | sub_825DEFC0 | `FM2_UIScreenHost_TickScreenAndDispatchEvents` | Slot 5 of both UIScreenHost vtbls (live stack: main loop → mgr thunk → here). Clears event vectors +724/+740/+756, manages current-screen ComPtr (+772), `FM2_UIScreenFireControlEventsOnPath`, mftb-budgeted event dispatch loop (74812.5/n ticks per element), tail-calls the PreRender broadcast. |
| 0x82609AB8 | sub_82609AB8 | `FM2_UIScreenPassList_RenderPasses` | Iterates pass array (+80, 304 B strides, count +92) in reverse; per pass calls the element-draw dispatch with the CRenderAdapterLink; broadcasts plugin children vtbl[6]/[4]/[5]/[7] around passes; zeroes count at end. Pass-timing scalars via `FM2_Render_SetPassTimingScalar`. |
| 0x8260A868 | sub_8260A868 | `FM2_UIScreenPassList_AddElementPass` | Element type dispatch: type `dword_82A0583C` → renderer vtbl[2](packed color); type `dword_82A056D0` (material) → grows pass array if needed, `*(this+92)++`, inits pass; other types → appends element to current pass (+96). Live-watched writing count 1..8+ per tick. |
| 0x826092F0 | sub_826092F0 | `FM2_UIScreenPass_ApplyStateAndDrawElements` | Per-pass: renderer +168 setup, uploads the pass's three 64-byte matrices via adapter slots 36/37/38 (SetWorld/View/ProjTransform), AddLight loop over 104-byte records (+212), SetRenderState(328,1)/(332,0xFFFF), reverse walk of 20-byte element records (+228) → per-element render, optional +4/+88 finish. Gate: element vector non-empty (47 elements live-read). |
| 0x826091C8 | sub_826091C8 | `FM2_UIScreenElement_RenderWithTilePredication` | Computes EDRAM tile-band mask for the element, `RecordSetPredication(mask)` (+80), type-id dispatch (two known ids), `RecordSetPredication(0)` after. |
| 0x826075E0 | sub_826075E0 | `FM2_UIScreenElement_RenderTimerText` | `osTIMER2::SnapshotSec`, static-init guarded string buffers + atexit, formats text, draws via renderer +132 (text) with scaled x/y, brackets with SetRenderState(64/84/88). |
| 0x82603DA0 | sub_82603DA0 | `FM2_UIScreenElement_ComputeTileBandMask` | Computes element y-range (from DeferredTaskParams fields), tests against band ranges, ORs `2<<n` per intersecting band; default full mask `2<<(2*count)`. Result feeds RecordSetPredication — predicated-tiling band mask. |
| 0x82603BE0 | FM2_ReleaseOwnedChildObjects | `FM2_UIScreenPassList_PreRenderRenderers` | MISNAME fixed (manifest synced). Body: `(*(this+16))->vtbl[0]()` = CRenderAdapterLink RecordPreRender; `(*(this+20))->vtbl[8]()` = COverlayRendererDeferred RecordPreRender (live-proven: the only overlay call in menus); walks 10 children calling vtbl[3] (ret-0 stubs for renderers). Nothing is released. |
| 0x82277CB0 | FM2_SceneCamera_CallVfunc20 | `FM2_GraphicsManager_TickUIScreenHost` | MISNAME fixed. `(*(*(mgr+2392)+8))->vtbl[5]()` — live stack proves dispatch lands in the UIScreenHost tick. mgr+2392 binder built in `FM2_GraphicsManager_InitRenderersAndTargets`. |
| 0x822792A0 | FM2_StartQueuedTask_VTable8200ECF4 | `FM2_COverlayRendererDeferred_RecordPreRender` | Stores `COverlayRendererDeferred::CParamsIOverlayRendererPreRender::vftable` (0x8200ECF4). Manifest synced. |
| 0x8227BD08 | FM2_StartQueuedTask_VTable8200F160 | `FM2_CSimpleModelRendererDeferred_RecordPreRender` | Stores `CSimpleModelRendererDeferred::CParamsISimpleModelRendererPreRender::vftable` (0x8200F160). Manifest synced. |
| 0x8227B318 | sub_8227B318 | `FM2_CRenderAdapterLink_RecordPreRender` | Stores `CRenderAdapterLink::CParamsIFixedFunctionRendererPreRender::vftable`. Slot 0 of vtbl 0x8200EEBC. |
| 0x8227A0E8 | sub_8227A0E8 | `FM2_CRenderAdapterLink_RecordSetRenderStateInline` | Stores `CParams2IFixedFunctionRendererSetRenderState_Inline` + (state,value). Slot 8. |
| 0x8227A500 | sub_8227A500 | `FM2_CRenderAdapterLink_RecordSetPredication` | Stores `CParams1IFixedFunctionRendererSetPredication` + mask. Slot 20 — NOT PlayCommandBuffer (session-2 assumption corrected). |
| 0x8227A9F8 | sub_8227A9F8 | `FM2_CRenderAdapterLink_RecordAddLight` | Stores `CParams1IFixedFunctionRendererAddLight` + 104-byte light payload. Slot 23. |
| 0x8227ACF0 | sub_8227ACF0 | `FM2_CRenderAdapterLink_RecordSetWorldTransform` | Stores `CParams1IFixedFunctionRendererSetWorldTransform` + 64-byte matrix. Slot 36 — the per-element 2D placement matrix carrier (the dropped CParams behind jumbled menus). |
| 0x8227ADE8 | sub_8227ADE8 | `FM2_CRenderAdapterLink_RecordSetViewTransform` | Stores `CParams1IFixedFunctionRendererSetViewTransform` + matrix. Slot 37. |
| 0x8227AEE0 | sub_8227AEE0 | `FM2_CRenderAdapterLink_RecordSetProjTransform` | Stores `CParams1IFixedFunctionRendererSetProjTransform` + matrix. Slot 38. |
| 0x825B6EA8 | (undefined code) | `FM2_COverlayRenderer_PreRender` | New func (3 instructions: `*(this+0x9C)=0; blr`). Slot 8 of immediate COverlayRenderer vtbl 0x82108F28 = IOverlayRenderer::PreRender impl. |

## Skipped / follow-ups

- `sub_82608F40` (element else-branch in RenderWithTilePredication) and
  `sub_82603C70..sub_82604320` container-method siblings: not yet decompiled
  with confidence — future pass.
- UIScreenHost class pair (vtbls 0x8210B600 vs 0x8210B6A8, ctors
  sub_825DFCD8/sub_825E0048/sub_825E03D8/sub_825E04C0): two sibling classes,
  no RTTI; dtors/ctors not renamed pending class-identity evidence.
- `FM2_Render_UiOrScreenDrawListSubmit` (0x825B8A60) keeps its name; it IS
  IOverlayRenderer::PlayCommandBuffer (slot 20) — accurate enough, and it is
  legitimately unused in menu flow.
