
## Iteration 13

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825BB6E0 | sub_825BB6E0 | render_get_index_count_by_primitive_type | 0.91 | Switch on D3D primitive type (1/2/3/4/5/6/8/13): returns index count via div2, div3, a2-1, a2-2. Used by render_object_pass_emit_indexed_draw. |
| 0x825A2370 | sub_825A2370 | render_classify_shader_primitive_draw_type | 0.88 | Reads dword at +8; returns 2 if value==1 else bitmask from (value-6). Drives blend/color mask in indexed draw path. |
| 0x825683B0 | sub_825683B0 | serialize_named_object_container_array | 0.89 | Builds "%s%s" + "_Container" name; writes count field; read/write loops call serialize_polymorphic_object_ref per element. |
| 0x82564F78 | sub_82564F78 | render_deferred_task_buffer_clear | 0.86 | Frees small block at +4 via FM2_Memory_FreeSmallBlockOrNull; zeros +4/+8/+12. Visibility pass deferred task cleanup. |
| 0x82563120 | sub_82563120 | serialize_polymorphic_object_ref | 0.87 | If stream reading, pool-alloc 64-byte node and init ctor; dispatches vtable serialize on object with base name prefix. |
| 0x825665E8 | sub_825665E8 | render_draw_context_slot_dtor | 0.88 | Sets vtable off_82047DE4; frees strings/COM refs; calls render_draw_context_vector_dtor. Multiple draw-context dtors. |
| 0x82566210 | sub_82566210 | car_load_brake_rotor_texture_resources | 0.90 | Loads rotor0.xds to four livery slots; builds game:\media\brakes\ paths from car part strings; sub_824A6A48 async load. |
| 0x825632F0 | sub_825632F0 | render_draw_context_vector_dtor | 0.87 | Walks vector at +8 releasing each COM object (+4 vtable); frees backing buffer; base dtor for draw context objects. |
| 0x82565498 | sub_82565498 | serialize_material_groups_without_packed_suffix | 0.89 | When not writing, iterates material groups; strips "_packed" suffix via substring helper before sub_825A85D0 serialize. |
| 0x82569118 | sub_82569118 | render_scene_object_instance_ctor | 0.88 | Large init: render_draw_context_slot_init_defaults, 9x render_draw_path_slot_init_vmx_defaults, environment bindings, float defaults. |
| 0x82565740 | sub_82565740 | render_shader_data_file_load_count_records | 0.90 | Appends "sh.data" to path; opens via vtable +84; reads count dword; allocates 76*count bytes for shader records. |
| 0x825686D0 | sub_825686D0 | render_draw_context_slot_init_defaults | 0.89 | Init vtable off_82047DE4; default floats 15/25/50; env binding contexts; clears shader-data fields and strings. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8255C868 | sub_8255C868 | 2KB VMX matrix/interpolator transform; truncated decompile—needs full read. |
| 0x825AA988 | sub_825AA988 | Material constant table builder; still truncated from iter 12 deferral. |
| 0x82747C10 | sub_82747C10 | 4.4KB sparse constant-register applicator; still deferred. |
| 0x825AF3C0 | sub_825AF3C0 | 1.3KB audio/render pass binder with many args; truncated mid-VMX setup. |
| 0x825693D0 | sub_825693D0 | Scene object dtor orchestrator; confident but left for focused pass with 825695E8 chain. |
| 0x8255A320 | sub_8255A320 | Clear stl_string_find_last_substring_offset; will batch with 82564FC0 next iteration. |
