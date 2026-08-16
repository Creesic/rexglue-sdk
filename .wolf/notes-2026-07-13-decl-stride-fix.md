# 2026-07-13 — Decl stride fix (post-fm2mmgrok10)

## Problem
`fm2mmgrok10.rdc`: stride-8 VB draws still bound a **32B FLOAT3** IA layout (POSITION as UInt garbage). `SamplesPassed=0`, RT black. Same class as grok7/8.

## Root cause
`ResolveVertexDeclaration` used:
`max(g_pipelineState.vertexStrides[0], g_inputSlots[0].stride)`.

Stale `vertexStrides[0]==32` with live `inputSlots[0].stride==8` → Match accepted 32B FLOAT3 while `SetVertexBuffers` used stride 8.

Secondary: `DeclarationFitsStreamStride(..., 0)` returned **true** for all decls; object-pass replay restored guest VB/strides but never re-bound host `SetStreamSource`.

## Fix (landed, fm2 rebuilt)
1. `EffectiveStream0Stride`: prefer `g_inputSlots[0].stride`, then pipeline, then guest `device+0x2FD8 * 4`.
2. `DeclarationFitsStreamStride(stride==0)` → **false**.
3. Match refuses unknown stride; keep FLOAT16_4 preference for stride 8.
4. `ReplayObjPassDraw`: re-`SetStreamSource` / `SetIndices` from restored guest ctx before draw.

## Verify (user: fm2mmgrok11)
- Stride-8 draws → IA FLOAT16_4 (or packed 8B), **not** float3@0+float3@12+float2@24.
- POSITION not UInt garbage; watch `SV_Position.w` / SamplesPassed.
- If still black with good IA → chase empty PS SRVs (`SetTexture`).
