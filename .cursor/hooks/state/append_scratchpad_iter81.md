## Iteration 81

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8249BFC0 | sub_8249BFC0 | car_audio_singleton_alloc | 0.92 | FM2_AllocPoolAcquireOrInit_Thunk(7632); car_audio_ctor; assigns TRefCountedObjectThreadSafe<CCarAudio> vftable; zeros field +1904. |
| 0x8249B2F8 | sub_8249B2F8 | car_audio_ctor | 0.91 | Sets CCarAudio::vftable; zero-inits byte slots via sub_82583A10; clears 64-dword table + float matrices at +4208/+4528/+4608. |
| 0x824950A0 | sub_824950A0 | render_frame_link_base_ctor | 0.90 | FM2_ComObject_InitBaseVtable423C0; assigns off_8203F11C/off_8203F114 vtables. Base of render frame-counter link object. |
| 0x82495300 | sub_82495300 | render_frame_counter_link_ctor | 0.91 | Calls render_frame_link_base_ctor; sets off_8203F164/off_8203F15C; field +4 cleared. Used by render_frame_link_attach_car_audio_notify. |
| 0x8249C020 | sub_8249C020 | render_frame_link_attach_car_audio_notify | 0.90 | car_audio_singleton_alloc + render_frame_counter_link_ctor; FM2_Render_GetFrameCounterField; FM2_Render_NotifyManagerStateChange. Caller audio_thread_link_init_from_car_cue_frame. |
| 0x824647D0 | sub_824647D0 | FM2_ComObject_ReleaseRefCountAtomic | 0.93 | lwarx/stwcx atomic dec at this+4; destructor vtable call when zero. Shared thread-safe Release; CAudioEffect vtable 0x8204A964. |
| 0x826759D8 | sub_826759D8 | fmod_channel_set_bound_effect_object | 0.90 | Stores a2 at channel+164. Called from fmod_channel_handle_bind_effect_object during audio_effect_bind_channel_parameter_tables. |
| 0x8266F558 | sub_8266F558 | fmod_channel_handle_bind_effect_object | 0.91 | resolve handle; fmod_channel_set_bound_effect_object(channel, effect_this). audio_effect_bind passes channel at r4 and this at r5. |
| 0x8266F080 | sub_8266F080 | fmod_channel_handle_enqueue_dsp_command | 0.89 | resolve then sub_82680A38: critical-section guarded enqueue into FMOD channel DSP command list with opcode a2. |
| 0x8266F160 | sub_8266F160 | fmod_channel_handle_set_volume | 0.90 | resolve; sub_82681250 lookup DSP unit by index; sub_8269E670 clamps float to [-1,1] at unit+140. |
| 0x8267B1E0 | sub_8267B1E0 | fmod_system_create_dsp_by_type_index | 0.91 | Type 1 builds "FMOD Mixer unit" string; else scans DSP list for matching type at +108. Caller fmod_system_create_dsp_for_channel. |
| 0x82495290 | sub_82495290 | render_frame_link_base_dtor | 0.90 | Release car-audio ComPtr at +12; FM2_ComObject_DeleteOptionalBody; FM2_Object_AssignBaseVtable. Pair of render_frame_link_base_ctor. |

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
| 0x8249BF40 | sub_8249BF40 | Car-audio deferred init dispatch; defer iter 82 with CParams cluster. |
| 0x82495540 | sub_82495540 | Frame-counter link scalar dtor; defer iter 82. |
| 0x8266F0C8 | sub_8266F0C8 | Channel-group setter; defer with 82680B28 iter 82. |
| 0x82681740 | sub_82681740 | Thunk to sub_82680A38 only. |
| 0x82681748 | sub_82681748 | Thunk to sub_82680B28 only. |
| 0x82681770 | sub_82681770 | Thin glue between 82681250 and 8269E670. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
