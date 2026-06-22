## Iteration 210

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827E4EB8 | sub_827E4EB8 | FM2_Render_InvokeProfileCallbacksOnUnitStringLink | 0.89 | `RtlEnterCriticalSection` at `a1+13900`; walks global + per-node-type callback vectors; invokes `(node, arg, field+28)`; used on unit-string link completion. |
| 0x827E5218 | sub_827E5218 | FM2_Render_CompleteUnitStringLinkAndDestroyPending | 0.90 | Calls profile callback fanout then `sub_827E47F0` vtable destroy; 5 unit-string link callers including flush queue. |
| 0x827E4A08 | sub_827E4A08 | FM2_Render_IsUnitStringLinkXmlNodeResolvable | 0.89 | `FM2_XmlNavigator_FindMatchingNode_0(node, 6, ...)` readiness probe; gates `FM2_Render_FlushPendingUnitStringLinkQueue`. |
| 0x827DD200 | sub_827DD200 | FM2_Render_LookupUnitStringChildIdBySlotSelector | 0.90 | Selects root `a1[130/131/132]` by `*a3`; `FM2_Lua_LookupOrCreateUnitStringTableSlot`; returns child id at `+4` or `-1`. |
| 0x82621360 | sub_82621360 | FM2_Render_CountUnitStringTableChildren | 0.91 | `FM2_Lua_GetUnitStringTableEntryPtr`; returns `(end-begin)>>2` child count or 0. |
| 0x82620B80 | sub_82620B80 | FM2_Lua_InitBindingSlotWithUint8NumberValue | 0.90 | Type `1`; pushes `(uint8 \| 0x100000000)` as double; deferred uint8 variant from iter 209. |
| 0x82620BE8 | sub_82620BE8 | FM2_Lua_InitBindingSlotWithUint16AsInt32Value | 0.90 | Type `1`; pushes `(uint16 \| 0x100000000)` as double; distinct from signed int16 slot init. |
| 0x82621000 | sub_82621000 | FM2_Lua_PushBindingSlotFromBoolByVariantType | 0.91 | Switch on variant type `2/9/5/14`; coerces bool to bool/double/uint32/string (`"true"`/`"false"`); removes temp slot. |
| 0x82621100 | sub_82621100 | FM2_Lua_PushBindingSlotFromDoubleByVariantType | 0.91 | Switch types `2/9/5/14`; double→bool/double/rounded-uint32/`snprintf %g` string; removes temp slot. |
| 0x826212F0 | sub_826212F0 | FM2_Lua_ResolveBindingObjectFromDeferredTaskParams | 0.89 | `FM2_Render_LookupUnitStringChildIdBySlotSelector` then `sub_8260FAB8` object-element lookup; returns 0 if child id `-1`. |
| 0x82621278 | sub_82621278 | FM2_Lua_AllocBindingEventRecordArray | 0.90 | Overflow-checked `124 * count` pool alloc; throws `std::bad_array_new_length` on OOB; used by binding-event table builder. |
| 0x8260FBE0 | sub_8260FBE0 | FM2_Lua_DispatchBindingEventWithDiagGate | 0.88 | `FM2_Diag_LogEventWithSubContext` gate; builds event context via `sub_827DDAE0`; dispatches deferred or immediate handler; else error index 1. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82620C50 | sub_82620C50 | Byte-identical uint32-as-int64 binding slot to `sub_82620B18`. |
| 0x82611DC0 | sub_82611DC0 | Generic 16-byte vector grow/append helper. |
| 0x827E47F0 | sub_827E47F0 | 32B vtable destroy thunk only. |
| 0x827E4A58 | sub_827E4A58 | 12B wrapper → `sub_827D70A8(..., 10)`. |
| 0x827DDAE0 | sub_827DDAE0 | Thin deref `sub_827E4968(*a1)+8`. |
| 0x8260FAB8 | sub_8260FAB8 | 8B thunk → `FM2_Lua_GetOrCreateObjectElementForBinding`. |
| 0x8260FB00 | sub_8260FB00 | Profile-field variant of diag-gated dispatch; defer pairing. |
| 0x82621430 | sub_82621430 | Thin deferred-task dispatch wrapper. |
| 0x82612288 | sub_82612288 | Large find/create binding-event record; defer with `sub_82610B10`. |
| 0x827DD100 | sub_827DD100 | 8B thunk → `sub_827EE6E0`. |
| 0x82620FF8 | sub_82620FF8 | 8B `FM2_DeferredTaskParams_GetField4` wrapper. |
| 0x827F4DB0 | sub_827F4DB0 | 2KB material-node factory switch. |
