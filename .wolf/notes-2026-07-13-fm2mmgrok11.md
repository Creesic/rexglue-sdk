# 2026-07-13 — fm2mmgrok11 (post decl-stride fix)

Capture: `renderdoccaps/fm2mmgrok11.rdc` (D3D12, 13 draws).

## Verdict
**Decl fix worked** for this capture. Still **100% black** (RT325 near_black_ratio=1.0); checked draws **SamplesPassed=0**.

## Decl / IA (was the grok10 failure)
All scene draws: VB **stride=8**, IA =
- TEXCOORD0: R16G16_FLOAT @0
- TEXCOORD1: R16G16_FLOAT @4
- rest slot-15 dummies

**Not** the old 32B `float3@0 + float3@12 + float2@24`. Mesh decode shows sane float16 values (e.g. `(0.81, -0.10)`).

## What this capture is / isn't
- Every draw uses the **same** PSO/VS/PS (`784`/`785`) — TEXCOORD-only signature (**no POSITION**). HUD/UI-like class (same as grok10 draw 68).
- **No** 3D FLOAT16_4 POSITION mesh draws in this RDC → cannot confirm that branch of Match yet.
- PS reflection has **zero** texture bindings; pipeline PS readonly empty (expected for this shader, not proof SetTexture is fixed).

## Why still 0 samples (despite good w)
Draw 54 V0 `SV_Position≈(0.87,0.86,0.87,0.92)` (w OK). But verts collapse: **x≈y≈z** tracking TEXCOORD0.x; first tri NDC clusters near (1,1) → near-degenerate. Depth off, NoCull, clipdist=0 — not depth-fail. Suspect **bad VS constants / transform**, not IA.

## Next
1. Later-frame capture with real 3D meshes (POSITION + textures) to prove FLOAT16_4 path + SetTexture SRVs.
2. Or chase constant upload for this TEXCOORD UI VS (why xyz collapse / 0 samples).
