## Iteration 106

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82706070 | sub_82706070 | FM2_SQLite_VprintfCore | 0.94 | Full printf engine: parses `%` flags/width/precision, emits literals and formatted ints/strings; references `(NULL)` / hex digit tables. Called by `VsnprintfWithCallback`. |
| 0x82706F18 | sub_82706F18 | FM2_SQLite_PrintfAppendChunk | 0.91 | Growable-buffer append callback: doubles capacity via realloc fn ptr; copies chunk with `FM2_MemcpyAligned`; null-terminates. Used as callback to `VprintfCore`. |
| 0x82707020 | sub_82707020 | FM2_SQLite_VsnprintfWithCallback | 0.92 | Drives `VprintfCore` then optionally realloc+copy when stack buffer filled; allocator callback at arg0. Called from `FM2_SQLite_Vsnprintf` and `Printf`. |
| 0x82707148 | sub_82707148 | FM2_SQLite_Printf | 0.90 | Variadic wrapper: 350-byte stack buf, calls `VsnprintfWithCallback` with `j_FM2_SQLite_ReallocOrAllocZeroed`. 31 callers across SQL codegen. |
| 0x826F0378 | sub_826F0378 | FM2_SQLite_SchemaHashInsertOrReplace | 0.91 | Hash insert/replace in schema table: computes ascii/collated hash, walks chain, allocates 20-byte node, may resize bucket array, stores payload at `[2]`. |
| 0x826F0FA0 | sub_826F0FA0 | FM2_SQLite_SchemaRemoveTableAndTriggers | 0.90 | Removes table from schema hash (+4), unlinks trigger entries from secondary hash (+88), calls `FM2_SQLite_ParseTriggerDestroyDeferred`, sets context flag +8 bit 0x10. |
| 0x826EFC60 | sub_826EFC60 | FM2_SQLite_ValueToErrorObject | 0.92 | Converts SQLite value types (`V`/`\|`\`{` text, `S` negates numeric, `}` blob→text) into `AllocErrorObject` + `ErrorObjectSetMessage`. |
| 0x826FFEF8 | sub_826FFEF8 | FM2_SQLite_ErrorObjectStripUtf16Bom | 0.91 | Detects UTF-16 BE/LE BOM (`FE FF` / `FF FE`) at message start; rebinds text via `ErrorObjectSetMessage` with type 2/3 and optional free callback. |
| 0x82700020 | sub_82700020 | FM2_SQLite_CountUtf8Chars | 0.88 | Walks UTF-8 (incl. surrogate-pair style sequences for astral codepoints); returns byte length / char count. Used when computing UTF-16 text length in `ErrorObjectSetMessage`. |
| 0x826ED810 | sub_826ED810 | FM2_SQLite_DequoteIdentifier | 0.93 | Strips surrounding `'\"`[\`]` quotes from SQL identifiers; handles doubled closing quote escape. Used before storing dequoted text/blob values. |
| 0x826FF068 | sub_826FF068 | FM2_SQLite_Vdbe_AddOpcodeWithP4 | 0.92 | Thin helper: `Vdbe_AppendOpcodeRecord` then `Vdbe_SetOpcodeP4`. 61 callers across VDBE codegen paths. |
| 0x826FE9B0 | sub_826FE9B0 | FM2_SQLite_Vdbe_SetOpcodeP4 | 0.91 | Sets P4 operand on 20-byte opcode record: frees prior P4, supports string dup (`sub_826EE6C0`), static ptr, subprog (-6), keyinfo (-9), etc. |

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
| 0x826ED7B8 | sub_826ED7B8 | Clears context error ptr at +8/+24 only; defer with context helpers. |
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826F34C8 | sub_826F34C8 | Large DROP TABLE codegen; defer dedicated DDL pass with `sub_8270FF28`. |
| 0x826FCBA0 | sub_826FCBA0 | Parse-context alloc; defer with `sub_82709EE0` cluster. |
| 0x826FD950 | sub_826FD950 | Vdbe column text setter; defer iter 107 with more VDBE helpers. |
| 0x82709EE0 | sub_82709EE0 | Thin get-or-create parse context wrapper only. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
