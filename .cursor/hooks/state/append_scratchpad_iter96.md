## Iteration 96

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8268C670 | sub_8268C670 | fmod_codec_read_pcm_into_buffer | 0.89 | Ring-buffer read from codec internal PCM at +176/+188/+192; optional decompress callback at +88; calls `fmod_codec_flush_decode_metadata_cache` on success. |
| 0x8268C570 | sub_8268C570 | fmod_codec_flush_decode_metadata_cache | 0.90 | Invokes codec callback at +212; lazy-alloc metadata list at +208 from `fmod_codec.cpp:197`; forwards to `fmod_codec_metadata_list_merge_nodes`. |
| 0x8268CCE8 | sub_8268CCE8 | fmod_codec_metadata_list_merge_nodes | 0.89 | Links decoded metadata nodes into circular list; dedupes by `fmod_ascii_stricmp` else `fmod_metadata_node_replace_payload_buffer` + `fmod_metadata_node_free`. |
| 0x8268C898 | sub_8268C898 | fmod_metadata_node_free | 0.91 | Frees payload at +20/+24 via `fmod_metadata.cpp:86/94/98` heap helper. |
| 0x82672B68 | sub_82672B68 | fmod_file_async_close_and_release | 0.88 | Sets close flag +383; waits on async handle; unlinks from file-manager list; vtable+12 close; system callback at +17496; frees buffer at `fmod_file.cpp:664`. |
| 0x82672778 | sub_82672778 | fmod_file_handle_release | 0.90 | Unlinks file handle circular list; `fmod_thread_shutdown_and_release`; deletes critsec; frees at `fmod_file.cpp:287`. |
| 0x8269AAB8 | sub_8269AAB8 | fmod_sound_set_playback_position | 0.89 | Recursive subsound index walk for mode a3==2; calls `fmod_sound_seek_pcm_samples` and `fmod_sound_reset_pcm_decode_cache`; invokes sound callback at +208+196. |
| 0x8268BB80 | sub_8268BB80 | fmod_sound_seek_pcm_samples | 0.88 | Bounds sample index; codec read at +108; PCM/timeunit conversion using format flags at +76; invokes seek callback at +96 when mode bits match. |
| 0x82674478 | sub_82674478 | fmod_sound_reset_pcm_decode_cache | 0.87 | Zeroes decode buffer at +44 sized by +47; invokes PCM refill callback at +34. Called before subsound seek. |
| 0x82698C58 | sub_82698C58 | fmod_dsp_plugin_unit_ctor_init | 0.90 | Calls `fmod_sound_ctor_init_defaults`; installs vtable `off_82117320`; sets plugin flag byte at +284. |
| 0x82675A08 | sub_82675A08 | fmod_sound_ctor_init_defaults | 0.91 | Installs sound vtable `off_821167D8`; default 44100 Hz, min/max distance, empty circular lists; called from system ctor and plugin ctor. |
| 0x826AE800 | sub_826AE800 | fmod_refcount_release | 0.88 | PowerPC `lwarx`/`stwcx` atomic decrement at +4; on zero calls `sub_826B93F8` then `sub_826AE760` destroy. |

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
| 0x82673E78 | sub_82673E78 | Single-call thunk to misnamed `FM2_CarAudio_AllocStreamBufferZeroed`; likely semaphore release — needs import verify. |
| 0x8269A6E8 | sub_8269A6E8 | Alternate sound ctor with vtable `off_821173F8`; defer paired sound-type pass. |
| 0x826AE760 | sub_826AE760 | Refcounted object destroy; defer with `sub_826B93F8` cluster. |
| 0x826B93F8 | sub_826B93F8 | Large refcount teardown; defer dedicated 0x826B* pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
