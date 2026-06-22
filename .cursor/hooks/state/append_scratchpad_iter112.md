## Iteration 112

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826FF248 | sub_826FF248 | FM2_SQLite_FindOrCreateCollation | 0.91 | Looks up collation in schema hash at db+196; allocates 49-byte entry with BINARY/NOCASE/RTRIM slots when `create` flag set; inserts via `SchemaHashInsertOrReplace`. |
| 0x826FF130 | sub_826FF130 | FM2_SQLite_InvokeCollationFactory | 0.89 | Invokes db collation factory callbacks at +120/+124 with duped name text and temp error object; used when collation missing before retry lookup. |
| 0x826FF6B8 | sub_826FF6B8 | FM2_SQLite_LoadCollationAcrossEncodings | 0.90 | Tries encodings 0–2 via `GetCollationByEncodingSlot` until collation compare fn non-null; copies 4 pointer slots into output. Called from `FindCollationSequence`. |
| 0x827080E8 | sub_827080E8 | FM2_SQLite_TableDef_FindColumnIndex | 0.92 | Linear search column name list (20-byte stride) with `StrcasecmpCollated`; returns index or -1. Used by join constraint builder. |
| 0x82708150 | sub_82708150 | FM2_SQLite_ParseTree_AllocIdentifierNode | 0.91 | Allocates TK_ID (23) parse node with identifier span `{ptr,len}`; null name yields zero-length node. |
| 0x827081C0 | sub_827081C0 | FM2_SQLite_AppendJoinEqualityConstraint | 0.90 | Builds two TK_EQ (111) identifier nodes and ANDs (67) them; appends to constraint list via `ParseTree_BuildAndExpr`. |
| 0x8270E038 | sub_8270E038 | FM2_SQLite_ParseTree_BuildAndExpr | 0.92 | Returns right expr if left null; else allocates TK_AND (60) combining two expr subtrees. |
| 0x8270BC38 | sub_8270BC38 | FM2_SQLite_CodeGen_SelectInExpr | 0.88 | 3.4KB SELECT-in-expression VDBE codegen; error `"only a single result allowed for a SELECT that is part of an expression"`; 14 callers incl `CodeGen_ExprTree`. |
| 0x8270D180 | sub_8270D180 | FM2_SQLite_ExprWalk_DetectCorrelation | 0.90 | Parse-tree walk callback: flags correlated subqueries/aggregates (TK_SELECT 63, TK_AGG 109, TK_IN 23, etc.) and clears walk state. |
| 0x8270DA08 | sub_8270DA08 | FM2_SQLite_ExprContainsCorrelatedSubquery | 0.91 | Walks expr with `ExprWalk_DetectCorrelation`; returns 0 if correlation found else 1. |
| 0x826FE948 | sub_826FE948 | FM2_SQLite_Vdbe_ClearOpcodeRange | 0.89 | Clears `count` VDBE opcode records starting at index; frees P4 strings and resets opcode type to 19 (NOP). |
| 0x826EDFE8 | sub_826EDFE8 | FM2_SQLite_ParseInt32Literal | 0.92 | Validates optional sign + decimal digits with INT32_MAX bound check against `"2147483647"`; parses via `sub_82415FC8`. |

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
| 0x8270C9A8 | sub_8270C9A8 | ExprGetCollateOpcode; defer with `sub_826F14D8` collation-classify cluster. |
| 0x826F14D8 | sub_826F14D8 | ClassifyCollationSuffix; defer iter 113. |
| 0x8270B858 | sub_8270B858 | Select subquery merge helper; needs more string/xref evidence. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
