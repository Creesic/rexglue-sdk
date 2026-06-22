
## Iteration 38

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8255C298 | sub_8255C298 | tmodel_binary_stream_read_scope_dtor | 0.90 | Sets vtable off_8204744C; FM2_BinaryStream_DtorReadScope; optional FM2_Memory_FreeSmallBlockOrNull. TModel serialize read-scope teardown. |
| 0x8255C2F0 | sub_8255C2F0 | serialize_tmodel_vbparams_subresource_create | 0.91 | FM2_AllocPoolAcquireOrInit_Thunk(92); sub_8259F630 resource init; copies 3 dwords to +17/+18/+19. serialize_tmodel_vertex_buffer_full/compact_with_vbdata callees. |
| 0x8255FC88 | sub_8255FC88 | render_shader_resource_view_bundle_dtor | 0.92 | vtable off_82047718 (pair of render_shader_resource_view_bundle_init); render_shader_resource_view_clear_lock; release COM at +4. VB serialize paths. |
| 0x8255D200 | sub_8255D200 | render_shader_decl_set_float_by_pair_names | 0.91 | sub_8224DB90 + render_shader_decl_find_entry_by_name; stores single float at entry+4. TwoToneScale/Bias/Power/Fresnel* in livery apply. |
| 0x8255C7B0 | sub_8255C7B0 | render_primitive_decl_prepare_and_load_xml | 0.89 | Invokes vtable at decl+56 then render_primitive_decl_load_xml. Primitive-decl XML load entry point. |
| 0x825601A0 | sub_825601A0 | render_car_apply_livery_shader_constants_by_part_name | 0.92 | Maps part name→wheel slot; applies PaintColor/GlassColor/TwoToneColor/TwoToneScale/Bias/Power/FresnelMin/Max via shader-decl setters on body/window materials (packed suffix when pass+532). |
| 0x82560618 | sub_82560618 | render_car_disable_primary_interface_thread_safe | 0.88 | SetInterfaceThreadSafe(0) on 11 wheel/livery COM interface offsets (+29680..+36368). render_car_instance_apply_draw_path_and_load_resources callee. |
| 0x825606E8 | sub_825606E8 | render_car_disable_secondary_interface_thread_safe | 0.88 | SetInterfaceThreadSafe(0) on 8 body/material interface offsets (+32368..+36496). Same car-instance init path. |
| 0x825609C0 | sub_825609C0 | serialize_object_base_dtor | 0.91 | vtable off_820477A8 (pair of serialize_object_base_init_defaults); d3d_resource_ptr_release_and_clear at +36; FM2_Stl_String_InitOrClear. sub_82560A88/sub_825AA420 callees. |
| 0x82560A88 | sub_82560A88 | serialize_object_base_dtor_with_free | 0.90 | serialize_object_base_dtor then optional FM2_Memory_FreeSmallBlockOrNull. Standard C++ deleting dtor pattern. |
| 0x82562E00 | sub_82562E00 | render_car_get_wheel_slot_shader_bundle_cached | 0.91 | Lazy-cache at part+248: render_car_map_part_name_to_wheel_slot + render_car_object_pass_get_wheel_slot_critsec_ptr. Returns shader decl bundle ptr. |
| 0x82564ED8 | sub_82564ED8 | render_object_pass_cleanup_invisible_children_once | 0.89 | One-shot flag pass+484; for each child without +252 visible flag calls sub_82564660 cleanup. Pass teardown lazy path. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938 command-buffer flag. |
| 0x8255B5A0 | sub_8255B5A0 | Enables texture constants 60/308; defer with pass-bind cluster. |
| 0x8255B7C4 | sub_8255B7C4 | Jump-table fragment, not standalone semantics. |
| 0x8255C768 | sub_8255C768 | One-line "Version" stream reader. |
| 0x82562348 | sub_82562348 | Large reflection/visibility pass (~2KB); needs dedicated read. |
| 0x82563198 | sub_82563198 | Polymorphic collection dtor off_820478A0; batch with 825632A0 next pass. |
| 0x82563428 | sub_82563428 | render_draw_context_vector_dtor + optional free; thin wrapper. |
| 0x82564660 | sub_82564660 | Child COM release helper; paired with 82564ED8—defer detail naming. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no observable side effects. |
