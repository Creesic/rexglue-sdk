
## Iteration 59

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825785C0 | sub_825785C0 | render_deferred_queue_command_params_invoke_vtable20_deferred_command | 0.90 | FM2_DeferredCommand_CopyAssign from params+4; dispatches object vtable+20. Pair to render_deferred_queue_command_params_dtor_release_command_ref. Vtable 0x82191D58. |
| 0x82578618 | sub_82578618 | render_deferred_queue_enqueue_deferred_command_type_c | 0.90 | Pool alloc 0x58; FM2_DeferredCommand_CopyAssign + FM2_DeferredCommand_CtorTypeC; ReleaseChild; FM2_DeferredCommand_DtorReleaseRef on source. Vtable 0x82191D60. |
| 0x82578698 | sub_82578698 | render_config_entry_presentation_object_dtor | 0.89 | vtable off_820496AC; inner off_820495CC; FM2_ConfigEntryVector_FreeBuffer at +12; FM2_ComObject_DeleteOptionalBody. Vtable 0x82191D68. |
| 0x82578700 | sub_82578700 | render_config_entry_presentation_object_release | 0.90 | Thin wrapper: render_config_entry_presentation_object_dtor_with_free(a1-8). Vtable xref 0x820495CC. |
| 0x82578708 | sub_82578708 | render_config_entry_presentation_object_init | 0.90 | FM2_ComObject_InitBaseVtable423C0; IPresentation vftable; off_820496AC/off_820495CC; stores context a2 at +32. Vtable 0x82191D70. |
| 0x82578780 | sub_82578780 | render_config_entry_presentation_object_dtor_with_free | 0.90 | Calls render_config_entry_presentation_object_dtor; optional FM2_Memory_FreeSmallBlockOrNull. Vtable 0x82191D78. |
| 0x825787D0 | sub_825787D0 | render_deferred_queue_command_params_init_b3e8_render_state | 0.89 | vtable off_820496B4; sub_821EFF80 copy into params+16; FM2_Render_HelperB3E8ResetState on source. Called from enqueue 0x82578880. |
| 0x82578828 | sub_82578828 | render_deferred_queue_command_params_invoke_vtable20_b3e8_render_state | 0.89 | sub_821EFF80 copy from params+16 (1800-byte stack temp); dispatches vtable+20. Vtable 0x82191D88. |
| 0x82578880 | sub_82578880 | render_deferred_queue_enqueue_command_params_b3e8_render_state | 0.90 | Pool alloc 0x710; sub_821EFF80 + render_deferred_queue_command_params_init_b3e8_render_state; ReleaseChild; FM2_Render_HelperB3E8ResetState. Vtable 0x82191D90. |
| 0x82578900 | sub_82578900 | render_deferred_queue_command_params_dtor_b3e8_render_state | 0.90 | vtable off_820496B4; FM2_Render_HelperB3E8ResetState at +16; assigns CCommandParams vftable. Vtable 0x82191D98. |
| 0x82571118 | sub_82571118 | render_post_process_car_slot_transform_factory | 0.91 | FM2_AllocPoolAcquireOrInit_Thunk 40; render_post_process_car_slot_transform_init; stores parent a1 at result+4. Vtable 0x82191688. |
| 0x825710C0 | sub_825710C0 | render_post_process_car_slot_object_dtor | 0.90 | vtable off_82048754; FM2_Object_AssignBaseVtable_82000E18; optional free. Vtable xref 0x82048754. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x82578970 | FM2_AudioManager_InitDefaultMixParameters | Already meaningfully named. |
| 0x82570E78 | sub_82570E78 | Deferred to iter 60 (ComPtr acquire off_8204867C). |
| 0x825721F0 | sub_825721F0 | Deferred to iter 60 (intrusive list splice + global). |
