## Iteration 239

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827D76C8 | sub_827D76C8 | FM2_XmlNavigator_WalkChildrenFindMatchingType | 0.91 | Walks XML child chain; calls `FM2_Profile_LookupCategoryChildListById` per node; returns first match index via `*a6`. Used from `FinalizePresentationUnitStringBindings`. |
| 0x827E2140 | sub_827E2140 | FM2_Profile_LookupCategoryChildListById | 0.89 | Resolves profile category child list pointer from signed/unsigned category id; uses sorted dword lookup or category byte table. |
| 0x827D7F78 | sub_827D7F78 | FM2_IntrusiveList_FreeSubtreeNodesRecursively | 0.90 | Post-order free of intrusive-list/RB-tree nodes skipping sentinels (`+17` flag); recursive on child pointers. |
| 0x827D80F0 | sub_827D80F0 | FM2_IntrusiveList_ClearAndFreeAllNodes | 0.91 | Frees all nodes then reinitializes circular sentinel links; called from presentation loader dtor. |
| 0x824FFBE8 | sub_824FFBE8 | FM2_Memory_FindOrInsertFrameAllocMapIterator | 0.90 | RB-tree lower-bound search/insert on frame-alloc map keyed by dword; used when exporting presentation `xmlref`. |
| 0x824FF5B8 | sub_824FF5B8 | FM2_Memory_InsertFrameAllocMapRbTreeNode | 0.89 | Inserts new RB-tree node with key at `a5`; throws `map/set<T> too long` on overflow. |
| 0x824FFE28 | sub_824FFE28 | FM2_IntrusiveList_EraseNodesEqualRangeByKey | 0.88 | Builds equal-range on RB-tree24 then erases node span; returns erased count. |
| 0x826253C0 | sub_826253C0 | FM2_Lua_RegisterCColorToluaBindings | 0.93 | Registers `CColor` tolua module: `new`/`set`/`setColor`/`tostring`/`copy`/`.eq` and `r/g/b/a` property accessors. |
| 0x82628DE0 | sub_82628DE0 | FM2_Lua_CColorToluaGcCollector | 0.92 | Tolua GC: frees CColor userdata block and pops stack. |
| 0x82624998 | sub_82624998 | FM2_Lua_CColorSetRgbaFromStack | 0.92 | Lua `Color.set` binding; validates four number args + self; calls `sub_827EB1A0` to set RGBA. |
| 0x8262B820 | sub_8262B820 | FM2_Lua_CMaterialGetAmbientColor | 0.91 | Material `ambient` getter; type-checks self; pushes `CColor` userdata from lazy ambient field. |
| 0x827DD108 | sub_827DD108 | FM2_SceneGraph_GetNodeFromHandle | 0.88 | Returns `*(_DWORD*)a2`; sole behavior across binding export/import and scene-graph destroy paths. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x827D47C8 | sub_827D47C8 | Large XGF presentation import (~1080 B); needs dedicated pass. |
| 0x827D5FD8 | sub_827D5FD8 | 8-byte `FM2_ComPtr_AssignRef` wrapper at `a1+272`. |
| 0x824FFDF8 | sub_824FFDF8 | Thin wrapper forwarding to `FindOrInsertFrameAllocMapIterator` at `a1+72`. |
| 0x82500290 | sub_82500290 | Thin wrapper to `EraseNodesEqualRangeByKey` at `a1+72`. |
| 0x8262BB68 | sub_8262BB68 | `CLuaObjectMaterial` module registration; defer material property cluster. |
| 0x8262AEA0 | sub_8262AEA0 | `CLuaTimeContext` module registration; defer time-context cluster. |
| 0x82631DA8 | sub_82631DA8 | 4-byte thunk to `sub_82631C50`. |
| 0x82632F40 | sub_82632F40 | Thin `FM2_Lua_SetStackTop(a1, -2)` after module registration. |
