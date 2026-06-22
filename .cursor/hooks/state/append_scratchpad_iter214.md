## Iteration 214

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82621DA8 | sub_82621DA8 | FM2_Render_BindingScriptRbTreeLowerBoundByName | 0.91 | Classic RB-tree lower_bound walk using `FM2_SceneGraph_CompareNodeNamePrefix` on node `+12`; callee of iterator helper. |
| 0x826220A8 | sub_826220A8 | FM2_Render_BindingScriptRbTreeIteratorLowerBoundByName | 0.90 | Wraps lower_bound into `{tree, node}` iterator pair; used by `FM2_Render_FindBindingScriptRbTreeIteratorByName`. |
| 0x827DE1C0 | sub_827DE1C0 | FM2_Profile_PeekBackDeferredTaskFromRingBuffer | 0.89 | Computes `writeIndex-1` on ring at `a1+12/+16`; returns task ptr via `FM2_Profile_GetDeferredTaskPtrAtRingIndex`; used by slot-7 dispatch. |
| 0x827DDC20 | sub_827DDC20 | FM2_Profile_GetDeferredTaskPtrAtRingIndex | 0.88 | Ring slab index math: `slab[row] + 4*(index&3)`; bounds checks against queue capacity. |
| 0x82624368 | sub_82624368 | FM2_Lua_ApplyBindingScriptMutationByVariantType | 0.90 | Reads name + value variants; unregister script; switch on type: bool pass-flag dispatch, float cache foreach, table register, string profile bind, nil invalidate. |
| 0x82624570 | sub_82624570 | FM2_Lua_HandleBindingScriptMutationWithTypeCoercion | 0.91 | Parses optional type prefix (`boolean`/`float`/`long`/`string`); coerces Lua stack slot; calls mutation; validates table arg. |
| 0x826234C8 | sub_826234C8 | FM2_Lua_PushBindingSlotFromStringByTargetType | 0.90 | String→binding-slot coercion by target type id (2/5/9/14); parses `true` for bool; uses string-to-double for numeric types. |
| 0x827DDE38 | sub_827DDE38 | FM2_Profile_InvokeMaterialPassCallbacksForEntityPtrList | 0.89 | Slot 8 pre-pass; loop entities with `+562` flag → slot 9; slot 10 post-pass; default list at `result+196`. |
| 0x827DE580 | sub_827DE580 | FM2_Profile_ProcessFlaggedEntityListAndFlushUnitLinks | 0.89 | For `+562` entities: enqueue deferred task, notify hooks, dispatch slot 7; ends with `FM2_Render_FlushPendingUnitStringLinkQueue`. |
| 0x827DDFE0 | sub_827DDFE0 | FM2_Profile_CreateEntityRecordAndNotifySlot1 | 0.88 | Alloc 564-byte record via `sub_827DD920`; append to list at `a1+196`; `FM2_Profile_InvokeCallbacksBySlotIndex(a1, 1, record)`. |
| 0x82630BD8 | sub_82630BD8 | FM2_Lua_DestroyElementAssetBindingObjectDtor | 0.90 | Vtable `off_82113260` ("element"/"asset"); frees 8 owned pointers `+56..+84`; calls `FM2_Lua_DestroyBindingObjectAndScriptTree`. |
| 0x826337A8 | sub_826337A8 | FM2_Lua_DestroyUndefinedBindingObjectAndUnitStringLink | 0.90 | Vtable `off_82113B5C` ("[undefined]"); if flag `+52` completes pending unit-string link; then base binding destroy. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826247C8 | sub_826247C8 | Thin wrapper: `sub_82624570(a1,a2,a3,0)`. |
| 0x827DDB08 | sub_827DDB08 | Import thunk `j_FM2_Render_CompleteUnitStringLinkAndDestroyPending`. |
| 0x827DD100 | sub_827DD100 | Thin wrapper `sub_827EE6E0(*(a1+536))`. |
| 0x82630FE0 | sub_82630FE0 | Placement-delete wrapper around element-asset dtor. |
| 0x82631670 | sub_82631670 | Smaller element-asset dtor variant; defer with `off_82113400` cluster. |
| 0x82631B58 | sub_82631B58 | Placement-delete wrapper for `sub_82631670`. |
| 0x826338C0 | sub_826338C0 | Placement-delete wrapper for undefined-binding dtor. |
| 0x827DDF70 | sub_827DDF70 | Generic `FM2_STL_IntVector_ResizeZeroed` append; thin helper. |
| 0x827DE658 | sub_827DE658 | Complex deferred-entity loop; needs `sub_827E4C40` context. |
| 0x826349E0 | sub_826349E0 | Thin `FM2_Stl_String_InitOrClear(a1+8)`. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
