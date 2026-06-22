## Iteration 184

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x822ECCE8 | sub_822ECCE8 | FM2_ProfileLua_InitBindingWithNumberMarker | 0.91 | `FM2_ProfileLua_InitBindingNilMarker` + `FM2_Lua_PushNumberAndStackValue`; links marker into binding; 23 callers. |
| 0x82201CB8 | sub_82201CB8 | FM2_ResourceLock_AllocFrameSlotAndCommit | 0.90 | Alloc 72-byte ref block; copies 44-byte frame fields; registers via `FM2_ResourceLock_GetNullFrameSlotSentinel`; commits via vtable+36; 24 callers. |
| 0x824D8130 | sub_824D8130 | FM2_Profile_InitUnitStringBinding | 0.92 | 56-byte binding vtable `off_82042184`; hooks `UnitStrings` notify keys; stores unit index/double/flags; paired with pool alloc wrapper. |
| 0x824D8B98 | sub_824D8B98 | FM2_Profile_AllocUnitStringBindingComPtr | 0.91 | `FM2_AllocPoolAcquireOrInit_Thunk(56)` + init helper; `FM2_ComPtr_ResetAndAssign`; 22 ProfileLua callers. |
| 0x822A2670 | sub_822A2670 | FM2_ProfileLua_PushNumberAndInvokeManagerCallback | 0.90 | `FM2_Lua_PushNumber` then `FM2_ProfileLua_InvokeManagerCallback`; 22 callers. |
| 0x82464170 | sub_82464170 | FM2_CmdLine_ParseFloatKeyEqualsValue | 0.93 | `strnicmp` key prefix; requires `=`; `sscanf("%f")`; writes float out-param; 22 cmdline parsers. |
| 0x824BFDF0 | sub_824BFDF0 | FM2_Callback_InvokeOnceIfUnset | 0.90 | Lazy-invokes function pointer at `a3+4` once; caches result at `a3+16`; 22 Lua callback thunks. |
| 0x825D0B28 | sub_825D0B28 | FM2_RbTree_RotateLeftChildAt8 | 0.91 | Standard left rotation using child at node+8; parent-pointer fixups; shared by profile/db/hash trees; 22 callers. |
| 0x827BE9D0 | sub_827BE9D0 | FM2_RbTree_RotateRightChildAt8 | 0.91 | Mirror right rotation via child at node+8; paired with left rotate; 22 callers. |
| 0x82255440 | sub_82255440 | FM2_CarUpgrade_SetInstallStateRecordTickOnComplete | 0.89 | Stores install state at `+76`; when state==5 records `FM2_D3D_ReadKernelTickCountImport` at `+72`; 22 upgrade callers. |
| 0x8276C698 | sub_8276C698 | FM2_NetworkMessage_GetSlotRecordBase | 0.90 | Returns `base + 24*slotIndex` from `+384`; used to read per-slot payload fields; 23 network callers. |
| 0x82238510 | sub_82238510 | FM2_CarUpgrade_ApplyInstalledPartFieldChange | 0.90 | `CInstalledParts` vftable; SQL `SELECT EngineId From List_UpgradeEngine`; `UPDATE %s SET %s`; handles engine/wheel/tire fields; 25 callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8235ED30 | sub_8235ED30 | 56B thin wrapper → vector push helper only. |
| 0x827B06B8 | sub_827B06B8 | 56B vtable init + delegate; defer render base cluster. |
| 0x825C5CE0 | sub_825C5CE0 | Thin wrapper → hash lookup + stream write; defer cluster. |
| 0x82420548 | sub_82420548 | Single-line SQLite open-flag mask lookup; too thin alone. |
| 0x82225188 | sub_82225188 | Com static-lifetime init thunk; insufficient standalone evidence. |
| 0x8279D7E8 | sub_8279D7E8 | 40B wrapper → `sub_8279F5E0` only. |
| 0x82789188 | sub_82789188 | 40B wrapper → `sub_82790848` only. |
| 0x827B6740 | sub_827B6740 | 40B wrapper → `sub_827B6828` only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only; too thin alone. |
| 0x82466040 | sub_82466040 | Single `stvx` vec128 store; too trivial alone. |
| 0x827EB5C0 | sub_827EB5C0 | 16-byte memcpy key init; defer with element-link struct naming. |
| 0x827D68A8 | sub_827D68A8 | Material pass-flag binary search; defer with material cluster. |
| 0x8261BBC0 | sub_8261BBC0 | Stream write with flush/backpressure; defer zlib/stream cluster. |
| 0x8244F5C8 | sub_8244F5C8 | HashName bit-buffer encoder; defer with `sub_8244EEE0` cluster. |
| 0x8295C378 | sub_8295C378 | Large (~1KB) unanalyzed function; defer next pass. |
| 0x8295CCD8 | sub_8295CCD8 | Large unanalyzed function; defer next pass. |
| 0x827E55E0 | sub_827E55E0 | Large unanalyzed render function; defer next pass. |
| 0x824F3828 | sub_824F3828 | Large unanalyzed function; defer next pass. |
| 0x8270DA40 | sub_8270DA40 | SQLite correlation walker; defer expr cluster. |
| 0x82718B28 | sub_82718B28 | SQLite parse reduce helper; defer codegen cluster. |
| 0x82753E00 | sub_82753E00 | Jump thunk only. |
| 0x8275C958 | sub_8275C958 | Large wchar scanf engine; defer stdio cluster. |
