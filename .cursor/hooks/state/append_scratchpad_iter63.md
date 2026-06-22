## Iteration 63

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82579BF0 | sub_82579BF0 | render_pass_lighting_normalize_transform_triple_store_slot | 0.87 | VMX normalize v1/v2; `render_pass_lighting_fetch_slot_vector_xyz`; triple `FM2_Render_TransformVec4x4VMX128` on slot row at object+744; `render_pass_lighting_store_incoming_vmx128_slot_and_bind`; optional stvx copy from a3. Vtable 0x82191DF0; called from dual-slot blend. |
| 0x82579EF8 | sub_82579EF8 | render_pass_lighting_apply_dual_slot_vmx128_blend | 0.86 | `render_pass_lighting_compute_blended_slot_vmx128_row`; fetches slots 4/5 or 7/8 via fetch_slot_vector_xyz; VMX normalize/blend branches; recursively calls normalize_transform_triple_store_slot. Vtable 0x82191DF8. |
| 0x8257A678 | sub_8257A678 | render_pass_lighting_apply_randomized_dual_slot_blend | 0.85 | Gate `render_pass_lighting_is_pass_ready`; builds pass-context VMX vectors; `FM2_BufferedFileRead_RandUnitFloat` ±0.05 jitter; calls apply_dual_slot_vmx128_blend twice (a2=0/1); ends `FM2_Render_ClearPassLightingSlotVMX`. Vtable 0x82191E08. |
| 0x8257BB18 | sub_8257BB18 | render_skinned_model_setup_from_asset_name | 0.88 | STL string assign; `render_skinned_model_assign_normalized_shader_path`; resource-lock init; mesh setup via sub_827C3EE8/4188; alloc small blocks 184-186; livery pending fields; sub_827C3D18 at 0.5. Vtable 0x82191E50. |
| 0x8257BC88 | sub_8257BC88 | render_car_driver_init_shader_livery_model_and_decl | 0.91 | `render_shader_resource_load_by_name_normalized`; `FM2_LiveryMask_ParseAndLoadEntry`; setup_from_asset_name; `render_car_driver_load_shader_resources_and_decl`. Vtable 0x82191E58. |
| 0x8257BD00 | sub_8257BD00 | render_car_driver_init_shader_livery_model_and_decl_thunk | 0.93 | Thin wrapper: `return render_car_driver_init_shader_livery_model_and_decl(*a1, ...)`. |
| 0x8257BD08 | sub_8257BD08 | render_car_material_write_constants_to_xts_buf | 0.85 | Material vtable +76 writes header to global byte_82143980 (XTS transport buf); loops `3*dword_829A1464` calling vtable +108 per dword at a1+16 step +4. Vtable 0x82191E60. |
| 0x8257BD98 | sub_8257BD98 | render_car_material_write_all_constants_to_xts_buf | 0.86 | Material vtable +132; loops `dword_829A1460` entries stride 208 calling write_constants_to_xts_buf. Vtable 0x82191E68. |
| 0x8257BE18 | sub_8257BE18 | render_ai_path_interpolate_track_samples | 0.88 | Index math on a1[24]/a1[28] 224-byte path records; linear lerp `(sample1-sample0)*t+sample0` for 3×16 float rows into output a7. Vtable 0x82191E70. |
| 0x8257C028 | sub_8257C028 | render_ai_path_interpolate_track_samples_if_ready | 0.89 | Validates path vector + dword_829F1B88 AI driver; `FM2_AIDriver_GetPathBufferLength`; calls interpolate_track_samples or zeroes 3*a7 outputs. Vtable 0x82191E78. |
| 0x82727E80 | sub_82727E80 | render_pass_lighting_fetch_slot_vector_xyz | 0.90 | `FM2_Render_ComputePassLightingSlotOffset`; reads float xyz at slot record +48/+52/+56 into out-param. Shared by pass-lighting blend/transform paths. |
| 0x825AD4F0 | sub_825AD4F0 | render_skinned_model_assign_normalized_shader_path | 0.88 | STL assign + `FM2_BufFile_NormalizePathToLowercase`; resource manager LoadAndWait; `FM2_ShaderResource_LoadIfReady`. Called from setup_from_asset_name. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x825AC180 | sub_825AC180 | Mesh/resource handle resolver; callee chain not fully mapped. |
| 0x827C3EE8 | sub_827C3EE8 | Skinned-model mesh binding init; needs paired 827C4188/4028 analysis. |
| 0x82728110 | sub_82728110 | Size query for skinned-model pool alloc; thin helper. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel; not enough standalone semantics. |
| 0x821DA010 | sub_821DA010 | Called from pass-lighting normalize path; purpose unclear this pass. |
