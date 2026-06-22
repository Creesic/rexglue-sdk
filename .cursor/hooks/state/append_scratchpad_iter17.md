
## Iteration 17

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8259EE00 | sub_8259EE00 | shader_static_decl_build_arg_list_from_records | 0.90 | Counts 6-word static records until 0xFF sentinel; pool-alloc 12*(n+1); copies records; terminal entry 0xFF0000/-1. shader_resource_load_static_decl_by_name callee. |
| 0x8255F620 | sub_8255F620 | render_object_pass_enable_material_by_wildcard | 0.91 | Iterates material vector; path_wildcard_match_ascii on name; sets byte+252=1 on first match. Many livery/script enable paths. |
| 0x8255F078 | sub_8255F078 | render_object_pass_clear_enabled_material_texture_data | 0.89 | For materials with +252 set: zeroes paired float arrays at +356/+360, dcbf cache flush, clears +336 and bytes +255/+256. Pre-transform cleanup path. |
| 0x8255F170 | sub_8255F170 | render_object_pass_apply_transform_to_material_vmx | 0.90 | Negates material floats +96/+100/+104 into VMX vector; FM2_Render_TransformVec4x4VMX128; stores four VMX128 results at material+272..320; sets +253. |
| 0x825A1530 | sub_825A1530 | shader_resource_lookup_or_create_by_static_decl | 0.88 | Critsec on stru_82A00FB4; sub_8259F978 lookup or alloc 80-byte shader resource, copy static decl, push to dword_82A00FD0 vector. |
| 0x82606290 | sub_82606290 | stl_vector_20byte_memmove_elements | 0.92 | Memmove 20-byte elements from [a1,a2) to a3 using 5-dword copy loop. stl_vector_20byte_erase_suffix_return_end helper. |
| 0x8255F310 | sub_8255F310 | render_object_pass_copy_matrix_to_material_vmx | 0.89 | Direct lvx128/stvx copy of four matrix rows from args to material+272..320 without transform; sets +253. Simpler matrix upload variant. |
| 0x8255F448 | sub_8255F448 | render_object_pass_accumulate_enabled_material_bounds_vmx | 0.87 | Init min/max sentinel vectors; for enabled (+252) non-skinned materials vaddfp bounds at +96/+112 and +128; expand *_R4/*_R5 AABB. Sort/cull helper. |
| 0x8255F6E0 | sub_8255F6E0 | render_object_pass_disable_material_by_wildcard | 0.91 | Mirror of enable helper: wildcard match then byte+252=0. Livery disable paths. |
| 0x8255F7A0 | sub_8255F7A0 | render_object_pass_find_material_index_by_wildcard | 0.90 | Finds index where byte+252==a3 and name matches wildcard; returns index or -1. Script lookup helper. |
| 0x8255C868 | sub_8255C868 | render_object_pass_apply_skinning_matrix_vmx | 0.88 | When material+253: VMX vmrghw/vmrglw/vmsum4fp skinning from pass matrices; writes transformed matrix to material; FM2_Render_ObjectPassSortAndEmitDraws. |
| 0x825AF3C0 | sub_825AF3C0 | render_pass_execute_draw_batch_with_state_save | 0.87 | Locks pass resource, upload constants, bind vertex stream, saves/restores depth/EDRAM/viewport/Z/tfetch state, indexed draw loop, optional FM2_AudioMix_SubmitPendingOutputBody, flush shader bits, GPU patches. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825A94E0 | sub_825A94E0 | 1KB stl_vector_20byte insert/realloc; still truncated mid-body. |
| 0x8259F978 | sub_8259F978 | Shader resource cache lookup in critsec; defer with 8259F498 cluster. |
| 0x8259F498 | sub_8259F498 | Thin shader resource ctor wrapper (FM2_ResourceManager_InitBase + vtable). |
| 0x8259EF68 | sub_8259EF68 | Static decl record compare; batch with shader cache helpers next pass. |
| 0x8259EEE8 | sub_8259EEE8 | Copies static decl blob into shader resource; paired with 8259EF68. |
