## Iteration 255

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8262E7C8 | sub_8262E7C8 | FM2_Lua_CLuaObjectElementClone | 0.92 | Registered as `clone`; `DispatchRecursiveMaterialPassOnUnitSubtreeWithElements`. |
| 0x8262E8E8 | sub_8262E8E8 | FM2_Lua_CLuaObjectElementFireEvent | 0.93 | Registered as `fireEvent`; `CLuaObjectEvent` arg; `DispatchBindingEventWithProfileFieldFromTask`. |
| 0x8262E9E0 | sub_8262E9E0 | FM2_Lua_CLuaObjectElementProcessEvent | 0.92 | Registered as `processEvent`; `DispatchDeferredBindingEventFromTask`. |
| 0x8262EAD8 | sub_8262EAD8 | FM2_Lua_CLuaObjectElementRegisterForEvent | 0.92 | `registerForEvent` overload with target element; `InvokeBindingScriptFromDeferredTaskParams`. |
| 0x8262EC18 | sub_8262EC18 | FM2_Lua_CLuaObjectElementRegisterForEventByNumber | 0.92 | `registerForEvent` numeric overload; falls back to element overload; `InvokeBindingScriptByEventName`. |
| 0x8262ED40 | sub_8262ED40 | FM2_Lua_CLuaObjectElementUnregisterForEvent | 0.92 | `unregisterForEvent` element overload; `DeactivateBindingEventByHashWithElementSlot`. |
| 0x8262EE80 | sub_8262EE80 | FM2_Lua_CLuaObjectElementUnregisterForEventByNumber | 0.92 | `unregisterForEvent` numeric overload; `DeactivateBindingEventByEventName`. |
| 0x8262EFA8 | sub_8262EFA8 | FM2_Lua_CLuaObjectElementRegisterForChange | 0.92 | Registered as `registerForChange`; `ActivateBindingScriptByNameWithElementSlot`. |
| 0x8262F0E8 | sub_8262F0E8 | FM2_Lua_CLuaObjectElementUnregisterForChange | 0.92 | Registered as `unregisterForChange`; `DeactivateBindingScriptByNameWithElementSlot`. |
| 0x8262F228 | sub_8262F228 | FM2_Lua_CLuaObjectElementGetCustomProperty | 0.92 | Registered as `get_customproperty`; `ReadBindingXmlPropertyByNameToStack`. |
| 0x8262F2F8 | sub_8262F2F8 | FM2_Lua_CLuaObjectElementSetCustomProperty | 0.92 | Registered as `set_customproperty`; `ApplyBindingScriptMutationByVariantType`. |
| 0x82630050 | sub_82630050 | FM2_Lua_AkFireEvent | 0.93 | Registered in `FM2_Lua_RegisterAkToluaBindings` as `fireEvent` on `CLuaObjectEvent`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8262FF10 | sub_8262FF10 | Ak `output` handler; defer Ak module cluster. |
| 0x82630100 | sub_82630100 | Ak `createElement`; defer Ak element lifecycle cluster. |
