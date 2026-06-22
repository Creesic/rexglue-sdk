## Iteration 222

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82612768 | sub_82612768 | FM2_Network_RbTree_InsertBindingEventNodeWithRebalance | 0.92 | `map/set<T> too long` guard; node via `FM2_Network_RbTree_InitBindingEventNode`; RB rebalance with `+128` color bytes and custom rotate helpers; callee of `FM2_Network_RbTree_InsertBindingEventKeyHint`. |
| 0x826116D8 | sub_826116D8 | FM2_Network_RbTree_InitBindingEventNode | 0.91 | 132-byte node: tree links, dword key `+12`, copies script-key record at `+16` via `FM2_Lua_CopyBindingScriptKeyRecord`; color `+128/+129`. |
| 0x82610AB8 | sub_82610AB8 | FM2_Lua_CopyBindingScriptKeyRecord | 0.90 | Copies element id, STL script name, and binding-slot loader from source record; used by binding-event node init and activate key build. |
| 0x82610B78 | sub_82610B78 | FM2_Lua_InitBindingScriptKeyFromSlotNameAndBinding | 0.90 | Sets element id from `*a2`; assigns script name string; copies binding slot from stack index; used when activating new network binding. |
| 0x82610D00 | sub_82610D00 | FM2_Lua_StealBindingScriptKeyIntoTypeHashPair | 0.89 | Stores type hash + moves script-key record via copy helper; clears source binding state/string after steal; feeds RB-tree insert on activate. |
| 0x82613510 | sub_82613510 | FM2_Network_RbTree_ErasePassFlagIteratorRange | 0.90 | STL erase-range on pass-flag subtree; clears all via `sub_82613418` or loops `FM2_Network_RbTree_ErasePassFlagNodeAndRebalance`; used by temp-head destroy. |
| 0x82612668 | sub_82612668 | FM2_Network_RbTree_EraseBindingEventIteratorRange | 0.90 | Nested binding-event erase-range; clears via `sub_82611E30` or loops `FM2_Network_RbTree_EraseNodeAndRebalance`; used by nested temp-head destroy. |
| 0x82614318 | sub_82614318 | FM2_Network_RbTree_EraseElementSlotIteratorRange | 0.90 | Element-slot tree erase-range twin of pass-flag range; calls `FM2_Network_RbTree_EraseElementSlotNodeAndRebalance`; used on deactivate cleanup. |
| 0x82612F18 | sub_82612F18 | FM2_Network_RbTree_ErasePassFlagNodeAndRebalance | 0.91 | Single-node erase `+29` sentinel tree; invalid-iterator guard; destroys nested head via `FM2_Network_RbTree_DestroyNestedTempHeadAndFreeSentinel` at `+16`. |
| 0x82613AF0 | sub_82613AF0 | FM2_Network_RbTree_EraseElementSlotNodeAndRebalance | 0.91 | Twin erase for element-slot nodes; destroys `+16` subtree via `FM2_Network_RbTree_DestroyTempHeadAndFreeSentinel`; decrements tree size. |
| 0x82613678 | sub_82613678 | FM2_Network_RbTree_CleanupEmptyPassFlagSubtreeOnDeactivate | 0.89 | Deactivate path when nested refcount zero: iterator pair build, count range, `FM2_Network_RbTree_ErasePassFlagIteratorRange`. |
| 0x826145D8 | sub_826145D8 | FM2_Network_RbTree_CleanupEmptyElementSlotOnDeactivate | 0.89 | Outer-tree deactivate cleanup twin; calls `FM2_Network_RbTree_EraseElementSlotIteratorRange` after range count. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82610180 | sub_82610180 | Pool alloc `132*count` with OOB guard; defer with `FM2_Network_RbTree_AllocSentinelNode` naming pass. |
| 0x82610090 | sub_82610090 | Custom left-rotate for `+129` binding-event tree; defer rotate-helper cluster. |
| 0x82610228 | sub_82610228 | Custom right-rotate twin of `sub_82610090`. |
| 0x82613418 | sub_82613418 | Clear-all pass-flag tree helper; defer with destroy-subtree cluster. |
| 0x826142C0 | sub_826142C0 | Clear-all element-slot tree helper. |
| 0x82611E30 | sub_82611E30 | Clear-all binding-event tree helper. |
| 0x827BF008 | sub_827BF008 | Counts iterator-range nodes before erase; thin loop body. |
| 0x82544960 | sub_82544960 | Iterator-pair builder; overlaps generic RB-tree helpers outside network cluster. |
| 0x826206E8 | sub_826206E8 | Thin `FM2_Lua_SetStackTop` wrapper. |
| 0x82620FF8 | sub_82620FF8 | Thin `FM2_DeferredTaskParams_GetField4` wrapper. |
| 0x8260FAB8 | sub_8260FAB8 | Thin `FM2_Lua_GetOrCreateObjectElementForBinding` wrapper. |
| 0x827DD108 | sub_827DD108 | Trivial deref `*a2`. |
