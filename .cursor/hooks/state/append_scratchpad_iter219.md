## Iteration 219

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82622F08 | sub_82622F08 | FM2_Lua_ReadBindingXmlPropertyByNameToStack | 0.91 | Reads variant string name; `ptrmapPutOvfl` type switch (bool/char/int16/uint32/uint8/float/string); pushes typed binding slots; fallback foreach children with profile-category protected call. |
| 0x82622A68 | sub_82622A68 | FM2_Lua_ForEachUnitStringChildReadXmlPropertyToCallback | 0.90 | Table-arg foreach; ptrmap type switch builds variants; `FM2_Lua_InvokeBindingPairUnderSlotGuard`; protected clear callback at end. |
| 0x826236F0 | sub_826236F0 | FM2_Lua_ReadBindingXmlPropertyWithTypeCheck | 0.91 | Entry wrapper: requires `FM2_Lua_IsNumberOrBooleanType()` else error 2; calls `FM2_Lua_ReadBindingXmlPropertyByNameToStack`. |
| 0x826208B0 | sub_826208B0 | FM2_Lua_InitBindingVariantCStringFromStdString | 0.90 | Sets variant type 4; copies MSVC `std::string` buffer pointer into variant slot. |
| 0x82620820 | sub_82620820 | FM2_Lua_InitBindingVariantDouble | 0.89 | Sets variant type 3 and stores double at `+8`. |
| 0x82620830 | sub_82620830 | FM2_Lua_InitBindingVariantMaterialTypeHandle | 0.89 | Sets variant type 4 with material-node `**(+4)` type handle; used when xml name is material prefix. |
| 0x826207D8 | sub_826207D8 | FM2_Lua_InitBindingVariantBool | 0.90 | Sets variant type 1 and bool byte at index slot `[2]`. |
| 0x82620878 | sub_82620878 | FM2_Lua_IsStackSlotNumberOrBooleanType | 0.89 | Thin but named entry gate: `FM2_Lua_IsNumberOrBooleanType()`; guards xml property read paths. |
| 0x82621870 | sub_82621870 | FM2_Lua_InvokeBindingDeferredMaterialPass0Recursive | 0.90 | Deferred field4 + `FM2_Render_DispatchMaterialPass0RecursiveOrInvalidate` on `*(a1+12)`. |
| 0x82622DA8 | sub_82622DA8 | FM2_Lua_ActivateBindingScriptByNameWithElementSlot | 0.90 | Script name + element slot from `a2`; `sub_82613EF0` network RB-tree activate path. |
| 0x82622E58 | sub_82622E58 | FM2_Lua_DeactivateBindingScriptByNameWithElementSlot | 0.90 | Same layout as activate but calls `sub_82614640` deactivate path. |
| 0x827E0540 | sub_827E0540 | FM2_Render_LookupMaterialNodePropertyPtrmapTypeByCategory | 0.88 | Resolves negative category id via sorted table/bitset; returns ptrmap type dword from material node `+8` table. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826247C8 | sub_826247C8 | Thin wrapper `sub_82624570(..., 0)`. |
| 0x8260FAB8 | sub_8260FAB8 | Thin wrapper `FM2_Lua_GetOrCreateObjectElementForBinding`. |
| 0x826206E8 | sub_826206E8 | Thin `FM2_Lua_SetStackTop(*a1, -1-a2)`. |
| 0x82620FF8 | sub_82620FF8 | Thin `FM2_DeferredTaskParams_GetField4` wrapper. |
| 0x827DEF38 | sub_827DEF38 | Thunk to `FM2_Render_DispatchMaterialPass0RecursiveOrInvalidate`. |
| 0x827DD108 | sub_827DD108 | Trivial deref `*a2`. |
| 0x82613EF0 | sub_82613EF0 | 972-byte binding-script activate RB-tree cluster; defer dedicated pass. |
| 0x82614640 | sub_82614640 | Pair with `sub_82613EF0` activate cluster. |
| 0x827D73E8 | sub_827D73E8 | Xml navigator parent-walk helper; defer material-node cluster. |
| 0x82620F68 | sub_82620F68 | Trivial element-slot equality compare. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
