## Iteration 147

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82721278 | sub_82721278 | FM2_Render_ApplyCullModeFromContext | 0.90 | State setter for `D3DRS_CULLMODE`: copies dword at +380 from main render TLS context into pass offset table. |
| 0x827213E8 | sub_827213E8 | FM2_Render_ResolvePassStateHandlers | 0.91 | Walks pass shader bytecode (type 32 constant refs, 29 end); resolves offsets via global callbacks and `LookupStateSetterByName`; tail `ResolvePassLightingHandlers`. |
| 0x82721510 | sub_82721510 | FM2_Render_InitDrawPassState | 0.90 | Copies `.data` section constants into pass buffers, memset GPU scratch, optional VB state init, identity matrix via `Math_SetIdentityMatrix4x4`. |
| 0x82721970 | sub_82721970 | FM2_Render_CreateGpuBuffers | 0.92 | Creates vertex decls (`D3DDevice_CreateVertexDeclaration`), GPU mem blocks, `XGSetVertexBufferHeader` / `XGSetIndexBufferHeader` loops. |
| 0x82721AB8 | sub_82721AB8 | FM2_Render_LoadShaderSections | 0.91 | Loads `.data`/`.gpu`/`.gpucached` shader sections then `CreateGpuBuffers`, `ResolvePassStateHandlers`, `InitDrawPassState`. |
| 0x82721D10 | sub_82721D10 | FM2_Render_AllocShaderResourceBlock | 0.90 | Allocates shader resource triple (.data/.gpu/.gpucached) via resource manager callbacks; rollback on failure; falls through to `LoadShaderSections`. |
| 0x82722718 | sub_82722718 | FM2_Render_UploadBoneMatrices | 0.91 | Uploads bone matrix array from pass data via `RenderContext_UploadMatrixConstants`; sets context bone-upload flag + source pointer. |
| 0x827221F0 | sub_827221F0 | FM2_Render_DrawIndexedPrimitive | 0.92 | Binds index buffer then dispatches indexed draw (`sub_827317A0` / `sub_82731C00`) with primitive-type index/count math. |
| 0x82728788 | sub_82728788 | FM2_Render_ResolvePassLightingHandlers | 0.89 | Second pass over shader bytecode: resolves lighting pair heads (type 32) and sampler bindings (type 45) from global tables; state handler array walk. |
| 0x82728738 | sub_82728738 | FM2_Render_LookupAlphaTestSetter | 0.93 | strcmp dispatch on `D3DRS_ALPHATESTENABLE`; returns `sub_82728730` setter writing `dword_829A77A8`. |
| 0x8270FFE8 | sub_8270FFE8 | FM2_SQLite_Parse_LinkDeferredObject | 0.85 | Prepends deferred codegen object onto parse-context chain at +176; callers include `CodeGen_ForeignKeyAction`. |
| 0x82710008 | sub_82710008 | FM2_SQLite_Parse_UnlinkDeferredObject | 0.85 | Unlinks head deferred object from +176 chain; paired with link helper in FK/trigger codegen paths. |

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
| 0x82713110 | sub_82713110 | Pager journal-mode flag setter; defer with pager cluster. |
| 0x82713520 | sub_82713520 | Pager page-size field accessor; too trivial alone. |
| 0x82716080 | sub_82716080 | Zeros 3 dwords only; too trivial alone. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
| 0x82728730 | sub_82728730 | 8-byte alpha-test value writer thunk; too thin alone. |
