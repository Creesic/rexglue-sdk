## Iteration 203

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x824BE490 | sub_824BE490 | FM2_Lua_FindTableSlotIndexForNextKey | 0.91 | Resolves `next` key index via `FM2_Lua_FindTableSlotForValue_0`; throws `"invalid key to 'next'"`; feeds `FM2_Lua_AdvanceTableNextKeyValue`. |
| 0x824BE5A0 | sub_824BE5A0 | FM2_Lua_AdvanceTableNextKeyValue | 0.90 | Advances Lua table iterator; writes next key/value TV pairs; called by `FM2_Lua_TableTraverseNextEntry`. |
| 0x82620670 | sub_82620670 | FM2_Lua_PushValueFromBindingVariant | 0.91 | Dispatches binding variant tag: nil/bool/lightuserdata/number/string/stack-slot copy; used in binding invoke paths. |
| 0x82620D70 | sub_82620D70 | FM2_Lua_InitBindingSlotGuard | 0.90 | Saves Lua state + stack depth + binding field4; calls `FM2_Lua_PushBindingContextLightUserdata`; RAII guard for binding ops. |
| 0x82620ED8 | sub_82620ED8 | FM2_Lua_PushObjectElementUserdataToStack | 0.92 | Wrapper → `FM2_Lua_ToluaPushUserdataFromUbox`; pushes created object-element to Lua stack. |
| 0x82620E68 | sub_82620E68 | FM2_Lua_ToluaCheckAndGetObjectElementPtr | 0.90 | `FM2_Lua_ToluaBeginTypeCheckAccess` for `"CLuaObjectElement"`; returns userdata pointer or 0. |
| 0x826207F0 | sub_826207F0 | FM2_Lua_SetBindingKeyNegativeIndexMarker | 0.89 | Sets binding key struct `type=2` and stores negative-index value; negative binding path in `GetOrCreateObjectElement`. |
| 0x82630B08 | sub_82630B08 | FM2_Lua_TryReadBindingValueFromStackSlot | 0.90 | Under slot guard: pushes binding key, reads stack slot into 16-byte variant; returns whether value present. |
| 0x82630A98 | sub_82630A98 | FM2_Lua_InvokeBindingPairUnderSlotGuard | 0.90 | Pushes key+value variants; `FM2_Lua_InvokeProtectedCall32`; removes binding slot guard. |
| 0x82630F68 | sub_82630F68 | FM2_Lua_CtorObjectElementSceneNode | 0.91 | `FM2_Lua_InitObjectElementFromBinding`; sets vtables `off_82113260`/`off_821138DC`; clears fields `+56..+84`. |
| 0x82632648 | sub_82632648 | FM2_Lua_CtorObjectElementSceneNodeExtended | 0.90 | Calls scene-node ctor then overrides vtable to `off_821138E0`; used for `dword_82A07058` type path. |
| 0x8261F3E8 | sub_8261F3E8 | FM2_Lua_GetOrCreateObjectElementForBinding | 0.89 | Main factory: negative-index fast path, RB-tree lookup/insert, alloc 56/88-byte elements, GC hook + userdata push. |
| 0x8261E4D8 | sub_8261E4D8 | FM2_Lua_PushBindingContextLightUserdata | 0.91 | Pushes binding-context pointer to registry; records stack depth at `a1[2]`; used by slot guards. |
| 0x827D6910 | sub_827D6910 | FM2_Render_GetParentMaterialNodeAtSlot | 0.91 | Returns linked child material handle `*(*(a1+4*(slot+4))+4)`; used in matrix cache recursion. |
| 0x827D7228 | sub_827D7228 | FM2_Render_UpdateMaterialPassAndForEachCacheEntry | 0.90 | `FM2_Render_LookupMaterialPassFlagByteByNegativeId`; `sub_827E1CA0`; `FM2_Render_ResourceCache_ForEachMatchingEntry`. |
| 0x827E09D0 | sub_827E09D0 | FM2_Profile_GetCategoryPassFlagByte | 0.89 | Bitmask test on profile category flags or sorted variant byte lookup; gates material matrix build. |
| 0x827E4210 | sub_827E4210 | FM2_Render_MultiplyGMatrix3DInPlaceSimd | 0.90 | VMX128 4x4 `GMatrix3D` multiply in place; used when composing parent/child material transforms. |
| 0x827DECF8 | sub_827DECF8 | FM2_Render_BuildMaterialLocalMatrixFromElementPasses | 0.88 | Builds `D3DXMATRIX` via element callbacks passes 13/21; `sub_827E43E0`; caches via `FM2_Profile_ResolveCategoryVariantDataPtr`. |
| 0x827E1B50 | sub_827E1B50 | FM2_Profile_ResolveCategoryVariantDataPtr | 0.89 | Resolves variant data pointer from bitmask or sorted dword record; returns pool alloc or default `a3`. |
| 0x8261F270 | sub_8261F270 | FM2_Network_RbTreeFindOrInsertDwordKey | 0.90 | Network RB-tree `lower_bound` on dword key at `+12`; inserts via `sub_8261EE50`; sentinel byte `+29`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8261F988 | sub_8261F988 | Thin wrapper → `FM2_Lua_GetStackDepth` only. |
| 0x8261E748 | sub_8261E748 | Thin binding-slot iterator init; defer with `sub_8261E6E0` cluster. |
| 0x8261E810 | sub_8261E810 | Thin wrapper copying binding slot + clear callback. |
| 0x8261EE50 | sub_8261EE50 | Network RB-tree insert rebalance; defer paired erase/iterator cluster. |
| 0x827E1CA0 | sub_827E1CA0 | Profile variant get-or-create helper; defer profile write cluster. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine; defer until sub-callees named. |
| 0x824F0160 | sub_824F0160 | 8B offset helper `a1+12` only. |
| 0x8279D7E8 | sub_8279D7E8 | Wrapper chain → `sub_8279F5E0` → `sub_827A0400`. |
| 0x827B6740 | sub_827B6740 | Thin wrapper → `FM2_Render_StateVector_ClearAndFree40` only. |
| 0x821F1B70 | sub_821F1B70 | Pure VMX128 math kernel; no strings/callees for semantic name. |
