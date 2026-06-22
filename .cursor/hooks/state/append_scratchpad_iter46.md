
## Iteration 46

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x825731B8 | sub_825731B8 | render_shader_resource_load_all_registered_if_ready | 0.88 | Critsec stru_82A00C6C; iterates dword_82A00C8C vector; FM2_ShaderResource_LoadIfReady per entry. Pair to render_shader_resource_load_all_registered_pending. |
| 0x82573A10 | sub_82573A10 | render_shader_resource_core_dtor_with_free | 0.91 | render_shader_resource_dtor_release_mesh_and_name without derived vtable pre-set; optional FM2_Memory_FreeSmallBlockOrNull. |
| 0x82571A88 | sub_82571A88 | render_config_entry_buffer_dtor | 0.89 | Frees heap buffer at a1[2]; clears capacity fields at +8..+16; FM2_Object_AssignBaseVtable_82000E18. |
| 0x82572680 | sub_82572680 | render_config_entry_buffer_dtor_with_free | 0.91 | vtable off_82048AD4; render_config_entry_buffer_dtor + optional free. |
| 0x82572C50 | sub_82572C50 | render_binary_stream_read_scope_dtor_with_free | 0.91 | vtable off_82048BC4; FM2_BinaryStream_DtorReadScope; optional heap free. Used by skinned mesh stream path. |
| 0x82572830 | sub_82572830 | render_post_process_draw_helper_base_dtor_with_free | 0.90 | FM2_Render_HelperB3E8ResetState at +16; base vtable reset; optional free. Pre-derived draw-helper teardown. |
| 0x82572E98 | sub_82572E98 | render_vector_shift_insert_32byte_range | 0.89 | FM2_MemcpyAligned 32-byte stride shift from [a1,a2) to make room at insert point; returns new end iterator. Called from render_vector_emplace_32byte_element. |
| 0x82573260 | sub_82573260 | render_vector_emplace_32byte_element | 0.88 | Validates iterator range; calls render_vector_shift_insert_32byte_range when insert != end; writes 32-byte element to output. Callers sub_825BEDB8, sub_825C44C0. |
| 0x82572E30 | sub_82572E30 | render_shader_resource_assign_from_registered_index | 0.87 | Bounds-check index against dword_82A00C8C vector; sub_824A1998(dest, registered_entry). Global shader-resource table lookup. |
| 0x82573488 | sub_82573488 | render_shader_resource_name_copy_from_entry | 0.90 | FM2_Stl_String_CopyAssign(dest, src+68). Copies shader-resource name field from entry object. |
| 0x82573C40 | sub_82573C40 | render_presentation_comobject_init | 0.88 | FM2_ComObject_InitBaseVtable423C0; vtable off_82048E18 with inner off_82048DF4. Base presentation COM object ctor. |
| 0x82573EC0 | sub_82573EC0 | render_model_presentation_object_init | 0.89 | render_presentation_comobject_init; sets vtable off_82048E5C and inner off_82048E38; clears +16. Caller sub_82577310. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82570B68 | sub_82570B68 | Atomic refcount dec at +192; no xrefs. |
| 0x825714C8 | sub_825714C8 | Atomic refcount dec at +1120; no callers. |
| 0x825719C8 | sub_825719C8 | Atomic refcount dec at +128; no callers. |
| 0x82570C30 | sub_82570C30 | Thin FM2_MemcpyAligned 24-byte copy thunk. |
| 0x82572180 | sub_82572180 | ForzaCmdLineList deferred flush; no callers. |
| 0x825721F0 | sub_825721F0 | Intrusive-list splice; no callers found. |
| 0x82572288 | sub_82572288 | Intrusive-list splice at +80 only; no callers. |
| 0x82573338 | sub_82573338 | String ctor "Skinned Model Resources" only. |
| 0x82573BE0 | sub_82573BE0 | vtable+84 under critsec; no callers. |
| 0x82573C98 | sub_82573C98 | Thin vtable+20 dispatch only. |
| 0x82573DB8 | sub_82573DB8 | 8-byte this-adjust thunk to sub_82573D50. |
| 0x82573FC0 | sub_82573FC0 | Thin vtable+24 invoke only. |
