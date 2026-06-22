## Iteration 217

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826226C8 | sub_826226C8 | FM2_Lua_ForEachUnitStringChildMatchingVariantInvokeCallback | 0.90 | Requires table stack slot; same child-match predicates as find variant; accumulates matches; `FM2_Lua_InvokeBindingPairUnderSlotGuard` + protected clear callback at end. |
| 0x82620840 | sub_82620840 | FM2_Lua_IsBindingVariantStackSlotTableType | 0.89 | Returns `FM2_Lua_GetStackValueType(...) == 5`; gates batch foreach path in `sub_826226C8`. |
| 0x82633980 | sub_82633980 | FM2_Lua_PushBindingCallbackArgAndInvokeAtIndex | 0.88 | Binding-slot guard; `FM2_Lua_SetBindingPairsTableFieldN(lua, -1, index)`; remove guard. |
| 0x827DDAB8 | sub_827DDAB8 | FM2_Profile_InvalidatePass10AndGetMaterialNodeChildId | 0.89 | `*(FM2_Render_InvalidateMaterialPass10AndSetProfileStrings(*slot)+8)`; 17 callers for unit-string child resolution. |
| 0x827DD8D0 | sub_827DD8D0 | FM2_Profile_DestroyElementAssetEntityRecordPlacementDelete | 0.90 | Vtable `[0]` at `off_82158474`; calls `FM2_Profile_DestroyElementAssetEntityRecord`; optional free. |
| 0x826215F8 | sub_826215F8 | FM2_Lua_DeactivateBindingEventByHashWithElementSlot | 0.90 | Like `FM2_Lua_DeactivateBindingEventByEventNameHash` but passes `*(a2+12)` as element-slot key in deactivate list. |
| 0x82621A58 | sub_82621A58 | FM2_Lua_PushUnitStringChildObjectElementByIndex | 0.90 | 1-based index into unit-string child array; bounds check; `sub_8260FAB8` object-element bind else `FM2_Lua_PushNil`. |
| 0x8254D4E0 | sub_8254D4E0 | FM2_Lua_SetBindingPairsTableFieldN | 0.89 | Resolves relative stack index; sets interned `"n"` field on binding-pairs table to integer `a3`; handles existing number field path. |
| 0x827DE768 | sub_827DE768 | FM2_Profile_ProcessDeferredListEnqueueSlot6AndDispatchSlot7 | 0.89 | Loop entity list `+196/+200`; if `+562`: `FM2_Profile_EnqueueDeferredTaskAndNotifySlot6` + `sub_827DD100` + slot 7 dispatch. |
| 0x827DE860 | sub_827DE860 | FM2_Profile_DrainDeferredTaskRingNotifySlot2 | 0.89 | While ring at `+208` non-empty: peek task, `FM2_Profile_EnqueueDeferredTaskNotifySlot2AndDispatch`, pop; respects byte `+224` guard. |
| 0x827DE8F8 | sub_827DE8F8 | FM2_Profile_DestroyManagerAndReleaseDeferredTasks | 0.90 | Reverse-walk deferred task list slot 2 dispatch; complete unit-string link; release manager; free entity list + 11 callback slot arrays. |
| 0x826216A8 | sub_826216A8 | FM2_Lua_GetOrCreateDeferredTaskParamsForBinding | 0.88 | Lazy init `a1[12]` via alloc 36 + `sub_82634990` from deferred field4 + binding string offset. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826247C8 | sub_826247C8 | Thin wrapper `sub_82624570(..., 0)`. |
| 0x8260FAB8 | sub_8260FAB8 | Thin wrapper `FM2_Lua_GetOrCreateObjectElementForBinding`. |
| 0x827DD108 | sub_827DD108 | Trivial deref `*a2`. |
| 0x82620800 | sub_82620800 | Small variant uint32 tag helper; defer with binding-slot cluster. |
| 0x826219F8 | sub_826219F8 | RB-tree sentinel alloc; defer with event-record cluster. |
| 0x82634890 | sub_82634890 | Unit-string parent walk; defer with resolver cluster. |
| 0x826217E8 | sub_826217E8 | Thin deferred-field deref only. |
| 0x82621828 | sub_82621828 | Thin xml-chain bool resolve only. |
| 0x82621900 | sub_82621900 | Thin pass-1 dispatch wrapper. |
| 0x82620C50 | sub_82620C50 | Duplicate uint32 binding-slot init. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
