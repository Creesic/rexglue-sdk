## Iteration 64

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8257C120 | sub_8257C120 | render_ai_path_vector_shift_records_forward | 0.90 | FM2_MemcpyAligned 224-byte records shifting forward in [begin,end); returns new end pointer. Used by insert_shift_forward. Vtable 0x82191E80. |
| 0x8257C180 | sub_8257C180 | render_ai_path_vector_shift_records_backward | 0.90 | FM2_MemcpyAligned 224-byte records shifting backward toward begin. Called from vector insert_records. Vtable 0x82191E88. |
| 0x8257C1E0 | sub_8257C1E0 | render_ai_path_vector_append_record_copies | 0.89 | Appends count copies of 224-byte source record at end; returns new end. Vtable 0x82191E90. |
| 0x8257C240 | sub_8257C240 | render_ai_path_vector_insert_shift_forward | 0.88 | Iterator insert: calls shift_records_forward when insert pos before end; updates out iterator. Vtable 0x82191E98. |
| 0x8257C300 | sub_8257C300 | render_ai_path_vector_insert_records | 0.89 | std::vector-style insert of N×224-byte records: in-place shift, append_copies, or FM2_AllocPoolAcquire224xCount reallocate + FM2_Stl_ThrowLengthError_VectorTooLong guard. Vtable 0x82191EA8. |
| 0x8257C618 | sub_8257C618 | render_ai_path_vector_insert_at_index | 0.88 | Insert at index a2: either insert_shift_forward at end or insert_records with copied element span. Vtable 0x82191EB0. |
| 0x8257C2B8 | sub_8257C2B8 | render_car_shader_pack_vector_free_storage | 0.87 | FM2_Memory_FreeSmallBlockOrNull at object+112; zeros vector begin/end/cap at +108..+116. Vtable 0x82191EA0; dtor path 0x82530F60. |
| 0x8257C748 | sub_8257C748 | render_car_shader_pack_init_state | 0.91 | FM2_D3D_InitGpuWaitTimerState; sets pack fields +22..+30 (counts/caps defaults). Vtable 0x82191EB8. |
| 0x8257C7A8 | sub_8257C7A8 | render_car_shader_pack_push_default_material_record | 0.88 | Zeroes 224-byte template tail; copies 176-byte header; insert_at_index into pack vector at +108. Vtable 0x82191EC0. |
| 0x8257C820 | sub_8257C820 | render_car_shader_pack_write_all_material_constants | 0.90 | Stream vtable +132 writes "count"; optional push_default_material_record; loops 224-byte records calling render_car_material_write_all_constants_to_xts_buf. |
| 0x8257C938 | sub_8257C938 | render_car_shader_pack_load_counts_and_write_constants | 0.91 | Stream reads material count + row width into dword_829A1460/dword_829A1464; FM2_Render_NotifyManagerStateChange; calls write_all_material_constants on vector at +108. |
| 0x827C3EE8 | sub_827C3EE8 | render_skinned_model_mesh_binding_init_from_resource | 0.89 | Stores resource ptr; FM2_CarDynamics_SetBasePointer; sub_827C4320; reads mesh ushort metadata (+58/+28/+30/+31); sub_827280B0; sub_827C3D60. Called from setup_from_asset_name. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x825AC180 | sub_825AC180 | Resource .data handle resolver; thin but needs sub_8272E188 name first. |
| 0x82728110 | sub_82728110 | Mesh buffer size formula only; thin pure calc. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only; accessor too trivial. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x82727ED0 | sub_82727ED0 | Stores dword at +108 only. |
| 0x827C3D18 | sub_827C3D18 | Animation phase wrap helper; wrap-mode branches need deeper read. |
| 0x827C4028 | sub_827C4028 | Animation callback dispatch; defer with 827C5938/82727F* subgraph. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
