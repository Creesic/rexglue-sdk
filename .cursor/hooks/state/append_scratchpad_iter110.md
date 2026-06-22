## Iteration 110

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8270D960 | sub_8270D960 | FM2_SQLite_ParseTree_Walk | 0.91 | Recursively walks parse tree: calls visitor on node, then left/right children and id-list via `IdList_Walk`. Returns early on nonzero visitor result. |
| 0x8270D118 | sub_8270D118 | FM2_SQLite_IdList_Walk | 0.90 | Iterates id-list entries (stride 12) calling `ParseTree_Walk` on each expr until visitor succeeds. |
| 0x8270EA58 | sub_8270EA58 | FM2_SQLite_ResolveSelectNames | 0.91 | Walks SELECT parse tree with `ResolveExprName`; tracks aggregate/HAVING flags at +16/+21; propagates DISTINCT/AGG bits on expr +2. 16 callers. |
| 0x8270E768 | sub_8270E768 | FM2_SQLite_ResolveExprName | 0.90 | Per-expr name resolver: handles TK_COLUMN/Tk_FUNCTION/Tk_AGG_FUNCTION; errors `"subqueries prohibited in CHECK constraints"`, `"no such function"`, etc.; calls `ResolveColumnReference`. |
| 0x8270E140 | sub_8270E140 | FM2_SQLite_ResolveColumnReference | 0.92 | Resolves `db.table.column` against SELECT scopes; errors `"no such column: %s"`, `"ambiguous column name: %s"`; handles OLD/NEW trigger columns. |
| 0x826FA1F8 | sub_826FA1F8 | FM2_SQLite_AppendVprintfErrorLine | 0.90 | Formats variadic message via `Vsnprintf`; appends to error buffer at +16 with newline prefix via `SetErrorMessageV`; frees temp buffer. 16 btree error callers. |
| 0x826EF0F0 | sub_826EF0F0 | FM2_SQLite_ValueToInt64 | 0.91 | Coerces SQLite value to int64: direct for INT/REAL types; for text runs `ValueEnsureNulTerminatedUtf8` then `ParseInt64Literal`. |
| 0x826EF2C8 | sub_826EF2C8 | FM2_SQLite_ValueSetInt64 | 0.92 | Sets value to int64 via `ValueToInt64` path then stores in `*a1`, releases statements, sets type flag word+24=4 (INTEGER). 16 callers. |
| 0x826EEE60 | sub_826EEE60 | FM2_SQLite_ValueEnsureNulTerminatedUtf8 | 0.89 | For TEXT values without nul terminator: realloc+copy with trailing `\\0\\0`; clears custom destructor. Used before atoi-style parsing. |
| 0x826EDEB0 | sub_826EDEB0 | FM2_SQLite_ParseInt64Literal | 0.93 | Parses optional sign + decimal digits to int64; validates against `"9223372036854775807"` bound for 19-digit overflow. |
| 0x826EE1B8 | sub_826EE1B8 | FM2_SQLite_CheckInterrupt | 0.88 | Three-state interrupt probe at db+48 magic constants; sets `db+8` bit 2 on second pass. Called throughout compile/exec paths. |
| 0x826FF820 | sub_826FF820 | FM2_SQLite_ResolveCollationSequence | 0.91 | Looks up collation name via `sub_826FF768`; on miss errors `"no such collation sequence: %s"` and bumps suppress counter at +24. |

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
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826F5268 | sub_826F5268 | Freelist page alignment math only; defer btree pager cluster. |
| 0x8270B180 | sub_8270B180 | Large SELECT flatten/resolve; defer with `sub_82709688` cluster. |
| 0x8270C9F0 | sub_8270C9F0 | Skip COLLATE expr chain helper; defer expr-type cluster. |
| 0x8270EE18 | sub_8270EE18 | 2.6KB expression-to-VDBE codegen; defer dedicated pass. |
| 0x8267A878 | sub_8267A878 | Large system sound-create path; defer dedicated pass. |
| 0x8264EB08 | sub_8264EB08 | Reads float at a1 only; too trivial alone. |
| 0x827C4188 | sub_827C4188 | Returns result+4 only. |
| 0x827C3D00 | sub_827C3D00 | Stores dword at +160 only. |
| 0x824173C8 | sub_824173C8 | Single-instruction fsel. |
| 0x821DA010 | sub_821DA010 | Pass-lighting helper; purpose still unclear. |
