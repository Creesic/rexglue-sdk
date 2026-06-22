
## Iteration 57

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82574218 | sub_82574218 | render_deferred_task_enqueue_presentation_block_64_vtbl_48efc | 0.87 | Pool alloc 0x50; vtable off_82048EFC; FM2_MemcpyAligned 64-byte block at +16; ReleaseChild. Presentation provider vtable 0x82191920. |
| 0x82574310 | sub_82574310 | render_deferred_task_enqueue_presentation_block_64_vtbl_48f04 | 0.87 | Pool alloc 0x50; vtable off_82048F04; FM2_MemcpyAligned 64-byte block at +16; ReleaseChild. Presentation provider vtable 0x82191930. |
| 0x82574A90 | sub_82574A90 | render_deferred_task_enqueue_presentation_block_64_vtbl_4903c | 0.87 | Pool alloc 0x50; vtable off_8204903C; FM2_MemcpyAligned 64-byte block at +16; ReleaseChild. Presentation provider vtable 0x821919A8. |
| 0x82576818 | sub_82576818 | render_deferred_task_release_with_byte_payload_vtbl_49234 | 0.87 | Pool alloc 8; vtable off_82049234; byte at +4; ReleaseChild. Presentation provider vtable 0x82191B30. |
| 0x82576910 | sub_82576910 | render_deferred_task_release_with_byte_payload_vtbl_49244 | 0.87 | Pool alloc 8; vtable off_82049244; byte at +4; ReleaseChild. Presentation provider vtable 0x82191B40. |
| 0x82576980 | sub_82576980 | render_deferred_task_release_with_byte_payload_vtbl_4924c | 0.87 | Pool alloc 8; vtable off_8204924C; byte at +4; ReleaseChild. Presentation provider vtable 0x82191B48. |
| 0x82576CB0 | sub_82576CB0 | render_deferred_task_release_with_byte_payload_vtbl_4926c | 0.87 | Pool alloc 8; vtable off_8204926C; byte at +4; ReleaseChild. Presentation provider vtable 0x82191B78. |
| 0x82576D38 | sub_82576D38 | render_deferred_task_release_with_dword_payload_vtbl_49274 | 0.87 | Pool alloc 8; vtable off_82049274; dword at +4; ReleaseChild. Presentation provider vtable 0x82191B80. |
| 0x82576E48 | sub_82576E48 | render_deferred_task_release_with_dword_payload_vtbl_49284 | 0.87 | Pool alloc 8; vtable off_82049284; dword at +4; ReleaseChild. Presentation provider vtable 0x82191B90. |
| 0x82577168 | sub_82577168 | render_deferred_task_release_with_dword_payload_vtbl_492e8 | 0.87 | Pool alloc 8; vtable off_820492E8; dword at +4; ReleaseChild. Presentation provider vtable 0x82191BC0. |
| 0x82577230 | sub_82577230 | render_deferred_task_release_with_float_payload_vtbl_49318 | 0.87 | Pool alloc 8; vtable off_82049318; float at +4; ReleaseChild. Presentation provider vtable 0x82191BD0. |
| 0x82577BC8 | sub_82577BC8 | render_deferred_queue_command_params_dtor_thunk | 0.90 | Thin wrapper: render_deferred_queue_command_params_dtor(a1); returns a1. Vtable xrefs 0x820495A8 and 0x82191CA0. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82574A28 | sub_82574A28 | Thin vtable+32 invoke; deferred to iteration 58 (last in 0x82574200–0x82578100 cluster). |
