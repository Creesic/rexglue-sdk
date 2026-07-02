# FM2 IDA renames — 2026-07-02 (renderer-class cluster: the "audio_*" misnaming epidemic)

Context: hunting the missing-menu-text root cause (see the plume_native state-rewire
memory arc). `FM2_Render_UiOrScreenDrawListSubmit` (0x825B8A60) — the handler that
stages per-element glyph placement matrices — has never fired in any logged
plume_native run. Tracing its ownership exposed a whole renderer-class family whose
factories/ctors carried **wrong `audio_*` / `FM2_Audio*` names** (from an early
heuristic pass). The binary's own RTTI vftable symbols (`??_7C...@@6B@`) and
`*Decl` shader-declaration strings give exact class names.

IDA db: FM2 = `default.xex.i64` / `Forza2\FM2.xex.i64` (MCP `ida37`). Do NOT `idb_save`.

## Key architecture identified (drives the missing-text work)

- `0x82108F28` = **vtable of `COverlayRenderer`** (272B; ctor sets it; decl string
  `"COverlayRendererDecl"`; pass keyword `"RenderOverlay"`).
  `FM2_Render_UiOrScreenDrawListSubmit` (0x825B8A60) is **slot 20 (+0x50)** of this
  vtable; field `+164` = the active command-buffer context it stages ModelView on.
- Each front-end renderer has a **`C*Deferred` wrapper** that records operations as
  `CParams*` objects (e.g. RTTI `COverlayRendererDeferred::CParams1IOverlayRendererSetGlobalOffset`)
  bump-allocated from the pool returned by `FM2_Render_GetDeferredCommandPool`
  (global `0x82A028DC`), enqueued to the render thread via `sub_8245CED8` and
  executed by the drain — **the same queue whose backpressure latch dropped the
  car/scene nodes in session 6P-2**. `FM2_ENQ_DROP` diagnostics show overlay-family
  CParams thunks (e.g. `0x82278FF0` = `call vtbl[8]` execute thunk) being dropped
  in plume_native ⇒ overlay submit never runs ⇒ no text.
- `FM2_GraphicsManager_InitRenderersAndTargets` (0x822864D0, was
  "FM2_AudioManager_InitAndBindSignalGate") constructs the full registry:
  global render context singleton (`g_FM2_GlobalRenderContext_` = 0x82A01570),
  `CFixedFunctionRendererX360` (+2144), its `CRenderAdapterLink` (+2152),
  `CDeferredLiveryRenderer` (+2156), `COverlayRenderer` (+2160),
  `COverlayRendererDeferred` (+2164), `CSimpleModelRenderer` (+2168),
  `CSimpleModelRendererDeferred` (+2172), `CGraphicsStreamDeferred` (+2140),
  `CRenderThreadLink` (+2392), plus the EDRAM/present texture set
  (XGSetTextureHeader A2B10G10R10/D24FS8/A16B16G16R16F...).

## Renames applied (ida37 / FM2)

