# 2026-07-13 — Tier A startup crash bisect

## Symptom
`fm2_313` / `fm2_314` die mid-startup after first PSO / StretchRect / texture uploads. No clean log footer.

## Likely causes (Tier A)
1. `REX_HOOK(FM2_D3D_EmitDirtyStateAndDrawList, …)` used HostToGuest marshalling — wrong/fragile ABI vs guest draw-list path.
2. Always-on flush extras: `FlushSamplerStates`, constant overlays / live files, colorWrite + implicit-depth guards, decl→SPEC bits.

## Fix applied (bisect rollback)
- All new Tier A guest hooks → pure `REX_HOOK_RAW` + `g_orig*.fn(ctx, base)` only (EmitDirty included).
- `FlushRenderState` restored to pre-Tier-A shape; **kept** `device+0x700/+0x1700` uploads.
- `ProcSetVertexDeclaration` no longer applies Tier A SPEC metadata.

## Confirmed (2026-07-13 ~13:30)
User: startup runs again after this rollback. Crash was Tier A extras, not the baseline Plume path.

## Re-enable progress
- **Step 1 (2026-07-13 ~13:34):** flush guards only — colorWrite force for PS passes, colorWrite→RT bind, implicit depth fallback, PSO `COUNT_1` + depth disabled when no DS. Samplers/overlays/SPEC/guest hooks still off. Awaiting user retest.
- **Step 2 (2026-07-13 ~13:38):** `FlushSamplerStates` wired back into `FlushRenderState` (before shared CBV upload). Overlays/SPEC/guest hooks still off. Awaiting user retest.
- **Step 3 (2026-07-13 ~13:43):** `ApplyVertexDeclarationMetadata` on bind + flush (SPEC_CONSTANT_POSITION_F16 / R11G11B10 / UBYTE4 + swizzle metadata). Overlays/guest hooks still off. Awaiting user retest.
- **Step 4 (2026-07-13 ~13:46):** constant live-file + pass/scene/obj overlay merge in `FlushRenderState`. Feeders still off (inert until steps 5–6). Awaiting user retest.
- **Step 5 (2026-07-13 ~13:49):** EmitDirty `REX_HOOK_RAW` + scene3d WVP overlay (regs 15..18) after original. No HostToGuest; no record/replay. Awaiting user retest.
- **Step 6 (2026-07-13 ~13:51):** UploadMatrix → MirrorPass + obj WVP capture; SetPending → scene3d overlay; draws set `SetLastDrawCallerLr`. Record/replay still off. Awaiting user retest.
- **Step 7 (2026-07-13 ~14:08):** `kObjPassRecordReplay=true` — Begin/Finalize/Clone capture, indexed-draw snapshots, EmitDirty scribble+replay, SetPending flush snapshots, traversal dirty from UploadMatrix. Placeholder PS off while replay on. **Highest crash risk — await user retest.**

## Next
2. FlushSamplerStates
3. Decl → SPEC bits
4. Constant overlays / live files
5. EmitDirty as `REX_HOOK_RAW` post-hook only
6. UploadMatrix / SetPending RAW post-hooks
7. Object-pass record/replay last
