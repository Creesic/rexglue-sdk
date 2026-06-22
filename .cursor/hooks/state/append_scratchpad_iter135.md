## Iteration 135

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826F0BB8 | sub_826F0BB8 | FM2_SQLite_Schema_ReplaceHashEntry | 0.90 | Inserts schema hash entry then unlinks/frees prior duplicate from chain; sets schema change flag 0x10. |
| 0x826F12F0 | sub_826F12F0 | FM2_SQLite_Parse_CheckReservedObjectName | 0.92 | Rejects names with `sqlite_` prefix; error "object name reserved for internal use: %s". |
| 0x826F19C0 | sub_826F19C0 | FM2_SQLite_Schema_BuildCreateTableSql | 0.91 | Builds CREATE TABLE SQL with strings `CREATE TABLE `, `CREATE TEMP TABLE `, column formatting. |
| 0x82705ED8 | sub_82705ED8 | FM2_SQLite_Parse_ApplyColumnAffinityString | 0.88 | Parses digit affinity string into table column affinity array via `LookupTableBySchemaHash`. |
| 0x826F0680 | sub_826F0680 | FM2_SQLite_CodeGen_FinishSelectResult | 0.90 | Finishes SELECT codegen: opcodes 34/102/97/91/19, column metadata, result set P4 blob. |
| 0x826F1660 | sub_826F1660 | FM2_SQLite_Parse_SetColumnDefaultExpr | 0.91 | Sets column DEFAULT expr; error "default value of column [%s] is not constant" if non-constant. |
| 0x826F1878 | sub_826F1878 | FM2_SQLite_Schema_QuoteIdentifier | 0.89 | Quotes SQL identifier with optional double-quotes; keyword/char-class checks via `LookupKeywordToken`. |
| 0x826F20D8 | sub_826F20D8 | FM2_SQLite_Schema_ClearPendingAlterTable | 0.88 | Clears pending ALTER TABLE state (flag 0x2 at +122): frees column defs and parse trees. |
| 0x826F4350 | sub_826F4350 | FM2_SQLite_CodeGen_DropIndex | 0.92 | DROP INDEX codegen: auth checks, `DELETE FROM %Q.%s WHERE name=%Q`, rootpage update SQL. |
| 0x826F5808 | sub_826F5808 | FM2_SQLite_Btree_AllocateCellSpace | 0.89 | Allocates/free-list cell space on page; adjusts cell pointer array; caller `InsertCellsOnPage`. |
| 0x826F2218 | sub_826F2218 | FM2_SQLite_CodeGen_UpdateIndexRootpages | 0.90 | Reindex helper: opcode 108 + `UPDATE %Q.%s SET rootpage=%d WHERE #0 AND rootpage=#0`. |
| 0x82700AD8 | sub_82700AD8 | FM2_SQLite_Vdbe_OpRandom | 0.90 | random() SQL func: `RandomFillBuffer(8)` then pushes int64 result. |

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
| 0x826EF998 | sub_826EF998 | Thin wrapper → `ErrorObjectSetMessage` only. |
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826EC450 | sub_826EC450 | Thin wrapper → `Statement_Finalize` only. |
| 0x826EC510 | sub_826EC510 | Thin wrapper → `Mem_SetRowid(mem+8)` only. |
| 0x826FABB0 | sub_826FABB0 | Thin wrapper → sub_8242A348 (12 bytes). |
| 0x826F6390 | sub_826F6390 | Thin wrapper (12 bytes). |
| 0x826FABC0 | sub_826FABC0 | Thin wrapper (12 bytes). |
| 0x826FABD0 | sub_826FABD0 | Thin wrapper (12 bytes). |
| 0x826FAC88 | sub_826FAC88 | Single-line `Btree_CheckMetaPageLock` wrapper; too thin alone. |
| 0x826F14A8 | sub_826F14A8 | Sets FK action byte on last constraint only (44 bytes); too thin alone. |
| 0x826F21B8 | sub_826F21B8 | Remaps trigger rootpage refs; defer trigger cluster. |
| 0x826F47C0 | sub_826F47C0 | REINDEX codegen; defer with reindex cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
| 0x826E1058 | sub_826E1058 | Low-level bit/mem transform; no strings; purpose unclear. |
| 0x826E1C38 | sub_826E1C38 | XAudio2 leap buffer path; outside SQLite cluster. |
| 0x826E8838 | sub_826E8838 | Win32 critical-section state machine; domain unclear. |
| 0x827294D8 | sub_827294D8 | D3D render pass setup; outside SQLite cluster. |
| 0x82729920 | sub_82729920 | Render/math sinc kernel; outside SQLite cluster. |
| 0x82721BD8 | sub_82721BD8 | Render matrix upload; outside SQLite cluster. |
| 0x8272B630 | sub_8272B630 | D3D texture size alignment; outside SQLite cluster. |
| 0x8272D308 | sub_8272D308 | Shader string-table lookup; outside SQLite cluster. |
| 0x82725DB8 | sub_82725DB8 | VMX128 normalize vector; outside SQLite cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
