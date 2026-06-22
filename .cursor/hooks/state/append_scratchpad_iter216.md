## Iteration 216

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82621F28 | sub_82621F28 | FM2_Lua_DispatchRecursiveMaterialPassOnUnitSubtreeWithElements | 0.91 | Optional `CLuaObjectElement` start/end ids or unit-string lookup; `FM2_Render_RecursiveDispatchMaterialPassOnUnitSubtree`; profile object-element bind. |
| 0x82622370 | sub_82622370 | FM2_Lua_FindUnitStringChildMatchingVariantValue | 0.90 | Walks unit-string child index range; bool/float/string/material-type predicate via ptrmap/xml; binds object element on match else `FM2_Lua_PushNil`. |
| 0x82621998 | sub_82621998 | FM2_Lua_ParseStlStringToDoubleOrZero | 0.89 | Reads MSVC `std::string` buffer; `FM2_Utility_vswprintf_s` parse; returns 0.0 when parse flag set; used by string coercion path. |
| 0x82630FE0 | sub_82630FE0 | FM2_Lua_DestroyElementAssetBindingObjectPlacementDelete | 0.90 | Vtable `[0]` at `off_82113260`; calls `FM2_Lua_DestroyElementAssetBindingObjectDtor`; optional free when flag bit0. |
| 0x826338C0 | sub_826338C0 | FM2_Lua_DestroyUndefinedBindingObjectPlacementDelete | 0.90 | Vtable `[0]` at `off_82113B5C`; calls `FM2_Lua_DestroyUndefinedBindingObjectAndUnitStringLink`; optional free. |
| 0x827E4B48 | sub_827E4B48 | FM2_Render_SetMaterialNodeProfileCategoryString | 0.90 | Appends optional C string then `sub_827D7398(materialNode, 5)`; called from pass-10 invalidate helper. |
| 0x827E4BE8 | sub_827E4BE8 | FM2_Render_SetMaterialNodeProfileNameString | 0.90 | If name provided: ctor string + `sub_827D7398(materialNode, 3)`; used in undefined-binding init. |
| 0x827D7398 | sub_827D7398 | FM2_Render_ForEachResourceCacheByPassFlagByte | 0.89 | `FM2_Render_LookupMaterialPassFlagByteByNegativeId`; `sub_827E32C0`; `FM2_Render_ResourceCache_ForEachMatchingEntry`. |
| 0x826221A0 | sub_826221A0 | FM2_Lua_FormatBindingProfileDiagnosticString | 0.88 | Builds diagnostic string: deferred field + profile category lookup + `snprintf("%ld")` + `": "` suffix pieces. |
| 0x82621588 | sub_82621588 | FM2_Lua_DeactivateBindingEventByEventNameHash | 0.90 | Requires event hash `a3`; `FM2_Lua_DeactivateBindingEventAndMoveToPendingList` with script loader from slot `a4`. |
| 0x827DD778 | sub_827DD778 | FM2_Profile_DestroyElementAssetEntityRecord | 0.91 | Entity dtor `off_82158474`: flush unit-string links; release 128-slot callback array; free transports/subsystems at `+520..+540`. |
| 0x826247D0 | sub_826247D0 | FM2_Lua_ToluaCtorCColorFromRgbaComponents | 0.91 | Tolua `new` for `"CColor"`: 4 number args default 255; alloc 12-byte color; `FM2_Lua_ToluaPushUserdataAndRegisterGc`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826247C8 | sub_826247C8 | Thin wrapper `sub_82624570(..., 0)`. |
| 0x827DDAB8 | sub_827DDAB8 | Thin wrapper: `*(FM2_Render_InvalidateMaterialPass10AndSetProfileStrings(...)+8)`. |
| 0x827DD8D0 | sub_827DD8D0 | Placement-delete wrapper around `FM2_Profile_DestroyElementAssetEntityRecord`. |
| 0x827DD108 | sub_827DD108 | Trivial deref `*a2` only. |
| 0x82620840 | sub_82620840 | One-liner table-type check; defer with table-binding cluster. |
| 0x82633980 | sub_82633980 | Thin callback push via `sub_8254D4E0`. |
| 0x826226C8 | sub_826226C8 | 928-byte batch variant of find/match; defer next iter. |
| 0x82634468 | sub_82634468 | Tolua arg-check helper wrapper. |
| 0x82620F68 | sub_82620F68 | Trivial compare `*(a2+12)==*(a1+12)`. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
