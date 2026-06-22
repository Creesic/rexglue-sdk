
## Iteration 21

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8256B440 | sub_8256B440 | render_draw_pass_setup_primitive_types_13_to_31 | 0.88 | FM2_Render_DrawPassMaterialSetupBodyB then gates primitive type a3 in [13,31]; toggles material byte+184 bit7 during sub_82569AE0 call. FM2_Render_TestPassVisibilityVMXCore callee. |
| 0x82569AE0 | sub_82569AE0 | render_object_pass_emit_draws_for_primitive_range | 0.87 | Clamps index range; requires a2>0xA; FM2_Render_ObjectPassDrawSetupMaterialCore + pass-resource lock; render_collect_livery_draw_pairs_for_lua or nested material emit loops; FM2_Render_ObjectPassDrawSetupSortKeys. |
| 0x8256AAF0 | sub_8256AAF0 | serialize_tmodel_part_compact_fields | 0.90 | Serialize Name + serialize_tmodel_vertex_buffer_compact_with_vbdata_read("VertexBuffer") + Material container. Compact branch of serialize_tmodel_part_with_d3d_resource_validate. |
| 0x82562EB0 | sub_82562EB0 | serialize_tmodel_vertex_buffer_full_with_vbdata_write | 0.89 | VBParams shader lookup (sub_8255C2F0 + FM2_ShaderResource_LoadIfReady); VBData via sub_825A8AF8 write path. Used for CollectedVerts on large TModel parts. |
| 0x82562FE8 | sub_82562FE8 | serialize_tmodel_vertex_buffer_compact_with_vbdata_read | 0.89 | Same VBParams/VBData flow as 82562EB0 but sub_825A8888 read path. serialize_tmodel_part_compact_fields helper. |
| 0x82563478 | sub_82563478 | render_tmodel_part_compute_vertex_buffer_range | 0.91 | From index span +180..+182 and VB stride +344: sets +348 count, +352/+356 byte offsets, pool-alloc +360 index table; FM2_D3D_ValidateResourceHandles. serialize_tmodel paths. |
| 0x825608E8 | sub_825608E8 | render_shader_static_decl_bundle_assign_resource_lock | 0.90 | Clears prior lock, FM2_ResourceLock_AssignRetainedHandle, WaitReadyState3, stores +76 offset at +36. render_shader_static_decl_bundle_init callee. |
| 0x8256B8A8 | sub_8256B8A8 | render_car_instance_dtor | 0.91 | vtable off_820480BC; render_car_instance_clear_wheel_resource_ptrs; release 11 wheel COM slots; render_scene_object_instance_dtor; FM2_Render_HelperB3E8PathA; FM2_PresentationCarConfig_Dtor. |
| 0x825607B0 | sub_825607B0 | render_car_instance_clear_wheel_resource_ptrs | 0.92 | SetInterfaceThreadSafe(0) on a1+9068..+9148 (11 wheel/livery COM ptr fields). render_car_instance_dtor first step. |
| 0x82560968 | sub_82560968 | render_shader_resource_view_bundle_init | 0.89 | vtable off_82047718; critsec; sub_8255FCE0 assigns shader resource lock (+80 data ptr). VB serialize shader-load wrapper. |
| 0x8256BD10 | sub_8256BD10 | lua_read_tmodel_part_and_collected_verts | 0.88 | Vtable +128 dispatch; lua_read_car_part_container_and_draw_pairs; if size>=0x2C6 serializes CollectedVerts VB to +64/+96 else +32. serialize_tmodel_part_update_bounds caller. |
| 0x8256BE38 | sub_8256BE38 | serialize_tmodel_part_update_bounds_and_vb_ranges | 0.87 | After lua_read_tmodel_part_and_collected_verts: bind VB ranges via render_tmodel_part_compute_vertex_buffer_range per part; render_object_pass_accumulate_enabled_material_bounds_vmx; VMX normalize radius at +34116. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82563A78 | sub_82563A78 | Still only counter loops under pass-resource lock—no side effects visible. |
| 0x82558E70 | sub_82558E70 | FP8 geometry kernel; no new caller/string context since iter 20. |
| 0x825505A8 | sub_825505A8 | Lua table quicksort (FM2_Lua_SetStackTop/recursive); needs pairing with sub_82550510 compare helper. |
| 0x82554C60 | sub_82554C60 | 16-byte vector insert/realloc; defer with 825546C8 56-byte vector cluster. |
| 0x82558BA8 | sub_82558BA8 | VMX vec4 normalize when w!=0; math helper without domain strings—defer. |
