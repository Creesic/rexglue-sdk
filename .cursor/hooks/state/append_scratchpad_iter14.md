
## Iteration 14

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8255A320 | sub_8255A320 | stl_string_find_last_substring_offset | 0.92 | Repeated strstr until last match; returns offset from string start or -1. Used to locate "_packed" suffix before strip in serialize_material_groups_without_packed_suffix. |
| 0x82564FC0 | sub_82564FC0 | stl_string_assign_substring_range | 0.90 | Ctor temp string from *a2; FM2_Stl_String_AssignRange(a1, temp, a3, a4); clears temp. Paired with find-last-substring for packed suffix removal. |
| 0x825693D0 | sub_825693D0 | render_scene_object_instance_dtor | 0.89 | Sets vtable off_82048014; frees draw queues via render_scene_object_free_draw_queue_slots; render_draw_context_slot_dtor x6; render_draw_queue_slot_array_release x2; clears 9 draw-path strings. |
| 0x825695E8 | sub_825695E8 | render_scene_object_alloc_draw_queue_slots | 0.88 | After render_scene_object_free_draw_queue_slots: allocates ten 36-byte nodes (vtable off_82047F4C, flag from +532); assigns ComPtr ref; calls sub_825AA988 per slot to build material tables. |
| 0x825697D8 | sub_825697D8 | render_scene_object_instance_init_subsystems | 0.87 | Chains sub_82567180 static decl init, render_scene_object_alloc_draw_queue_slots, inits four draw-context slots via vtable +8, wires parent pointers, sub_8257B2B0 env binding hookup. |
| 0x825698C0 | sub_825698C0 | render_filter_object_names_by_wildcard | 0.88 | Clears output vector; iterates object-name vector; sub_8245B640 wildcard match on STL string data; Vector_PushBack32 on hit. Livery/object lookup paths. |
| 0x82569988 | sub_82569988 | render_collect_livery_draw_pairs_for_lua | 0.87 | Walks ushort {objectIdx, materialIdx} pairs; badge/emblem filter; sub_8255D628/sub_8255F8E0 category checks; sub_82566440 visibility gate; FM2_Lua_BindingVector_AppendPair. |
| 0x82560A10 | sub_82560A10 | serialize_object_base_init_defaults | 0.89 | Vtable off_820477A8; clears name string; type=1; outptr zero; bytes 52-55 = 0xFF; field +48 = 4. Base for polymorphic serialize nodes. |
| 0x82560AD8 | sub_82560AD8 | serialize_polymorphic_object_node_ctor | 0.88 | Calls serialize_object_base_init_defaults; sets vtable off_820477B4; sub_825C18E0 on +14 subobject. Alloc path from serialize_polymorphic_object_ref. |
| 0x82568078 | sub_82568078 | render_scene_object_free_draw_queue_slots | 0.90 | Loop 10 slots at +34168: sub_8224DA10 cleanup then FM2_Memory_FreeSmallBlockOrNull; nulls pointer. Called from dtor and alloc prep. |
| 0x825670C0 | sub_825670C0 | render_draw_context_base_init_critsecs | 0.91 | Vtable off_820478AC; zeros fields +8..+28; FM2_CritSec_InitAndZeroOwner x3 at +32/+64/+96. Base of render_draw_context_slot_init_defaults. |
| 0x8255FD60 | sub_8255FD60 | render_draw_queue_slot_array_release | 0.87 | Releases four nested COM objects (-32..0 from +160) via vtable +8; then releases root *a1. Scene object dtor inner cleanup. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82567180 | sub_82567180 | 1.9KB CarStaticDecl/WheelStaticDecl builder; truncated mid-record loop—needs full read. |
| 0x82566440 | sub_82566440 | Strong wing/mirror/interior livery gate; deferred to batch with 8255F8E0/8255D628 next pass. |
| 0x8255F8E0 | sub_8255F8E0 | Complex livery category matcher; defer with 8255F868 material-id lookup. |
| 0x8245B640 | sub_8245B640 | Wildcard matcher (*/?); confident name but left for dedicated path-utils batch. |
| 0x825AA988 | sub_825AA988 | Material constant table builder; still truncated. |
| 0x82747C10 | sub_82747C10 | 4.4KB sparse constant-register applicator; still deferred. |
| 0x825AF3C0 | sub_825AF3C0 | Audio/render pass binder; truncated VMX setup. |
| 0x8255C868 | sub_8255C868 | 2KB VMX matrix transform; truncated decompile. |
