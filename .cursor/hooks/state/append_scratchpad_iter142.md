## Iteration 142

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827182E0 | sub_827182E0 | FM2_SQLite_CodeGen_DropTableTriggers | 0.90 | DROP TABLE trigger cleanup: loops trigger list emitting opcodes 56/84, builds `tbl_name=%Q` filter, calls `Schema_BuildTriggerNameOrFilter`; callers alter/drop table paths. |
| 0x8271FB18 | sub_8271FB18 | FM2_SQLite_Pragma_SetTempStore | 0.92 | PRAGMA temp_store handler: parses `file`/`memory`/`0-2`, destroys temp btree on change; error `"temporary storage cannot be changed from within a transaction"`. |
| 0x8271E048 | sub_8271E048 | FM2_SQLite_Select_FromUsesTableColumn | 0.89 | Recursive FROM-clause walk: matches table cookie (+60) and column index (+20); self-recurses on subqueries; caller select dependency checks. |
| 0x827183D0 | sub_827183D0 | FM2_SQLite_CodeGen_AlterTableRename | 0.93 | ALTER TABLE RENAME: auth 26, rejects `sqlite_` tables/views, updates `sqlite_master`/`sqlite_sequence` via `sqlite_rename_table`/`sqlite_rename_trigger` SQL helpers. |
| 0x82707FA0 | sub_82707FA0 | FM2_SQLite_Parse_CheckJoinType | 0.91 | Validates JOIN keywords (`natural`, inner/left/right/full/cross) against 7-entry table; errors `"unknown or unsupported join type"` and `"RIGHT and FULL OUTER JOINs are not currently supported"`. |
| 0x82708520 | sub_82708520 | FM2_SQLite_CodeGen_EmitInsertAutoincrementSequence | 0.88 | INSERT autoincrement path: opcodes 5/99/87/105 then 114/43/91/49/88 sequence update; clears autoinc register at +48. |
| 0x82701310 | sub_82701310 | FM2_SQLite_AggregateFunc_SumStep | 0.90 | sum() aggregate step: aux-data +32 accumulates int64/double totals with overflow tracking bytes +24/+25; registered in builtin table. |
| 0x82710F90 | sub_82710F90 | FM2_SQLite_CodeGen_CommitTriggerToSchema | 0.89 | Finalizes CREATE TRIGGER: auth walk, emits `type='trigger' AND name='%q'` delete, inserts into schema hash linked list; pairs with `CodeGen_CreateTrigger`. |
| 0x827018C0 | sub_827018C0 | FM2_SQLite_RegisterBuiltinFunctions | 0.92 | DB init: registers 24 scalar/aggregate builtins from `unk_821191A6`, calls `RegisterRenameHelperFunctions`, attach/detach, 7 more funcs, `RegisterDateTimeFunctions`, like/glob. |
| 0x82718048 | sub_82718048 | FM2_SQLite_Internal_RenameTableSql | 0.91 | Internal `sqlite_rename_table` helper: tokenizes SQL, formats `%.*s%Q%s` replacement segment; registered by `RegisterRenameHelperFunctions`. |
| 0x827180F8 | sub_827180F8 | FM2_SQLite_Internal_RenameTriggerSql | 0.91 | Internal `sqlite_rename_trigger` helper: same printf pattern with different token termination; used in ALTER RENAME UPDATE statements. |
| 0x827181D8 | sub_827181D8 | FM2_SQLite_RegisterRenameHelperFunctions | 0.90 | Registers `sqlite_rename_table` and `sqlite_rename_trigger` via `CreateUserFunction` loop over `unk_8211B1F8`; callee of `RegisterBuiltinFunctions`. |

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
| 0x826FC0A8 | sub_826FC0A8 | Btree cursor insert; defer btree cluster pass. |
| 0x82700B30 | sub_82700B30 | Thin Vdbe scalar forwarder cluster (last_insert_rowid). |
| 0x82700B70 | sub_82700B70 | Thin Vdbe scalar forwarder cluster (changes). |
| 0x82700BB0 | sub_82700BB0 | Thin Vdbe scalar forwarder cluster (total_changes). |
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x82709168 | sub_82709168 | INSERT column register loader; defer insert codegen cluster. |
| 0x8270AD00 | sub_8270AD00 | min/max index optimization; defer select optimization cluster. |
| 0x8270B4A0 | sub_8270B4A0 | UPSERT DO UPDATE codegen; defer upsert cluster. |
| 0x82710180 | sub_82710180 | Parser alloc type 109/99; yacc rule mapping still unclear. |
| 0x82710288 | sub_82710288 | Parser alloc type 99; yacc rule mapping still unclear. |
| 0x82710300 | sub_82710300 | Parser alloc type 98/99; yacc rule mapping still unclear. |
| 0x8270FFE8 | sub_8270FFE8 | Parse-object list prepend; defer tiny list helpers. |
| 0x82710008 | sub_82710008 | Parse-object list unlink; defer tiny list helpers. |
| 0x82713110 | sub_82713110 | Pager journal-mode flag setter; thin helper. |
| 0x82713520 | sub_82713520 | Pager page-size field accessor; too trivial alone. |
| 0x82713B90 | sub_82713B90 | Pager dirty-page bitset marking; defer pager cluster. |
| 0x82716080 | sub_82716080 | Zeros 3 dwords only; too trivial alone. |
| 0x82717E88 | sub_82717E88 | Thin UDF init wrapper → `DateTime_OpTime` only. |
| 0x82717F00 | sub_82717F00 | Thin UDF init wrapper → `DateTime_OpDate` only. |
| 0x82717F78 | sub_82717F78 | Thin UDF init wrapper → `DateTime_OpDatetime` only. |
| 0x827016E8 | sub_827016E8 | Registers like/glob only; thin but defer with string func cluster. |
| 0x82719088 | sub_82719088 | Parse-stack reduce trampoline; thin wrapper. |
| 0x827212E0 | sub_827212E0 | D3D render-state dispatch; outside SQLite cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
