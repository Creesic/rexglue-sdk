## Iteration 126

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8271D2B8 | sub_8271D2B8 | FM2_SQLite_CodeGen_UpdateFromSelect | 0.91 | UPDATE FROM SELECT codegen (3.1KB): auth, writable-target check, FK/index helpers; error strings `"no such column: %s"`, `"rows updated"`; 2 parse callers. |
| 0x8271EB60 | sub_8271EB60 | FM2_SQLite_CodeGen_InsertFromSelect | 0.92 | INSERT FROM SELECT codegen (3.8KB): column-count validation (`%d values for %d columns`), `"rows inserted"`; shares DML path with Update/Delete. |
| 0x8271E0F0 | sub_8271E0F0 | FM2_SQLite_CodeGen_EnforceUniqueConstraints | 0.90 | NOT NULL/UNIQUE enforcement during INSERT/UPDATE: emits VDBE compare/backpatch; strings `"PRIMARY KEY must be unique"`, `" may not be NULL"`; called from Insert/Update codegen. |
| 0x8271AD00 | sub_8271AD00 | FM2_SQLite_Where_EstimateIndexCost | 0.89 | WHERE index cost estimator: `Schema_FindIndexForColumn`, row-count heuristics, returns best index/cost; sole caller `Where_BuildJoinPlan`. |
| 0x826FA640 | sub_826FA640 | FM2_SQLite_Btree_CheckPageIntegrity | 0.91 | Btree integrity checker: walks page cells, validates depth/ptrmap/fragmentation; diagnostic strings `"Corruption detected in cell %d on page %d"`, `"Page %d: "`. |
| 0x826F4C08 | sub_826F4C08 | FM2_SQLite_CodeGen_CreateViewOrTable | 0.90 | CREATE TABLE/VIEW DDL: updates `sqlite_master`/`sqlite_temp_master`, `sqlite_sequence`; rejects view parameters; caller `ParseTable_Reduce`. |
| 0x827179B0 | sub_827179B0 | FM2_SQLite_DateTime_FormatString | 0.91 | strftime-style formatter: validates tokens via `DateTime_TokensMatchInterval`, expands `%Y/%m/%d/%H/%M/%S/%f/%w` etc. into output buffer. |
| 0x82716298 | sub_82716298 | FM2_SQLite_ScanDecimalDigits | 0.88 | Scans consecutive decimal digits from input using char-class table; used by ISO8601 date/time and timezone parsers. |
| 0x827163D8 | sub_827163D8 | FM2_SQLite_DateTime_ParseTimezoneOffset | 0.89 | Parses signed timezone offset (`±HH:MM`) after datetime; stores seconds offset at context+28; called from `ParseIso8601Time`. |
| 0x82710668 | sub_82710668 | FM2_SQLite_SrcList_GetUpdateColumnFlags | 0.88 | Walks ON/UPDATE clause list OR-ing column flags when IdList names match; used by Delete/Insert/Update DML codegen. |
| 0x82711378 | sub_82711378 | FM2_SQLite_CodeGen_CheckWritableTarget | 0.92 | Pre-DML guard: errors `"table %s may not be modified"` and `"cannot modify %s because it is a view"`; shared by Delete/Insert/Update paths. |
| 0x826F4A38 | sub_826F4A38 | FM2_SQLite_CodeGen_ApplyPrimaryKey | 0.90 | PRIMARY KEY clause handler: marks PK columns, rejects multiple PKs/`AUTOINCREMENT` misuse; may call `CodeGen_CreateIndexStatement`. |

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
| 0x826FDA00 | sub_826FDA00 | Multi-db transaction commit (`%s-mj%08X`); defer transaction cluster. |
| 0x82705810 | sub_82705810 | Index constraint VDBE builder; defer index-codegen cluster. |
| 0x827107C0 | sub_827107C0 | DML parse reducer dispatch; needs fuller read. |
| 0x8271ABD8 | sub_8271ABD8 | WHERE cost helper; defer with planner cluster. |
| 0x826FA2B0 | sub_826FA2B0 | Btree integrity prereq check; defer with integrity cluster. |
| 0x826FBAC0 | sub_826FBAC0 | Btree integrity driver; defer dedicated pass. |
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
