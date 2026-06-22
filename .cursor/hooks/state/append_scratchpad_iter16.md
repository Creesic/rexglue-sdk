
## Iteration 16

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825A8128 | sub_825A8128 | render_pass_decode_material_slot_descriptor | 0.91 | Decodes packed shader slot index a2 into 40-byte descriptor: register/type/count fields from pass+264/+268 tables and optional remap at +612/+644. render_draw_queue_build_material_constant_table. |
| 0x825A8270 | sub_825A8270 | render_pass_test_material_constant_flag_bit | 0.86 | Reads 20-byte material-group entry at pass+528; indexes bitfield qwords by a3; returns single flag bit via variable shifts. Material table build inner loop. |
| 0x825A9370 | sub_825A9370 | render_material_constant_table_index_20byte | 0.90 | Bounds-checked index into vector with 20-byte elements; throws stl_throw_invalid_vector_subscript on OOB. Returns element pointer. |
| 0x825A9428 | sub_825A9428 | render_shader_constant_table_index_32byte | 0.90 | Same pattern with 32-byte stride for shader constant source vector. Paired with 20-byte material indexer. |
| 0x825A8C90 | sub_825A8C90 | render_pass_resource_com_ptr_assign | 0.88 | render_pass_resource_add_ref on new ptr; swap *a1; render_pass_resource_release on old. render_draw_queue_build_material_constant_table lock holder. |
| 0x825A9CB8 | sub_825A9CB8 | stl_vector_20byte_ensure_capacity_at | 0.87 | Ensures 20-byte vector has capacity for index HIDWORD(a2): erase suffix or insert default elements via sub_825A94E0/sub_825A9238. Material binding vector growth. |
| 0x825A20A8 | sub_825A20A8 | shader_resource_load_static_decl_by_name | 0.89 | sub_8259EE00 builds arg list from static record; sub_825A1530 lookup; FM2_ResourceManager_LoadAndWait + FM2_ShaderResource_LoadIfReady. CarStaticDecl/WheelStaticDecl paths. |
| 0x825A9310 | sub_825A9310 | stl_throw_invalid_vector_subscript | 0.93 | Throws std::out_of_range with string "invalid vector<T> subscript". Shared bounds-check failure helper. |
| 0x825A9238 | sub_825A9238 | stl_vector_20byte_erase_suffix_return_end | 0.88 | Validates begin/end pointers; sub_82606290 memmove shrink; returns new end iterator pair. Vector erase helper for 20-byte elements. |
| 0x823CBD38 | sub_823CBD38 | render_pass_resource_add_ref | 0.92 | Increments refcount dword at pass+772; returns new count. Called when assigning pass resource pointer. |
| 0x823CC290 | sub_823CC290 | render_pass_resource_release | 0.90 | Decrements pass+772; on zero: critsec, FM2_Render_BindPassSurfacesForKick, recursive child release, FM2_Memory_FreeTagged. Com-ptr release path. |
| 0x82747C10 | sub_82747C10 | render_pass_apply_sparse_gpu_constant_patches | 0.87 | Large pass applicator: VMX128 load/store loops over pass+256/+288 blocks; iterates pass+292 entries; FM2_D3D_ApplyGpuMemoryPatches per set bit; dcbz128 tail. Pair to render_pass_vmx_clear_constant_register_masks. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825A94E0 | sub_825A94E0 | 1KB stl_vector_20byte insert/realloc; truncated mid-memmove—defer. |
| 0x825AF3C0 | sub_825AF3C0 | Audio/render pass binder; truncated VMX setup. |
| 0x8255C868 | sub_8255C868 | 2KB VMX matrix/interpolator transform; truncated. |
| 0x8259EE00 | sub_8259EE00 | Static-decl arg list builder; defer with shader load cluster docs. |
| 0x8255F620 | sub_8255F620 | Wildcard-enable material slot helper; batch with 8255F078 next pass. |
| 0x8255F078 | sub_8255F078 | Clears material float texture arrays + dcbf; needs pairing name with 8255F170. |
