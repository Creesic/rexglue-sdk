## Iteration 86

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8266E7E0 | sub_8266E7E0 | fmod_system_get_channel_speaker_mode | 0.93 | FM2_FMOD_System_ValidateChannelInGlobalList; asm loads dword at channel+0x44F0. Caller compares result==6 (FMOD_SPEAKERMODE_5POINT1) before surround pan math. |
| 0x826705B8 | sub_826705B8 | fmod_event_instance_set_lowpass_cutoff_by_descriptor | 0.90 | Descriptor lookup wrapper → sub_82684718. Called from fmod_event_apply_automated_channel_properties before highpass; v102 cutoff compare. |
| 0x82670570 | sub_82670570 | fmod_event_instance_set_highpass_cutoff_by_descriptor | 0.90 | Descriptor lookup wrapper → sub_82684690. Paired with lowpass wrapper when cutoff changed. |
| 0x8267A448 | sub_8267A448 | fmod_system_prepare_sound_on_channel | 0.91 | Allocates/reuses sound instance (340-byte nodes); channel vtable+132 mode query; sub_8268D2C0 channel-group wiring; called before fmod_sound_start_on_channel. |
| 0x826787F8 | sub_826787F8 | fmod_system_prepare_sound_for_channel_id | 0.90 | Same sound-node alloc path as prepare_sound_on_channel but keyed by channel id/int handle; sub_8268D2C0 with mode 64. |
| 0x82686D20 | sub_82686D20 | fmod_sound_start_on_channel | 0.91 | Body after fmod_system_play_sound_on_validated_channel prepare; sub_826833C8 bind channel object; pause/unpause + sub_82683730 play each child channel. |
| 0x82686E98 | sub_82686E98 | fmod_sound_start_on_channel_id | 0.91 | Parallel to start_on_channel but sub_826835A8 binds by channel id; used from fmod_system_play_sound_on_channel_by_id. |
| 0x826854B8 | sub_826854B8 | fmod_event_instance_set_volume | 0.92 | Clamps 0..1 at event+156; honors mute flag at +116 bit2; propagates vtable+84 to each child channel. Called from set_volume_by_descriptor. |
| 0x82683858 | sub_82683858 | fmod_event_instance_set_pitch | 0.92 | Clamps to channel pitch min/max at +144/+148; stores event+160; vtable+64 per child. Called from set_pitch_by_descriptor. |
| 0x82684718 | sub_82684718 | fmod_event_instance_set_lowpass_cutoff | 0.89 | Iterates child channels at event+48; vtable+120 with cutoff int. Always paired with highpass in system init loop at sub_82679A08. |
| 0x82684690 | sub_82684690 | fmod_event_instance_set_highpass_cutoff | 0.89 | Iterates child channels; vtable+116 with cutoff int. Paired with lowpass setter on same cutoff value. |
| 0x82691638 | sub_82691638 | fmod_plugin_factory_create_dsp_unit | 0.93 | fmod_pluginfactory.cpp source strings; switch on DSP type index allocates unit, assigns vtables off_82116F70 etc., calls init vtable+12. |

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
| 0x82683B00 | sub_82683B00 | Defer iter 87; 3D occlusion bounds setter needs callee vtable slot naming. |
| 0x82684618 | sub_82684618 | Defer iter 87; spread-angle impl behind descriptor wrapper. |
| 0x826864C8 | sub_826864C8 | Defer iter 87; reverb-level impl behind descriptor wrapper. |
| 0x82684E70 | sub_82684E70 | Defer iter 87; channel-mode getter impl behind descriptor wrapper. |
| 0x82681900 | sub_82681900 | Defer iter 87; DSP unit base ctor used from plugin factory. |
| 0x826833C8 | sub_826833C8 | Defer iter 87; sound bind-to-channel setup. |
| 0x826835A8 | sub_826835A8 | Defer iter 87; sound bind by channel id. |
| 0x82686FD0 | sub_82686FD0 | Defer iter 87; large channel stop/teardown routine. |
| 0x82682C80 | sub_82682C80 | Defer iter 87; sound handle id bump. |
| 0x82684F40 | sub_82684F40 | Defer iter 87; stream timing update. |
| 0x826594D8 | sub_826594D8 | Defer iter 87; surround pan vector helper. |
| 0x82659740 | sub_82659740 | Defer iter 87; cubic interpolation helper. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
