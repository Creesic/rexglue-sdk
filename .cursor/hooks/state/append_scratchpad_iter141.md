## Iteration 141

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827178B0 | sub_827178B0 | FM2_SQLite_DateTime_OpTime | 0.91 | time() UDF: `TokensMatchInterval`, `DecomposeTimeOfDay`, formats `"%02d:%02d:%02d"`; init wrapper at `0x82717E88`. |
| 0x82717938 | sub_82717938 | FM2_SQLite_DateTime_OpDate | 0.91 | date() UDF: formats `"%04d-%02d-%02d"` after Julian decompose; init wrapper at `0x82717F00`. |
| 0x82717FF0 | sub_82717FF0 | FM2_SQLite_RegisterDateTimeFunctions | 0.92 | Registers 8 datetime builtins via `CreateUserFunction` loop over table at `unk_8211B004`; DB init caller. |
| 0x82718650 | sub_82718650 | FM2_SQLite_CodeGen_AlterTableAddColumn | 0.92 | ALTER TABLE ADD COLUMN: auth 26, validates PK/UNIQUE/NOT NULL/default constraints; updates `sqlite_master` SQL via printf; error strings documented. |
| 0x82718890 | sub_82718890 | FM2_SQLite_CodeGen_InitAlterTableAddColumn | 0.90 | ALTER ADD COLUMN setup: clones table schema, dupes column names, opens schema for write; error `"Cannot add a column to a view"`. |
| 0x8270DE68 | sub_8270DE68 | FM2_SQLite_ParseTree_WalkExprListAndCountAutoincrementErrors | 0.89 | Walks expr-list (+3 stride), sums `WalkAndCountAutoincrementErrors` per entry; callers `CodeGen_SelectInExpr`. |
| 0x8271DF90 | sub_8271DF90 | FM2_SQLite_CodeGen_SetInsertAffinityP4 | 0.90 | Builds insert affinity string from table columns (+18 stride 20), caches at +48, sets Vdbe P4; callee `FinishInsertRow`. |
| 0x8271E928 | sub_8271E928 | FM2_SQLite_CodeGen_FinishInsertRow | 0.89 | Completes INSERT codegen: opcodes 105/87/107, `SetInsertAffinityP4`, optional table-name P4 (-2); distinct from `EmitInsertRow`. |
| 0x8271B338 | sub_8271B338 | FM2_SQLite_CodeGen_WhereEmitIndexedRowFetch | 0.88 | WHERE index row fetch: opcodes 65/10/91/87 + `BuildRecordAffinityP4`; caller `Where_BuildJoinPlan`. |
| 0x8271B3C8 | sub_8271B3C8 | FM2_SQLite_CodeGen_WhereRightHandExpr | 0.90 | WHERE RHS codegen: `?` param emits Bind opcode 119 + tracks in WhereLevel; else `CodeGen_ExprNode`; ends with `WhereLevel_DisableBranch`. |
| 0x8271B490 | sub_8271B490 | FM2_SQLite_CodeGen_WhereAndIndexTerms | 0.89 | AND-term index setup: `Schema_FindIndexForColumn` per column, calls `WhereRightHandExpr`, optional DISTINCT opcode 118 loop. |
| 0x82718238 | sub_82718238 | FM2_SQLite_Schema_BuildTriggerNameOrFilter | 0.90 | Builds `name=%Q` / `%s OR name=%Q` SQL filter for triggers on attached DBs matching cookie; callee drop-trigger codegen. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826F14A8 | sub_826F14A8 | Sets FK action byte on last constraint only (44 bytes); too thin alone. |
| 0x826F2628 | sub_826F2628 | Sets single byte on nested parse object; too thin alone. |
| 0x826F6300 | sub_826F6300 | Thin pager callback setter; callee unknown. |
| 0x826F6360 | sub_826F6360 | Thin wrapper → sub_82713110 only. |
| 0x826F6390 | sub_826F6390 | Thin wrapper (12 bytes). |
| 0x826F6440 | sub_826F6440 | Too trivial alone. |
| 0x826F6458 | sub_826F6458 | Too trivial alone. |
| 0x826F72F0 | sub_826F72F0 | Thin wrapper only. |
| 0x826F7310 | sub_826F7310 | Thin wrapper only. |
| 0x826FA1D0 | sub_826FA1D0 | Reads single byte from mem cell; too trivial alone. |
| 0x826FABB0 | sub_826FABB0 | Thin wrapper (12 bytes). |
| 0x826FABC0 | sub_826FABC0 | Thin wrapper (12 bytes). |
| 0x826FABD0 | sub_826FABD0 | Thin wrapper (12 bytes). |
| 0x826FAC88 | sub_826FAC88 | Single-line wrapper; too thin alone. |
| 0x826FE880 | sub_826FE880 | Too trivial alone. |
| 0x826FE898 | sub_826FE898 | Too trivial alone. |
| 0x82700B30 | sub_82700B30 | Vdbe scalar wrapper (last_insert_rowid); thin forwarder cluster. |
| 0x82700B70 | sub_82700B70 | Vdbe scalar wrapper (changes); thin forwarder cluster. |
| 0x82700BB0 | sub_82700BB0 | Vdbe scalar wrapper (total_changes); thin forwarder cluster. |
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x82710180 | sub_82710180 | Parser alloc type 109/99; yacc rule mapping still unclear. |
| 0x82710288 | sub_82710288 | Parser alloc type 99; yacc rule mapping still unclear. |
| 0x82710300 | sub_82710300 | Parser alloc type 98/99; yacc rule mapping still unclear. |
| 0x8270FFE8 | sub_8270FFE8 | Parse-object list prepend; defer tiny list helpers. |
| 0x82710008 | sub_82710008 | Parse-object list unlink; defer tiny list helpers. |
| 0x82713110 | sub_82713110 | Pager journal-mode flag setter; thin helper. |
| 0x82713520 | sub_82713520 | Pager page-size field accessor; too trivial alone. |
| 0x82716080 | sub_82716080 | Zeros 3 dwords only; too trivial alone. |
| 0x82717E88 | sub_82717E88 | Thin UDF init wrapper → `DateTime_OpTime` only. |
| 0x82717F00 | sub_82717F00 | Thin UDF init wrapper → `DateTime_OpDate` only. |
| 0x82717F78 | sub_82717F78 | Thin UDF init wrapper → `DateTime_OpDatetime` only. |
| 0x827180F8 | sub_827180F8 | printf/quote UDF; defer string function cluster. |
| 0x827182E0 | sub_827182E0 | Drop-table trigger codegen; defer with schema filter cluster. |
| 0x82719088 | sub_82719088 | Parse-stack reduce trampoline; thin wrapper. |
| 0x8271E048 | sub_8271E048 | FROM-clause table/column ref search; defer select cluster. |
| 0x8271FB18 | sub_8271FB18 | PRAGMA temp_store; defer pragma cluster. |
| 0x827212E0 | sub_827212E0 | D3D render-state dispatch; outside SQLite cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
