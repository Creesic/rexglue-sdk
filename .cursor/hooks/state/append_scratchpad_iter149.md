## Iteration 149

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82721260 | sub_82721260 | FM2_Render_PopShaderConstantSnapshot | 0.90 | Decrements `dword_82A419A0` shader-constant ring index; paired with `PushShaderConstantSnapshot`; callers in draw-pass tail. |
| 0x82723948 | sub_82723948 | FM2_Render_InitTlsContextDefaults | 0.91 | Bootstraps main/worker TLS render contexts (identity matrix, pass callback `sub_82723750`, scale defaults); tail `CreateDefaultLookupTextures`. |
| 0x82723B38 | sub_82723B38 | FM2_RenderResource_GetShaderScratchSize | 0.88 | Returns scratch size from shader pool type at +28: pool 0 → `ComputeShaderBlockSize`+32, pool 1 → +16; caller `AllocShaderResourceBlock`. |
| 0x82723B90 | sub_82723B90 | FM2_Render_InitPassScratchBufferFromShaderPool | 0.89 | Initializes pass variable-length scratch from shader pool type 0/1; dispatches `sub_8272CCE0` or `sub_8272CEE0`; caller `InitDrawPassState`. |
| 0x82722108 | sub_82722108 | FM2_Render_UploadContextBoneMatricesAndPass | 0.90 | Pops shader snapshot, batch-submits draw packets, uploads bone matrix constants from TLS context, tail `UploadPassMatrices`. |
| 0x82729240 | sub_82729240 | FM2_Render_LookupShaderConstantEntryByIndex | 0.88 | Bounds-checked index into 12-byte shader-constant table; callers `ResolvePassStateHandlers` / `ResolvePassLightingHandlers`. |
| 0x8272CE38 | sub_8272CE38 | FM2_RenderResource_ComputeShaderBlockSize | 0.91 | Computes total shader block size from version string `04.05.05.0032` and `.data` section sizes via `GetSectionDataSize`. |
| 0x82723A18 | sub_82723A18 | FM2_Render_UpdateGpuBufferHeaders | 0.92 | Re-applies `XGSetVertexBufferHeader` / `XGSetIndexBufferHeader` loops over pass GPU descriptor; caller version gate `30.06.06.0036`. |
| 0x8272D3E8 | sub_8272D3E8 | FM2_RenderResource_GetSectionDataSize | 0.89 | Looks up section by name then returns `ResourceManager_GetPendingRecordThreadContext` size; used in block-size math. |
| 0x82721EE0 | sub_82721EE0 | FM2_Render_ApplyIndexedPassMatrixUpload | 0.90 | Loads indexed pass matrix block via VMX multiply helper, accumulates into draw-packet batch, tail `UploadPassMatrices`. |
| 0x82727DE8 | sub_82727DE8 | FM2_Render_MultiplyMatrix4x4TransposeVMX | 0.92 | VMX `lvx128`/`vmsum3fp128` 4×4 matrix multiply-transpose; sole caller `ApplyIndexedPassMatrixUpload`. |
| 0x82729368 | sub_82729368 | FM2_Render_LookupShaderSymbolByName | 0.90 | `bsearch` on sorted 12-byte symbol table then strcmp on name field; ordinal match returns symbol record; fallback linear scan. |

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
| 0x827216D8 | sub_827216D8 | Generic shader-resource free callback; thin destructor glue. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
| 0x82723750 | sub_82723750 | Pass-state bind thunk; calls large `sub_82722FD8`. |
| 0x827237E8 | sub_827237E8 | Thin `BindVertexStream` wrapper (80 bytes). |
| 0x82723AD0 | sub_82723AD0 | Shader version gate only; defer with resource cluster. |
| 0x82725D68 | sub_82725D68 | Zeros 3 floats only (24 bytes). |
| 0x82728640 | sub_82728640 | Optional global callback invoke (40 bytes). |
| 0x82728730 | sub_82728730 | 8-byte alpha-test value writer thunk; too thin alone. |
| 0x82728FB0 | sub_82728FB0 | Recreate GPU resources; defer with shader-pool load cluster. |
| 0x82729088 | sub_82729088 | GPU mem block init loop; defer with resource cluster. |
| 0x82729228 | sub_82729228 | Returns dword[0] count only (24 bytes). |
| 0x82729298 | sub_82729298 | Linear symbol scan fallback; defer with symbol table cluster. |
| 0x8272CCE0 | sub_8272CCE0 | Shader pool-type-0 section loader; defer dedicated pass. |
| 0x8272CEE0 | sub_8272CEE0 | Shader pool-type-1 section loader; defer dedicated pass. |
