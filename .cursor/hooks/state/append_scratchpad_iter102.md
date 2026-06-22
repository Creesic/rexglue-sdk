## Iteration 102

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826C11C8 | sub_826C11C8 | FM2_XAudio2_Engine_NotifyClientVoiceContextChanged | 0.91 | Engine client callback at +24 with struct type 12 and code -65520; dispatched from `FM2_XAudio2_Voice_SetBufferTextAndNotify` when voice flag 0x100 set. |
| 0x826C1218 | sub_826C1218 | FM2_XAudio2_Engine_NotifyClientSubmixVoiceText | 0.90 | Client callback code -65532; dispatched when flag 0x200 and engine voice type at +12 equals 2 (submix). |
| 0x826C1268 | sub_826C1268 | FM2_XAudio2_Engine_NotifyClientSourceVoiceText | 0.90 | Client callback code -65515; dispatched when flag 0x400 and engine voice type at +12 equals 1 (source). |
| 0x826C1900 | sub_826C1900 | FM2_XAudio2_Engine_NotifyClientBufferSubmitted | 0.91 | Builds 28-byte notify struct; invokes client callback with code -65519; then `FM2_XAudio2_Voice_DecPendingCallbackAndDispatch`. Called from flush-submit path. |
| 0x826DD488 | sub_826DD488 | FM2_XAudio2_XapoInstance_DecRefAndFree | 0.89 | Atomic dec at +40; on zero calls `FM2_XAudio2_XapoInstance_DestroyResources` and frees pool `stru_82A3CCC0`. 11 xrefs in XAPO graph. |
| 0x826EA4B0 | sub_826EA4B0 | FM2_XAudio2_XapoInstance_DestroyResources | 0.88 | Closes wait/event handles; frees aux pool at +56; releases registration via `FM2_XAudio2_XapoRegistration_DecRefAndFree`; signals parent event. |
| 0x826DD388 | sub_826DD388 | FM2_XAudio2_XapoRegistration_DecRefAndFree | 0.89 | Atomic dec at +4; on zero calls `sub_826E5598` then pool free `stru_82A3CC98`. Called from XAPO instance destroy. |
| 0x826DA4D8 | sub_826DA4D8 | FM2_XAudio2_SourceVoice_ProcessPendingPacketQueue | 0.88 | Async worker: dequeues packets from list at +156; switch on packet type 0–5 dispatches handlers; frees via `FM2_XAudio2_PacketDescriptor_ReleaseResources`. |
| 0x826CF170 | sub_826CF170 | FM2_XAudio2_PacketDescriptor_ReleaseResources | 0.90 | Releases voice refs at +16/+20, leap buffer at +24, COM object at +28; frees descriptor pool `unk_82A3C808`. |
| 0x826C4D00 | sub_826C4D00 | FM2_XAudio2_RoutingTable_InsertSlot | 0.89 | Under leap-buffer thunk: inserts packet into routing table slot; marks slot busy; returns composite routing id. Used by submix routing alloc. |
| 0x826D73C0 | sub_826D73C0 | FM2_XAudio2_Stream_SubmitVoiceControlBuffer | 0.88 | Looks up voice; alloc leap slot 0x4F; submits buffer opcode 222 via `FM2_XAudio2_SourceVoice_SubmitBuffer`; releases resources on completion. |
| 0x826C5820 | sub_826C5820 | FM2_XAudio2_TraceGuardedPool_Alloc | 0.90 | If trace hook `dword_82A3CB84` null or returns true, alloc from pool; else returns hook failure. Used for debug/trace packet nodes. |

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
| 0x826B5400 | sub_826B5400 | Atomic inc at +172 only; too trivial alone. |
| 0x826C0CC0 | sub_826C0CC0 | Thin pool-free wrapper (`unk_82A3C880`) only. |
| 0x826C1128 | sub_826C1128 | Client notify code -65521; defer with C1678/C17C0 batch. |
| 0x826C1678 | sub_826C1678 | Client notify code -65527; defer pending-callback notify batch. |
| 0x826C17C0 | sub_826C17C0 | Client notify code -65528; defer pending-callback notify batch. |
| 0x826C3428 | sub_826C3428 | Sets submit-entry packet ref; defer submit-entry cluster. |
| 0x826D3D98 | sub_826D3D98 | Large engine flush/finalize path; defer iter 103. |
| 0x826E5598 | sub_826E5598 | XAPO handle teardown helper; thin but may pair next iter. |
| 0x826ED720 | sub_826ED720 | String/format parser (80 xrefs); needs `sub_82707108` context first. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
