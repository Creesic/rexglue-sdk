
## Iteration 34

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82557578 | sub_82557578 | render_vmx_build_rotation_matrix_from_quaternion | 0.88 | VMX vnmsubfp/vmaddfp builds 3×3 from quaternion vec4 (a40) and 3 basis vectors; stvx 16-byte matrix. sub_821F88F0 camera path (2×). |
| 0x82557AA8 | sub_82557AA8 | render_build_matrix3x3_from_int16_quaternion | 0.90 | Scales 4×int16 by 1/32768; quaternion normalize via fsel+fsqrts; FM2_Math_VmxPackMatrixRows → 3×3 floats at a1. sub_821F88F0/sub_8229F288 callees. |
| 0x82557700 | sub_82557700 | physics_interp_float_curve_segment | 0.89 | Piecewise linear interp on float table a1 with count a2; fsel picks segment; lerp between *(a1+4*i) samples. sub_82643DF0 car-dynamics grip curve (2×). |
| 0x825577B0 | sub_825577B0 | audio_denormalize_signed_int_to_double | 0.92 | Returns (int/a2) * scale where int masked to (2^(bits-1)-1) signed max. sub_821F8150/sub_821F9A80/sub_825D4FE8 callees. |
| 0x825577F0 | sub_825577F0 | audio_denormalize_unsigned_int_to_double | 0.92 | Returns (uint/a2) * scale with mask (2^bits-1). sub_821F8150 converts int16 PCM fields to float. |
| 0x82557828 | sub_82557828 | audio_quantize_double_to_signed_int16 | 0.91 | (int)((2^(bits-1)-1) * (a1/a2)). sub_825D4FE8 serializes float params to int16 XML fields. |
| 0x82557868 | sub_82557868 | audio_quantize_clamped_double_to_signed_int16 | 0.90 | fsel clamps a1 to [0,a2] then signed quantize. sub_821F7EE8 filter-coeff export path. |
| 0x82557C80 | sub_82557C80 | audio_biquad_init_state | 0.91 | Init 28-byte biquad block: clamp a2/a3≥0 at +0/+4; +8=1/a4; zeros +12/+16/+20; mode byte +24. sub_82499268 XML loader. |
| 0x82557CF8 | sub_82557CF8 | audio_biquad_set_frequency_squared_coeff | 0.92 | *result = (2πf)² / result[2] with f clamped ≥0. sub_82495960 tire-rumble filter setup (freq×6.2831855). |
| 0x82557DE8 | sub_82557DE8 | audio_biquad_set_damping_coeff | 0.91 | result[1]=sqrt(result[2]*result[0])*2/result[2]*damping. Pairs with frequency coeff in sub_82495960. |
| 0x82557D30 | sub_82557D30 | audio_biquad_process_sample | 0.90 | Direct-form biquad step; updates state +12/+16; returns output sample. Mode byte +24 selects branch. sub_82495960 advances filter 2×/frame. |
| 0x82557C08 | sub_82557C08 | render_vmx_lerp_vec4_clamped | 0.89 | VMX vsubfp distance ratio clamped [0,1]; vmaddfp lerp v1→v2. sub_824CA128 crowd-excitement position blend. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825578B8 | sub_825578B8 | Stack-return unsigned quantize pair; batch with 825578F0 next pass. |
| 0x825578F0 | sub_825578F0 | Stack-return clamped unsigned quantize pair; thin wrapper variant of 82557868. |
| 0x82557CE8 | sub_82557CE8 | One-line setter *(a1+12); defer with biquad getter/setter batch. |
| 0x82557CF0 | sub_82557CF0 | One-line setter *(a1+20); defer with biquad getter/setter batch. |
| 0x82557D20 | sub_82557D20 | Getter *(a1+12); trivial accessor. |
| 0x82557D28 | sub_82557D28 | Getter *(a1+16); trivial accessor. |
| 0x82557E28 | sub_82557E28 | Pose/bone-chain copy with VMX128; needs struct layout confirmation. |
| 0x82557EA0 | sub_82557EA0 | Init 3 pointer chain (+32/+64/+96); needs pose struct naming context. |
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx store wrapper. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no observable side effects. |
