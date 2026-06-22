## Iteration 88

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82683730 | sub_82683730 | fmod_sound_play_all_child_channels | 0.92 | Iterates child channels at sound+48; vtable+32 play; clears stop/pause flags and sets playing bit 0x40. Called from fmod_sound_start_on_channel paths. |
| 0x826853D0 | sub_826853D0 | fmod_sound_set_child_channels_paused | 0.91 | vtable+52 pause per child; toggles pause flag 0x20 on channel+104; updates sound+29 paused state from primary channel+104 bit5. |
| 0x826848B8 | sub_826848B8 | fmod_event_instance_get_effective_audibility | 0.90 | Computes scalar from channel+56 and event volume fields (+156/+252/+208/+204) for 2D vs 3D mode. Used by fmod_sound_update_stream_playback_cursor. |
| 0x82685A58 | sub_82685A58 | fmod_sound_apply_playback_startup_parameters | 0.91 | Randomizes pitch/volume from substream metadata; calls fmod_event_instance_set_pitch/set_volume; multichannel sub_82683D00 or pan via fmod_event_instance_set_pan. Called at play start. |
| 0x82685840 | sub_82685840 | fmod_sound_set_stream_position | 0.92 | Codec vtable+76 length query with FMOD_TIMEUNIT_PCM/PCMBYTES/MS (0x10000/0x20000/0x40000); vtable+88 seek on each child channel; sub_82685150 refresh. |
| 0x82685F28 | sub_82685F28 | fmod_event_instance_tick_reverb_occlusion_fade | 0.90 | Steps occlusion floats +280/+284 toward targets +288/+292; sub_82685700; FM2_FMOD_Dsp_ProcessReverbMixBlock; may unpause via vtable+36. Called from set_reverb_level and relink paths. |
| 0x82686560 | sub_82686560 | fmod_event_instance_relink_to_channel_group | 0.91 | Unlinks/relinks event+136 channel-group list; child vtable+12 reconnect; reapplies volume/pitch/3D/pan/multichannel when a3. Called from fmod_sound_stop_child_channels when a7. |
| 0x82682BE8 | sub_82682BE8 | fmod_sound_reset_pcm_buffer_queues | 0.90 | Reinitializes circular buffer lists at sound+12/+4 and hooks parent stream queue at channel+576. Called from fmod_sound_stop_child_channels when a3. |
| 0x826808B0 | sub_826808B0 | fmod_dsp_alloc_persistent_string_buffer | 0.91 | One-time init of 10KB byte_82A115A8 pool; returns first slot with high bit set and zeroes it. Stored at DSP unit+236 from fmod_dsp_unit_ctor_init_common. |
| 0x82686338 | sub_82686338 | fmod_event_instance_set_3d_position_and_velocity | 0.92 | Stores position +216..224 and velocity +228..236; sets dirty flag +116 bit3; may call tick_reverb_occlusion_fade when reverb paused. Used at 3D play startup. |
| 0x82679EB0 | sub_82679EB0 | fmod_system_stop_duplicate_channel_plays | 0.90 | Under critical section walks global channel-use list; if channel a2 already playing, sub_82687320 release. Also scans event list for matching user data. |
| 0x82683938 | sub_82683938 | fmod_event_instance_set_pan | 0.92 | Clamps pan -1..1 at event+164; sets mode +120=0; vtable+68 per child (stereo split when two channels). Called from startup/relink paths. |

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
| 0x82683D00 | sub_82683D00 | Defer iter 89; multichannel speaker-gain matrix needs layout enum naming. |
| 0x82691468 | sub_82691468 | Defer iter 89; ParamEQ ctor type 3 vs type 4 compact variant. |
| 0x82691550 | sub_82691550 | Defer iter 89; plugin factory DSP type 4 ctor; distinguish from ParamEQ type 3. |
| 0x8268D2C0 | sub_8268D2C0 | Defer iter 89; channel-pool acquire wrapper; decompiler signature incomplete. |
| 0x8268DCA0 | sub_8268DCA0 | Defer iter 89; channel pool slot acquire internals. |
| 0x82685150 | sub_82685150 | Defer iter 89; stream buffer refill after seek. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
