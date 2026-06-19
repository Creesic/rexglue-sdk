### Infrastructure pass 5 (33 functions)

Car-parts lookup, render/SQLite/XTS helpers, deferred-task and input utilities.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82252f98` | `FM2_CarParts_MergeUpgradePathListFromLookup` | Lookup upgrade path node; splice matching intrusive lists into output. |
| `0x82252350` | `FM2_CarParts_FindUpgradePathNodeByName` | Walk car-parts upgrade list; find node whose name matches lookup key. |
| `0x827fc3a8` | `FM2_STL_ListNode_GetPayloadAtOffset21` | Returns list-node payload pointer at `node+21` (camera-list helper). |
| `0x827fc398` | `FM2_STL_ListNode_GetPayloadAtOffset20` | Returns list-node payload pointer at `node+20`. |
| `0x827e3260` | `FM2_InputBindingState_Dtor` | Input binding-state dtor: reset vtable, free heap block at +32. |
| `0x8279f458` | `FM2_STL_ListNode_InitSentinelFromLinkPtr` | Init STL list sentinel via `FM2_STL_ListNode_InitSentinelB` on link ptr. |
| `0x82621670` | `FM2_DeferredTaskParams_GetField12` | Deferred-task params: read field at +12 after field-4 accessor. |
| `0x82505e18` | `FM2_Render_ForwardPassLightingArgs` | Forwards pass-lighting args to core renderer helper `sub_82725560`. |
| `0x8245ca18` | `FM2_HashName_AppendDecimalField` | Format int as decimal string; append hash-name field via `sub_821D2968`. |
| `0x8235c068` | `FM2_Stl_ThrowLengthError_VectorTooLong` | Raise `std::length_error` for `vector<T> too long`. |
| `0x827eaf00` | `FM2_Input_SortKeyboardScanMapPairs` | Bubble-sort keyboard scan-code pairs in binding map table. |
| `0x827d6f38` | `FM2_XmlTree_ResolveIndexedChildChain` | Recursively resolve XML/tree child by index chain; returns leaf or default. |
| `0x827d6b90` | `FM2_Diag_LogEventWithSubContext` | Diag wrapper: fetch sub-context then log event 22. |
| `0x8279da10` | `FM2_STL_Map_KeyCompare_WStringThunk` | STL map wstring key-compare thunk -> `sub_827F6180`. |
| `0x8279d910` | `FM2_XtsClient_AppendPayloadFromNode` | Append XTS client payload node from source descriptor. |
| `0x8278f2c0` | `FM2_SharedPtr_AssignAndReleaseOldRef` | Assign shared-ptr field at +4; release old ref with AddRef/Release. |
| `0x827796e0` | `FM2_STL_EhUnwind_ReleaseSharedPtrGuard` | EH unwind guard: release held ref at guard+4. |
| `0x82762b70` | `FM2_WebGate_LogAssertMessage` | Format WebGate assert string and dispatch to COM log interface. |
| `0x827624c8` | `FM2_LiveConnection_CloseXtsTask` | Close XTS task handle (`XTS_TaskClose` / `XTS_TASK_HANDLE`). |
| `0x82762450` | `FM2_LiveConnection_AddRefXtsTask` | AddRef/acquire XTS task handle when session valid. |
| `0x827321e0` | `FM2_Render_SetPassDrawOverride` | Set/clear render-pass draw override target and related pass flags. |
| `0x827257e0` | `FM2_Render_TransformPassLightingVectors` | Transform pass lighting vectors via matrix multiply (`sub_821D7400`). |
| `0x82721790` | `FM2_RenderTls_GetDrawPacketBatchBase` | Render TLS: return main context draw-packet base at `ctx+64`. |
| `0x8270d578` | `FM2_SQLite_ParseTree_DestroyRecursive` | Recursively destroy SQLite parse-tree nodes and free owned strings. |
| `0x8270d0b0` | `FM2_SQLite_ParseStack_DestroyEntries` | Destroy SQLite parse-stack entry array (counted triplets). |
| `0x826fd060` | `FM2_SQLite_Vdbe_PatchRecordChainEnd` | Vdbe finalize: patch prior record chain end index when closing slot. |
| `0x826fcda0` | `FM2_SQLite_Vdbe_BackpatchPriorStackCell` | Vdbe finalize: backpatch stack cell from current record index. |
| `0x826fcca0` | `FM2_SQLite_Vdbe_AppendOpcodeRecord` | Append 20-byte opcode record to Vdbe program array (grow if needed). |
| `0x826ef9b8` | `FM2_SQLite_Database_FreeAndMarkClosed` | Close SQLite database: release statements, mark closed, free block. |
| `0x824b9780` | `FM2_Lua_ProtectedCallWithTracebackRestore` | Protected Lua call helper: restore stack state after traceback hook. |
| `0x824b7bd0` | `FM2_LuaIo_DispatchFileOpStub` | Lua IO file-op dispatch stub: early return for op codes 0-7. |
| `0x82795520` | `FM2_DeferredTask_InitCallbackHolder` | Init deferred-task callback holder vtable + optional member invoke. |
| `0x82763670` | `FM2_DeferredTask_AssignSharedField4` | Deferred task: assign shared field from params field-4 via SharedPtr helper. |