## Iteration 120

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826F8030 | sub_826F8030 | FM2_SQLite_Btree_FreePage | 0.90 | Inverse of `Btree_AllocatePage`: increments page-count header, links page into freelist trunk via `Btree_SetFreelistTrunkEntry`, updates first-freepage pointer; 7 btree callers. |
| 0x826EBA80 | sub_826EBA80 | FM2_SQLite_Database_CloseAllBtrees | 0.90 | Walks attached-db array (24-byte entries); calls `Btree_Close` on each open btree; may detach temp db; invokes rollback callback. Used from `Statement_ResetVdbeState`. |
| 0x826FB290 | sub_826FB290 | FM2_SQLite_BtreeCursor_UnlinkAndFree | 0.91 | Unlinks cursor from btree list, releases pinned page via `HashEntry_Release` + `Btree_PageRelease`, frees cursor object; called from `BtreeCursor_Destroy` and `BtreeCursor_CloseAllOnBtree`. |
| 0x82716C80 | sub_82716C80 | FM2_SQLite_DateTime_DecomposeTimeOfDay | 0.88 | Lazy decompose of time fraction into hours (+20), minutes (+24), seconds (+32 double); pairs with `DateTime_DecomposeJulianDay` at `0x82716AE8`; 7 datetime callers. |
| 0x826EFE48 | sub_826EFE48 | FM2_SQLite_MemMethods_InitDefault | 0.92 | Initializes sqlite Mem methods struct: type bytes, xMalloc=`sub_826EEAD8`, xFree=`FreeIfNonNull`; called during `Database_Open` collation registration. |
| 0x826EFE80 | sub_826EFE80 | FM2_SQLite_MemMethods_DestroyChain | 0.90 | Walks linked allocation chain at +8, invokes destructor at +16 for each node, clears counters; paired with `MemMethods_InitDefault`. |
| 0x826ECB00 | sub_826ECB00 | FM2_SQLite_Statement_GetColumnName | 0.93 | Thin `sqlite3_column_name`: `Statement_AccessColumnMem` + `AppendLowercaseIdentifierAlt` with name-array offset 0. |
| 0x826ECB10 | sub_826ECB10 | FM2_SQLite_Statement_GetColumnTableName | 0.92 | Thin `sqlite3_column_table_name`: same accessor with offset 1 into column-name table. |
| 0x826ECA78 | sub_826ECA78 | FM2_SQLite_Statement_AccessColumnMem | 0.91 | Bounds-checks column index against `stmt+548`, indexes 64-byte Mem array at `stmt+52`, applies callback to selected Mem field. |
| 0x826F1740 | sub_826F1740 | FM2_SQLite_ParseContext_LookupCollation | 0.92 | Resolves collation sequence for parse context; error `"no such collation sequence: %.*s"`; 7 callers in expr/WHERE codegen. |
| 0x826FBF80 | sub_826FBF80 | FM2_SQLite_Btree_Close | 0.90 | Invalidates sibling stmts, unlinks btree savepoint list, decrements write-lock count, releases root page; called from `Database_CloseAllBtrees` and cursor teardown. |
| 0x826EEB78 | sub_826EEB78 | FM2_SQLite_GetMemMethodsTable | 0.91 | Returns `&dword_82A3CE70`; FM2 init at `0x825CF668` walks successive offsets installing allocator hooks into this global table. |

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
| 0x826EC4F0 | sub_826EC4F0 | Thin wrapper → `ErrorObjectSetMessage` at db+8; defer. |
| 0x826FCFF0 | sub_826FCFF0 | Sets dword in 20-byte pragma/expr entry; used from PRAGMA handler — defer. |
| 0x826FCAB0 | sub_826FCAB0 | Closes all cursors on btree then frees; defer btree cursor cluster pass. |
| 0x826FABE0 | sub_826FABE0 | Returns whether btree savepoint type == 2; too small alone. |
| 0x826FA0A8 | sub_826FA0A8 | Reads rootpage from meta hash page; defer btree meta cluster. |
| 0x826FB4D0 | sub_826FB4D0 | Btree cursor advance/next (452B); defer cursor navigation cluster. |
| 0x82713AC8 | sub_82713AC8 | Pager cache page invalidation; defer pager cache cluster. |
| 0x827109B8 | sub_827109B8 | FK/trigger constraint matcher; defer schema constraint cluster. |
| 0x8271C1A0 | sub_8271C1A0 | Large WHERE planner driver (4.2KB); defer dedicated pass. |
| 0x8271FD40 | sub_8271FD40 | PRAGMA handler (5.2KB); defer dedicated pass. |
| 0x82729920 | sub_82729920 | Render/math sinc kernel; outside SQLite cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
