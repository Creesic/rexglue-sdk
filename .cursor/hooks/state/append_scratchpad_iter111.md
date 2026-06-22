## Iteration 111

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8270C9F0 | sub_8270C9F0 | FM2_SQLite_ExprSkipCollatePrefix | 0.90 | Walks COLLATE/ordering prefix expr nodes (types 21/31); validates collation via `ResolveCollationSequence`; returns underlying expr pointer. |
| 0x826F5268 | sub_826F5268 | FM2_SQLite_Btree_AlignFreelistOffset | 0.88 | Computes aligned freelist page offset from page size and `usable_size/5+1`; handles edge case at `0x40000000/page_size+1`. 13 btree callers. |
| 0x82709688 | sub_82709688 | FM2_SQLite_Select_FlattenSubqueries | 0.91 | Flattens nested SELECTs into parent scope; assigns column numbers via `AssignSelectColumnNumbers`; errors `"no such table"`, `"sqlite_subquery_%p_"`. |
| 0x8270B180 | sub_8270B180 | FM2_SQLite_Select_ResolveAndValidate | 0.90 | Runs flatten then `ResolveSelectNames`; validates GROUP BY/HAVING/ORDER BY via `ResolveOrderByColumnList`; errors on missing HAVING or aggregates in GROUP BY. |
| 0x8270B088 | sub_8270B088 | FM2_SQLite_ResolveOrderByColumnList | 0.92 | Resolves ORDER/GROUP BY integer column refs via `ExprExtractSortColumnIndex`; replaces expr with dup from select list; range error message with clause name. |
| 0x826FF768 | sub_826FF768 | FM2_SQLite_FindCollationSequence | 0.90 | Looks up/creates collation via `GetCollationByEncodingSlot` and `sub_826FF130`; may register new collation via `sub_826FF6B8`. Used by `ResolveCollationSequence`. |
| 0x8270EE18 | sub_8270EE18 | FM2_SQLite_CodeGen_ExprNode | 0.91 | Large expr→VDBE switch: emits opcodes per TK_* type; errors `"RAISE() may only be used within a trigger-program"`, `"misuse of aggregate: %T"`. 53 callers. |
| 0x8270EAF0 | sub_8270EAF0 | FM2_SQLite_CodeGen_ExprTree | 0.89 | Top-level expr codegen: may emit transaction opcodes 1/9/46; dispatches TK_SELECT subqueries and calls `CodeGen_ExprNode` for leaves. |
| 0x8270D200 | sub_8270D200 | FM2_SQLite_ExprExtractSortColumnIndex | 0.91 | Parses `+/-` integer or unary-minus-wrapped integer sort keys from ORDER/GROUP BY expr; uses `ParseInt32Literal` for TK_INTEGER nodes. |
| 0x826F28C8 | sub_826F28C8 | FM2_SQLite_AssignSelectColumnNumbers | 0.89 | Assigns monotonic column indices at parse+28 to SELECT result columns; recurses into nested SELECT expr lists when high bit set. |
| 0x82708300 | sub_82708300 | FM2_SQLite_Select_BuildJoinConstraints | 0.91 | Builds NATURAL/ON/USING join constraints; errors `"a NATURAL join may not have an ON or USING clause"`, `"cannot join using column %s..."`. |
| 0x826FF338 | sub_826FF338 | FM2_SQLite_GetCollationByEncodingSlot | 0.88 | Returns collation entry from db collation hash at offset `(encoding*16)&0xFF0`; falls back to default slot when name null. |

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
| 0x826EDFE8 | sub_826EDFE8 | ParseInt32Literal; defer with `sub_82415FC8` atoi cluster. |
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826FF248 | sub_826FF248 | FindOrCreateCollation; defer with `sub_826FF130` cluster. |
| 0x827080E8 | sub_827080E8 | SchemaTable_FindColumnIndex; defer join helper cluster. |
| 0x827081C0 | sub_827081C0 | Join constraint append helper; defer iter 112. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
