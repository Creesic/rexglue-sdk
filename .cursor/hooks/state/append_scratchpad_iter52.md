
## Iteration 52

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82575910 | sub_82575910 | render_deferred_task_payload_init_vmx128_triple_float | 0.90 | vtable off_820490CC; stores VMX128 at +16; material id at +32; FM2_MemcpyAligned 64-byte block at +48; three floats at +112..+120; dword at +124. Called from render_deferred_task_enqueue_vmx128_triple_float_payload_128. |
| 0x82575A58 | sub_82575A58 | render_deferred_task_enqueue_vmx128_triple_float_payload_128 | 0.89 | Pool alloc 0x80; copies 16-byte aux block; forwards VMX128 + triple float args to render_deferred_task_payload_init_vmx128_triple_float; ReleaseChild. Presentation provider vtable 0x82191A78. |
| 0x825759B8 | sub_825759B8 | render_presentation_invoke_vtable144_with_triple_float_vmx | 0.89 | Loads VMX128 from payload+16; copies 16-byte tail; reads three floats from payload+112..+120; dispatches object vtable+144 with material id and 64-byte block fields. |
| 0x82575BB8 | sub_82575BB8 | render_deferred_task_release_with_vmx128_quad_and_float | 0.90 | Pool alloc 0x60; vtable off_820490D4; stores four VMX128 rows at +16..+64; float at +80; ReleaseChild. Calls render_presentation_invoke_vtable156_with_vmx128_and_float on execute. |
| 0x82574440 | sub_82574440 | render_deferred_task_release_with_byte_payload_vtbl_f14 | 0.87 | Pool alloc 8; vtable off_82048F14; byte at +4; ReleaseChild. Presentation provider vtable 0x82191940. |
| 0x825744C8 | sub_825744C8 | render_deferred_task_release_with_byte_payload_vtbl_f1c | 0.87 | Pool alloc 8; vtable off_82048F1C; byte at +4; ReleaseChild. Presentation provider vtable 0x82191948. |
| 0x82574550 | sub_82574550 | render_deferred_task_release_with_dword_payload_vtbl_f24 | 0.87 | Pool alloc 8; vtable off_82048F24; dword at +4; ReleaseChild. Presentation provider vtable 0x82191950. |
| 0x825746D8 | sub_825746D8 | render_deferred_task_release_with_dword_payload_vtbl_f3c | 0.87 | Pool alloc 8; vtable off_82048F3C; dword at +4; ReleaseChild. Presentation provider vtable 0x82191968. |
| 0x82574748 | sub_82574748 | render_deferred_task_release_with_vtable_only_f44 | 0.86 | Pool alloc 4; vtable off_82048F44 only (no payload fields); ReleaseChild. Presentation provider vtable 0x82191970. |
| 0x825747C8 | sub_825747C8 | render_deferred_task_release_with_vtable_only_f4c | 0.86 | Pool alloc 4; vtable off_82048F4C only; ReleaseChild. Presentation provider vtable 0x82191978. |
| 0x82574E60 | sub_82574E60 | render_deferred_task_enqueue_material_block_with_id_vtbl_4905c | 0.88 | Pool alloc 0x50; vtable off_8204905C; stores material id at +4; FM2_MemcpyAligned 64-byte block at +16; ReleaseChild. Parallel to render_deferred_task_enqueue_material_block_with_id (off_82049044). |
| 0x82577F90 | sub_82577F90 | render_presentation_provider_invoke_vtable76_with_notify_state | 0.90 | RtlEnterCriticalSection on provider+24; FM2_Render_NotifyManagerStateChange; dispatches held object vtable+76; optional Release on out ComPtr. Presentation provider vtable 0x82191CE0. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82574218 | sub_82574218 | Duplicate 64-byte deferred enqueue (off_82048F04 family). |
| 0x82574310 | sub_82574310 | 64-byte block enqueue off_82048F04; duplicate structure. |
| 0x82574A28 | sub_82574A28 | Thin vtable+32 invoke only. |
| 0x82574A90 | sub_82574A90 | 64-byte block enqueue off_8204903C; duplicate structure. |
| 0x82575488 | sub_82575488 | Deferred to next batch (vtable+100 88-byte tail invoke). |
| 0x82577BC8 | sub_82577BC8 | Thin wrapper calling render_deferred_queue_command_params_dtor. |
