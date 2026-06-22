## Iteration 148

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82713110 | sub_82713110 | FM2_SQLite_Pager_SetJournalModeFlags | 0.87 | Sets pager bytes +7/+8/+9/+14 from journal mode (1=DELETE, 3=PERSIST); caller `CodeGen_Pragma` via `sub_826F6360`. |
| 0x82713538 | sub_82713538 | FM2_SQLite_Pager_InitPageBufferAndVfsRef | 0.88 | `memset` page buffer then `Vfs_AddRef` on pager VFS when not readonly (+17); caller `Btree_OpenDatabaseFile`. |
| 0x8272D308 | sub_8272D308 | FM2_RenderResource_LookupSectionByName | 0.91 | strcmp walk of resource section table; returns section record or -1; used by shader `.data`/`.gpu`/`.gpucached` loads. |
| 0x8272D390 | sub_8272D390 | FM2_RenderResource_BindSectionData | 0.90 | Looks up section by name, stores data pointer at +20, calls `MapSectionData`; used from `LoadShaderSections`. |
| 0x8272D420 | sub_8272D420 | FM2_RenderResource_ApplySectionPatches | 0.89 | Applies pending 16-byte relocation patch list (section+offset pairs) after section bind; tail of `LoadShaderSections`. |
| 0x8272D488 | sub_8272D488 | FM2_RenderResource_MapSectionData | 0.88 | Alloc/maps section memory via callback table; applies inline reloc patches when present; callee of `BindSectionData`. |
| 0x82721F68 | sub_82721F68 | FM2_Render_ApplyPassMatrixWithBoneUpload | 0.90 | Copies pass matrix, VMX multiply into TLS contexts, batch-submits draw packets, uploads bone matrix constants, tail `UploadPassMatrices`. |
| 0x82722418 | sub_82722418 | FM2_Render_BindPassVertexStreamsWithConstants | 0.91 | Resolves per-stream VB bindings from pass matrix table, `BindVertexStream` loop, uploads matrix constant block from scratch. |
| 0x827225A0 | sub_827225A0 | FM2_Render_SetupPassShaderAndVertexStreams | 0.91 | Same stream-binding path as `BindPassVertexStreamsWithConstants` plus `SetActivePassId` and `SetVertexShaderState`. |
| 0x82721370 | sub_82721370 | FM2_Render_ApplyShaderConstantOffset | 0.90 | Writes float offset into render TLS slot +76/+77/+78 based on type byte at +8 (0/1/2). |
| 0x82721208 | sub_82721208 | FM2_Render_PushShaderConstantSnapshot | 0.89 | Copies 144-byte shader-constant snapshot into global ring buffer `unk_82A3D010`; increments `dword_82A419A0`. |
| 0x82728478 | sub_82728478 | FM2_Render_CreateDefaultLookupTextures | 0.92 | One-time init: creates three 4×4 `D3DFMT_A8R8G8B8` textures (magenta/zero/white fill) via `D3DDevice_CreateTexture` + `TileSurface`. |

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
| 0x82721260 | sub_82721260 | Pop snapshot counter only (20 bytes); pair with push helper deferred. |
| 0x827216D8 | sub_827216D8 | Generic shader-resource free callback; thin destructor glue. |
| 0x82722108 | sub_82722108 | Bone-matrix upload variant; defer with draw-pass cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
| 0x82723750 | sub_82723750 | Pass-state bind thunk; calls large `sub_82722FD8`. |
| 0x82723948 | sub_82723948 | TLS context init; defer with render bootstrap cluster. |
| 0x82723B38 | sub_82723B38 | Scratch size helper only (84 bytes). |
| 0x82723B90 | sub_82723B90 | Pass scratch init; defer with shader-pool cluster. |
| 0x82728730 | sub_82728730 | 8-byte alpha-test value writer thunk; too thin alone. |
| 0x82729228 | sub_82729228 | Returns dword[0] count only (24 bytes). |
| 0x82729240 | sub_82729240 | Index lookup in 12-byte table; thin but may name next iteration. |
