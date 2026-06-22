## Iteration 201

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827F14A8 | sub_827F14A8 | FM2_Profile_BinarySearchSortedDwordKeyRecord | 0.91 | Binary search 48-byte sorted vector; dword key at `+0`; returns payload at `+16`; optional out-index; 35+ profile callers. |
| 0x827F1550 | sub_827F1550 | FM2_Profile_LookupSortedDwordKeyRecord | 0.90 | Thin wrapper → `FM2_Profile_BinarySearchSortedDwordKeyRecord(a1,a2,0)`; profile variant lookup entry point. |
| 0x827F1558 | sub_827F1558 | FM2_Profile_ContainsSortedDwordKey | 0.90 | Returns `BinarySearch(...) != 0`; BOOL membership test on sorted dword-key table. |
| 0x827F1F88 | sub_827F1F88 | FM2_Profile_UpsertSortedDwordKeyRecord48 | 0.89 | Lookup by key; on miss calls `FM2_Profile_InsertSortedDwordKeyRecord48`; else copies 32-byte payload (4 QWORDs). |
| 0x827F1DC8 | sub_827F1DC8 | FM2_Profile_InsertSortedDwordKeyRecord48 | 0.88 | Grows sorted 48-byte vector; shifts elements; inserts key+dword payload at index; uses vector append helper. |
| 0x824287B0 | sub_824287B0 | FM2_SQLite_GetTextEncodingMode | 0.91 | Returns `*((_DWORD*)off_82998468 + 43)`; `FM2_SQLite_StringLooksLikeNumber` branches `<=1` vs `FM2_Sqlite_GetOpenFlagMask`. |
| 0x824F3168 | sub_824F3168 | FM2_RaceGhost_GetTimedEventControllerFromReplayBuffer | 0.89 | Returns `*(replayBuffer+50616)` pointer; fed to `FM2_RaceGhost_IsActiveTimedEventWindowOpen`; Lua ghost queries. |
| 0x824F48D0 | sub_824F48D0 | FM2_RaceGhostManager_GetActiveReplayBufferPtr | 0.92 | Returns `*(manager+8332)`; 36 race-ghost/Lua callers before replay-buffer field access. |
| 0x824F4210 | sub_824F4210 | FM2_RaceGhost_IsActiveTimedEventWindowOpen | 0.90 | Requires `+136==1`; compares `KeQuerySystemTime` against SYSTEMTIME pair at `+104/+120`. |
| 0x825FC2F0 | sub_825FC2F0 | FM2_SceneGraph_RbTreeEraseNodeAndRebalance | 0.90 | STL erase + RB rebalance via rotate helpers; `FM2_SceneGraph_UpdateNodeWithNotifyStateField`; `FM2_Memory_FreeSmallBlockOrNull`. |
| 0x825FB978 | sub_825FB978 | FM2_SceneGraph_ForEachNodeInRangeInvokeVtable32 | 0.89 | RB-tree iterator loop; calls node object vtable `+32` with range args; uses `RbTreeIteratorIncrement`. |
| 0x825B7648 | sub_825B7648 | FM2_TuningUI_ConvertPoint2DAcrossAxisModes | 0.90 | Builds `TPoint2D<float>` vftable; remaps X/Y using profile controller/hash counts when axis modes differ. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x824F0160 | sub_824F0160 | 8B offset helper `a1+12` only. |
| 0x82785028 | sub_82785028 | 16B offset helper `a1+308` only. |
| 0x8277CCF8 | sub_8277CCF8 | 16B offset helper `a1+32` only. |
| 0x82633048 | sub_82633048 | Lua const-metatable registration; defer until `sub_82632960`/`sub_826326F8` cluster named. |
| 0x8261F3E8 | sub_8261F3E8 | Large Lua object-element factory; needs deeper binding-context naming. |
| 0x827DF140 | sub_827DF140 | Recursive render material matrix resolve; defer material-pass cluster. |
| 0x824897B0 | sub_824897B0 | AI path SIMD dot-product compare; defer AI blocking-margin cluster. |
| 0x821F1B70 | sub_821F1B70 | Pure VMX128 math kernel; no strings/callees for semantic name yet. |
| 0x8279D7E8 | sub_8279D7E8 | Wrapper chain → `sub_8279F5E0` → `sub_827A0400`. |
| 0x827B6740 | sub_827B6740 | Thin wrapper → `FM2_Render_StateVector_ClearAndFree40` only. |
