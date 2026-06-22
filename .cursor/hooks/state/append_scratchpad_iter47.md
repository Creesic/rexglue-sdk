
## Iteration 47

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82573F18 | sub_82573F18 | render_model_presentation_object_dtor | 0.90 | vtable off_82048E5C / inner off_82048E38 reset; FM2_ComObject_DeleteOptionalBody. Pair to render_model_presentation_object_init. Caller sub_825772C0 deleting dtor. |
| 0x825772C0 | sub_825772C0 | render_model_presentation_object_dtor_with_free | 0.91 | render_model_presentation_object_dtor + optional FM2_Memory_FreeSmallBlockOrNull. |
| 0x82573F68 | sub_82573F68 | render_imodel_presentation_comobject_init | 0.89 | FM2_ComObject_InitBaseVtable423C0; vtable off_82048E84; inner IModelPresentation/off_82048E64. Caller sub_82577458. |
| 0x82577458 | sub_82577458 | render_imodel_presentation_object_init | 0.90 | render_imodel_presentation_comobject_init; derived vtable off_82049340/off_82049320; clears +16. Created under critsec by sub_82577C60 via provider vtable+24. |
| 0x825740B8 | sub_825740B8 | render_track_presentation_comobject_init | 0.88 | FM2_ComObject_InitBaseVtable423C0; vtable off_82048EEC; inner off_82048E94. Caller sub_82577500. |
| 0x82577500 | sub_82577500 | render_track_presentation_object_init | 0.89 | render_track_presentation_comobject_init; derived off_820493A4/off_8204934C; clears +16. sub_82577CE8 acquire via provider vtable+28. |
| 0x82573D50 | sub_82573D50 | render_com_object_optional_body_dtor_with_free | 0.90 | Resets inner vtable at a1+2; FM2_ComObject_DeleteOptionalBody; optional heap free. Used via this-adjust thunk sub_82573DB8. |
| 0x82573CC0 | sub_82573CC0 | render_deferred_task_release_with_float_payload | 0.88 | FM2_DeferredTaskParams_GetField4; pool alloc 8 bytes; stores float a2 at +4; vtable off_82048E20; FM2_DeferredTaskParams_ReleaseChild. |
| 0x82574028 | sub_82574028 | render_deferred_task_enqueue_presentation_block_64 | 0.87 | Pool alloc 0x50; vtable off_82048E8C; FM2_MemcpyAligned 64-byte block from register args; ReleaseChild. Deferred presentation payload enqueue. |
| 0x82574130 | sub_82574130 | render_deferred_task_enqueue_presentation_block_24 | 0.87 | Pool alloc 0x1C; vtable off_82048EF4; FM2_MemcpyAligned 24-byte block; ReleaseChild. |
| 0x82574D30 | sub_82574D30 | render_deferred_task_release_with_vmx128_transform_and_ids | 0.89 | Pool alloc 0x50; vtable off_82049054; stores a2/a3/a4 and three VMX128 rows; ReleaseChild. Transform + resource id deferred release. |
| 0x82574B20 | sub_82574B20 | render_presentation_invoke_vtable40_from_material_slot | 0.88 | FM2_MemcpyAligned 16 bytes from material+64; dispatches a1 vtable+40 with eight args read from material slot +16. Caller sub_82574B80. |

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
| 0x82573BE0 | sub_82573BE0 | vtable+84 under critsec; no callers. |
| 0x82573C98 | sub_82573C98 | Thin vtable+20 dispatch only. |
| 0x82573DB8 | sub_82573DB8 | 8-byte this-adjust thunk to sub_82573D50. |
| 0x82573DC0 | sub_82573DC0 | Stub vtable off_82048E28 only; defer batch with similar stubs. |
| 0x82573E40 | sub_82573E40 | Stub vtable off_82048E30 only. |
| 0x82573FC0 | sub_82573FC0 | Thin vtable+24 invoke only. |
| 0x82574218 | sub_82574218 | Same 64-byte enqueue pattern as 82574028 with different vtable; defer. |
| 0x825748C8 | sub_825748C8 | Third presentation slot (provider vtable+36); defer with 825775A8 pair. |
