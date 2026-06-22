## Iteration 213

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82624230 | sub_82624230 | FM2_Lua_DestroyBindingObjectAndScriptTree | 0.91 | Binding-object dtor body: walks script RB-tree, `FM2_Lua_DeactivateBindingEventAndMoveToPendingList` per node, `FM2_Render_DestroyBindingScriptRbTreeContainer`, string clear; vtable `off_8210F5D0`. |
| 0x82624520 | sub_82624520 | FM2_Lua_DestroyBindingObjectPlacementDelete | 0.90 | Vtable entry `[0]` at `off_8210F5D0`; calls destroy body; optional `FM2_Memory_FreeSmallBlockOrNull` when flag bit0 set (placement delete). |
| 0x826240D0 | sub_826240D0 | FM2_Render_DestroyBindingScriptRbTreeContainer | 0.90 | Erases full tree via `FM2_Render_EraseBindingScriptRbTreeIteratorRange`; frees sentinel node; zeros size fields. |
| 0x82623FD0 | sub_82623FD0 | FM2_Render_EraseBindingScriptRbTreeIteratorRange | 0.89 | STL map erase-range: clear-all if end==begin else loop `FM2_Render_EraseBindingScriptRbTreeNodeWithRebalance`; returns iterator pair. |
| 0x82623758 | sub_82623758 | FM2_Render_EraseBindingScriptRbTreeNodeWithRebalance | 0.91 | RB-tree erase with `"invalid map/set<T> iterator"` guard; rotations; `FM2_Lua_ResetBindingStateAndInvokeClear` + node free. |
| 0x82623DF0 | sub_82623DF0 | FM2_Lua_UnregisterBindingScriptByNameFromRbTree | 0.90 | Iterates name matches via find-iterator; deactivate event + erase node until end sentinel; used on script unregister. |
| 0x82622108 | sub_82622108 | FM2_Render_FindBindingScriptRbTreeIteratorByName | 0.89 | lower_bound + `FM2_SceneGraph_CompareNodeNamePrefix`; returns iterator pair for script-name lookup. |
| 0x827DDCE8 | sub_827DDCE8 | FM2_Profile_InvokeCallbacksBySlotIndex | 0.88 | Loops `12*a2+result` callback table at `+28/+32`; indirect call per 12-byte entry; 7 callers across profile/deferred paths. |
| 0x827DE528 | sub_827DE528 | FM2_Profile_EnqueueDeferredTaskAndNotifySlot6 | 0.89 | `FM2_Profile_PushDeferredTaskToRingBuffer(a1+160)` then `FM2_Profile_InvokeCallbacksBySlotIndex(a1, 6, task)`. |
| 0x827DE268 | sub_827DE268 | FM2_Profile_DispatchDeferredTaskFromQueueSlot7 | 0.89 | If queue non-empty: peek-back task, dispatch slot 7 callbacks, decrement `+176` pending count. |
| 0x82610018 | sub_82610018 | FM2_Profile_RemoveManagerBindingOnEventDeactivate | 0.90 | Called from deactivate path; `FM2_Profile_RemoveManagerBindingByHashedName` with lap-tracker field532 + hashed name. |
| 0x825891A8 | sub_825891A8 | FM2_Profile_PushDeferredTaskToRingBuffer | 0.88 | Ring-buffer push at `result+160` layout: grow slab if needed, store dword task id, increment write index. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826349E0 | sub_826349E0 | Thin wrapper: `FM2_Stl_String_InitOrClear(a1+8)`. |
| 0x82630BD8 | sub_82630BD8 | Derived-class dtor shell; needs `off_82113260` cluster context. |
| 0x827DE1C0 | sub_827DE1C0 | Peek-back helper only; defer with ring-buffer cluster. |
| 0x827DDC20 | sub_827DDC20 | Low-level ring index→pointer; thin accessor. |
| 0x826220A8 | sub_826220A8 | RB-tree lower_bound only; pair with find-iterator next iter. |
| 0x827E4968 | sub_827E4968 | Thin `FM2_XmlParser_CreateNodeIfNeeded` wrapper. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
