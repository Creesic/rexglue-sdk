# 2026-07-13 — fm2mmgrok10 (post Tier A re-enable)

Capture: `renderdoccaps/fm2mmgrok10.rdc` (D3D12, 22 draws). Tier A steps 1–7 all on; still black.

## Verdict
RT325 still **100% black** after last scene draw (EID 277). Every checked draw **SamplesPassed=0**. Tier A did not unblock rasterization.

## Progress vs grok8/9
- Draw 68 (no-POSITION / stride-8 FLOAT16 TEXCOORD pair): **SV_Position.w ≈ 0.92** (was 0). Viewport min/max OK. So `+0x700` + overlays helped this HUD-like path’s clip w.
- Still SamplesPassed=0 with depth **off** → not a depth fail; clip/cull/empty raster or clip-distance edge case. PS has **zero textures**.

## Hard failures still present
1. **Draw 200 (and similar 3D):** VB **stride=8** but IA layout is **32B** (float3@0 + float3@12 + float2@24). POSITION arrives as **UInt** garbage bits. Same MatchDeclaration / device-decl class of bug as grok7–8.
2. **Textures:** draw 68/200 PS `readonly` bindings empty — SetTexture / translate path still not feeding the PS.
3. Object-pass replay surviving startup ≠ pixels; capture does not prove replay is the source of these draws.

## Next fixes (priority)
1. Re-harden `MatchDeclarationForShader` / Resolve for stride-8 FLOAT16_4 POSITION (reject 32B layouts when stride=8).
2. Prove `D3DDevice_SetTexture` → descriptor bind on a scene draw (RDC should show PS SRVs).
3. Only then chase clip-distance / reverse-Z on draws that already have valid SV_Position.
