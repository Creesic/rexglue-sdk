## Iteration 194

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x824684A8 | sub_824684A8 | FM2_Physics_GetGripSampleArrayEntryPtr | 0.90 | Returns `16*index + *(base+20)`; called by `FM2_Physics_GetGripSampleVecAtTier`; 15 physics grip callers. |
| 0x82334CF0 | sub_82334CF0 | FM2_ComObject_RefCountedThreadSafe_Dtor | 0.92 | `CRefCountedObjectThreadSafe` vtable; `FM2_Object_AssignBaseVtable_82000E18`; optional pool free; 15 COM vtable dtors. |
| 0x8242DD80 | sub_8242DD80 | FM2_IOSys_FileEvent_Dtor | 0.92 | `IOSys::CFileEvent` vtable; clears STL string at `+10`; optional free; 15 IOSys vtable dtors. |
| 0x8228B968 | sub_8228B968 | FM2_Core_SystemEventParam_Dtor | 0.92 | `Core::CSystemEventParam` vtable; base vtable assign + optional free; 14 core event vtable dtors. |
| 0x824825E0 | sub_824825E0 | FM2_CallbackSlot_InstallHookIfEnabled | 0.88 | If `+20` enabled: invoke old callback, swap fn ptr, reset counter, call new hook with `-1`; 15 physics/AI callback installs. |
| 0x8278C648 | sub_8278C648 | FM2_Render_TsConnection_PushFrameSliceState | 0.91 | `TSConnection.cpp` assert; frame vtable `+12`; `FM2_Render_InitSliceHandleWithNotifier` + `FM2_Render_StateRingBuffer_PushCopy40`; 14 render callers. |
| 0x82487090 | sub_82487090 | FM2_AIOvertake_IsRivalAheadByTimeMargin | 0.89 | Compares rival vs self race-time delta via vtable `+816/+844`; SIMD vec at `+2128`; threshold at `+672`; 13 AI overtake callers. |
| 0x82488DA0 | sub_82488DA0 | FM2_AIOvertake_ShouldAttemptPassAtSlot | 0.90 | `FM2_AIOvertake_RefreshPassTargetIndex`; checks slot `+548` active flag and time vs `+668 * scale`; 14 overtake state callers. |
| 0x82487F00 | sub_82487F00 | FM2_AIOvertake_RefreshPassTargetIndex | 0.90 | Caches `FM2_AIOvertake_GetGlobalRaceTimeFloat` at `+204`; scans 24B slots at `+208`; picks min-time pass target at `+548`; 6 overtake callers. |
| 0x82352820 | sub_82352820 | FM2_TuningUI_ConstructPoint2DFromScrollOffset | 0.89 | `TPoint2D<float>` vtable; maps scroll offsets through tuning scale fields at `+68/+72/+80/+88`; flag at `+96`; 14 tuning UI callers. |
| 0x825FA428 | sub_825FA428 | FM2_ComObject_WalkParentWhileRefNonNull | 0.88 | Walks `+172` parent chain while dword non-zero (`_cntlzw` test); 16 COM hierarchy callers. |
| 0x827C3CB8 | sub_827C3CB8 | FM2_Crt_DeleteScalarOrVector | 0.91 | If `*(ptr-2)==3` invoke dtor at `ptr-8` with size `*(ptr-1)`; else `FM2_Crt_Free`; 15 CRT delete thunks. |

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
| 0x8242C920 | sub_8242C920 | Thin ComPtr assign → `sub_8242C660` wrapper only. |
| 0x827E4A68 | sub_827E4A68 | Thin wrapper → `sub_827E0E40(..., 10, ...)` only. |
| 0x827D78E8 | sub_827D78E8 | Thin wrapper → `sub_827D77D8` after material pass lookup. |
| 0x827E7EF0 | sub_827E7EF0 | BufFile write + hash lookup wrapper; defer profile cluster. |
| 0x827E8358 | sub_827E8358 | BufFile write + hash lookup wrapper; defer profile cluster. |
| 0x82224658 | sub_82224658 | Bounded `FM2_Crt_VsprintfS_L` wrapper; defer CRT cluster. |
| 0x82762C18 | sub_82762C18 | Bounded sprintf thunk → `sub_82388A48`. |
| 0x8245C5A0 | sub_8245C5A0 | Wide-string STL assign; UTF conversion path needs more context. |
| 0x8260AFA8 | sub_8260AFA8 | RB-tree left rotate sentinel `+61`; defer profile RB-tree cluster with `0x8260ADF0`. |
| 0x8260ADF0 | sub_8260ADF0 | RB-tree right rotate sentinel `+61`; defer profile RB-tree cluster. |
| 0x8253D778 | sub_8253D778 | Profile RB-tree lower_bound; defer with `0x8253DE08`/`0x8253E4E0`. |
| 0x8253DE08 | sub_8253DE08 | Profile RB-tree iterator init; defer cluster. |
| 0x8253E4E0 | sub_8253E4E0 | Profile RB-tree insert-hint pair; defer cluster. |
| 0x8253E5D0 | sub_8253E5D0 | Profile editor state dtor; defer with RB-tree cluster. |
| 0x827D1E40 | sub_827D1E40 | 160-byte state vector copy; defer render vector cluster. |
| 0x824A1DD0 | sub_824A1DD0 | `FM2_PresentationCarConfig_Dtor` + COM release; defer car-config dtor cluster. |
