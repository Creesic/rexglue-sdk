## Iteration 225

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82611218 | sub_82611218 | FM2_Lua_ActivateBindingScriptReadXmlPropertiesToVariants | 0.91 | Network RB-tree activate by element slot; foreach binding-event node reads 4 xml properties via `FM2_Lua_ReadBindingXmlPropertyByNameToStack`; invokes `FM2_Lua_InvokeBindingEventAndCollectReturnStrings`. |
| 0x826117F8 | sub_826117F8 | FM2_Lua_DispatchBindingScriptletWithPass0Recursive | 0.90 | Profile category lookup; pass-0 recursive dispatch; sets `"self"` metatable; invokes `"scriptlet"` via `FM2_Lua_InvokeBindingScriptletOrCopyLoaderError`. |
| 0x82611748 | sub_82611748 | FM2_Lua_InvokeBindingScriptletOrCopyLoaderError | 0.91 | `FM2_Lua_LoadOrAppendScriptBufferToContext` then event invoke; on failure copies loader error from `FM2_Lua_GetAppForzaStateOffset8`. |
| 0x82611F30 | sub_82611F30 | FM2_Lua_DispatchBindingOnInitializeFromXmlLuaScriptlet | 0.90 | XML navigator finds Lua scriptlet nodes; scriptlet invoke; pass-0 recursive; `"onInitialize"` via `FM2_Lua_DispatchBindingEventWithDiagGateAndProfileField`; pass 10/11 invalidate. |
| 0x82610530 | sub_82610530 | FM2_Lua_ForEachIntVectorInvokeCallbackExceptId | 0.90 | Iterates dword vector `[begin,end)`; skips entries equal to `a4`; invokes callback; clears temp STL string. |
| 0x82610D68 | sub_82610D68 | FM2_Lua_ForEachIntVectorInvokeCStringCallbackExceptId | 0.90 | Same skip-id foreach but builds temp string from `a3` C string per entry for callback `(id, string)`. |
| 0x82610BD8 | sub_82610BD8 | FM2_Lua_ForEachBindingIntVectorInvokeProfileCallback | 0.89 | Copies profile string then `FM2_Lua_ForEachIntVectorInvokeCallbackExceptId` with `sub_827EEE10` callback at `a1+16`. |
| 0x826117E0 | sub_826117E0 | FM2_Lua_ForEachProfileIntVectorInvokeVtableMethod4 | 0.89 | Thin entry: `FM2_Lua_ForEachIntVectorInvokeCStringCallbackExceptId` at `a1+16` with `FM2_Lua_InvokeIntVectorEntryVtableMethod4`. |
| 0x826101F8 | sub_826101F8 | FM2_Lua_InvokeIntVectorEntryVtableMethod4 | 0.88 | One-liner vtable dispatch `(*(a1+4))(a1)`; used as profile-vector foreach callback. |
| 0x82610208 | sub_82610208 | FM2_Lua_InvokeIntVectorEntryVtableMethod8 | 0.88 | Vtable dispatch `(*(a1+8))(a1)`; used when binding event invoke fails to broadcast error to int vector. |
| 0x82615348 | sub_82615348 | FM2_Lua_CreateBindingObjectAndRegisterScriptlet | 0.91 | Alloc 60-byte object; `FM2_Lua_InitBindingObjectContext`; registers `"scriptlet"` binding via `FM2_Profile_AppendManagerBindingByHashedName`. |
| 0x826153F0 | sub_826153F0 | FM2_Lua_DispatchBindingObjectLifecycleByOpcode | 0.90 | Factory switch: opcode 1 create, 2 scene destroy, 6 attach profile, 7 finalize presentation. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826115B0 | sub_826115B0 | Arg-shuffle thunk to `FM2_Lua_DispatchBindingEventOnInitializeWithPass0`. |
| 0x826115C0 | sub_826115C0 | Arg-shuffle thunk to `FM2_Lua_ActivateBindingScriptReadXmlPropertiesToVariants`. |
| 0x82611998 | sub_82611998 | Arg-shuffle thunk to `FM2_Lua_DispatchBindingScriptletWithPass0Recursive`. |
| 0x82612660 | sub_82612660 | Arg-shuffle thunk to `FM2_Lua_DispatchBindingOnInitializeFromXmlLuaScriptlet`. |
| 0x82544960 | sub_82544960 | Generic `+29` RB-tree iterator-pair builder; defer material-node cluster. |
| 0x82544828 | sub_82544828 | Upper-bound iterator init twin; defer with `sub_825447D8`. |
| 0x825447D8 | sub_825447D8 | Upper-bound find on `+29` sentinel tree. |
| 0x826206E8 | sub_826206E8 | Thin `FM2_Lua_SetStackTop` wrapper. |
| 0x82620FF8 | sub_82620FF8 | Thin `FM2_DeferredTaskParams_GetField4` wrapper. |
| 0x8260FAB8 | sub_8260FAB8 | Thin `FM2_Lua_GetOrCreateObjectElementForBinding` wrapper. |
| 0x82610E38 | sub_82610E38 | 12-byte thunk; defer. |
