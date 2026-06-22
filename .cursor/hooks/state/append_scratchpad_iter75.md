## Iteration 75

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82581D48 | sub_82581D48 | audio_manager_acquire_car_audio_cue_list | 0.90 | If a2+4: create threadsafe cue + frame link; else spin on dword_82A00CEC (CAudioManager) vtable+104 and splice car-audio list. Vtable 0x8204A400 / 0x82192200. |
| 0x82581F10 | sub_82581F10 | audio_manager_release_car_audio_handle | 0.89 | If held object +56 non-null: vtable+24 shutdown then Release. CAudioManagerDeferred vtable 0x8204A440. |
| 0x82581F88 | sub_82581F88 | audio_car_cue_get_scaled_volume | 0.88 | If +26 flag returns 0; else CAudioManager singleton vtable+208 * vtable+124 volume scale. Vtable 0x8204A468. |
| 0x82582020 | sub_82582020 | audio_car_cue_refresh_active_cues_and_volumes | 0.89 | Splices active cue list from CAudioManager vtable+80 into +13; clears flags; computes target/current volumes via vtable+76. Vtable 0x8204A474. |
| 0x825820E8 | sub_825820E8 | audio_car_cue_bank_set_name_at_index | 0.88 | Iterates 148-byte cue records at +12/+16; FM2_BufFile_WriteCString to field +112. Called from sub_825834F8 XML parse. |
| 0x825830B8 | sub_825830B8 | audio_car_cue_update_volume_fade_tick | 0.90 | Clamps dt to 0.066666s; interpolates fade timers +28/+32/+36/+44; updates car-audio handle +52 vtable+24/+32; may reload XML cue list. Vtable 0x8204A444. |
| 0x82583298 | sub_82583298 | audio_car_cue_load_from_xml | 0.89 | FM2_XmlElement parse via sub_82582BD0; sub_82582E08 builds cue list; splices into +14. CAudioManagerDeferred vtable 0x8204A454. |
| 0x8257F868 | sub_8257F868 | audio_cue_basic_deferred_play_params_dtor | 0.91 | Sets `CParams2IAudioCueBasicPlay::vftable`; releases notify ComPtr; reverts to CDeferredQueue params. Vtable 0x82049FB0 via F8C8. |
| 0x8257FA40 | sub_8257FA40 | audio_cue_basic_deferred_stop_params_dtor | 0.91 | Sets `CParams1IAudioCueBasicStop::vftable`; releases notify ComPtr; reverts to CDeferredQueue params. Vtable 0x82049FB8 via FAA0. |
| 0x82586728 | sub_82586728 | audio_effect_is_channel_paused | 0.90 | sub_8266F218 on channel +4; reads FMOD channel flag bit 2 at +232. CAudioEffect vtable 0x8204A93C. |
| 0x825867E8 | sub_825867E8 | audio_effect_get_channel_parameter_float | 0.91 | sub_8266F368 indexed param lookup via tables +8/+2. CAudioEffect vtable 0x8204A94C. |
| 0x825868E0 | sub_825868E0 | audio_effect_release | 0.92 | COM Release refcount at +20; destroys at zero. CAudioEffect vtable 0x8204A92C. |

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
| 0x82586720 | sub_82586720 | Wrapper to sub_8266F1B8 missing pause arg in decompile; semantics unclear. |
| 0x82586760 | sub_82586760 | Bypass setter only; defer with 86768/867A0 iter 76. |
| 0x82586768 | sub_82586768 | Bypass getter; defer with 86760 iter 76. |
| 0x82586870 | sub_82586870 | Thin wrapper → sub_8266F120 channel start. |
| 0x82582B50 | sub_82582B50 | Car-cue resource release; defer with car-cue cluster iter 76. |
| 0x82586658 | sub_82586658 | Internal FMOD channel release; covered by audio_effect_dtor. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
