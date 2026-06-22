
## Iteration 60

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82570E78 | sub_82570E78 | render_post_process_acquire_com_object_vtbl_4867c | 0.89 | FM2_AllocPoolAcquireOrInit_Thunk 8; vtable off_8204867C; FM2_ComPtr_AssignRef at parent+28. Presentation provider vtable 0x82191658. |
| 0x82571160 | sub_82571160 | render_post_process_acquire_car_slot_object | 0.90 | FM2_AllocPoolAcquireOrInit_Thunk 12; vtable off_82048754 (same family as render_post_process_car_slot_object_dtor); ComPtr at parent+24. Vtable 0x82191690. |
| 0x82570FF8 | sub_82570FF8 | render_post_process_object_release_atomic | 0.89 | Atomic lwarx/stwcx refcount dec at a1+8; COM delete when zero. Vtable 0x82191670. |
| 0x825714C8 | sub_825714C8 | render_post_process_object_release_atomic_offset_280 | 0.88 | Atomic refcount dec at a1+280; COM delete when zero. Vtable 0x821916C0. |
| 0x825719C8 | sub_825719C8 | render_post_process_object_release_atomic_offset_32 | 0.88 | Atomic refcount dec at a1+32; COM delete when zero. Vtable 0x821916F8. |
| 0x825715F0 | sub_825715F0 | render_post_process_string_holder_dtor | 0.89 | FM2_Stl_String_InitOrClear at +4; FM2_Object_AssignBaseVtable_82000E18; optional free. Vtable xref off_820487EC / 0x821916D0. |
| 0x82572180 | sub_82572180 | render_post_process_flush_forza_cmdline_if_dirty | 0.88 | When byte at +84 set: FM2_Crt_StaticInit_ForzaCmdLineList_829F194C; vtable+16 flush call; clears flag. Vtable 0x82191730. |
| 0x825721F0 | sub_825721F0 | render_deferred_list_splice_to_global_and_release | 0.89 | FM2_IntrusiveList_SpliceNodes to parent+88 and global dword_82A00C68; Release on out ComPtr. Vtable 0x82191738. |
| 0x82572288 | sub_82572288 | render_deferred_list_splice_and_release | 0.89 | FM2_IntrusiveList_SpliceNodes to parent+80; Release on out ComPtr. Vtable 0x82191748. |
| 0x82579130 | sub_82579130 | render_sh_lighting_load_material_float_params | 0.92 | Loads SHLighting material group; reads floats via vtable+108 for strings SHTopRed/Green/Blue, SHBottom*, SHDir*, optional SHEnvMap*; vtable+148 finalize. Vtable 0x82191DA8. |
| 0x82579410 | sub_82579410 | render_notify_manager_state_change_by_vector_index | 0.90 | Bounds-checked index into vector at a2+8; FM2_Render_NotifyManagerStateChange on selected element. Vtable 0x82191DB0. |
| 0x82579470 | sub_82579470 | render_notify_manager_invoke_vtable32_on_matching_entry | 0.90 | Iterates ComPtr vector; on pointer match calls entry vtable+32 with notify out-param. Vtable 0x82191DB8. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
