## Iteration 198

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8232D360 | sub_8232D360 | FM2_AsyncWrapper_ReleaseComRef | 0.91 | `IAsyncWrapper` vtable; COM `Release` on held object at `a1[1]` then nulls slot; sole caller `FM2_AsyncWrapper_Dtor`. |
| 0x8221A930 | sub_8221A930 | FM2_HashName_AssignSlashPathIfEnabled | 0.90 | Gate on `result+8`; calls `FM2_HashName_AssignFromSlashPath`; 13 property-bag callers. |
| 0x82644230 | sub_82644230 | FM2_AIDriver_ApplyRaceLineLateralJitter | 0.88 | `FM2_AIDriver_ResetRaceLineInterpBScalar`; `lcg_rand_range_float`; vtable `+708` scale; fsel clamps lateral offset into `*a1`; AI race-line cluster. |
| 0x8264A7A8 | sub_8264A7A8 | FM2_AIDriver_BlendRaceLineOffsetWithFsel | 0.87 | Calls `FM2_AIDriver_ApplyRaceLineLateralJitter` then PPC `fsel` blend of two float inputs; 13 AI physics callers. |
| 0x82768D00 | sub_82768D00 | FM2_NetworkMessage_GetSlotRecordField8 | 0.92 | Returns `*(FM2_NetworkMessage_GetSlotRecordBase(a1) + 8)`; pairs with `FM2_NetworkMessage_SetSlotRecordField8`; network send cluster. |
| 0x82768E20 | sub_82768E20 | FM2_NetworkMessage_IsSlotRecordField12Set | 0.91 | `_cntlzw` test on dword at slot record `+12`; used by `FM2_NetworkMessage_ResolveSendSlotTarget`. |
| 0x8276B760 | sub_8276B760 | FM2_NetworkMessage_GetNextSlotRecordOffset36 | 0.90 | Validates index in 36-byte slot array; returns `a2+36` or 0; called from send-slot resolver. |
| 0x82768D30 | sub_82768D30 | FM2_NetworkMessage_ResolveSendSlotTarget | 0.88 | Reads field8; branches on field12-set vs slot-chain walk via `GetNextSlotRecordOffset36`; falls back to `sub_82768CA0`. |
| 0x827E4D18 | sub_827E4D18 | FM2_Profile_InvokeManagerCallbacksUnderCritSec | 0.89 | Critsec at `a1+13900`; iterates callback vector at `+1884` and per-category table at `+2264`; 13 profile callers. |
| 0x8277D578 | sub_8277D578 | FM2_Stl_List_InsertNodeBefore | 0.90 | Classic STL doubly-linked list insert: rewires `prev/next` around sentinel node; 13 render/network callers. |
| 0x82363688 | sub_82363688 | FM2_Crt_FreeWithTrackingHook | 0.91 | `FM2_Memory_IsTrackingAllocsEnabled` → `FM2_Memory_RecordFreeIfTracking`; then `FM2_Crt_JumpToFreeTail`; 13 dtor paths. |
| 0x821FCC08 | sub_821FCC08 | FM2_LiveryEditor_FindOrInsertDecalTabNodeWithNotify | 0.90 | `FM2_IntrusiveList_InitIteratorWithSentinel`; `FM2_Render_NotifyManagerStateChange`; `FM2_LiveryEditor_RbTreeInsertDecalTabKey_0`; returns node `+16`. |

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
| 0x827788A8 | sub_827788A8 | Thin wrapper → `sub_8277C7E8` → `sub_8277DF80` only. |
| 0x82768CA0 | sub_82768CA0 | Network send retry path; rename after sibling cluster settled (iter 199). |
| 0x82600348 | sub_82600348 | Scene-graph find/insert with notify; defer to next batch. |
| 0x82413C80 | sub_82413C80 | `fabs`+`fsel` sign-copy helper; needs math-cluster naming pass. |
