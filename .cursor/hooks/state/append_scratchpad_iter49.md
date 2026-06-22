
## Iteration 49

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825762D0 | sub_825762D0 | render_presentation_slot40_comobject_init | 0.88 | FM2_ComObject_InitBaseVtable423C0; vtable off_820491EC; inner off_82049134. Base for slot40 object init at 82577650. |
| 0x82577650 | sub_82577650 | render_presentation_slot40_object_init | 0.89 | render_presentation_slot40_comobject_init; derived off_82049534/off_8204947C; clears +16. Vtable entry 0x82191C20; dtor 825776A8. |
| 0x825776A8 | sub_825776A8 | render_presentation_slot40_object_dtor | 0.90 | Resets off_82049534/off_8204947C; FM2_ComObject_DeleteOptionalBody. Vtable entry 0x82191C28. |
| 0x82577A00 | sub_82577A00 | render_presentation_slot40_object_dtor_with_free | 0.91 | render_presentation_slot40_object_dtor + optional FM2_Memory_FreeSmallBlockOrNull. Vtable entry 0x82191C78. |
| 0x82577DF8 | sub_82577DF8 | render_presentation_provider_create_slot40 | 0.89 | Critsec; pool alloc; render_presentation_slot40_object_init; provider COM vtable+40 id; FM2_Render_GetFrameCounterField. Vtable ptr 0x82191CC8. |
| 0x82576F68 | sub_82576F68 | render_presentation_slot44_comobject_init | 0.88 | FM2_ComObject_InitBaseVtable423C0; vtable off_820492D0; inner off_82049294. Base for slot44 object init at 825776F8. |
| 0x825776F8 | sub_825776F8 | render_presentation_slot44_object_init | 0.89 | render_presentation_slot44_comobject_init; derived off_82049578/off_8204953C; clears +16. Vtable entry 0x82191C30; dtor 825777C0. |
| 0x825777C0 | sub_825777C0 | render_presentation_slot44_object_dtor | 0.90 | Resets off_82049578/off_8204953C; FM2_ComObject_DeleteOptionalBody. Vtable entry 0x82191C40 area. |
| 0x82577A50 | sub_82577A50 | render_presentation_slot44_object_dtor_with_free | 0.91 | render_presentation_slot44_object_dtor + optional free. Vtable entry 0x82191C80. |
| 0x82577E80 | sub_82577E80 | render_presentation_provider_create_slot44 | 0.89 | Critsec; pool alloc; render_presentation_slot44_object_init; provider COM vtable+44 id; FM2_Render_GetFrameCounterField. Vtable ptr 0x82191CD0. |
| 0x82577400 | sub_82577400 | render_presentation_provider_invoke_vtable88_under_critsec | 0.87 | Enters critsec at a2+24; calls provider COM at a2+20 vtable+88 with a1. Vtable entry 0x82191BE8 in presentation provider table. |
| 0x82574C40 | sub_82574C40 | render_deferred_task_release_with_vmx128_scalar_and_flag | 0.88 | Pool alloc 0x30; vtable off_8204904C; stores a2, float a3, byte a5, one VMX128 row at +16; ReleaseChild. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82570B68 | sub_82570B68 | Atomic refcount dec at +192; no xrefs. |
| 0x825714C8 | sub_825714C8 | Atomic refcount dec at +1120; no callers. |
| 0x825719C8 | sub_825719C8 | Atomic refcount dec at +128; no callers. |
| 0x82570C30 | sub_82570C30 | Thin FM2_MemcpyAligned 24-byte copy thunk. |
| 0x82574218 | sub_82574218 | Duplicate 64-byte deferred enqueue; vtable differs only. |
| 0x825743B8 | sub_825743B8 | Byte payload deferred release; defer batch naming with F14/F1C variants. |
| 0x82573DC0 | sub_82573DC0 | 4-byte stub vtable off_82048E28 only. |
| 0x82577748 | sub_82577748 | Atomic refcount dec helper at +8 only. |
| 0x82574A28 | sub_82574A28 | Thin vtable+32 invoke only. |
