## Iteration 226

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825447D8 | sub_825447D8 | FM2_RbTree_FindUpperBoundNodeByDwordKey | 0.90 | Upper_bound walk on dword key at `+12` with sentinel `+29`; paired with lower-bound network helpers. |
| 0x82544828 | sub_82544828 | FM2_RbTree_InitUpperBoundIteratorPair | 0.90 | Wraps upper-bound node into `{tree,node}` iterator pair; feeds bounds builder. |
| 0x82544960 | sub_82544960 | FM2_RbTree_BuildIteratorPairBoundsFromKey | 0.91 | Combines upper/lower bound iterators via `FM2_RbTreeNode_InitWithParent`; used by deactivate cleanup paths. |
| 0x8260FF60 | sub_8260FF60 | FM2_Lua_AttachPresentationProfileToBindingObject | 0.91 | Stores profile id; pushes `"CPresentation"` userdata; sets `"Presentation"` metatable field on binding lua state. |
| 0x827DD4B0 | sub_827DD4B0 | FM2_Profile_InsertBindingObjectIntoHashTable | 0.90 | Calls binding-object vtable hash `+4`; inserts pointer into profile hash table at `a1+264`. |
| 0x827DE228 | sub_827DE228 | FM2_Profile_PeekFrontDeferredTaskIdFromRingBuffer | 0.90 | If ring buffer non-empty at `a1+160`, returns front deferred-task id dword; else 0. |
| 0x82615430 | sub_82615430 | FM2_Lua_InitBindingPresentationElementContext | 0.92 | Vtable `off_8210EBF8`; reserves variant vectors; registers lifecycle opcodes; installs binding dispatch thunk callbacks. |
| 0x82615678 | sub_82615678 | FM2_Lua_DestroyBindingPresentationElementContext | 0.91 | Destroys child scene subtrees; flushes network context; erases lifecycle links; frees vectors/callbacks/strings. |
| 0x82615870 | sub_82615870 | FM2_Lua_DestroyBindingPresentationElementContextAndMaybeFree | 0.89 | Calls presentation-element destroy; optional `FM2_Memory_FreeSmallBlockOrNull` when flag bit set. |
| 0x826115B0 | sub_826115B0 | FM2_Lua_ThunkDispatchOnInitializeWithPass0 | 0.88 | Arg-shuffle thunk stored at `a1[8]`; forwards to `FM2_Lua_DispatchBindingEventOnInitializeWithPass0`. |
| 0x826115C0 | sub_826115C0 | FM2_Lua_ThunkActivateBindingReadXmlToVariants | 0.88 | Callback thunk at `a1[12]`; forwards to `FM2_Lua_ActivateBindingScriptReadXmlPropertiesToVariants`. |
| 0x82611998 | sub_82611998 | FM2_Lua_ThunkDispatchScriptletPass0Recursive | 0.88 | Callback thunk at `a1[9]`; forwards to `FM2_Lua_DispatchBindingScriptletWithPass0Recursive`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82612660 | sub_82612660 | Arg-shuffle thunk to `FM2_Lua_DispatchBindingOnInitializeFromXmlLuaScriptlet`; defer with `a1[10]` cluster. |
| 0x82610E38 | sub_82610E38 | Thin thunk to `FM2_Lua_ForEachBindingIntVectorInvokeProfileCallback`. |
| 0x82615B40 | sub_82615B40 | Static element-type→pass-flag lookup table; needs table constant analysis. |
| 0x82615B98 | sub_82615B98 | 576-byte animation-track XML import; defer dedicated pass. |
| 0x82615DD8 | sub_82615DD8 | 1232-byte binding handler; defer dedicated pass. |
| 0x82615918 | sub_82615918 | Thread-local monotonic id allocator; borderline thin. |
| 0x82615A60 | sub_82615A60 | Deferred-task field lookup with parent fallback; defer profile cluster. |
| 0x825446A0 | sub_825446A0 | Intrusive-list node copy helper; defer RB-tree node-init cluster. |
| 0x82544888 | sub_82544888 | RB-tree node init with nested list at `+16`; defer. |
| 0x826206E8 | sub_826206E8 | Thin `FM2_Lua_SetStackTop` wrapper. |
| 0x82620FF8 | sub_82620FF8 | Thin `FM2_DeferredTaskParams_GetField4` wrapper. |
| 0x8260FAB8 | sub_8260FAB8 | Thin `FM2_Lua_GetOrCreateObjectElementForBinding` wrapper. |
