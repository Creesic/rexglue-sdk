
## Iteration 24

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82552200 | sub_82552200 | lua_string_append_quoted_with_escapes | 0.93 | Wraps Lua string in `"` via buffer helpers; escapes `\`, `"`, `\n`→`\`, `\r`→`\\r`, NUL→`="000`. lua_string_format callee for `%q`. |
| 0x82551DD8 | sub_82551DD8 | lua_string_gsub | 0.91 | Validates 2 string args; pushes 0 + FM2_FindAndReplaceDelimitedTextRange userdata closure. Registered 0x82190D58. |
| 0x82551E60 | sub_82551E60 | lua_gsub_expand_replacement_with_captures | 0.90 | Walks replacement string; `%d` uses FM2_FindAndReplaceDelimitedTextRangeTail capture slot; `%0` copies matched span; else literal append. gsub/gmatch helper; reg 0x82190D60. |
| 0x82551F50 | sub_82551F50 | lua_gmatch_apply_replacement_callback | 0.88 | Dispatches gmatch repl arg type 3/4/5/6 (string/function/table); validates replacement value type; calls lua_gsub_expand or metamethod path. lua_string_gmatch callee. |
| 0x825520A0 | sub_825520A0 | lua_string_gmatch | 0.91 | Pattern scan via FM2_FindAndReplaceDelimitedTextRangeBody; optional `^` anchor; max count; returns iterator string + match count. Registered 0x82190D70. |
| 0x82552368 | sub_82552368 | lua_string_format_parse_specifier | 0.92 | Parses `-+ #0` flags, width/precision digits, builds `%` prefix into temp; errors on repeated flags/oversize width. lua_string_format helper. |
| 0x825527E8 | sub_825527E8 | lua_open_string_library | 0.90 | FM2_Lua_OpenLib "string" off_82046A08; registers interned C closures from table at 0x82190D90. Called from sub_824EDE68/sub_82620100. |
| 0x825591C8 | sub_825591C8 | render_raycast_find_nearest_sphere_hit_vmx | 0.89 | Loops sphere records; vmsum3fp ray distance tests; keeps min t and hit VMX128; sub_82557F80 or render_ray_sphere_intersect_by_segment_distance branch. sub_8263B270 callee. |
| 0x82559508 | sub_82559508 | render_vmx_clip_point_to_dual_sphere_bounds | 0.88 | Packs VMX128 bounds from float args; render_vmx_project_point_between_dual_spheres; writes clipped VMX128 to out on success. sub_821DF6F8 callee. |
| 0x82559108 | sub_82559108 | render_ray_sphere_intersect_by_segment_distance | 0.90 | Dot ray dir with sphere normal; render_compute_point_to_segment_distance vs radius². Used by raycast + sub_82647838/sub_82654B00. |
| 0x82558E70 | sub_82558E70 | render_compute_point_to_segment_distance | 0.87 | FP8 geometry: closest approach of point to segment; returns fabs distance. Compared to radius² in render_ray_sphere_intersect_by_segment_distance. |
| 0x8255B318 | sub_8255B318 | render_vmx_compute_reflection_direction | 0.88 | FM2_D3D_ValidateResourceHandlesOrRecoverCoreB; if object size>=0x258 uses matrix constants unk_82045D50/60 else reads fields +25..+27; VMX vmaddfp reflection/dir to out. sub_82562348 callee. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82557F80 | sub_82557F80 | VMX 3-segment plane-facing test; pair with raycast cluster next pass. |
| 0x82559340 | sub_82559340 | Quadratic root solver for unit interval; helper to render_vmx_project_point_between_dual_spheres. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock wrapper; nested loops still have no visible side effects. |
