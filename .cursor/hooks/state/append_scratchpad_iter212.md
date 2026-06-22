## Iteration 212

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82623ED0 | sub_82623ED0 | FM2_Render_LookupOrInsertBindingScriptRbTreeByNamePrefix | 0.90 | RB-tree lower_bound by `FM2_SceneGraph_CompareNodeNamePrefix`; insert via `FM2_Render_InsertBindingScriptRbTreeNodeWithRebalance`; used when registering table-variant scripts. |
| 0x82623B68 | sub_82623B68 | FM2_Render_InsertBindingScriptRbTreeNodeWithRebalance | 0.91 | STL `map/set<T> too long` guard; allocates node; RB rebalance via `FM2_RbTree_RotateLeft/LeftAtChild`; sentinel byte `+120/+121`. |
| 0x82623608 | sub_82623608 | FM2_Render_InitBindingScriptRbTreeNode | 0.90 | Alloc 124-byte node; copy script name string; `FM2_Lua_InitScriptLoaderFromCopiedBindingSlot`; sets color bytes `+120/+121`. |
| 0x82623680 | sub_82623680 | FM2_Render_DestroyBindingScriptRbTreeSubtree | 0.91 | Post-order walk; `FM2_Lua_ResetBindingStateAndInvokeClear` + string clear + free; sentinel `+121`. |
| 0x82623D98 | sub_82623D98 | FM2_Render_ClearBindingScriptRbTree | 0.90 | Destroy subtree from root child; reset head links; zero size; mirrors network RB-tree clear pattern. |
| 0x82621430 | sub_82621430 | FM2_Lua_DispatchDeferredBindingEventFromTask | 0.89 | Reads deferred-task field; pass-10 cache refresh; `FM2_Lua_DispatchBindingEventWithDiagGate`. |
| 0x826213B8 | sub_826213B8 | FM2_Lua_DispatchBindingEventWithProfileFieldFromTask | 0.89 | Same task setup as above but passes `FM2_Profile_GetFieldAt40` into profile-field dispatch. |
| 0x82621790 | sub_82621790 | FM2_Lua_BuildBindingScriptKeyWithCopiedSlot | 0.90 | Stores event-name C string + `FM2_Lua_InitScriptLoaderFromCopiedBindingSlot`; clears temp loader. |
| 0x827E0E40 | sub_827E0E40 | FM2_Render_GetMaterialPassVariantPayloadPtr | 0.88 | Resolves pass-flag bit to variant payload offset; negative ids via sorted table/category byte; 8 callers. |
| 0x827E4AC8 | sub_827E4AC8 | FM2_Render_DispatchMaterialPass13ByNegativeSlot | 0.91 | Thin but exact: `FM2_Render_DispatchMaterialPassByNegativeSlot(a1, 13)`; deferred enqueue path. |
| 0x82611DC0 | sub_82611DC0 | FM2_Lua_GrowBindingVariantVectorAppendNTimes | 0.89 | `FM2_Vector_ReallocGrow16ByteElements` then copy 16-byte template `a2` times; script-loader init uses count 4. |
| 0x82612480 | sub_82612480 | FM2_Lua_DeactivateBindingEventAndMoveToPendingList | 0.88 | Match event by type hash/key/slot; clear active `+120`; swap-remove from lap vector; push to pending at `a1+68`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82624230 | sub_82624230 | Binding-object dtor; defer with `sub_82623FD0` erase cluster. |
| 0x826240D0 | sub_826240D0 | Thin wrapper → `sub_82623FD0` script-tree destroy. |
| 0x82624520 | sub_82624520 | Thin wrapper around dtor + optional free. |
| 0x827E4968 | sub_827E4968 | Thin `FM2_XmlParser_CreateNodeIfNeeded` wrapper. |
| 0x827DDCE8 | sub_827DDCE8 | Generic profile callback vtable dispatch by slot index. |
| 0x827DE528 | sub_827DE528 | Two-call deferred-task enqueue helper. |
| 0x827DE268 | sub_827DE268 | Deferred-task queue pop + callback dispatch. |
| 0x82620C50 | sub_82620C50 | Duplicate uint32-as-int64 binding slot init. |
| 0x827DDAE0 | sub_827DDAE0 | Thin deref helper only. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
| 0x82623FD0 | sub_82623FD0 | RB-tree erase iterator range; defer with dtor cluster. |
