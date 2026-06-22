
## Iteration 25

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82557F80 | sub_82557F80 | render_vmx_test_ray_segment_chain_front_facing | 0.88 | Up to 3 segment points; VMX cross/dot vs plane normal at +16; returns 0 if vmsum3fp<0. render_raycast_find_nearest_sphere_hit_vmx branch when a3<=0. |
| 0x82559340 | sub_82559340 | render_solve_quadratic_root_in_unit_interval | 0.90 | Discriminant a3²-4*a2*a4; quadratic formula; picks root in [0,1]. render_vmx_project_point_between_dual_spheres projection step. |
| 0x82550038 | sub_82550038 | lua_table_foreach | 0.91 | table+function args; ipairs-like walk calling metamethod `<` on key/value; returns on first truthy result. Registered in table lib off_820468F0 "foreach". |
| 0x8254FF78 | sub_8254FF78 | lua_table_foreachi | 0.91 | Numeric for i=1..getn; calls function(i, t[i]); stops on truthy return. Table lib "foreachi" entry. |
| 0x825500F8 | sub_825500F8 | lua_table_maxn | 0.90 | Iterates array part; tracks max numeric index with value type 3. Table lib "maxn". |
| 0x825501A0 | sub_825501A0 | lua_table_getn | 0.92 | Returns sub_8254D5E0 array length as number. Table lib "getn". |
| 0x825501F0 | sub_825501F0 | lua_table_setn | 0.89 | Sets array length via sub_8254D4E0 at index arg; returns table. Table lib "setn". |
| 0x82550250 | sub_82550250 | lua_table_insert | 0.92 | 2 or 3 args; shifts elements with sub_824B7498/sub_824B77D8; inserts at pos. Error "wrong number of arguments to 'insert'". |
| 0x82550338 | sub_82550338 | lua_table_remove | 0.91 | Removes index (default last); compacts tail; returns removed value. Table lib "remove". |
| 0x82550400 | sub_82550400 | lua_table_concat | 0.92 | Joins string table elements with optional sep/start/end; validates all strings. Table lib "concat". |
| 0x82550958 | sub_82550958 | lua_open_table_library | 0.90 | FM2_Lua_OpenLib "table" off_820468F0. |
| 0x82550990 | sub_82550990 | lua_string_len | 0.93 | Returns #str from sub_8254E2D0 length. String lib "len" off_82046A08. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops only; still no observable effects. |
| 0x825509D8 | sub_825509D8 | string.sub; deferred to string-lib completion batch (iter 26). |
