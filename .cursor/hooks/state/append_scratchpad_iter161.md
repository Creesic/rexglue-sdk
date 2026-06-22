## Iteration 161

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827335C8 | sub_827335C8 | FM2_Math_ComputeSphericalHarmonicBasis | 0.93 | Evaluates real SH basis polynomials up to order `a2` from direction vector `a3`; uses `1/sqrt(pi)`, `sqrt(3)`, `sqrt(5/7/15/35/105)` constants; 8 xrefs from blur/SH lighting matrix path `sub_827464A8`. |
| 0x82732E90 | sub_82732E90 | FM2_Math_BuildSphericalHarmonicBasisMatrix | 0.91 | Builds SH basis matrix into `a3` with `a3[0]=1.0` and direction terms from `a2[8..10]`; same sqrt/pi polynomial family as basis compute; caller `sub_82745F00`. |
| 0x827328E8 | sub_827328E8 | FM2_Render_BlitAudioMixResolveRegionsPm4 | 0.89 | Iterates resolve rects; `SetPassDrawOverride`, `AudioMix_SubmitPendingOutputBody`, viewport constant PM4, secondary command buffers; called from `FM2_AudioMix_SubmitPendingOutput`. |
| 0x827241C8 | sub_827241C8 | FM2_Render_ExtractPassLightingCoefficientsA | 0.90 | Copies six floats `result[63..68]` into output pointers; used before `ComputeSinCosForPassLighting` in `sub_8275F8A8`. |
| 0x82724238 | sub_82724238 | FM2_Render_ExtractPassLightingCoefficientsB | 0.90 | Copies four floats `result[69..72]` when secondary-coeffs flag set; paired with coeff extractor A in same lighting eval path. |
| 0x8272B918 | sub_8272B918 | FM2_Render_LoadTextureMipLayoutDispatchPtr | 0.88 | Returns `dword_82A41F50`; tested after `XGSetTextureHeader` before `DispatchTextureMipLayoutSetupByType`. |
| 0x827281F0 | sub_827281F0 | FM2_Render_SetMeshVertexLayoutRemapIndex | 0.89 | If remap table at `+124` set, stores `table[a2]` into `+32`; else stores `a2`; called from `render_skinned_model_bind_mesh_layout_from_resource`. |
| 0x82727F80 | sub_82727F80 | FM2_Render_UpdateMeshVertexStreamStride | 0.88 | Stores `a2` at `+17`, updates running stride sum at `+20` from fields `+18/+19`; mesh layout bind path. |
| 0x82725EF0 | sub_82725EF0 | FM2_Math_ReciprocalFloat | 0.92 | Returns `1.0f / x`; used by `render_skinned_model_eval_animation_sample_time_and_frames`. |
| 0x826F6330 | sub_826F6330 | FM2_SQLite_PragmaApplyCacheSizeToPager | 0.90 | PRAGMA `cache_size` handler: loads pager from db+4, calls pager cache setter with parsed value in r4; used from `FM2_SQLite_CodeGen_Pragma`. |
| 0x827130F0 | sub_827130F0 | FM2_SQLite_PagerSetCacheSizeMin10 | 0.91 | Writes `max(a2, 10)` to pager field `+64`; callee of `FM2_SQLite_PragmaApplyCacheSizeToPager`. |
| 0x82724260 | sub_82724260 | FM2_Render_TestPassLightingUseSecondaryCoeffs | 0.87 | Returns `*(a1+24) & 2`; branches lighting path between coeff set A vs B in `sub_8275F8A8`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826F14A8 | sub_826F14A8 | Sets FK action byte on last constraint only (44 bytes); too thin alone. |
| 0x826F2628 | sub_826F2628 | Sets single byte on nested parse object; too thin alone. |
| 0x826F6300 | sub_826F6300 | Thin pager callback setter; callee unknown. |
| 0x826F6360 | sub_826F6360 | Thin wrapper → `Pager_SetJournalModeFlags` only. |
| 0x826F6390 | sub_826F6390 | Thin wrapper (12 bytes). |
| 0x826F6440 | sub_826F6440 | Too trivial alone. |
| 0x826F6458 | sub_826F6458 | Too trivial alone. |
| 0x826F72F0 | sub_826F72F0 | Thin wrapper only. |
| 0x826F7310 | sub_826F7310 | Thin wrapper only. |
| 0x826FA1D0 | sub_826FA1D0 | Reads single byte from mem cell; too trivial alone. |
| 0x826FABB0 | sub_826FABB0 | Thin wrapper (12 bytes). |
| 0x826FABC0 | sub_826FABC0 | Thin wrapper (12 bytes). |
| 0x826FABD0 | sub_826FABD0 | Thin wrapper (12 bytes). |
| 0x826FAC88 | sub_826FAC88 | Single-line wrapper; too thin alone. |
| 0x826FE880 | sub_826FE880 | Too trivial alone. |
| 0x826FE898 | sub_826FE898 | Too trivial alone. |
| 0x826EB8E8 | sub_826EB8E8 | 8-byte db+32 reader; too trivial alone. |
| 0x826EC510 | sub_826EC510 | 12-byte Mem_SetRowid wrapper; too thin alone. |
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x82713520 | sub_82713520 | Pager page-size field accessor; too trivial alone. |
| 0x82716080 | sub_82716080 | Zeros 3 dwords only; too trivial alone. |
| 0x82723B30 | sub_82723B30 | 8-byte wrapper → `ReleaseGpuResourceArray` only. |
| 0x82723C28 | sub_82723C28 | Thin dispatch to memcpy helpers only (24 bytes). |
| 0x827218D8 | sub_827218D8 | Writes dword to indexed array only (16B). |
| 0x827218E8 | sub_827218E8 | Writes dword to nested array only (16B). |
| 0x8270DA40 | sub_8270DA40 | SQLite token helper; needs paired pragma cluster pass. |
| 0x82718B28 | sub_82718B28 | SQLite btree helper; defer pager cluster. |
