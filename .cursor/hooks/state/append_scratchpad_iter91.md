## Iteration 91

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82673DE8 | sub_82673DE8 | fmod_time_read_microseconds | 0.92 | `FM2_Timer_ReadTimeBase64` + `QueryPerformanceFrequency` scales to microseconds; fallback `1000 * FM2_D3D_ReadKernelTickCountImport()`. |
| 0x8267F8B0 | sub_8267F8B0 | fmod_profiler_record_elapsed_milliseconds | 0.90 | Calls `fmod_time_read_microseconds` then divides by `0x3E8` (1000); used after speaker-level mixer fill. |
| 0x8267FA18 | sub_8267FA18 | fmod_dsp_unit_clear_speaker_level_dirty_flag | 0.89 | Recurses child DSP list at +8; clears bit0 of dword +58; sets bit3 if byte +59 set; called from `fmod_system_fill_speaker_levels_from_dsp`. |
| 0x8268D898 | sub_8268D898 | fmod_channel_group_all_physical_voices_busy | 0.88 | Scans channel list; checks DSP +232 bit3 and byte +236; returns 0 when idle voice found, 10 (`FMOD_ERR_CHANNEL_ALLOC`) when all busy. Used from `fmod_system_prepare_sound_on_channel`. |
| 0x8268C490 | sub_8268C490 | fmod_dsp_codec_lazy_init_metadata_append | 0.90 | Lazy-allocates circular metadata list at +208 from `fmod_codec.cpp:446`; forwards to `fmod_metadata_list_append_or_update_node`. |
| 0x8268CDB8 | sub_8268CDB8 | fmod_metadata_list_append_or_update_node | 0.91 | Alloc from `fmod_metadata.cpp:491`; links node via `fmod_metadata_node_init_from_buffer`; dedupes by `fmod_ascii_stricmp` on name + type id. |
| 0x8268C7E0 | sub_8268C7E0 | fmod_metadata_node_init_from_buffer | 0.90 | `fmod_string_dup_to_heap_pool` for name; heap alloc payload; `FM2_MemcpyAligned` copy; stores type id at +12 and format byte at +16. |
| 0x826A0298 | sub_826A0298 | fmod_reverb_dispatch_geometry_raycast | 0.89 | Wraps `fmod_reverb_clip_ray_against_geometry_tree` with callback struct; returns BOOL from hit flag at +8. Called from `fmod_reverb_cast_occlusion_ray_segment`. |
| 0x8269FC18 | sub_8269FC18 | fmod_reverb_clip_ray_against_geometry_tree | 0.90 | Walks geometry mesh list at +56; clips ray segment to AABB slabs; recurses children at +48/+52; invokes callback per triangle. |
| 0x826875F0 | sub_826875F0 | fmod_reverb_evaluate_triangle_occlusion | 0.88 | Ray-triangle intersection with listener basis at +120/+124/+128/+132; winding test on polygon at +148; attenuates occlusion floats at a2+6/+7 below 0.05 threshold. |
| 0x826725B8 | sub_826725B8 | fmod_string_dup_to_heap_pool | 0.91 | strlen loop; `FM2_FMOD_HeapAllocFromPoolLocked` from `fmod_string.cpp:225`; byte-copy including NUL. Used by metadata node init. |
| 0x82672488 | sub_82672488 | fmod_string_strnicmp_bounded | 0.90 | Case-folds A-Z to lower per char; bounded compare loop; returns signed diff like stricmp. |

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
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
