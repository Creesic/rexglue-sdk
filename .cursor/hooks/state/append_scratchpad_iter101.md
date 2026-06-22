## Iteration 101

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826CC050 | sub_826CC050 | FM2_XAudio2_Stream_EnqueueOutputPacket | 0.88 | Alloc stream packet type 10 from pool `unk_82A3CA90`; fills header at +116/+118; links into engine queue at +32; may wake processing thread via `sub_826CB6D8`. |
| 0x826C01F8 | sub_826C01F8 | FM2_XAudio2_Voice_SetBufferTextAndNotify | 0.89 | Under critsec: copies pVoice (slot 0x20) and extra (slot 0x21) text via heap alloc; when flag 0x2000 set dispatches engine notify helpers by voice type flags 0x10/0x100/0x200/0x400. |
| 0x826C0CD0 | sub_826C0CD0 | FM2_XAudio2_VoiceState_DecRefAndReleaseChildren | 0.90 | Atomic dec at +24; on zero releases child voices at +28/+32 via `FM2_XAudio2_VoicePool_ReleaseVoiceLocked`; calls `FM2_XAudio2_VoiceLink_DecRefAndUnlink`; frees self via pool `unk_82A3C880`. |
| 0x826C2EC0 | sub_826C2EC0 | FM2_XAudio2_Voice_SetActivePacketRef | 0.91 | Atomic inc packet ref; calls `FM2_XAudio2_Voice_IncInflightBufferCount` on packet parent at +12; stores active packet pointer at voice+96. |
| 0x826C1FC8 | sub_826C1FC8 | FM2_XAudio2_Stream_SubmitQueueUnlinkPacket | 0.89 | Unlinks packet from submit queue list; decrements pending count; calls `FM2_XAudio2_StreamPool_UnlinkAndNotifyBody` when last entry. Used after submit in `FM2_XAudio2_SourceVoice_SubmitBuffer`. |
| 0x826C3240 | sub_826C3240 | FM2_XAudio2_VoiceLink_DecRefAndUnlink | 0.90 | Atomic dec at +16; unlinks from voice list under critsec; releases linked voice and resources; frees from pool `unk_82A3C8F8`. |
| 0x826DA768 | sub_826DA768 | FM2_XAudio2_SourceVoice_EnqueuePacketAndProcess | 0.88 | Under critsec at +684: inserts packet into pending list at +156; inc field at +172; starts async process via parent vtable+12 with `sub_826DA4D8`. |
| 0x826D0EE8 | sub_826D0EE8 | FM2_XAudio2_SubmixVoice_AllocRoutingPacket | 0.89 | Alloc packet via `FM2_XAudio2_StreamPacket_AllocObject`; sets type 9 and flag 0x2; registers routing via `sub_826C4D00`; returns packet id in out-param. |
| 0x826DB628 | sub_826DB628 | FM2_XAudio2_XapoEffect_DecRefAndMaybeDestroy | 0.87 | Split atomic refcount at +100; when high word hits 1 calls `FM2_XAudio2_XapoEffect_DestroyAndNotifyEngine`; dec at +96 frees pool `stru_82A3CD60` when zero. 24 xrefs in XAPO chain. |
| 0x826E8010 | sub_826E8010 | FM2_XAudio2_XapoEffect_DestroyAndNotifyEngine | 0.86 | Teardown path: optional `sub_826E6BD0`/`sub_826E6C30` reset; notifies engine callbacks types 6/7; clears effect fields. Called only from XAPO dec path. |
| 0x826C0A88 | sub_826C0A88 | FM2_XAudio2_Voice_FlushPendingSubmitList | 0.88 | Drains circular list at voice+32; for type-1 entries calls `sub_826C1900` submit; handles packet acquire/release and error rollback; clears flag 0x80000. |
| 0x826C1178 | sub_826C1178 | FM2_XAudio2_Engine_NotifyClientVoiceChanged | 0.90 | Invokes engine client callback at +24 with type 12 and code -65524 (0xFFFF000C); dispatched when voice flag 0x10 set from text-update path. |

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
| 0x826B5400 | sub_826B5400 | Atomic inc at +172 only; paired helper too thin alone. |
| 0x826C0CC0 | sub_826C0CC0 | Thin pool-free wrapper (`unk_82A3C880`) only. |
| 0x826C3230 | sub_826C3230 | Thin pool-free wrapper (`unk_82A3C8F8`) only. |
| 0x826C11C8 | sub_826C11C8 | Client notify with code -65520; defer batch with C1218/C1268. |
| 0x826C1218 | sub_826C1218 | Client notify with code -65532; defer notify batch. |
| 0x826C1268 | sub_826C1268 | Client notify with code -65515; defer notify batch. |
| 0x826C5820 | sub_826C5820 | Trace-gated pool alloc via `dword_82A3CB84`; defer with debug hook cluster. |
| 0x826DD488 | sub_826DD488 | XAPO dec path; defer paired with `sub_826EA4B0` analysis. |
| 0x826ED720 | sub_826ED720 | High xref (80) in 0x826E* FMOD/DSP area; unanalyzed this iter. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
