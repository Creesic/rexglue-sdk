
## Iteration 29

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82552EA0 | sub_82552EA0 | lua_math_rad | 0.94 | ToNumberStrict * 0.0174532925199433 (deg→rad). Math lib "rad" off_82046CD0. |
| 0x82552EE8 | sub_82552EE8 | lua_math_frexp | 0.95 | crt_frexp_decompose_double; pushes mantissa + exponent (2 returns). Math lib "frexp". |
| 0x82553020 | sub_82553020 | lua_math_max | 0.93 | Max over args 1..StackDepth via ToNumberStrict loop. Math lib "max". |
| 0x825531F0 | sub_825531F0 | lua_math_randomseed | 0.95 | FM2_Lua_MathTwoArgDispatch arg1 → srand(); returns 0. Math lib "randomseed". |
| 0x82553220 | sub_82553220 | lua_open_math_library | 0.92 | FM2_Lua_OpenLib "math" off_82046CD0; sets pi, huge, nan fields. Callers sub_824EDE68/sub_82620100. |
| 0x82552AC8 | sub_82552AC8 | lua_math_acos | 0.94 | ToNumberStrict + sub_824173C8 acos helper push. Math lib "acos". |
| 0x82552A88 | sub_82552A88 | lua_math_asin | 0.94 | ToNumberStrict + sub_82417300 asin helper push. Math lib "asin". |
| 0x82552B48 | sub_82552B48 | lua_math_atan2 | 0.93 | Two-arg ToNumberStrict + FM2_FMOD_SelectSignOrMagnitude (atan2) push. Math lib "atan2". |
| 0x82552B08 | sub_82552B08 | lua_math_atan | 0.94 | ToNumberStrict + sub_82413C80 atan push. Math lib "atan". |
| 0x82552BA8 | sub_82552BA8 | lua_math_ceil | 0.91 | ToNumberStrict + ceil helper push (decompiler labels FM2_AIDriver_ResetRaceLineStateClearSector). Math lib "ceil". |
| 0x82553828 | sub_82553828 | presentation_timeline_clear_all_nodes | 0.90 | Walks intrusive list at +4; timeline_node_release_attached_com_objects + FM2_Memory_FreeSmallBlockOrNull each node; clears count +8. sub_82554F48/sub_82554FE0 (CCollisionAudio timeline member). |
| 0x825535F0 | sub_825535F0 | presentation_timeline_set_nodes_enabled_flag | 0.88 | Stores enable byte at +4; walks node list; vtable+56 on COM refs at node+6/+7/+8 with flag. Pairs with presentation_timeline_update_nodes_to_time cluster. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825532D0 | sub_825532D0 | Clears/releases 3 COM slots on struct; caller sub_82553AD0 not yet analyzed—defer. |
| 0x82553488 | sub_82553488 | Glass/WindshieldSmash render path; needs string/vtable context beyond one call site. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no observable side effects. |
