## Iteration 167

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82754D60 | sub_82754D60 | FM2_Xam_ClearTlsWorkContextPointerAndFree | 0.89 | Reads TLS `+256→+356`; clears pointer and `FreeTagged(0x64800000)`; callee of `FreeKernelStackWorkContext`. |
| 0x82754E90 | sub_82754E90 | FM2_Xam_FreeKernelStackWorkContext | 0.90 | If TLS current matches, clear+`ExitThread`; else `MmDeleteKernelStack` + `FreeTagged`; pairs with `AllocKernelStackWorkContext`. |
| 0x82754F00 | sub_82754F00 | FM2_Xam_KernelStackUnhandledExceptionFilter | 0.87 | Invokes work-context callback from TLS then `KeBugCheck(0)`; used by kernel-stack exception path at `0x82756580`. |
| 0x82753D30 | sub_82753D30 | FM2_Xam_XNetStartupViaXamOrdinalOrNetDll | 0.90 | Resolves `xam.xex` ordinal `0x50` on NXE 2.0.16256+; falls back to `NetDll_XNetStartup`. |
| 0x82754028 | sub_82754028 | FM2_Xam_SessionCreateStartIoRequest | 0.91 | Validates session flags; `XamSessionCreateHandle` + `XMsgStartIORequest(0xFB,0xB0010)`; 3 live callers. |
| 0x827548B0 | sub_827548B0 | FM2_Xam_SessionStartPropertyIoRequest | 0.88 | `XamSessionRefObjByHandle` + `XMsgStartIORequest(0xFB,0xB0025)`; session property write path. |
| 0x82754BA0 | sub_82754BA0 | FM2_Nt_QueryPathFileAttributes | 0.91 | `NtQueryFullAttributesFile` on `\\??\\` path; returns `FileAttributes` or -1. |
| 0x82754F70 | sub_82754F70 | FM2_Nt_RemoveDirectoryPath | 0.90 | `RtlInitAnsiString` + `RtlRemoveDirectory`; returns 1/0. |
| 0x827527B0 | sub_827527B0 | FM2_AtiSSM_InitIRInstOpcode48_AddToCfgRootSet | 0.87 | `AllocWorkItemBody(48)`; vtable `off_8213BE58`; `CFG::AddToRootSet`; callee of mov-IR creator. |
| 0x82750EA0 | sub_82750EA0 | FM2_AtiSSM_CreateMovIRInstBindOperandsAndMask | 0.91 | Creates opcode-48 IR inst; binds operands/VRegs; `vreginfo.cpp:361` mask assert; writes swizzle mask via `dword_8213B4DC`. |
| 0x82753658 | sub_82753658 | FM2_AtiSSM_AllocScalarConstOneWorkItem | 0.88 | `AllocWorkItemBody`; vtable `off_8213C620`; sets const `0x01010101` at slot 32. |
| 0x82752CF0 | sub_82752CF0 | FM2_AtiSSM_InitIRInstByOpcode_AddToCfgRootSet | 0.87 | Generic `AllocWorkItemBody(opcode)`; vtable `off_8213C4B8`; `CFG::AddToRootSet`; flags `0x18`. |

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
| 0x82752990 | sub_82752990 | Returns constant 1; vtable predicate stub (8B). |
| 0x82753788 | sub_82753788 | Bump wrapper → `AllocAddrIndexedRegisterSetWithId`; too thin alone. |
| 0x82753DE8 | sub_82753DE8 | 12-byte wrapper → `XNetStartupViaXamOrdinalOrNetDll(XNCALLER_TITLE)`. |
| 0x82754FC8 | sub_82754FC8 | XEX header field `0x40006` lookup; defer image-header cluster. |
| 0x827551B0 | sub_827551B0 | Console signature read/write via `XeKeys`; defer security cluster. |
| 0x82756580 | sub_82756580 | 16B kernel-stack trap thunk; covered by exception-filter rename. |
| 0x82753E00 | sub_82753E00 | 16-byte jump thunk to `0x8294F028`. |
