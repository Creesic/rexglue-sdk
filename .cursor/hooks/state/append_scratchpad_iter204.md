## Iteration 204

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8261EE50 | sub_8261EE50 | FM2_Network_RbTreeInsertNodeWithRebalance | 0.91 | STL map insert + RB rebalance via `FM2_Stl_RedBlackTree_RotateLeft/Right`; sentinel byte `+28`; callee of `FM2_Network_RbTreeFindOrInsertDwordKey`. |
| 0x8261E7A0 | sub_8261E7A0 | FM2_Network_RbTree_InitNodeWithDwordKey | 0.90 | Sets tree links; stores dword key at `+12`; copies binding slot via `FM2_Lua_CopyBindingSlotFromSource`; color byte `+28/+29`. |
| 0x827E1CA0 | sub_827E1CA0 | FM2_Profile_GetOrCreateVariantRecordForNegativeId | 0.90 | Negative id → category byte lookup or `FM2_Profile_LookupSortedDwordKeyRecord`; creates via `FM2_Profile_UpsertSortedDwordKeyRecord48` if missing. |
| 0x827E0320 | sub_827E0320 | FM2_Profile_WriteCategoryVariantSimdBlock | 0.89 | Writes 64-byte SIMD variant block to profile offset table; updates category bitmask at `+16`; called from get-or-create path. |
| 0x8261E678 | sub_8261E678 | FM2_Lua_CopyBindingSlotFromSource | 0.90 | Copies binding slot under guard via `FM2_Lua_InitBindingSlotGuard` + `FM2_Lua_ClaimStackTopAsBindingOwner`. |
| 0x8261E6E0 | sub_8261E6E0 | FM2_Lua_PushStackSlotAndClaimBinding | 0.90 | Copies stack slot to top then `FM2_Lua_ClaimStackTopAsBindingOwner`; used by binding-slot init. |
| 0x8261E590 | sub_8261E590 | FM2_Lua_ClaimStackTopAsBindingOwner | 0.91 | Records stack value type; pushes binding owner lightuserdata; `FM2_Lua_InvokeProtectedCall32` on registry. |
| 0x8261E748 | sub_8261E748 | FM2_Lua_InitBindingSlotFromStackIndex | 0.89 | Initializes `{luaState, -1}` slot; calls `FM2_Lua_PushStackSlotAndClaimBinding` for stack index. |
| 0x8261E810 | sub_8261E810 | FM2_Lua_BuildBindingKeyPairFromSlot | 0.90 | Stores binding key id + copies slot; clears protected binding callback; used after object-element create. |
| 0x827E43E0 | sub_827E43E0 | FM2_Render_BuildMaterialMatrixFromAxesAndTranslation | 0.90 | Builds axis matrix from `a4/a5` vectors; `FM2_Render_MultiplyGMatrix3DInPlaceSimd`; adds translation `a2`; used in material local matrix path. |
| 0x82620DE8 | sub_82620DE8 | FM2_Lua_GetMetatableTostringOrCheckNumber | 0.91 | Invokes metatable `tostring`; returns C string or validates stack slot is number; 12 Lua binding callers. |
| 0x82621B58 | sub_82621B58 | FM2_Render_RecursiveDispatchMaterialPassOnUnitSubtree | 0.88 | Recurses unit-string children; `FM2_Render_DispatchMaterialPassByNegativeSlot(17)`; links parent material via `FM2_Render_GetParentMaterialNodeAtSlot`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8261E920 | sub_8261E920 | Network RB-tree clear helper; defer erase/clear cluster naming. |
| 0x8261E978 | sub_8261E978 | Lua table walk + `FM2_LuaIo_DispatchFileOpStub`; needs IO stub context. |
| 0x827EC398 | sub_827EC398 | Euler→rotation matrix from profile variant; defer with `sub_827EC208` cluster. |
| 0x827DD170 | sub_827DD170 | Thin dispatch to `sub_827EF3D0` by slot index only. |
| 0x826108D0 | sub_826108D0 | Material node resolve wrapper; defer render lookup cluster. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine; defer sub-callees. |
| 0x8261F988 | sub_8261F988 | Thin wrapper → `FM2_Lua_GetStackDepth` only. |
| 0x824F0160 | sub_824F0160 | 8B offset helper `a1+12` only. |
| 0x8279D7E8 | sub_8279D7E8 | Wrapper chain → `sub_8279F5E0` → `sub_827A0400`. |
| 0x821F1B70 | sub_821F1B70 | Pure VMX128 math kernel; no strings/callees for semantic name. |
