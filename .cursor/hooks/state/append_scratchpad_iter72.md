## Iteration 72

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8257E000 | sub_8257E000 | audio_engine_get_max_tire_category_index | 0.88 | Returns `*(a1+224)`; field loaded from AudioEngineConfig.xml `"maxTire"` in audio_engine_load_config_from_xml; paired float at +360. Vtable 0x82049F14. |
| 0x8257F428 | sub_8257F428 | audio_engine_create_threadsafe_emission_for_stream | 0.89 | sub_8266EBF0 stream open; alloc CAudioEmission off_82049A1C (thread-safe vtbl); vtable+140 play; channel query. CAudioEngine vtable 0x82049E34. |
| 0x8257F548 | sub_8257F548 | audio_engine_create_emission_group_for_event | 0.88 | sub_8266ECF0 event resolve; CAudioEmissionGroup off_82049AB0; vtable+48 bind; query invoke; render notify. Vtable 0x82049E38. |
| 0x8257F608 | sub_8257F608 | audio_engine_init_music_emission_group | 0.90 | DSP lookup `"music"` via held object +4; nested channel; alloc emission group off_82049AB0 at engine+88; vtable+48 bind. Vtable 0x82049E54. |
| 0x8257F6F8 | sub_8257F6F8 | audio_engine_notify_emission_group_state_change | 0.87 | FM2_Render_NotifyManagerStateChange(a1, a2+84) on engine-held emission group ComPtr. Vtable 0x82049E50. |
| 0x8257FE10 | sub_8257FE10 | audio_engine_create_audio_effect_and_notify | 0.88 | sub_82586988 DSP effect off_8204A924; channel query invoke; render notify. Vtable 0x82049E3C. |
| 0x8257FE88 | sub_8257FE88 | audio_engine_restore_output_channel_binding | 0.88 | Saves/restores notify state; sub_8266EF18 rebind FMOD output channel; channel query invoke. Vtable 0x82049E40. |
| 0x8257FEF0 | sub_8257FEF0 | audio_engine_create_geometry_for_sound_emitter | 0.89 | sub_8266EF60 sound resolve; FMOD::System::setGeometrySettings(4.0); CAudioGeometry off_82049AEC; vtable+100 set geometry. Vtable 0x82049E44. |
| 0x825814B8 | sub_825814B8 | audio_engine_load_cue_project_by_path | 0.90 | Alloc CAudioCueProject off_82049B7C; FM2_Stl_String_CtorFromCStr path; vtable+20 load +24 activate. CAudioEngine vtable 0x82049E60. |
| 0x8257F780 | sub_8257F780 | audio_cue_basic_deferred_threadsafe_dtor | 0.92 | Sets `TRefCountedObjectThreadSafe<CAudioCueBasicDeferred>::vftable`; sub_82585F80 teardown. Vtable 0x82049F8C. |
| 0x8257FC28 | sub_8257FC28 | audio_engine_scalar_dtor | 0.91 | Calls audio_engine_dtor; optional FM2_Memory_FreeSmallBlockOrNull. Secondary CAudioEngine dtor entry. |
| 0x825803F0 | sub_825803F0 | audio_engine_load_config_from_xml | 0.92 | Opens `Game:\\Media\\Audio\\AudioEngineConfig.xml`; parses maxEngine/maxTire/maxTransmission/etc category indices and volume pairs into engine fields. Called from FMOD init chain. |

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
| 0x8257F060 | sub_8257F060 | Scalar dtor wrapper to sub_82585F80 without threadsafe vtbl; defer. |
| 0x8257F7E8 | sub_8257F7E8 | Thin adjustor thunk → audio_cue_basic_deferred_threadsafe_dtor. |
| 0x8257FFD0 | sub_8257FFD0 | CAudioCueUI+thread-link factory; defer iter 73. |
| 0x825800C0 | sub_825800C0 | CAudioCueBasicDeferred factory; defer iter 73. |
| 0x82583CC0 | sub_82583CC0 | Internal CAudioEmission FMOD teardown helper. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
