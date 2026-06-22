## Iteration 90

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8268D370 | sub_8268D370 | fmod_system_fill_speaker_levels_from_dsp | 0.91 | Speaker-mode switch sizes row stride; mixer DSP vtable+8 fills matrix; FM2_MemcpyAligned into dest buffer; fmod_system_flush_pending_channel_commands + critical section. Called via adjustor from DSP connection pool. |
| 0x826738B8 | sub_826738B8 | fmod_dsp_parameter_list_init | 0.90 | Init circular list at a1; vtable off_82116730; calls fmod_dsp_parameter_state_init with dword_829A685C default block size. Used from ParamEQ ctors. |
| 0x82672810 | sub_82672810 | fmod_dsp_parameter_state_init | 0.91 | Zeroes 0x100-byte parameter scratch at +48; sets read/write cursors +356/+364/+376; stores block size a3. Core of fmod_dsp_parameter_list_init. |
| 0x82690668 | sub_82690668 | fmod_dsp_get_parameter_data_by_name | 0.92 | Plugin callback slot; forwards to fmod_dsp_read_parameter_data with element size 1. Registered in ParamEQ ctor at +1347. |
| 0x82690678 | sub_82690678 | fmod_dsp_set_parameter_cursor | 0.91 | Plugin callback; forwards to fmod_dsp_advance_parameter_cursor(a2,0). ParamEQ ctor at +1348. |
| 0x82690680 | sub_82690680 | fmod_dsp_effect_plugin_release_thunk | 0.90 | Adjustor thunk (-24) into sub_8268C490 codec/metadata release. ParamEQ ctor at +1349. |
| 0x826897A8 | sub_826897A8 | fmod_reverb_clear_geometry_query_list | 0.91 | Walks list at a1+16; frees nodes via fmod_reverb_accumulate_geometry_hit_samples cleanup path. Called at start of fmod_reverb_compute_occlusion_attenuation. |
| 0x82673198 | sub_82673198 | fmod_dsp_read_parameter_data | 0.93 | Name-based parameter read; ring-buffer bounds checks at +356/+364/+376; may invoke codec read callback; 194 xrefs across DSP plugins. |
| 0x82673588 | sub_82673588 | fmod_dsp_advance_parameter_cursor | 0.92 | Moves read cursor +364 by offset a2; mode a3 selects base (start/current/end); returns FMOD_ERR_NOTREADY (20) when unreadable. |
| 0x826881B8 | sub_826881B8 | fmod_reverb_accumulate_geometry_hit_samples | 0.90 | Processes geometry hit chain at +48; interpolates hit segments; accumulates occlusion energy into floats at +148+. Called when clearing query list. |
| 0x826892E8 | sub_826892E8 | fmod_reverb_cast_occlusion_ray_segment | 0.91 | Transforms ray segment by listener basis at a1+104; dispatches sub_826A0298 geometry ray cast; restores endpoints. Called from geometry_ray_hit_callback. |
| 0x82689750 | sub_82689750 | fmod_reverb_geometry_ray_hit_callback | 0.90 | Stores hit triangle pointer at a2+32; calls fmod_reverb_cast_occlusion_ray_segment. Passed to sub_826A0298 from cast path. |

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
| 0x82687320 | sub_82687320 | Thin wrapper → fmod_sound_stop_child_channels with fixed args only. |
| 0x8268D5D8 | sub_8268D5D8 | Adjustor thunk (-24) to fmod_system_fill_speaker_levels_from_dsp only. |
| 0x82673DE8 | sub_82673DE8 | Defer iter 91; high-res timer read helper. |
| 0x8267F8B0 | sub_8267F8B0 | Defer iter 91; profiler tick accumulator scale. |
| 0x8267FA18 | sub_8267FA18 | Defer iter 91; DSP unit post-mix dirty flag helper. |
| 0x8268D898 | sub_8268D898 | Defer iter 91; channel group idle voice probe. |
| 0x8268C490 | sub_8268C490 | Defer iter 91; codec/metadata release list helper. |
| 0x8268CDB8 | sub_8268CDB8 | Defer iter 91; metadata node alloc (fmod_metadata.cpp). |
| 0x826A0298 | sub_826A0298 | Defer iter 91; geometry ray cast dispatch wrapper. |
| 0x8269FC18 | sub_8269FC18 | Defer iter 91; 3D line intersection test for reverb geometry. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
