## Iteration 130

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82715F90 | sub_82715F90 | FM2_SQLite_Pager_EndWriteTransaction | 0.90 | Ends pager write txn: frees overflow staging or calls `Pager_Commit`+`Pager_Reset`; sole caller `Btree_EndTransaction`. |
| 0x826F6A88 | sub_826F6A88 | FM2_SQLite_Btree_UpdateCellPageRef | 0.89 | Patches child page numbers in btree cells (types 3/4/5) during ptrmap relocate; caller `Btree_RelocatePagePtrmap`. |
| 0x8270D658 | sub_8270D658 | FM2_SQLite_SrcList_CloneDeep | 0.90 | Deep-clones SrcList entries via `StrdupCString`, `Select_Dup`, `ParseTree_DupNode`, `IdList_CloneWithStrings`; used by `Select_Dup`. |
| 0x82700FE8 | sub_82700FE8 | FM2_SQLite_Vdbe_OpLikeWithEscape | 0.91 | LIKE ESCAPE opcode handler; error string "ESCAPE expression must be a single character"; uses `Utf8_CountCodepoints`. |
| 0x82713E80 | sub_82713E80 | FM2_SQLite_Pager_ResetCommitState | 0.89 | Post-commit journal cleanup: frees journal buffer, clears dirty-page write chain; caller `Pager_FinishCommit`. |
| 0x82714460 | sub_82714460 | FM2_SQLite_Pager_SyncJournalToDisk | 0.90 | Writes journal sector batches via `VdbeExecStep`; caller `Pager_FinishCommit`. |
| 0x82712AC0 | sub_82712AC0 | FM2_SQLite_Pager_ReadJournalHeader | 0.90 | Reads journal header (magic, page count) via VFS; used by `Pager_RecoverFromJournal`. |
| 0x82715B10 | sub_82715B10 | FM2_SQLite_Pager_CopyPageContent | 0.88 | `HashLookupOrInsert` + `Btree_DirtyPage` + `MemcpyAligned`; used by `Btree_CopyPagesOnAttach`. |
| 0x826F6950 | sub_826F6950 | FM2_SQLite_Btree_InsertOverflowCellsOnPage | 0.88 | Reinserts overflow cells into freelist trunk during ptrmap relocate; caller `Btree_RelocatePagePtrmap`. |
| 0x82718B60 | sub_82718B60 | FM2_SQLite_Vacuum_RebuildDatabase | 0.92 | Full VACUUM: `ATTACH '%q' AS vacuum_db`, schema copy SQL strings, transaction checks. |
| 0x827100A8 | sub_827100A8 | FM2_SQLite_ParseNode_CloneDeep | 0.89 | Deep-clones parse trigger/node: dup select, expr tree, id lists; used by trigger/view DDL paths. |
| 0x82700BF0 | sub_82700BF0 | FM2_SQLite_Expr_LikeMatch | 0.90 | LIKE pattern matcher with escape/charset support; called from `Vdbe_OpLikeWithEscape` and related expr paths. |

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
| 0x826FB848 | sub_826FB848 | Btree overflow-page allocate helper; defer auto-vacuum cluster tail. |
| 0x826FC708 | sub_826FC708 | Btree overflow-page free helper; defer auto-vacuum cluster tail. |
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
