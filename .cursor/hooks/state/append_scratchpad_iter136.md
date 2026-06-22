## Iteration 136

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826F47C0 | sub_826F47C0 | FM2_SQLite_CodeGen_ReindexObject | 0.91 | REINDEX statement: resolves db/table/index, calls `ReindexTableIndexes` or `CreateIndex`; error "unable to identify the object to be reindexed". |
| 0x826F4700 | sub_826F4700 | FM2_SQLite_CodeGen_ReindexTableIndexes | 0.90 | Rebuilds all indexes on table matching collation name; `OpenSchemaForWrite` + `CreateIndex` loop. |
| 0x826F21B8 | sub_826F21B8 | FM2_SQLite_Schema_RemapTriggerRootpages | 0.88 | Patches trigger/index rootpage field (+20) in schema hash chains when pgno remapped. |
| 0x826F59A8 | sub_826F59A8 | FM2_SQLite_Btree_InsertCellPointer | 0.89 | Inserts cell offset into page cell pointer array; coalesces adjacent free blocks; caller page insert paths. |
| 0x826F56F0 | sub_826F56F0 | FM2_SQLite_Btree_DefragmentPage | 0.90 | Compacts page cells to eliminate fragmentation; `ParseCellHeader`, memmove cells, zeroes gap; caller `AllocateCellSpace`. |
| 0x826F6708 | sub_826F6708 | FM2_SQLite_Btree_InitializeFirstPage | 0.92 | Writes fresh page-1 header "SQLite format 3", page size fields, creates empty page 1 on new DB. |
| 0x826F67E8 | sub_826F67E8 | FM2_SQLite_Btree_BeginTransaction | 0.91 | Starts btree txn: parse header, `PreparePageWrite`, `InitializeFirstPage`, busy-handler retry on SQLITE_BUSY(5). |
| 0x826F2A38 | sub_826F2A38 | FM2_SQLite_CodeGen_BeginTransaction | 0.92 | BEGIN codegen: auth "BEGIN", opcodes 102 (savepoint) + 14; string literal in decompiler. |
| 0x826F2B28 | sub_826F2B28 | FM2_SQLite_CodeGen_CommitTransaction | 0.92 | COMMIT codegen: auth "COMMIT", opcode 14 P2=1; string literal in decompiler. |
| 0x826F5FE0 | sub_826F5FE0 | FM2_SQLite_Btree_OpenDatabaseFile | 0.89 | Opens/attaches database file: `:memory:` fast path, path canonicalization, shared-cache lookup, pager init. |
| 0x82701F20 | sub_82701F20 | FM2_SQLite_Value_ApplyNumericAffinity | 0.90 | Applies numeric affinity to mem value, sets type flag byte at +26 (1=int,2=real,3=text,4=blob,5=null). |
| 0x826F3348 | sub_826F3348 | FM2_SQLite_Parse_SetColumnCollation | 0.88 | Sets COLLATE on last column in CREATE TABLE: `LookupCollation`, strdup name, updates index collations. |

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
| 0x826F6E98 | sub_826F6E98 | Btree begin-write helper; defer pager cluster (`sub_82713D38`). |
| 0x82708298 | sub_82708298 | Expr register assignment recurse; defer expr cluster. |
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
