
## Iteration 32

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82555F50 | sub_82555F50 | render_copy_matrix4x4_affine_zero_translation | 0.94 | Copies 3×3 from a2 with w=0 columns, translation zero, w=1. Pair of render_copy_matrix4x4_affine_with_translation. sub_82497820 callee. |
| 0x82555C30 | sub_82555C30 | presentation_apply_car_camera_vmx_transform | 0.90 | FM2_MemcpyAligned camera params; FM2_Render_MatrixMultiplyVMX128_From16Byte; VMX transpose pack; FM2_Presentation_ApplyCarCameraVMXBodyB; stores result VMX128 to out. sub_8264DA40 callee. |
| 0x82555BF0 | sub_82555BF0 | render_wrap_angle_double_to_pm_pi | 0.93 | Double-precision wrap to (-π,π] via ±2π. Scalar variant paired with render_wrap_angle_to_pm_pi. sub_8235D4E0 callee. |
| 0x82556030 | sub_82556030 | render_transpose_matrix3x3_from_column_major | 0.92 | Reads a2[0,4,8,1,5,9,2,6,10] into row-major 3×3 output. sub_821D91C8/sub_821DE4D0 render callers. |
| 0x825562F8 | sub_825562F8 | render_build_rotation_matrix_from_axis_half_angle | 0.91 | FM2_Render_ComputeSinCosForPassLighting(angle*0.5); Rodrigues/quaternion expansion into 3×3 at a1[0,5,10,1,4,8,2,6,9]. render_build_rotation_matrix_from_axis_angle callee. |
| 0x825563F0 | sub_825563F0 | render_build_rotation_matrix_from_axis_angle | 0.92 | VMX vmsum3fp normalize axis if length≥1e-12 else identity; else render_build_rotation_matrix_from_axis_half_angle. sub_82647838/sub_82556488 callee. |
| 0x82556488 | sub_82556488 | render_transform_vec3_array_by_matrix3x3 | 0.90 | render_build_rotation_matrix_from_axis_angle then 3× matrix3×vec3 row transforms writing to a1. sub_8264E5C0 callee. |
| 0x82556538 | sub_82556538 | render_transform_vec3_by_matrix3x3 | 0.90 | Same matrix build; transforms single input vec3 at a2 into output a1. sub_826459D0 callee. |
| 0x82556768 | sub_82556768 | render_transform_vec3_array_by_rotation_and_diff | 0.88 | render_build_rotation_matrix_from_axis_half_angle; transforms 3 vec3s; VMX vsubfp/vaddfp writes diff back to a2. sub_82640900 callee. |
| 0x82556C30 | sub_82556C30 | render_vmx_build_orthonormal_basis_from_direction | 0.89 | VMX normalize input dir at result+32; builds orthogonal tangent/bitangent at +16/+32 (fallback axis if length≤1e-5). sub_821EC670/sub_8222A2D8 callees. |
| 0x82556D90 | sub_82556D90 | render_build_rotation_matrix_euler_xyz | 0.91 | Init identity 3×3; sub_82556958/82556898/82556A10 compose X/Y/Z axis rotations from three angles. sub_821F88F0 callee. |
| 0x82556B40 | sub_82556B40 | render_rotate_matrix3x3_about_y_by_angle | 0.90 | FM2_Render_ComputeSinCosForPassLighting; rotates x/z components per row (Y-axis spin). sub_821D7DE8/sub_82350D10 callees. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Thin VMX128 store wrapper only; insufficient standalone semantics. |
| 0x825565F8 | sub_825565F8 | Variant matrix×vec3 batch; defer with 825566A8 cluster. |
| 0x82556AC8 | sub_82556AC8 | Y/Z row rotation variant; batch with 82556BB8 Z-axis next pass. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no observable side effects. |
