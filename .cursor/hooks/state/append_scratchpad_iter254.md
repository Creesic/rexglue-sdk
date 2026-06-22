## Iteration 254

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8262DD10 | sub_8262DD10 | FM2_Lua_CLuaObjectElementGetParent | 0.92 | Property getter `parent`; resolves via `FM2_Lua_ResolveBindingObjectFromDeferredTaskParams`. |
| 0x8262DD78 | sub_8262DD78 | FM2_Lua_CLuaObjectElementToString | 0.93 | Registered as `tostring`; `ToString` / `FM2_Lua_FormatBindingProfileDiagnosticString`. |
| 0x8262DE40 | sub_8262DE40 | FM2_Lua_CLuaObjectElementOperatorEquals | 0.92 | Registered as `.eq`; compares two `CLuaObjectElement`; `operator==` error string. |
| 0x8262DF48 | sub_8262DF48 | FM2_Lua_CLuaObjectElementGet | 0.92 | Registered as `get`; `FM2_Lua_ReadBindingXmlPropertyWithTypeCheck`. |
| 0x8262E018 | sub_8262E018 | FM2_Lua_CLuaObjectElementSet | 0.92 | Registered as `set`; `FM2_Lua_HandleBindingScriptMutationSkipTypePrefix`. |
| 0x8262E0F0 | sub_8262E0F0 | FM2_Lua_CLuaObjectElementSetAsType | 0.92 | Registered as `setAsType`; type string arg; `HandleBindingScriptMutationWithTypeCoercion`. |
| 0x8262E1F8 | sub_8262E1F8 | FM2_Lua_CLuaObjectElementGetNumChildren | 0.93 | Registered as `getNumChildren`; `FM2_Render_CountUnitStringTableChildren`. |
| 0x8262E2D0 | sub_8262E2D0 | FM2_Lua_CLuaObjectElementGetChildByIndex | 0.92 | Registered as `getChildByIndex`; index float; `PushUnitStringChildObjectElementByIndex`. |
| 0x8262E3E0 | sub_8262E3E0 | FM2_Lua_CLuaObjectElementGetChildWithValue | 0.92 | Registered as `getChildWithValue`; string key + variant stack index. |
| 0x8262E4E0 | sub_8262E4E0 | FM2_Lua_CLuaObjectElementGetChildrenWithValue | 0.92 | Registered as `getChildrenWithValue`; multi-child variant match. |
| 0x8262E5F8 | sub_8262E5F8 | FM2_Lua_CLuaObjectElementGetChildValuesFor | 0.92 | Registered as `getChildValuesFor`; `ForEachUnitStringChildReadXmlPropertyToCallback`. |
| 0x8262E6F8 | sub_8262E6F8 | FM2_Lua_CLuaObjectElementGetTimeContext | 0.93 | Registered as `getTimeContext`; returns `CLuaTimeContext` userdata. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8262E7C8 | sub_8262E7C8 | Element `clone`; defer with fireEvent/processEvent cluster. |
| 0x8262E8E8 | sub_8262E8E8 | Element `fireEvent`; defer event-handler cluster. |
