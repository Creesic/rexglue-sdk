## Iteration 92

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826723D0 | sub_826723D0 | fmod_string_strncmp_bounded | 0.91 | Bounded byte strcmp loop; returns signed char diff; stops at NUL or max length a3. |
| 0x826722B8 | sub_826722B8 | fmod_string_strncpy_bounded | 0.90 | Copies up to a3 bytes from src; stops on NUL; classic strncpy semantics. |
| 0x826722E8 | sub_826722E8 | fmod_string_strcat | 0.90 | Finds dest NUL terminator then appends src byte-by-byte including terminator. |
| 0x82672418 | sub_82672418 | fmod_string_stricmp | 0.91 | Case-folds A-Z per char; unbounded stricmp returning signed diff. Distinct from case-sensitive `fmod_ascii_stricmp`. |
| 0x8268C930 | sub_8268C930 | fmod_metadata_node_replace_payload_buffer | 0.90 | Compares payload at +24/+32; on size/content change frees via `fmod_metadata.cpp:136` and realloc+copy at :140. Called from metadata list append update path. |
| 0x82674110 | sub_82674110 | fmod_os_init_critical_section | 0.92 | Allocates 28-byte critsec from `fmod_os_misc.cpp:505` or uses static `unk_82A11418`; `RtlInitializeCriticalSection`. |
| 0x826741C8 | sub_826741C8 | fmod_os_delete_critical_section | 0.91 | Frees heap critsec unless static `unk_82A11418`; `fmod_os_misc.cpp:542`. |
| 0x82671848 | sub_82671848 | fmod_dsp_delay_line_register_connection_slot | 0.89 | `FM2_FMOD_Dsp_AdjustDelayLinePointers_0`; finds free slot among 32 at +48; stores handle at +12*4+ and partner at +44*4+. |
| 0x826718E0 | sub_826718E0 | fmod_dsp_delay_line_unregister_connection_slot | 0.89 | Scans 32 slots matching delay-line handle + partner id; clears slot arrays and flag byte at +304. |
| 0x82673720 | sub_82673720 | fmod_dsp_get_parameter_bytes_remaining | 0.88 | Returns read cursor +364 minus base +376; FMOD_ERR_INVALID_PARAM (36) if out pointer null. |
| 0x82674368 | sub_82674368 | fmod_os_wait_for_object_infinite | 0.90 | `FM2_Win32_WaitForSingleObject(a1, -1)`; returns 36 if handle is -1. |
| 0x826A0998 | sub_826A0998 | fmod_reverb_geometry_mesh_unlink_from_tree | 0.87 | Unlinks geometry mesh node (flag 0x40) from spatial hash parent/child/sibling lists; clears link fields; may call `sub_8269F890` to free subtree. Called from reverb geometry build/teardown paths. |

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
| 0x8269D9C8 | sub_8269D9C8 | Plugin heap-free callback only; defer unless vtable slot naming needed. |
| 0x82681D90 | sub_82681D90 | DSP child-input connect; partial read — defer iter 93. |
| 0x82679230 | sub_82679230 | Channel pitch/volume table write; callee `sub_826AB8B8` not resolved. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
