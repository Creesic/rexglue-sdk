## Iteration 152

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82727FA0 | sub_82727FA0 | FM2_Render_ApplySkinnedModelBoneMatrixSwizzles | 0.91 | Swaps/negates bone matrix rows per pass index table; special index 61166; caller `render_skinned_model_dispatch_animation_callbacks` when result==1. |
| 0x827291A0 | sub_827291A0 | FM2_Render_ReleaseGpuResourceArray | 0.92 | Loops `D3DResource_Release` over resource pointer array at +24/+32; caller shader teardown thunk at `0x82723B30`. |
| 0x827282B0 | sub_827282B0 | FM2_Render_CountPassLightingSlots | 0.88 | Returns max lighting slot index +1 via `ComputePassLightingSlotOffset64B` scan or cached ushort table at +124. |
| 0x827283A0 | sub_827283A0 | FM2_Render_ResolveShaderSymbolOffset | 0.90 | `LookupShaderSymbolByAltField` then writes symbol data offset dword; caller builds `TEXANIM_%s.%s` name. |
| 0x82728690 | sub_82728690 | FM2_Render_ApplyRenderStateCallbackBlock | 0.89 | Sets render-state mode bytes +32/+33; invokes 16-byte callback table; memset 0x50 state block `unk_82A41D38`. |
| 0x82729228 | sub_82729228 | FM2_Render_GetShaderConstantTableCount | 0.87 | Returns `*table` entry count or 0; paired with `LookupShaderConstantEntryByIndex` in pass-state resolve. |
| 0x827237E8 | sub_827237E8 | FM2_Render_BindPassVertexStreamBySlot | 0.90 | Resolves VB handle from pass slot byte and calls `RenderContext_BindVertexStream` with stream index mask. |
| 0x827294C8 | sub_827294C8 | FM2_Render_LookupShaderSymbolByAltField | 0.91 | Thin wrapper calling `LookupShaderSymbolByName` with alternate field selector a4=1. |
| 0x8272C4F8 | sub_8272C4F8 | FM2_Render_HashShaderSymbolName | 0.90 | Polynomial rolling hash over symbol name bytes; bsearch key builder for `LookupShaderSymbolByName`. |
| 0x8272D018 | sub_8272D018 | FM2_RenderResource_InitShaderChildSamplerNodes | 0.89 | Initializes child-shader sampler/pass nodes (+20 stride) at scratch offsets; version gate `04.05.05.0032`. |
| 0x82729920 | sub_82729920 | FM2_Math_ComputeGaussianKernelWeight | 0.91 | Gaussian weight: `cos(-(x²+y²)/(2σ²)) / sin(σ²·2π)`; many animation kernel callers. |
| 0x827216D8 | sub_827216D8 | FM2_Render_FreeShaderResourceBlock | 0.90 | Frees `.gpucached` (+24), `.gpu` (+20) via callbacks then main block; caller presentation slot clear path. |

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
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
| 0x82723750 | sub_82723750 | Pass-state bind thunk; calls large `sub_82722FD8`. |
| 0x82723B30 | sub_82723B30 | 8-byte wrapper → `ReleaseGpuResourceArray` only. |
| 0x82723C28 | sub_82723C28 | Thin dispatch to memcpy helpers only (24 bytes). |
| 0x82725D68 | sub_82725D68 | Zeros 3 floats only (24 bytes). |
| 0x82728640 | sub_82728640 | Optional global callback invoke (40 bytes). |
| 0x82728730 | sub_82728730 | 8-byte alpha-test value writer thunk; too thin alone. |
| 0x82729288 | sub_82729288 | Reads single dword at +8 (12 bytes). |
| 0x8272A410 | sub_8272A410 | Large draw-object setup; defer render draw cluster. |
| 0x8272A7C0 | sub_8272A7C0 | Render-target surface alloc; defer D3D surface cluster. |
| 0x8272D660 | sub_8272D660 | Command-name table lookup; string table not resolved in IDA. |
| 0x8272D710 | sub_8272D710 | Command dispatch wrapper; depends on unresolved D660 table. |
| 0x8272D800 | sub_8272D800 | Resource path lookup; defer debug/resource registry cluster. |
| 0x8272E010 | sub_8272E010 | XEX module load counter; thin helper needs more context. |
