## Iteration 117

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827127C0 | sub_827127C0 | FM2_SQLite_Pager_ReadJournalRecord | 0.91 | Reads journal via `VfsReadSchemaCallback`; parses two `Vfs_ReadUint24BE` fields; validates 8-byte magic `qword_8211A864`; allocates and checksums journal record buffer. |
| 0x82712948 | sub_82712948 | FM2_SQLite_Pager_SyncJournalHeader | 0.89 | Aligns journal write offset to sector size (+168), fills header via `RandomFillBuffer`, syncs pager file handle via vtable dispatch. Called from `Btree_ReadDatabaseHeader`. |
| 0x826EF290 | sub_826EF290 | FM2_SQLite_ValueConvertFloatToIntIfExact | 0.92 | Casts double at +8 to int64 in-place; sets integer flag (+24 bit 4) when float value is exactly integral. |
| 0x82718A90 | sub_82718A90 | FM2_SQLite_ExecWithRowCallback | 0.90 | `Prepare` then loops `Vdbe_Step` while ROW (100); invokes row callback via `Statement_GetColumnInt` + `ExecOneStatement` per row; finalizes stmt. |
| 0x82718A28 | sub_82718A28 | FM2_SQLite_ExecOneStatement | 0.91 | `Prepare` single SQL string; drains all ROW results with `Vdbe_Step`; returns finalize result via `Statement_Finalize`. |
| 0x826FE8A8 | sub_826FE8A8 | FM2_SQLite_Vdbe_InvalidateAllStatements | 0.90 | Walks db statement list at +72; sets stale flag byte at +568 on each VDBE. Called when UDF/schema changes. |
| 0x826EC570 | sub_826EC570 | FM2_SQLite_Vdbe_Step | 0.93 | Validates stmt magic -1108210269; handles progress/profile callbacks; calls `Vdbe_ExecuteProgram` or explain stepper; maps result to error via `ContextSetErrorV`. |
| 0x826ED278 | sub_826ED278 | FM2_SQLite_Prepare | 0.92 | Core prepare: `ParseSqlString`, schema-lock checks, emits VDBE; error `"database schema is locked: %s"`; EXPLAIN output strings `opcode`/`order`/`detail`. |
| 0x826FF0A8 | sub_826FF0A8 | FM2_SQLite_Statement_Finalize | 0.91 | Validates stmt magic types; calls internal teardown `sub_826FEE60`; frees statement object via `sub_826FEF78`. |
| 0x826EC8B0 | sub_826EC8B0 | FM2_SQLite_Statement_GetColumnMem | 0.92 | Returns 64-byte Mem pointer for column index from stmt result array (+44); handles pseudo-column offset (+548); error 25 on out-of-range. |
| 0x826EECB8 | sub_826EECB8 | FM2_SQLite_RandomFillBuffer | 0.90 | Fills `n` bytes via RC4 `sub_826EEB88` PRNG; used for journal/randomness and crypto salt generation. |
| 0x82701FA0 | sub_82701FA0 | FM2_SQLite_Vdbe_ExecuteProgram | 0.93 | 14KB VDBE opcode interpreter switch; strings for schema queries (`sqlite_master`), transaction errors, table lock messages; called from `Vdbe_Step`. |

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
| 0x826EC028 | sub_826EC028 | Returns db error code at +12; defer thin API wrappers. |
| 0x826EC9F8 | sub_826EC9F8 | Thin wrapper around `GetColumnMem` + affinity; defer. |
| 0x826EEB88 | sub_826EEB88 | RC4 PRNG byte; defer with crypto random cluster. |
| 0x826FD3C0 | sub_826FD3C0 | EXPLAIN-mode step handler; defer explain cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
