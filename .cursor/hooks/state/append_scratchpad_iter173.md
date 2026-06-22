## Iteration 173

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x824E7430 | sub_824E7430 | FM2_Xam_StartFriendsEnumeratorOverlappedFetch | 0.90 | `XFriendsCreateEnumerator(user,0,0x64)`; alloc friends buffer; async pending `997`; ComPtr overlapped dispatch. |
| 0x824E6098 | sub_824E6098 | FM2_Xam_InitFriendsFetchOverlappedContext | 0.88 | 32-byte context; vtable `off_82042B3C`; `NotifyManagerStateChange` with presence user ComPtr. |
| 0x82633AB0 | sub_82633AB0 | FM2_Lua_ToluaGetUserdataPointerDword | 0.91 | Tolua stack helper; `GetUserdataPointer`; returns `*dword` (263 xrefs). |
| 0x82634070 | sub_82634070 | FM2_Lua_ToluaIsCInstanceUserdata | 0.92 | Checks userdata type 5; compares field `".c_instance"` via `sub_824B6B40`. |
| 0x82634100 | sub_82634100 | FM2_Lua_ToluaIsTypeOnStack | 0.91 | strcmp typename; walks `tolua_super` table; `IsStackSlotTruthy` for inheritance. |
| 0x826344E0 | sub_826344E0 | FM2_Lua_ToluaBeginTypeCheckAccess | 0.90 | Validates stack depth/type; calls `IsTypeOnStack`; fills access record triple. |
| 0x82634258 | sub_82634258 | FM2_Lua_ToluaFailNoObject | 0.90 | Sets access record with string `"[no object]"`; returns error 1. |
| 0x82634360 | sub_82634360 | FM2_Lua_ToluaFailExpectedNumber | 0.90 | `IsNumberOrCoercibleToNumber` guard; sets `"number"` type error string. |
| 0x826339E8 | sub_826339E8 | FM2_Lua_ToluaToNumberOrDefault | 0.91 | Returns `ToNumberOrZero` when stack slot present else default double arg. |
| 0x82464208 | sub_82464208 | FM2_ParseCmdLineIntAssignment | 0.92 | `key=value` via `strnicmp` + `=`; parses int with `strtol(...,10)`; 56 callers. |
| 0x825C3C28 | sub_825C3C28 | FM2_Render_SetMaterialPropertyIfChanged | 0.89 | Vtable getter `+544` compare; queues type-3 record; setter `+64` when changed (162 xrefs). |
| 0x825C3918 | sub_825C3918 | FM2_Render_AppendMaterialStateChangeRecord | 0.88 | Appends 48-byte event to material state vector; grow via `sub_825C37D8`. |

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
| 0x827218D8 | sub_827218E8 | Writes dword to indexed array only (16B). |
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
| 0x82415FC8 | sub_82415FC8 | 12B `strtol(...,10)` thunk; defer with cmdline cluster. |
| 0x826331E0 | sub_826331E0 | Tolua register field helper; defer next tolua pass. |
| 0x82780718 | sub_82780718 | STL EH unwind critical-section drain; defer with `807F8` cluster. |
| 0x8243AB00 | sub_8243AB00 | Lazy singleton init; callee `sub_8243AAB8` not yet named. |
