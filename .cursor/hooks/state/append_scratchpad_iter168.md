## Iteration 168

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82754FC8 | sub_82754FC8 | FM2_Xex_GetOptionalHeaderField40006 | 0.90 | `RtlImageXexHeaderField(..., 0x40006)`; callee of `XapiGetCurrentTitleId`. |
| 0x82755630 | sub_82755630 | FM2_Xam_CreateHarddiskCachePartition | 0.91 | Reads/writes 1KB cache header; `\\Device\\Harddisk0\\Cache%u`; schedules `CreateStfsCacheDeviceTask`; `XapiValidateDiskPartitionEx`. |
| 0x827552E8 | sub_827552E8 | FM2_Xam_CreateStfsCacheDeviceTask | 0.90 | `XamTask` proc; `StfsCreateDevice`; symbolic link `\\Device\\cache%d`; registers title-terminate handler. |
| 0x82755130 | sub_82755130 | FM2_Xam_VerifyCacheHeaderConsoleSignature | 0.91 | `XeCryptSha` + `XeKeysConsoleSignatureVerification` on cache header; gate in partition creator. |
| 0x82755098 | sub_82755098 | FM2_Xam_CacheDeviceTitleTerminateHandler | 0.88 | `ExRegisterTitleTerminateNotification` unregister; signals cache teardown event; registered by STFS task. |
| 0x827551B0 | sub_827551B0 | FM2_Xam_WriteSignedCacheHeaderToFile | 0.90 | Signs header via `XeKeysConsolePrivateKeySign`; read/modify/write 1KB cache file on `unk_8213F190`. |
| 0x827567D8 | sub_827567D8 | FM2_Xam_QueryWindowsPartitionSizeForCache | 0.89 | Maps `Partition1` → `WindowsPartition`; IOCTL `0x74004` size query; callee of format helper. |
| 0x827568D8 | sub_827568D8 | FM2_Xam_FormatHarddiskCachePartition | 0.88 | Opens cache device; IOCTLs `0x70000`/`0x74004`; computes aligned partition layout; writes partition metadata. |
| 0x827559C0 | sub_827559C0 | FM2_Xam_CreateHarddiskCachePartitionForCurrentTitle | 0.90 | Thin wrapper: `XapiGetCurrentTitleId()` then `CreateHarddiskCachePartition`. |
| 0x82752990 | sub_82752990 | FM2_AtiSSM_IRInstGetOperationInputCountOne | 0.89 | Returns 1; vtable slot +4 (`OperationInputs`) on 224 IR-inst vtables; used in asm encode asserts. |
| 0x82753158 | sub_82753158 | FM2_AtiSSM_BumpAllocAndInitIRInstByOpcode | 0.88 | `BumpAllocAligned(964)` then `InitIRInstByOpcode_AddToCfgRootSet`; vtable ctor xref. |
| 0x827562A0 | sub_827562A0 | FM2_Xam_FileTimeToLocalTimeWithBias | 0.87 | `XapipGetTimeZoneBias`; subtracts bias from FILETIME low dword; 4 callers. |

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
| 0x827536A8 | sub_827536A8 | Bump wrapper → `AllocScalarConstOneWorkItem`; too thin alone. |
| 0x82753788 | sub_82753788 | Bump wrapper → `AllocAddrIndexedRegisterSetWithId`; too thin alone. |
| 0x82753DE8 | sub_82753DE8 | 12-byte wrapper → `XNetStartupViaXamOrdinalOrNetDll(XNCALLER_TITLE)`. |
| 0x82755060 | XapiGetCurrentTitleId | Already has meaningful XAPI name. |
| 0x82757EF0 | sub_82757EF0 | `log2` polynomial approx + Lua concat; defer math/Lua cluster. |
| 0x82753E00 | sub_82753E00 | 16-byte jump thunk to `0x8294F028`. |
