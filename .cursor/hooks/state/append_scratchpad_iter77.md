## Iteration 77

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82583900 | sub_82583900 | audio_car_cue_play_next_active_track | 0.91 | When cue bank non-empty and +13 active/+14 idle: select active record, acquire handle from path +172, splice +56, set volume on +52 handle. CAudioManagerDeferred vtable 0x8204A43C. |
| 0x825823E8 | sub_825823E8 | audio_car_cue_bank_copy_metadata_at_index | 0.90 | Inits xml element; bounds-checks 148-byte bank at a2; copies four metadata strings via audio_car_cue_record_copy_metadata_strings. Vtable 0x8204A438. |
| 0x82583340 | sub_82583340 | audio_car_cue_bank_select_active_cue_record | 0.89 | Scans bank for +140 active flag; audio_car_cue_record_assign to out; marks +141 used; else falls back to audio_car_cue_bank_select_random_unused_cue. |
| 0x82581D00 | sub_82581D00 | audio_thread_link_init_from_car_cue_frame | 0.90 | sub_8249C020 render frame-link notify from car-cue +0x18; AddRef on result. CAudioThreadLink vtable slot 0x8204A3F8. |
| 0x82582380 | sub_82582380 | audio_car_cue_record_assign | 0.92 | Copies four metadata strings + string at +112 and bytes/dword at +140/+141/+144 between 148-byte cue records. |
| 0x82582310 | sub_82582310 | audio_car_cue_record_copy_metadata_strings | 0.91 | FM2_Stl_String_AssignRange on four string fields at +0/+28/+56/+84 within cue record. |
| 0x825821F0 | sub_825821F0 | audio_car_cue_record_dtor | 0.90 | Clears string +112 and xml element header; optional FM2_Memory_FreeSmallBlockOrNull. Used when destroying 148-byte record ranges. |
| 0x82583438 | sub_82583438 | audio_car_cue_bank_push_back_record | 0.89 | Grows 148-byte vector at result+8: in-place construct via sub_825822B0 or emplace via sub_82582F10. Called from audio_music_cue_bank_load_from_xml. |
| 0x825826A8 | sub_825826A8 | audio_car_cue_vector_clear | 0.90 | Resets vector begin/end via sub_825825C8; destroys elements in range. Called on bank reload and music XML load. |
| 0x82586658 | sub_82586658 | audio_fmod_channel_holder_dtor | 0.90 | Assigns off_8204A8E8 vtable; sub_8266F038 detaches FMOD channel at +4; FM2_Object_AssignBaseVtable. Used by audio_effect_dtor. |
| 0x82586878 | sub_82586878 | audio_fmod_channel_holder_scalar_dtor | 0.91 | Calls audio_fmod_channel_holder_dtor; optional heap free. First slot of off_8204A8E8 vtable. |
| 0x82586ED8 | sub_82586ED8 | audio_effect_threadsafe_scalar_dtor | 0.89 | Restores CRefCountedObjectThreadSafe vtable at this+24; optional free. CAudioEffect adjustor target from sub_82586ED0. |

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
| 0x82582250 | sub_82582250 | CopyAssign helper; defer with vector insert cluster iter 78. |
| 0x825822B0 | sub_825822B0 | Copy-construct helper; thin callee of push_back/insert. |
| 0x82582468 | sub_82582468 | Vector shift helper; defer iter 78. |
| 0x825824C0 | sub_825824C0 | Vector destroy-range helper; defer iter 78. |
| 0x825825C8 | sub_825825C8 | Vector resize-end helper; defer iter 78. |
| 0x82582650 | sub_82582650 | Bank heap free helper; defer iter 78. |
| 0x82582708 | sub_82582708 | Large vector insert; defer iter 78. |
| 0x82582F10 | sub_82582F10 | Vector emplace-back growth; defer iter 78. |
| 0x82586ED0 | sub_82586ED0 | CAudioEffect adjustor thunk only. |
| 0x82581E90 | loc_82581E90 | Not sub_*; label inside deferred vtable cluster. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
