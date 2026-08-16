# 2026-07-13 — Restructure gap audit (vs 080plume) + fm2mmgrok9

## User ask
Stop nibble-fixing from RDC; list what the clean plume rebuild dropped that still matters (no tiling / ResizeTileSurface / band destY).

## fm2mmgrok9 snapshot
- Capture: `renderdoccaps/fm2mmgrok9.rdc` — RT325 still 100% black; all checked draws `SamplesPassed=0`.
- **+0x700 constants helped:** draw 49 `SV_Position.w=1`; draw 84 clip pos ~`(0.36,0.35,0.35,0.61)` (not w=0).
- Draw 84: depth OFF, cull NONE, write_mask 15, float16-ish IA, **PS has zero textures bound**.
- So remaining black is not “next tiny decl quirk” alone — larger missing subsystems.

## What clean plume kept
- Unleashed render queue + deferred StretchRect / present override
- Direct D3D hooks + `device+0x700/+0x1700` upload
- Viewport min/max swap for inverted guest Z

## What was deliberately dropped (and still hurts)

### Tier A — ship blockers (do as one tranche)
1. **Constant transport spine** (not full PM4 archaeology): live context files at draw, pass/scene/obj WVP overlays, merge in Flush — 080 `SetLiveFloatConstantFiles` / `MirrorPassVsConstants` / `SetScene3dVsOverlay` / EmitDirty-time snapshot. Prefer Unleashed snapshot-at-flush semantics.
2. **EmitDirty / object-pass replay** (`FM2_D3D_EmitDirtyStateAndDrawList` + restore raster/shader/constants) — or prove 100% of scene is direct-draw (current is not).
3. **`FlushSamplerStates`** — samplerIndices still hardcoded 0.
4. **Decl → spec constants** at bind (`POSITION_F16`, `R11G11B10_NORMAL`, `UNPACK_UBYTE4`, `swappedTexcoords` / blend weights) — ProcSetVertexDeclaration currently stores pointer only.
5. **Flush guards:** force `colorWriteEnable` when PS bound; MSAA PSO `COUNT_1` if host RT is 1x; optional implicit depth attach.

### Tier B — after geometry visible
6. Present RT tracking (`SetScenePresentRT` / last-drawn color) under existing aperture chain.
7. Cull + stencil hooks (SetStencilState exists, unwired; no CULLMODE REX_HOOK).
8. VTE from guest `PA_CL_VTE_CNTL` (hardcoded `vteFlags=8`).

### Skip (cerebrum)
- Tiling / `ResizeTileSurface` / `g_tileViewportOffsetY` / native_renderer / shader_probe / constant_trace as production.

## Recommended next move
Implement **Tier A as one curated port** (logic from 080 Flush/hooks, no C:\temp spam, no tiling), then one RDC — not another single-line MatchDeclaration tweak.
