## Iteration 87

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82683B00 | sub_82683B00 | fmod_event_instance_set_3d_attributes | 0.92 | Clamps eight floats 0..5 at event+168..196; propagates via child-channel vtable+76 when a38 set. Body behind fmod_event_instance_set_3d_attributes_by_descriptor. |
| 0x82684618 | sub_82684618 | fmod_event_instance_set_spread_angle | 0.91 | Requires 3D mode flags at channel+100; validates 0..360; stores event+304. Body behind spread_angle_by_descriptor wrapper. |
| 0x826864C8 | sub_826864C8 | fmod_event_instance_set_reverb_level | 0.91 | Validates 0..1; stores event+308; may call sub_82685F28 to unpause reverb DSP when level<1. Body behind reverb_level_by_descriptor wrapper. |
| 0x82684E70 | sub_82684E70 | fmod_event_instance_get_channel_mode | 0.90 | Reads channel mode dword at primary child channel+100 into out param. Body behind get_channel_mode_by_descriptor wrapper. |
| 0x82681900 | sub_82681900 | fmod_dsp_unit_ctor_init_common | 0.91 | Assigns base vtable off_821169C0; init connection lists at +32/+44/+168; sample rate 44100 at +212; called from fmod_plugin_factory_create_dsp_unit before type-specific vtable. |
| 0x826833C8 | sub_826833C8 | fmod_sound_attach_to_channel_object | 0.90 | Copies 3D/volume params from channel object a2; assigns per-child substream pointers at channel+92; called from fmod_sound_start_on_channel. |
| 0x826835A8 | sub_826835A8 | fmod_sound_attach_to_channel_id | 0.90 | Writes channel id a2 to each child at +96; sets mode 72 at +100; parent back-pointer at +84. Called from fmod_sound_start_on_channel_id. |
| 0x82686FD0 | sub_82686FD0 | fmod_sound_stop_child_channels | 0.90 | Sets stop flag 0x80 on children when a5; vtable+44 stop each channel; optional sub_82682BE8 reset; bumps handle id at sound+28 when a2. Used from prepare_sound paths. |
| 0x82682C80 | sub_82682C80 | fmod_sound_allocate_channel_handle_id | 0.92 | Increments low-16-bit counter in composite handle at sound+112 (or +36); preserves upper handle index bits. Called after successful play. |
| 0x82684F40 | sub_82684F40 | fmod_sound_update_stream_playback_cursor | 0.89 | sub_826848B8 effective position ×1000; updates stream watermark at channel+128 and requeues buffer nodes when changed. |
| 0x826594D8 | sub_826594D8 | fmod_surround_pan_compute_speaker_gains | 0.90 | log10 pan magnitude; writes four speaker gain floats to a4 using direction a2 and 5.1 layout flag a3. Called from fmod_event_apply_automated_channel_properties after speaker_mode==6 check. |
| 0x82659740 | sub_82659740 | fmod_curve_evaluate_cubic_bezier | 0.91 | Evaluates cubic-bezier segment from eight control floats in a2 at parameter a3; stores XY result in result/out. Used for fade envelope interpolation in event automation. |

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
| 0x82683730 | sub_82683730 | Defer iter 88; play-all-child-channels helper. |
| 0x826853D0 | sub_826853D0 | Defer iter 88; pause/unpause child channels. |
| 0x826848B8 | sub_826848B8 | Defer iter 88; effective playback position calc. |
| 0x82685A58 | sub_82685A58 | Defer iter 88; stream seek/decode prep. |
| 0x82685840 | sub_82685840 | Defer iter 88; stream set position. |
| 0x82685F28 | sub_82685F28 | Defer iter 88; reverb DSP unpause helper. |
| 0x82686560 | sub_82686560 | Defer iter 88; channel disconnect/cleanup. |
| 0x82682BE8 | sub_82682BE8 | Defer iter 88; sound buffer list reset. |
| 0x826808B0 | sub_826808B0 | Defer iter 88; DSP scratch string allocator. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
