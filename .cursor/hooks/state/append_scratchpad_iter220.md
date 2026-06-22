## Iteration 220

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82613EF0 | sub_82613EF0 | FM2_Lua_ActivateBindingScriptInNetworkRbTree | 0.92 | Network RB-tree lower_bound by element slot; nested pass-flag insert; `FM2_Network_RecordPacketStatsInBucket`; creates binding event via `sub_82610B78`/`sub_82612A50` when slot not duplicate. |
| 0x82614640 | sub_82614640 | FM2_Lua_DeactivateBindingScriptInNetworkRbTree | 0.91 | Mirror activate: find element+slot match; `FM2_Network_RbTree_EraseNodeAndRebalance`; `FM2_Render_ResourceCache_UpdateOrRemoveEntry` when refcount zero. |
| 0x827D73E8 | sub_827D73E8 | FM2_Render_FindMaterialXmlPropertyWalkUpParentChain | 0.90 | Parent-chain loop calling property read; sets out ptr from `a1+8`; used by `FM2_Lua_ReadBindingXmlPropertyByNameToStack`. |
| 0x827E2ED0 | sub_827E2ED0 | FM2_Render_ReadMaterialXmlPropertyByIdToString | 0.91 | Negative id sorted lookup or bitset index path; `FM2_Stl_String_AssignRange` into out string; returns bool found. |
| 0x826119B0 | sub_826119B0 | FM2_Network_RbTree_EraseNodeAndRebalance | 0.92 | STL map erase with `"invalid map/set<T> iterator"` guard; rotations; sentinel byte `+129`; frees node. |
| 0x826100F8 | sub_826100F8 | FM2_Network_RbTree_AdvanceIterator | 0.90 | Iterator successor walk on network binding RB-tree; sentinel `+129`; used in activate/deactivate scan loops. |
| 0x826104D8 | sub_826104D8 | FM2_Network_RbTree_InitEmptySentinelHead | 0.89 | Alloc sentinel via `FM2_Network_RbTree_AllocSentinelNode`; self-linked head; `+129=1` end marker. |
| 0x826103B8 | sub_826103B8 | FM2_Network_RbTree_AllocSentinelNode | 0.90 | Alloc 1 node; zero links; `+128=1` red, `+129=0` non-sentinel-child marker pattern. |
| 0x82612B60 | sub_82612B60 | FM2_Network_RbTree_InitNodeWithDwordKeyAndSubtree | 0.90 | Stores parent links + dword key `+12`; inits nested tree head at `+16`; color bytes `+28/+29`. |
| 0x826247C8 | sub_826247C8 | FM2_Lua_HandleBindingScriptMutationSkipTypePrefix | 0.91 | 8-byte thunk calling `FM2_Lua_HandleBindingScriptMutationWithTypeCoercion(..., 0)`. |
| 0x82620F68 | sub_82620F68 | FM2_Lua_IsDeferredTaskElementSlotEqual | 0.90 | Returns `*(a2+12)==*(a1+12)`; deferred-task element-slot equality gate. |
| 0x82610770 | sub_82610770 | FM2_Network_RbTree_BuildIteratorPairFromNode | 0.88 | Combines two child-boundary iterators into 4-dword pair for nested-tree range walk. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82613470 | sub_82613470 | Clone-merge helper; defer with `sub_82613610` destroy pair naming pass. |
| 0x82613610 | sub_82613610 | Temporary-head destroy; needs paired naming with clone helpers. |
| 0x826129B0 | sub_826129B0 | +129-sentinel clone-merge; parallel to `sub_82613470`. |
| 0x82612AF8 | sub_82612AF8 | Temporary-head destroy variant; defer cluster. |
| 0x82613988 | sub_82613988 | Lower_bound insert hint wrapper; defer with `sub_82613740` rebalance. |
| 0x82612E08 | sub_82612E08 | Nested pass-flag tree insert hint; defer cluster. |
| 0x82613740 | sub_82613740 | `map/set<T> too long` insert-rebalance; defer dedicated network RB-tree pass. |
| 0x82612BC0 | sub_82612BC0 | Nested-tree insert-rebalance twin of `sub_82613740`. |
| 0x826145D8 | sub_826145D8 | Activate cleanup tail; symmetric with `sub_82613678`; defer. |
| 0x82613678 | sub_82613678 | Deactivate cleanup; calls `sub_82544960` + `sub_827BF008`. |
| 0x826206E8 | sub_826206E8 | Thin `FM2_Lua_SetStackTop` wrapper. |
| 0x82620FF8 | sub_82620FF8 | Thin `FM2_DeferredTaskParams_GetField4` wrapper. |
| 0x8260FAB8 | sub_8260FAB8 | Thin `FM2_Lua_GetOrCreateObjectElementForBinding` wrapper. |
| 0x827DD108 | sub_827DD108 | Trivial deref `*a2`. |
| 0x827DEF38 | j_FM2_Render_DispatchMaterialPass0RecursiveOrInvalidate | Import thunk only. |
