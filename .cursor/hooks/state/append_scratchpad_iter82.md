## Iteration 82

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8249BF40 | sub_8249BF40 | car_audio_deferred_enqueue_init_params | 0.91 | Alloc 0xD4 CParams1ICarAudioInit; car_audio_init_params_ctor; deferred pool bump alloc. Vtable off_8203F130/178. |
| 0x8249BE40 | sub_8249BE40 | car_audio_init_params_ctor | 0.92 | Sets CCarAudioDeferred::CParams1ICarAudioInit::vftable; car_audio_init_params_copy_construct; car_audio_clear_owned_strings. |
| 0x8249B280 | sub_8249B280 | car_audio_init_params_copy_construct | 0.90 | Inits strings; car_audio_init_params_assign_from_car_audio from source car-audio object. |
| 0x8249B100 | sub_8249B100 | car_audio_init_params_assign_from_car_audio | 0.91 | Copies vtable ptr, strings, dword fields, byte flags, string vector at +104 from live CCarAudio. |
| 0x82495120 | sub_82495120 | car_audio_deferred_enqueue_mute_params | 0.90 | Alloc 8-byte deferred task; off_8203F14C (CParams1ICarAudioMute); stores mute byte at +4. |
| 0x824951F0 | sub_824951F0 | car_audio_deferred_enqueue_update_params | 0.89 | Alloc 0x200 deferred task; off_8203F154 (CParams2ICarAudioUpdate); FM2_MemcpyAligned state blob from caller stack. |
| 0x82495540 | sub_82495540 | render_frame_counter_link_scalar_dtor | 0.91 | Restores off_8203F164/off_8203F15C; render_frame_link_base_dtor; optional heap free. Vtable 0x8203F164. |
| 0x82496090 | sub_82496090 | render_frame_link_scalar_dtor | 0.90 | render_frame_link_base_dtor; optional FM2_Memory_FreeSmallBlockOrNull. Vtable 0x8203F11C. |
| 0x82680A38 | sub_82680A38 | fmod_channel_enqueue_dsp_command | 0.90 | Critical-section enqueue to system command list; opcode +24=1; stores command arg at node+16. Tail of fmod_channel_handle_enqueue_dsp_command. |
| 0x82680B28 | sub_82680B28 | fmod_channel_enqueue_channel_group_command | 0.90 | Enqueue when a2||a3; opcode +24=2/3/4 from mute/group flags. Dispatched in fmod_system_flush_pending_channel_commands cases 2-4. |
| 0x8266F0C8 | sub_8266F0C8 | fmod_channel_handle_enqueue_channel_group_command | 0.91 | resolve handle then fmod_channel_enqueue_channel_group_command(channel, a2, a3). |
| 0x8267A198 | sub_8267A198 | fmod_system_flush_pending_channel_commands | 0.90 | Drains command list at system+16496; switch on node+24: case0 sub_82681B20, case1 sub_826813E0, cases2-4 channel-group ops. Called before enqueue when list busy. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x8257EFA0 | sub_8257EFA0 | Abstract off_82049CAC vtable with purecall slots only. |
| 0x8257F7E8 | sub_8257F7E8 | Thin adjustor thunk → audio_cue_basic_deferred_threadsafe_dtor. |
| 0x82580268 | sub_82580268 | Thin wrapper → init_params_dtor only. |
| 0x8257F8C8 | sub_8257F8C8 | Thin wrapper → play_params_dtor only. |
| 0x8257FAA0 | sub_8257FAA0 | Thin wrapper → stop_params_dtor only. |
| 0x82581948 | sub_82581948 | Shared scalar-delete thunk (CSkidMarksLink, CAudioThreadLink vtables). |
| 0x82582AA0 | sub_82582AA0 | Body of audio_manager_deferred_dtor; no separate rename needed. |
| 0x82586ED0 | sub_82586ED0 | CAudioEffect adjustor thunk only. |
| 0x82583A10 | sub_82583A10 | Single-byte zero store only; too trivial alone. |
| 0x82495868 | sub_82495868 | Adjustor scalar-dtor thunk at this-4 only. |
| 0x82495870 | sub_82495870 | Adjustor release thunk at this-4 only. |
| 0x8249AE00 | sub_8249AE00 | String-vector assign helper; defer iter 83. |
| 0x82681740 | sub_82681740 | Thunk to fmod_channel_enqueue_dsp_command only. |
| 0x82681748 | sub_82681748 | Thunk to fmod_channel_enqueue_channel_group_command only. |
| 0x82681770 | sub_82681770 | Thin glue fmod_channel_get_dsp_unit_by_index + set volume. |
| 0x82681250 | sub_82681250 | DSP unit lookup; defer iter 83 with 826813E0 cluster. |
| 0x826813E0 | sub_826813E0 | Channel-group command executor; defer iter 83. |
| 0x82681B20 | sub_82681B20 | Complex channel connect command; defer iter 83. |
| 0x8266F3C0 | sub_8266F3C0 | Thin vtable+52 wrapper; defer iter 83. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
