## Iteration 211

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82612288 | sub_82612288 | FM2_Lua_FindOrCreateBindingEventByTypeHash | 0.90 | Scans lap-tracker event vector by type hash + binding key + stack slot; creates 124-byte record via `FM2_Lua_InitBindingEventRecord`; `FM2_Profile_AppendManagerBindingByHashedName`. |
| 0x82610B10 | sub_82610B10 | FM2_Lua_InitBindingEventRecord | 0.91 | Stores type hash, binding key, sub-id, event name string; `FM2_Lua_InitScriptLoaderFromCopiedBindingSlot`; sets active byte `+120`. |
| 0x8260FB00 | sub_8260FB00 | FM2_Lua_DispatchBindingEventWithDiagGateAndProfileField | 0.89 | Diag gate + `sub_827DDAE0` context; `FM2_Render_DispatchOrInvalidateMaterialPass6`; deferred enqueue or immediate dispatch; profile field `a4`. |
| 0x826214A0 | sub_826214A0 | FM2_Lua_InvokeBindingScriptByEventName | 0.90 | Nil-check event name → error index 3; init script loader; `FM2_Lua_FindOrCreateBindingEventByTypeHash`; reset binding state. |
| 0x82621510 | sub_82621510 | FM2_Lua_InvokeBindingScriptFromDeferredTaskParams | 0.89 | Same as by-name invoke but passes deferred-task field `a2+12` as binding sub-id to find/create. |
| 0x827DDB10 | sub_827DDB10 | FM2_Lua_IsBindingEventNameOnUpdate | 0.92 | Case-sensitive strcmp against `"onUpdate"`; skips sub-id when true in find/create path. |
| 0x827E49C0 | sub_827E49C0 | FM2_Render_DispatchOrInvalidateMaterialPass6 | 0.91 | `a2!=0` → `FM2_Render_InvalidateMaterialPassByNegativeSlotId(pass 6)` else `FM2_Render_DispatchMaterialPassByNegativeSlot(6)`. |
| 0x827DE420 | sub_827DE420 | FM2_Render_EnqueueOrDispatchDeferredBindingEvent | 0.88 | Lookup deferred queue entry; if absent dispatch pass 13 + push to `a1+180`; else immediate handler via `sub_827DD0F8`. |
| 0x827D70A8 | sub_827D70A8 | FM2_Render_RefreshMaterialPassCacheAfterInvalidate | 0.89 | Resolve pass flag byte; `FM2_Profile_AssignMaterialPassFlagVariantForNegativeId`; `FM2_Render_ResourceCache_ForEachMatchingEntry`. |
| 0x827E17B0 | sub_827E17B0 | FM2_Profile_AssignMaterialPassFlagVariantForNegativeId | 0.88 | Negative pass id → category byte or sorted dword upsert; sets variant type `10` with pass flag value. |
| 0x82620F88 | sub_82620F88 | FM2_Lua_InvokeProtectedCallWithTwoBindingVariants | 0.90 | Resolves binding object; pushes two variant slots; `FM2_Lua_InvokeProtectedCall32(-3)`. |
| 0x82624138 | sub_82624138 | FM2_Lua_RegisterBindingScriptForTableVariantType | 0.88 | Variant type `6`: lazily init RB-tree at `+44`; script-name tree insert; find/create binding event. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82621430 | sub_82621430 | Thin wrapper → diag-gated dispatch without profile field. |
| 0x826213B8 | sub_826213B8 | Thin wrapper → profile-field dispatch path only. |
| 0x82611DC0 | sub_82611DC0 | Generic 16-byte vector grow/append; used broadly. |
| 0x82621790 | sub_82621790 | Thin pair builder: C string + copied binding slot. |
| 0x82623ED0 | sub_82623ED0 | RB-tree script-name lookup; defer with `sub_82623B68` insert. |
| 0x827E4AC8 | sub_827E4AC8 | Thin wrapper → `FM2_Render_DispatchMaterialPassByNegativeSlot(13)`. |
| 0x827E4A68 | sub_827E4A68 | 24B wrapper → `sub_827E0E40`. |
| 0x827DD0F8 | sub_827DD0F8 | 8B thunk → `sub_827EE220`. |
| 0x827E4A58 | sub_827E4A58 | 12B wrapper → `sub_827D70A8(..., 10)`. |
| 0x827DDAE0 | sub_827DDAE0 | Thin deref helper only. |
| 0x82620C50 | sub_82620C50 | Duplicate uint32-as-int64 binding slot init. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
