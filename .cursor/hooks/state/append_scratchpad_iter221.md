## Iteration 221

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82613988 | sub_82613988 | FM2_Network_RbTree_LowerBoundInsertElementSlotKey | 0.91 | Outer activate tree: lower_bound by dword `+12` (element slot); sentinel `+29`; calls insert-rebalance on miss; used by `FM2_Lua_ActivateBindingScriptInNetworkRbTree`. |
| 0x82612E08 | sub_82612E08 | FM2_Network_RbTree_LowerBoundInsertPassFlagKey | 0.91 | Nested pass-flag tree under element node `+16`; same lower_bound pattern; calls `FM2_Network_RbTree_InsertPassFlagNodeWithRebalance`. |
| 0x82613740 | sub_82613740 | FM2_Network_RbTree_InsertElementSlotNodeWithRebalance | 0.92 | STL `map/set<T> too long` guard; allocates via `FM2_Network_RbTree_InitElementSlotNodeWithClonedSubtree`; RB rotations `+28` color bytes. |
| 0x82612BC0 | sub_82612BC0 | FM2_Network_RbTree_InsertPassFlagNodeWithRebalance | 0.92 | Twin of element-slot insert; node init via `FM2_Network_RbTree_InitNodeWithDwordKeyAndSubtree`; nested-tree rebalance. |
| 0x826136E0 | sub_826136E0 | FM2_Network_RbTree_InitElementSlotNodeWithClonedSubtree | 0.90 | Element-slot node: dword key `+12`; `FM2_IntrusiveList_InitSentinelHead` at `+16`; clones subtree via merge helper; color `+28/+29`. |
| 0x82613470 | sub_82613470 | FM2_Network_RbTree_MergeCloneSubtreeIntoHead | 0.90 | Copies source tree into dest head; recursive `FM2_Network_RbTree_CloneSubtreeNodesRecursive`; fixes min/max links; sentinel `+29`. |
| 0x82613370 | sub_82613370 | FM2_Network_RbTree_CloneSubtreeNodesRecursive | 0.90 | Post-order clone alloc; calls `FM2_Network_RbTree_InitNodeWithDwordKeyAndSubtree`; preserves color byte `+28`. |
| 0x82613610 | sub_82613610 | FM2_Network_RbTree_DestroyTempHeadAndFreeSentinel | 0.89 | Erases temp head range then frees sentinel; used after activate tree restructure. |
| 0x826129B0 | sub_826129B0 | FM2_Network_RbTree_MergeCloneBindingSubtreeIntoHead | 0.90 | Same merge pattern as `MergeCloneSubtreeIntoHead` but `+129` sentinel binding-event subtree. |
| 0x82611E88 | sub_82611E88 | FM2_Network_RbTree_CloneBindingSubtreeNodesRecursive | 0.90 | Recursive clone for `+129`-sentinel nodes via `FM2_Network_RbTree_AllocSentinelNode` path; color `+128`. |
| 0x82612AF8 | sub_82612AF8 | FM2_Network_RbTree_DestroyNestedTempHeadAndFreeSentinel | 0.89 | Nested-tree temp-head destroy twin of `DestroyTempHeadAndFreeSentinel`; calls `sub_82612668` erase walk. |
| 0x82612A50 | sub_82612A50 | FM2_Network_RbTree_InsertBindingEventKeyHint | 0.90 | Lower_bound walk on `+129` tree by binding-event key `+12`; always inserts via `sub_82612768`; used when activating new slot. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82612768 | sub_82612768 | Binding-event insert-rebalance; defer paired with `sub_826116D8` node-init naming. |
| 0x826116D8 | sub_826116D8 | Binding-event node ctor; defer with `sub_82612768`. |
| 0x82610B78 | sub_82610B78 | Thin key builder: element id + script name + binding slot copy. |
| 0x82610D00 | sub_82610D00 | Thin pair builder wrapping `sub_82610AB8` + binding clear. |
| 0x82613510 | sub_82613510 | Iterator-range erase walk for temp heads; defer with `sub_82612668`/`sub_82614318`. |
| 0x82612668 | sub_82612668 | Nested temp-head erase walk twin of `sub_82613510`. |
| 0x826145D8 | sub_826145D8 | Deactivate cleanup tail; symmetric with `sub_82613678`. |
| 0x82613678 | sub_82613678 | Deactivate nested-tree cleanup; calls `sub_82544960` + `sub_827BF008`. |
| 0x826206E8 | sub_826206E8 | Thin `FM2_Lua_SetStackTop` wrapper. |
| 0x82620FF8 | sub_82620FF8 | Thin `FM2_DeferredTaskParams_GetField4` wrapper. |
| 0x8260FAB8 | sub_8260FAB8 | Thin `FM2_Lua_GetOrCreateObjectElementForBinding` wrapper. |
| 0x827DD108 | sub_827DD108 | Trivial deref `*a2`. |
