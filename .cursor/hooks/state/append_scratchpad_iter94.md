## Iteration 94

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8269F890 | sub_8269F890 | fmod_reverb_geometry_expand_mesh_aabb_from_children | 0.89 | Walks mesh sibling list at +44; `FMOD::aabbAdd` on child bounds; expands parent min/max from child mesh list at +56. Called from spatial-hash insert. |
| 0x826A0330 | sub_826A0330 | fmod_reverb_geometry_spatial_hash_insert_mesh | 0.88 | Computes hash key bitmasks at +8/+9/+10; links mesh into tree; calls `fmod_reverb_geometry_expand_mesh_aabb_from_children`; recurses child meshes. |
| 0x8267E0C8 | sub_8267E0C8 | fmod_system_ctor_init_defaults | 0.91 | FMOD system ctor: init circular lists, 512 channel slots, default pitch/volume table via `fmod_system_set_channel_pitch_volume_table_entry`, sample rate 48000, geometry/reverb sub-inits. |
| 0x82681318 | sub_82681318 | fmod_channel_get_dsp_connection_by_index | 0.90 | Under channel critsec; walks connection list at +11 by index; returns connection object (+2) and output DSP (+132). |
| 0x82682040 | sub_82682040 | fmod_channel_flush_pending_dsp_commands | 0.88 | When single input/output DSP: resolves connection, enqueues command, invokes output vtable+28; else clears flag and enqueues only. |
| 0x82681008 | sub_82681008 | fmod_dsp_connection_get_description | 0.89 | Copies 32-byte name from +76; optional out-fields at +108/+112/+156/+160. |
| 0x82680EA8 | sub_82680EA8 | fmod_dsp_get_parameter_description_by_index | 0.90 | 48-byte stride table at +140; copies name/label strings and min/max floats per parameter index. |
| 0x82680DD0 | sub_82680DD0 | fmod_dsp_read_parameter_by_index | 0.88 | Invokes plugin callback at +37; bounds index against +34; returns float value and 16-byte label copy. |
| 0x82679C78 | sub_82679C78 | fmod_system_enter_mixer_critical_section | 0.91 | `FMOD_OS_CriticalSection_Enter` on system block at +2076; paired with leave helper. |
| 0x82679CA0 | sub_82679CA0 | fmod_system_leave_mixer_critical_section | 0.91 | `FM2_FMOD_CriticalSection_Leave` on system block at +2076. |
| 0x8269F878 | sub_8269F878 | fmod_geometry_set_spatial_hash_inv_cell_size | 0.88 | Stores `1.0 / cell_size` float at hash root +16. Called before spatial-hash rebuild. |
| 0x82689810 | sub_82689810 | fmod_geometry_mgr_release_pending_object | 0.89 | Decrements refcount at +12; when zero frees pending object via `fmod_geometry_mgr.cpp:162`. |

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
| 0x82684F30 | sub_82684F30 | Stores single dword at +32 only. |
| 0x826A0150 | sub_826A0150 | Geometry mesh replace-from-template; defer paired with polygon release analysis. |
| 0x82687A08 | sub_82687A08 | Geometry polygon release; partial read — defer iter 95. |
| 0x82687F18 | sub_82687F18 | Set polygon vertex; defer iter 95. |
| 0x8268A6C8 | sub_8268A6C8 | Thread create with priority switch; callee `sub_82673ED8` unresolved. |
| 0x8268A7E0 | sub_8268A7E0 | Thread shutdown/join; defer with thread cluster. |
| 0x8269AAB8 | sub_8269AAB8 | Subsound play-cursor recursion; needs `sub_8268BB80` context. |
| 0x826929E8 | sub_826929E8 | XMA codec context init; defer dedicated codec pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
