## Iteration 218

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82634890 | sub_82634890 | FM2_Lua_ResolveUnitStringParentChildPair | 0.90 | From child slot walks parents via `FM2_Render_LookupUnitStringChildIdBySlotSelector` until lap-tracker root or `FM2_Render_IsUnitStringNodeXmlChain8Truthy`; stores start/end node pair. |
| 0x82634990 | sub_82634990 | FM2_Lua_InitDeferredTaskParamsFromUnitStringSlot | 0.89 | Zero-init 36-byte struct + string field; calls resolver; used by `FM2_Lua_GetOrCreateDeferredTaskParamsForBinding`. |
| 0x826219F8 | sub_826219F8 | FM2_Lua_AllocInitBindingEventRbTreeSentinelRecord | 0.91 | `FM2_Lua_AllocBindingEventRecordArray(1)`; zero child pointers; sentinel bytes `+120=1`, `+121=0`; used by `FM2_RbTree_InitSentinelNode`. |
| 0x82620800 | sub_82620800 | FM2_Lua_InitBindingVariantUint32WithHighTag | 0.89 | Sets variant type 3 and double `(index \| 0x300000000)`; used when pushing foreach callback index variants. |
| 0x82620C50 | sub_82620C50 | FM2_Lua_InitBindingSlotWithTaggedUint32OnStack | 0.90 | Binding-slot type 3; pushes tagged uint32 number on Lua stack; records stack depth; used in xml-ptrmap read path. |
| 0x826218A8 | sub_826218A8 | FM2_Lua_GetBindingDeferredFloatFromXmlChain11 | 0.90 | Deferred field4 + `FM2_XmlTree_ResolveIndexedChildChain` at index 11; returns float for binding getter. |
| 0x826217E8 | sub_826217E8 | FM2_Lua_GetBindingDeferredMaterialNodeTypeHandle | 0.88 | Deferred field4 then `**(*(a1+12)+4)` material-node type handle deref. |
| 0x82621828 | sub_82621828 | FM2_Lua_GetBindingDeferredBoolFromXmlChain1 | 0.90 | Deferred field4 + `FM2_XmlTree_ResolveIndexedChildChain_0(node, 1, 1, 1)` bool getter. |
| 0x82621900 | sub_82621900 | FM2_Lua_DispatchBindingDeferredMaterialPass1 | 0.91 | Thin exact wrapper: `FM2_Render_DispatchMaterialPassByNegativeSlot(*(a1+12), 1, &flag)`. |
| 0x82621948 | sub_82621948 | FM2_Lua_ForEachBindingDeferredResourceCachePass11Float | 0.90 | Passes first double arg as float to `FM2_Render_ForEachResourceCacheByPassFlag(..., 11, &float)`. |
| 0x827E8A00 | sub_827E8A00 | FM2_Render_IsUnitStringNodeXmlChain8Truthy | 0.89 | `FM2_XmlTree_ResolveIndexedChildChain_0(node, 8, 0, 1)` truth test; breaks parent walk in resolver. |
| 0x827DEC00 | sub_827DEC00 | FM2_Render_DispatchMaterialPass0RecursiveOrInvalidate | 0.89 | Recursive parent dispatch when xml-chain1 true; `FM2_Render_DispatchMaterialPassByNegativeSlot(a1, 0)`; used via `sub_827DEF38`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826247C8 | sub_826247C8 | Thin wrapper `sub_82624570(..., 0)`. |
| 0x8260FAB8 | sub_8260FAB8 | Thin wrapper `FM2_Lua_GetOrCreateObjectElementForBinding`. |
| 0x827DD108 | sub_827DD108 | Trivial deref `*a2`. |
| 0x826206E8 | sub_826206E8 | Thin `FM2_Lua_SetStackTop(*a1, -1-a2)`. |
| 0x826207D8 | sub_826207D8 | 3-field variant bool init helper only. |
| 0x82620820 | sub_82620820 | Tiny variant double init only. |
| 0x82620830 | sub_82620830 | Tiny variant int32 init only. |
| 0x82620878 | sub_82620878 | Thin `FM2_Lua_IsNumberOrBooleanType()`. |
| 0x826208B0 | sub_826208B0 | CString variant from std::string; defer with ptrmap-read cluster. |
| 0x82620F68 | sub_82620F68 | Trivial compare `*(a2+12)==*(a1+12)`. |
| 0x82620FF8 | sub_82620FF8 | Thin `FM2_DeferredTaskParams_GetField4` wrapper. |
| 0x82621870 | sub_82621870 | Thin thunk to `FM2_Render_DispatchMaterialPass0RecursiveOrInvalidate`. |
| 0x827DEF38 | sub_827DEF38 | Thunk to `FM2_Render_DispatchMaterialPass0RecursiveOrInvalidate`. |
| 0x82622F08 | sub_82622F08 | 1.4KB xml-ptrmap typed read switch; defer dedicated pass. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
