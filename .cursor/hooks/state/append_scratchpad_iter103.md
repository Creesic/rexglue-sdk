## Iteration 103

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826C1128 | sub_826C1128 | FM2_XAudio2_Engine_NotifyClientVoiceOperationEnd | 0.91 | Client callback code -65521 with 8-byte struct; dispatched from `FM2_XAudio2_Voice_DecPendingCallbackAndDispatch` default path with voice id at +52. |
| 0x826C1678 | sub_826C1678 | FM2_XAudio2_Engine_NotifyClientVoiceOperationResume | 0.90 | Client callback code -65527 with 16-byte struct (voice fields at +11/+13/+21); dispatched when flag 0x10 and resume conditions met. |
| 0x826C17C0 | sub_826C17C0 | FM2_XAudio2_Engine_NotifyClientVoiceOperationBegin | 0.90 | Client callback code -65528 with same 16-byte layout as resume; dispatched when flag 0x10 and flag 4 clear. |
| 0x826D3D98 | sub_826D3D98 | FM2_XAudio2_Engine_FinalizePendingSubmit | 0.88 | Copies optional leap buffer to active voice; calls `FM2_XAudio2_SourceVoice_CompletePendingSubmit`; clears engine pending fields; releases COM refs; submits tail via `FM2_XAudio2_Stream_SubmitBufferLockedInner`. |
| 0x826E5598 | sub_826E5598 | FM2_XAudio2_XapoRegistration_CloseProcessingHandles | 0.89 | Stops processing via vtable+48; closes thread/event handles at +120/+124; clears registration back-pointer at +8. Called from XAPO registration destroy. |
| 0x826E5420 | sub_826E5420 | FM2_XAudio2_XapoRegistration_QueueProcessingWork | 0.88 | Alloc work item from pool `stru_82A3CD88`; creates event+worker thread on first use; enqueues callback at +112; signals event. |
| 0x826E4CB8 | sub_826E4CB8 | FM2_XAudio2_XapoRegistration_ProcessWorkQueue | 0.89 | Worker loop: wait on event; dequeue work items; invoke callback at item+12; free item pool. Called from worker thread main. |
| 0x826E5258 | sub_826E5258 | FM2_XAudio2_XapoRegistration_WorkerThreadMain | 0.92 | Thread entry registered by `FM2_Kernel_XapipCreateThread`; calls `FM2_XAudio2_XapoRegistration_ProcessWorkQueue`. |
| 0x826C3428 | sub_826C3428 | FM2_XAudio2_SubmitEntry_SetPacketRef | 0.90 | Atomic inc packet ref at +40; stores packet pointer at submit-entry+32. Used during pending submit flush. |
| 0x826C4B98 | sub_826C4B98 | FM2_XAudio2_RoutingTable_Grow | 0.91 | Doubles routing slot table (cap 0xFFFFFE); heap realloc 8-byte entries; initializes free-list links. Called from `FM2_XAudio2_RoutingTable_InsertSlot` when full. |
| 0x826DD940 | sub_826DD940 | FM2_XAudio2_XapoEffect_TryAcquireRef | 0.88 | Split atomic inc at +100 when HIWORD nonzero; inc refcount at +96; returns 0 if HIWORD already zero. Pair of `FM2_XAudio2_XapoEffect_DecRefAndMaybeDestroy`. |
| 0x826B5400 | sub_826B5400 | FM2_XAudio2_SourceVoice_IncPendingProcessCount | 0.91 | Atomic inc at source-voice+172. Called from `FM2_XAudio2_SourceVoice_EnqueuePacketAndProcess` when queueing packet work. |

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
| 0x826C0CC0 | sub_826C0CC0 | Thin pool-free wrapper (`unk_82A3C880`) only. |
| 0x826DDE48 | sub_826DDE48 | Thin wrapper → `FM2_XAudio2_XapoEffect_TryAcquireRef` only. |
| 0x826ED720 | sub_826ED720 | SQLite error builder ("no such table"); defer SQLite cluster pass. |
| 0x826ED8C0 | sub_826ED8C0 | SQLite collated strcasecmp; defer SQLite cluster pass. |
| 0x826ED1C8 | sub_826ED1C8 | SQLite parse step helper; defer SQLite cluster pass. |
| 0x82707108 | sub_82707108 | SQLite vsnprintf wrapper; defer with ED720 cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
