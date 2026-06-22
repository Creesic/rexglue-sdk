## Iteration 192

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82792980 | sub_82792980 | FM2_Render_StateIterator_NotEqualAtIndex | 0.88 | Returns `!FM2_Render_StateIterator_EqualAtIndex`; 16 render state-loop vtable thunks. |
| 0x826347A0 | sub_826347A0 | FM2_Lua_ToluaPushUserdataAndRegisterGc | 0.90 | `FM2_Lua_ToluaPushUserdataFromUbox` then `FM2_Lua_ToluaRegisterGcHookIfMissing`; 16 tolua push callers. |
| 0x826159E0 | sub_826159E0 | FM2_RefCount_DecrementThreadSafeAndTestZero | 0.91 | `FM2_ThreadLocalCsArray_LockByType`; dec `+4`; returns 0 if still >0 else self; 16 Lua/runtime callers. |
| 0x824A9A08 | sub_824A9A08 | FM2_Lua_GrowConstTableAndGetSlotPtr | 0.90 | Grows via `sub_824A8D28` when `index+1 > cap`; returns `base + 4*index`; 16 Lua const-table callers. |
| 0x824F7C10 | sub_824F7C10 | FM2_Render_CopyManagerNotifyState160 | 0.91 | Two `FM2_Render_NotifyManagerStateChange`; copies `+8` block + 24B at `+132` + float `+156`; 16 render state copies. |
| 0x82502100 | sub_82502100 | FM2_Render_PostProcessState_Dtor | 0.90 | Vtables `off_82043D54`/`off_82043D4C`; releases COM refs at `+4/+5/+6`; `FM2_Object_AssignBaseVtable_82000E18`; 16 dtors. |
| 0x8276B9B0 | sub_8276B9B0 | FM2_NetworkMessage_SetSlotRecordField8 | 0.89 | `FM2_NetworkMessage_GetSlotRecordBase`; stores dword at `+8`; 16 network slot thunks. |
| 0x82501FF8 | sub_82501FF8 | FM2_Render_PostProcessState_DeleteIfRefZero | 0.89 | `arg2==2`; vtable `+24` destroy; release `+8`; returns refcount==0; 16 COM delete vtable entries. |
| 0x82468FD8 | sub_82468FD8 | FM2_Physics_FindNearestGripSampleIndex | 0.90 | Loops grip samples; SIMD `vsubfp`/`vmsum3fp` distance² min; uses `sub_82468C38`/`sub_82468EE0`; 16 physics callers. |
| 0x8242BDE8 | sub_8242BDE8 | FM2_ComObject_QueryBoolViaVtable140 | 0.88 | Invokes vtable `+140` into local bool; writes back to out-param; 16 COM query vtable thunks. |
| 0x821E6428 | sub_821E6428 | FM2_ComObject_IsStaticLifetimeReady | 0.87 | `FM2_ComObject_GetStaticLifetimeBlock` vtable `+112`; then ref-count vtable `+7` check; 16 AI/menu init callers. |
| 0x8232C3F8 | sub_8232C3F8 | FM2_CareerRace_CompareCarUpgradeModifier | 0.91 | Compares graphics-stream emptiness at `+440`; else `FM2_CareerRace_GetUpgradeModifierOrStockTune` ordering; 16 car-sort callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8279D7E8 | sub_8279D7E8 | Wrapper chain → `sub_8279F5E0` → `sub_827A0400`. |
| 0x827B6740 | sub_827B6740 | Thin wrapper → `FM2_Render_StateVector_ClearAndFree40` only. |
| 0x825FE048 | sub_825FE048 | Thin wrapper → `FM2_CarAudioMixChannel_ReplaceVoiceRange_0` only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only. |
| 0x82779608 | sub_82779608 | `FM2_Noop` only. |
| 0x82782C68 | sub_82782C68 | Thin wrapper only. |
| 0x827A6380 | sub_827A6380 | Thin wrapper → `FM2_Render_SliceHandleView_Dtor` only. |
| 0x827DDAB8 | sub_827DDAB8 | Thin wrapper → `sub_827E4C40` field fetch only. |
| 0x82424C00 | sub_82424C00 | CRT `ungetc`; defer stdio cluster. |
| 0x8275E148 | sub_8275E148 | CRT wide-char fread; defer stdio cluster. |
| 0x824DAEE8 | sub_824DAEE8 | Single nested vtable `+56` delegate only. |
| 0x8242C960 | sub_8242C960 | Thin ComPtr assign → `sub_8242C7A0` wrapper only. |
| 0x824A8D28 | sub_824A8D28 | Const-table grow helper; defer with slot accessor cluster. |
| 0x82224658 | sub_82224658 | Bounded `FM2_Crt_VsprintfS_L` wrapper; defer CRT cluster. |
| 0x825FA428 | sub_825FA428 | Parent-chain walk at `+172`; needs more cluster context. |
| 0x82468C38 | sub_82468C38 | Grip sample count cache; defer with nearest-index cluster. |
| 0x82468EE0 | sub_82468EE0 | Grip sample vec fetch; defer with physics grip cluster. |
| 0x82762C18 | sub_82762C18 | Bounded sprintf thunk → `sub_82388A48`. |
