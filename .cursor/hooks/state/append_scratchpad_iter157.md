## Iteration 157

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8272B928 | sub_8272B928 | FM2_RenderResource_Setup2DTextureMipLayout | 0.91 | Dispatched for texture type 0; walks mips/faces; uses format layout helpers, `XGGetMipTailLevelOffset`, tile/untile, and `FM2_RenderResource_FillTextureSurfaceLayoutByFormat`. |
| 0x8272BBB0 | sub_8272BBB0 | FM2_RenderResource_SetupCubeTextureMipLayout | 0.91 | Dispatched for cube type 2; iterates 6 faces via `unk_829A7D70`; same mip sizing/tile path as 2D setup. |
| 0x8272BED0 | sub_8272BED0 | FM2_RenderResource_SetupArrayTextureMipLayout | 0.90 | Dispatched for array type 4; loops `a1[14]` slice textures; computes per-slice mip offsets and layout fill. |
| 0x8272C1B8 | sub_8272C1B8 | FM2_RenderResource_SetupVolumeTextureMipLayout | 0.90 | Dispatched for volume type 5; nested loops over depth slices at +56; volume mip tail offsets via XG. |
| 0x8272AEB8 | sub_8272AEB8 | FM2_RenderResource_FillTextureSurfaceLayoutByFormat | 0.92 | Large format switch (`0x1A200186`, `0x28280144`, etc.) writes tiled surface bytes using `byte_829A7AEC` tables; shared by all mip setup paths. |
| 0x82727D30 | sub_82727D30 | FM2_Render_InitPassLightingSlots | 0.89 | Runs pass-lighting batch A/B with arg 0 then `FM2_Render_AllocPassLightingSlotArray`; called from render init path. |
| 0x82727D80 | sub_82727D80 | FM2_Render_ResetPassLightingSlots | 0.89 | Same as init variant but passes 1 to lighting batch processors before reallocating slot array. |
| 0x82728640 | sub_82728640 | FM2_Render_InvokeGlobalRenderCallbackIfSet | 0.88 | Optional invoke of `dword_82A41D90(dword_82A41D8C)`; used before bound draw pass execution. |
| 0x8272A410 | sub_8272A410 | FM2_Render_SetupObjectDrawPassState | 0.90 | Sets render-context state bits on `dword_82A41BEC`; submits object draw constants; disables depth/index modes; calls large draw helper `sub_827294D8`. |
| 0x82728408 | sub_82728408 | FM2_Render_LoadGlobalPassStatePtr | 0.90 | Stores `dword_82A41D2C` (pair of `FM2_RenderTls_SetGlobalPassStatePtrA`); 8 callers in PM4 draw dispatch table. |
| 0x82728458 | sub_82728458 | FM2_Render_LoadDefaultLookupTextureMagentaPtr | 0.91 | Stores `dword_82A41D20` — 4×4 magenta lookup texture created in `FM2_Render_CreateDefaultLookupTextures`. |
| 0x82728468 | sub_82728468 | FM2_Render_LoadDefaultLookupTextureZeroPtr | 0.91 | Stores `dword_82A41D24` — 4×4 zero/black lookup texture from same default-texture init. |

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
| 0x82722808 | sub_82722808 | Large PM4 opcode dispatch (~2KB); needs dedicated pass. |
| 0x82722FD8 | sub_82722FD8 | Large draw pass (~1.9KB); defer render draw cluster. |
| 0x82723B30 | sub_82723B30 | 8-byte wrapper → `ReleaseGpuResourceArray` only. |
| 0x82723C28 | sub_82723C28 | Thin dispatch to memcpy helpers only (24 bytes). |
| 0x82725C60 | sub_82725C60 | Cross product math only (64B); thin helper alone. |
| 0x82725CC8 | sub_82725CC8 | Vector distance-squared math only (52B); thin helper alone. |
| 0x82725D68 | sub_82725D68 | Zeros 3 floats only (24 bytes). |
| 0x82725EF0 | sub_82725EF0 | Float reciprocal only (16 bytes). |
| 0x827294D8 | sub_827294D8 | Large object draw emit (~1.1KB); defer with draw cluster. |
| 0x8272AE80 | sub_8272AE80 | Sets texture GPU base + type 0 only (16B). |
| 0x8272F650 | sub_8272F650 | D3D indexed-triangle fan command emit (748B); defer GPU emit cluster. |
| 0x8272F940 | sub_8272F940 | Tiled surface resolve/blit helper (608B); defer GPU emit cluster. |
| 0x8272F5C8 | sub_8272F5C8 | Packed section decompress loop; defer transport cluster. |
| 0x82721930 | sub_82721930 | Reads TLS main-context +348 only (44B). |
