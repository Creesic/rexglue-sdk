## Iteration 71

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8257EBC0 | sub_8257EBC0 | audio_engine_query_device_and_open_channel_invoke | 0.92 | Zeroes 88-byte out struct; calls vtable+0x160 = audio_engine_query_device_and_open_channel. Used after emission/stream setup in FC78/F548/FE10. |
| 0x8257EC50 | sub_8257EC50 | audio_engine_query_device_and_open_channel_get_success_flag | 0.90 | Same invoke path as EBC0; returns v3[0] success flag from query buffer. CAudioEngine interface vtable 0x82049F68. |
| 0x8257ED98 | sub_8257ED98 | audio_engine_create_fmod_stream_and_query_channel | 0.89 | sub_8266EBF0 on FMOD system +8 with stream flags; then query invoke. CAudioEngine vtable 0x82049E14. |
| 0x8257EE08 | sub_8257EE08 | audio_engine_release_fmod_stream_and_requery_channel | 0.88 | sub_8266F9C0 release then EBC0 requery. CAudioEngine vtable 0x82049E1C. |
| 0x8257EB08 | sub_8257EB08 | audio_cue_basic_deferred_release | 0.92 | COM Release refcount at +88; destroys at zero. CAudioCueBasicDeferred vtable 0x82049C2C. |
| 0x8257EA28 | sub_8257EA28 | audio_cue_ui_threadsafe_release | 0.91 | lwarx/stwcx atomic dec at +88 (+22*4). CAudioCueUI vtable 0x82049BA4. RTTI TRefCountedObjectThreadSafe. |
| 0x8257E600 | sub_8257E600 | audio_engine_held_delegate_com_release | 0.87 | COM Release at +12; slot between get_channel_parameter and audio_emission_release in FMOD interface vtable 0x82191FD8. |
| 0x8257EFF8 | sub_8257EFF8 | audio_cue_basic_deferred_ctor | 0.93 | Sets `CAudioCueBasicDeferred::vftable` on outer+inner COM bodies via FM2_ComObject_InitBaseVtable423C0. Vtable 0x82192080. |
| 0x8257EF30 | sub_8257EF30 | audio_engine_update_music_channel_volume | 0.88 | If held object +16 non-null: vtable+64 set float +372 volume; vtable+36 follow-up. CAudioEngine vtable 0x82049E68. |
| 0x8257F210 | sub_8257F210 | audio_engine_init_or_clear_reverb_dsp_chain | 0.89 | DSP lookup "Reverb" via held object +4; on fail clears +24/+20/+84; on success creates reverb channel DSP 18 and emission group off_82049948. |
| 0x8257F338 | sub_8257F338 | audio_engine_reset_dsp_config_to_defaults | 0.90 | Calls audio_fmod_reverb_config_init_defaults; copies defaults into engine EQ/reverb fields (+24..+84). CAudioEngine vtable 0x82049E4C. |
| 0x8257FC78 | sub_8257FC78 | audio_engine_create_emission_for_sound | 0.88 | sub_8266FB88 sound resolve; alloc CAudioEmission off_82049984; vtable+140 play; EBC0 query; FM2_Render_NotifyManagerStateChange. Vtable 0x82049E18. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x825560F8 | sub_825560F8 | Still only lvx128/stvx128 store wrapper. |
| 0x8255AA38 | sub_8255AA38 | Returns constant flt_82A00BAC only. |
| 0x8255AA48 | sub_8255AA48 | Returns constant flt_82A00BA8 only. |
| 0x8255B4C0 | sub_8255B4C0 | Thin wrapper → sub_8236F938. |
| 0x82563A78 | sub_82563A78 | Pass-resource lock loops only increment counters. |
| 0x8256D940 | sub_8256D940 | Refcount dec at +92; no xrefs. |
| 0x82578960 | sub_82578960 | Returns global dword_82A00C9C only. |
| 0x8257E000 | sub_8257E000 | Returns paired volume-table dword at +224; per-category handle semantics still unclear. |
| 0x8257EFA0 | sub_8257EFA0 | Assigns off_82049CAC base vtable only; no RTTI/class string. |
| 0x8257F060 | sub_8257F060 | Scalar dtor wrapper to sub_82585F80; defer with F780 threadsafe variant. |
| 0x8257FC28 | sub_8257FC28 | Thin wrapper calling audio_engine_dtor. |
| 0x82583CC0 | sub_82583CC0 | Internal CAudioEmission FMOD teardown; covered by emission dtors. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
