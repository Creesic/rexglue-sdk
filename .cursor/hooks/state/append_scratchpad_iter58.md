
## Iteration 58

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82574A28 | sub_82574A28 | render_deferred_task_invoke_vtable32_no_args | 0.88 | Thin invoke: (*(a1->vtable+32))(). Caller 0x82574B0C execute path. Presentation provider vtable 0x821919A0. Completes 0x82574200–0x82578100 cluster. |
| 0x82578138 | sub_82578138 | render_deferred_queue_command_params_init_string_and_floats | 0.90 | vtable off_820495B0; FM2_Stl_String_CopyAssign at +4; dword a3 at +32; floats at +36/+40; clears source string. Caller enqueue 0x82578290. |
| 0x825781A8 | sub_825781A8 | render_deferred_queue_command_params_default_ctor | 0.89 | Sets off_820495B0; FM2_Stl_String_InitOrClear at +4; assigns CDeferredQueue::CCommandParams vftable. Vtable 0x82191D00. |
| 0x825781F8 | sub_825781F8 | render_deferred_queue_command_params_default_ctor_thunk | 0.89 | Thin wrapper calling render_deferred_queue_command_params_default_ctor; returns a1. Vtable xref 0x820495B0. |
| 0x82578228 | sub_82578228 | render_deferred_queue_command_params_invoke_vtable20_with_string | 0.89 | Copies string from params+4; reads dword at +32 and floats at +36/+40; dispatches object vtable+20. Vtable 0x82191D10. |
| 0x82578290 | sub_82578290 | render_deferred_queue_enqueue_command_params_string_and_floats | 0.90 | Pool alloc 0x2C; calls render_deferred_queue_command_params_init_string_and_floats; ReleaseChild; clears source string. Vtable 0x82191D18. |
| 0x82578340 | sub_82578340 | render_deferred_queue_command_params_init_string_and_vmx128 | 0.90 | vtable off_820495B8; string at +4; lvx128/stvx copy at +32; clears source string. Called from enqueue 0x82578480. |
| 0x82578398 | sub_82578398 | render_deferred_queue_command_params_string_vmx128_ctor | 0.89 | Sets off_820495B8; string init at +4; assigns CCommandParams vftable. Vtable 0x82191D28. |
| 0x825783E8 | sub_825783E8 | render_deferred_queue_command_params_string_vmx128_ctor_thunk | 0.89 | Thin wrapper calling render_deferred_queue_command_params_string_vmx128_ctor. Vtable xref 0x820495B8. |
| 0x82578418 | sub_82578418 | render_deferred_queue_command_params_invoke_vtable20_string_vmx128 | 0.89 | Copies string from params+16; loads VMX128 from params+48; dispatches vtable+20. Vtable 0x82191D38. |
| 0x82578480 | sub_82578480 | render_deferred_queue_enqueue_command_params_string_vmx128 | 0.90 | Pool alloc 0x40; builds temp string+vmx128; calls render_deferred_queue_command_params_init_string_and_vmx128; ReleaseChild. Vtable 0x82191D40. |
| 0x82578570 | sub_82578570 | render_deferred_queue_command_params_dtor_release_command_ref | 0.90 | vtable off_820495C0; FM2_DeferredCommand_DtorReleaseRef at +4; assigns CCommandParams vftable. Vtable 0x82191D50. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x8255B7C4 | sub_8255B7C4 | Decompiler shows broken register use; table lookup only. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x825785C0 | sub_825785C0 | Deferred to iter 59 (deferred-command ref invoke vtable+20). |
| 0x82578618 | sub_82578618 | Deferred to iter 59 (enqueue deferred command type C). |
