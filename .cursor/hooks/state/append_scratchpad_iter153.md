## Iteration 153

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8272A7C0 | sub_8272A7C0 | FM2_Render_AllocRenderTargetSurfaceDescriptor | 0.91 | Maps surface format to bpp multiplier (1/2/3/4); clamps to 250000 pixels; allocates via `sub_827308A8`; stores dims in descriptor. |
| 0x8272A8E0 | sub_8272A8E0 | FM2_Render_DecrementRenderTargetMipLevel | 0.89 | Decrements mip counters at +16/+28 then reissues surface alloc with reduced height; caller render-target draw path. |
| 0x8272A978 | sub_8272A978 | FM2_Render_UploadPassCompositeMatrixConstants | 0.90 | VMX multiply `unk_82A41ED0`×`unk_82A41F10` then variant multiply; uploads matrix constants slot 0. |
| 0x8272A9E0 | sub_8272A9E0 | FM2_Render_SetupPassVertexShaderByIndex | 0.91 | Lazily creates GPU mem block + vertex decl per pass index; sets VS/decl; toggles viewport; may call composite matrix upload. |
| 0x8272AA98 | sub_8272AA98 | FM2_Render_SetPassPixelShaderByIndex | 0.90 | Lazily allocates pixel-shader GPU block from `off_829A7888`; `SetPixelShaderState` for pass index. |
| 0x8272D660 | sub_8272D660 | FM2_RenderResource_LookupTypeByName | 0.93 | strcmp scan of `dword_82A42328` table; registered names include `rendergraph`, `bfont`, `vfont`, `texture`, `shader`, `animation`. |
| 0x8272D6D0 | sub_8272D6D0 | FM2_RenderResource_RegisterTypeHandler | 0.92 | Appends name + 3 callbacks into parallel tables `82A42328/42428/424A8`; caller `sub_825AD2C0` init. |
| 0x8272D710 | sub_8272D710 | FM2_RenderResource_DispatchLoadByTypeName | 0.91 | Looks up type by name then invokes load callback; returns 0/1/2 status; used in XTS transport load loop. |
| 0x8272D800 | sub_8272D800 | FM2_RenderResource_LookupRegistryEntryByPath | 0.90 | strcmp walk of 24-byte registry records `unk_82A42540`; returns matching entry pointer. |
| 0x8272D890 | sub_8272D890 | FM2_RenderResource_RegisterOrUpdateRegistryEntry | 0.90 | Finds or appends registry entry for resource path; stores 3 dwords; copies record to `unk_82A42600` when path is `.data`. |
| 0x8272E010 | sub_8272E010 | FM2_XexModule_GetPendingLoadCount | 0.89 | Returns module pending-load count at +24 when `xex_module_is_load_ready_flag`; decrements by 1 when flag at +25 set. |
| 0x82723750 | sub_82723750 | FM2_Render_ExecuteBoundDrawPass | 0.90 | dcbt prefetch pass data; `BindPassStateToContext`; optional global callback; tail large draw pass `sub_82722FD8`. |

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
| 0x82723B30 | sub_82723B30 | 8-byte wrapper → `ReleaseGpuResourceArray` only. |
| 0x82723C28 | sub_82723C28 | Thin dispatch to memcpy helpers only (24 bytes). |
| 0x82725D68 | sub_82725D68 | Zeros 3 floats only (24 bytes). |
| 0x82727D30 | sub_82727D30 | Thin pass-lighting init wrapper (76 bytes). |
| 0x82728640 | sub_82728640 | Optional global callback invoke (40 bytes). |
| 0x82728730 | sub_82728730 | 8-byte alpha-test value writer thunk; too thin alone. |
| 0x82729288 | sub_82729288 | Reads single dword at +8 (12 bytes). |
| 0x8272A410 | sub_8272A410 | Large draw-object setup; defer render draw cluster. |
| 0x8272C4B8 | sub_8272C4B8 | Resource-type dispatch switch; defer with loader cluster. |
| 0x8272C538 | sub_8272C538 | Polynomial hash over byte buffer; thin helper alone. |
| 0x8272D958 | sub_8272D958 | TLS heap alloc+memset only (96 bytes). |
| 0x8272EBE0 | sub_8272EBE0 | Texture stream enqueue wrapper; defer IO cluster. |
| 0x82730D40 | sub_82730D40 | Resets D3D command buffer write ptr (24 bytes). |
