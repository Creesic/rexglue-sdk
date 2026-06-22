## Iteration 170

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82753F88 | sub_82753F88 | FM2_Xam_PlayerStatsXMsgInProcessCall | 0.90 | `XMsgInProcessCall(0xFC,0x58035)` with param 254; used with wchar `"Player_Stats"`; maps HRESULT to Win32 errors. |
| 0x824DE7E0 | sub_824DE7E0 | FM2_Xam_PlayerStatsReadWriteInProcess | 0.91 | Copies `word_820424D8` (`"Player_Stats"`); calls `PlayerStatsXMsgInProcessCall` with mode `3`; returns BOOL. |
| 0x824DE9C8 | sub_824DE9C8 | FM2_Xam_LuaAsyncIoCompletionDispatch | 0.89 | Handles sync/async IO results; sets pending/complete bit masks; ComPtr + Lua metatable callback dispatch. |
| 0x824DF018 | sub_824DF018 | FM2_Xam_PlayerStatsStartOverlappedRead | 0.90 | `PlayerStatsReadWriteInProcess` then overlapped read via `sub_823E3DC0`; completion to `LuaAsyncIoCompletionDispatch` slot 2. |
| 0x824DED58 | sub_824DED58 | FM2_Xam_PlayerStatsStartOverlappedWrite | 0.90 | Overlapped write via `sub_823E40A0` from `+60` buffer; completion to `LuaAsyncIoCompletionDispatch` slot 1; clears `+40`. |
| 0x824DEC78 | sub_824DEC78 | FM2_LuaGarage_DeserializeCarRecordFromStream | 0.88 | Alloc read-scope stream; `EnsureCarRecordLookupTail`; `NotifyManagerStateChange`; Lua callback returns 5 on success. |
| 0x824DEBA8 | sub_824DEBA8 | FM2_LuaGarage_GetCarRecordFieldValue | 0.87 | Garage tail vtable `+52` acquire / `+60` read field into out ptr; releases ComPtr on exit. |
| 0x82757A70 | sub_82757A70 | FM2_Crt_CanonicalizeUrlPathAnsi | 0.89 | ANSI parallel to `CanonicalizeUrlPath`; strips `\\?\`; validates component ptr/len; `FM2_Thunk_12` strncpy. |
| 0x8275A098 | sub_8275A098 | FM2_Crt_FgetsLocked | 0.90 | Locked `FILE*` read until newline/EOF; EINVAL on bad handles; 6 callers. |
| 0x82758CA0 | sub_82758CA0 | FM2_Crt_StrerrorCopyToTlsBuffer | 0.91 | Per-thread 0x86-byte buffer; `LookupErrnoMessageString` + `strcpy_s`; CRT OOM fallback string. |
| 0x82758C48 | sub_82758C48 | FM2_Crt_LookupErrnoMessageString | 0.88 | Bounds-checks errno against CRT table; returns message pointer from `sub_8275C588`. |
| 0x82758758 | sub_82758758 | FM2_Crt_IndexOfFirstCharNotInSet | 0.89 | Builds 32-byte charset bitmap; returns index of first `a1` char not in `a2` set (strspn inverse). |

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
| 0x82757DB0 | sub_82757DB0 | Thin wrapper → `CanonicalizeUrlPathAnsi` with 0x100 bufs. |
| 0x82758B70 | sub_82758B70 | 24B wrapper → `BuildUrlPathFromComponents(a1,-1,...)`. |
| 0x82759108 | sub_82759108 | CRT `fprintf` core; defer stdio cluster. |
| 0x8275B6B8 | sub_8275B6B8 | CRT SEH frame-handler/unwind helper; defer exception cluster. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine (~4KB); defer stdio cluster. |
| 0x824DE028 | sub_824DE028 | Lazy singleton init + `atexit`; defer paired with `dword_829F3930`. |
| 0x82756580 | sub_82756580 | 16B kernel-stack trap thunk; covered by exception-filter rename. |
