## Iteration 100

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826C24A8 | sub_826C24A8 | FM2_XAudio2_SourceVoice_SubmitBuffer | 0.87 | Alloc packet via `FM2_XAudio2_StreamPacket_AllocObject`; init type 5; `FM2_XAudio2_StreamPacket_SetLeapBufferRef`; submit via `sub_826C85C0`/`sub_826DA7E8`; error rollback frees packet. 25 callers. |
| 0x826C2A98 | sub_826C2A98 | FM2_XAudio2_SourceVoice_CompletePendingSubmit | 0.88 | State at +60 transitions 1/2→3; calls `FM2_XAudio2_Stream_SubmitBufferLockedTail`; wakes engine work refs; invokes voice callback vtable+24 with packet metadata. |
| 0x826C2DD8 | sub_826C2DD8 | FM2_XAudio2_StreamPacket_SetLeapBufferRef | 0.91 | `XAUDIO2::CLeapBuffer::AddRef` then stores leap buffer at packet+100 (+25 dwords). Called from submit path after packet alloc. |
| 0x826C2210 | sub_826C2210 | FM2_XAudio2_Voice_TryReleaseResourcesIfIdle | 0.89 | Reads recursion count from voice+16; calls parent vtable+40; on success invokes `FM2_XAudio2_Voice_ReleaseResourcesLocked` and clears owner fields. |
| 0x826C0008 | sub_826C0008 | FM2_XAudio2_Voice_IncPendingCallbackCount | 0.92 | Atomic inc at voice+60. Pair of `FM2_XAudio2_Voice_DecPendingCallbackAndDispatch`. 17 callers on submit/flush paths. |
| 0x826C00A8 | sub_826C00A8 | FM2_XAudio2_Voice_DecPendingCallbackAndDispatch | 0.90 | Atomic dec at +60; when zero and flag 0x40000 set, dispatches via `sub_826C1678`/`sub_826C17C0`/`sub_826C1128`; unlinks from pending list; clears flags. |
| 0x826CD890 | sub_826CD890 | FM2_XAudio2_Voice_IncInflightBufferCount | 0.88 | Atomic inc at parent voice+36 when `sub_826C2EC0` links active packet. Used across submit/flush paths (18 xrefs). |
| 0x826BFCD8 | sub_826BFCD8 | FM2_XAudio2_SourceVoice_AllocPacketDescriptor | 0.90 | Slot 0x2B gate; pool `unk_82A3C808`. Called from source submit (`sub_826B8CB8`) to alloc packet descriptors with types 2/5. |
| 0x826BF990 | sub_826BF990 | FM2_XAudio2_SubmixVoice_AllocDescriptor | 0.89 | Slot 0x23 gate; pool `unk_82A3C920`. Submix voice create (`sub_826B5510`) allocates two descriptors from this pool. |
| 0x826BFAF8 | sub_826BFAF8 | FM2_XAudio2_VoiceCallbackQueue_AllocObject | 0.88 | Slot 0x27 gate; pool `unk_82A3C858`. Called from voice callback queue init (`sub_826B1198`) during voice setup. |
| 0x826B5390 | sub_826B5390 | FM2_XAudio2_Engine_DecWorkRefAndSignalEvent | 0.89 | Under critsec at engine+516: dec dword at +28; signals event at +32 when zero and flag 0x10000 clear. |
| 0x826C0728 | sub_826C0728 | FM2_XAudio2_Voice_MarshalBufferToStream | 0.88 | Under critsec: appends pVoice/extra buffers via `FM2_XAudio2_StreamBuffer_AppendBytes`; builds 48-byte descriptor; `FM2_XAudio2_StreamBuffer_ConsumeBytes` into stream. |

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
| 0x826C01F8 | sub_826C01F8 | Large voice property update; needs dedicated pass (10 xrefs). |
| 0x826C0CD0 | sub_826C0CD0 | Voice child release cluster; defer paired with `sub_826C3240`. |
| 0x826C0CC0 | sub_826C0CC0 | Thin pool-free wrapper only (`unk_82A3C880`). |
| 0x826C2EC0 | sub_826C2EC0 | Set active packet + inc parent; defer with voice state cluster. |
| 0x826C5820 | sub_826C5820 | Debug-gated pool alloc; purpose tied to `dword_82A3CB84` hook. |
| 0x826CC050 | sub_826CC050 | Large stream packet enqueue; defer dedicated pass (12 xrefs). |
| 0x826DB628 | sub_826DB628 | High xref (24) but unanalyzed this iter — defer iter 101. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
