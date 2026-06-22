## Iteration 215

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827DD920 | sub_827DD920 | FM2_Profile_InitElementAssetEntityRecord564 | 0.90 | Vtable `off_82158474` ("element"/"asset"/"transport"); 564-byte record; `strncpy` name at `+4`; assigns id `dword_82A722DC`; wires TCP transport + subsystems at `+520..+540`. |
| 0x827DD2E8 | sub_827DD2E8 | FM2_Profile_InitEntityRecordDualPassFlagBitsets | 0.88 | Called from entity ctor `+264`; sets count 31; zeroes two 124-byte flag arrays (`0x7C` each). |
| 0x827DDF70 | sub_827DDF70 | FM2_Profile_AppendEntityPtrToDeferredList | 0.89 | `FM2_STL_IntVector_ResizeZeroed` grow; append dword entity ptr; used by `FM2_Profile_CreateEntityRecordAndNotifySlot1`. |
| 0x827DE658 | sub_827DE658 | FM2_Profile_ProcessEntityListWithOptionalDeferredDispatch | 0.89 | Iterates entity ptr list at `+196/+200`; if `a5` and `+562` flag: enqueue slot 6 + `FM2_Render_InvalidateMaterialPass10AndSetProfileStrings` + slot 7 dispatch; else invalidate only. |
| 0x827DE7E8 | sub_827DE7E8 | FM2_Profile_EnqueueDeferredTaskNotifySlot2AndDispatch | 0.90 | Enqueue slot 6; slot 2 callback; `FM2_Profile_FlushEntityPendingUnitStringLinks`; slot 7 dispatch; vtable release on task object. |
| 0x827DD550 | sub_827DD550 | FM2_Profile_FlushEntityPendingUnitStringLinks | 0.90 | Drains int vector at entity `+544`; `FM2_Render_CompleteUnitStringLinkAndDestroyPending` per entry; toggles byte `+561` guard. |
| 0x827DD088 | sub_827DD088 | FM2_Profile_NotifyEntityStateToForzaApp | 0.88 | Reads entity `+516` id and Lua app state from transport `+520`; `sub_827E85A0` state notify; called from flagged-entity flush path. |
| 0x82621E08 | sub_82621E08 | FM2_Lua_InitBindingObjectFromLightUserdata | 0.90 | Base binding ctor: vtable `off_8210F5D0`; `FM2_Lua_GetLightUserdataAndRestoreStack`; scene-graph compare field; clears script tree/string fields. |
| 0x826336A0 | sub_826336A0 | FM2_Lua_InitUndefinedBindingObjectFromLightUserdata | 0.91 | Calls base init then vtable `off_82113B5C` ("[undefined]"); unit-string child id via `sub_827DDAB8`; optional profile name string; pass-6 dispatch. |
| 0x82631670 | sub_82631670 | FM2_Lua_DestroyElementAssetBindingObjectDtorLite | 0.90 | Vtable `off_82113400` ("element"/"asset"); frees 3 owned pointers `+56..+64`; base `FM2_Lua_DestroyBindingObjectAndScriptTree`. |
| 0x82631B58 | sub_82631B58 | FM2_Lua_DestroyElementAssetBindingObjectLitePlacementDelete | 0.89 | Vtable `[0]` at `off_82113400`; calls lite dtor; optional `FM2_Memory_FreeSmallBlockOrNull` when flag bit0. |
| 0x827E4C40 | sub_827E4C40 | FM2_Render_InvalidateMaterialPass10AndSetProfileStrings | 0.90 | `FM2_Render_GetMaterialNodeFromSlotAndNotify`; refresh pass 10 cache; `sub_827E4B48` category + `sub_827E4BE8` name strings. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82630FE0 | sub_82630FE0 | Placement-delete wrapper; vtable `off_82113260` already has full dtor named. |
| 0x826338C0 | sub_826338C0 | Placement-delete wrapper around named undefined-binding dtor. |
| 0x826247C8 | sub_826247C8 | Thin wrapper `sub_82624570(..., 0)`. |
| 0x8260FAB8 | sub_8260FAB8 | Thin wrapper `FM2_Lua_GetOrCreateObjectElementForBinding`. |
| 0x827DDB08 | sub_827DDB08 | Import thunk only. |
| 0x827DD100 | sub_827DD100 | Thin wrapper `sub_827EE6E0(*(a1+536))`. |
| 0x82621F28 | sub_82621F28 | Large recursive material-pass dispatch; defer one iter. |
| 0x82622370 | sub_82622370 | 856-byte unit-string ptrmap mutation; needs deeper read. |
| 0x82621998 | sub_82621998 | Small string-parse helper; pair with coercion cluster next. |
| 0x827E4B48 | sub_827E4B48 | Covered by parent `FM2_Render_InvalidateMaterialPass10AndSetProfileStrings`. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
| 0x8248D4C8 | sub_8248D4C8 | 4.5KB AI driver state machine. |
