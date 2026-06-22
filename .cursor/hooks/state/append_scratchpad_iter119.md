## Iteration 119

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826EC088 | sub_826EC088 | FM2_SQLite_CreateCollation | 0.94 | `sqlite3_create_collation` semantics: validates encoding (`"unknown encoding"`), blocks when active stmts (`"Unable to delete/modify collation sequence..."`), registers factory via `GetCollationByEncodingSlot`. |
| 0x826FEB30 | sub_826FEB30 | FM2_SQLite_Statement_ResetVdbeState | 0.90 | Validates stmt magic -1108210269; closes btree cursors via `BtreeCursor_Destroy`; on OOM/abort scans opcodes 38/102 and may rollback via `InvalidateSiblingStatements` + `sub_826EBA80`. Called from `Statement_ResetInternal`. |
| 0x8271A840 | sub_8271A840 | FM2_SQLite_WhereLevel_Add | 0.92 | Grows WHERE plan array (28-byte entries, doubles capacity); stores src pointer, flags byte, parent WhereInfo; 8 callers in `0x8271*`. |
| 0x827112F0 | sub_827112F0 | FM2_SQLite_SrcList_OpenReadCursors | 0.90 | Walks SrcList entries: `ContextLookupTable`, deferred trigger destroy, stores table ptr and bumps table refcount (+36). Used during SELECT/DDL codegen. |
| 0x8271BA20 | sub_8271BA20 | FM2_SQLite_WhereAnalyzeTerm | 0.88 | WHERE term analyzer (1.9KB): `ExprComputeUsedTableMask` on term expr, builds constraint masks; 6 callers from WHERE planner (`0x8271BD64` etc.). |
| 0x826EC800 | sub_826EC800 | FM2_SQLite_Vdbe_GetOrAllocAuxData | 0.91 | Lazily allocates VDBE aux-data blob (inline if ≤32 bytes else heap); sets destructor `FreeIfNonNull`; 8 callers from `Vdbe_ExecuteProgram`. |
| 0x826FDE50 | sub_826FDE50 | FM2_SQLite_Database_InvalidateSiblingStatements | 0.90 | Walks db stmt list; for other live stmts closes cursors and sets expired flag (+567); used on schema change/rollback paths. |
| 0x826ED980 | sub_826ED980 | FM2_SQLite_StringLooksLikeNumber | 0.92 | Validates numeric UTF-8/UTF-16 text (digits, optional sign, decimal, exponent); sets hasReal flag; used by value parsing and datetime code. |
| 0x826EC9A0 | sub_826EC9A0 | FM2_SQLite_Statement_GetColumnInt64 | 0.91 | `Statement_GetColumnMem` + `ValueToInt64` + `NormalizeResultOrOom`; public column int64 getter. |
| 0x826ECB40 | sub_826ECB40 | FM2_SQLite_Schema_ValidateTableDefinition | 0.91 | Parses CREATE TABLE AST: runs SQL via `sub_82701A90`, error `"malformed database schema"`; updates table hash rootpage on success. |
| 0x826EC898 | sub_826EC898 | FM2_SQLite_Statement_GetColumnCount | 0.93 | Returns `stmt+548` result-column count (0 if null stmt); used by game code and VDBE helpers. |
| 0x826F7A20 | sub_826F7A20 | FM2_SQLite_Btree_AllocatePage | 0.89 | Allocates new btree page: decrements/increments page-count header, `Btree_GetOrCreateCursor`, `Btree_DirtyPage`, freelist alignment; 7 btree callers. |

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
| 0x826EC450 | sub_826EC450 | Thin wrapper → `Statement_Finalize` only. |
| 0x826ECB00 | sub_826ECB00 | Thin wrapper → `Statement_GetColumnValue` with name offset 0; defer column-name cluster. |
| 0x826ECB10 | sub_826ECB10 | Thin wrapper → `Statement_GetColumnValue` with table offset 1; defer column-name cluster. |
| 0x826ECA78 | sub_826ECA78 | Generic column Mem accessor; defer with column-name wrappers. |
| 0x826EC4F0 | sub_826EC4F0 | Thin wrapper → `ErrorObjectSetMessage` at db+8; defer. |
| 0x826EFE48 | sub_826EFE48 | Initializes default Mem vtable (xMalloc/xFree); defer mem-methods cluster. |
| 0x826FCFF0 | sub_826FCFF0 | Sets dword in 20-byte sort/plan entry; caller context unclear. |
| 0x826F8030 | sub_826F8030 | Btree page free path; pair with allocate — defer iter 120. |
| 0x826EBA80 | sub_826EBA80 | Closes all btree handles on db; needs paired analysis with reset path. |
| 0x826FB290 | sub_826FB290 | Btree cursor unlink+free; defer btree cursor cluster. |
| 0x82716C80 | sub_82716C80 | DateTime time-of-day decompose; defer datetime cluster (`DateTime_DecomposeJulianDay` nearby). |
| 0x827109B8 | sub_827109B8 | FK/trigger constraint matcher; defer schema constraint cluster. |
| 0x8271C1A0 | sub_8271C1A0 | Large WHERE planner driver (4.2KB); defer dedicated pass. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
