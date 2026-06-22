## Iteration 84

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8249AE00 | sub_8249AE00 | car_audio_string_vector_assign | 0.91 | Assigns vector of 28-byte STL strings (7 dwords each) between car-audio init param blocks; grow/shrink via sub_8235AFE8. Called from car_audio_init_params_assign_from_car_audio. |
| 0x82680CB8 | sub_82680CB8 | fmod_channel_get_output_unit_count | 0.90 | Flush pending commands; returns channel+60 output-unit count under critical section. Pair to fmod_channel_get_dsp_unit_count at +56. |
| 0x82680940 | sub_82680940 | fmod_channel_alloc_delay_line_for_depth | 0.90 | Sets channel+18 depth; allocates per-depth delay buffer from fmod_dspi.cpp heap when missing; recurses output chain at unit+128. |
| 0x8269DAC0 | sub_8269DAC0 | fmod_dsp_unit_reset_state | 0.89 | Sets unit+140 volume to 1.0; zeroes coefficient buffers; clears byte +144. Called after fmod_dsp_connection_pool_alloc setup. |
| 0x8269DA10 | sub_8269DA10 | fmod_dsp_unit_init_connection_buffer_layout | 0.90 | Stores in/out channel counts +12/+16; partitions bump-allocated coefficient pointers across six bands. Called from connection pool alloc. |
| 0x82672398 | sub_82672398 | fmod_ascii_stricmp | 0.92 | Byte-wise strcmp loop returning signed char diff. Used to match FMOD parameter names like "Volume" and "Time offset". |
| 0x82672298 | sub_82672298 | fmod_strcpy | 0.93 | Copies null-terminated byte string. Used when building "FMOD Mixer unit" DSP descriptor strings. |
| 0x8266ED48 | sub_8266ED48 | fmod_system_play_sound_on_channel | 0.90 | Validate channel in global list; sub_8267B508 plays sound on existing channel object with priority/flags. |
| 0x8266ED98 | sub_8266ED98 | fmod_system_play_sound_on_channel_ex | 0.90 | Validate channel; sub_82679868 plays with optional FM2_FMOD_Event_LookupDescriptorById when a2==-2. |
| 0x8265A3F0 | sub_8265A3F0 | fmod_automation_evaluate_parameter_curve | 0.89 | Walks automation keyframe list at a1+32; interpolates float curve by time a2; writes result to a4. Used before parameter writes. |
| 0x8265AC98 | sub_8265AC98 | fmod_event_update_parameter_automation | 0.90 | Iterates event parameter automation nodes; bypass sync, curve eval, fmod_channel_handle_reset_parameter_automation, type-specific gain math, set_parameter_float. |
| 0x8266F3C0 | sub_8266F3C0 | fmod_channel_handle_reset_parameter_automation | 0.88 | resolve; channel vtable+52(param_index,0,0,0,0,0,0). Asm: r4=param index at automation node+16 before curve update. |

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
| 0x82681740 | j_fmod_channel_enqueue_dsp_command | Jump thunk only. |
| 0x82681748 | j_fmod_channel_enqueue_channel_group_command | Jump thunk only. |
| 0x8266FAA0 | sub_8266FAA0 | Overlaps F500 range getter; defer iter 85. |
| 0x8266FA08 | sub_8266FA08 | Thin wrapper channel vtable+20; defer iter 85. |
| 0x8267AF50 | sub_8267AF50 | DSP create from descriptor; defer iter 85. |
| 0x8267B508 | sub_8267B508 | Body of fmod_system_play_sound_on_channel; defer iter 85. |
| 0x82679868 | sub_82679868 | Body of fmod_system_play_sound_on_channel_ex; defer iter 85. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
