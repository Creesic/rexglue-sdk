
## Iteration 35

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825578B8 | sub_825578B8 | audio_quantize_double_to_unsigned_int16_pair | 0.90 | Returns stack pair [quantized=(2^bits-1)*(a1/a2), max=(2^bits-1)]. sub_825D4FE8 serializes angle 0–360 to uint16. |
| 0x825578F0 | sub_825578F0 | audio_quantize_clamped_double_to_unsigned_int16_pair | 0.89 | fsel clamps input to [0,a2] then unsigned quantize pair on stack. sub_821F7EE8 filter export (4×). |
| 0x82557CE8 | sub_82557CE8 | audio_biquad_set_target_level | 0.91 | Stores float at state+12. sub_82495960 tire-rumble filter reset/advance. |
| 0x82557CF0 | sub_82557CF0 | audio_biquad_set_input_level | 0.91 | Stores float at state+20 (drive level). sub_82495960 sets spring velocity input. |
| 0x82557D20 | sub_82557D20 | audio_biquad_get_target_level | 0.92 | Returns *(a1+12). sub_82495960 reads filter target before mixing. |
| 0x82557D28 | sub_82557D28 | audio_biquad_get_output_level | 0.92 | Returns *(a1+16) output sample. sub_82495960/sub_825B3590/sub_825B8728. |
| 0x82557E28 | sub_82557E28 | render_copy_pose_state_with_bone_chain | 0.88 | Copies 2 flag bytes; VMX128 at +16; 3×32-byte bone blocks via pointer chain at +4. sub_82647838/sub_82647D90. |
| 0x82557EA0 | sub_82557EA0 | render_pose_init_bone_chain_pointers | 0.90 | Sets +4/+8/+12 to self+32/+64/+96 (3 bone slots). sub_8263E3F0/sub_8263E500 pose ctor. |
| 0x82557EC8 | sub_82557EC8 | render_test_triangle_pair_winding_consistent | 0.87 | Cross-product sign tests on shared-edge triangles (8 float coords); returns 0 if winding mismatched. sub_821F71D0 mesh culling. |
| 0x82558008 | sub_82558008 | render_vmx_test_segment_chain_front_facing | 0.88 | Variable-length segment chain (*a1 count); VMX cross/dot vs plane normal at +48; returns 0 if back-facing. sub_82558710 callee. |
| 0x825580A0 | sub_825580A0 | render_vmx_test_triangle_winding_ccw | 0.89 | 3-segment CCW test via 2D cross products on edge vectors. sub_82558120 raycast prefilter. |
| 0x82558120 | sub_82558120 | render_raycast_find_nearest_segment_hit | 0.87 | Iterates segment array; render_vmx_test_triangle_winding_ccw gate; computes parametric hit t vs ray; keeps nearest within tolerance. sub_8263B360. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825581E8 | sub_825581E8 | Large VMX OBB overlap test; decompiler truncated—defer with 82558590 cluster. |
| 0x825560F8 | sub_825560F8 | Thin VMX128 store wrapper only. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no observable side effects. |
