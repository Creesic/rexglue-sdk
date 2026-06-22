## Iteration 76

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82586760 | sub_82586760 | audio_effect_set_channel_bypass | 0.92 | Tail-calls sub_8266F260(channel, a2); sets/clears FMOD channel flag bit 4 at +232. CAudioEffect vtable 0x8204A940. |
| 0x82586768 | sub_82586768 | audio_effect_is_channel_bypassed | 0.92 | sub_8266F2C0 reads flag bit 4 into out-byte. CAudioEffect vtable 0x8204A944. |
| 0x82586720 | sub_82586720 | audio_effect_set_channel_paused | 0.90 | Tail-call branch to sub_8266F1B8; r4 pause flag passed from caller; sets bit 2 at channel+232. Vtable 0x8204A938. |
| 0x825867A0 | sub_825867A0 | audio_effect_lerp_set_channel_parameter | 0.89 | fsel-clamped lerp between param tables at a1[3]/a1[4]; writes via sub_8266F308 channel vtable+40. Vtable 0x8204A948. |
| 0x82586870 | sub_82586870 | audio_effect_start_channel | 0.91 | sub_8266F120 → FMOD channel vtable+32 play. CAudioEffect vtable 0x8204A950. |
| 0x82586838 | sub_82586838 | audio_effect_detach_channel | 0.88 | sub_8266F038 releases channel at +4 and nulls pointer. CAudioEffect vtable 0x8204A954. |
| 0x82582B50 | sub_82582B50 | audio_car_cue_release_car_audio_handles | 0.90 | Release COM at +52; sub_825826A8 clears cue vector +8; Release held manager object +56. |
| 0x82582BD0 | sub_82582BD0 | audio_car_cue_bank_select_random_unused_cue | 0.89 | Random index over 148-byte cue records; skips +141 used flag; FM2_BufFile_WriteCString to +43; sub_82582380 copies record. |
| 0x82582E08 | sub_82582E08 | audio_car_cue_acquire_handle_from_path | 0.90 | CAudioManager dword_82A00CEC vtable+48 with path string; on success clears +56 else releases handles via sub_825826A8. |
| 0x825834F8 | sub_825834F8 | audio_music_cue_bank_load_from_xml | 0.91 | BufFile+XmlReader parse; child elements filename/title/artist/album/label/start; calls audio_car_cue_bank_set_name_at_index. Vtable 0x8204A430. |
| 0x82583068 | sub_82583068 | audio_manager_deferred_dtor | 0.90 | sub_82582AA0 tears down CMusic/xml/strings/cue bank; optional FM2_Memory_FreeSmallBlockOrNull. Vtable 0x8204A41C. |
| 0x825866B0 | sub_825866B0 | audio_effect_bind_channel_parameter_tables | 0.88 | Stores channel at +4; assigns param float tables from off_829A269C arrays by index a3. CAudioEffect vtable 0x8204A958. |

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
| 0x82581D00 | sub_82581D00 | Frame-link ctor via sub_8249C020; defer until render-notify cluster reviewed. |
| 0x825823E8 | sub_825823E8 | Cue-record copy by index; defer iter 77 with bank helpers. |
| 0x82582310 | sub_82582310 | Metadata string assign helper; thin callee of 82380. |
| 0x82586ED0 | sub_82586ED0 | CAudioEffect adjustor thunk only. |
| 0x82586658 | sub_82586658 | Internal FMOD channel release; covered by audio_effect_dtor. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
