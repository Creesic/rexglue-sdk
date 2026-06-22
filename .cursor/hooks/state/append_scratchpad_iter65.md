## Iteration 65

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825AC180 | sub_825AC180 | render_resource_lock_resolve_data_section_offset | 0.89 | If resource handle at a1+4 ready (`xex_module_is_load_ready_flag`), calls `xex_module_get_data_section_offset_wait_ready` for ".data"; returns offset out-param. Called from skinned_model_setup_from_asset_name. |
| 0x8272E188 | sub_8272E188 | xex_module_lookup_section_offset_by_name | 0.90 | Linear search module section table (40-byte named entries at module+18); strcmp name; returns section base offset at +20/+24 sum. |
| 0x8272E300 | sub_8272E300 | xex_module_get_data_section_offset_wait_ready | 0.91 | If module flag+22 bit1 set, SleepEx spin until bit0 ready; then lookup ".data" section offset. |
| 0x8272E2F0 | sub_8272E2F0 | xex_module_is_load_ready_flag | 0.92 | Returns `*(_DWORD *)(a1+88) & 1`. Gate for data-section resolve. |
| 0x82728110 | sub_82728110 | render_skinned_mesh_compute_buffer_size_bytes | 0.90 | Pure size formula from ushort counts at +40/+42 plus bit-packed tail sizes; used before small-block alloc in setup_from_asset_name. |
| 0x827280B0 | sub_827280B0 | render_skinned_mesh_binding_body_init_zero | 0.91 | Sets body ptr to qword_8211E160; zeros vertex/index counts, stream ptrs, and dword fields across binding body +4..+124. |
| 0x827C4320 | sub_827C4320 | render_skinned_model_init_material_constants_from_decl | 0.88 | Reads material decl at resource+40; index via sub_827C4268; conditionally loads float constants into object+42..+48 based on byte flags +28..+35. |
| 0x827C3D60 | sub_827C3D60 | render_skinned_model_bind_mesh_layout_from_resource | 0.89 | Copies resource layout to binding body: frame counter, vertex counts, index/vertex buffer setup via sub_82728080/82727F80/82803180/828030F8; packs channel flags; sub_82803088/E8. |
| 0x827C4028 | sub_827C4028 | render_skinned_model_dispatch_animation_callbacks | 0.87 | Queries mesh buffer ptrs/counts; sub_827C5938 sample time; dispatches off_829BA5C0 callbacks for resource+36 and +72 decl slots; sub_827C3D30/sub_82727FA0 fallback. |
| 0x827C3D18 | sub_827C3D18 | render_skinned_model_update_animation_phase_wrapped | 0.86 | Updates phase state at object+136: stores new phase, prior phase copy, wrap-mode switch on +16 (clamp/loop/ping-pong) with integer cycle counter at +22. |
| 0x827CBA20 | sub_827CBA20 | render_skinned_model_animation_timeline_init_defaults | 0.88 | Init timeline at +136: AbsFloat(1.0) rate, sub_827CB848 bounds 1.0/1.0, zero phase fields. Called from mesh_binding_init_from_resource. |
| 0x8257CA38 | sub_8257CA38 | render_car_shader_pack_load_from_named_stream | 0.90 | STL path; stream vtable +84 open; on success NotifyManagerStateChange + render_car_shader_pack_load_counts_and_write_constants. Vtable 0x82191ED8. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x8257CB40 | sub_8257CB40 | Thin notify wrapper around load_from_named_stream; defer. |
| 0x8257CC98 | sub_8257CC98 | CHashName init; FM2_HashName_CtorEmpty exists nearby—defer batch. |
| 0x8257CE18 | sub_8257CE18 | CHashName copy ctor; defer hash_name cluster. |
| 0x8257CE80 | sub_8257CE80 | CHashName stream deserialize; defer. |
| 0x8257CF68 | sub_8257CF68 | LCG hi15 helper; defer with CFd0/D018 cluster. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x82727ED0 | sub_82727ED0 | Stores dword at +108 only. |
| 0x82727ED8 | sub_82727ED8 | Single field read; defer mesh ptr getter batch. |
| 0x827C5938 | sub_827C5938 | Animation sample evaluator; needs 827CB838/4228 subgraph. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
