## Iteration 155

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8272E7B8 | sub_8272E7B8 | FM2_RenderResource_DispatchSectionAsyncReads | 0.91 | Validates CAFF header + parses sections; counts pending section reads; issues async IO via `dword_82A42620/424` into `FM2_RenderResource_ProcessSectionIoCompletion`. |
| 0x8272E488 | sub_8272E488 | FM2_RenderResource_CompleteAllSectionsLoad | 0.90 | Decrements section IO refcount; on zero runs post-process, registry upload, syncs pending count, invokes completion callback. |
| 0x8272DAC8 | sub_8272DAC8 | FM2_RenderResource_CopyPackedSectionDataFromBuffer | 0.90 | Walks child sections; `FM2_MemcpyAligned` from packed IO buffer into section dest offsets with alignment padding. |
| 0x8272EA38 | sub_8272EA38 | FM2_RenderResource_StartBufferedSectionLoad | 0.89 | Page-aligns read cursor; calls `dword_82A42620` then `FM2_RenderResource_DispatchSectionAsyncReads`; sync callback path at `a1+4`. |
| 0x8272EB00 | sub_8272EB00 | FM2_RenderResource_SubmitTextureStreamLoadRequest | 0.90 | Allocates load request via `sub_8272D9B8`; increments global counter; dispatches buffered load or async stream callback. |
| 0x8272E2C0 | sub_8272E2C0 | FM2_RenderResource_GetRelocationEntryOffset | 0.88 | Bounds-checks reloc index vs container +28; returns dword from 14-byte reloc table at `a1[9]`. |
| 0x8272F310 | sub_8272F310 | FM2_RenderResource_ApplyRelocationFixups | 0.90 | For reloc pairs adds base offsets into target pointers when below section bound; used after byteswap pass. |
| 0x8272F1F8 | sub_8272F1F8 | FM2_RenderResource_ByteswapPackedRelocationData | 0.89 | When presentation mode off, endian-swaps packed reloc entries via `render_byteswap_value_by_element_width`. |
| 0x8272F3D0 | sub_8272F3D0 | FM2_RenderResource_ValidateCaffContainerHeader | 0.93 | strcmp magic `CAFF` + version `21.11.05.0034`; verifies checksum via `sub_8272C538` over 0x190 bytes. |
| 0x8272DDB8 | sub_8272DDB8 | FM2_RenderResource_ParseContainerAndAllocSections | 0.91 | Endian-swaps CAFF header/sections; allocates each section via registry alloc callback; sets `.data` section index. |
| 0x8272F498 | sub_8272F498 | FM2_RenderResource_PostProcessLoadedContainer | 0.90 | `FM2_Render_InitShaderConstantTables`; optional byteswap + reloc fixup passes; finalizes loaded container. |
| 0x8272DBB0 | sub_8272DBB0 | FM2_RenderResource_UploadSectionDataViaRegistry | 0.90 | Per-section registry lookup; special-case `.data` size; invokes registry write callback at +20. |

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
| 0x82726E58 | sub_82726E58 | Sets pass-state bit via lookup table (36B); thin alone. |
| 0x82726EA0 | sub_82726EA0 | Tests pass-state bit via lookup table (32B); thin alone. |
| 0x82727D30 | sub_82727D30 | Thin pass-lighting init wrapper (76 bytes). |
| 0x82728408 | sub_82728408 | Stores global ptr `dword_82A41D2C` only (16B). |
| 0x82728640 | sub_82728640 | Optional global callback invoke (40 bytes). |
| 0x82728730 | sub_82728730 | 8-byte alpha-test value writer thunk; too thin alone. |
| 0x82729288 | sub_82729288 | Reads single dword at +8 (12 bytes). |
| 0x8272A410 | sub_8272A410 | Large draw-object setup; defer render draw cluster. |
| 0x8272AE90 | sub_8272AE90 | Resolves texture/base offset by type (40B); thin alone. |
| 0x8272B760 | sub_8272B760 | Looks up D3D format params table entry (92B); defer texture cluster. |
| 0x8272C4B8 | sub_8272C4B8 | Resource-type dispatch switch; defer with loader cluster. |
| 0x8272C538 | sub_8272C538 | Polynomial hash over byte buffer; thin helper alone. |
| 0x8272D958 | sub_8272D958 | TLS heap alloc+memset only (96 bytes). |
| 0x8272E230 | sub_8272E230 | Finds section index by path strcmp (140B); defer next resource pass. |
| 0x8272EBE0 | sub_8272EBE0 | Thin wrapper → `SubmitTextureStreamLoadRequest` only (88B). |
| 0x82730D40 | sub_82730D40 | Resets D3D command buffer write ptr (24 bytes). |
