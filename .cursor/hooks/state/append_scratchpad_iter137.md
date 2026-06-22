## Iteration 137

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826F6E98 | sub_826F6E98 | FM2_SQLite_Btree_BeginWriteTransaction | 0.91 | Btree write txn entry: checks txn state (+8==2), calls `Pager_BeginWriteTransaction`, sets dirty flag (+12); pair of `Btree_BeginTransaction`. |
| 0x82713D38 | sub_82713D38 | FM2_SQLite_Pager_BeginWriteTransaction | 0.92 | Pager write path: allocates page bitmap, syncs journal header via I/O callbacks, sets dirty (+5); callee of btree begin-write. |
| 0x82708298 | sub_82708298 | FM2_SQLite_Expr_AssignRegisterRecursive | 0.90 | Walks expr tree via +12 sibling link; sets register +52 and ORs flag +2 on each node; self-recurses on +8 child. |
| 0x82708AD8 | sub_82708AD8 | FM2_SQLite_KeyInfo_AllocFromOrderBy | 0.89 | Allocates KeyInfo (5*n+16): copies ORDER BY expr pointers via `ExprSkipCollatePrefix`, stores sort-order bytes; callers index/FK codegen. |
| 0x826F26E0 | sub_826F26E0 | FM2_SQLite_Parse_InitWindowFrameBounds | 0.88 | Initializes window-function bound array: first slot=1000000, middle=5, tail descending 11..n; optional +24 flag sets extra bound=1. |
| 0x826F6F18 | sub_826F6F18 | FM2_SQLite_Btree_RollbackWriteTransaction | 0.91 | Inverse of begin-write: if dirty and not committed calls `Pager_ResetCommitState`, clears dirty (+12). |
| 0x826F63A0 | sub_826F63A0 | FM2_SQLite_Btree_SetPageSize | 0.90 | Validates power-of-two page size 512..32768, calls pager resize helper, updates usable-size fields (+20/+22); returns 8 if read-only. |
| 0x826F2950 | sub_826F2950 | FM2_SQLite_Parse_AppendSrcListEntry | 0.87 | If src-list count>0 pushes new entry via `ParseStack_PushEntry_0`, stores pointer at slot 18*n-12 in stack frame. |
| 0x82708B90 | sub_82708B90 | FM2_SQLite_CodeGen_EmitConstraintCheckLoop | 0.90 | FK/constraint check codegen: Vdbe labels, opcodes 17/2/45/107/118/105 by action type, `EmitFkCheckEpilogue`; string "c" for child table. |
| 0x82709090 | sub_82709090 | FM2_SQLite_CodeGen_LoadFkChildKeys | 0.89 | For each FK child column calls `FkInfo_GetReferencedColumnNames`, loads 4 register text fields via `Vdbe_SetRegisterText`. |
| 0x82708E78 | sub_82708E78 | FM2_SQLite_FkInfo_GetReferencedColumnNames | 0.90 | Resolves FK reference expr to table/column/db names; defaults "TEXT"/"INTEGER"/"rowid"; walks parse scope chain for column metadata. |
| 0x827093E0 | sub_827093E0 | FM2_SQLite_CodeGen_CreateViewFromSelect | 0.91 | View creation: flatten/resolve SELECT, alloc column-name table ("column%d"/"%T"), dequote; caller `Schema_MaterializeView`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826F14A8 | sub_826F14A8 | Sets FK action byte on last constraint only (44 bytes); too thin alone. |
| 0x826F2628 | sub_826F2628 | Small helper; insufficient evidence this pass. |
| 0x826F5F68 | sub_826F5F68 | Hash-entry clear at offset; defer hash cluster. |
| 0x826F6300 | sub_826F6300 | Small btree helper; needs paired analysis. |
| 0x826F6360 | sub_826F6360 | Small btree helper; needs paired analysis. |
| 0x826F6390 | sub_826F6390 | Thin wrapper (12 bytes). |
| 0x826F6440 | sub_826F6440 | Too trivial alone. |
| 0x826F6458 | sub_826F6458 | Too trivial alone. |
| 0x826F72F0 | sub_826F72F0 | Thin wrapper only. |
| 0x826F7310 | sub_826F7310 | Thin wrapper only. |
| 0x826FA1D0 | sub_826FA1D0 | Small helper; insufficient evidence. |
| 0x826FABB0 | sub_826FABB0 | Thin wrapper → sub_8242A348 (12 bytes). |
| 0x826FABC0 | sub_826FABC0 | Thin wrapper (12 bytes). |
| 0x826FABD0 | sub_826FABD0 | Thin wrapper (12 bytes). |
| 0x826FAC88 | sub_826FAC88 | Single-line `Btree_CheckMetaPageLock` wrapper; too thin alone. |
| 0x826FE880 | sub_826FE880 | Too trivial alone. |
| 0x826FE898 | sub_826FE898 | Too trivial alone. |
| 0x82700B30 | sub_82700B30 | Vdbe scalar wrapper (random/changes); thin forwarder cluster. |
| 0x82700B70 | sub_82700B70 | Vdbe scalar wrapper; thin forwarder cluster. |
| 0x82700BB0 | sub_82700BB0 | Vdbe scalar wrapper; thin forwarder cluster. |
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x82709C60 | sub_82709C60 | ORDER BY resolver; defer select cluster dedicated pass. |
| 0x8270ABE0 | sub_8270ABE0 | Index collation copy helper; defer index cluster. |
| 0x8270B388 | sub_8270B388 | FK codegen cluster; needs paired analysis. |
| 0x8270CCC8 | sub_8270CCC8 | Large parse helper; defer dedicated pass. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
