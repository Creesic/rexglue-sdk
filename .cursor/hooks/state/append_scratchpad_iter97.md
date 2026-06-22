## Iteration 97

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826BBCD0 | sub_826BBCD0 | FM2_XAudio2_Pool_AllocEntry | 0.91 | `InterlockedPopEntrySList` or heap alloc; optional init callback at pool+0; atomic inc live count at +36; pairs with `FM2_XAudio2_Pool_FreeWithCallback`. |
| 0x826BBC50 | sub_826BBC50 | FM2_XAudio2_Pool_DrainAndFreeAll | 0.90 | Drains SLIST at pool+24; optional per-entry callback; `HeapFree` each node; clears pool count. Called from XAudio teardown paths. |
| 0x826BBC18 | sub_826BBC18 | FM2_XAudio2_Pool_InitDescriptor | 0.89 | Stores alloc/free callbacks and element size; zeroes SLIST header fields at +24/+32. |
| 0x826BBAC8 | sub_826BBAC8 | fmod_os_critsec_init_defaults | 0.90 | `RtlInitializeCriticalSection` then immediate enter/leave probe. |
| 0x826AE760 | sub_826AE760 | FM2_XAudio2_Voice_Destroy | 0.90 | Closes voice handles; `FM2_XAudio2_Voice_ReleaseResources`; frees voice object heap block. Called from `fmod_refcount_release`. |
| 0x826B1238 | sub_826B1238 | FM2_XAudio2_Voice_ReleaseResources | 0.91 | Frees voice buffer heap block; closes event handles; `FM2_XAudio2_VoicePool_ReleaseVoiceLocked`. |
| 0x826B5068 | sub_826B5068 | FM2_XAudio2_Voice_FlushPendingRelease | 0.88 | Under critsec clears pending-release pointer at +152; forwards to `FM2_XAudio2_PendingReleaseContext_Free`. |
| 0x826CD760 | sub_826CD760 | FM2_XAudio2_PendingReleaseContext_Free | 0.89 | Invokes submix teardown helper; vtable+8 release; frees pending-release context heap block. |
| 0x826B93F8 | sub_826B93F8 | FM2_XAudio2_Voice_DeferredRelease | 0.88 | Frame-sync deferred voice teardown: waits on voice list, `FM2_XAudio2_Stream_SubmitBufferLocked`, HRESULT-style errors; called when refcount hits zero. |
| 0x8269A6E8 | sub_8269A6E8 | fmod_sound_sample_ctor_init | 0.90 | `fmod_sound_ctor_init_defaults`; installs sample vtable `off_821173F8`; sets channel index -1 at +224 and default play state fields. |
| 0x826B0DA8 | sub_826B0DA8 | FM2_XAudio2_VoicePool_AllocVoiceObject | 0.89 | `FM2_XAudio2_Pool_AllocEntry` from `stru_82A3CC08`; init via `sub_826BE1C8`; installs vtable `off_829A7578` or frees on failure. |
| 0x82672060 | sub_82672060 | fmod_dsp_delay_line_unregister_global_pool | 0.88 | Thin wrapper forwarding to `fmod_dsp_delay_line_unregister_connection_slot` with global FMOD heap pool `off_829A6848[1]`. |

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
| 0x82681750 | sub_82681750 | Thin forwarder → fmod_channel_get_dsp_unit_by_index only. |
| 0x82684F30 | sub_82684F30 | Stores single dword at +32 only. |
| 0x82673E78 | sub_82673E78 | Thunk to misnamed `FM2_CarAudio_AllocStreamBufferZeroed` (actually `li r4,0` + `FM2_Thread_SleepMilliseconds`). |
| 0x8267F8F8 | sub_8267F8F8 | Thin wrapper → sub_82673E78 only. |
| 0x826B4820 | sub_826B4820 | Generic `HeapFree` if non-null; too trivial alone. |
| 0x826B4940 | sub_826B4940 | Zeros two dwords only; defer with callback-context cluster. |
| 0x826B1128 | sub_826B1128 | Atomic voice-wait refcount; needs paired event-alloc naming. |
| 0x826BF4C8 | sub_826BF4C8 | Tiered heap slot allocator (index ≤0x5F); defer with BF5F0/BF560 cluster. |
| 0x826BF5F0 | sub_826BF5F0 | Tiered heap slot busy path returning E_OUTOFMEMORY; defer cluster pass. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
