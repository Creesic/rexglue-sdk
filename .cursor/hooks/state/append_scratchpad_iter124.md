## Iteration 124

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827086D0 | sub_827086D0 | FM2_SQLite_CodeGen_EmitInsertRow | 0.90 | INSERT row codegen: emits Copy/IntCopy/IdxInsert opcodes, optional FK epilogue via `CodeGen_EmitFkCheckEpilogue`, handles DEFAULT VALUES and CHECK paths; 6 DDL callers. |
| 0x826EF408 | sub_826EF408 | FM2_SQLite_Mem_SetRowid | 0.91 | Sets 64-byte Mem to INTEGER rowid (type flag 4): stores int64 at +0, marks valid; called via `sub_826EC510(mem+8, rowid)`. |
| 0x826FABE0 | sub_826FABE0 | FM2_SQLite_Btree_SavepointIsExclusive | 0.88 | Returns 1 when btree savepoint record byte+8 equals 2 (exclusive/trans top-level); used during `Database_CloseAllBtrees` rollback decisions. |
| 0x827071B0 | sub_827071B0 | FM2_SQLite_Context_AppendErrorVprintf | 0.92 | Formatted error append: `VprintfCore` into `PrintfAppendChunk` buffer; used for schema/VDBE error messages with varargs. |
| 0x82709F20 | sub_82709F20 | FM2_SQLite_CodeGen_EmitCheckConstraint | 0.91 | CHECK constraint codegen: evaluates expr, emits MustBeInt/If/Ne opcodes with backpatch chain for pass/fail paths. |
| 0x82701A90 | sub_82701A90 | FM2_SQLite_Schema_ExecSqlLines | 0.92 | Executes semicolon-separated SQL for schema load: `Prepare`/`Vdbe_Step` loop, collects column-name arrays, handles schema-changed retry (17). |
| 0x8271A8F0 | sub_8271A8F0 | FM2_SQLite_WhereLevel_AddFromExpr | 0.90 | Walks expr tree skipping AND-nodes (opcode==a3); adds `WhereLevel` entry for each OR/term leaf; 5 WHERE planner callers. |
| 0x8271B2C0 | sub_8271B2C0 | FM2_SQLite_WhereLevel_DisableBranch | 0.89 | Walks parent WhereLevel chain decrementing refcounts, sets disabled flag (+12 bit 4); stops at already-disabled or RIGHT-JOIN levels. |
| 0x826EED10 | sub_826EED10 | FM2_SQLite_Expr_ApplyAffinityIfNeeded | 0.90 | If expr lacks TEXT/NUMERIC affinity or target differs, calls `ExprAppendLowercaseToken`; used during comparison coercion. |
| 0x826F8D30 | sub_826F8D30 | FM2_SQLite_Btree_BalancePage | 0.89 | Full btree page balancer (3.2KB): splits/merges cells across siblings, relocates overflow cells; called from `Btree_BalanceIfNeeded`. |
| 0x826F99B0 | sub_826F99B0 | FM2_SQLite_Btree_CopyNodeForBalance | 0.88 | Copies sibling page content during balance: `Btree_InitPage`, cell relocation, overflow insertion; OOM returns 7. |
| 0x82717710 | sub_82717710 | FM2_SQLite_DateTime_TokensMatchInterval | 0.89 | Validates datetime modifier token list against parsed interval; uses `DateTime_ParseTokenToContext` and `sub_82716ED8`; returns mismatch flag. |

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
| 0x82716A00 | sub_82716A00 | DateTime_ParseTokenToContext (`"now"` + numeric); defer datetime cluster. |
| 0x8271C1A0 | sub_8271C1A0 | Large WHERE planner driver (4.2KB); defer dedicated pass. |
| 0x8271FD40 | sub_8271FD40 | PRAGMA handler (5.2KB); defer dedicated pass. |
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
