
## Iteration 45

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825734C0 | sub_825734C0 | render_skinned_model_load_mesh_from_stream | 0.90 | BinaryStream read scope; render_redirect_stream_ctor_from_source; vtable dispatch with "Skin"; sub_825A22F0 vertex buffer + D3DFMT_INDEX16 index buffer upload via FM2_D3D_ValidateResourceHandlesRecoverSlot; returns 1 when both GPU buffers created. |
| 0x825737B0 | sub_825737B0 | render_skinned_model_load_mesh_by_name | 0.89 | FM2_Stl_String_CtorFromCStr from a3; resource vtable+84 lookup; render_skinned_model_load_mesh_from_stream on hit. Caller render_skinned_model_load_mesh_from_resource_lock. |
| 0x825738D0 | sub_825738D0 | render_skinned_model_load_mesh_from_resource_lock | 0.88 | Extracts SSO string at +72; FM2_Render_NotifyManagerStateChange; forwards to render_skinned_model_load_mesh_by_name at object+96. |
| 0x825739B8 | sub_825739B8 | render_shader_resource_init_with_skinned_bundle | 0.89 | FM2_ResourceManager_InitBase; vtable off_82048DAC; FM2_Stl_String_InitOrClear at +68; render_skinned_model_draw_buffers_init at +96. Ctor from render_shader_resource_get_or_create_by_name. |
| 0x82573938 | sub_82573938 | render_skinned_model_draw_buffers_init | 0.88 | vtable off_82048D9C; zeroes vertex/index CPU buffer triplets; off_82048D4C draw-context stub; FM2_Outptr_WriteZero on +72/+84 outptrs. |
| 0x82573370 | sub_82573370 | render_skinned_model_clear_cpu_mesh_buffers | 0.90 | render_draw_context_clear_subobject_buffers at +28; frees heap block at +16 and clears size fields. Called from shader resource dtor path. |
| 0x825733C8 | sub_825733C8 | render_shader_resource_dtor_release_mesh_and_name | 0.89 | d3d_resource_ptr_release_and_clear on +168/+180; render_skinned_model_clear_cpu_mesh_buffers; FM2_Stl_String_InitOrClear at +68; FM2_PresentationCarConfig_Dtor. |
| 0x82573430 | sub_82573430 | render_shader_resource_dtor | 0.90 | vtable off_82048D54; extra D3D lock releases at +168/+180; chains render_shader_resource_dtor_release_mesh_and_name. |
| 0x82573880 | sub_82573880 | render_shader_resource_dtor_with_free | 0.91 | render_shader_resource_dtor + optional FM2_Memory_FreeSmallBlockOrNull. |
| 0x82573B50 | sub_82573B50 | render_shader_resource_load_by_name_normalized | 0.90 | FM2_Stl_String_AssignRange + FM2_BufFile_NormalizePathToLowercase; render_shader_resource_get_or_create_by_name; FM2_ShaderResource_LoadIfReady. Callers sub_8253C830, sub_8257BC88. |
| 0x82573018 | sub_82573018 | render_shader_resource_trigger_load_from_pending_queue | 0.87 | If entry+49 ready and +32 unset: under critsec finds entry in dword_82A00C8C, FM2_ResourceManager_AtomicIncPendingLoadCount, removes from pending via sub_8258AB18. Else direct load. |
| 0x82573118 | sub_82573118 | render_shader_resource_load_all_registered_pending | 0.88 | Critsec over dword_82A00C8C vector; FM2_ResourceManager_LoadAndWait + AtomicIncPendingLoadCount for each registered shader resource. |

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
| 0x82571118 | sub_82571118 | Thin pool-alloc wrapper. |
| 0x825714C8 | sub_825714C8 | Atomic refcount dec at +1120; no callers. |
| 0x825719C8 | sub_825719C8 | Atomic refcount dec at +128; no callers. |
| 0x82570C30 | sub_82570C30 | Thin FM2_MemcpyAligned 24-byte copy thunk. |
| 0x82571A88 | sub_82571A88 | Config-entry buffer dtor; defer with 82572680 pair. |
| 0x82572830 | sub_82572830 | Thin draw-helper base dtor without derived vtable. |
| 0x82572BC0 | sub_82572BC0 | Thin D3D lock release only; subsumed by dtor path. |
| 0x82572C50 | sub_82572C50 | Thin binary-stream deleting dtor off_82048BC4. |
| 0x82573338 | sub_82573338 | String ctor "Skinned Model Resources" only. |
| 0x82573488 | sub_82573488 | Thin FM2_Stl_String_CopyAssign from +68. |
| 0x82572180 | sub_82572180 | ForzaCmdLineList deferred flush; no callers. |
