## Iteration 95

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82687A08 | sub_82687A08 | fmod_geometry_polygon_release | 0.90 | Unlinks polygon from geometry object list at +16/+63; frees mesh via `fmod_reverb_geometry_mesh_unlink_from_tree` and `fmod_geometryi.cpp:399/404/409/422`. |
| 0x82687F18 | sub_82687F18 | fmod_geometry_set_polygon_vertex_position | 0.89 | Bounds-checks polygon/vertex indices; writes float triplet; unlinks embedded mesh from spatial hash when flag 0x40 set. |
| 0x826A0150 | sub_826A0150 | fmod_geometry_mesh_replace_from_template | 0.88 | When flag 0x400: `FM2_MemcpyAligned` 60-byte mesh node from template; relinks parent/child/sibling pointers in spatial hash tree. |
| 0x826A0B98 | sub_826A0B98 | fmod_reverb_geometry_update_mesh_spatial_hash | 0.87 | Compares mesh hash key vs cell; on match expands AABB else unlinks; calls `fmod_reverb_geometry_assign_mesh_hash_key`. |
| 0x826A0828 | sub_826A0828 | fmod_reverb_geometry_assign_mesh_hash_key | 0.88 | Computes quantized hash coords from AABB center; sets fields at +28/+32/+36/+40; inserts via `fmod_reverb_geometry_spatial_hash_insert_mesh`. |
| 0x8268A6C8 | sub_8268A6C8 | fmod_thread_create_with_priority | 0.90 | Maps priority -2..3; copies thread name; calls `fmod_thread_create_and_bind_processor` with `fmod_thread_worker_main_loop` entry. |
| 0x8268A7E0 | sub_8268A7E0 | fmod_thread_shutdown_and_release | 0.89 | Signals shutdown; waits on semaphore; closes thread/sync handles; frees block at `fmod_thread.cpp:297`. |
| 0x8268A5F0 | sub_8268A5F0 | fmod_thread_worker_main_loop | 0.90 | Loop: wait on work semaphore, invoke callback at +288 (or vtable), optional `sub_8267F8F8` notify until shutdown flag cleared. |
| 0x82673ED8 | sub_82673ED8 | fmod_thread_create_and_bind_processor | 0.91 | `FM2_Kernel_XapipCreateThread`; matches name prefixes ("FMOD mixer thread", "FMOD stream thread", etc.) to `XSetThreadProcessor` affinity. |
| 0x826929E8 | sub_826929E8 | fmod_codec_xma_ensure_context | 0.92 | Alloc 0x3A28 bytes from `fmod_codec_xma.cpp:102`; aligns XMA buffer pointers; `XMACreateContext`. |
| 0x82694D58 | sub_82694D58 | fmod_sound_fill_subsound_format_exinfo | 0.88 | Zeroes 0x128-byte `FMOD_SOUND_FORMAT` exinfo; derives format/type/block size from subsound mode flags at +48. |
| 0x8267B088 | sub_8267B088 | fmod_system_create_dsp_from_plugin_descriptor | 0.89 | Copies `FMOD_PLUGINLIST` descriptor fields; calls `fmod_plugin_factory_create_dsp_unit`; stores system back-pointer at DSP +16. |

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
| 0x82681750 | sub_82681750 | Thin forwarder → fmod_channel_get_dsp_unit_by_index only. |
| 0x8268C670 | sub_8268C670 | Codec ring-buffer read; callee `sub_8268C570` unresolved — defer iter 96. |
| 0x82672B68 | sub_82672B68 | Async file close; partial read — defer iter 96. |
| 0x8269AAB8 | sub_8269AAB8 | Subsound playback position setter; needs `sub_8268BB80` naming first. |
| 0x82698C58 | sub_82698C58 | DSP plugin unit ctor; vtable `off_82117320` — defer plugin cluster. |
| 0x826AE800 | sub_826AE800 | Atomic refcount dec with lwarx/stwcx; defer refcount cluster at 0x826B*. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
