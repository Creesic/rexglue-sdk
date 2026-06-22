
## Iteration 42

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8256FDE8 | sub_8256FDE8 | render_post_process_upload_packed_shader_constants | 0.90 | VMX128 packs 24 rows of vec4 into 640-byte buffer; sub_82224188 hash lookup; ends with render-device vtable+148 upload (slot 4, 36 constants). Called from tonemap/material paths. |
| 0x825704B8 | sub_825704B8 | render_post_process_apply_tonemap_to_material | 0.89 | When material+2384 resource lock set: copies 64 bytes from +1808; adds exposure floats +372..+374; sets globals flt_8299EF50 to 0.6/0.2; calls upload_packed_shader_constants 4x on +468 blocks. Caller render_post_process_execute_draw_pass. |
| 0x82570608 | sub_82570608 | render_post_process_upload_material_shader_constants | 0.91 | Unpacks 7 qwords from material+64 and forwards to render_post_process_upload_packed_shader_constants. Secondary material list in execute_draw_pass. |
| 0x825716D0 | sub_825716D0 | render_post_process_bind_render_targets_from_locks | 0.88 | When child lock at +4 present: configures command buffer; binds textures from resource locks at +68,+100,+36,+4 via vtable+48/+40/+56/+104; sets pixel shader slot 4 from +276*4. Called from execute_draw_pass. |
| 0x82571850 | sub_82571850 | render_post_process_material_init_from_deferred_command | 0.90 | FM2_ResourceLock_AssignRetainedHandle; loads PS/VS id 5; shader_resource_load_static_decl_by_name "DefaultPresTrackVertexDecl"; sub_82578C10 on +144; FM2_DeferredCommand_DtorReleaseRef. |
| 0x82572AE8 | sub_82572AE8 | render_post_process_pass_acquire_and_assign | 0.88 | FM2_AllocPoolAcquireOrInit_Thunk(208); render_post_process_pass_init_defaults; vtable off_820485C4; assigns ComPtr at a1+48 via sub_825DA040; returns ring-buffer index. |
| 0x82571278 | sub_82571278 | render_post_process_material_dtor | 0.89 | vtable off_82048774; SetInterfaceThreadSafe clears locks at +4,+36,+68,+100; releases COM refs; FM2_Object_AssignBaseVtable_82000E18. |
| 0x82571538 | sub_82571538 | render_post_process_material_dtor_with_free | 0.91 | vtable off_820487B0; render_post_process_material_dtor + optional FM2_Memory_FreeSmallBlockOrNull. |
| 0x82571940 | sub_82571940 | render_post_process_effect_descriptor_init | 0.87 | vtable off_820487EC then off_82048828; FM2_Stl_String_InitOrClear at +16; clears field +128. Caller sub_825729B8. |
| 0x82570C80 | sub_82570C80 | render_com_object_release_child_and_reset_vtable | 0.90 | Releases COM object at a1[8] via vtable+8; FM2_Object_AssignBaseVtable_82000E18. Used by deleting dtors. |
| 0x82570D28 | sub_82570D28 | render_com_object_dtor_with_free | 0.91 | render_com_object_release_child_and_reset_vtable + optional heap free. |
| 0x82571430 | sub_82571430 | render_ring_buffer_get_current_index | 0.88 | Reads ring head at a1+8; sub_825286A0 advance-by-1; bounds-check against container at *v2; returns index. Used by sub_8223A678 and post-process acquire path. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs; owning type unknown. |
| 0x82570B68 | sub_82570B68 | Atomic refcount dec at +192; no xrefs. |
| 0x82570BD8 | sub_82570BD8 | Thin deleting dtor off_820485C4; defer with pass type cluster. |
| 0x82571068 | sub_82571068 | Thin deleting dtor off_820486FC; defer. |
| 0x825719C8 | sub_825719C8 | Atomic refcount dec at +128; no callers. |
| 0x82570C30 | sub_82570C30 | Thin FM2_MemcpyAligned 24-byte copy thunk. |
