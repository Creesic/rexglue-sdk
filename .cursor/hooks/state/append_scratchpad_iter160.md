## Iteration 160

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827317A0 | sub_827317A0 | FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset | 0.92 | Flushes pending D3D state bits then loops emitting PM4 type `8450` packets with GPU vertex offset (`a4`), index-type `a2`, count `a5`; called from `FM2_Render_DispatchPm4DrawOpcode`, `DrawIndexedPrimitive`, `WalkAndDispatchPm4DrawList`. |
| 0x827313B0 | sub_827313B0 | FM2_D3D_EmitIndexedDrawPm4Packets | 0.91 | Same indexed-draw PM4 emit path without separate GPU-offset arg; called from `render_pass_execute_draw_batch_with_state_save`. |
| 0x82731C00 | sub_82731C00 | FM2_D3D_EmitIndexedDrawPm4WithVertexFormatSetup | 0.90 | Preloads vertex-format tables (`unk_82120148`/`unk_82120234`) into context +10504 before indexed-draw PM4 emit; sole caller `FM2_Render_DrawIndexedPrimitive`. |
| 0x8272AB00 | sub_8272AB00 | FM2_Render_Pm4CopyMatrixToSlotF10_UploadComposite | 0.89 | Copies 8 qwords from PM4 arg to `unk_82A41F10` then `FM2_Render_UploadPassCompositeMatrixConstants`. |
| 0x8272AB30 | sub_8272AB30 | FM2_Render_Pm4CopyMatrixToSlotEd0_UploadComposite | 0.89 | Same pattern targeting `unk_82A41ED0` (composite matrix operand B). |
| 0x8272AB60 | sub_8272AB60 | FM2_Render_Pm4CopyMatrixToSlotE90_UploadComposite | 0.89 | Same pattern targeting `unk_82A41E90` (composite matrix operand A). |
| 0x82732E28 | sub_82732E28 | FM2_Math_ScaleFloatBufferByCountSquared | 0.91 | Scales `a2*a2` floats: `dst[i] = src[i] * scale`; 21 xrefs from blur/lighting matrix code in `sub_827464A8`. |
| 0x82732E58 | sub_82732E58 | FM2_Math_AddFloatBuffersByCountSquared | 0.91 | Accumulates `a2*a2` floats: `dst[i] += src[i]`; paired with scale helper in same caller cluster. |
| 0x82725D00 | sub_82725D00 | FM2_Math_SubtractVector3 | 0.92 | Component-wise 3-vector subtract `a-b` into output; pairs with `FM2_Math_AddVector3` callers. |
| 0x8272E370 | sub_8272E370 | FM2_RenderResource_RegisterSectionLoaderCallbacksOnce | 0.90 | If `dword_82A42630==0`, stores five section-loader callbacks (`dword_82A4261C`–`2C`); called during CAFF type registration (`rendergraph`, etc.) at `sub_825AD2C0`. |
| 0x8272A968 | sub_8272A968 | FM2_Render_ResetGlobalD3DCommandBufferWritePtr | 0.88 | Thin wrapper calling `FM2_Render_ResetD3DCommandBufferWritePtr(dword_82A41BEC)`. |
| 0x82732368 | sub_82732368 | FM2_Render_EmitViewportAndConstantPm4Batch | 0.88 | Viewport/constant PM4 upload via `GpuKick_SubmitViewportConstantPm4`, `RenderContext_UploadConstantBlock`, `ApplyViewportConstants`; used by `audio_render_frame_upload_color_constant_block`. |

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
| 0x82725EF0 | sub_82725EF0 | Float reciprocal only (16 bytes). |
| 0x827218D8 | sub_827218D8 | Writes dword to indexed array only (16B). |
| 0x827218E8 | sub_827218E8 | Writes dword to nested array only (16B). |
| 0x827241C8 | sub_827241C8 | Extracts 6 pass lighting floats; defer lighting cluster. |
| 0x8272B918 | sub_8272B918 | Returns `dword_82A41F50` only (12B); defer texture layout cluster. |
| 0x827328E8 | sub_827328E8 | Large audio/viewport PM4 batch (~1.2KB); needs dedicated pass. |
| 0x827335C8 | sub_827335C8 | Large blur matrix kernel (~1.3KB); defer with `sub_827464A8` cluster. |
| 0x82732E90 | sub_82732E90 | Large matrix helper (~1.8KB); defer blur cluster. |
