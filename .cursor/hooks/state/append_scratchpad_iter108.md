## Iteration 108

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82707A68 | sub_82707A68 | FM2_SQLite_ParseSqlString | 0.93 | Tokenizes SQL via `LexNextToken`; dispatches tokens through `ParseDriver`; handles `"unrecognized token"` and `"interrupt"` errors; allocates parser stack via `ParseStack_Alloc`. |
| 0x82707300 | sub_82707300 | FM2_SQLite_LexNextToken | 0.92 | SQLite lexer: whitespace/operator jump table, identifier scan via `byte_82119CD0`, keyword resolve via `LookupKeywordToken`; returns token id and consumed length. |
| 0x82707230 | sub_82707230 | FM2_SQLite_LookupKeywordToken | 0.94 | Perfect-hash keyword lookup over collated bytes (`StrncmpCollated`); keyword blob contains `SELECT`/`CREATE`/`TABLE`/etc.; returns token id or 23 (ID). |
| 0x8271A588 | sub_8271A588 | FM2_SQLite_ParseDriver | 0.91 | LALR driver loop: `ParseTable_LookupAction` / `ParseTable_Reduce`; emits `"near \"%T\": syntax error"` and `"incomplete SQL statement"`. |
| 0x827190E8 | sub_827190E8 | FM2_SQLite_ParseTable_LookupAction | 0.90 | Parser action lookup from `word_8211C978`/`byte_8211C480` tables; maps (state,token)→shift/reduce/goto or error. |
| 0x82719240 | sub_82719240 | FM2_SQLite_ParseTable_Reduce | 0.91 | Large reduce dispatcher: pops RHS symbols, calls semantic actions via jump table `byte_8211D380`, pushes goto state from `word_8211BA90`. |
| 0x82719010 | sub_82719010 | FM2_SQLite_ParseStack_Pop | 0.89 | Pops one symbol from parse stack; calls `sub_82718F50` to free semantic value; decrements stack depth. |
| 0x82718F10 | sub_82718F10 | FM2_SQLite_ParseStack_Alloc | 0.90 | Allocates 2012-byte parser stack via callback; initializes top state to -1. Used at start of `ParseSqlString`. |
| 0x826FD368 | sub_826FD368 | FM2_SQLite_Vdbe_ReleaseRegisterRange | 0.91 | Walks `count` consecutive 64-byte register slots calling `Database_ReleaseOpenStatements` on each. Used when shrinking register file. |
| 0x826EEAE0 | sub_826EEAE0 | FM2_SQLite_ReallocGrowPtr | 0.90 | `ReallocOrAllocZeroed` on `*ptr`; frees old buffer via `dword_82A3CEB8` on failure; stores new pointer. Used for label/array growth. |
| 0x8270E068 | sub_8270E068 | FM2_SQLite_ParseTree_DupNode | 0.91 | Deep-copies 68-byte parse node: `Strdup` text span, recursive dup left/right children, dup expr list and select struct. 28 callers. |
| 0x8270CC38 | sub_8270CC38 | FM2_SQLite_ParseTree_SetSpan | 0.89 | Copies source span coordinates from two child token spans into parse node +28/+32 when heap limit OK. Called from `ParseTree_AllocNode`. |

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
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826F2DE8 | sub_826F2DE8 | Schema-write lock helper; defer with `sub_826F2D40` cluster. |
| 0x826F54E0 | sub_826F54E0 | Btree cell header parser; defer dedicated btree pass. |
| 0x8270EE18 | sub_8270EE18 | 2.6KB expression codegen switch; defer dedicated pass. |
| 0x82718F50 | sub_82718F50 | Parse-stack symbol dtor jump table; defer with destroy cluster. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
