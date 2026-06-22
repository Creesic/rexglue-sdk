## Iteration 78

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82582250 | sub_82582250 | audio_car_cue_record_metadata_copy_assign | 0.91 | FM2_Stl_String_CopyAssign on four metadata string fields (+0/+28/+56/+84). Used by copy_construct and vector insert paths. |
| 0x825822B0 | sub_825822B0 | audio_car_cue_record_copy_construct | 0.92 | Calls metadata_copy_assign + CopyAssign at +112; copies bytes +140/+141 and dword +144. Core 148-byte record clone. |
| 0x82582468 | sub_82582468 | audio_car_cue_vector_shift_elements_forward | 0.90 | Shifts 148-byte range [a1,a2) forward via audio_car_cue_record_assign; returns new tail pointer. |
| 0x825824C0 | sub_825824C0 | audio_car_cue_vector_destroy_elements | 0.91 | Walks 148-byte elements calling audio_car_cue_record_dtor. Used on clear, shrink, and reallocate. |
| 0x825825C8 | sub_825825C8 | audio_car_cue_vector_reset_begin_pointer | 0.89 | When end!=begin: shift_forward then destroy_elements; updates vector end at a2+8. Called from audio_car_cue_vector_clear. |
| 0x82582650 | sub_82582650 | audio_car_cue_vector_free_storage | 0.90 | destroy_elements on [begin,end) then FM2_Memory_FreeSmallBlockOrNull; zeroes vector fields. audio_manager_deferred_dtor body. |
| 0x82582708 | sub_82582708 | audio_car_cue_vector_insert_n | 0.90 | STL vector insert: copy_construct temp record; shift/emplace/realloc paths; max size check 29020049; throws on overflow. |
| 0x82582F10 | sub_82582F10 | audio_car_cue_vector_emplace_back | 0.91 | insert_n(a2, end, 1, record) then returns iterator pair. Called from audio_car_cue_bank_push_back_record growth path. |
| 0x82582518 | sub_82582518 | audio_car_cue_vector_uninitialized_fill_n | 0.89 | Constructs n records at a2 via audio_car_cue_record_copy_construct from prototype a4; returns end pointer. |
| 0x82582570 | sub_82582570 | audio_car_cue_vector_shift_elements_backward | 0.90 | Shifts 148-byte range backward via audio_car_cue_record_assign during insert-with-capacity path. |
| 0x82581E90 | loc_82581E90 | audio_car_cue_get_bank_record_count | 0.92 | Returns (vector_end - vector_begin) / 148 for bank at +12/+16; 0 if begin null. CAudioManagerDeferred vtable 0x8204A434. |
| 0x82581E40 | loc_82581E40 | audio_car_cue_set_active_flag | 0.90 | stb r4, +24 (0x18); cleared in audio_car_cue_refresh_active_cues_and_volumes. Vtable 0x8204A448. |

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
| 0x82581E48 | loc_82581E48 | Getter lbz +25; defer with playback-flag cluster iter 79. |
| 0x82581E50 | loc_82581E50 | Volume/fade setter cluster; defer iter 79. |
| 0x82581E60 | loc_82581E60 | Fade timer + flag setter; defer iter 79. |
| 0x82581E70 | loc_82581E70 | Volume-suppress setter +26; defer iter 79. |
| 0x82581E78 | FM2_SQLiteToken_GetFlagAt26 | Shared +26 byte getter also used by SQLite; do not overwrite. |
| 0x82581E80 | loc_82581E80 | Current/target volume setter; defer iter 79. |
| 0x82581EB8 | loc_82581EB8 | Sets +25 and notifies handle vtable+28; defer iter 79. |
| 0x82581EF0 | loc_82581EF0 | Returns whether car-audio handle +56 attached; defer iter 79. |
| 0x82581908 | loc_82581908 | CAudioThreadLink adjustor thunk; defer iter 79. |
| 0x82581918 | loc_82581918 | Virtual dispatch through +0x12C; defer iter 79. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
