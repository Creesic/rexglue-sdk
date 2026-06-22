
## Iteration 33

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82556958 | sub_82556958 | render_rotate_matrix3x3_compose_x_rotation | 0.91 | Sin/cos; updates matrix rows 0 and 2 (X-axis rotation). First step in render_build_rotation_matrix_euler_xyz; sub_821D7CA0/render_multiply_rotation_matrix_euler_xyz callee. |
| 0x82556898 | sub_82556898 | render_rotate_matrix3x3_compose_y_rotation | 0.91 | Sin/cos; updates rows 1 and 2 (Y-axis). Second step in render_build_rotation_matrix_euler_xyz. |
| 0x82556A10 | sub_82556A10 | render_rotate_matrix3x3_compose_z_rotation | 0.91 | Sin/cos; updates rows 0 and 1 (Z-axis). Third step in render_build_rotation_matrix_euler_xyz; sub_8234DDE0 callee. |
| 0x82556AC8 | sub_82556AC8 | render_rotate_matrix3x3_rows_about_x_by_angle | 0.90 | Per-row Y/Z spin at indices 1,2 (X-axis). Paired with render_rotate_matrix3x3_about_y_by_angle. sub_82350D10 callee. |
| 0x82556BB8 | sub_82556BB8 | render_rotate_matrix3x3_rows_about_z_by_angle | 0.90 | Per-row X/Y spin at indices 0,1 (Z-axis). sub_82350D10/sub_82355F78 callee. |
| 0x82556F78 | sub_82556F78 | render_multiply_rotation_matrix_euler_xyz | 0.92 | Chains compose X→Y→Z rotations without identity init. sub_821E2BC8 callee. |
| 0x825565F8 | sub_825565F8 | render_transform_vec3_array_by_half_angle_rotation | 0.89 | render_build_rotation_matrix_from_axis_half_angle then 3× vec3 transform to a1. sub_821D91C8/sub_82640900 callee. |
| 0x825566A8 | sub_825566A8 | render_transform_vec3_by_half_angle_rotation | 0.89 | Same half-angle matrix build; single vec3 a2→a1 output. sub_8263D900/sub_82640900 callee. |
| 0x82556E20 | sub_82556E20 | render_extract_pitch_yaw_from_forward_vmx | 0.88 | VMX normalize forward; atan2-style outputs to a2/a3/a4 via FM2_FMOD_SelectSignOrMagnitude. sub_821F88F0 callee (2×). |
| 0x82556FD8 | sub_82556FD8 | render_apply_axis_angle_to_matrix_vmx | 0.87 | Normalizes axis at a1+32; VMX vmaddfp applies axis-angle rotation to matrix rows with translation vector a9-a11. sub_821F88F0 callee. |
| 0x82557250 | sub_82557250 | render_adjust_angle_toward_target_wrapped | 0.90 | Wraps delta to (-π,π]; nudges *a3 toward target angle a1 by ±2π when outside range. sub_821F88F0 callee (3×). |
| 0x82557110 | sub_82557110 | render_apply_camera_axis_rotation_vmx | 0.86 | VMX normalize + axis-angle vmaddfp on matrix at a1+32 using angle a23. sub_821F88F0 callee. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx store; no new context. |
| 0x82557578 | sub_82557578 | VMX matrix helper fragment; needs pairing with 82557AA8 int16 path. |
| 0x82557AA8 | sub_82557AA8 | Int16×scale→float quaternion/matrix path; decompiler warns local alloc failed—defer. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no observable side effects. |
