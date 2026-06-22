## Iteration 199

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82768CA0 | sub_82768CA0 | FM2_NetworkMessage_TrySendIfSlotReady | 0.90 | Reads `GetSlotRecordField8`; gated by `sub_8276B988`; calls `FM2_NetworkMessage_ResolveSendSlotTarget`; network send retry path. |
| 0x82600348 | sub_82600348 | FM2_SceneGraph_FindOrInsertNodeByNamePrefixWithNotify | 0.91 | `FM2_SceneGraph_CompareNodeNamePrefix`; `FM2_Render_NotifyManagerStateChange`; `FM2_SceneGraph_RbTreeInsertHintByNamePrefix`; returns node `+40`. |
| 0x82413C80 | sub_82413C80 | FM2_Math_CopySignDoubleFsel | 0.88 | `__fabs` then PPC `fsel` sign restore on low mantissa dword; shared by physics/AI float paths. |
| 0x825FBC78 | sub_825FBC78 | FM2_SceneGraph_InitRbTreeIteratorLowerBound | 0.90 | Calls `FM2_SceneGraph_RbTreeLowerBoundByNamePrefix`; fills `{tree, node}` iterator pair; used by find/insert helpers. |
| 0x825FB498 | sub_825FB498 | FM2_SceneGraph_RbTreeLowerBoundByNamePrefix | 0.91 | RB-tree walk on flag byte `+49`; `FM2_SceneGraph_CompareNodeNamePrefix` at node `+12`; classic `lower_bound`. |
| 0x825FF6E0 | sub_825FF6E0 | FM2_SceneGraph_RbTreeInsertHintByNamePrefix | 0.90 | Hinted insert with predecessor/successor walks; delegates to `FM2_SceneGraph_RbTreeInsertNodeWithRebalance`. |
| 0x825FE3E8 | sub_825FE3E8 | FM2_SceneGraph_RbTreeInsertNodeWithRebalance | 0.89 | Alloc node via `sub_825FC6F0`; STL `map/set too long` guard; RB-tree rotations `sub_825FA6A8`/`sub_825FA710`. |
| 0x8276C660 | sub_8276C660 | FM2_NetworkMessage_DecrementPendingSendCount | 0.91 | If `*(a1+384)` nonzero, decrements and returns 1; else 0; used by send-slot gate. |
| 0x822B8BF8 | sub_822B8BF8 | FM2_SceneGraph_EvalPiecewiseCurveByChannelIndex | 0.88 | Four knot floats; vtable callbacks at `+420/+424/+432`; piecewise `fsel` interpolation by channel index. |
| 0x82762780 | sub_82762780 | FM2_Network_DispatchXtsTaskWithSlice | 0.89 | `FM2_Network_ValidateConnectionContextReady`; COM vtable `+84`; optional `FM2_Network_GetXtsTaskHandleFromSlice` out-param. |
| 0x826E5690 | sub_826E5690 | FM2_XAudio2_CreatePacketVoiceWithEndpointFormat | 0.90 | `FM2_XAudio2_VoicePool_AllocVoiceObject`; formats IP/MAC hex or `%u.%u.%u.%u`; voice vtable `+72` packet writes. |
| 0x827EB600 | sub_827EB600 | FM2_Profile_GetVariantDwordArrayElemPtr | 0.92 | Returns `a1 + 4*a2`; 57 profile variant-table callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8279D7E8 | sub_8279D7E8 | Wrapper chain → `sub_8279F5E0` → `sub_827A0400`. |
| 0x827B6740 | sub_827B6740 | Thin wrapper → `FM2_Render_StateVector_ClearAndFree40` only. |
| 0x82785028 | sub_82785028 | 16B offset helper `a1+308` only; defer accessor cluster. |
| 0x8277CCF8 | sub_8277CCF8 | 16B offset helper `a1+32` only. |
| 0x824A6B28 | sub_824A6B28 | 8B store `*(a1+16)=a2` only. |
| 0x824F48D0 | sub_824F48D0 | 8B getter `*(a1+8332)` only. |
| 0x82632F40 | sub_82632F40 | Thin wrapper → `FM2_Lua_SetStackTop(a1,-2)` only. |
| 0x8253DB90 | sub_8253DB90 | Thin wrapper → `FM2_IntrusiveList_SpliceNodes(a1+8,...)`. |
| 0x8276B988 | sub_8276B988 | Thin wrapper → `FM2_NetworkMessage_DecrementPendingSendCount` only. |
| 0x82482750 | sub_82482750 | 8B float store at `a1+10228` only. |
