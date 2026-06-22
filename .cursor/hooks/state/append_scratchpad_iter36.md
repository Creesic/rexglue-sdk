
## Iteration 36

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825581E8 | sub_825581E8 | render_test_obb_overlap_sat_vmx | 0.90 | VMX transforms OBB axes; SAT projection tests on 15 axes (3+3 face + 9 cross); returns BOOL. sub_82652FD0 builds OBB from sub_82652220 and calls overlap test. |
| 0x82558590 | sub_82558590 | render_aabb_init_from_two_points | 0.91 | Per-axis min/max of two vec3 inputs (a23–a25 vs a27–a29) into 7-float AABB [min.xyz, max.xyz]. sub_8263B270 raycast AABB prep before render_raycast_find_nearest_sphere_hit_vmx. |
| 0x82558610 | sub_82558610 | render_aabb_expand_to_include_point | 0.92 | Shrinks min[0–2] and grows max[4–6] toward new point triplet. sub_82652310/sub_82655C08 bounding-volume growth. |
| 0x825586A0 | sub_825586A0 | render_aabb_contains_aabb | 0.93 | Returns 1 iff this min≤other.min and this max≥other.max on all 3 axes. sub_8263C3C8 containment gate. |
| 0x82558710 | sub_82558710 | render_vmx_test_convex_chains_separating_axis | 0.89 | SAT between two convex segment chains (*a1/*a2 counts); finds up to 2 contact axes; render_vmx_test_segment_chain_front_facing gate; writes separation depth to a3. sub_826531E0 collision. |
| 0x82558AB8 | sub_82558AB8 | render_pack_normal_21bit_from_float_triplet | 0.90 | Packs 3 floats scaled by 1023/1023/511 into 21-bit integer (11+11+9 layout). sub_8263B8B8 vertex/normal packing. |
| 0x82558B28 | sub_82558B28 | render_unpack_float_triplet_from_21bit | 0.91 | Inverse: bit-extract 3 fields from int a2; scale by 0.00097751711/0.0019569471. render_vmx_normalize_vec4_if_w_nonzero callee. |
| 0x82558D88 | sub_82558D88 | render_vmx_raycast_segment_hit_test | 0.88 | VMX dot/cross tests ray vs segment between v1–v2; parametric t clamped [0,1]; vmaddfp hit point to out. sub_821DF6F8 raycast path. |
| 0x82559E98 | sub_82559E98 | render_car_wheel_static_decl_lookup_by_slot | 0.90 | Index (a2>>18) into 20-byte table at pass+528; fills 8 dwords with decl pointers/counts. render_car_wheel_static_decl_load_once callee. |
| 0x82559F60 | sub_82559F60 | render_pass_bind_packed_shader_slot | 0.89 | Stores packed slot id at pass+520; base ptr at +516; dcbz128 clears pass+320 if a3&3; sets pass+692. sub_825D6BB0/sub_825D7560 material bind. |
| 0x8255A438 | sub_8255A438 | render_object_pass_sort_by_sort_key_desc | 0.92 | qsort comparator: *(float*)(*a1+104) > *(float*)(*a2+104). FM2_Render_ObjectPassDrawSetupBody + render_object_pass_emit_draws_for_primitive_range. |
| 0x8255A6A8 | sub_8255A6A8 | render_car_driver_visual_damage_load_xml | 0.91 | XML loader: Damage/value, Tires/FrontTireScale+RearTireScale, Driver/BodyLean/ButtOffset/HandOffsets/ShifterOffset/HandBrakeOffset, bool HasPaddleShifter/HasNoHandBrake, accel scales. render_car_instance_init_model_and_livery_resources callee. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper; used as helper inside OBB test. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only; no behavior. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only; no behavior. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper: FM2_Render_GetGlobalRenderContext → sub_8236F938(0). |
| 0x8255B610 | sub_8255B610 | Tire/shadow param init with 0.0254/0.001 scales; defer with render_car_instance cluster. |
| 0x8255B6D0 | sub_8255B6D0 | 48-byte ring-buffer push; callers sub_8251ADB0 not yet analyzed. |
| 0x8255B7C4 | sub_8255B7C4 | Decompiler shows uninitialized base pointer; insufficient evidence. |
| 0x8255B7E8 | sub_8255B7E8 | Rolling nibble string hash; used for Car%d id—defer with identifier cluster. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no observable side effects. |
