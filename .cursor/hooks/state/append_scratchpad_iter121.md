## Iteration 121

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826FB4D0 | sub_826FB4D0 | FM2_SQLite_BtreeCursor_Advance | 0.90 | Steps btree cursor to next cell or child page; sets EOF flag via out-param; uses `GotoChildPage` + `GotoCellChildPage`; 6 VDBE/btree callers. |
| 0x826FCAB0 | sub_826FCAB0 | FM2_SQLite_BtreeHandle_Destroy | 0.89 | Closes all cursors on btree handle via `BtreeCursor_UnlinkAndFree`, calls `Btree_Close`, decrements connection refcount; frees global db registry entry when last ref. |
| 0x826FA0A8 | sub_826FA0A8 | FM2_SQLite_Btree_GetMetaValue | 0.91 | Locks meta page 1, reads 32-bit big-endian value at `4*(index+9)` from sqlite schema header; paired lock/unlock helpers `sub_826F50E0`/`sub_826F5190`. |
| 0x82713AC8 | sub_82713AC8 | FM2_SQLite_PagerCache_InvalidatePage | 0.88 | Hashes page number into pager cache, marks entry dirty (+40), unlinks from dirty-page list; called from `Btree_FreePage` paths. |
| 0x826F1260 | sub_826F1260 | FM2_SQLite_ParseContext_ResolveDatabase | 0.92 | Resolves attached database token to schema pointer; error `"unknown database %T"`; used in DDL/PRAGMA/view codegen. |
| 0x826FF8B0 | sub_826FF8B0 | FM2_SQLite_Utf8_ReadCodepoint | 0.93 | UTF-8 decoder using `byte_82118FF0` length table and `dword_821190F0` offset table; returns codepoint or 65533 on invalid lead byte; 7 UTF-8 consumers. |
| 0x826FA348 | sub_826FA348 | FM2_SQLite_Btree_ValidatePtrmapEntry | 0.94 | Reads ptrmap page via `sub_826F53B0`, compares type/page against expected; errors `"Failed to read ptrmap key=%d"` and `"Bad ptr map entry..."`. |
| 0x826EC4F0 | sub_826EC4F0 | FM2_SQLite_Database_SetErrorMessage | 0.91 | Sets db error-pending flag at +76, forwards to `ErrorObjectSetMessage` at db+8; 7 callers from VDBE/schema error paths. |
| 0x826EE480 | sub_826EE480 | FM2_SQLite_GetGlobalDbRegistry | 0.90 | Returns live sqlite connection registry via `dword_82A3CEAC(0)` or static fallback `unk_82118478`; used by btree lock helpers and `BtreeHandle_Destroy`. |
| 0x826F74C8 | sub_826F74C8 | FM2_SQLite_BtreeCursor_GotoCellChildPage | 0.89 | Follows interior-table cell pointer at current index to child page via `BtreeCursor_GotoChildPage`; used during cursor advance. |
| 0x826FCFF0 | sub_826FCFF0 | FM2_SQLite_Vdbe_SetOpcodeP2 | 0.92 | Patches P2 field (+4) of 20-byte VDBE opcode at index; paired with `Vdbe_SetOpcodeP4`; used by PRAGMA handler to set cache_size opcode args. |
| 0x826F33D8 | sub_826F33D8 | FM2_SQLite_Schema_MaterializeView | 0.91 | Expands VIEW into SELECT parse tree, runs `sub_827093E0` codegen; error `"view %s is circularly defined"` on recursive view. |

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
| 0x826F50E0 | sub_826F50E0 | Btree meta-page lock conflict check; defer lock pair with `sub_826F5190`. |
| 0x826F5190 | sub_826F5190 | Btree meta-page lock acquire; defer lock pair. |
| 0x826F2E88 | sub_826F2E88 | Builds per-index collation array; defer index DDL cluster. |
| 0x826F5638 | sub_826F5638 | Btree overflow-cell insert; defer page balance cluster. |
| 0x826F87A8 | sub_826F87A8 | Btree interior-page cell delete; defer page balance cluster. |
| 0x826F9DB8 | sub_826F9DB8 | Btree page balance trigger; defer page balance cluster. |
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
