### Infrastructure pass 14 (33 functions)

FileSys/config vectors, hash-table RB-tree, profile Lua, resource lock, buf-file.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825720c8` | `FM2_ConfigEntryVector_FreeBuffer` | Frees config-entry vector buffer via destroy-range helper. |
| `0x821e6ab0` | `FM2_RenderAdapter_SetPresentIntervalMode` | Evidence from decompile and caller context. |
| `0x821ea1d8` | `FM2_ContentEntry_CopyAssignHead304` | Evidence from decompile and caller context. |
| `0x821eecc0` | `FM2_ContentEntry_ReserveDwordVector` | Evidence from decompile and caller context. |
| `0x821f1a48` | `FM2_Animation_NormalizeKeyframeWeightVMX` | Evidence from decompile and caller context. |
| `0x82203420` | `FM2_HashTableList_DestroySubtreeNodes` | Evidence from decompile and caller context. |
| `0x82203518` | `FM2_HashTableList_EraseNodeRebalance` | RB-tree erase/rebalance sibling to hash-name list helper. |
| `0x82206100` | `FM2_ConfigEntryVector_DestroyAndFree` | Evidence from decompile and caller context. |
| `0x824ce030` | `FM2_FileSysStream_DestroyNested` | Evidence from decompile and caller context. |
| `0x8242cb90` | `FM2_RedirectStream_Dtor` | Evidence from decompile and caller context. |
| `0x82411d20` | `FM2_Thread_SleepMilliseconds` | Wraps `KeDelayExecutionThread` for millisecond sleep. |
| `0x824d3730` | `FM2_ComObject_Ctor16BytePool` | Evidence from decompile and caller context. |
| `0x8245a478` | `FM2_D3D_GetGlobalPresentThrottleSingleton` | Evidence from decompile and caller context. |
| `0x824a5608` | `FM2_ResourceLock_WaitForReadyOrTimeout` | Evidence from decompile and caller context. |
| `0x824cfe00` | `FM2_BufFile_TrySeekPosition` | Evidence from decompile and caller context. |
| `0x824cfeb8` | `FM2_BufFile_GetStreamTell` | Evidence from decompile and caller context. |
| `0x824cfff8` | `FM2_BufFile_ReleaseRefCount` | Evidence from decompile and caller context. |
| `0x82206208` | `FM2_FileSys_DestroyEntryRange` | Evidence from decompile and caller context. |
| `0x8220b658` | `FM2_SceneGraph_CompareNodeNamePrefix` | Evidence from decompile and caller context. |
| `0x8220c8e8` | `FM2_ProfileLua_InvokeManagerCallback` | Evidence from decompile and caller context. |
| `0x8220c9d8` | `FM2_Lua_BindingVector_DecrementIterByIndex` | Evidence from decompile and caller context. |
| `0x8220cb60` | `FM2_ProfileLua_InitBindingContext` | Evidence from decompile and caller context. |
| `0x822023e8` | `FM2_HashTableNode_DtorOptionalFree` | Evidence from decompile and caller context. |
| `0x82252f40` | `FM2_ConfigEntry_DestroyRange` | Evidence from decompile and caller context. |
| `0x821e7218` | `FM2_ContentEntry_CopyMemcpyBlock128` | Evidence from decompile and caller context. |
| `0x825dcd20` | `FM2_AllocPoolAcquire4xCount` | Evidence from decompile and caller context. |
| `0x8242bc68` | `FM2_RefCountedThreadSafe_AssignBaseVtable` | Evidence from decompile and caller context. |
| `0x8220e3a0` | `FM2_ProfileLua_IsRegistryValueString` | Evidence from decompile and caller context. |
| `0x8220e408` | `FM2_ProfileLua_RegisterManagerClosure` | Evidence from decompile and caller context. |
| `0x8221a870` | `FM2_AudioManager_RouteInitByCmdlineFlag` | Branches audio init on Forza cmdline flag at +1048. |
| `0x8221b688` | `FM2_Profile_GetManagerHeapIfAlloc` | Evidence from decompile and caller context. |
| `0x82225058` | `FM2_BufferedStream_CtorRetainSource` | Evidence from decompile and caller context. |
| `0x82204d08` | `FM2_FileSysEntry_ReleaseRefAndClearString` | Evidence from decompile and caller context. |