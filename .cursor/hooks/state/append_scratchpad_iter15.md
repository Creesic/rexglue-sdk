
## Iteration 15

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82566440 | sub_82566440 | render_livery_draw_pass_visibility_gate | 0.90 | Tests material flags at a2+184 vs body visibility helpers; rejects interior category, wing/mirror strstr on material name. render_collect_livery_draw_pairs_for_lua gate. |
| 0x8255F8E0 | sub_8255F8E0 | render_livery_draw_match_material_category | 0.88 | Requires byte+252 set; compares draw category a2 against render_object_pass_lookup_glass_material_category_id result; special cases for badge (a4), categories 3/6-7/12-31. |
| 0x8255F868 | sub_8255F868 | render_object_pass_lookup_glass_material_category_id | 0.91 | Scans off_8299EA20 table: strstr material name against glass*/lexan* substrings; returns paired dword category id or -1. |
| 0x8255D628 | sub_8255D628 | render_object_pass_is_exterior_body_draw | 0.89 | Returns true if visible-body without visible-check, or render_object_pass_is_blur_rim_material. Livery pair collection exterior filter. |
| 0x8255D5A0 | sub_8255D5A0 | render_object_pass_is_blur_rim_material | 0.91 | When byte+55 has 0x80, caches strstr(materialName,"blur_rim") result in +55. Same pattern as badge/emblem helper. |
| 0x8245B640 | sub_8245B640 | path_wildcard_match_ascii | 0.92 | Recursive * / ? glob matcher; optional case-sensitive (a3); else sub_82757FC0 fold per char. Used by render_filter_object_names_by_wildcard. |
| 0x82569068 | sub_82569068 | render_scene_lod_draw_context_ctor | 0.88 | render_draw_context_slot_init_defaults + vtable off_82047FD4; VMX128 zero vector at +800; sub_825C18E0 subobject. Four LOD slots in scene ctor. |
| 0x82568E28 | sub_82568E28 | render_scene_wheel_draw_context_ctor | 0.88 | Same init pattern with vtable off_82047F8C; VMX128 defaults at +800/+808. Two wheel draw contexts in scene ctor. |
| 0x8255D748 | sub_8255D748 | render_draw_context_free_pointer_pair_arrays | 0.90 | FM2_Memory_FreePointerArray_AndSelf on *a1 and a1[1] (count=3 each). render_draw_context_vector_dtor cleanup. |
| 0x82568620 | sub_82568620 | render_object_pass_material_slot_init | 0.87 | FM2_TModel_InitVMXBounds_0; vtable off_82047DD8; sub_825C2BC0; enables +252; clears category cache dwords to -1. Object-pass material node ctor. |
| 0x82567180 | sub_82567180 | render_scene_object_init_car_wheel_static_decls | 0.86 | Loads "CarStaticDecl"/"WheelStaticDecl" via sub_825A20A8 into global/static buffers; pool-alloc nodes; FM2_ComPtr_AssignRef wiring. Scene init subsystems callee. |
| 0x825AA988 | sub_825AA988 | render_draw_queue_build_material_constant_table | 0.87 | Locks RenderPassResource; clears prior 24-byte material entries; iterates shader slots; strstr "_packed" filter; builds constant binding vectors via sub_825A8128/sub_825A8270. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82747C10 | sub_82747C10 | 4.4KB sparse constant-register applicator; still deferred. |
| 0x825AF3C0 | sub_825AF3C0 | Audio/render pass binder; truncated VMX setup. |
| 0x8255C868 | sub_8255C868 | 2KB VMX matrix/interpolator transform; truncated. |
| 0x825A8128 | sub_825A8128 | Pass material slot descriptor decoder; defer with 825A8270 cluster. |
| 0x825A8C90 | sub_825A8C90 | Thin RenderPassResource com-ptr swap; low standalone value. |
