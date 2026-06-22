
## Iteration 48

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825748C8 | sub_825748C8 | render_aux_presentation_comobject_init | 0.88 | FM2_ComObject_InitBaseVtable423C0; vtable off_82049024; inner off_82048F5C. Base for aux slot init at 825775A8. Provider factory uses vtable+36. |
| 0x825775A8 | sub_825775A8 | render_aux_presentation_object_init | 0.89 | render_aux_presentation_comobject_init; derived off_82049474/off_820493AC; clears +16. Vtable entry at 0x82191C10; dtor at 82577600. |
| 0x82577600 | sub_82577600 | render_aux_presentation_object_dtor | 0.90 | Resets off_82049474/off_820493AC; FM2_ComObject_DeleteOptionalBody. Vtable entry at 0x82191C18. Pair to render_aux_presentation_object_init. |
| 0x825779B0 | sub_825779B0 | render_aux_presentation_object_dtor_with_free | 0.91 | render_aux_presentation_object_dtor + optional FM2_Memory_FreeSmallBlockOrNull. |
| 0x825774B0 | sub_825774B0 | render_imodel_presentation_object_dtor | 0.90 | Resets off_82049340/off_82049320; FM2_ComObject_DeleteOptionalBody. Vtable entry at 0x82191BF8; pair to render_imodel_presentation_object_init. |
| 0x82577558 | sub_82577558 | render_track_presentation_object_dtor | 0.90 | Resets off_820493A4/off_8204934C; FM2_ComObject_DeleteOptionalBody. Vtable entry at 0x82191C08; pair to render_track_presentation_object_init. |
| 0x82577C60 | sub_82577C60 | render_presentation_provider_create_imodel_slot | 0.89 | Critsec; pool alloc; render_imodel_presentation_object_init; provider COM vtable+24 id; FM2_Render_GetFrameCounterField; returns object+2. Vtable ptr 0x82191CB0. |
| 0x82577CE8 | sub_82577CE8 | render_presentation_provider_create_track_slot | 0.89 | Same pattern with render_track_presentation_object_init and provider vtable+28. Vtable ptr 0x82191CB8. |
| 0x82577D70 | sub_82577D70 | render_presentation_provider_create_aux_slot | 0.89 | Same pattern with render_aux_presentation_object_init and provider vtable+36. Vtable ptr 0x82191CC0. |
| 0x82577310 | sub_82577310 | render_model_presentation_provider_acquire_with_transform | 0.88 | Critsec; render_model_presentation_object_init; copies 204-byte arg block; provider vtable+56 with 3x uint64 + a8; stores returned id at +12. Vtable entry 0x82191BE0. |
| 0x82574CE0 | sub_82574CE0 | render_presentation_invoke_vtable64_with_vmx_args | 0.89 | Loads three VMX128 rows from material slot; calls object vtable+64 with slot[1], slot[2], slot[16]. Caller render_deferred_task_release_with_vmx128_transform_and_ids. |
| 0x82574E00 | sub_82574E00 | render_presentation_invoke_vtable68_from_material_slot | 0.88 | FM2_MemcpyAligned 16 bytes from material+64; dispatches vtable+68 with eight args from slot+16. Pair to render_presentation_invoke_vtable40_from_material_slot (+40). Caller sub_82574E60. |

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
| 0x82574218 | sub_82574218 | Duplicate 64-byte deferred enqueue; only vtable off_82048EFC differs from 82574028. |
| 0x82574310 | sub_82574310 | Duplicate 64-byte deferred enqueue; vtable off_82048F04 only. |
| 0x82573DC0 | sub_82573DC0 | 4-byte stub vtable off_82048E28 only. |
| 0x82573E40 | sub_82573E40 | 4-byte stub vtable off_82048E30 only. |
| 0x82574A28 | sub_82574A28 | Thin vtable+32 invoke only. |
| 0x82577400 | sub_82577400 | Thin critsec wrapper calling provider vtable+88. |
