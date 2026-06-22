
## Iteration 53

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82575488 | sub_82575488 | render_presentation_invoke_vtable100_with_88byte_tail | 0.89 | Reads dword fields from payload+16; FM2_MemcpyAligned 88 bytes from payload+72; dispatches object vtable+100 with seven dword args. Caller 0x82575564 enqueue path. |
| 0x82574F10 | sub_82574F10 | render_deferred_task_release_with_id_and_float | 0.88 | Pool alloc 0xC; vtable off_82049064; dword id at +4; float at +8; ReleaseChild. Presentation provider vtable 0x821919E8. |
| 0x825751A0 | sub_825751A0 | render_deferred_task_release_with_id_and_float_vtbl_49074 | 0.88 | Pool alloc 0xC; vtable off_82049074; dword id at +4; float at +8; ReleaseChild. Parallel to render_deferred_task_release_with_id_and_float. |
| 0x82575FB0 | sub_82575FB0 | render_deferred_task_release_with_id_and_dual_float | 0.89 | Pool alloc 0x10; vtable off_82049104; dword id at +4; two floats at +8/+12; ReleaseChild. Presentation provider vtable 0x82191AB8. |
| 0x82574848 | sub_82574848 | render_deferred_task_release_with_vtable_only_f54 | 0.86 | Pool alloc 4; vtable off_82048F54 only; ReleaseChild. Presentation provider vtable 0x82191980. |
| 0x825749B8 | sub_825749B8 | render_deferred_task_release_with_byte_payload_vtbl_49034 | 0.87 | Pool alloc 8; vtable off_82049034; byte at +4; ReleaseChild. Presentation provider vtable 0x82191998. |
| 0x82578008 | sub_82578008 | render_presentation_provider_invoke_vtable80_with_notify_state | 0.90 | RtlEnterCriticalSection on provider+24; FM2_Render_NotifyManagerStateChange; dispatches held object vtable+80; optional Release on out ComPtr. Pair to render_presentation_provider_invoke_vtable76_with_notify_state at 0x82191CE0/+0x82191CE8. |
| 0x825774A8 | sub_825774A8 | render_imodel_presentation_object_release | 0.91 | Thin wrapper: render_imodel_presentation_object_dtor_with_free(a1-8). Vtable xref 0x82049320. |
| 0x82577550 | sub_82577550 | render_track_presentation_object_release | 0.91 | Thin wrapper: render_track_presentation_object_dtor_with_free(a1-8). Vtable xref 0x8204934C. |
| 0x825775F8 | sub_825775F8 | render_aux_presentation_object_release | 0.91 | Thin wrapper: render_aux_presentation_object_dtor_with_free(a1-8). Vtable xref 0x820493AC. |
| 0x825776A0 | sub_825776A0 | render_presentation_slot40_object_release | 0.91 | Thin wrapper: render_presentation_slot40_object_dtor_with_free(a1-8). Vtable xref 0x8204947C. |
| 0x825777B8 | sub_825777B8 | render_presentation_slot44_object_release | 0.91 | Thin wrapper: render_presentation_slot44_object_dtor_with_free(a1-8). Vtable xref 0x8204953C. |

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
| 0x82577748 | sub_82577748 | Atomic refcount release; defer to pair with 82577860. |
| 0x82577860 | sub_82577860 | Simple refcount dec release; defer to pair naming. |
| 0x82577BC8 | sub_82577BC8 | Thin wrapper calling render_deferred_queue_command_params_dtor. |
