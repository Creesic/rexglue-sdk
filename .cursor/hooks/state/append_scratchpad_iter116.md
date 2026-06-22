## Iteration 116

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826FCF28 | sub_826FCF28 | FM2_SQLite_Vdbe_AppendOpcodeBatch | 0.92 | Grows VDBE opcode array then copies `count` 8-byte template records (op/p1/p2/p3/p4) into 20-byte slots; resolves negative jump targets. 9 codegen callers. |
| 0x82701D98 | sub_82701D98 | FM2_SQLite_ValueStack_PopN | 0.91 | Pops `n` 64-byte Mem values from stack pointer; releases ephemeral values (flag 0x40) via `Database_ReleaseOpenStatements`. |
| 0x82701E98 | sub_82701E98 | FM2_SQLite_ValueApplyAffinity | 0.90 | Applies affinity char: 'a'(97) clears type flags; 'b'(98) no-op; else delegates to `ValueNumericAffinity` and may finalize float→int. |
| 0x82701DE8 | sub_82701DE8 | FM2_SQLite_ValueNumericAffinity | 0.89 | Ensures UTF-8 for text values, attempts ASCII→int64 via `ParseInt64Literal`, else promotes to float/int representation. |
| 0x82712760 | sub_82712760 | FM2_SQLite_Vfs_ReadUint24BE | 0.88 | After `Vfs_AddRef`, packs three big-endian bytes into 24-bit integer out-param; used reading journal header fields. |
| 0x826F5B98 | sub_826F5B98 | FM2_SQLite_Btree_DecodePageFlags | 0.91 | Decodes btree page header flag byte into cursor fields: intkey, leaf, table/btree layout, min cell offset, payload fraction. Called from `Btree_InitPage`/`CreateEmptyPage`. |
| 0x82713820 | sub_82713820 | FM2_SQLite_Pager_AcquirePageRef | 0.90 | Increments page refcnt at +42 or links page into dirty list via `Pager_LinkDirtyPage` on first acquire. |
| 0x82713780 | sub_82713780 | FM2_SQLite_Pager_LinkDirtyPage | 0.89 | Splices page into pager dirty-page doubly-linked list (+104/+108), updates list head, increments dirty-page count (+60). |
| 0x826FB330 | sub_826FB330 | FM2_SQLite_BtreeCursor_GetRowid | 0.91 | Returns 64-bit rowid/key at cursor (+40); may reset seek cache and parse current cell header first. 9 btree/VDBE callers. |
| 0x826F5DD8 | sub_826F5DD8 | FM2_SQLite_Btree_CreateEmptyPage | 0.92 | Zero-fills new page buffer, writes page-type byte and header fields, then calls `Btree_DecodePageFlags` to initialize cursor state. |
| 0x82716AE8 | sub_82716AE8 | FM2_SQLite_DateTime_DecomposeJulianDay | 0.88 | Converts Julian day double at +0 into year/month/day fields (+8/+12/+16); default 2000-01-01 when time component absent (+42). 9 datetime callers. |
| 0x826EEF00 | sub_826EEF00 | FM2_SQLite_ValueFormatAsTextForCompare | 0.90 | Formats int64 (`"%lld"`) or float (`"%!.15g"`) into compare buffer (+32); sets text flags; optional `ExprAppendLowercaseToken` for case-insensitive compare. |

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
| 0x826E5690 | sub_826E5690 | XAudio2 voice pool IP formatting; defer networking/audio cluster. |
| 0x826E64E0 | sub_826E64E0 | XAudio voice object heap cleanup; defer audio cluster. |
| 0x826FE8A8 | sub_826FE8A8 | Marks `+568` on VDBE statement chain only; defer stmt-invalidate cluster. |
| 0x826EF290 | sub_826EF290 | Thin float→int exact conversion; defer value-coercion helpers. |
| 0x827127C0 | sub_827127C0 | Journal header reader; defer pager journal cluster. |
| 0x82712948 | sub_82712948 | Journal sector write during open; defer pager journal cluster. |
| 0x826ED278 | sub_826ED278 | Large `sqlite3_prepare` core; defer dedicated prepare pass. |
| 0x826EC570 | sub_826EC570 | `sqlite3_step` core; defer VDBE execution pass. |
| 0x82718A90 | sub_82718A90 | Exec-until-not-ROW wrapper; defer stmt API cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
