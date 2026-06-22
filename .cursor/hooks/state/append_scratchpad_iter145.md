## Iteration 145

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82700840 | sub_82700840 | FM2_SQLite_Vdbe_OpRound | 0.91 | Builtin `round()` (1–2 arg): `ValueToDouble`, sprintf `"%.*f"`, `ParseAsciiDouble`; registered builtin entries 9–10. |
| 0x8271FBF8 | sub_8271FBF8 | FM2_SQLite_Pragma_EmitFlagQuery | 0.90 | Pragma flag query: opcode 45 + register text + opcode 54; callee of `Pragma_HandleDbFlag` when no value arg. |
| 0x826FF528 | sub_826FF528 | FM2_SQLite_Connection_ResetDeferredState | 0.89 | Resets connection deferred lists: snapshots mem-method chains, destroys parse contexts and deferred triggers, clears +116 flag. |
| 0x826FE6F0 | sub_826FE6F0 | FM2_SQLite_BtreeCursor_ReadColumnMem | 0.90 | Reads current row column 0 into mem: `GetRowid`, seek row, parse record tail codepoint, `InitMemFromColumnType`. |
| 0x826F2BD0 | sub_826F2BD0 | FM2_SQLite_CodeGen_RollbackTransaction | 0.90 | `ROLLBACK` statement: auth 22 on `"ROLLBACK"`, emits Halt opcode 14; caller parse reduce path. |
| 0x8270F868 | sub_8270F868 | FM2_SQLite_CodeGen_ExprToRegister | 0.89 | Codegen expr to register: `CodeGen_ExprNode`, if non-constant emits Bind opcode 118, sets node type 126. |
| 0x826F01F0 | sub_826F01F0 | FM2_SQLite_Mem_FreePoolChunk | 0.88 | Frees mem-pool chunk: unlinks from bucket list, invokes free callback at +16, destroys pool when count hits zero. |
| 0x826F2648 | sub_826F2648 | FM2_SQLite_CodeGen_AlterTableScanExistingRows | 0.87 | ALTER ADD COLUMN row scan: opcodes 109/45/71/3 loop; caller `CodeGen_AlterTableAddColumn`. |
| 0x826F2768 | sub_826F2768 | FM2_SQLite_DynArray_AppendZeroed | 0.90 | Grows dynamic array (2×+slack), zeroes new element (`memset`), returns index; callers parse-tree alloc paths. |
| 0x826FB3D0 | sub_826FB3D0 | FM2_SQLite_BtreeCursor_GetCellPayloadSize | 0.89 | Returns current cell payload byte count at cursor +48; parses cell header when needed; btree cursor read paths. |
| 0x82701538 | sub_82701538 | FM2_SQLite_AggregateFunc_CountStep | 0.91 | `count()` step: increments aux qword unless `*` arg is NULL type 5; registered as count aggregate step. |
| 0x82710028 | sub_82710028 | FM2_SQLite_Parse_DestroyObjectChain | 0.90 | Walks linked parse-object chain (+36): frees names, destroys parse trees/stacks/index lists per node. |

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
| 0x826EB8E8 | sub_826EB8E8 | 8-byte db+32 reader; too trivial alone. |
| 0x826EC510 | sub_826EC510 | 12-byte Mem_SetRowid wrapper; too thin alone. |
| 0x826F16E0 | sub_826F16E0 | Appends CHECK constraint AND-expr; thin yacc helper. |
| 0x826F7610 | sub_826F7610 | Thin wrapper; defer btree cluster. |
| 0x826FDEF8 | sub_826FDEF8 | Invokes cleanup callbacks by bitmask; defer mem-methods cluster. |
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
| 0x82719088 | sub_82719088 | Parse-stack reduce trampoline; thin wrapper. |
| 0x827212E0 | sub_827212E0 | D3D render-state name dispatch; outside SQLite cluster. |
| 0x82721970 | sub_82721970 | D3D VB/IB/decl creation; outside SQLite cluster. |
| 0x82721AB8 | sub_82721AB8 | Shader section loader (.data/.gpu); outside SQLite cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
