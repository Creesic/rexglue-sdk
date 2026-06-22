## Iteration 151

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8272C888 | sub_8272C888 | FM2_RenderResource_BindShaderScratchGpuElement | 0.90 | Binds GPU scratch element at pool offset; sets `dword_82A41F70/F6C`; tail `InitShaderScratchPassTableBytes` + child scratch init. |
| 0x8272CC38 | sub_8272CC38 | FM2_RenderResource_InitShaderScratchCachedElementTree | 0.90 | `InitShaderScratchCachedBlock` then recurse children via `BindShaderScratchGpuElement`; per-element path in pool-cached loader. |
| 0x8272C7B0 | sub_8272C7B0 | FM2_RenderResource_InitShaderScratchPassTableBytes | 0.89 | Zero-fills pass-table byte arena at scratch offset; walks 8 pass slots recursively when walk-skip flag clear. |
| 0x8272C580 | sub_8272C580 | FM2_RenderResource_WalkShaderPassTableRecursive | 0.88 | Recurses 8 pass-index slots (+28 stride) when walk-skip flag clear; called from `InitShaderScratchGpuBlock`. |
| 0x8272D2A8 | sub_8272D2A8 | FM2_RenderResource_InitChildShaderScratchBlock | 0.89 | Version gate `04.05.05.0032` then delegates to `InitShaderSamplerPassScratch`; nested shader scratch tail. |
| 0x8272D130 | sub_8272D130 | FM2_RenderResource_InitShaderSamplerPassScratch | 0.90 | Lays out child-shader sampler/pass scratch nodes (+44/+20 strides) and initializes linked lists at scratch base. |
| 0x8272CE18 | sub_8272CE18 | FM2_RenderResource_TestShaderWalkSkipFlag | 0.91 | Returns `*a1 & 0x40000000`; when set, pass-table walks in C580/C7B0 are skipped. |
| 0x8272CE28 | sub_8272CE28 | FM2_RenderResource_GetShaderResourceIndexBase | 0.90 | Returns `*a1 & 0x3FFFFFFF`; used as scratch offset base in pass-table byte init. |
| 0x827251B8 | sub_827251B8 | FM2_Math_BuildScaleTranslateMatrix4x4 | 0.92 | Builds 4×4 matrix: diagonal 1.0, translation at [12..14] from args; many animation/render callers. |
| 0x82725DB8 | sub_82725DB8 | FM2_Math_NormalizeVector3VMX | 0.93 | VMX `vrsqrtefp`/`vnmsubfp` vector normalize; stores 3 floats to output; multiple gameplay callers. |
| 0x82725E58 | sub_82725E58 | FM2_Math_MultiplyMatrix3x3ByVector3 | 0.91 | Multiplies 3×3 portion of 4×4 matrix (rows 0-2) by input vector; writes result vector to a3. |
| 0x827290F8 | sub_827290F8 | FM2_Render_ApplyPrimitiveTypeGpuState | 0.90 | Dispatches on primitive-type enum (0/1/3): sets sampler nibbles and index-buffer mode via render context. |

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
| 0x82723C28 | sub_82723C28 | Thin dispatch to memcpy helpers only (24 bytes). |
| 0x82725D68 | sub_82725D68 | Zeros 3 floats only (24 bytes). |
| 0x82727FA0 | sub_82727FA0 | Pass bone-matrix swizzle; defer dedicated animation pass. |
| 0x827282B0 | sub_827282B0 | Pass lighting slot counter; needs more caller context. |
| 0x827283A0 | sub_827283A0 | Thin symbol lookup wrapper (88 bytes). |
| 0x82728640 | sub_82728640 | Optional global callback invoke (40 bytes). |
| 0x82728730 | sub_82728730 | 8-byte alpha-test value writer thunk; too thin alone. |
| 0x827291A0 | sub_827291A0 | D3D resource release loop; defer resource teardown cluster. |
| 0x82729228 | sub_82729228 | Returns dword[0] count only (24 bytes). |
| 0x82729288 | sub_82729288 | Reads single dword at +8 (12 bytes). |
| 0x827294C8 | sub_827294C8 | Thin wrapper → `LookupShaderSymbolByName` with field=1. |
| 0x82729920 | sub_82729920 | Gaussian kernel helper; defer animation/math cluster. |
