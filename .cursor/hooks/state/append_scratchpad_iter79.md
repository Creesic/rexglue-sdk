## Iteration 79

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82581E48 | loc_82581E48 | audio_car_cue_get_playback_flag | 0.92 | Returns *(uint8_t*)(this+25). Cleared in audio_car_cue_refresh_active_cues_and_volumes. Vtable 0x8204A470. |
| 0x82581E50 | loc_82581E50 | audio_car_cue_set_current_volume_and_fade_targets | 0.91 | Sets float +32 to a2; +36 and +40 to a3. Matches fade tick fields used by audio_car_cue_update_volume_fade_tick. Vtable 0x8204A458. |
| 0x82581E60 | loc_82581E60 | audio_car_cue_set_fade_delay_and_play_flag | 0.91 | Stores float fade timer +44 and byte play flag +48. Vtable 0x8204A45C. |
| 0x82581E70 | loc_82581E70 | audio_car_cue_set_volume_suppress_flag | 0.92 | stb a2 at +26; audio_car_cue_get_scaled_volume returns 0 when +26 set. Vtable 0x8204A460. |
| 0x82581E80 | loc_82581E80 | audio_car_cue_set_current_and_target_volume | 0.90 | Sets floats +28 and +32 both to a2; same offsets written in audio_car_cue_refresh_active_cues_and_volumes. Vtable 0x8204A46C. |
| 0x82581EB8 | loc_82581EB8 | audio_car_cue_set_playback_flag_and_update_handle | 0.91 | Sets byte +25; if handle +56 non-null calls vtable+28. Vtable 0x8204A44C. |
| 0x82581EF0 | loc_82581EF0 | audio_car_cue_has_car_audio_handle | 0.92 | Returns whether pointer at +56 is non-null (cntlzw test). Vtable 0x8204A450. |
| 0x82581908 | loc_82581908 | audio_thread_link_shutdown_thunk | 0.90 | addi this+0x18; tail-calls audio_thread_link_shutdown. CAudioThreadLink vtable 0x8204A3F0. |
| 0x82581918 | loc_82581918 | audio_thread_link_invoke_frame_notifier | 0.88 | Loads nested object at +0x12C, adjust +8, tail-call vtable[0]. CAudioThreadLink vtable 0x8204A3FC. |
| 0x825FFCA8 | loc_825FFCA8 | FM2_ComObject_AddRef | 0.93 | Increments refcount at +4 and returns new count. Shared COM AddRef; CAudioManagerDeferred vtable 0x8204A420. |
| 0x8228B050 | sub_8228B050 | FM2_ComObject_ReleaseRefCount | 0.93 | Decrements refcount at +4; calls destructor vtable when zero. Shared COM Release; vtable 0x8204A424. |
| 0x8245D5C0 | sub_8245D5C0 | audio_thread_link_shutdown | 0.89 | Critical-section guarded deferred-queue drain; clears intrusive lists and circular buffer nodes at +28/+52. Called from audio_thread_link_shutdown_thunk. |

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
| 0x82581910 | FM2_RewardsQuery_GetRecordOffset24 | Already meaningfully named; also CAudioThreadLink vtable slot. |
| 0x82581E78 | FM2_SQLiteToken_GetFlagAt26 | Shared +26 byte getter also used by SQLite; do not overwrite. |
| 0x82582AA0 | sub_82582AA0 | Body of audio_manager_deferred_dtor; no separate rename needed. |
| 0x82586ED0 | sub_82586ED0 | CAudioEffect adjustor thunk only. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
