## Iteration 191

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8276E950 | sub_8276E950 | FM2_Stl_StringGetBufferPointer | 0.93 | MSVC `std::string` SSO: if `capacity@+24 < 0x10` return `this+4` else `*(this+4)`; 17 string/network callers. |
| 0x826561E8 | sub_826561E8 | FM2_CarAudioMix_GetMaterialEntryAt152 | 0.91 | Indexed access into vector at `+16` with 152-byte stride; bounds trap; 17 car-audio mix callers. |
| 0x821E6348 | sub_821E6348 | FM2_AIDriver_LerpPathSegmentHalf | 0.92 | `FM2_AIDriver_GetPathBufferLength`; SIMD `vsubfp`/`vmulfp` with splat `0.5`; writes vec128 to out; 17 AI path callers. |
| 0x82790848 | sub_82790848 | FM2_Render_StateIterator_GetPoolEntry | 0.90 | Iterator index bounds checks; maps index table → `40*slot` pool entry; 4 iterator deref callers. |
| 0x82789188 | sub_82789188 | FM2_Render_StateIterator_DerefPoolEntry | 0.89 | Vtable thunk forwarding to `FM2_Render_StateIterator_GetPoolEntry`; 26 render state iterators. |
| 0x82789768 | sub_82789768 | FM2_Render_StateIterator_EqualAtIndex | 0.91 | Requires same container `+4`; compares index at `+8`; used by iterator compare wrapper. |
| 0x827B0670 | sub_827B0670 | FM2_Render_TransactionLogReader_Dtor | 0.91 | Calls `FM2_Render_TransactionLogReader_InitVtable`; optional `FM2_Memory_FreeSmallBlockOrNull`; vtable `off_8214C9A0` entry 0. |
| 0x827B6828 | sub_827B6828 | FM2_Render_StateVector_ClearAndFree40 | 0.90 | Clears 40-byte element vector via `sub_8276C4D8` + `FM2_STL_VectorClearAndFreeRange_0`; zeros begin/end/cap; vtable xref. |
| 0x82793088 | sub_82793088 | FM2_Stl_StringCopyNulTerminate | 0.90 | Bounded char copy with NUL terminate; `E_INVALIDARG`/`STRUNCATE` HRESULTs; core of `FM2_Render_BoundedStringCopy`. |
| 0x826343D8 | sub_826343D8 | FM2_Lua_ToluaValidateStringArgOrFillTypeError | 0.91 | Stack depth/type checks; rejects number/boolean; fills error triple with `"string"`; 16 tolua binding callers. |
| 0x82632D38 | sub_82632D38 | FM2_Lua_ToluaRegisterGcHookIfMissing | 0.91 | Looks up `"tolua_gc"` in registry; registers userdata→gc func if absent; returns 0 on success; 16 push-userdata callers. |
| 0x827EBA50 | sub_827EBA50 | FM2_Render_InvokeElementCallbackWithVec128 | 0.90 | `lvx128` store to `this`; invokes callback triple at `+16/+20`; 16 element-link/render callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82792980 | sub_82792980 | Thin `!FM2_Render_StateIterator_EqualAtIndex` wrapper only. |
| 0x827B6740 | sub_827B6740 | Thin wrapper → `FM2_Render_StateVector_ClearAndFree40` only. |
| 0x8279D7E8 | sub_8279D7E8 | Wrapper chain → `sub_8279F5E0` → `sub_827A0400`. |
| 0x825FE048 | sub_825FE048 | Thin wrapper → `FM2_CarAudioMixChannel_ReplaceVoiceRange_0` only. |
| 0x82767500 | sub_82767500 | 24B vtable reset only. |
| 0x82779608 | sub_82779608 | `FM2_Noop` only. |
| 0x82782C68 | sub_82782C68 | Thin wrapper only. |
| 0x827A6380 | sub_827A6380 | Thin wrapper → `FM2_Render_SliceHandleView_Dtor` only. |
| 0x827DDAB8 | sub_827DDAB8 | Thin wrapper → `sub_827E4C40` field fetch only. |
| 0x82424C00 | sub_82424C00 | CRT `ungetc`; defer stdio cluster. |
| 0x8275E148 | sub_8275E148 | CRT wide-char fread; defer stdio cluster. |
| 0x824DAEE8 | sub_824DAEE8 | Single nested vtable `+56` delegate only. |
| 0x826347A0 | sub_826347A0 | Composes push-userdata + GC register; defer thin glue. |
| 0x826159E0 | sub_826159E0 | Thread-local refcount decrement; defer ref-count cluster. |
| 0x824A9A08 | sub_824A9A08 | Lua const-table slot grow; defer with `sub_824A8D28`. |
| 0x824F7C10 | sub_824F7C10 | Render manager state copy; defer notify cluster. |
| 0x82502100 | sub_82502100 | Post-process state dtor; defer COM release cluster. |
| 0x8276B9B0 | sub_8276B9B0 | Thin `FM2_NetworkMessage_GetSlotRecordBase` field store. |
