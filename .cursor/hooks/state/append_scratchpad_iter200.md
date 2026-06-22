## Iteration 200

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825FA6A8 | sub_825FA6A8 | FM2_SceneGraph_RbTree_RotateRightChildAt8 | 0.91 | Right rotation via child at `+8`; parent/sibling pointer fixups; sentinel byte `+49`; used by `FM2_SceneGraph_RbTreeInsertNodeWithRebalance`. |
| 0x825FA710 | sub_825FA710 | FM2_SceneGraph_RbTree_RotateLeftChildAt8 | 0.91 | Left rotation via child at `+8`; mirror of right rotate; paired in scene-graph insert rebalance. |
| 0x825FC6F0 | sub_825FC6F0 | FM2_SceneGraph_InitRbTreeNodeWithString | 0.90 | Sets tree links at `+0/+4/+8`; `FM2_Stl_String_CopyAssign` at `+12`; `FM2_Render_NotifyManagerStateChange` at `+40`; RB color byte `+48/+49`. |
| 0x825FF0C0 | sub_825FF0C0 | FM2_SceneGraph_RbTreeInsertByNamePrefix | 0.89 | `lower_bound` walk with `FM2_SceneGraph_CompareNodeNamePrefix`; calls `FM2_SceneGraph_RbTreeInsertNodeWithRebalance`; returns iterator + inserted flag. |
| 0x825FA2C0 | sub_825FA2C0 | FM2_SceneGraph_RbTreeIteratorDecrement | 0.90 | STL `operator--` on RB-tree iterator; walks predecessor via `+8`/`+4` links; sentinel `+49`. |
| 0x825FA620 | sub_825FA620 | FM2_SceneGraph_RbTreeIteratorIncrement | 0.90 | STL `operator++` on RB-tree iterator; walks successor via leftmost-right / parent climb; sentinel `+49`. |
| 0x82768DF0 | sub_82768DF0 | FM2_NetworkMessage_GetSlotRecordField0 | 0.92 | Returns `*(FM2_NetworkMessage_GetSlotRecordBase(a1))`; used by send-slot resolver chain walk. |
| 0x8276BB38 | sub_8276BB38 | FM2_NetworkMessage_GetSlotRecordField12 | 0.92 | Returns dword at slot record `+12`; pairs with `FM2_NetworkMessage_IsSlotRecordField12Set`. |
| 0x82768E60 | sub_82768E60 | FM2_NetworkMessage_GetSlotRecordField16 | 0.92 | Returns dword at slot record `+16`; compared against field20 in send cursor advance. |
| 0x82768E90 | sub_82768E90 | FM2_NetworkMessage_GetSlotRecordField20 | 0.92 | Returns dword at slot record `+20`; incremented by send cursor advance helper. |
| 0x8276BAE8 | sub_8276BAE8 | FM2_NetworkMessage_AddSlotRecordField4 | 0.91 | `*(GetSlotRecordBase+4) += a2`; called when advancing send slot cursor. |
| 0x8276BA48 | sub_8276BA48 | FM2_NetworkMessage_TryAdvanceSendSlotCursor | 0.88 | `++field20`; copies peer `field20` into `field4`; returns `field20 <= field16`; gates `ResolveSendSlotTarget`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x824F0160 | sub_824F0160 | 8B offset helper `a1+12` only. |
| 0x82785028 | sub_82785028 | 16B offset helper `a1+308` only. |
| 0x8277CCF8 | sub_8277CCF8 | 16B offset helper `a1+32` only. |
| 0x824A6B28 | sub_824A6B28 | 8B store `*(a1+16)=a2` only. |
| 0x824287B0 | sub_824287B0 | Global singleton dword fetch only; needs `off_82998468` context. |
| 0x827F1550 | sub_827F1550 | Thin wrapper → `FM2_Profile_BinarySearchSortedDwordKeyRecord(a1,a2,0)`. |
| 0x827DD108 | sub_827DD108 | 8B deref `*(int*)a2` only. |
| 0x8279D7E8 | sub_8279D7E8 | Wrapper chain → `sub_8279F5E0` → `sub_827A0400`. |
| 0x827B6740 | sub_827B6740 | Thin wrapper → `FM2_Render_StateVector_ClearAndFree40` only. |
| 0x82424490 | sub_82424490 | CRT `amsg_exit(2)` invalid-parameter stub; defer CRT cluster. |
| 0x827B9D48 | sub_827B9D48 | Thunk to `dword_82A43E74` only. |
