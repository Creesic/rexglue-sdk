## Iteration 139

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x8270FD70 | sub_8270FD70 | FM2_SQLite_Auth_CheckColumnAccess | 0.92 | Column-read auth callback (code 20): resolves table/column/ROWID, invokes xAuth; handles IGNORE(2)/DENY(1) with `"access to %s.%s.%s is prohibited"`. |
| 0x82710558 | sub_82710558 | FM2_SQLite_Schema_RemoveParseObjectFromHash | 0.90 | Inserts replacement into schema hash then unlinks/destroys prior parse object chain (+44); sets parse context dirty flag (+8) bit 0x10. |
| 0x827126D0 | sub_827126D0 | FM2_SQLite_Schema_TableHasDependentObjects | 0.91 | Walks schema dependency list checking `TriggerDependsOn`, `DependencyChainContains`, `FkeyDependsOn`; caller CREATE TRIGGER validation. |
| 0x82711D08 | sub_82711D08 | FM2_SQLite_AttachDatabase | 0.93 | ATTACH implementation: enforces max 10 DBs, txn guard, duplicate-name check, encoding match; errors include `"too many attached databases"`, `"database %s is already in use"`. |
| 0x82712000 | sub_82712000 | FM2_SQLite_DetachDatabaseByName | 0.92 | DETACH by alias: lookup attached DB array, rejects main/temp (`index<2`), txn guard, destroys btree handle; errors `"no such database"`, `"cannot detach database"`. |
| 0x82712390 | sub_82712390 | FM2_SQLite_RegisterAttachDetachFunctions | 0.93 | Registers builtins `"sqlite_attach"` (3-arg → AttachDatabase) and `"sqlite_detach"` (1-arg → DetachDatabaseByName) via `CreateUserFunction`. |
| 0x82712128 | sub_82712128 | FM2_SQLite_CodeGen_UserFunctionCall | 0.90 | UDF call codegen: resolves expr names, emits opcodes 18/13, `FindUserFunction` P4 -5; auth check via `CheckAuthorization`; destroys arg expr trees. |
| 0x82711490 | sub_82711490 | FM2_SQLite_CodeGen_EmitTriggerAssignFromNewRow | 0.89 | Trigger assignment codegen: opcode 35 setup, per-column NEW/OLD row opcodes 90/2, `MakeRecord` + opcode 51; caller trigger step emitters. |
| 0x82711558 | sub_82711558 | FM2_SQLite_CodeGen_EmitTriggerStepList | 0.88 | Iterates linked trigger-step list (+32 chain), calls `EmitTriggerAssignFromNewRow`, emits opcode 61 per step. |
| 0x827115D0 | sub_827115D0 | FM2_SQLite_CodeGen_EmitTriggerStepPrefix | 0.89 | Trigger step prefix: opcode 42 + step list + opcode 88; optional P4 table name (-2); patches jump via `PatchRecordChainEnd`. |
| 0x8270DA88 | sub_8270DA88 | FM2_SQLite_ParseTree_RegisterAutoincrementColumn | 0.90 | Parse-tree walk callback: on TK_COLUMN (148) registers autoincrement use in parse context arrays; dedupes table/column pairs. |
| 0x8270DE10 | sub_8270DE10 | FM2_SQLite_ParseTree_WalkAndCountAutoincrementErrors | 0.88 | Wraps `ParseTree_Walk` with autoincrement callback; returns parse error-count delta (+24); caller `CodeGen_SelectInExpr`. |

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
| 0x82700B30 | sub_82700B30 | Vdbe scalar wrapper; thin forwarder cluster. |
| 0x82700B70 | sub_82700B70 | Vdbe scalar wrapper; thin forwarder cluster. |
| 0x82700BB0 | sub_82700BB0 | Vdbe scalar wrapper; thin forwarder cluster. |
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x82710180 | sub_82710180 | Parser alloc type 109/99; rule mapping unclear. |
| 0x82710288 | sub_82710288 | Parser alloc type 99; rule mapping unclear. |
| 0x82710300 | sub_82710300 | Parser alloc type 98/99; rule mapping unclear. |
| 0x82712368 | sub_82712368 | Thin wrapper → `CodeGen_UserFunctionCall` for sqlite_attach only. |
| 0x82712410 | sub_82712410 | Auth-context init helper; defer auth cluster. |
| 0x8270DE68 | sub_8270DE68 | Thin wrapper looping `WalkAndCountAutoincrementErrors`. |
| 0x82715230 | sub_82715230 | Pager dirty-list reset; defer pager destroy cluster. |
| 0x827152E8 | sub_827152E8 | Pager destroy; defer pager cluster. |
| 0x82716198 | sub_82716198 | Value stack pop; defer stack cluster. |
| 0x82716238 | sub_82716238 | Value stack clear; defer stack cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
