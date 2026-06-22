## Iteration 175

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8243AB60 | sub_8243AB60 | FM2_IOSys_CompleteFileOperation | 0.91 | Finds queued op by serial; sets completion time + NTSTATUS/Win32; vtable `+4` log string; `WriteStringToFileSink`. |
| 0x8243B2E8 | sub_8243B2E8 | FM2_IOSys_EnqueueFileOperation | 0.92 | Assigns monotonic serial; `D3D_GetFrameCounter`; `KeQuerySystemTime`; `CircularBuffer_PushFrontNode`. |
| 0x8242DE40 | sub_8242DE40 | FM2_RaceGhost_RemoveDirectoryAsync | 0.90 | Builds `CFileEventRemoveDir`; `Nt_RemoveDirectoryPath`; enqueue + complete via file-io manager singleton. |
| 0x8242DCD0 | sub_8242DCD0 | FM2_IOSys_InitFileEventRemoveDir | 0.91 | `IOSys::CFileEvent` → `CFileEventRemoveDir` vtable; event type `9`; copies path string. |
| 0x8243D490 | sub_8243D490 | FM2_IOSys_WriteStringToFileSink | 0.89 | Writes STL string buffer to sink vtable `+36` with length `size+1`. |
| 0x82632B28 | sub_82632B28 | FM2_Lua_ToluaCastBinding | 0.91 | Lua `tolua.cast`; resolves userdata + typename; `PushUserdataFromUbox` or nil. |
| 0x82632BC8 | sub_82632BC8 | FM2_Lua_ToluaInheritSetCInstance | 0.92 | Sets `".c_instance"` field on metatable for inheritance wiring. |
| 0x82632C18 | sub_82632C18 | FM2_Lua_ToluaSetPeer | 0.93 | Error string `Invalid argument #1 to setpeer: userdata expected.`; assigns peer table. |
| 0x82632CA8 | sub_82632CA8 | FM2_Lua_ToluaGetPeer | 0.90 | Reads peer from userdata env; returns nil when peer equals self. |
| 0x82634598 | sub_82634598 | FM2_Lua_ToluaPushUserdataFromUbox | 0.92 | Manages `tolua_ubox` weak table + `tolua_super`; creates/pushes userdata by pointer. |
| 0x82468F38 | sub_82468F38 | FM2_CMLPArrayFloat_ReleaseOwnedBuffer | 0.91 | `CMLPArray<float>` vtable; frees `+8` buffer when `+5` ownership flag clear; 51 callers. |
| 0x82633A58 | sub_82633A58 | FM2_Lua_ToluaToLStringOrDefault | 0.90 | `ToLString` when stack depth ok else default; used by `ToluaCastBinding` for type name. |

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
| 0x82633BE8 | sub_82633BE8 | Thin helper combining userdata/lightuserdata read; defer next tolua pass. |
