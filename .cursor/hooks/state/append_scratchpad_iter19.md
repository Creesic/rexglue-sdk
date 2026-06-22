
## Iteration 19

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8255A460 | sub_8255A460 | render_scene_material_vmx_init_identity_defaults | 0.89 | VMX lvlx/vrlimi128/stvx writes identity-ish floats (1.0 diagonals, 0.0 elsewhere) at material offsets +32/+64/+96/+176; byte flags cleared. render_scene_object_instance_ctor callee—not Lua reader. |
| 0x8255A1D8 | sub_8255A1D8 | lua_read_count_and_alloc_dword_buffer | 0.90 | Lua +116 read into a1+1 count; if count>0 FM2_AllocPoolAcquireOrInit_Thunk(4*count) into *a1; +32 advance. lua_read_car_livery_draw_pair_ptr_array helper. |
| 0x8255A270 | sub_8255A270 | lua_read_dword_pair_array_from_table | 0.89 | Reads count via +116; loops count times calling Lua +136 on v8 and v8+2 (dword pairs). Second livery draw-pair reader path. |
| 0x8255AB80 | sub_8255AB80 | render_global_car_attributes_parse_xml_sections | 0.88 | Large XML attribute loader: GeomBlendStart/End, MaxRPM, Dirt, Driver, WheelScale/Offset/Opacity, BlurRim, etc. Called from render_load_global_car_attributes_if_needed with "GlobalCarAtttributes". |
| 0x8256D9E8 | sub_8256D9E8 | global_car_attributes_xml_reader_ctor | 0.87 | FM2_Render_NotifyManagerStateChange + sub_822072B0 base init; vtable off_8204837C; retains filesystem ptr; field +23 cleared. Global car XML load path. |
| 0x8256BB98 | sub_8256BB98 | lua_read_car_part_container_and_draw_pairs | 0.88 | If Lua table depth>=4 reads draw pairs via lua_read_car_livery_draw_pair_ptr_array at +20; lua_read_named_part_ptr_container on +4 field "Part". |
| 0x825ABB38 | sub_825ABB38 | serialize_car_part_container_and_draw_pairs | 0.88 | Mirror of 8256BB98 for serialize path: draw pairs + serialize_named_part_ptr_container("Part"). FM2_TModel_SerializeMinMaxBounds caller. |
| 0x8256B5D0 | sub_8256B5D0 | lua_read_named_part_ptr_container | 0.90 | snprintf "%s_Container"; read/write count; resize vector sub_82619680; per element sub_8256ABA0 alloc material slot 400 bytes. |
| 0x825AB8B0 | sub_825AB8B0 | serialize_named_part_ptr_container | 0.90 | Same container pattern as 8256B5D0 but serialize branch uses sub_825AB6F8 (FM2_TModel_InitVMXBounds 128-byte objects). |
| 0x8251FE58 | sub_8251FE58 | render_car_draw_path_copy_state_from_source | 0.89 | Deep copy car draw-path block: strings, intrusive list splice, resource locks, 9×144-byte wheel slot copies via sub_8251DE40, VMX128 blocks, livery vectors. Shared by DE90/BAD8. |
| 0x8256BC30 | sub_8256BC30 | render_car_instance_acquire_or_create_in_critsec | 0.87 | Critsec stru_82A00C30; optional draw traversal; pool 36640 + sub_8256B7B0 ctor; SetInterfaceThreadSafe; push dword_82A00C4C. render_car_instance_apply_draw_path caller. |
| 0x8256DE90 | sub_8256DE90 | render_car_instance_apply_draw_path_and_load_resources | 0.86 | Frame allocator bind; FM2_Render_HelperB3E8DrawPathTail; copy path state; wheel slots sub_8256DA68×9; render_object_pass_apply_livery_material_mask_from_xml; wheel/brake texture load; shader FM2_ShaderResource_LoadIfReady. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x8256E588 | sub_8256E588 | 4KB car-init orchestrator; tail shows livery mask + wheel slots but body mostly truncated. |
| 0x8256DA68 | sub_8256DA68 | Wheel-slot livery apply helper; critsec + sub_8255B788 jump table—needs full read. |
| 0x82565E88 | sub_82565E88 | Loads game:\\media\\wheels\\...\\wheel.xds + tire xds via FM2_LiveryMask_ParseAndLoadEntry; defer with E588 cluster. |
| 0x8256ABA0 | sub_8256ABA0 | Thin lua alloc+vtable dispatch to render_object_pass_material_slot_init; batch with B5D0 next pass. |
| 0x82558E70 | sub_82558E70 | FP geometry/math kernel (8 doubles in); no domain strings—insufficient naming context. |
