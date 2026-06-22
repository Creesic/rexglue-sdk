
## Iteration 54

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825778B8 | sub_825778B8 | render_presentation_slot52_object_release | 0.91 | Thin wrapper: render_presentation_slot52_object_dtor_with_free(a1-8). Vtable xref 0x82049580. Completes slot52 release pair with init/dtor. |
| 0x82577748 | sub_82577748 | render_presentation_slot44_object_release_atomic | 0.90 | Atomic lwarx/stwcx refcount dec at a1+8; COM delete at a1-8 when zero. Presentation provider vtable 0x82191C38 (slot44 cluster). |
| 0x82577860 | sub_82577860 | render_presentation_slot52_object_release_refcount | 0.90 | Non-atomic refcount dec at a1+8; COM delete at a1-8 when count hits 1. Presentation provider vtable 0x82191C50 (slot52 cluster). |
| 0x82576FD0 | sub_82576FD0 | render_presentation_invoke_vtable40_with_88byte_tail | 0.89 | Copies 88 bytes from payload+72; reads dword fields from payload+16; dispatches object vtable+40. Parallel to render_presentation_invoke_vtable100_with_88byte_tail. |
| 0x825769F0 | sub_825769F0 | render_deferred_task_invoke_vtable96 | 0.87 | Thin invoke: (*(a1->vtable+96))(). Caller 0x82576AD4 deferred execute path. Presentation provider vtable 0x82191B50. |
| 0x82576AE8 | sub_82576AE8 | render_deferred_task_invoke_vtable100_no_args | 0.87 | Thin invoke: (*(a1->vtable+100))() with no extra args. Caller 0x82576BCC. Presentation provider vtable 0x82191B60. |
| 0x82576750 | sub_82576750 | render_deferred_task_release_with_quad_float_payload | 0.89 | Pool alloc 0x14; vtable off_8204922C; stores four floats at +4..+16; ReleaseChild. Presentation provider vtable 0x82191B28. |
| 0x82576BF8 | sub_82576BF8 | render_deferred_task_release_with_dual_float_payload | 0.88 | Pool alloc 0xC; vtable off_82049264; two floats at +4/+8; ReleaseChild. Presentation provider vtable 0x82191B70. |
| 0x825770D8 | sub_825770D8 | render_deferred_task_release_with_dual_dword_payload | 0.88 | Pool alloc 0xC; vtable off_820492E0; two dwords at +4/+8; ReleaseChild. Presentation provider vtable 0x82191BB8. |
| 0x82576ED0 | sub_82576ED0 | render_deferred_task_release_with_vmx128_row_vtbl_4928c | 0.88 | Pool alloc 0x20; vtable off_8204928C; stores incoming VMX128 at +16; ReleaseChild. Presentation provider vtable 0x82191B98. |
| 0x82576A58 | sub_82576A58 | render_deferred_task_enqueue_presentation_block_64_vtbl_49254 | 0.87 | Pool alloc 0x50; vtable off_82049254; FM2_MemcpyAligned 64-byte block at +16; ReleaseChild. Presentation provider vtable 0x82191B58. |
| 0x825755B0 | sub_825755B0 | render_deferred_task_release_with_float_payload_vtbl_490a4 | 0.87 | Pool alloc 8; vtable off_820490A4; float at +4; ReleaseChild. Presentation provider vtable 0x82191A40. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82574218 | sub_82574218 | Duplicate 64-byte deferred enqueue. |
| 0x82574310 | sub_82574310 | Duplicate 64-byte deferred enqueue off_82048F04. |
| 0x82574A28 | sub_82574A28 | Thin vtable+32 invoke only. |
| 0x82574A90 | sub_82574A90 | Duplicate 64-byte deferred enqueue off_8204903C. |
| 0x825752E0 | sub_825752E0 | Byte payload off_82049084; defer with float-stub batch. |
| 0x82577030 | sub_82577030 | Duplicate 144-byte enqueue off_820492D8. |
| 0x82577BC8 | sub_82577BC8 | Thin wrapper calling render_deferred_queue_command_params_dtor. |
