## Iteration 205

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8261E8C8 | sub_8261E8C8 | FM2_Network_RbTree_DestroySubtreeAndFreeNodes | 0.91 | Post-order RB-tree walk; sentinel byte `+29`; `FM2_Lua_InvokeProtectedClearBindingCallback` at `+16`; `FM2_Memory_FreeSmallBlockOrNull`. |
| 0x8261E920 | sub_8261E920 | FM2_Network_RbTree_ClearTree | 0.90 | Calls destroy on root child; resets head links to self; used when erase hits tree root only child. |
| 0x8261EA50 | sub_8261EA50 | FM2_Network_RbTree_EraseNodeWithRebalance | 0.90 | STL `invalid map/set<T> iterator` guard; RB erase rebalance via `FM2_Stl_RedBlackTree_RotateLeft/Right`; color byte `+28`; decrements size; frees node. |
| 0x8261F098 | sub_8261F098 | FM2_Network_RbTree_EraseIteratorRange | 0.89 | Erases `[first,last)` network-tree iterators; root-only case calls `FM2_Network_RbTree_ClearTree`; else repeated `EraseNodeWithRebalance`. |
| 0x8261F198 | sub_8261F198 | FM2_Network_RbTreeFindOrEraseDwordKeyOrBindNegative | 0.88 | Non-negative key: `FM2_Network_RbTreeLowerBoundInsertHint` then erase on match; negative key: `FM2_Lua_SetBindingKeyNegativeIndexMarker` + guarded bind invoke. |
| 0x8261F380 | sub_8261F380 | FM2_Network_RbTreeContainer_Destroy | 0.90 | `EraseIteratorRange` over full tree then frees root block; nulls container head/size fields. |
| 0x827EC208 | sub_827EC208 | FM2_Render_NormalizeQuaternionInPlace | 0.92 | Normalizes 4-float quaternion; zero-norm → identity `(0,0,0,1)`; `__fsqrts` scale when length ≠ 1. |
| 0x827EC398 | sub_827EC398 | FM2_Render_BuildRotationMatrixFromProfileVariantAngles | 0.90 | Reads euler floats from profile variant array; swap/negate flags at `+32`; sin/cos via `FM2_FMOD_NormalizeSinLookupInput`; finishes with quaternion normalize. |
| 0x8261E440 | sub_8261E440 | FM2_Lua_IsStackTopDeepEqualToBindingSlot | 0.91 | Dual binding-slot guards; `_cntlzw` on `FM2_Lua_AreStackSlotsDeepEqual(lua,-1)`; 4 Lua binding callers. |
| 0x8261E540 | sub_8261E540 | FM2_Lua_RemoveTrackedStackSlotAndReset | 0.90 | `FM2_Lua_RemoveStackSlotAtIndex` at tracked index; clears `a1[2]`; thunked by `sub_82630BD0`. |
| 0x8261E608 | sub_8261E608 | FM2_Lua_InitEmptyBindingSlotWithClaimedNil | 0.90 | Init slot `{lua,-1}`; push nil + `FM2_Lua_ClaimStackTopAsBindingOwner`; pop to restore stack. |
| 0x824B6BE0 | sub_824B6BE0 | FM2_Lua_AreStackSlotsDeepEqual | 0.91 | Compares two stack slots via type tag `+8` then `lua_value_equal_deep`; nil-sentinel short-circuit. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8261E978 | sub_8261E978 | Lua table walk + IO dispatch stub; needs file-op context. |
| 0x8261E0D0 | sub_8261E0D0 | Large scene-graph XML export walker; defer with `sub_82617E28` cluster. |
| 0x827DD108 | sub_827DD108 | 8B deref `return *a2` only. |
| 0x827E47D0 | sub_827E47D0 | Thin wrapper → `sub_827F4DB0`. |
| 0x827E53D8 | sub_827E53D8 | Material resolve + profile callback; defer with `sub_827EAD10` hash lookup. |
| 0x827EAD10 | sub_827EAD10 | 307-bucket hash scan; needs paired insert/alloc naming. |
| 0x82630BD0 | sub_82630BD0 | 4B thunk → `FM2_Lua_RemoveTrackedStackSlotAndReset`. |
| 0x8261F988 | sub_8261F988 | Thin `FM2_Lua_GetStackDepth` wrapper. |
| 0x827EC5A0 | sub_827EC5A0 | Thin ctor wrapper → rotation-matrix builder. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
