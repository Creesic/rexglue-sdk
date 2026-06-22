## Iteration 169

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827536A8 | sub_827536A8 | FM2_AtiSSM_BumpAllocAndInitScalarConstWorkItem | 0.88 | `BumpAllocAligned(964)` then `AllocScalarConstOneWorkItem`; vtable ctor xref. |
| 0x82753788 | sub_82753788 | FM2_AtiSSM_BumpAllocAddrIndexedRegisterSet | 0.88 | `BumpAllocAligned(56)` then `AllocAddrIndexedRegisterSetWithId`; vtable xref `off_8213ED10`. |
| 0x827531F8 | sub_827531F8 | FM2_AtiSSM_BumpAllocAndInitIRInstOpcode136 | 0.88 | `BumpAllocAligned(964)` then `InitIRInstOpcode136_AddToCfgRootSet`; vtable ctor xref. |
| 0x82753248 | sub_82753248 | FM2_AtiSSM_BumpAllocAndInitIRInstOpcode135 | 0.88 | `BumpAllocAligned(964)` then `InitIRInstOpcode135_AddToCfgRootSet`; vtable ctor xref. |
| 0x82753DE8 | sub_82753DE8 | FM2_Xam_XNetStartupTitleCaller | 0.90 | Forwards to `XNetStartupViaXamOrdinalOrNetDll(XNCALLER_TITLE, xnsp)`; export xref. |
| 0x827560A8 | sub_827560A8 | FM2_Xam_GetGameRatingFromConfigAndRegion | 0.89 | `ExGetXConfigSetting(3,0xE)` jump table + `XGetGameRegion` fallback; returns rating byte 20/21/35/36. |
| 0x82759C80 | sub_82759C80 | FM2_Crt_BuildUrlPathFromComponents | 0.90 | Concatenates scheme/host/path/extension with `:` `\\` `.`; ERANGE/EINVAL handling; 9 callers. |
| 0x82759940 | sub_82759940 | FM2_Crt_CanonicalizeUrlPath | 0.89 | Strips `\\?\` prefix; validates component ptr/len pairs; copies drive/host/path segments; 7 callers. |
| 0x82757EF0 | sub_82757EF0 | FM2_Math_ApproxFrexpLog2 | 0.88 | Uses `1.442695` (1/ln2) polynomial; stores exponent in `*a3`; fractional part via `CoalesceStringConcatExpBody`. |
| 0x827588B0 | sub_827588B0 | FM2_Crt_UitoaRadixReverse | 0.90 | Radix 2–36 digit loop with reverse in-place; optional negative prefix; EINVAL/ERANGE checks. |
| 0x82758560 | sub_82758560 | FM2_Crt_StrtodParseFloat | 0.89 | Skips whitespace; `fltin2` locale parse; handles INF/overflow flags; updates end pointer. |
| 0x82759F28 | sub_82759F28 | FM2_Crt_WscanfCoreViaVscanFn | 0.87 | Wide scanf adapter calling `vscan_fn_0` with `sub_8275C958` input fn; 12 callers. |

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
| 0x82723B30 | sub_82723B30 | 8-byte wrapper → `ReleaseGpuResourceArray` only. |
| 0x82723C28 | sub_82723C28 | Thin dispatch to memcpy helpers only (24 bytes). |
| 0x827218D8 | sub_827218D8 | Writes dword to indexed array only (16B). |
| 0x827218E8 | sub_827218E8 | Writes dword to nested array only (16B). |
| 0x8270DA40 | sub_8270DA40 | SQLite correlation walker; defer expr cluster. |
| 0x82718B28 | sub_82718B28 | SQLite parse reduce helper; defer codegen cluster. |
| 0x827477AC | sub_827477AC | Jump-table stub cluster (18×12B); covered by dispatcher rename. |
| 0x82753E00 | sub_82753E00 | 16-byte jump thunk to `0x8294F028`. |
| 0x82753ED0 | sub_82753ED0 | Jump thunk to `0x8294F0D8` (28B). |
| 0x82753F88 | sub_82753F88 | `XMsgInProcessCall(0x58035)`; message semantics unclear alone. |
| 0x82757DB0 | sub_82757DB0 | Thin wrapper → `CanonicalizeUrlPath` with default buflen. |
| 0x82757A70 | sub_82757A70 | Near-duplicate of `CanonicalizeUrlPath`; defer paired pass. |
| 0x82758B70 | sub_82758B70 | 24B wrapper → `BuildUrlPathFromComponents(a1,-1,...)`. |
| 0x82759108 | sub_82759108 | CRT `fprintf` core; defer stdio cluster. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine (~4KB); defer with `WscanfCoreViaVscanFn`. |
| 0x82756580 | sub_82756580 | 16B kernel-stack trap thunk; covered by exception-filter rename. |
