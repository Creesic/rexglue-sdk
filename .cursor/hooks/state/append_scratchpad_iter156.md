## Iteration 156

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8272E230 | sub_8272E230 | FM2_RenderResource_FindSectionIndexByPath | 0.89 | strcmp walk of CAFF section path strings at `a1[18]`; returns 1-based index; used by `FM2_RenderResource_ParseContainerAndAllocSections` for `.data` flag. |
| 0x8272EBE0 | sub_8272EBE0 | FM2_RenderResource_EnqueueTextureStreamLoad | 0.88 | Increments `dword_82A42630`; picks stream buffer size 2048 when `dword_82A42624`; delegates to `FM2_RenderResource_SubmitTextureStreamLoadRequest`. |
| 0x8272D9B8 | sub_8272D9B8 | FM2_RenderResource_AllocLoadRequestFromPath | 0.90 | TLS-allocates load-request struct; copies resource path; sets flags at +88/+22; used by texture stream submit. |
| 0x8272D958 | sub_8272D958 | FM2_RenderResource_AllocZeroedTlsBlock | 0.88 | Calls TLS heap `dword_82A4260C` then `memset` zero; backing allocator for load-request objects. |
| 0x8272C538 | sub_8272C538 | FM2_RenderResource_HashBufferPolynomial | 0.90 | Polynomial rolling hash with nibble fold; compared against CAFF header checksum in `FM2_RenderResource_ValidateCaffContainerHeader`. |
| 0x8272C4B8 | sub_8272C4B8 | FM2_RenderResource_DispatchTextureMipLayoutSetupByType | 0.91 | Switch on texture type at +28: 0→`sub_8272B928`, 2→`sub_8272BBB0`, 4→`sub_8272BED0`, 5→`sub_8272C1B8`; called after XG header setup. |
| 0x8272B760 | sub_8272B760 | FM2_Render_LookupGpuFormatLayoutParams | 0.92 | Linear search 36-entry table `unk_829A7B30` by D3D format dword; returns 4 layout params used by mip sizing helpers. |
| 0x82726E58 | sub_82726E58 | FM2_Render_SetPassStateBit | 0.89 | ORs bit mask `byte_829A76E4[bit&7]` into byte array at `bit>>3`; paired with test/clear helpers in draw paths. |
| 0x82726EA0 | sub_82726EA0 | FM2_Render_TestPassStateBit | 0.89 | Tests pass-state bit via lookup mask `byte_829A76E4`; 8 callers in large draw-pass functions. |
| 0x82726E80 | sub_82726E80 | FM2_Render_ClearPassStateBit | 0.88 | AND-clears single bit in pass-state byte array using `~(128>>(bit&7))` mask. |
| 0x8272AE90 | sub_8272AE90 | FM2_Render_GetTextureResourceGpuOffset | 0.88 | Reads texture type at +28; for type 4 indexes per-face offset array at +52; else returns base GPU offset dword. |
| 0x82730D40 | sub_82730D40 | FM2_Render_ResetD3DCommandBufferWritePtr | 0.90 | Restores command write cursor from `a1+13076` into `a1+48`; 8 callers after surface/pixel uploads. |

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
| 0x82727D30 | sub_82727D30 | Pass-lighting init orchestrator (76B); defer with lighting cluster. |
| 0x82728408 | sub_82728408 | Stores global ptr `dword_82A41D2C` only (16B). |
| 0x82728458 | sub_82728458 | Stores global ptr `dword_82A41D20` only (16B). |
| 0x82728468 | sub_82728468 | Stores global ptr `dword_82A41D24` only (16B). |
| 0x82728640 | sub_82728640 | Optional global callback invoke (40 bytes). |
| 0x82728730 | sub_82728730 | 8-byte alpha-test value writer thunk; too thin alone. |
| 0x82729288 | sub_82729288 | Reads single dword at +8 (12 bytes). |
| 0x8272A410 | sub_8272A410 | Large draw-object setup; defer render draw cluster. |
| 0x8272B928 | sub_8272B928 | 2D texture mip layout setup (648B); defer texture mip cluster pass. |
| 0x8272BBB0 | sub_8272BBB0 | Cube texture mip layout (796B); defer texture mip cluster. |
| 0x8272BED0 | sub_8272BED0 | Array texture mip layout (744B); defer texture mip cluster. |
| 0x8272F650 | sub_8272F650 | D3D indexed-triangle fan command emit (748B); defer GPU draw emit cluster. |
| 0x8272F940 | sub_8272F940 | Tiled surface resolve/blit helper (608B); defer GPU emit cluster. |
