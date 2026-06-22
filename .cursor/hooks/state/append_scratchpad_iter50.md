
## Iteration 50

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825771D8 | sub_825771D8 | render_presentation_slot52_comobject_init | 0.88 | FM2_ComObject_InitBaseVtable423C0; vtable off_82049310; inner off_820492F0. Base for slot52 object init at 82577810. |
| 0x82577810 | sub_82577810 | render_presentation_slot52_object_init | 0.89 | render_presentation_slot52_comobject_init; derived off_820495A0/off_82049580; clears +16. Vtable entry 0x82191C48; dtor 825778C0. |
| 0x825778C0 | sub_825778C0 | render_presentation_slot52_object_dtor | 0.90 | Resets off_820495A0/off_82049580; FM2_ComObject_DeleteOptionalBody. Vtable entry 0x82191C58. |
| 0x82577AA0 | sub_82577AA0 | render_presentation_slot52_object_dtor_with_free | 0.91 | render_presentation_slot52_object_dtor + optional FM2_Memory_FreeSmallBlockOrNull. Vtable entry 0x82191C88. |
| 0x82577F08 | sub_82577F08 | render_presentation_provider_create_slot52 | 0.89 | Critsec; pool alloc; render_presentation_slot52_object_init; provider COM vtable+52 id; FM2_Render_GetFrameCounterField. Vtable ptr 0x82191CD8. |
| 0x82577910 | sub_82577910 | render_imodel_presentation_object_dtor_with_free | 0.91 | render_imodel_presentation_object_dtor + optional free. Vtable deleting dtor entry 0x82191C60. |
| 0x82577960 | sub_82577960 | render_track_presentation_object_dtor_with_free | 0.91 | render_track_presentation_object_dtor + optional free. Vtable deleting dtor entry 0x82191C68. |
| 0x825745D8 | sub_825745D8 | render_deferred_task_release_with_two_byte_payload | 0.88 | Pool alloc 8; vtable off_82048F2C; stores bytes at +4/+5; ReleaseChild. |
| 0x82574650 | sub_82574650 | render_deferred_task_release_with_dword_payload | 0.88 | Pool alloc 8; vtable off_82048F34; stores dword a2 at +4; ReleaseChild. |
| 0x82574938 | sub_82574938 | render_deferred_task_release_with_float_and_byte_flag | 0.89 | Pool alloc 12; vtable off_8204902C; float at +4, byte flag at +8; ReleaseChild. |
| 0x82575020 | sub_82575020 | render_presentation_invoke_vtable76_from_material_slot | 0.88 | Copies 16 bytes from material+64; loads VMX128 from material+80; dispatches vtable+76 with eight slot args. Caller sub_825750A8. Completes +40/+64/+68 dispatch set. |
| 0x82577B68 | sub_82577B68 | render_deferred_queue_command_params_dtor | 0.90 | vtable off_820495A8; releases notify COM at +4; resets to CDeferredQueue::CCommandParams vftable. Pair to deferred command param init path. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82577860 | sub_82577860 | Refcount dec at +8 only. |
| 0x82574218 | sub_82574218 | Duplicate 64-byte deferred enqueue. |
| 0x825743B8 | sub_825743B8 | Byte payload F0C; defer batch with F14/F1C. |
| 0x82574A28 | sub_82574A28 | Thin vtable+32 invoke only. |
| 0x82577BC8 | sub_82577BC8 | Thin wrapper calling render_deferred_queue_command_params_dtor. |
| 0x82577AF0 | sub_82577AF0 | Deferred queue params init; defer with 82577BF8 pair next pass. |
