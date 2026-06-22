## Iteration 99

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826C9B38 | sub_826C9B38 | FM2_XAudio2_CLeapBuffer_DecRefAndFree | 0.90 | Atomic dec refcount at +4; on negative calls `FM2_XAudio2_Pool_FreeWithCallback(&unk_82A3CB08)`. Pair of `FM2_XAudio2_CLeapBuffer_DecRefAndInvokeCallback` at +0. |
| 0x826C9B80 | sub_826C9B80 | FM2_XAudio2_StreamPacket_DecRefAndFree | 0.89 | Atomic dec at +4; frees via pool `unk_82A3CA90` when zero. Called from `FM2_XAudio2_SubmixVoice_FlushAndDestroy` after packet finalize. |
| 0x826C1D18 | sub_826C1D18 | FM2_XAudio2_StreamBuffer_AppendBytes | 0.91 | Advances write cursor at +16; decrements remaining at +12; tail memcpy via `FM2_MemcpyAligned`; sets overflow flag at +20; returns `XAUDIO2_E_INVALID_CALL` (-2146074368). |
| 0x826C1C50 | sub_826C1C50 | FM2_XAudio2_StreamBuffer_ConsumeBytes | 0.91 | Mirror of append: copies from read ptr at +4, advances read ptr, decrements remaining; same overflow/error path. |
| 0x826C1BD8 | sub_826C1BD8 | FM2_XAudio2_StreamBuffer_Init | 0.92 | Sets read/write/end pointers from base+size; optional 4-byte align trim; zeros counters or sets error flag when null base. |
| 0x826B4E78 | sub_826B4E78 | FM2_XAudio2_SubmixVoice_FlushAndDestroy | 0.88 | Under critsec: drains three packet lists, calls `FM2_XAudio2_Stream_DecRefAndFinalizePacket` + `FM2_XAudio2_StreamPacket_DecRefAndFree`, waits on voice vtable+52, frees voice heap block. |
| 0x826B0E90 | sub_826B0E90 | FM2_XAudio2_VoiceBufferTable_Grow | 0.90 | Doubles slot table via `FM2_XAudio2_Heap_AllocFromSlotOrHeap`; copies old entries; initializes free-list links; frees old table. |
| 0x826B1298 | sub_826B1298 | FM2_XAudio2_VoiceBufferTable_InsertSlot | 0.89 | Unlinks slot from free list; stores packet ptr; sets slot flag bit; links packet into voice list (head vs tail by flag 0x10). |
| 0x826B13C0 | sub_826B13C0 | FM2_XAudio2_Voice_SubmitBufferByKey | 0.88 | Waits on voice event; grows table if needed; inserts by key `(bufferKey ^ voiceKey) & 0xFFFFF`; returns OOM/E_FAIL. |
| 0x826B1760 | sub_826B1760 | FM2_XAudio2_Voice_SubmitBufferWithPacket | 0.88 | Assigns monotonic sequence at +80; stores id at packet+44/+72 under critsec; calls `FM2_XAudio2_VoiceBufferTable_InsertSlot`. |
| 0x826C0528 | sub_826C0528 | FM2_XAudio2_Stream_BuildPacketDescriptor | 0.87 | Fills 24-byte packet header via buffer consume/append; sets type=24, flags from voice state bits 0x1/0x2/0x10/0x40. |
| 0x826BFB70 | sub_826BFB70 | FM2_XAudio2_StreamPacket_AllocObject | 0.90 | Checks heap slot 0x28 busy; allocates from pool `unk_82A3C9C0`; returns packet object pointer. Used by `sub_826C24A8` submit path. |

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
| 0x826C0008 | sub_826C0008 | Atomic inc at +60 only; defer until paired dec path named. |
| 0x826C24A8 | sub_826C24A8 | Large XAudio2 buffer-submit orchestrator; needs dedicated pass (25 xrefs). |
| 0x826C2A98 | sub_826C2A98 | Stream flush/submit callback path; partial read — defer iter 100. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
