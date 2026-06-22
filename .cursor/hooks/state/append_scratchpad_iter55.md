
## Iteration 55

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825752E0 | sub_825752E0 | render_deferred_task_release_with_byte_payload_vtbl_49084 | 0.87 | Pool alloc 8; vtable off_82049084; byte at +4; ReleaseChild. Presentation provider vtable 0x82191A18. |
| 0x82575368 | sub_82575368 | render_deferred_task_release_with_byte_payload_vtbl_4908C | 0.87 | Pool alloc 8; vtable off_8204908C; byte at +4; ReleaseChild. Presentation provider vtable 0x82191A20. |
| 0x82575658 | sub_82575658 | render_deferred_task_release_with_float_payload_vtbl_490AC | 0.87 | Pool alloc 8; vtable off_820490AC; float at +4; ReleaseChild. Presentation provider vtable 0x82191A48. |
| 0x825757D8 | sub_825757D8 | render_deferred_task_release_with_float_payload_vtbl_490BC | 0.87 | Pool alloc 8; vtable off_820490BC; float at +4; ReleaseChild. Presentation provider vtable 0x82191A58. |
| 0x82575880 | sub_82575880 | render_deferred_task_release_with_float_payload_vtbl_490C4 | 0.87 | Pool alloc 8; vtable off_820490C4; float at +4; ReleaseChild. Presentation provider vtable 0x82191A60. |
| 0x82575CA0 | sub_82575CA0 | render_deferred_task_release_with_float_payload_vtbl_490DC | 0.87 | Pool alloc 8; vtable off_820490DC; float at +4; ReleaseChild. Presentation provider vtable 0x82191A90. |
| 0x82575D48 | sub_82575D48 | render_deferred_task_release_with_float_payload_vtbl_490E4 | 0.87 | Pool alloc 8; vtable off_820490E4; float at +4; ReleaseChild. Presentation provider vtable 0x82191A98. |
| 0x82575DF0 | sub_82575DF0 | render_deferred_task_release_with_byte_payload_vtbl_490EC | 0.87 | Pool alloc 8; vtable off_820490EC; byte at +4; ReleaseChild. Presentation provider vtable 0x82191AA0. |
| 0x82575E78 | sub_82575E78 | render_deferred_task_release_with_byte_payload_vtbl_490F4 | 0.87 | Pool alloc 8; vtable off_820490F4; byte at +4; ReleaseChild. Presentation provider vtable 0x82191AA8. |
| 0x82575F00 | sub_82575F00 | render_deferred_task_release_with_float_payload_vtbl_490FC | 0.87 | Pool alloc 8; vtable off_820490FC; float at +4; ReleaseChild. Presentation provider vtable 0x82191AB0. |
| 0x82576388 | sub_82576388 | render_deferred_task_release_with_dword_payload_vtbl_491F4 | 0.87 | Pool alloc 8; vtable off_820491F4; dword at +4; ReleaseChild. Presentation provider vtable 0x82191AF0. |
| 0x82577030 | sub_82577030 | render_deferred_task_enqueue_presentation_block_144_vtbl_492D8 | 0.88 | Pool alloc 0xA0; vtable off_820492D8; FM2_MemcpyAligned 144-byte block at +16; ReleaseChild. Parallel to render_deferred_task_enqueue_presentation_block_144 (off_8204909C). |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82574218 | sub_82574218 | Duplicate 64-byte enqueue off_82048EFC. |
| 0x82574310 | sub_82574310 | Duplicate 64-byte enqueue off_82048F04. |
| 0x82574A28 | sub_82574A28 | Thin vtable+32 invoke only. |
| 0x82574A90 | sub_82574A90 | Duplicate 64-byte enqueue off_8204903C. |
| 0x82576050 | sub_82576050 | Vtable-only marker off_8204910C; defer with 491xx batch. |
| 0x82577BC8 | sub_82577BC8 | Thin wrapper calling render_deferred_queue_command_params_dtor. |
