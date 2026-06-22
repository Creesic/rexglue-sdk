## Iteration 206

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8261E978 | sub_8261E978 | FM2_Lua_FlushBindingTablePairsToIoDispatch | 0.89 | Optional `FM2_Network_RbTree_ClearTree`; walks binding table via `FM2_Lua_TableTraverseNextEntry` into upvalue; ends with `FM2_LuaIo_DispatchFileOpStub(...,2)`. |
| 0x8261F810 | sub_8261F810 | FM2_Lua_DestroyBindingNetworkContext | 0.90 | Calls flush/io path then `FM2_Lua_InvokeProtectedClearBindingCallback` + `FM2_Network_RbTreeContainer_Destroy`; 2 binding teardown callers. |
| 0x8261F850 | sub_8261F850 | FM2_Lua_InitBindingNetworkRbTreeContext | 0.90 | Stores lua state; `FM2_IntrusiveList_InitSentinelHead`; inits weak-value registry via `FM2_Lua_InitBindingWeakValueRegistryTable`. |
| 0x826309D0 | sub_826309D0 | FM2_Lua_InitBindingWeakValueRegistryTable | 0.91 | Empty binding slot + push closure env table; sets `__mode`=`v`; `FM2_Lua_ClaimStackTopAsBindingOwner`. |
| 0x8261F8A8 | sub_8261F8A8 | FM2_Lua_RegisterAnarkErrorHandlerGlobals | 0.92 | Pushes LString `"anark_error_handler"`; `FM2_Lua_RegisterBindingPairsModuleTail` on globals. |
| 0x8261F8F8 | sub_8261F8F8 | FM2_Lua_SetBindingVariantToMetatableField | 0.91 | `FM2_Lua_PushValueFromBindingVariant` then `FM2_Lua_SetFieldFromCString` on `LUA_ENVIRONINDEX`; 7 binding callers. |
| 0x8261FA38 | sub_8261FA38 | FM2_Lua_DestroyBindingScriptContext | 0.89 | `FM2_LuaIo_DispatchFileOpStub`; drains Lua GC via `sub_824B9E70`; clears STL string at `a1+2`. |
| 0x82620100 | sub_82620100 | FM2_Lua_InitBindingScriptContext | 0.90 | Opens lua/table/string/math libs by flag mask; registers gc finalizer `sub_826200D0`, `"anark_error_handler"`, `FM2_Lua_LoadScriptFromBuffer`. |
| 0x827EAD10 | sub_827EAD10 | FM2_Render_LookupMaterialHashBucketByDwordKey | 0.91 | 307-bucket hash (`abs32(key%307)+3`); linear probe vector; match on node field `+4`; 6 material-resolve callers. |
| 0x827E53D8 | sub_827E53D8 | FM2_Render_ResolveMaterialNodeByDwordKeyAndNotify | 0.90 | Hash lookup + `sub_827E47D0` material factory; optional `FM2_Profile_InvokeManagerCallbacksUnderCritSec`; used by scene material walker. |
| 0x827E5310 | sub_827E5310 | FM2_Render_GetMaterialNodeByDwordKeyAndNotify | 0.89 | Direct dword-key hash lookup + `sub_827E47C0` resolve; optional profile manager callback. |
| 0x826108D0 | sub_826108D0 | FM2_Render_ResolveMaterialUnitSlotFromSceneNode | 0.88 | Calls `FM2_Render_ResolveMaterialNodeByDwordKeyAndNotify`; if flag `0x10` on node `+20`, runs `sub_827D78E8` pair + `sub_826105F0`; returns slot `+8`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8261F990 | sub_8261F990 | Thin wrapper → `FM2_Lua_ErrorVprintf` only. |
| 0x8261FF78 | sub_8261FF78 | Custom print via metatable tostring; defer with gc finalizer naming. |
| 0x826200D0 | sub_826200D0 | 48B gc finalizer thunk → `sub_8261FF78`. |
| 0x827E52A8 | sub_827E52A8 | Type-name hash variant of material lookup; defer with `sub_827EACC8`. |
| 0x827EACC8 | sub_827EACC8 | Thin `FM2_Xml_GetTypeHandleFromNameBuffer` wrapper. |
| 0x827E47C0 | sub_827E47C0 | 16B wrapper → `sub_827F5590`. |
| 0x827E51C8 | sub_827E51C8 | Material resolve without hash lookup step; thin over `sub_827E47C0`. |
| 0x827E5218 | sub_827E5218 | Two-call wrapper only. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch; needs sub-ctor cluster. |
| 0x8261FA88 | sub_8261FA88 | Script load/append helper; defer with `sub_82620580`. |
| 0x82630BD0 | sub_82630BD0 | 4B thunk (already named target). |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
