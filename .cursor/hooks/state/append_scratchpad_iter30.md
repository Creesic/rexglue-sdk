
## Iteration 30

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82554F48 | sub_82554F48 | collision_audio_ctor | 0.93 | Sets CCollisionAudio vftable; FM2_AsyncQueue_InitSentinelListHead at +8; presentation_timeline_clear_all_nodes; init 3 embedded 16-byte vectors via stl_vector_16byte_clear; defaults +68=250 +72=1.0. Reg 0x82190F48; sub_82555210 callee. |
| 0x82554FE0 | sub_82554FE0 | collision_audio_destroy_members | 0.92 | Clears timeline + 3 vectors + 3 embedded element groups (sub_825549A0); presentation_timeline_clear_all_nodes; frees list head; FM2_Object_AssignBaseVtable_82000E18. Vtable 0x82190F50; collision_audio_refcounted_dtor/dtor callees. |
| 0x82555210 | sub_82555210 | collision_audio_create_and_register_with_render | 0.91 | Pool-alloc 84 bytes; collision_audio_ctor; TRefCountedObject<CCollisionAudio> vftable; FM2_ComPtr_ResetAndAssign + FM2_Render_NotifyManagerStateChange. sub_821D67A8 caller. |
| 0x825550D8 | sub_825550D8 | collision_audio_refcounted_dtor | 0.94 | TRefCountedObject<CCollisionAudio> vftable 0x82046E8C; calls collision_audio_destroy_members; optional FM2_Memory_FreeSmallBlockOrNull. Reg 0x82190F60. |
| 0x825552B0 | sub_825552B0 | collision_audio_dtor | 0.93 | CCollisionAudio vftable 0x82046E5C; collision_audio_destroy_members + optional free. Reg 0x82190F78. |
| 0x82555080 | sub_82555080 | collision_audio_reset | 0.90 | Clears 3 stl_vector_16byte_clear slots at +20/+36/+52 then presentation_timeline_clear_all_nodes. Vtable 0x82046E74/0x82046EA4; reg 0x82190F58. |
| 0x82554898 | sub_82554898 | stl_vector_16byte_move_assign_range | 0.91 | For [a1,a2) 16-byte elements: stl_vector_56byte_assign_from on embedded payload at dest offset. stl_vector_16byte_erase_suffix_destroy helper. |
| 0x825549A0 | sub_825549A0 | stl_vector_16byte_clear_and_destroy_elements | 0.92 | Walks [begin,end) 16-byte records; sub_825A5FC8 destroy each; frees buffer; zeros begin/end/cap. collision_audio_destroy_members callee. |
| 0x825549F8 | sub_825549F8 | stl_vector_16byte_uninitialized_fill_n_copies | 0.90 | Loop a3 times: stl_vector_56byte_copy_construct_from at each 16-byte slot. stl_vector_16byte_insert_n_at callee (2 xrefs). |
| 0x82554A50 | sub_82554A50 | stl_vector_16byte_erase_suffix_destroy | 0.91 | stl_vector_16byte_move_assign_range to shrink; sub_825A5FC8 destroys erased tail; updates end pointer. stl_vector_16byte_clear core. |
| 0x82554C00 | sub_82554C00 | stl_vector_16byte_clear | 0.92 | Erases full [begin,end) via stl_vector_16byte_erase_suffix_destroy. collision_audio_ctor/reset/destroy paths. |
| 0x82555130 | sub_82555130 | stl_vector_16byte_emplace_back | 0.90 | stl_vector_16byte_insert_n_at at end for 1 element; returns iterator pair in a1. sub_82555300 slow-path callee. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82555300 | sub_82555300 | stl_vector_16byte_push_back; defer with 825553B8/825554E0 collision-audio XML loader cluster. |
| 0x825A5FC8 | sub_825A5FC8 | 16-byte element destroy (embedded 56-byte vector); batch with 825545C0/82554AF0 reserve helpers next pass. |
| 0x825532D0 | sub_825532D0 | COM triplet release; caller sub_82553AD0 still unanalyzed. |
| 0x82553488 | sub_82553488 | Glass/WindshieldSmash render; thin context. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no side effects. |
