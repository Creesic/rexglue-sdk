## Iteration 172

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x824DE398 | sub_824DE398 | FM2_Xam_StartupOnlinePresenceAndSubscriptions | 0.91 | `XOnlineStartup`; registers 9 cmdline subscriptions via vtable `+8`; `XPresenceInitialize(0x190)`; calls `SyncPresenceForSignedInUsers`. |
| 0x824DE610 | sub_824DE610 | FM2_Xam_PresenceEventSubscriberCallback | 0.90 | System-event subscriber; event `10` → `SyncPresence`; `14` → signout bitmask; `0x4000001-3` → profile refresh. |
| 0x824E7D30 | sub_824E7D30 | FM2_Xam_InitPresenceUserContext | 0.91 | 128-byte context init; `XamUserGetName`/`XUserGetXUID`; `LiveStrings` key 57686 fallback; multiple vtables. |
| 0x824E7C48 | sub_824E7C48 | FM2_Xam_LoadPresenceUserProfileData | 0.88 | Loads profile list into out-list; early exit if `+49` set; calls `sub_824E7430` when `+56` unset. |
| 0x824DDF70 | sub_824DDF70 | FM2_Xam_NotifyPresenceUsersSignoutBitmask | 0.89 | Iterates 4 user slots from bitmask; `NotifyManagerStateChange`; vtable `+36`/`+0` signout notify. |
| 0x824DE088 | sub_824DE088 | FM2_Xam_RefreshPresenceUserProfilesAllSlots | 0.89 | All 4 slots; `NotifyManagerStateChange`; vtable `+48` refresh when `+57` clear; clears `+56`. |
| 0x824D2A38 | sub_824D2A38 | FM2_ScriptScope_ResolvePathLocked | 0.90 | `RtlEnterCriticalSection` at `+72`; delegates to `ResolveScopedPath`; 141 xrefs from script thunks. |
| 0x824D2768 | sub_824D2768 | FM2_ScriptScope_ResolveScopedPath | 0.90 | Parses first path segment; `CScriptScope` RTTI dynamic_cast; creates child scope via `SslDeviceBinding_Ctor`. |
| 0x82464290 | sub_82464290 | FM2_ParseCmdLineBoolAssignment | 0.92 | `stricmp` exact or `key=true` via `strnicmp` + `=`; sets bool out; 132 callers in cmdline parser. |
| 0x82421138 | sub_82421138 | FM2_Crt_FilbufRefillAndReadByte | 0.93 | CRT `_filbuf`: `read` into stream buffer; EOF/error flags; returns next byte; used by `FgetsLocked`. |
| 0x8241B6C8 | sub_8241B6C8 | FM2_Crt_PrintfOutputCore | 0.91 | Va_list printf formatter (~3KB); debug `OutputDebugStringA` for stdout/stderr lock-table handles. |
| 0x8275C7F0 | sub_8275C7F0 | FM2_Crt_StrSpanIncludingCharsBackward | 0.88 | Walks backward in `[a1,a2)` skipping ctype flag `4` chars; used by `BuildUrlPathFromComponents`. |

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
| 0x8275A30C | sub_8275A30C | 56B unlock trampoline inside `FgetsLocked`; too thin alone. |
| 0x8275B6B8 | sub_8275B6B8 | CRT SEH frame-handler/unwind helper; defer exception cluster. |
| 0x8275C8A8 | sub_8275C8A8 | 8B thunk → `StrSpanIncludingCharsBackward`. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine (~4KB); defer stdio cluster. |
| 0x82633AB0 | sub_82633AB0 | High-xref render/GPU cluster; defer dedicated pass. |
| 0x825C3C28 | sub_825C3C28 | High-xref unnamed cluster; needs paired analysis. |
| 0x824E7430 | sub_824E7430 | Presence profile fetch helper; defer with `LoadPresenceUserProfileData`. |
