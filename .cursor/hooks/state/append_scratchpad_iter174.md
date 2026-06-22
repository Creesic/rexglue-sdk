## Iteration 174

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826331E0 | sub_826331E0 | FM2_Lua_ToluaSetFieldLightuserdata | 0.91 | Push string key + lightuserdata; `SetTableFieldFromStack(-3)`; 104 callers in binding tables. |
| 0x82633238 | sub_82633238 | FM2_Lua_ToluaRegisterPropertyGetSet | 0.92 | Ensures `.get`/`.set` tables; registers field lightuserdata for getter/setter; strings `".get"`/`".set"`. |
| 0x826333B0 | sub_826333B0 | FM2_Lua_ToluaOpenRuntime | 0.93 | One-time init: `tolua_opened`, weak `tolua_ubox`, `tolua_super`, `tolua_gc`, `tolua_commonclass`. |
| 0x82632EF0 | sub_82632EF0 | FM2_Lua_ToluaPushModuleTable | 0.90 | Push module name or registry root `-10002`; `RegisterBindingPairsModuleTail`. |
| 0x82632F48 | sub_82632F48 | FM2_Lua_ToluaOpenModuleTable | 0.90 | Open/create module table; optional metatable via `sub_82636370`; used by `ToluaOpenRuntime`. |
| 0x82415FC8 | sub_82415FC8 | FM2_Crt_StrtolDecimal | 0.94 | Direct `strtol(a1,0,10)`; 52 callers including `ParseCmdLineIntAssignment`. |
| 0x82780718 | sub_82780718 | FM2_STL_CriticalSectionGuardDtorDrainAll | 0.90 | EH guard dtor; loops `DtorPopOne` while count `>0`; vtable `off_82145748` (134 xrefs). |
| 0x827807F8 | sub_827807F8 | FM2_STL_CriticalSectionGuardDtorPopOne | 0.89 | Pops one held lock via `LeaveCriticalSectionOrNull`; decrements guard count. |
| 0x82780838 | sub_82780838 | FM2_STL_LeaveCriticalSectionOrNull | 0.91 | `RtlLeaveCriticalSection(a1+4)` when handle non-null. |
| 0x82780888 | sub_82780888 | FM2_STL_CriticalSectionGuardDtorMaybeFree | 0.89 | Calls `DtorDrainAll`; frees guard object when flag bit 0 set. |
| 0x8243AB00 | sub_8243AB00 | FM2_IOSys_GetFileIoManagerSingleton | 0.90 | Once-init `unk_829F1468`; `atexit`; used with `Nt_RemoveDirectoryPath` / `CFileEvent`. |
| 0x8243AAB8 | sub_8243AAB8 | FM2_InitIOSysFileIoManager | 0.88 | Zeros manager; `IntrusiveList_InitSentinel` at `+20`; called from singleton getter. |

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
| 0x8243AB60 | sub_8243AB60 | IOSys async completion handler; defer with `EnqueueFileOperation`. |
| 0x8243B2E8 | sub_8243B2E8 | IOSys enqueue op with `D3D_GetFrameCounter`; defer paired pass. |
| 0x82468F38 | sub_82468F38 | `CMLPArray<float>` buffer release dtor; defer CML cluster. |
| 0x8242DE40 | sub_8242DE40 | Ghost file delete orchestrator; defer IOSys cluster completion. |
