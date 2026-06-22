## Iteration 140

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82715230 | sub_82715230 | FM2_SQLite_Pager_ResetDirtyPageList | 0.90 | Clears pager dirty-page linked list (+116), resets write counters; may call `Pager_Rollback`; invoked when pager ref-count hits zero via `HashEntry_Release`. |
| 0x827152E8 | sub_827152E8 | FM2_SQLite_Pager_Close | 0.91 | Pager teardown: rollback if txn state>1, free dirty pages, invoke VFS close callbacks (+88/+92/+96), free journal buffer and pager struct. |
| 0x82713170 | sub_82713170 | FM2_SQLite_Pager_OpenVfsFile | 0.92 | Opens DB via VFS: `:memory:` fast path, path open with retry loop, journal-mode probing (`-journal`); caller btree attach/open path. |
| 0x82716098 | sub_82716098 | FM2_SQLite_ValueStack_Push | 0.90 | Pushes 64-bit value onto chunked stack (alloc 176 or grow); increments count at +0; pairs with `ValueStack_Pop`. |
| 0x82716198 | sub_82716198 | FM2_SQLite_ValueStack_Pop | 0.91 | Pops qword from stack chunk; returns 101 when empty; frees exhausted chunk and clears stack head. |
| 0x82716238 | sub_82716238 | FM2_SQLite_ValueStack_Clear | 0.89 | Frees all stack chunks via +12 chain, zeros stack header fields (+0/+4/+8). |
| 0x827177B8 | sub_827177B8 | FM2_SQLite_DateTime_OpStrftime | 0.90 | SQL strftime-like: `TokensMatchInterval` + `DateTime_FormatToken`, stores formatted text to result mem. |
| 0x82717810 | sub_82717810 | FM2_SQLite_DateTime_OpDatetime | 0.91 | datetime() UDF: parses modifier tokens, formats `"%04d-%02d-%02d %02d:%02d:%02d"` via sprintf, appends to result. |
| 0x82716D68 | sub_82716D68 | FM2_SQLite_DateTime_ApplyLocaltimeModifier | 0.92 | `localtime` modifier: converts Julian context via `mktime`/`localtime_r` helpers, returns delta; callee of `DateTime_ApplyModifier`. |
| 0x82712410 | sub_82712410 | FM2_SQLite_Auth_InitSchemaContext | 0.88 | Initializes 16-byte auth walk context: db index, schema ptr, object-type/name out-ptrs; skips main/temp (index<0 or ==1). |
| 0x8271DED8 | sub_8271DED8 | FM2_SQLite_CodeGen_BuildRecordAffinityP4 | 0.90 | Lazily builds column-affinity string from table metadata (+18 per column), sets Vdbe P4 on prior opcode; caller trigger/MakeRecord paths. |
| 0x8271D238 | sub_8271D238 | FM2_SQLite_CodeGen_SetColumnCollSeqP4 | 0.89 | For non-virtual tables sets collation seq P4 (-8) on prior opcode via `ValueToErrorObject`; used in trigger column load codegen. |

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
| 0x8270DE68 | sub_8270DE68 | Thin wrapper looping `WalkAndCountAutoincrementErrors` over expr list. |
| 0x827178B0 | sub_827178B0 | time() UDF; defer remaining datetime scalar cluster. |
| 0x8271DF90 | sub_8271DF90 | Insert affinity P4 builder; defer paired codegen cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
