## Iteration 138

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82709C60 | sub_82709C60 | FM2_SQLite_Select_ResolveOrderByTerms | 0.92 | Resolves ORDER BY list against SELECT columns: `ExprExtractSortColumnIndex`, numeric position validation errors, recursive compound-select walk via `Select_FlattenSubqueries`. |
| 0x8270ABE0 | sub_8270ABE0 | FM2_SQLite_Index_CopyTokenStringsFromSchema | 0.89 | Walks index parse node column lists (+0/+20/+28) and calls `ParseToken_CopyFromTable` on each token; caller index collation setup. |
| 0x8270B388 | sub_8270B388 | FM2_SQLite_CodeGen_EmitAggregateDistinctSort | 0.91 | Aggregate DISTINCT codegen: emits opcodes 122/112 with `KeyInfo_AllocFromOrderBy`; error `"DISTINCT in aggregate must be followed by an expression"`. |
| 0x8270CCC8 | sub_8270CCC8 | FM2_SQLite_Parse_ResolveBindVariable | 0.90 | Resolves `?NN` bind names via atoi, assigns bind index, tracks max bind count (+28); named binds matched against prior list; error `"variable number must be between ?1 and ?%d"`. |
| 0x8270A158 | sub_8270A158 | FM2_SQLite_Select_GetOrderByExpr | 0.88 | Recursively walks compound SELECT chain (+32) then returns ORDER BY expr at index via `ExprSkipCollatePrefix`. |
| 0x8270D298 | sub_8270D298 | FM2_SQLite_Name_IsRowidAlias | 0.93 | Returns true for case-insensitive `"ROWID"`, `"OID"`, or `"_ROWID_"` via `StrcasecmpCollated`. |
| 0x8270D4F0 | sub_8270D4F0 | FM2_SQLite_Parse_AllocBetweenExpr | 0.89 | Allocates 0x44 BETWEEN parse node (type byte 0x93/-109), stores low/high bound expr pointers; OOM destroys parse stack. |
| 0x8270D5F8 | sub_8270D5F8 | FM2_SQLite_Token_DequoteOnce | 0.90 | One-shot identifier dequote: sets flag +2 bit 0x40, dupes token if needed, calls `DequoteIdentifier` on token string. |
| 0x827101F8 | sub_827101F8 | FM2_SQLite_Parse_AllocCreateIndexNode | 0.91 | Parser reduce helper: allocates type-100 node (0x2C), clones deep; caller `ParseTable_Reduce`; pairs with `CodeGen_CreateIndex`. |
| 0x826F5F68 | sub_826F5F68 | FM2_SQLite_HashEntry_ClearAtOffset | 0.87 | Clears hash slot at `base+offset`: releases chained entry (+76) via `HashEntry_Release`, zeroes slot flag byte. |
| 0x827103E8 | sub_827103E8 | FM2_SQLite_CodeGen_DropTrigger | 0.92 | DROP TRIGGER codegen: auth codes 16/14, opens schema/table, deletes from `sqlite_master`; callee of drop-trigger statement. |
| 0x827111D0 | sub_827111D0 | FM2_SQLite_CodeGen_DropTriggerStatement | 0.91 | DROP TRIGGER driver: schema lookup by name, error `"no such trigger: %S"`, calls `CodeGen_DropTrigger`; destroys trigger list. |

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
| 0x826FABB0 | sub_826FABB0 | Thin wrapper → sub_8242A348 (12 bytes). |
| 0x826FABC0 | sub_826FABC0 | Thin wrapper (12 bytes). |
| 0x826FABD0 | sub_826FABD0 | Thin wrapper (12 bytes). |
| 0x826FAC88 | sub_826FAC88 | Single-line `Btree_CheckMetaPageLock` wrapper; too thin alone. |
| 0x826FE880 | sub_826FE880 | Too trivial alone. |
| 0x826FE898 | sub_826FE898 | Too trivial alone. |
| 0x82700B30 | sub_82700B30 | Vdbe scalar wrapper; thin forwarder cluster. |
| 0x82700B70 | sub_82700B70 | Vdbe scalar wrapper; thin forwarder cluster. |
| 0x82700BB0 | sub_82700BB0 | Vdbe scalar wrapper; thin forwarder cluster. |
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x82710300 | sub_82710300 | Parser alloc type 98/99; rule mapping unclear without parse-table lookup. |
| 0x8270DE10 | sub_8270DE10 | Thin wrapper around `ParseTree_Walk`; defer with sub_8270DA88 cluster. |
| 0x8270DA88 | sub_8270DA88 | Large parse-tree walker (~900 bytes); needs dedicated pass. |
| 0x8270FD70 | sub_8270FD70 | Column authorization callback; defer auth cluster. |
| 0x82710558 | sub_82710558 | Schema hash removal helper; defer schema cluster. |
| 0x827126D0 | sub_827126D0 | Dependency-chain check; defer schema cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
