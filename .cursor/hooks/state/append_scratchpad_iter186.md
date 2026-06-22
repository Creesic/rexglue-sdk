## Iteration 186

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82620248 | sub_82620248 | FM2_Lua_ResetBindingStateAndInvokeClear | 0.91 | Frees binding buffers/strings; zeros vector triples; calls `FM2_Lua_InvokeProtectedClearBindingCallback`; 20 callers. |
| 0x827DD260 | sub_827DD260 | FM2_Lua_GetUnitStringTableEntryPtr | 0.90 | Selects table `a1[130/131/132]` by tag; lookup via slot helper; returns entry `+8` or empty sentinel; 20 callers. |
| 0x827EF320 | sub_827EF320 | FM2_Lua_LookupOrCreateUnitStringTableSlot | 0.91 | Indexed dword table access; lazy create via vtable `+4` when `a3` set; stores via release-assign helper; 8 callers. |
| 0x82619680 | sub_82619680 | FM2_Stl_IntVector_ResizeOrEraseToSize | 0.92 | Grows via `sub_824A2C88` or erases tail via `FM2_IntVector_EraseRangeShift`; count from `(end-begin)>>2`; 20 callers. |
| 0x82506380 | sub_82506380 | FM2_Object_ScalarDtorMaybeFree_82000E18 | 0.90 | `FM2_Object_AssignBaseVtable_82000E18`; optional `FM2_Memory_FreeSmallBlockOrNull`; 20 vtable dtors. |
| 0x8245AEF8 | sub_8245AEF8 | FM2_HashName_InitFloatPropertyNode | 0.91 | Sets vtable `off_8203CEA4`; `FM2_HashName_AssignPropertyByTypeId(9)`; stores float at `+8`; 20 callers. |
| 0x822B3078 | sub_822B3078 | FM2_ProfileLua_PushNumberAndInvokeBindingProtectedCall | 0.90 | `FM2_Lua_PushNumber` + `FM2_ProfileLua_InvokeBindingProtectedCall`; 20 callers. |
| 0x82298088 | sub_82298088 | FM2_Lua_SetUserInterfaceLocStringOnBinding | 0.92 | Uses `"UserInterface::LocString"` env helper; pops into binding slot; 20 callers. |
| 0x82298008 | sub_82298008 | FM2_Lua_PushUserInterfaceLocStringEnv | 0.91 | `FM2_Lua_PushUserdataForKey(4)`; notify state change; sets closure env from stack; paired with LocString setter. |
| 0x822613B0 | sub_822613B0 | FM2_D3D_TryAcquirePresentThrottleSlot | 0.89 | `FM2_D3D_GetGlobalPresentThrottleSingleton` vtable `+64` with arg `8`; maps success to `0` else `result-4`; 20 callers. |
| 0x825A3F10 | sub_825A3F10 | FM2_Memory_ClearSmallBlockVectorTriple | 0.90 | `FM2_Memory_TryFreeViaPoolHandler` on `+4` block; zeros begin/end/cap at `+4/+8/+12`; 19 callers. |
| 0x827E5E60 | sub_827E5E60 | FM2_Network_RecordPacketStatsInBucket | 0.90 | Critsec `+13900`; bucket `id%0xBB9`; 24-byte stat records; dedupe/increment or push; 22 network callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8235ED30 | sub_8235ED30 | 56B thin wrapper → `sub_82334768` vector push only. |
| 0x827B06B8 | sub_827B06B8 | 56B vtable init + delegate; defer render base cluster. |
| 0x825C5CE0 | sub_825C5CE0 | Thin wrapper → hash lookup + stream write; defer cluster. |
| 0x82420548 | sub_82420548 | Single-line SQLite open-flag mask lookup; too thin alone. |
| 0x82225188 | sub_82225188 | Com static-lifetime init thunk; insufficient standalone evidence. |
| 0x8279D7E8 | sub_8279D7E8 | 40B wrapper only. |
| 0x82789188 | sub_82789188 | 40B wrapper only. |
| 0x827B6740 | sub_827B6740 | 40B wrapper only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only; too thin alone. |
| 0x82779608 | sub_82779608 | `FM2_Noop` only; too trivial alone. |
| 0x82782C68 | sub_82782C68 | Thin wrapper → `sub_82782F18` only. |
| 0x825FE048 | sub_825FE048 | Thin wrapper → `FM2_CarAudioMixChannel_ReplaceVoiceRange_0`. |
| 0x82424C00 | sub_82424C00 | CRT `ungetc` implementation; defer stdio cluster. |
| 0x827D6960 | sub_827D6960 | Thin release-and-assign slot helper; defer with unit-string cluster. |
| 0x82762918 | sub_82762918 | Network open-connection wrapper; defer with `sub_827623A0` cluster. |
| 0x827623A0 | sub_827623A0 | Connection context validate; defer network open cluster. |
| 0x8295C378 | sub_8295C378 | Large (~1KB) unanalyzed function; defer next pass. |
| 0x8295CCD8 | sub_8295CCD8 | Large unanalyzed function; defer next pass. |
| 0x827E55E0 | sub_827E55E0 | Large unanalyzed render function; defer next pass. |
| 0x824F3828 | sub_824F3828 | Large unanalyzed function; defer next pass. |
| 0x8270DA40 | sub_8270DA40 | SQLite correlation walker; defer expr cluster. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine; defer stdio cluster. |
