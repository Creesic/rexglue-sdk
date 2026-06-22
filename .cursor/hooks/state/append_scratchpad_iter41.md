
## Iteration 41

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82567010 | sub_82567010 | render_pass_resource_refcounted_dtor_with_free | 0.92 | render_pass_resource_refcounted_dtor then optional FM2_Memory_FreeSmallBlockOrNull. MSVC deleting dtor pattern. |
| 0x82567130 | sub_82567130 | render_scene_object_draw_context_derived_dtor_with_free | 0.92 | render_scene_object_draw_context_derived_dtor + optional heap free. |
| 0x82568ED0 | sub_82568ED0 | render_draw_context_slot_dtor_with_free | 0.92 | render_draw_context_slot_dtor + optional FM2_Memory_FreeSmallBlockOrNull. |
| 0x8256ACF8 | sub_8256ACF8 | render_car_instance_resource_base_dtor_with_free | 0.91 | render_scene_object_instance_dtor(a1+516); FM2_Render_HelperB3E8PathA(a1+20); FM2_PresentationCarConfig_Dtor; optional free. Partial car-instance teardown. |
| 0x8256B088 | sub_8256B088 | render_scene_object_instance_dtor_with_free | 0.92 | render_scene_object_instance_dtor + optional free. |
| 0x8256BA88 | sub_8256BA88 | render_car_instance_dtor_with_free | 0.92 | render_car_instance_dtor + optional FM2_Memory_FreeSmallBlockOrNull. |
| 0x8256BDA0 | sub_8256BDA0 | serialize_tmodel_part_read_with_d3d_validate | 0.90 | Vtable dispatch at +784; lua_read_tmodel_part_and_collected_verts; FM2_D3D_ValidateResourceHandlesOrRecover. Serialize read path with GPU handle recovery. |
| 0x8256BDF0 | sub_8256BDF0 | serialize_tmodel_part_read_from_lua_stream | 0.91 | Vtable at +784 then lua_read_tmodel_part_and_collected_verts only. Pair without D3D validate. |
| 0x8256FD90 | sub_8256FD90 | render_copy_render_state_block_64_from_offset80 | 0.93 | FM2_MemcpyAligned(dest, src+80, 64). Copies 64-byte render-state/matrix block. |
| 0x82570668 | sub_82570668 | render_post_process_pass_init_defaults | 0.90 | vtable off_8204850C; type=3; zeros exposure floats; splats VMX128 identity/permutation constants at +16..+128. sub_82572AE8 ctor path. |
| 0x82570710 | sub_82570710 | render_pass_compute_lighting_basis_vectors_vmx | 0.89 | FM2_Render_ComputePassLightingBasisVectors; packs direction/exposure scalars into VMX128 rows at +16/+32/+64 based on +176 flag. Post-process lighting setup. |
| 0x82571B38 | sub_82571B38 | render_post_process_execute_draw_pass | 0.88 | Configures GPU render states; VMX matrix pack to shader constants; iterates materials calling sub_825704B8/sub_82570608; dispatches draw callbacks via vtable+24. Full post-process pass driver. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938 command-buffer flag. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops still only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no direct xrefs found—defer type naming. |
| 0x8256FDE8 | sub_8256FDE8 | Large VMX constant pack + vtable+148 upload; batch with 825704B8/82570608 next pass. |
| 0x825704B8 | sub_825704B8 | Tonemap/material constant path calling 8256FDE8; defer with post-process cluster. |
| 0x82570608 | sub_82570608 | Thin wrapper unpacking material+64 into 8256FDE8. |
| 0x82570B68 | sub_82570B68 | PowerPC atomic refcount dec; defer with 8256D940 pair. |
| 0x82570D28 | sub_82570D28 | sub_82570C80 + optional free; thin deleting dtor. |
