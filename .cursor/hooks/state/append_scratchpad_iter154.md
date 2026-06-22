## Iteration 154

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827308A8 | sub_827308A8 | FM2_Render_AllocSurfaceViaD3DCommands | 0.92 | Flushes dirty D3D state batches then `D3D_AllocateCommandBuffer` for surface (PM4 8450/346298); returns GPU ptr; called from render-target alloc + `sub_82730D60`. |
| 0x82730D60 | sub_82730D60 | FM2_Render_AllocSurfaceAndMemcpyPixels | 0.91 | Wraps `FM2_Render_AllocSurfaceViaD3DCommands` then `FM2_MemcpyAligned` pixel payload; resets command write ptr; 12 callers in audio/render upload paths. |
| 0x8272B630 | sub_8272B630 | FM2_Render_ComputeTextureSliceAllocationSize | 0.90 | Aligns dims via `FM2_SQLite_NextPow2`; format switch rounds to 256B for D3D surface formats; base slice size helper for mip/texture alloc cluster. |
| 0x8272B7C0 | sub_8272B7C0 | FM2_Render_ComputeMipChainTotalAllocationSize | 0.90 | Loops mip levels summing page-aligned sizes via `sub_8272B630`; used by texture/GPU buffer sizing paths. |
| 0x8272B888 | sub_8272B888 | FM2_Render_ComputeMipLevelPageAlignedSize | 0.89 | Single mip level: calls slice size helper then rounds to 4KB page (`0xFFFFF000` mask). |
| 0x8272E0C0 | sub_8272E0C0 | FM2_XexModule_PopNextPendingLoadCallback | 0.90 | When `xex_module_is_load_ready_flag`, decrements pending count at +92 and validates entry via `sub_8272E070`; outputs callback at +4. |
| 0x8272E070 | sub_8272E070 | FM2_XexModule_ValidatePendingLoadQueueEntry | 0.88 | Indexes 14-byte pending-load ring at +36; checks type byte +12 and id dword vs module +100. |
| 0x8272E148 | sub_8272E148 | FM2_XexModule_SyncPendingLoadCountFromModule | 0.89 | When load-ready, copies module pending count from `*a1+28` into load-request +92. |
| 0x8272E3A0 | sub_8272E3A0 | FM2_RenderResource_FinishLoadAndInvokeCallback | 0.91 | On load completion decrements global counters; calls `FM2_RenderResource_ReleaseChildRegistryRefs`; invokes completion callback with status flags. |
| 0x8272DCC0 | sub_8272DCC0 | FM2_RenderResource_ReleaseChildRegistryRefs | 0.90 | Walks child sections; `FM2_RenderResource_LookupRegistryEntryByPath` + release callback `dword_82A42610` on unload. |
| 0x8272E5C8 | sub_8272E5C8 | FM2_RenderResource_ProcessSectionIoCompletion | 0.90 | Decrements per-section IO refcount; on zero copies packed data (`sub_8272DAC8`) or issues next read (`sub_8272E488`); may finish via `sub_8272E3A0`. |
| 0x8272AB90 | sub_8272AB90 | FM2_RenderResource_SetupXgTextureHeaders | 0.93 | Registered as `"texture"` handler at `0x825AD37C`; switch on texture type calls `XGSetTextureHeader`/`XGSetCubeTextureHeader`/`XGSetVolumeTextureHeader`/`XGSetArrayTextureHeader`. |

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
| 0x82722FD8 | sub_82722FD8 | Large draw pass (~1.9KB); defer render draw cluster. |
| 0x82723B30 | sub_82723B30 | 8-byte wrapper → `ReleaseGpuResourceArray` only. |
| 0x82723C28 | sub_82723C28 | Thin dispatch to memcpy helpers only (24 bytes). |
| 0x82725C60 | sub_82725C60 | Cross product math only (64B); thin helper alone. |
| 0x82725D68 | sub_82725D68 | Zeros 3 floats only (24 bytes). |
| 0x82726EA0 | sub_82726EA0 | Pass-state bit test via lookup table (32B); thin alone. |
| 0x82727D30 | sub_82727D30 | Thin pass-lighting init wrapper (76 bytes). |
| 0x82728408 | sub_82728408 | Stores global ptr `dword_82A41D2C` only (16B). |
| 0x82728640 | sub_82728640 | Optional global callback invoke (40 bytes). |
| 0x82728730 | sub_82728730 | 8-byte alpha-test value writer thunk; too thin alone. |
| 0x82729288 | sub_82729288 | Reads single dword at +8 (12 bytes). |
| 0x8272A410 | sub_8272A410 | Large draw-object setup; defer render draw cluster. |
| 0x8272C4B8 | sub_8272C4B8 | Resource-type dispatch switch; defer with loader cluster. |
| 0x8272C538 | sub_8272C538 | Polynomial hash over byte buffer; thin helper alone. |
| 0x8272D958 | sub_8272D958 | TLS heap alloc+memset only (96 bytes). |
| 0x8272DAC8 | sub_8272DAC8 | Packed section memcpy helper; defer with IO completion cluster. |
| 0x8272E488 | sub_8272E488 | Section load finalizer; defer with `sub_8272E7B8` cluster. |
| 0x8272E7B8 | sub_8272E7B8 | Section async-read dispatcher (640B); defer next IO cluster pass. |
| 0x8272EA38 | sub_8272EA38 | Begin section async read wrapper; defer IO cluster. |
| 0x8272EBE0 | sub_8272EBE0 | Texture stream enqueue wrapper; defer IO cluster. |
| 0x82730D40 | sub_82730D40 | Resets D3D command buffer write ptr (24 bytes). |
