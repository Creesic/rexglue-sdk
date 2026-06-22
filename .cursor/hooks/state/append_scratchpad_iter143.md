## Iteration 143

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826FC0A8 | sub_826FC0A8 | FM2_SQLite_BtreeCursor_Insert | 0.91 | Btree cursor INSERT: txn/cursor checks, `Btree_SeekKey`, `Btree_InsertCellPayload`, optional interior-cell delete, `Btree_BalanceIfNeeded`, reload page. |
| 0x82709168 | sub_82709168 | FM2_SQLite_CodeGen_LoadInsertColumnRegisters | 0.90 | INSERT VALUES codegen: grows register file, loads literals/column refs/`rowid` into registers (`column%d` fallback), then `CodeGen_LoadFkChildKeys`. |
| 0x8270AD00 | sub_8270AD00 | FM2_SQLite_CodeGen_TryMinMaxIndexOptimization | 0.89 | SELECT min/max rewrite: matches `min`/`max` aggregate over indexed column, finds index term, emits optimized index opcodes 119/49; caller select planner. |
| 0x8270B4A0 | sub_8270B4A0 | FM2_SQLite_CodeGen_EmitUpsertDoUpdate | 0.90 | ON CONFLICT DO UPDATE: loops upsert terms, opcodes 87/106/91/105 insert path, IdxInsert 98 with table P4, SET expr list via opcode 118. |
| 0x82713B90 | sub_82713B90 | FM2_SQLite_Pager_AddDirtyPage | 0.89 | Marks page dirty in pager bitsets (+68/+72), links PgHdr into pager dirty list at +120; caller page write path. |
| 0x827016E8 | sub_827016E8 | FM2_SQLite_RegisterLikeGlobFunctions | 0.92 | Registers `like` (2/3 arg) and `glob` with `Vdbe_OpLikeWithEscape`; sets func flags on FindUserFunction entries; callee `RegisterBuiltinFunctions`. |
| 0x826F2308 | sub_826F2308 | FM2_SQLite_Parse_CreateForeignKey | 0.91 | FK constraint parse: validates column counts/refs, errors on mismatch/unknown column; allocates FK struct on parse context. |
| 0x826F2F40 | sub_826F2F40 | FM2_SQLite_CodeGen_CreateTable | 0.92 | CREATE TABLE driver: temp-name rules, auth 18/2/8, `sqlite_master` lookup, `"table %T already exists"`; pairs with `Schema_BuildCreateTableSql`. |
| 0x82705FB8 | sub_82705FB8 | FM2_SQLite_Schema_LoadStat1 | 0.90 | On schema open: resets window frames, runs `SELECT idx, stat FROM %Q.sqlite_stat1` via `Schema_ExecSqlLines` + affinity callback. |
| 0x82700B30 | sub_82700B30 | FM2_SQLite_Vdbe_OpLastInsertRowid | 0.91 | Builtin `last_insert_rowid()`: reads db+32, stores int64 to result mem; registered in builtin table entry 21. |
| 0x82700B70 | sub_82700B70 | FM2_SQLite_Vdbe_OpChanges | 0.91 | Builtin `changes()`: reads db+52 (`nChange`), stores to result; registered in builtin table entry 22. |
| 0x82700BB0 | sub_82700BB0 | FM2_SQLite_Vdbe_OpTotalChanges | 0.91 | Builtin `total_changes()`: reads db total-change counter, stores to result; registered in builtin table entry 23. |

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
| 0x826FE420 | sub_826FE420 | UTF-8 collated compare; defer string/collation cluster. |
| 0x826EB8E8 | sub_826EB8E8 | 8-byte db+32 reader; too trivial alone. |
| 0x826EC510 | sub_826EC510 | 12-byte Mem_SetRowid wrapper; too thin alone. |
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x8270DF80 | sub_8270DF80 | Syntax-error expr alloc; defer parse-error cluster. |
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
| 0x82719088 | sub_82719088 | Parse-stack reduce trampoline; thin wrapper. |
| 0x8271AA98 | sub_8271AA98 | Trigger func encoding validator; defer trigger parse cluster. |
| 0x8271EA98 | sub_8271EA98 | Index column load codegen; defer index codegen cluster. |
| 0x827212E0 | sub_827212E0 | D3D render-state dispatch; outside SQLite cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
