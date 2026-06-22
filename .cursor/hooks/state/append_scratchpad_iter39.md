
## Iteration 39

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8255C768 | sub_8255C768 | serialize_stream_read_version_field | 0.90 | Dispatches stream vtable+116 to read "Version" field into out param. Serialize stream helper. |
| 0x82562348 | sub_82562348 | render_car_process_reflection_probe_command_ring | 0.88 | Drains 512-entry ring at pass+25392..+25396; for visible objects (+252) with materials calls render_vmx_compute_reflection_direction; accumulates overlay +336; may enqueue deferred ComPtr task. Uses render_car_map_part_name_to_wheel_slot. |
| 0x82563198 | sub_82563198 | serialize_polymorphic_object_collection_dtor | 0.91 | vtable off_820478A0; releases COM refs in vector at +17; frees buffer; releases object at +8; clears name string. sub_82566588/sub_825632A0 callees. |
| 0x825632A0 | sub_825632A0 | serialize_polymorphic_object_collection_dtor_with_free | 0.90 | serialize_polymorphic_object_collection_dtor + optional FM2_Memory_FreeSmallBlockOrNull. |
| 0x82563428 | sub_82563428 | render_draw_context_dtor_with_free | 0.91 | render_draw_context_vector_dtor then optional heap free. Standard deleting dtor. |
| 0x82564660 | sub_82564660 | render_object_pass_release_child_com_refs_and_clear_buffer | 0.90 | Frees block at +90; releases each child COM at vector+16; FM2_CircularBuffer_EraseRange. render_object_pass_cleanup_invisible_children_once callee. |
| 0x82565868 | sub_82565868 | render_shader_data_file_splice_nodes_and_load | 0.91 | FM2_IntrusiveList_SpliceNodes at +496; render_shader_data_file_load_count_records; sets byte+500. sub_8251D960 shader preload path. |
| 0x825658A8 | sub_825658A8 | render_car_load_livery_texture_and_metadata_assets | 0.92 | Builds game:\media\cars\{name}\ paths; loads lights/damageLights/badge2.xds, shared textures, CarAttribs.xml, .carbin, driver.anim.bin, ShaderSettings.xml via FM2_LiveryMask_ParseAndLoadEntry/sub_824A6A48. render_car_instance_apply_draw_path callee. |
| 0x82566588 | sub_82566588 | render_scene_object_draw_context_derived_dtor | 0.89 | vtable off_82047DD8; frees +90 block; render_draw_context_clear_subobject_buffers; serialize_polymorphic_object_collection_dtor. sub_82567130 wrapper. |
| 0x825732D8 | sub_825732D8 | render_draw_context_clear_subobject_buffers | 0.90 | Frees vector buffers at a1+6 and a1+2; zeros capacity fields. render_scene_object_draw_context_derived_dtor callee. |
| 0x82566F98 | sub_82566F98 | render_pass_resource_refcounted_dtor | 0.91 | render_pass_resource_release at +6; frees buffer +3; assigns CRefCountedObjectThreadSafe then FM2_Object_AssignBaseVtable_82000E18. sub_82567010 wrapper. |
| 0x8255B5A0 | sub_8255B5A0 | render_pass_enable_wheel_texture_constants | 0.90 | vtable+24 enables shader constants 60 and 308 from pass+864/+868. Wheel texture bind path. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938 command-buffer flag. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops still only increment counters. |
| 0x82567010 | sub_82567010 | render_pass_resource_refcounted_dtor + optional free; thin deleting dtor. |
| 0x82567130 | sub_82567130 | render_scene_object_draw_context_derived_dtor + optional free; thin wrapper. |
