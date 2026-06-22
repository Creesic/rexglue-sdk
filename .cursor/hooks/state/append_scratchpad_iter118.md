## Iteration 118

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826EC028 | sub_826EC028 | FM2_SQLite_Database_FinishOpen | 0.90 | After provisional db alloc: checks OOM/close state; returns open result code at db+12. Called from `Database_Open`. |
| 0x826EC9F8 | sub_826EC9F8 | FM2_SQLite_Statement_GetColumnInt | 0.91 | Gets column Mem via `Statement_GetColumnMem`, applies integer affinity, normalizes result code. Used by exec row callback. |
| 0x826EEB88 | sub_826EEB88 | FM2_SQLite_RandomNextByte | 0.93 | RC4 PRNG: lazy key schedule from `dword_82A3CE94` seed; returns next pseudo-random byte. Called from `RandomFillBuffer`. |
| 0x826FD3C0 | sub_826FD3C0 | FM2_SQLite_Vdbe_StepExplain | 0.90 | EXPLAIN mode stepper: advances opcode index, emits opcode name from `off_8211AD70`, returns ROW/DONE; handles interrupt error 9. |
| 0x826FEE60 | sub_826FEE60 | FM2_SQLite_Statement_ResetInternal | 0.91 | Validates stmt magic, runs `sub_826FEB30` cleanup, clears transient state, marks stmt dead magic 649915045; may detach on schema error 17. |
| 0x826FEF78 | sub_826FEF78 | FM2_SQLite_Statement_Destroy | 0.92 | Unlinks stmt from db list, frees opcodes/registers/labels, calls `Vdbe_ClearTransientState`, frees stmt memory; sets dead magic -1241070648. |
| 0x826FD7D0 | sub_826FD7D0 | FM2_SQLite_Vdbe_ClearTransientState | 0.90 | Releases result Mem array, subquery cursors, sort buffers, and aux data during reset/finalize. |
| 0x826EE9D8 | sub_826EE9D8 | FM2_SQLite_NormalizeResultOrOom | 0.91 | Maps any pending result to SQLITE_NOMEM (7) when soft heap limit hit; otherwise returns passed-through code. Used throughout stmt API. |
| 0x826EF1B0 | sub_826EF1B0 | FM2_SQLite_ValueToDouble | 0.92 | Coerces Mem to double: fast paths for float/int types; else parses UTF-8 text via `sub_826EDBD8`. |
| 0x826EC950 | sub_826EC950 | FM2_SQLite_Statement_GetColumnDouble | 0.91 | Wraps `Statement_GetColumnMem` + `ValueToDouble`; updates stmt error code via `NormalizeResultOrOom`. |
| 0x826FD768 | sub_826FD768 | FM2_SQLite_BtreeCursor_Destroy | 0.90 | Closes btree cursor via `sub_826FB290`, frees aux structures, releases cursor object. Used during VDBE teardown. |
| 0x826EC1E0 | sub_826EC1E0 | FM2_SQLite_Database_Open | 0.93 | `sqlite3_open` core: alloc db, register BINARY/NOCASE collations, open main/temp via `sub_826EBF18`, strings `"main"`/`"temp"`, finalize via `Database_FinishOpen`. |

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
| 0x826EEAD8 | sub_826EEAD8 | Thin `AllocZeroed` callback only. |
| 0x826EEB78 | sub_826EEB78 | Returns `&dword_82A3CE70` only; defer mem-methods getter. |
| 0x826EF998 | sub_826EF998 | Thin wrapper → `ErrorObjectSetMessage` only. |
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826E07D0 | sub_826E07D0 | NUISPEECH `CVoiceInput` lifecycle; defer XUI/speech cluster. |
| 0x826E5690 | sub_826E5690 | XAudio2 voice pool IP formatting; defer networking/audio cluster. |
| 0x826E64E0 | sub_826E64E0 | XAudio voice object heap cleanup; defer audio cluster. |
| 0x826EC450 | sub_826EC450 | Thin wrapper → `Statement_Finalize` only. |
| 0x826EC088 | sub_826EC088 | CreateCollation API; defer iter 119 collation cluster. |
| 0x826FEB30 | sub_826FEB30 | Large statement reset helper; defer with reset cluster. |
| 0x8271BA20 | sub_8271BA20 | WHERE term analyzer (1.9KB); defer dedicated WHERE planner pass. |
| 0x8271A840 | sub_8271A840 | WhereLevel grow array; defer WHERE planner pass. |
| 0x827112F0 | sub_827112F0 | Schema read-cursor opener; defer DDL codegen cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
