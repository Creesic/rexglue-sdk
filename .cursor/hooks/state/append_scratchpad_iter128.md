## Iteration 128

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826FAE58 | sub_826FAE58 | FM2_SQLite_Btree_AutoVacuumPages | 0.90 | Pre-commit auto-vacuum: computes target freelist page, allocates/moves pages via `Btree_AllocatePage`/`sub_826F6CA8`, clears meta freelist fields; caller `Btree_CommitPager`. |
| 0x82705CC0 | sub_82705CC0 | FM2_SQLite_CodeGen_EmitTableIndexChecks | 0.91 | Per-database REINDEX driver: opens schema, `OpenStat1Table`, walks table index list calling `EmitIndexKeyCheck`, emits opcode 111. |
| 0x8270CFF0 | sub_8270CFF0 | FM2_SQLite_IdList_CloneWithStrings | 0.90 | Deep-copies IdList: allocates name array, `StrdupCString` per entry, preserves affinity flags; used by Insert/SELECT DML codegen. |
| 0x82713CF8 | sub_82713CF8 | FM2_SQLite_Btree_CachePagerGlobals | 0.87 | Copies 6 pager/btree fields into global scratch (`dword_82A3CFD4`..`CFE8`) for integrity-check context; caller `Btree_RunIntegrityCheck`. |
| 0x82705700 | sub_82705700 | FM2_SQLite_CodeGen_OpenStat1Table | 0.92 | Ensures `sqlite_stat1` exists (`CREATE TABLE %Q.sqlite_stat1(tbl,idx,stat)`), optional DELETE by tbl; emits OpenWrite/MakeRecord opcodes. |
| 0x82705D60 | sub_82705D60 | FM2_SQLite_CodeGen_ReindexTable | 0.89 | REINDEX/ANALYZE codegen entry: resolves target table or loops all DBs, runs stat1 setup + index key checks; caller `ParseTable_Reduce`. |
| 0x82714750 | sub_82714750 | FM2_SQLite_Pager_SetPageCount | 0.89 | Sets pager database size (page count), syncs journal via `SyncJournal`, truncates cache; called during commit and auto-vacuum. |
| 0x82712C10 | sub_82712C10 | FM2_SQLite_Pager_OpenWriteJournal | 0.90 | Opens write journal on pager (VFS dispatch chain), computes journal size from page count; first step in `Pager_Commit`. |
| 0x82713860 | sub_82713860 | FM2_SQLite_Pager_SyncJournal | 0.89 | Flushes journal pages to storage via VFS sync calls; resets dirty-page write chain; used in commit path. |
| 0x82713970 | sub_82713970 | FM2_SQLite_Pager_WriteCheckpointList | 0.88 | Writes checkpoint/savepoint records through journal linked list with lock level 4; tail of commit sequence. |
| 0x82715B70 | sub_82715B70 | FM2_SQLite_Pager_IncrementChangeCounter | 0.91 | Dirties page 1 and increments 4-byte change-counter at page header offset +80; called early in `Pager_Commit`. |
| 0x82714FC8 | sub_82714FC8 | FM2_SQLite_Pager_Rollback | 0.90 | Aborts pager transaction: frees dirty overflow buffers, truncates cache, resets state to 1 or calls `Pager_Reset`; used on auto-vacuum failure. |

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
| 0x82714600 | sub_82714600 | Pager cache truncate helper; defer pager cluster tail. |
| 0x826F53B0 | sub_826F53B0 | Ptrmap entry reader; defer btree ptrmap cluster. |
| 0x826F11A0 | sub_826F11A0 | Database index lookup by name; defer parse cluster. |
| 0x826FFFC8 | sub_826FFFC8 | UTF-8 codepoint counter via `byte_82118FF0`; defer string util cluster. |
| 0x82722808 | sub_82722808 | Large function; needs dedicated pass. |
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
