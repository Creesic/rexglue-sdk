
## Iteration 18

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8259EF68 | sub_8259EF68 | shader_static_decl_records_equal | 0.93 | Byte-wise compare of paired 12-byte static-decl records until first word 255; returns 1 on full match. Used by shader_resource_lookup_by_static_decl_in_critsec. |
| 0x8259EEE8 | sub_8259EEE8 | shader_static_decl_copy_records_to_pool | 0.92 | Frees prior buffer; counts 6-word records until 255; pool-alloc 12*(n+1); FM2_MemcpyAligned copy. shader_resource_lookup_or_create_by_static_decl callee. |
| 0x8259F498 | sub_8259F498 | shader_resource_init_base | 0.91 | FM2_ResourceManager_InitBase; vtable off_82106FB8; clears fields +17/+19. Pool ctor path from shader_resource_lookup_or_create_by_static_decl. |
| 0x8259F978 | sub_8259F978 | shader_resource_lookup_by_static_decl_in_critsec | 0.90 | RtlEnterCriticalSection stru_82A00FB4; scans dword_82A00FD0 vector; shader_static_decl_records_equal on +68; SetInterfaceThreadSafe out ptr. |
| 0x825A94E0 | sub_825A94E0 | stl_vector_20byte_insert_n_copies_at | 0.91 | MSVC vector insert: copies 20-byte template from a4; grow/realloc via stl_vector_20byte_allocate_n_elements; FM2_Stl_ThrowLengthError_VectorTooLong guard. |
| 0x825A8340 | sub_825A8340 | stl_vector_20byte_allocate_n_elements | 0.92 | Overflow check 0xFFFFFFFF/0x14; FM2_AllocPoolAcquireOrInit_Thunk(20*n). Helper for 20-byte element vectors. |
| 0x825A90E0 | sub_825A90E0 | stl_vector_20byte_shift_range_backward | 0.90 | Shifts [a1,a2) 20-byte elements backward by computed slot count using 5-dword copy loop. Insert helper tail. |
| 0x825A26F0 | sub_825A26F0 | d3d_resource_ptr_release_and_clear | 0.94 | D3DResource_Release if non-null; stores 0. car_presentation_lazy_release_d3d_refs callee. |
| 0x8256C248 | sub_8256C248 | render_object_pass_apply_livery_material_mask_from_xml | 0.88 | Seeds default car-part attr names (body/bumper/wheel/glass/lexan/...); enables/disables via wildcard helpers; bumperR/exhaust special-case wiring. |
| 0x82562BF8 | sub_82562BF8 | render_car_pass_bind_environment_texture_samplers | 0.87 | FM2_Render_ObjectPassDrawSetupCore then FM2_Render_DrawPassMaterialSetupSharedHelper for envSampler/envStaticSampler/lightsSampler/dirt/damage/carbon samplers. |
| 0x8256E350 | sub_8256E350 | render_load_global_car_attributes_if_needed | 0.86 | One-shot load of game:\\media\\cars\\GlobalCarAttribs.xml via filesystem COM; sub_8255AB80 GlobalCarAtttributes; gated on byte_82A00C01. |
| 0x8255C460 | sub_8255C460 | lua_read_car_livery_draw_pair_ptr_array | 0.85 | Lua table read: get count (+144 vtable), alloc count×8 pointer pairs, populate via draw-pair readers sub_8255A1D8/sub_8255A270. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8255A460 | sub_8255A460 | Mirror of 8255C460 cluster; need paired read to distinguish semantics. |
| 0x8256DE90 | sub_8256DE90 | Large livery/XML caller of 8256C248; truncated body this pass. |
| 0x8256E588 | sub_8256E588 | Second caller of livery mask helper; defer with DE90 cluster. |
