
## Iteration 51

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82577AF0 | sub_82577AF0 | render_deferred_queue_command_params_init | 0.90 | vtable off_820495A8; FM2_Render_NotifyManagerStateChange at +4; stores float a3 at +8. Pair to render_deferred_queue_command_params_dtor. Caller sub_82578080. |
| 0x82577BF8 | sub_82577BF8 | render_presentation_invoke_vtable20_with_notify_and_float | 0.88 | Reads float from params+8; FM2_Render_NotifyManagerStateChange from params+4; dispatches object vtable+20. Caller sub_82578080 deferred queue path. |
| 0x82578080 | sub_82578080 | render_deferred_queue_enqueue_command_params | 0.89 | Pool alloc 12; ComPtr from child field+12; render_deferred_queue_command_params_init; ReleaseChild. Enqueues CDeferredQueue command params with notify + scalar. |
| 0x82574F90 | sub_82574F90 | render_deferred_task_payload_init_vmx128_block_64 | 0.89 | vtable off_8204906C; stores id at +4; FM2_MemcpyAligned 64-byte block at +16; stores VMX128 at +80; dword at +96. Called from render_deferred_task_release_with_vmx128_material_block_112. |
| 0x825750A8 | sub_825750A8 | render_deferred_task_release_with_vmx128_material_block_112 | 0.88 | Pool alloc 0x70; copies 16-byte tail args; forwards VMX128 register args to render_deferred_task_payload_init_vmx128_block_64; ReleaseChild. |
| 0x825743B8 | sub_825743B8 | render_deferred_task_release_with_byte_payload | 0.87 | Pool alloc 8; vtable off_82048F0C; byte at +4; ReleaseChild. First of byte-flag deferred release family. |
| 0x82575238 | render_deferred_task_release_with_float_payload_0 | (unchanged) | 0.88 | Already named before apply; pool alloc 8; vtable off_8204907C; float at +4; ReleaseChild. |
| 0x825753F0 | sub_825753F0 | render_deferred_task_release_with_vmx128_row_only | 0.88 | Pool alloc 0x20; vtable off_82049094; stores incoming VMX128 at +16 only; ReleaseChild. |
| 0x825754E8 | sub_825754E8 | render_deferred_task_enqueue_presentation_block_144 | 0.87 | Pool alloc 0xA0; vtable off_8204909C; FM2_MemcpyAligned 144-byte presentation block; ReleaseChild. |
| 0x82575708 | sub_82575708 | render_deferred_task_release_with_dual_float_and_vmx128 | 0.89 | Pool alloc 0x20; vtable off_820490B4; floats at +4/+8; VMX128 at +16; ReleaseChild. |
| 0x82574B80 | sub_82574B80 | render_deferred_task_enqueue_material_block_with_id | 0.88 | Pool alloc 0x50; vtable off_82049044; stores material id a2 at +4; FM2_MemcpyAligned 64-byte block at +16; ReleaseChild. |
| 0x82575B68 | sub_82575B68 | render_presentation_invoke_vtable156_with_vmx128_and_float | 0.89 | Loads four VMX128 rows from material+16..+64; calls object vtable+156 with float at material+80. Caller sub_82575BB8. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82574440 | sub_82574440 | Byte payload off_82048F14; duplicate structure of 825743B8. |
| 0x825744C8 | sub_825744C8 | Byte payload off_82048F1C; duplicate structure. |
| 0x82574550 | sub_82574550 | Dword payload off_82048F24; defer with F3C pair. |
| 0x825746D8 | sub_825746D8 | Dword payload off_82048F3C; duplicate structure. |
| 0x82574A28 | sub_82574A28 | Thin vtable+32 invoke only. |
| 0x82577BC8 | sub_82577BC8 | Thin wrapper calling render_deferred_queue_command_params_dtor. |
| 0x82574218 | sub_82574218 | Duplicate 64-byte deferred enqueue. |
