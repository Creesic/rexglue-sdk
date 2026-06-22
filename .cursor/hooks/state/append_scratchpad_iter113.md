## Iteration 113

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8270C9A8 | sub_8270C9A8 | FM2_SQLite_ExprGetCollateOpcode | 0.91 | Skips TK_COLLATE (21) chain; unwraps TK_DOT (109) to underlying id; for TK_ASC/DESC (31) calls `ClassifyCollationNameSuffix`; else returns expr byte at +1. |
| 0x826F14D8 | sub_826F14D8 | FM2_SQLite_ClassifyCollationNameSuffix | 0.90 | Rolling 4-char hash over identifier suffix; detects BINARY/NOCASE/RTRIM/DESC patterns; returns collation opcode bytes 97–101 ('a'–'e'). |
| 0x8270CA68 | sub_8270CA68 | FM2_SQLite_MergeCollateOpcodes | 0.89 | Combines collate opcodes from two expr sides via `ExprGetCollateOpcode`; returns merged comparison opcode (98/99 rules for mixed BINARY/NOCASE). |
| 0x8270CBA0 | sub_8270CBA0 | FM2_SQLite_CodeGen_ComparisonWithCollation | 0.92 | Merges collations, resolves P4 via `ExprSkipCollatePrefix`, emits `Vdbe_AddOpcodeWithP4` for EQ/LT/GT-style compares; 10 callers in expr codegen. |
| 0x8270F900 | sub_8270F900 | FM2_SQLite_CodeGen_ExprIfTrue | 0.90 | Boolean/IN/BETWEEN/AND/OR expr codegen for truth branch: labels, `CodeGen_ComparisonWithCollation`, BETWEEN opcodes 69–71, calls `CodeGen_ExprIfFalse` for NOT. |
| 0x8270FB40 | sub_8270FB40 | FM2_SQLite_CodeGen_ExprIfFalse | 0.90 | Mirror of `CodeGen_ExprIfTrue` with inverted test polarity (`type ^ 1`); handles AND/OR short-circuit with backpatched labels. |
| 0x8270B858 | sub_8270B858 | FM2_SQLite_Select_MergeSubqueryIntoParent | 0.88 | Coalesces nested SELECT into parent SrcList: validates join/aggregate flags, moves FROM entries (36-byte stride), merges WHERE via `ParseTree_BuildAndExpr`, destroys child parse context. Called from `CodeGen_SelectInExpr`. |
| 0x826F45D8 | sub_826F45D8 | FM2_SQLite_SrcList_Append | 0.91 | Grows SrcList (18×count+4 bytes), zeroes new 0x24-byte entry, pushes table/subquery names via `ParseStack_PushEntry_0`. |
| 0x826FF398 | sub_826FF398 | FM2_SQLite_FindUserFunction | 0.90 | Scores hash-chain UDF entries at db+168 by db index/encoding match; optionally creates new function record and inserts into schema hash. |
| 0x826F52B0 | sub_826F52B0 | FM2_SQLite_Btree_SetFreelistTrunkEntry | 0.91 | Uses `Btree_AlignFreelistOffset` + hash lookup; writes 5-byte trunk slot (type byte + page number) when changed; may grow page via `sub_82714BB8`. |
| 0x826F15E8 | sub_826F15E8 | FM2_SQLite_ParseContext_SetCollation | 0.89 | Stores duped collation name on parse-context column list tail; sets collate opcode byte via `ClassifyCollationNameSuffix`. |
| 0x826F5470 | sub_826F5470 | FM2_SQLite_Btree_CellPtrAtIndex | 0.92 | Returns cell data pointer for index `a2` in btree page using cell-offset array at page+14 or interior pointer table. 10 btree callers. |

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
| 0x82714BB8 | sub_82714BB8 | Large btree dirty-page writer (27 callers); defer dedicated btree write pass. |
| 0x82713590 | sub_82713590 | Btree page-size/overflow sizing helper; needs more callee context. |
| 0x826F6678 | sub_826F6678 | Btree page unref/release; defer with page lifecycle cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
