## Iteration 114

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82714BB8 | sub_82714BB8 | FM2_SQLite_Btree_DirtyPage | 0.89 | Links page into btree dirty list (+124), sets dirty flag (+15), updates page bitmap at pager+68/+72, may realloc page buffer and dispatch pager write; 27 btree callers. |
| 0x82713590 | sub_82713590 | FM2_SQLite_Btree_GetPageCount | 0.90 | When page count field negative, reads file size via `VfsReadSchemaCallback` and derives page count from page size (+48); clamps overflow edge at `0x40000000/page_size`. 18 callers. |
| 0x826F6678 | sub_826F6678 | FM2_SQLite_Btree_PageRelease | 0.88 | When page not dirty and refcnt zero, returns pager hash entry (+68) and clears page cache link (+8); may register page in pager LRU at btree+16/+17. |
| 0x8270B7F0 | sub_8270B7F0 | FM2_SQLite_IdList_RemapTableTokens | 0.91 | Walks IdList entries (12-byte stride) and rewrites each name via `ParseToken_CopyFromTable` during subquery merge. |
| 0x82714AF0 | sub_82714AF0 | FM2_SQLite_Btree_PreparePageWrite | 0.88 | Ensures pager write lock/state before dirtying page; may call `Btree_ReadDatabaseHeader` or `sub_82713718` to escalate lock level. |
| 0x82714990 | sub_82714990 | FM2_SQLite_Btree_ReadDatabaseHeader | 0.90 | Computes page count, allocates page bitmap, reads page 1 via VFS, initializes btree header fields and journal state via `sub_82712948`. |
| 0x826F76E0 | sub_826F76E0 | FM2_SQLite_Btree_SeekKey | 0.89 | Binary search on interior/table btree page for target int64 key: reads cell via `Btree_CellPtrAtIndex`, compares varint key, returns seek result in optional out-param. |
| 0x82711400 | sub_82711400 | FM2_SQLite_CodeGen_LoadColumn | 0.92 | Emits VDBE sequence: `RecordColumnUse`, opcode 45 (NullRow), column-load opcode, opcode 100 (Copy); 11 DDL/DML codegen callers. |
| 0x826EBD40 | sub_826EBD40 | FM2_SQLite_CreateUserFunction | 0.93 | Registers/replaces UDF via `FindUserFunction`; validates arity/encoding/flags; error `"Unable to delete/modify user-function due to active statements"`. |
| 0x826F1808 | sub_826F1808 | FM2_SQLite_CodeGen_NullRowAndLoadColumn | 0.91 | Emits opcode 45 with register `col+1` then opcode 3 (Column) for table column `a3`; 10 VDBE codegen callers. |
| 0x826F0598 | sub_826F0598 | FM2_SQLite_ParseContext_RecordColumnUse | 0.90 | Appends deduped 16-byte column-use record `{table,column,flag,expr}` to parse context array at +104/+108. |
| 0x8271A950 | sub_8271A950 | FM2_SQLite_Schema_FindIndexForColumn | 0.90 | Scans schema index list (28-byte entries) matching table/column/flags; checks collation compatibility via `sub_8270CAE0` and `StrcasecmpCollated`. |

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
| 0x826FE8A8 | sub_826FE8A8 | Marks `+568` on VDBE statement chain only; defer stmt-invalidate cluster. |
| 0x82713718 | sub_82713718 | Pager lock-level acquire with busy retry; defer pager lock pass. |
| 0x8270CAE0 | sub_8270CAE0 | Collate-opcode compatibility test; defer with index-resolution cluster. |
| 0x826F73A8 | sub_826F73A8 | Btree cursor page fetch; defer cursor lifecycle pass. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
