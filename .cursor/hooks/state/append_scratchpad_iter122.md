## Iteration 122

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826F50E0 | sub_826F50E0 | FM2_SQLite_Btree_CheckMetaPageLock | 0.91 | Before meta-page read: walks btree lock list at +64, returns SQLITE_LOCKED (6) if another connection holds conflicting page lock; paired with `Btree_AcquireMetaPageLock`. |
| 0x826F5190 | sub_826F5190 | FM2_SQLite_Btree_AcquireMetaPageLock | 0.91 | Alloc/links 16-byte lock record on btree +64 with page number and lock level; used by `Btree_GetMetaValue` around meta reads. |
| 0x826F2E88 | sub_826F2E88 | FM2_SQLite_Index_BuildCollationKeyInfo | 0.90 | Allocates KeyInfo for index: per-column `ParseContext_LookupCollation` + sort-order bytes; consumed by `CodeGen_CreateIndex` as P4 -9. |
| 0x826F5638 | sub_826F5638 | FM2_SQLite_Btree_InsertOverflowCell | 0.89 | Parses cell header; when payload exceeds local space writes ptrmap entry type 3 via `Btree_SetFreelistTrunkEntry`; used during page insert/balance. |
| 0x826F87A8 | sub_826F87A8 | FM2_SQLite_Btree_DeleteInteriorCell | 0.90 | Removes cell from interior btree page, shifts cell pointer array, decrements cell count in page header; 6 page-balance callers. |
| 0x826F9DB8 | sub_826F9DB8 | FM2_SQLite_Btree_BalanceIfNeeded | 0.88 | Decides whether to balance btree page: checks child count vs fill factor, calls `sub_826F8D30` or `sub_826F99B0`; entry point after insert/delete. |
| 0x826F3750 | sub_826F3750 | FM2_SQLite_CodeGen_CreateIndex | 0.93 | CREATE INDEX codegen: auth check 27, emits OpenRead/OpenWrite + IdxInsert opcodes; UNIQUE index error `"indexed columns are not unique"`. |
| 0x826EDBD8 | sub_826EDBD8 | FM2_SQLite_ParseAsciiDouble | 0.92 | Parses UTF-8 text to double (sign, integer, fraction, exponent with overflow scaling); used by `ValueToDouble` and datetime functions. |
| 0x826FB128 | sub_826FB128 | FM2_SQLite_BtreeCursor_Open | 0.91 | Allocates 0x58-byte cursor, `Btree_FetchPage` for root/requested page, links into btree cursor list; returns BUSY/IOERR codes. |
| 0x826F2858 | sub_826F2858 | FM2_SQLite_IdList_FindIndexByName | 0.92 | Linear search IdList name table via `StrcasecmpCollated`; returns index or -1; used by FK matcher and DDL helpers. |
| 0x826EFA50 | sub_826EFA50 | FM2_SQLite_ValueCompare | 0.91 | Generic Mem comparison: handles NULL ordering, int/float/text/blob types, optional collation callback; used by VDBE sort and expr paths. |
| 0x826FE038 | sub_826FE038 | FM2_SQLite_ValueSerializedSize | 0.90 | Returns on-disk serial type byte-length for Mem value (varint int sizes 1-9, float=8, text/blob length formulas); used before record encoding. |

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
| 0x826FABE0 | sub_826FABE0 | Returns whether btree savepoint type == 2; too small alone. |
| 0x827109B8 | sub_827109B8 | FK/trigger constraint matcher (opcode 'c'); defer schema constraint cluster. |
| 0x827124C8 | sub_827124C8 | Recursive schema dependency walk; defer with constraint cluster. |
| 0x8270CE68 | sub_8270CE68 | Token/string dup helper; defer token cluster. |
| 0x8271C1A0 | sub_8271C1A0 | Large WHERE planner driver (4.2KB); defer dedicated pass. |
| 0x8271FD40 | sub_8271FD40 | PRAGMA handler (5.2KB); defer dedicated pass. |
| 0x82729920 | sub_82729920 | Render/math sinc kernel; outside SQLite cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
