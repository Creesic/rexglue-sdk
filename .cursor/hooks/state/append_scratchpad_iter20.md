
## Iteration 20

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8256DA68 | sub_8256DA68 | render_car_object_pass_apply_wheel_slot_livery_attrs | 0.88 | Copies 144-byte wheel slot via sub_8251DE40; parses livery attr vector; critsec on render_car_object_pass_get_wheel_slot_critsec_ptr; assigns carbon_fiber(_packed) to hood* matches; mirror* filter; VMX128 copies for slots 3/4. |
| 0x82565E88 | sub_82565E88 | render_car_load_wheel_tire_and_carbin_livery_assets | 0.90 | Builds game:\\media\\wheels\\{name}\\wheel.xds; FM2_LiveryMask_ParseAndLoadEntry for wheel/tire slots; tire%c0.xds shared; per-wheel wheel%d.xds loop; _rightside.carbin via sub_824A6A48. render_car_instance_apply_draw_path caller. |
| 0x8256ABA0 | sub_8256ABA0 | lua_alloc_and_read_car_material_slot | 0.91 | On Lua read path: pool 400 + render_object_pass_material_slot_init; vtable serialize/read dispatch. lua_read_named_part_ptr_container per-element helper. |
| 0x825AB6F8 | sub_825AB6F8 | serialize_alloc_and_write_tmodel_vmx_bounds | 0.91 | Serialize path mirror: pool 128 + FM2_TModel_InitVMXBounds; vtable dispatch. serialize_named_part_ptr_container helper. |
| 0x8256B7B0 | sub_8256B7B0 | render_car_instance_ctor | 0.89 | render_car_instance_resource_base_init then vtable off_820480BC; inits 11 critsecs +36624..36627 byte flags cleared. render_car_instance_acquire_or_create_in_critsec callee. |
| 0x8256ACA8 | sub_8256ACA8 | render_car_instance_resource_base_init | 0.88 | FM2_ResourceManager_InitBase; vtable off_82048064; FM2_Render_HelperB3E8DrawPathTail; render_scene_object_instance_ctor at +516. Base before full car ctor. |
| 0x8255B788 | sub_8255B788 | render_car_object_pass_get_wheel_slot_critsec_ptr | 0.87 | Returns a1+272 or jump-table dispatch via byte_82047410 for slot index 1..8. Used by wheel-slot livery apply and 825601A0/82562E00. |
| 0x8256E588 | sub_8256E588 | render_car_instance_init_model_and_livery_resources | 0.85 | Large car bootstrap: Car/CarAtttributes/Model COM loads; driver.anim.bin/skinbin/xds; livery texture lock churn; 9× wheel slot apply; render_object_pass_apply_livery_material_mask_from_xml; optional sub_82563A78. |
| 0x82563588 | sub_82563588 | render_car_wheel_static_decl_load_once | 0.88 | One-shot byte_82A00B02 gate; pass-resource lock; iterates static decl records; strstr "packed" picks alt bundle; sub_823840F8 load. render_scene_object_init_car_wheel_static_decls caller. |
| 0x8256AD58 | sub_8256AD58 | serialize_tmodel_part_with_d3d_resource_validate | 0.86 | If part size>=0x258 serializes TModelPart Name/VB/Materials; else compact path sub_8256AAF0; recomputes +224 from AABB floats; FM2_D3D_ValidateResourceHandles + sub_82563478 VB bind. |
| 0x82560890 | sub_82560890 | render_shader_static_decl_bundle_release | 0.90 | vtable off_820477A0; FM2_ResourceLock_ClearAndReleaseHandle; release COM at +4. Paired dtor for static-decl bundle. |
| 0x82562E58 | sub_82562E58 | render_shader_static_decl_bundle_init | 0.90 | Sets off_820477A0 vtable; critsec spin 0x600; sub_825608E8 copies source bundle. Used by render_car_wheel_static_decl_load_once. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82563A78 | sub_82563A78 | Pass-resource lock wrapper whose nested loops only increment counters—no observable side effects in decompile. |
| 0x82558E70 | sub_82558E70 | Pure FP8 geometry/math kernel; still no domain strings or callers with context. |
| 0x8256B440 | sub_8256B440 | Draw-pass primitive-range gate (types 13–0x25) calling sub_82569AE0; defer until 82569AE0 analyzed. |
| 0x8256AAF0 | sub_8256AAF0 | Compact TModelPart serialize (Name/VB/Material only); thin—batch with 82562EB0 next pass. |
| 0x8255B7C4 | sub_8255B7C4 | Jump-table target fragment; not a standalone callable with clear semantics. |
