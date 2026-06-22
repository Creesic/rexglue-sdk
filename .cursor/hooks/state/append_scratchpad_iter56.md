
## Iteration 56

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82576050 | sub_82576050 | render_deferred_task_release_with_vtable_only_f10c | 0.86 | Pool alloc 4; vtable off_8204910C only; ReleaseChild. Presentation provider vtable 0x82191AC0. |
| 0x825760D0 | sub_825760D0 | render_deferred_task_release_with_vtable_only_f114 | 0.86 | Pool alloc 4; vtable off_82049114 only; ReleaseChild. Presentation provider vtable 0x82191AC8. |
| 0x82576150 | sub_82576150 | render_deferred_task_release_with_vtable_only_f11c | 0.86 | Pool alloc 4; vtable off_8204911C only; ReleaseChild. Presentation provider vtable 0x82191AD0. |
| 0x825761D0 | sub_825761D0 | render_deferred_task_release_with_vtable_only_f124 | 0.86 | Pool alloc 4; vtable off_82049124 only; ReleaseChild. Presentation provider vtable 0x82191AD8. |
| 0x82576250 | sub_82576250 | render_deferred_task_release_with_vtable_only_f12c | 0.86 | Pool alloc 4; vtable off_8204912C only; ReleaseChild. Presentation provider vtable 0x82191AE0. |
| 0x82576410 | sub_82576410 | render_deferred_task_release_with_dword_payload_vtbl_491fc | 0.87 | Pool alloc 8; vtable off_820491FC; dword at +4; ReleaseChild. Presentation provider vtable 0x82191AF8. |
| 0x82576498 | sub_82576498 | render_deferred_task_release_with_dword_payload_vtbl_49204 | 0.87 | Pool alloc 8; vtable off_82049204; dword at +4; ReleaseChild. Presentation provider vtable 0x82191B00. |
| 0x82576520 | sub_82576520 | render_deferred_task_release_with_dword_payload_vtbl_4920c | 0.87 | Pool alloc 8; vtable off_8204920C; dword at +4; ReleaseChild. Presentation provider vtable 0x82191B08. |
| 0x825765A8 | sub_825765A8 | render_deferred_task_release_with_dword_payload_vtbl_49214 | 0.87 | Pool alloc 8; vtable off_82049214; dword at +4; ReleaseChild. Presentation provider vtable 0x82191B10. |
| 0x82576630 | sub_82576630 | render_deferred_task_release_with_float_payload_vtbl_4921c | 0.87 | Pool alloc 8; vtable off_8204921C; float at +4; ReleaseChild. Presentation provider vtable 0x82191B18. |
| 0x825766C0 | sub_825766C0 | render_deferred_task_release_with_float_payload_vtbl_49224 | 0.87 | Pool alloc 8; vtable off_82049224; float at +4; ReleaseChild. Presentation provider vtable 0x82191B20. |
| 0x82576B50 | sub_82576B50 | render_deferred_task_enqueue_presentation_block_64_vtbl_4925c | 0.87 | Pool alloc 0x50; vtable off_8204925C; FM2_MemcpyAligned 64-byte block at +16; ReleaseChild. Presentation provider vtable 0x82191B68. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82574218 | sub_82574218 | Duplicate 64-byte enqueue off_82048EFC; defer to final enqueue batch. |
| 0x82574310 | sub_82574310 | Duplicate 64-byte enqueue off_82048F04. |
| 0x82574A28 | sub_82574A28 | Thin vtable+32 invoke only. |
| 0x82574A90 | sub_82574A90 | Duplicate 64-byte enqueue off_8204903C. |
| 0x82577BC8 | sub_82577BC8 | Thin wrapper calling render_deferred_queue_command_params_dtor. |
