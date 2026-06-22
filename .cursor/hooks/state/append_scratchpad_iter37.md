
## Iteration 37

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8255B610 | sub_8255B610 | render_car_tire_contact_shadow_params_init | 0.90 | Copies tire dims from pass+832–844 to out+32; converts a2×0.001/a4×0.0254 into shadow contact scalars at out+16/+20/+24/+28 using flt_8299E98C blend. render_car_instance_init_model_and_livery_resources callee. |
| 0x8255B6D0 | sub_8255B6D0 | render_command_buffer_ring_push_entry | 0.89 | 512-entry ring at pass+25396: writes 48-byte record (VMX128 at +832, float +848, flags +860–862); wraps index. sub_8251ADB0 pushes when pass[851] active. |
| 0x8255B7E8 | sub_8255B7E8 | hash_string_nibble_fold | 0.91 | Rolling hash v=16*v+c with nibble fold when (v&0xF000000). sub_821FFC60 stores hash of sprintf "Car%d" at object+244. |
| 0x8255D060 | sub_8255D060 | render_pass_apply_tint_and_texture_constants | 0.90 | FM2_Render_MatchShaderPassKeyword "TintColor" → FM2_ShaderConstant_SetVectorById at pass+800; binds texture constants 60/308 from +864/+868 then enables. Render-pass material bind path. |
| 0x8255D118 | sub_8255D118 | render_pass_apply_caliper_tint_constant | 0.91 | Lazy-cache keyword "caliperTintColor" at pass+836; FM2_ShaderConstant_SetVectorById(pass+800); sets constant 56=0 via vtable+24. |
| 0x8255FDC8 | sub_8255FDC8 | render_car_map_part_name_to_wheel_slot | 0.92 | Maps part name substrings: hood→1, mirror→2, wing→5, glassF/LF/…→8, etc. render_car_apply_livery_paint_by_wheel_slot (825601A0) callee. |
| 0x8255C350 | sub_8255C350 | render_primitive_decl_load_xml | 0.90 | XML: Name string, optional PrimitiveType enum, Index via sub_825A27D8. sub_8255C7B0 vtable prelude then this loader. |
| 0x8255C6A8 | sub_8255C6A8 | render_shader_decl_find_entry_by_name | 0.91 | Linear search 20-byte records in [begin,end); sub_8224DB30 name compare. render_shader_decl_set_float4_by_pair_names callee. |
| 0x8255C7F8 | sub_8255C7F8 | render_shader_decl_lookup_with_packed_suffix | 0.89 | If decl+32 flag: append "_packed" to lookup name then sub_8224DB90. Packed/unpacked body material path. |
| 0x8255D188 | sub_8255D188 | render_shader_decl_set_float4_by_pair_names | 0.90 | sub_8224DB90(a2,a3) then find entry a4; writes 4 floats to result+1..+4. Used for PaintColor/GlassColor livery apply. |
| 0x8255F3C8 | sub_8255F3C8 | render_object_pass_reset_livery_overlay_state | 0.88 | For each child ptr in pass vector: zero float+336; clear bytes+253/+254. sub_8251F068 livery reset prelude. |
| 0x8255FAB8 | sub_8255FAB8 | render_object_pass_assign_visible_draw_indices | 0.90 | Walks pass children; if +252 and FM2_Render_ObjectPassShouldDrawVisible: assign sequential index at +340. FM2_Render_TestPassVisibilityVMXCore callee. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938 sets command-buffer flag bit. |
| 0x8255B5A0 | sub_8255B5A0 | Duplicate texture-constant bind via vtable+24; defer with pass-bind cluster. |
| 0x8255B7C4 | sub_8255B7C4 | Jump-table fragment; not standalone callable semantics. |
| 0x8255C298 | sub_8255C298 | Binary-stream dtor only; batch with tmodel_vertex_subresource cluster. |
| 0x8255C2F0 | sub_8255C2F0 | Pool-alloc vertex subresource; defer with 8255FC88 dtor pair. |
| 0x8255C768 | sub_8255C768 | One-line Version field reader. |
| 0x8255C7B0 | sub_8255C7B0 | Vtable call + render_primitive_decl_load_xml wrapper. |
| 0x8255D200 | sub_8255D200 | Single-float variant of 8255D188; batch next pass. |
| 0x825601A0 | sub_825601A0 | Large livery paint applier; needs full read (PaintColor/GlassColor paths). |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no observable side effects. |
