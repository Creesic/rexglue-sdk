## Iteration 89

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82683D00 | sub_82683D00 | fmod_event_instance_set_multichannel_speaker_levels | 0.92 | Switch on system speaker mode at +17648; validates speaker index; clamps gains 0..1 into matrix at sound+50 via fmod_speaker_levels_pool_alloc_for_sound; child vtable+80. Called from startup/relink multichannel paths. |
| 0x82691468 | sub_82691468 | fmod_dsp_param_eq_multiband_ctor | 0.91 | fmod_plugin_factory_create_dsp_unit type 3; heap flag 0x100000; large layout (+1339 lists); vtable off_821170D0 near "FMOD ParamEQ" strings; callback slots 90668/90678/90680. |
| 0x82691550 | sub_82691550 | fmod_dsp_param_eq_ctor | 0.90 | Factory type 4; same ParamEQ vtable off_821170D0/CC; compact layout (+827 lists); same parameter callbacks as multiband variant. |
| 0x8268D2C0 | sub_8268D2C0 | fmod_channel_group_acquire_idle_channel | 0.89 | If mode bit 0x10 clear uses group+40 pool else +44; calls fmod_channel_pool_acquire_idle_channels with index -1. Used from fmod_system_prepare_sound_on_channel. |
| 0x8268DCA0 | sub_8268DCA0 | fmod_channel_pool_acquire_idle_channels | 0.92 | Scans pool slots; requires channel+104 without busy flags; marks 0x10|0x100 and clears 0x80; fills out pointer array up to count a4; vtable+124 idle check. |
| 0x82685150 | sub_82685150 | fmod_sound_advance_stream_buffer_for_pitch | 0.90 | Uses codec vtable+92 length; walks PCM buffer nodes forward/back per pitch sign; invokes stream refill callback at sound+324. Called after fmod_sound_set_stream_position. |
| 0x8269C840 | sub_8269C840 | fmod_speaker_levels_pool_alloc_for_sound | 0.93 | Source string "..\\src\\fmod_speakerlevels_pool.cpp"; allocates per-speaker gain rows sized from system channel count. Used by set_multichannel_speaker_levels. |
| 0x82685700 | sub_82685700 | fmod_event_instance_set_reverb_occlusion_levels | 0.91 | Clamps direct/reverb occlusion 0..1 at event+280/+284; updates +204; child vtable+112. Called from tick_reverb_occlusion_fade. |
| 0x826855F8 | sub_826855F8 | fmod_event_instance_set_mute_state | 0.91 | Sets mute flag +116 bit2; when muted vtable+60 zero volume on children else restores via fmod_event_instance_set_volume. Honors channel-group mute byte +68. |
| 0x82684108 | sub_82684108 | fmod_event_instance_copy_speaker_level_row | 0.90 | Copies floats from speaker-level matrix at sound+50 for row a2 into buffer a3 length a4. Used when relinking multichannel mode 2 events. |
| 0x82689890 | sub_82689890 | fmod_reverb_compute_occlusion_attenuation | 0.90 | Builds listener/source vectors; dispatches geometry query; writes direct/reverb occlusion scalars (1.0 - result). Called from tick_reverb_occlusion_fade. |
| 0x826A6350 | sub_826A6350 | fmod_dsp_effect_plugin_ctor_init | 0.91 | Calls fmod_dsp_unit_ctor_init_common then assigns effect vtable off_821177C8; default parameter ranges at +272/+288/+264. Base for ParamEQ ctors. |

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
| 0x8268D370 | sub_8268D370 | Defer iter 90; speaker-level matrix DSP fill path. |
| 0x826738B8 | sub_826738B8 | Defer iter 90; DSP parameter list ctor helper. |
| 0x82690668 | sub_82690668 | Defer iter 90; DSP get-parameter-by-name thunk. |
| 0x82690678 | sub_82690678 | Defer iter 90; DSP set-parameter-by-name thunk. |
| 0x82690680 | sub_82690680 | Defer iter 90; DSP release adjustor thunk. |
| 0x826897A8 | sub_826897A8 | Defer iter 90; reverb geometry query list reset. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
