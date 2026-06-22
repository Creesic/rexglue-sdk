
## Iteration 44

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825722D8 | sub_825722D8 | render_post_process_pipeline_config_dtor | 0.89 | vtable off_8204884C; releases COM at +20/+88; FM2_ConfigEntryVector_FreeBuffer on four config vectors; FM2_Object_AssignBaseVtable_82000E18. |
| 0x825723C0 | sub_825723C0 | render_post_process_pipeline_config_dtor_with_free | 0.91 | render_post_process_pipeline_config_dtor + optional FM2_Memory_FreeSmallBlockOrNull. |
| 0x82572410 | sub_82572410 | render_post_process_draw_helper_init | 0.90 | vtable off_8204892C; FM2_Render_HelperB3E8DrawPathInit at +16; clears +2384/+2388. Ctor body for 2416-byte draw helper. |
| 0x82572A40 | sub_82572A40 | render_post_process_draw_helper_acquire_and_assign | 0.89 | Pool alloc 2416; render_post_process_draw_helper_init; vtable off_82048AFC; ComPtr assign at a1+32 via sub_825DA040; ring-buffer index return. |
| 0x82572770 | sub_82572770 | render_post_process_pipeline_node_dtor_with_free | 0.90 | vtable off_820489F4; render_post_process_pipeline_config_dtor; optional heap free. |
| 0x82572888 | sub_82572888 | render_post_process_pipeline_node_acquire_and_push | 0.88 | Pool alloc 96; vtable off_820489F4; zero-init fields; Vector_PushBack32 at a1+4; render_ring_buffer_get_current_index. |
| 0x82571A38 | sub_82571A38 | render_post_process_effect_descriptor_dtor | 0.90 | vtable off_82048828; FM2_Stl_String_InitOrClear at +16; base vtable reset. Pair to render_post_process_effect_descriptor_init. |
| 0x82571AE8 | sub_82571AE8 | render_post_process_effect_descriptor_dtor_with_free | 0.91 | render_post_process_effect_descriptor_dtor + optional free. |
| 0x825727C8 | sub_825727C8 | render_post_process_draw_helper_dtor_with_free | 0.90 | vtable off_82048AFC; FM2_Render_HelperB3E8ResetState at +16; optional free. |
| 0x82572D60 | sub_82572D60 | render_redirect_stream_ctor_from_source | 0.89 | FM2_RedirectStream_CtorFromSource; vtable off_82048C8C; AddRef source stream. Used by sub_825734C0 skin mesh load path. |
| 0x82572EF0 | sub_82572EF0 | render_shader_resource_assign_by_cstring_match | 0.88 | Under critsec stru_82A00C6C: iterates global dword_82A00C8C vector; FM2_BufFile_WriteCString name compare; SetInterfaceThreadSafe result. Caller render_shader_resource_get_or_create_by_name. |
| 0x82573A60 | sub_82573A60 | render_shader_resource_get_or_create_by_name | 0.89 | Critsec guarded lookup via render_shader_resource_assign_by_cstring_match; on miss pool-alloc 192, vtable off_82048D54, FM2_Stl_String_AssignRange from a2; push to dword_82A00C88 pending-load vector; sets +49 ready flag. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82570B68 | sub_82570B68 | Atomic refcount dec at +192; no xrefs. |
| 0x825710C0 | sub_825710C0 | Thin deleting dtor off_82048754 only. |
| 0x82571118 | sub_82571118 | Thin pool-alloc wrapper around car_slot_transform_init. |
| 0x82571160 | sub_82571160 | Thin ComPtr assign off_82048754 stub object. |
| 0x825714C8 | sub_825714C8 | Atomic refcount dec at +1120; no callers. |
| 0x825719C8 | sub_825719C8 | Atomic refcount dec at +128; no callers. |
| 0x82570C30 | sub_82570C30 | Thin FM2_MemcpyAligned 24-byte copy thunk. |
| 0x82571A88 | sub_82571A88 | Config-entry buffer dtor; defer with off_82048AD4 deleting dtor pair. |
| 0x82572830 | sub_82572830 | Thin draw-helper base dtor without derived vtable set. |
| 0x82572180 | sub_82572180 | Deferred ForzaCmdLineList flush; no callers found. |
| 0x82572C00 | sub_82572C00 | Refcount dec at +28; no callers. |
