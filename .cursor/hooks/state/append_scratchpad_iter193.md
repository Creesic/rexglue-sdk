## Iteration 193

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82468C38 | sub_82468C38 | FM2_Physics_GetGripSampleCountCached | 0.91 | Lazily caches grip sample count at `a2+993`; sums tier arrays + base offsets; used by `FM2_Physics_FindNearestGripSampleIndex` cluster. |
| 0x82468E28 | sub_82468E28 | FM2_Physics_ResolveGripSampleTierAndLocalIndex | 0.90 | Maps global grip index to tier code (8/9/10/11+) and local index via `FM2_Physics_GetGripSampleCountCached`; 7 physics callers. |
| 0x82468EE0 | sub_82468EE0 | FM2_Physics_GetGripSampleVecAtTier | 0.90 | Calls count cache + tier resolver + `sub_824684A8` vec fetch; 14 physics grip callers including nearest-index search. |
| 0x824A8D28 | sub_824A8D28 | FM2_Lua_GrowConstTableAligned | 0.91 | Rounds capacity to growth unit `a1[2]`; pool alloc, memset, copy old slots, free old; caller of `FM2_Lua_GrowConstTableAndGetSlotPtr`. |
| 0x8276BDD0 | sub_8276BDD0 | FM2_NetworkMessage_GetSlotRecordCount | 0.92 | Returns `(*+8 - *+4) / 40` or 0; 15 network slot/message callers near `FM2_NetworkMessage_GetSlotRecordBase`. |
| 0x82295898 | sub_82295898 | FM2_TuningRecord_ScrollSliderAdjustAndPropagate | 0.90 | `FM2_TuningRecord_AdjustScrollSliderUp/Down` then propagates float to adjacent slider slots; 15 tuning UI callers. |
| 0x8278F548 | sub_8278F548 | FM2_Render_StateRingBuffer_PushCopy40 | 0.89 | Ring-buffer grow/push; `FM2_STL_CopyConstructRange40_BodyThunk`; lazy slot alloc via `sub_82789730`; 15 render state callers. |
| 0x8242C288 | sub_8242C288 | FM2_ComStream_ReadOrWriteVec128 | 0.91 | Vtable `+48` bool picks `+32` write vs `+36` read four dwords; stores `lvx128` to dest; 15 COM stream vtable entries. |
| 0x82462A08 | sub_82462A08 | FM2_MediaData_OpenSourceFileHandle | 0.92 | Secured-path probe then `CreateFileA`; fatal `"Could not open source media data file."`; 8B handle wrapper alloc; 15 media loaders. |
| 0x82500490 | sub_82500490 | FM2_LiveryEditor_FindOrInsertDecalTabNode | 0.90 | `FM2_IntrusiveList_InitIteratorWithSentinel` + `FM2_LiveryEditor_RbTreeInsertDecalTabKey`; returns node `+16`; 15 livery callers. |
| 0x82465FA8 | sub_82465FA8 | FM2_Physics_ClassifyGripSign | 0.91 | Classifies float grip at indexed offset: 1 if >0, 0 if <0, 2 if ==0, else 3; 15 physics surface-grip callers. |
| 0x824CDF18 | sub_824CDF18 | FM2_Lua_CountLibRegFieldChainLength | 0.88 | Walks lib-reg field sibling chain via `+12/+16/+20`; count drives XML lib-reg vector alloc in `sub_82292CD0`. |

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
| 0x827E4A68 | sub_827E4A68 | Thin wrapper → `sub_827E0E40(..., 10, ...)` only. |
| 0x824684A8 | sub_824684A8 | 16B pointer math `16*idx + base+20`; defer with grip vec cluster. |
| 0x82334CF0 | sub_82334CF0 | `CRefCountedObjectThreadSafe` dtor; defer COM dtor cluster. |
| 0x8242DD80 | sub_8242DD80 | `IOSys::CFileEvent` dtor; defer IOSys cluster. |
| 0x8245C5A0 | sub_8245C5A0 | Wide-string STL assign; UTF conversion path needs more context. |
| 0x8253E4E0 | sub_8253E4E0 | RB-tree lower_bound iterator pair; needs `sub_8253DE08` cluster. |
| 0x827D78E8 | sub_827D78E8 | Thin wrapper → `sub_827D77D8` after material pass lookup. |
| 0x827E7EF0 | sub_827E7EF0 | BufFile write + hash lookup wrapper; needs profile cluster context. |
| 0x82224658 | sub_82224658 | Bounded `FM2_Crt_VsprintfS_L` wrapper; defer CRT cluster. |
| 0x825FA428 | sub_825FA428 | Parent-chain walk at `+172`; needs more cluster context. |
| 0x82762C18 | sub_82762C18 | Bounded sprintf thunk → `sub_82388A48`. |
