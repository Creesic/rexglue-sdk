
## Iteration 40

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8256AF30 | sub_8256AF30 | render_object_pass_emit_draws_with_drop_shadow_constants | 0.91 | If material+184 has bit4 and +168 set: binds "c_dropShadow" vector (1,1,1,0.1); temporarily inverts shadow scales at +64/+84; calls render_object_pass_emit_draws_for_primitive_range; restores scales. render_draw_pass_setup_primitive_types_13_to_31 callee. |
| 0x8256BAD8 | sub_8256BAD8 | render_car_draw_path_copy_extended_state_from_source | 0.90 | render_car_draw_path_copy_state_from_source then copies fields +448..+487 (ints + 6 STL strings). render_car_instance_acquire_or_create_in_critsec callee. |
| 0x8256B330 | sub_8256B330 | render_car_execute_object_pass_draw_traversal | 0.89 | FM2_Render_HelperB3E8DrawPathTail; FM2_Render_ObjectPassDrawTraversalInner; optional sub_824A1140 resource wait; SetInterfaceThreadSafe(0). sub_8234F658/sub_82350D10 callees. |
| 0x82568F20 | sub_82568F20 | render_car_apply_wheel_basis_normalize_and_draw_setup | 0.88 | VMX vmaddfp transforms wheel basis from first child +160 using pass matrices at +320; vrsqrtefp normalize; stores length at pass+848; FM2_Render_ObjectPassDrawSetupBody. Wheel draw setup path. |
| 0x825562F0 | sub_825562F0 | render_copy_affine_transform_48_bytes | 0.93 | FM2_MemcpyAligned(a1,a2,48). Used when copying 3×4 affine blocks in euler/pose paths (sub_821E2BC8/sub_8263D730). |
| 0x825572F8 | sub_825572F8 | render_vmx_build_orthonormal_basis_from_vec3 | 0.90 | Normalizes vec3 at result; builds tangent at +16 and bitangent at +32 via vpermwi128 cross products. sub_8264AE08/sub_8264C758 callees. |
| 0x825573A8 | sub_825573A8 | render_vmx_clamp_vec3_length_squared | 0.91 | vmsum3fp length²; if > a2² scales vec3 down by a2/sqrt(len²). sub_82647838/sub_82652310 render paths. |
| 0x825573F8 | sub_825573F8 | render_vmx_project_point_onto_line | 0.90 | Computes t=dot(v1,v2)/dot(v2,v2); returns v1 - t*v2 as point on line through origin along v2. sub_82290620/sub_82641880 callees. |
| 0x82557468 | sub_82557468 | math_clamp_value_toward_target_by_max_step | 0.92 | If (target-current)>maxStep return current+maxStep; if within ±maxStep return target; else current-maxStep. sub_824967F0 spring/damper (4×). |
| 0x825574A0 | sub_825574A0 | math_quintic_smoothstep_blend | 0.89 | Quintic polynomial in t=a5 blending four coefficients a1..a4. sub_821E2BC8 euler/animation compose (3×). |
| 0x8256C170 | sub_8256C170 | render_cxml_stream_ctor | 0.90 | FM2_RedirectStream_CtorFromSource; CXMLStream vftable; FM2_XmlReader_CtorWithBufferSizes(50,50); sub_8242CA30 parse or error string path. render_car_instance_init_model_and_livery_resources callee. |
| 0x8256F568 | sub_8256F568 | render_compute_sin_half_angle_ratio | 0.88 | Returns sin(a9°×π/180×0.5) / sin(*(a1+172)°×π/180×0.5); early-out 1.0 if +172 is zero. LOD/angle ratio helper. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938 command-buffer flag. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops still only increment counters. |
| 0x82567010 | sub_82567010 | render_pass_resource_refcounted_dtor + optional free; thin deleting dtor. |
| 0x82567130 | sub_82567130 | render_scene_object_draw_context_derived_dtor + optional free; thin wrapper. |
| 0x82568ED0 | sub_82568ED0 | render_draw_context_slot_dtor + optional free; thin wrapper. |
| 0x8256ACF8 | sub_8256ACF8 | Car resource-base deleting dtor; batch with 8256BA88/8256B088. |
| 0x8256BDA0 | sub_8256BDA0 | lua_read_tmodel_part + D3D validate; pair with 8256BDF0. |
| 0x8256D940 | sub_8256D940 | Refcount decrement/release thunk; needs owning type context. |
