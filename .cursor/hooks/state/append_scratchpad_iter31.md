
## Iteration 31

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82555300 | sub_82555300 | stl_vector_16byte_push_back | 0.91 | Fast path: stl_vector_56byte_copy_construct_from at end if capacity; else stl_vector_16byte_emplace_back. sub_825553B8 callee. |
| 0x82554AF0 | sub_82554AF0 | stl_vector_16byte_reserve_n | 0.92 | FM2_Stl_ThrowLengthError if a2>0xFFFFFFF; realloc via FM2_TuningDb_AllocListNode + stl_vector_16byte_backward_shift_swap; destroy old via sub_825A5FC8 loop. collision_audio_init_empty_sound_grid. |
| 0x825545C0 | sub_825545C0 | stl_vector_56byte_reserve_n | 0.91 | sub_824ED008 alloc; move 56-byte elements; stl_vector_56byte_destroy_range on old buffer. Embedded in 16-byte vector elements; collision_audio_init_empty_sound_grid. |
| 0x825A5FC8 | sub_825A5FC8 | stl_vector_16byte_element_destroy | 0.93 | Clears embedded 56-byte vector at +4 via stl_vector_56byte_destroy_range + free; zeros begin/end/cap. Used by 16-byte vector erase/clear/reserve paths. |
| 0x825A5480 | sub_825A5480 | stl_vector_56byte_destroy_range | 0.92 | Destroys [result,a2) stepping 56 bytes via sub_825A4118. stl_vector_56byte_erase_suffix_from/assign/reserve callees. |
| 0x825553B8 | sub_825553B8 | collision_audio_init_empty_sound_grid | 0.90 | For 3 vectors at +20: stl_vector_16byte_reserve_n(a2); push a2 rows of empty STL string pairs via sub_824EE8E8. Reg 0x82190F88; collision_audio_load_collision_data_xml callee. |
| 0x825554E0 | sub_825554E0 | collision_audio_load_collision_data_xml | 0.91 | Loads Game:\\Media\\Audio\\Collisions\\CollisionData.xml; parses Surface1/Surface2/Impact3D/ImpactLFE/Scrape3D/MSBetweenDiffCollisions/Group/Event; calls collision_audio_init_empty_sound_grid. Vtable 0x82046E70/0x82046EA0; reg 0x82190F90. |
| 0x82553AD0 | sub_82553AD0 | collision_audio_dispatch_timeline_impact | 0.89 | Walks timeline at a1+8; VMX normalize velocities; sets COM params MomentumDiff/RelativeVelocity; collision_audio_impact_params_clear_com_refs on temp; timeline_node_release_attached_com_objects. Vtable 0x82046E78/0x82046EA8; reg 0x82190ED0. |
| 0x825532D0 | sub_825532D0 | collision_audio_impact_params_clear_com_refs | 0.90 | Zeros param fields; releases COM at +16/+20/+24 via vtable+8. collision_audio_dispatch_timeline_impact prologue; reg 0x82190E80. |
| 0x82553488 | sub_82553488 | render_windshield_glass_smash_draw | 0.88 | Creates Glass/Glass material from aWindshieldSmash[slot]; sets VMX128 uniforms; alpha from a1+72. Vtable 0x82046E84/0x82046EB4; reg 0x82190E90. |
| 0x82555B98 | sub_82555B98 | render_wrap_angle_to_pm_pi | 0.93 | Wraps float angle to (-π,π] using ±2π adjustments. sub_826459D0 callee. |
| 0x82555EC0 | sub_82555EC0 | render_copy_matrix4x4_affine_with_translation | 0.92 | Copies 3×3 from a2 into result with w=0 columns; sets translation from args a23-a25 and w=1. sub_821D7CA0/sub_821EA058 render callers. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x82555F50 | sub_82555F50 | render_copy_matrix4x4_affine zero-translation variant; defer with 82555C30 camera VMX batch. |
| 0x82555C30 | sub_82555C30 | Large VMX car-camera transform calling FM2_Presentation_ApplyCarCameraVMXBodyB; needs fuller read. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock counter loops; still no observable side effects. |
