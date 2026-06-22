## Iteration 98

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826BF4C8 | sub_826BF4C8 | FM2_XAudio2_Heap_AllocFromSlotOrHeap | 0.90 | Per-slot gate arrays at +185/+281 (index ≤0x5F); returns `HeapAlloc` when slot free else 0 when busy. Used from voice buffer resize (`sub_826B0E90`). |
| 0x826BF560 | sub_826BF560 | FM2_XAudio2_Heap_ReleaseToSlot | 0.89 | Clears busy slot flags then forwards to `FM2_XAudio2_Heap_AllocBySizeClass` for actual free. Pair of BF4C8 alloc path. |
| 0x826BF5F0 | sub_826BF5F0 | FM2_XAudio2_Heap_TryAllocSlotElseCallback | 0.88 | When slot busy returns `E_OUTOFMEMORY` (-2147024882); else calls `sub_826BF8F8` callback alloc path. |
| 0x826BF6A0 | sub_826BF6A0 | FM2_XAudio2_Heap_IsSlotBusy | 0.91 | Returns 1 when both slot gate dwords set for index ≤0x5F; used before `FM2_XAudio2_CLeapBuffer_AllocFromPool`. |
| 0x826C19B0 | sub_826C19B0 | FM2_XAudio2_Heap_AllocBySizeClass | 0.90 | Routes by size to `FM2_XAudio2_Pool_AllocEntry` pools (`unk_82A3C7E0`…`82A3C9E8`) or raw `HeapAlloc` for >0x800 bytes. |
| 0x826BF8F8 | sub_826BF8F8 | FM2_XAudio2_CLeapBuffer_AllocFromPool | 0.89 | Alloc from `unk_82A3CA10` via `FM2_XAudio2_Pool_AllocEntry`; init via `FM2_XAudio2_PoolEntry_InitCallback`; rollback via `FM2_XAudio2_CLeapBuffer_DecRefAndInvokeCallback`. |
| 0x826BC038 | sub_826BC038 | FM2_XAudio2_PoolEntry_InitCallback | 0.90 | Stores pool pointer and user callback; invokes init callback to fill entry; returns error if init fails. |
| 0x826BE1C8 | sub_826BE1C8 | FM2_XAudio2_VoiceObject_InitDefaults | 0.89 | Under critsec at +12: resets voice fields, sets format id 66, init empty circular list at +60. Called from `FM2_XAudio2_VoicePool_AllocVoiceObject`. |
| 0x826B1128 | sub_826B1128 | FM2_XAudio2_Voice_WaitEventRefDecrement | 0.88 | Atomic dec on wait counter at +96 with `0x80000000` bias; closes event handle at +132 when count reaches sentinel; leaves critsec at +104. |
| 0x826B2DF8 | sub_826B2DF8 | FM2_XAudio2_Stream_AcquirePacketRef | 0.89 | Inc wait/event ref at +96; waits on handle; inc packet ref at +56; returns active packet pointer in out-param. |
| 0x826B1610 | sub_826B1610 | FM2_XAudio2_Stream_ReleasePacketById | 0.88 | Walks stream packet list; matches id at +36; decrements refs and calls `FM2_XAudio2_VoicePool_ReleaseVoiceLocked`. |
| 0x826C0490 | sub_826C0490 | FM2_XAudio2_Stream_GetActivePacketRef | 0.88 | Under critsec at +116: atomic inc packet ref at +32 when packet active; returns packet pointer. |

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
| 0x826B4820 | sub_826B4820 | Generic `HeapFree` if non-null; too trivial alone. |
| 0x826B4940 | sub_826B4940 | Zeros two dwords only; defer callback-context cluster. |
| 0x826C9B38 | sub_826C9B38 | CLeapBuffer refcount release; defer paired with existing `FM2_XAudio2_CLeapBuffer_DecRefAndInvokeCallback`. |
| 0x826C1D18 | sub_826C1D18 | Stream ring-buffer append; defer stream-buffer cluster pass. |
| 0x826B4E78 | sub_826B4E78 | Submix voice flush; partial read — defer iter 99. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
