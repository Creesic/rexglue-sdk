## Iteration 171

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x824DE028 | sub_824DE028 | FM2_Xam_GetPlayerStatsServiceSingleton | 0.90 | Once-flag init; calls `InitPlayerStatsServiceContext(dword_829F3930)`; `atexit`; 8 callers return singleton ptr. |
| 0x824DDB58 | sub_824DDB58 | FM2_InitPlayerStatsServiceContext | 0.89 | Sets vtables `off_820424B4`/`off_820424B0` + `ISystemEventSubscriber`; zeros 6 dwords + 0x10 memset. |
| 0x824DE848 | sub_824DE848 | FM2_Xam_GameClipXMsgInProcessCall | 0.91 | `WcsncpyChecked` copies `word_820424F4` (`"Game_Clip"`); `PlayerStatsXMsgInProcessCall` mode `1`; returns BOOL. |
| 0x824DE8B0 | sub_824DE8B0 | FM2_Xam_GameClipStartReadRequest | 0.90 | Stores read params on context; calls `GameClipXMsgInProcessCall`; sets pending bit `|2` on success. |
| 0x824DF0C8 | sub_824DF0C8 | FM2_Xam_GameClipStartOverlappedRead | 0.90 | `GameClipXMsgInProcessCall` then 4MB (`0x3FE000`) overlapped read; `LuaAsyncIoCompletionDispatch` slot 0. |
| 0x824DE140 | sub_824DE140 | FM2_Xam_SyncPresenceForSignedInUsers | 0.88 | Loops `XamUserGetSigninState` for 4 users; dispatches vtable `+20`; sign-in transitions + cmdline list splice. |
| 0x824DDCF8 | sub_824DDCF8 | FM2_Xam_AllocPresenceUserContextComPtr | 0.87 | Pool alloc 128 bytes; `sub_824E7D30` init; `ComPtr_AssignRef` at `a1+4*(user+2)`. |
| 0x82757DB0 | sub_82757DB0 | FM2_Crt_CanonicalizeUrlPathAnsiBuf256 | 0.91 | Wrapper → `CanonicalizeUrlPathAnsi` with per-component buflen `0x100` (or 3 for drive). |
| 0x82759108 | sub_82759108 | FM2_Crt_FprintfLocked | 0.92 | Locked `FILE*`; `stbuf`/`ftbuf`; `sub_8241B6C8` format core; EINVAL on bad handles. |
| 0x82759F88 | sub_82759F88 | FM2_Crt_AtofParseFloat | 0.91 | Skips whitespace via ctype table; `fltin2` parse; no endptr (parallel to `StrtodParseFloat`). |
| 0x8275C578 | sub_8275C578 | FM2_Crt_GetErrnoTableBoundPtr | 0.88 | Returns `&unk_829BA478`; used by `LookupErrnoMessageString` for upper bound. |
| 0x8275C588 | sub_8275C588 | FM2_Crt_GetErrnoMessageTablePtr | 0.88 | Returns `off_829BA3C8` string table; indexed by errno in `LookupErrnoMessageString`. |

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
| 0x82758B70 | sub_82758B70 | 24B wrapper → `BuildUrlPathFromComponents(a1,-1,...)`. |
| 0x8275A088 | sub_8275A088 | 8B thunk → `AtofParseFloat` only. |
| 0x8275A30C | sub_8275A30C | 56B unlock trampoline after `FgetsLocked`; too thin alone. |
| 0x8275B6B8 | sub_8275B6B8 | CRT SEH frame-handler/unwind helper; defer exception cluster. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine (~4KB); defer stdio cluster. |
| 0x824DE398 | sub_824DE398 | XOnlineStartup + presence init orchestrator; defer with `SyncPresence` cluster. |
