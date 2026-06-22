## Iteration 223

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82610180 | sub_82610180 | FM2_Network_RbTree_AllocBindingEventNodePool | 0.91 | Overflow-checked pool alloc `132 * count` (`0x84` stride); `bad_array_new_length` path; used by sentinel alloc and binding-event insert. |
| 0x82610090 | sub_82610090 | FM2_Network_RbTree_RotateLeftAtChild | 0.90 | Left rotation on `+129` binding-event RB-tree; checks sentinel byte `+129`; used by insert/erase rebalance paths. |
| 0x82610228 | sub_82610228 | FM2_Network_RbTree_RotateRightAtChild | 0.90 | Right rotation twin of left-rotate; same sentinel semantics on binding-event nodes. |
| 0x82613418 | sub_82613418 | FM2_Network_RbTree_ClearPassFlagTree | 0.90 | Wipes pass-flag tree head to empty sentinel self-loop; calls recursive destroy from root child. |
| 0x826142C0 | sub_826142C0 | FM2_Network_RbTree_ClearElementSlotTree | 0.90 | Same clear pattern for outer element-slot tree; used in binding-object destroy (`sub_82614480`). |
| 0x82611E30 | sub_82611E30 | FM2_Network_RbTree_ClearBindingEventTree | 0.90 | Clears `+129` binding-event tree head; recursive destroy from root child. |
| 0x82613318 | sub_82613318 | FM2_Network_RbTree_DestroyPassFlagSubtreeRecursive | 0.91 | Post-order destroy `+29` nodes; frees nested head at `+16` via `FM2_Network_RbTree_DestroyNestedTempHeadAndFreeSentinel`. |
| 0x82613A98 | sub_82613A98 | FM2_Network_RbTree_DestroyElementSlotSubtreeRecursive | 0.91 | Post-order destroy element-slot nodes; nested destroy via `FM2_Network_RbTree_DestroyTempHeadAndFreeSentinel`. |
| 0x82611668 | sub_82611668 | FM2_Network_RbTree_DestroyBindingEventSubtreeRecursive | 0.91 | Post-order destroy `+129` nodes; clears binding state/string at `+48`/`+20` before free. |
| 0x827BF008 | sub_827BF008 | FM2_RbTree_CountNodesInIteratorRange | 0.89 | Walks iterator range incrementing out counter until end==begin; used before erase-range cleanup on deactivate. |
| 0x82610290 | sub_82610290 | FM2_Network_RbTree_FindLowerBoundNodeByDwordKey | 0.90 | Classic lower_bound on dword key at node `+12` with `+129` sentinel; feeds iterator-pair builder. |
| 0x826102E0 | sub_826102E0 | FM2_Network_RbTree_FindUpperBoundNodeByDwordKey | 0.90 | Upper_bound variant (`*a2 < node[3]` go left); paired with lower_bound in range iteration. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82610418 | sub_82610418 | Thin wrapper around lower_bound node into iterator pair. |
| 0x82610478 | sub_82610478 | Thin wrapper around upper_bound node into iterator pair. |
| 0x82544960 | sub_82544960 | Generic iterator-pair builder; shared outside network binding cluster. |
| 0x82614418 | sub_82614418 | Temp-head destroy calling `FM2_Network_RbTree_EraseElementSlotIteratorRange`; defer binding-object dtor cluster. |
| 0x82614480 | sub_82614480 | Full binding-object context destroy; defer dedicated pass. |
| 0x826148D8 | sub_826148D8 | 980-byte network binding handler; needs dedicated analysis. |
| 0x82610870 | sub_82610870 | Binding-event record reset+optional free; defer with object dtor cluster. |
| 0x826101F8 | sub_826101F8 | 16-byte thunk/wrapper. |
| 0x82610208 | sub_82610208 | 16-byte thunk/wrapper. |
| 0x826206E8 | sub_826206E8 | Thin `FM2_Lua_SetStackTop` wrapper. |
| 0x82620FF8 | sub_82620FF8 | Thin `FM2_DeferredTaskParams_GetField4` wrapper. |
| 0x8260FAB8 | sub_8260FAB8 | Thin `FM2_Lua_GetOrCreateObjectElementForBinding` wrapper. |
