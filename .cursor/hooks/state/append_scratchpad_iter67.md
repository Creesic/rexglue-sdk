## Iteration 67

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82727ED8 | sub_82727ED8 | render_skinned_mesh_get_vertex_data_base_ptr | 0.91 | `*a2 = *(result+108)`; first query in dispatch_animation_callbacks buffer walk. |
| 0x82727F60 | sub_82727F60 | render_skinned_mesh_get_index_buffer_ptr | 0.90 | `*a2 = 48*bone_count(+42) + base(+108)`; index buffer offset for skinned mesh. |
| 0x82727EE8 | sub_82727EE8 | render_skinned_mesh_get_vertex_stream_offset_ptr | 0.90 | `*a2 = 4*(12*bones+counts34/36/38) + base`; skinning vertex stream offset. |
| 0x82727F20 | sub_82727F20 | render_skinned_mesh_get_vertex_buffer_end_ptr | 0.90 | Same vertex sum plus `(bones+7)>>3` bitfield tail; end of packed vertex buffer. |
| 0x82728140 | sub_82728140 | render_skinned_mesh_get_active_vertex_count | 0.88 | If +124 set, clamp bone count to sub_827281E8 max else sub_82728218 path; writes count to *a2 and skin table ptr to *a3. Vtable 0x8219CA88. |
| 0x82727ED0 | sub_82727ED0 | render_skinned_mesh_set_vertex_data_base_ptr | 0.91 | Stores a2 at binding body +108; paired with getters; called from skinned_model_setup. |
| 0x827C4228 | sub_827C4228 | render_skinned_model_get_material_decl_frame_count | 0.89 | If resource+40 non-null return ushort at resource+34 else 0; used when a4 set in animation eval. |
| 0x827C3D20 | sub_827C3D20 | render_skinned_model_get_mesh_frame_count | 0.90 | Returns `*(ushort*)(*a1+34)` mesh frame count for animation sampling. |
| 0x827CB838 | sub_827CB838 | render_skinned_model_animation_get_channel_count | 0.91 | Returns `*(ushort*)(a1+20)` on timeline object; scales rate and output frame indices. |
| 0x827C5938 | sub_827C5938 | render_skinned_model_eval_animation_sample_time_and_frames | 0.87 | Frame count select; rate*channel scaling; phase via sub_8264EB08; writes sample time *a2 and up to 4 frame indices a3[] with wrap modes a1[40]. Vtable 0x821A25F8. |
| 0x8257D3D0 | sub_8257D3D0 | memory_free_xphysical_guarded_pool | 0.90 | Unlinks sentinel block from dword_82A00CE0 list; memset 0xDC guard; calls sub_82363688. Pair to memory_alloc_xphysical_guarded_pool. Vtable 0x82191F50. |
| 0x8257D510 | sub_8257D510 | memory_free_small_block_guarded_pool | 0.90 | Unlinks dword_82A00CE4 sentinel; memset 0xDC; FM2_Memory_FreeSmallBlock. Pair to memory_alloc_small_block_guarded_pool. Vtable 0x82191F60. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x8257D200 | sub_8257D200 | Thin wrapper around hash_name_set_from_cstr_and_hash. |
| 0x8257D618 | sub_8257D618 | FMOD capture path; defer audio vtable cluster iter 68. |
| 0x8257D668 | sub_8257D668 | FMOD context destroy; defer with D700+ cluster. |
| 0x8257D700 | sub_8257D700 | FMOD driver/channel init; defer. |
| 0x8257DA98 | sub_8257DA98 | "Reverb" DSP lookup; defer with D820/D980. |
| 0x827281E8 | sub_827281E8 | Returns ushort at +32 only; TLS/global context unclear. |
| 0x82728218 | sub_82728218 | Vertex influence mask builder; needs 8264EB08 naming first. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
