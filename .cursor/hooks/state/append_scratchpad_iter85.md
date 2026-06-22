## Iteration 85

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8266FAA0 | sub_8266FAA0 | fmod_channel_object_get_parameter_range | 0.90 | resolve; rejects locked channel+47; vtable+24 writes min/default/max floats. Paired with fmod_channel_handle_get_parameter_min_max_default at +72; used from fmod_event_apply_automated_channel_properties when inner channel handle present. |
| 0x8266FA08 | sub_8266FA08 | fmod_channel_object_set_parameter_triplet | 0.89 | resolve; vtable+20 with three doubles. Companion setter after get_parameter_range in automation path. |
| 0x8267AF50 | sub_8267AF50 | fmod_system_create_dsp_from_description | 0.91 | Copies DSP description string via fmod_strcpy; packs descriptor fields; sub_82691638 creates unit; stores system back-pointer at unit+16. |
| 0x8267B508 | sub_8267B508 | fmod_system_play_sound_on_validated_channel | 0.90 | Body of fmod_system_play_sound_on_channel; validates channel state (locked, mode 15); sub_8267A448 prepare + sub_82686D20 start playback. |
| 0x82679868 | sub_82679868 | fmod_system_play_sound_on_channel_by_id | 0.90 | Body of fmod_system_play_sound_on_channel_ex; takes channel id/handle a3; FM2_FMOD_Event_LookupDescriptorById when a2==-2; returns handle at sound+112. |
| 0x8265B7E8 | sub_8265B7E8 | fmod_event_apply_automated_channel_properties | 0.92 | Large event automation tick: Volume string match, curve eval, pitch/volume/pan/3D/reverb/filter updates via descriptor wrappers; fade envelope math on event struct. |
| 0x82670170 | sub_82670170 | fmod_event_instance_set_volume_by_descriptor | 0.91 | FM2_FMOD_Event_LookupDescriptorById then sub_826854B8 clamps 0..1 and propagates to child channels. |
| 0x82670228 | sub_82670228 | fmod_event_instance_set_pitch_by_descriptor | 0.91 | Lookup descriptor then sub_82683858 clamps to channel pitch min/max and updates children. |
| 0x82670328 | sub_82670328 | fmod_event_instance_set_3d_attributes_by_descriptor | 0.90 | Lookup descriptor then sub_82683B00 forwards eight floats to channel 3D attribute setter. |
| 0x82670A38 | sub_82670A38 | fmod_event_instance_set_spread_angle_by_descriptor | 0.90 | Lookup then sub_82684618 validates 0..360 and stores at event+304 when 3D flags set. |
| 0x82670A80 | sub_82670A80 | fmod_event_instance_set_reverb_level_by_descriptor | 0.90 | Lookup then sub_826864C8 validates 0..1, stores at event+308, may unpause reverb DSP. |
| 0x82670D00 | sub_82670D00 | fmod_event_instance_get_channel_mode_by_descriptor | 0.89 | Lookup then sub_82684E70 reads channel mode dword at channel+100 into out param. |

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
| 0x8266E7E0 | sub_8266E7E0 | Defer iter 86; speaker-mode getter needs callee naming first. |
| 0x826705B8 | sub_826705B8 | Defer iter 86; reverb filter setter wrapper. |
| 0x82670570 | sub_82670570 | Defer iter 86; companion filter setter wrapper. |
| 0x8267A448 | sub_8267A448 | Defer iter 86; sound prepare path needs more callee context. |
| 0x826787F8 | sub_826787F8 | Defer iter 86; sound prepare by handle path. |
| 0x82686D20 | sub_82686D20 | Defer iter 86; channel play start internals. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
