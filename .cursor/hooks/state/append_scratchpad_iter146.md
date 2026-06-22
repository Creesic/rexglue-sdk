## Iteration 146

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x826F16E0 | sub_826F16E0 | FM2_SQLite_Parse_AppendCheckConstraintExpr | 0.89 | Yacc helper: dups CHECK expr, ANDs onto table constraint list at +52 via `ParseTree_BuildAndExpr`; caller `ParseTable_Reduce`. |
| 0x826FDEF8 | sub_826FDEF8 | FM2_SQLite_MemMethods_InvokeCleanupByMask | 0.88 | Invokes registered cleanup callbacks (+8 stride) whose bit is clear in mask; callers btree teardown / vdbe paths. |
| 0x82710180 | sub_82710180 | FM2_SQLite_ParseTree_AllocSelectInDmlNode | 0.88 | Allocates 0x2C DML node type 109 (SELECT-IN per `CodeGen_EmitDmlList`); `ParseNode_CloneDeep`; yacc reduce caller. |
| 0x82710288 | sub_82710288 | FM2_SQLite_ParseTree_AllocUpdateDmlNode | 0.90 | Allocates DML node type 99 (UPDATE per `CodeGen_EmitDmlList`); stores table/expr/idlist fields; yacc reduce. |
| 0x82710300 | sub_82710300 | FM2_SQLite_ParseTree_AllocDeleteDmlNode | 0.90 | Allocates DML node type 98 (DELETE per `CodeGen_EmitDmlList`); yacc reduce caller `ParseTable_Reduce`. |
| 0x82717E88 | sub_82717E88 | FM2_SQLite_DateTime_InitOpTime | 0.91 | UDF init probe: calls `DateTime_OpTime` with `'now'` test value via `AllocErrorObject`; datetime registration table. |
| 0x82717F00 | sub_82717F00 | FM2_SQLite_DateTime_InitOpDate | 0.91 | UDF init probe: calls `DateTime_OpDate` with `'now'`; chained in datetime registration table. |
| 0x82717F78 | sub_82717F78 | FM2_SQLite_DateTime_InitOpDatetime | 0.91 | UDF init probe: calls `DateTime_OpDatetime` with `'now'`; callee before `RegisterDateTimeFunctions`. |
| 0x82719088 | sub_82719088 | FM2_SQLite_ParseStack_ReduceAndDispatch | 0.89 | Pops parse stack until empty then calls reduce callback; used by yacc driver at `0x82707C14`. |
| 0x826F7610 | sub_826F7610 | FM2_SQLite_BtreeCursor_LoadPageOrGotoChild | 0.89 | `BtreeCursor_LoadPage`; if +65 set calls `GotoCellChildPage` else marks done; btree cursor walk helper. |
| 0x827212E0 | sub_827212E0 | FM2_Render_LookupStateSetterByName | 0.90 | Dispatches render-state name to setter: `SHADERCONSTANT_OFFSET`→noop, `D3DRS_CULLMODE`→`sub_82721278`, else alpha-test lookup. |
| 0x82721BD8 | sub_82721BD8 | FM2_Render_UploadPassMatrices | 0.89 | Uploads world/view/proj matrix constants via `RenderContext_UploadMatrixConstants` + VMX multiply; post-draw-pass tail. |

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
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x8270FFE8 | sub_8270FFE8 | Prepend deferred-object list (28 bytes); too thin alone. |
| 0x82710008 | sub_82710008 | Unlink deferred-object list (32 bytes); too thin alone. |
| 0x82713110 | sub_82713110 | Pager journal-mode flag setter; thin helper. |
| 0x82713520 | sub_82713520 | Pager page-size field accessor; too trivial alone. |
| 0x82716080 | sub_82716080 | Zeros 3 dwords only; too trivial alone. |
| 0x82721278 | sub_82721278 | D3DRS_CULLMODE setter thunk (72 bytes); defer render setter cluster. |
| 0x827213E8 | sub_827213E8 | Render pass state resolve; defer render pass cluster. |
| 0x82721970 | sub_82721970 | D3D VB/IB/decl creation; defer render resource cluster. |
| 0x82721AB8 | sub_82721AB8 | Shader section loader; defer render resource cluster. |
| 0x82722808 | sub_82722808 | Large function (~2KB); needs dedicated pass. |
