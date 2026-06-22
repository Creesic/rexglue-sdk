## Iteration 93

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82681D90 | sub_82681D90 | fmod_dsp_unit_connect_child_input | 0.90 | Validates DSP types/ancestry; `fmod_dsp_connection_pool_alloc` + `fmod_dsp_unit_reset_state`; links connection node with parent at +12 and child at +16 under system critsec. |
| 0x82679230 | sub_82679230 | fmod_system_set_channel_pitch_volume_table_entry | 0.87 | Per-channel index `a2` writes pitch/volume floats at +17652/+17656 and scaled table at +17668; calls `fmod_math_atan2_degrees` then `fmod_system_rebuild_dsp_unit_priority_order`. |
| 0x82681178 | sub_82681178 | fmod_channel_set_physical_voice_allocation_state | 0.89 | Sets physical-voice byte at +236 and bit8 of +232; optional path enters/leaves system mixer critsec via `sub_82679C78`/`sub_82679CA0`. |
| 0x82670B10 | sub_82670B10 | fmod_event_query_layer_muted_by_descriptor_id | 0.88 | `FM2_FMOD_Event_LookupDescriptorById` then `fmod_event_descriptor_any_layer_muted`; clears out-byte on lookup failure. |
| 0x826847A0 | sub_826847A0 | fmod_event_descriptor_any_layer_muted | 0.89 | Iterates layer list; vtable+124 mute query per layer; returns first non-zero mute in out-byte. |
| 0x82678BA8 | sub_82678BA8 | fmod_system_rebuild_dsp_unit_priority_order | 0.88 | Clears priority slots at +4461; greedy pick lowest-cost DSP units per speaker mode (3/4/7 → 6 channels). |
| 0x826AB8F8 | sub_826AB8F8 | fmod_math_atan2_degrees | 0.86 | Fixed-point `atan2(y,x)` using ratio 804/2412 constants; normalizes result to 0–359 degrees. |
| 0x82687438 | sub_82687438 | fmod_reverb_geometry_compute_transformed_aabb | 0.88 | Transforms OBB corners with rotation matrix at +128/+136/+144; writes 6-float AABB; links or unlinks mesh via geometry tree helpers. |
| 0x82689470 | sub_82689470 | fmod_geometry_rebuild_spatial_hash_from_meshes | 0.87 | Unlinks all meshes from spatial hash at +228 then relinks each mesh object into list at +48; marks geometry dirty. |
| 0x826742B8 | sub_826742B8 | fmod_os_create_semaphore | 0.91 | `CreateSemaphoreA(0,0,0xFFFF,0)`; returns 36 if handle pointer is -1. |
| 0x82674328 | sub_82674328 | fmod_os_close_handle | 0.90 | `FM2_NtCloseHandleOrSetLastError`; returns 36 if handle is -1. |
| 0x8269D9C8 | sub_8269D9C8 | fmod_plugin_heap_free_callback | 0.89 | Plugin release path; frees block via `FM2_FMOD_Dsp_ReverbProcessDelayLine` from `fmod_plugin.cpp:30`. |

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
| 0x8268D5D8 | sub_8268D5D8 | Adjustor thunk (-24) to fmod_system_fill_speaker_levels_from_dsp only. |
| 0x82672060 | sub_82672060 | Thin wrapper → fmod_dsp_delay_line_unregister_connection_slot with global pool only. |
| 0x82679C78 | sub_82679C78 | One-line `FMOD_OS_CriticalSection_Enter`; defer unless paired rename pass. |
| 0x82679CA0 | sub_82679CA0 | One-line `FM2_FMOD_CriticalSection_Leave`; defer unless paired rename pass. |
| 0x82684F30 | sub_82684F30 | Stores single dword at +32 only. |
| 0x8269F890 | sub_8269F890 | AABB merge over mesh children; partial read — defer iter 94. |
| 0x826A0330 | sub_826A0330 | Spatial-hash insert helper; needs full tree-walk analysis. |
| 0x8267E0C8 | sub_8267E0C8 | Large FMOD system ctor/init; defer dedicated pass. |
| 0x82682040 | sub_82682040 | Channel DSP flush; callee `sub_82681318` unresolved. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
