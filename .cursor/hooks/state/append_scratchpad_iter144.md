## Iteration 144

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8271EA98 | sub_8271EA98 | FM2_SQLite_CodeGen_LoadIndexColumns | 0.90 | Loads index key columns: `CodeGen_LoadColumn` then per-index-term opcodes 45 + collation P4 (-9); updates max register at +28. |
| 0x8271AA98 | sub_8271AA98 | FM2_SQLite_Parse_ValidateTriggerFuncEncoding | 0.88 | Validates trigger step encoding via `Parse_GetTriggerFuncEncoding`, checks WHEN/UPDATE token markers, dequotes func name, returns body offset. |
| 0x8270DF80 | sub_8270DF80 | FM2_SQLite_Parse_AllocSyntaxErrorBindExpr | 0.89 | Syntax-error recovery: `"near \"%T\": syntax error"`, alloc parse node 126, Variable opcode 90 + Bind 118 for placeholder. |
| 0x826F8858 | sub_826F8858 | FM2_SQLite_Btree_InsertCellOnPage | 0.90 | Inserts cell on btree page: defrag if needed, shift cell pointer array, memcpy payload, optional ptrmap type-3 overflow entry. |
| 0x826F8B60 | sub_826F8B60 | FM2_SQLite_Btree_InsertCellToOverflowPage | 0.89 | Overflow insert: allocates new page, moves tail cell, `InsertCellPayload` + `InsertCellOnPage`, ptrmap type 5, balance. |
| 0x826F82B0 | sub_826F82B0 | FM2_SQLite_Btree_ClearOverflowChain | 0.90 | Walks overflow page chain from cell header, `GetOrCreateCursor` + `Btree_FreePage` until chain ends; caller insert/delete paths. |
| 0x826FE420 | sub_826FE420 | FM2_SQLite_Utf8CollateCompare | 0.88 | UTF-8 codepoint collated strcmp using `byte_82118F74` table; xref from `Vdbe_ExecuteProgram` LIKE/compare paths. |
| 0x8271FA38 | sub_8271FA38 | FM2_SQLite_Pragma_ParseBoolKeyword | 0.91 | Parses pragma bools: digit 0-2, or matches `on`/`off`/`false`/`yes`/`true`/`full` string table; callee of `Pragma_HandleDbFlag`. |
| 0x8271FC78 | sub_8271FC78 | FM2_SQLite_Pragma_HandleDbFlag | 0.90 | Handles 13 db-flag pragmas (`vdbe_trace`, `count_changes`, `read_uncommitted`, etc.): query via `Pragma_EmitFlagQuery` or set/clear db+8 bits. |
| 0x826F1388 | sub_826F1388 | FM2_SQLite_Parse_AppendColumnDef | 0.91 | Appends column to CREATE TABLE list (+20 stride): duplicate-name check `"duplicate column name: %s"`, grows array, sets type byte 98. |
| 0x82700550 | sub_82700550 | FM2_SQLite_Vdbe_OpLength | 0.92 | Builtin `length()`: UTF-8 char count from token flag at +26; registered builtin table entry 5. |
| 0x82701488 | sub_82701488 | FM2_SQLite_AggregateFunc_AvgFinalize | 0.90 | `avg()` finalize: returns double from aux only when count at +20 > 0; registered as avg finalize (distinct from `OpAvgFinalize` for total). |

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
| 0x826F01F0 | sub_826F01F0 | Mem pool chunk unlink/free; defer mem-methods cluster. |
| 0x826F2BD0 | sub_826F2BD0 | ROLLBACK opcode 14 emitter; thin but defer txn cluster. |
| 0x826EB8E8 | sub_826EB8E8 | 8-byte db+32 reader; too trivial alone. |
| 0x826EC510 | sub_826EC510 | 12-byte Mem_SetRowid wrapper; too thin alone. |
| 0x826FE6F0 | sub_826FE6F0 | Btree cursor read column to mem; defer cursor cluster. |
| 0x826FF528 | sub_826FF528 | Connection deferred-state reset; defer connection cluster. |
| 0x82700840 | sub_82700840 | Builtin `round()` UDF; defer next scalar cluster. |
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x8270F868 | sub_8270F868 | Expr-to-register Bind codegen; defer expr cluster. |
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
| 0x8271FBF8 | sub_8271FBF8 | Pragma flag query emitter (opcodes 45/54); thin but pairs with `Pragma_HandleDbFlag`. |
| 0x827212E0 | sub_827212E0 | D3D render-state dispatch; outside SQLite cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
