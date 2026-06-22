## Iteration 150

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8272CCE0 | sub_8272CCE0 | FM2_RenderResource_LoadShaderScratchPoolCached | 0.90 | Pool-type-0 scratch loader: binds `.data` sections, `InitShaderScratchTreeCached` + per-element cached init; caller `InitPassScratchBufferFromShaderPool`. |
| 0x8272CEE0 | sub_8272CEE0 | FM2_RenderResource_LoadShaderScratchPoolGpu | 0.90 | Pool-type-1 scratch loader: same `.data` bind loop but `InitShaderScratchGpuBlock` path; paired caller at pool +28==1. |
| 0x8272C5F8 | sub_8272C5F8 | FM2_RenderResource_AdvanceShaderScratchCursor | 0.89 | Computes per-subshader scratch offset from version `04.05.05.0032` and `.data` section sizes; stores cursor at a2+8. |
| 0x8272C968 | sub_8272C968 | FM2_RenderResource_InitShaderScratchRootBlock | 0.91 | Lays out root shader scratch block: identity matrices, pointer chains for constants/samplers/children at base+a2. |
| 0x8272CB00 | sub_8272CB00 | FM2_RenderResource_InitShaderScratchCachedBlock | 0.90 | Initializes cached-layout scratch element (+48 header, memset child arena); caller `InitShaderScratchElementCachedTree`. |
| 0x8272C6B0 | sub_8272C6B0 | FM2_RenderResource_InitShaderScratchGpuBlock | 0.90 | Initializes GPU-layout scratch block (+96 header, identity matrix, dispatches pass tables via `sub_8272C580`). |
| 0x82728FB0 | sub_82728FB0 | FM2_Render_RecreateGpuResourcesFromDescriptor | 0.91 | Recreates vertex decls, GPU mem block headers from descriptor; parallel to `CreateGpuBuffers`; caller `UpdateGpuBufferHeaders`. |
| 0x82723AD0 | sub_82723AD0 | FM2_Render_ValidateShaderVersionAndUpdateBuffers | 0.93 | `strncmp` gate on shader version `30.06.06.0036`; on match calls `UpdateGpuBufferHeaders` on descriptor +252. |
| 0x82729088 | sub_82729088 | FM2_Render_InitGpuMemoryBlocksFromDescriptor | 0.90 | Loops GPU resource entries: `sub_8236DCD8` + `D3D_InitGpuMemoryBlockHeader` per 16-byte record. |
| 0x82729298 | sub_82729298 | FM2_Render_ScanShaderSymbolByName | 0.89 | Linear strcmp scan of 12-byte symbol table with ordinal match; fallback from `LookupShaderSymbolByName`. |
| 0x82729200 | sub_82729200 | FM2_Render_CompareShaderSymbolSortKey | 0.91 | `bsearch` comparator: primary dword then secondary dword; data xref from `LookupShaderSymbolByName`. |
| 0x8272CA98 | sub_8272CA98 | FM2_RenderResource_InitShaderScratchTreeCached | 0.89 | Init root cached scratch then recurse child shaders via `InitShaderScratchGpuBlock`; head of pool-cached loader. |

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
| 0x82725D68 | sub_82725D68 | Zeros 3 floats only (24 bytes). |
| 0x82728640 | sub_82728640 | Optional global callback invoke (40 bytes). |
| 0x82728730 | sub_82728730 | 8-byte alpha-test value writer thunk; too thin alone. |
| 0x82729228 | sub_82729228 | Returns dword[0] count only (24 bytes). |
| 0x8272C888 | sub_8272C888 | GPU scratch element bind; defer with scratch-element cluster. |
| 0x8272CC38 | sub_8272CC38 | Cached scratch element tree walk; defer next iteration. |
