## Iteration 224

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82614480 | sub_82614480 | FM2_Lua_DestroyBindingObjectContext | 0.92 | Full binding-object teardown: profile hook free, destroys pending event records, clears element-slot RB-tree, flush table pairs, `FM2_Lua_DestroyBindingNetworkContext` + `FM2_Lua_DestroyBindingScriptContext`. |
| 0x82610870 | sub_82610870 | FM2_Lua_ResetBindingEventRecordAndMaybeFree | 0.91 | `FM2_Lua_ResetBindingStateAndInvokeClear` at `+40`; clears name string `+12`; optional `FM2_Memory_FreeSmallBlockOrNull` when flag bit set. |
| 0x82610418 | sub_82610418 | FM2_Network_RbTree_InitIteratorAtLowerBound | 0.90 | Wraps `FM2_Network_RbTree_FindLowerBoundNodeByDwordKey` into `{tree,node}` iterator pair for range walks. |
| 0x82610478 | sub_82610478 | FM2_Network_RbTree_InitIteratorAtUpperBound | 0.90 | Upper-bound twin used with lower-bound in `FM2_Network_RbTree_BuildIteratorPairFromNode`. |
| 0x82614418 | sub_82614418 | FM2_Network_RbTree_DestroyElementSlotHeadAndFreeSentinel | 0.90 | Calls `FM2_Network_RbTree_EraseElementSlotIteratorRange` then frees sentinel; partial binding-object dtor helper. |
| 0x82610980 | sub_82610980 | FM2_Lua_DestroyBindingEventRecordVectorAndClear | 0.90 | Drains int-vector of pending records via `FM2_Lua_ResetBindingEventRecordAndMaybeFree`; `FM2_IntVector_EraseRangeShift`. |
| 0x82615040 | sub_82615040 | FM2_Lua_InitBindingObjectContext | 0.91 | Binding-object ctor `off_8210EBF0`: script context, network RB-tree context, empty binding slot, registers many Lua bindings, nested-object lookup hook. |
| 0x82615280 | sub_82615280 | FM2_Lua_DestroyBindingObjectPartial | 0.89 | Partial dtor: clears element-slot tree, frees int-vector storage, resets vtable to `off_8210EBC8`. |
| 0x826152F8 | sub_826152F8 | FM2_Lua_DestroyBindingObjectPartialAndMaybeFree | 0.89 | Calls partial dtor; optional `FM2_Memory_FreeSmallBlockOrNull` on object when flag bit set. |
| 0x82610C48 | sub_82610C48 | FM2_Lua_FinalizeBindingPresentationAndAttachProfile | 0.90 | Clears pending events; pushes variant; sets metatable `"Presentation"`; attaches profile via `sub_827DE228`. |
| 0x826148D8 | sub_826148D8 | FM2_Lua_DeactivateBindingScriptForGatorContentTypesRecursive | 0.91 | Per Gator content-type pass-flag deactivate + erase; resource-cache update; recurses unit-string child vector; called from scene-graph destroy. |
| 0x82610F68 | sub_82610F68 | FM2_Lua_DispatchBindingEventOnInitializeWithPass0 | 0.90 | Active binding-event gate; `"onInitialize"` string check; pass-11 dispatch; `FM2_Render_DispatchMaterialPass0RecursiveOrInvalidate`; collects return strings. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82610530 | sub_82610530 | Generic int-vector foreach with skip-id callback; defer with profile-dispatch cluster. |
| 0x82610BD8 | sub_82610BD8 | Thin wrapper copying string then calling `sub_82610530` with profile callback. |
| 0x82610D68 | sub_82610D68 | Int-vector foreach with per-entry C string callback; needs paired naming with `sub_82610530`. |
| 0x82611218 | sub_82611218 | 920-byte activate+xml-property read path; defer dedicated pass. |
| 0x826117F8 | sub_826117F8 | Scriptlet dispatch with pass-0 + `"self"` metatable; defer with `sub_82611748`. |
| 0x82611748 | sub_82611748 | Scriptlet invoke or copy loader error string; used by `sub_826117F8`. |
| 0x82611F30 | sub_82611F30 | 856-byte binding handler; needs dedicated analysis. |
| 0x82544960 | sub_82544960 | Generic RB-tree iterator-pair builder shared outside binding cluster. |
| 0x826206E8 | sub_826206E8 | Thin `FM2_Lua_SetStackTop` wrapper. |
| 0x82620FF8 | sub_82620FF8 | Thin `FM2_DeferredTaskParams_GetField4` wrapper. |
| 0x8260FAB8 | sub_8260FAB8 | Thin `FM2_Lua_GetOrCreateObjectElementForBinding` wrapper. |
| 0x826101F8 | sub_826101F8 | 16-byte callback thunk. |