| addr | old | new | evidence |
|------|-----|-----|----------|
| `0x825b9800` | `audio_effect_processor_create_and_init` | **`FM2_COverlayRenderer_CreateAndInit`** | Allocates 272B, ctor sets vtable 0x82108F28; init loads `"COverlayRendererDecl"`. |
| `0x825b96c8` | `sub_825B96C8` | **`FM2_COverlayRenderer_Ctor`** | Sets vtable 0x82108F28; `+164 = GetActiveCommandBufferContext()` (matches `UiOrScreenDrawListSubmit`'s `*(this+164)`). |
| `0x825b95c0` | `sub_825B95C0` | **`FM2_COverlayRenderer_Dtor`** | Resets vtable 0x82108F28, releases members, reassigns base vtable. |
| `0x825b8570` | `sub_825B8570` | **`FM2_COverlayRenderer_LoadDeclResources`** | `shader_resource_load_static_decl_by_name(this+76, "COverlayRendererDecl")` + resource resolve. |
| `0x825b8200` | `sub_825B8200` | **`FM2_COverlayRenderer_ReleaseResources`** | Clears resource lock (+9), interface (+1), zeroes +38; called on init failure. |
| `0x825b5d58` | `audio_render_target_create_or_validate` | **`FM2_CSimpleModelRenderer_CreateAndInit`** | 400B factory; init loads `"SimpleModelDecl"` (or per-object custom decl at +128). |
| `0x825b5928` | `sub_825B5928` | **`FM2_CSimpleModelRenderer_Ctor`** | Ctor called by the factory above. |
| `0x825b5b98` | `sub_825B5B98` | **`FM2_CSimpleModelRenderer_LoadDeclResources`** | The `"SimpleModelDecl"` loader. |
| `0x82278e00` | `FM2_AudioSignalGate_Ctor_EC5C` | **`FM2_COverlayRendererDeferred_Ctor`** | Sets RTTI vftables `IOverlayRenderer` → `COverlayRendererDeferred`. |
| `0x82278ff0` | `sub_82278FF0` | **`FM2_COverlayRendererDeferred_CParamsExecuteThunk`** | `return vtbl[8](this)` — the enqueued CParams execute trampoline (seen in FM2_ENQ_DROP). |
| `0x82278f80` | `sub_82278F80` | **`FM2_COverlayRendererDeferred_RecordSetGlobalOffset`** | Bump-allocates 8B CParams, sets RTTI vftable `CParams1IOverlayRendererSetGlobalOffset`. |
| `0x82557d28` | `audio_biquad_get_output_level` | **`FM2_ReadObjFloatAt16_`** | `return *(float*)(this+16)` (matches `FM2_ReadObjDwordAt8_/12_` convention). |
| `0x825d0af0` | `FM2_AudioManager_GetSignalGateField` | **`FM2_Render_GetDeferredCommandPool`** | Returns global 0x82A028DC; stored at deferred wrappers' +12 and consumed by `FM2_AllocPoolBumpAllocate` in CParams recording. |
| `0x822864d0` | `FM2_AudioManager_InitAndBindSignalGate` | **`FM2_GraphicsManager_InitRenderersAndTargets`** | See architecture section — builds renderer registry + EDRAM/present textures; zero audio content. |
| `0x8227b780` | `FM2_AudioSignalGate_Ctor_F0F0` | **`FM2_CSimpleModelRendererDeferred_Ctor`** | RTTI vftables `ISimpleModelRenderer` → `CSimpleModelRendererDeferred`. |
| `0x8227b618` | `FM2_AudioSignalGate_Ctor_F0A4` | **`FM2_CDeferredLiveryRenderer_Ctor`** | RTTI vftables `IDeferredLiveryRenderer` → `CDeferredLiveryRenderer`. |
| `0x82279e18` | `FM2_AudioSignalGate_Ctor_EEBC` | **`FM2_CRenderAdapterLink_Ctor`** | RTTI vftables `IFixedFunctionRenderer` → `CRenderAdapterLink`. |
| `0x8227ed30` | `FM2_AudioSignalGate_Ctor_F3E4` | **`FM2_CGraphicsStreamDeferred_Ctor`** | RTTI vftable `TRefCountedObjectThreadSafe<CGraphicsStreamDeferred>`. |
| `0x822768d0` | `FM2_AudioSignalGate_Ctor_E734` | **`FM2_CRenderThreadLink_Ctor`** | RTTI vftables `IRenderThread` → `CRenderThreadLink`. |
| `0x825b40b8` | `audio_manager_create_and_init` | **`FM2_Render_CreateGlobalRenderContextSingleton`** | Creates the 120B singleton stored in 0x82A01570 = exactly what `FM2_Render_GetGlobalRenderContext` returns. |
| `0x825ba160` | `audio_effect_processor_create_and_configure` | **`FM2_CFixedFunctionRendererX360_CreateAndInit`** | 1200B factory; init loads `"CFixedFunctionRendererX360Decl"`. |
| `0x825b9e50` | `sub_825B9E50` | **`FM2_CFixedFunctionRendererX360_Ctor`** | Sets vtable 0x82108F88. |
| `0x825b9f90` | `sub_825B9F90` | **`FM2_CFixedFunctionRendererX360_LoadDeclResources`** | The `"CFixedFunctionRendererX360Decl"` loader. |
| `0x825b9ef8` | `sub_825B9EF8` | **`FM2_CFixedFunctionRendererX360_Dtor`** | Called on failed init before `FreeSmallBlockOrNull`. |
| data `0x82a01570` | `dword_82A01570` | **`g_FM2_GlobalRenderContext_`** | Written by the singleton factory, read by `FM2_Render_GetGlobalRenderContext`. |

## Caveats / follow-ups

- The wider `FM2_Audio*` prefix cluster around `FM2_GraphicsManager_InitRenderersAndTargets`
  (e.g. `FM2_AudioManager_InitDefaultMixParameters`, `FM2_AudioResource_RegisterHook_*`,
  `FM2_AudioMix_SetupRenderTargetTextures`, `FM2_AudioSignalGate_*` timestamps) is
  probably equally misnamed (graphics, not audio) — not yet re-evidenced; rename in a
  later pass, one decompile at a time.
- Manifest (`FM2/fm2_manifest.toml`) still says `FM2_Render_UiOrScreenDrawListSubmit`
  for 0x825B8A60; the IDA-side context now identifies it as
  `COverlayRenderer::SubmitElementDrawList`-like (vtable slot 20). Manifest rename
  deferred (would require codegen regen + hook symbol change mid-experiment).
