## Iteration 115

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82713718 | sub_82713718 | FM2_SQLite_Pager_SetLockLevel | 0.90 | Escalates pager lock byte at +10 via pager vtable xLock; retries on SQLITE_BUSY (5) through `BusyHandler_Invoke`. |
| 0x8270CAE0 | sub_8270CAE0 | FM2_SQLite_CollateOpcodeCompatible | 0.91 | Compares index-column collate opcode to expr collate via `ExprGetCollateOpcode`/`MergeCollateOpcodes`; BINARY(97) exact, NOCASE(98) permissive, else threshold ≥99. Used by `Schema_FindIndexForColumn`. |
| 0x826F73A8 | sub_826F73A8 | FM2_SQLite_BtreeCursor_LoadPage | 0.90 | Fetches btree page for cursor pgno (+20) via `Btree_FetchPage`; may reset seek via `BtreeCursor_ResetSeek`; reads right-child pgno from interior header when leaf. |
| 0x82725F38 | sub_82725F38 | FM2_SQLite_NextPow2 | 0.93 | Classic bit-smear on `n-1` then `+1`; returns smallest power-of-two ≥ input. 13 hash-table growth callers in `0x8272*`. |
| 0x8271B890 | sub_8271B890 | FM2_SQLite_ExprComputeUsedTableMask | 0.90 | Recursive expr walk: TK_REGISTER (148) maps to `1<<tableIndex`; AND/OR/SELECT terms OR child masks for WHERE clause table-usage analysis. |
| 0x8271B9B8 | sub_8271B9B8 | FM2_SQLite_ExprComputeIdListTableMask | 0.91 | OR-combines `ExprComputeUsedTableMask` across IdList entries (12-byte stride). |
| 0x826F7330 | sub_826F7330 | FM2_SQLite_BtreeCursor_GotoChildPage | 0.89 | Fetches child page via `Btree_FetchPage`, releases old page hash ref, resets cell index; returns 11 if child is empty leaf. |
| 0x826F5EF8 | sub_826F5EF8 | FM2_SQLite_Btree_FetchPage | 0.92 | Wraps `Btree_GetOrCreateCursor` then `Btree_InitPage` when page not yet initialized. |
| 0x826EBCC0 | sub_826EBCC0 | FM2_SQLite_BusyHandler_Invoke | 0.91 | Calls busy-handler function pointer with user data; increments/decrements retry counter at +8; returns handler result. |
| 0x826FAD18 | sub_826FAD18 | FM2_SQLite_BtreeCursor_ResetSeek | 0.90 | Clears cursor validity (+65), optionally re-seeks cached key (+68/+76/+80), frees temp seek buffer. |
| 0x826F5C30 | sub_826F5C30 | FM2_SQLite_Btree_InitPage | 0.89 | Parses btree page header bytes into cursor fields (cell count, first cell offset, freeblock, leaf/int flags) and validates cell pointer chain. |
| 0x826F7240 | sub_826F7240 | FM2_SQLite_BtreeCursor_GetCellPayloadOffset | 0.91 | Uses `Btree_ParseCellHeader` on current cell; returns payload start offset and length in out-param for seek/compare paths. |

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
| 0x826E07D0 | sub_826E07D0 | NUISPEECH `CVoiceInput` lifecycle; defer XUI/speech cluster. |
| 0x826FE8A8 | sub_826FE8A8 | Marks `+568` on VDBE statement chain only; defer stmt-invalidate cluster. |
| 0x826FCF28 | sub_826FCF28 | Vdbe batch opcode append; defer iter 116 VDBE cluster. |
| 0x82701D98 | sub_82701D98 | Value stack pop helper; defer value-array cluster. |
| 0x82701E98 | sub_82701E98 | Value affinity coercion; defer with `sub_82701DE8`. |
| 0x82712760 | sub_82712760 | Vfs page-number read; defer VFS cluster. |
| 0x826F5B98 | sub_826F5B98 | Page header flag decode; defer with `Btree_InitPage` helpers. |
| 0x82713820 | sub_82713820 | Page refcnt bump; defer pager ref cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
