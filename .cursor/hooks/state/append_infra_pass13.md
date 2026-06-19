### Infrastructure pass 13 (33 functions)

Render adapter timing, D3D hang path, buf-file, hash-table RB-tree, content entry.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x821e7760` | `FM2_Stl_StringIter_InitFromBufferBounds` | Init string iterator with bounds check against STL string buffer. |
| `0x824aee08` | `FM2_RenderAdapter_GetFrameTimingVtablePtr` | Returns static frame-timing vtable pointer `off_8299B8C4`. |
| `0x821f1488` | `FM2_RenderAdapter_ApplyFrameTimingDeltaFlagFB` | Evidence from decompile and caller context. |
| `0x8221b4e8` | `FM2_PropertyBag_AllocNodePool128xCount` | Pool alloc `0x80 * count` for property-bag nodes. |
| `0x82369470` | `FM2_D3D_SyncRingBufferSlotAfterGpuError` | Evidence from decompile and caller context. |
| `0x8240bff8` | `FM2_Xam_ShowDirtyDiscAndRelaunch` | Shows dirty-disc UI then `XLaunchNewImage` (GPU hang path). |
| `0x82413490` | `FM2_Crt_VsprintfS_L` | Thin `vsprintf_s_l` CRT wrapper. |
| `0x824615e8` | `FM2_D3D_SuspendLowPriorityWorkerThreads` | Evidence from decompile and caller context. |
| `0x824a68f0` | `FM2_ResourceLock_WaitReadyState3` | Evidence from decompile and caller context. |
| `0x8258b2f8` | `FM2_AllocPoolAcquire44xCount` | Pool alloc `44 * count` (FMOD/network RB-tree nodes). |
| `0x8253f108` | `FM2_HashNamePropertyList_EraseNodeRebalance` | RB-tree erase/rebalance for hash-name property list. |
| `0x821d7980` | `FM2_Render_MatrixMultiplyVMX128_From16Byte` | Evidence from decompile and caller context. |
| `0x821d9ef8` | `FM2_BufFile32768_FlushWriteBuffer` | Evidence from decompile and caller context. |
| `0x821df050` | `FM2_BufFile_CompareRangeSubstr` | Evidence from decompile and caller context. |
| `0x821df128` | `FM2_BufFile32768_DtorInner` | Tears down buf-file stream vtables and decrefs camera script. |
| `0x821e6238` | `FM2_Render_TransformVec4x4VMX128` | Evidence from decompile and caller context. |
| `0x821e6e40` | `FM2_Stl_SnprintfPartNames128` | Evidence from decompile and caller context. |
| `0x821e7678` | `FM2_RenderAdapter_SetVblankWaitState` | Evidence from decompile and caller context. |
| `0x821e82f8` | `FM2_SceneObject_ReleaseRefFields78_80_88` | Evidence from decompile and caller context. |
| `0x821ec2d0` | `FM2_ContentEntry_CopyTailFields384` | Evidence from decompile and caller context. |
| `0x821ef7f8` | `FM2_ContentEntry_CopyVec4AndSubrecord` | Evidence from decompile and caller context. |
| `0x821f13b8` | `FM2_RenderAdapter_ApplyFrameTimingDeltaFlagFD` | Evidence from decompile and caller context. |
| `0x821f1f70` | `FM2_RenderAdapter_QueueFrameTimingUpdateInner` | Evidence from decompile and caller context. |
| `0x821f2330` | `FM2_RenderAdapter_TogglePresentInterval` | Evidence from decompile and caller context. |
| `0x821f3b08` | `FM2_Animation_ClampKeyframeWeightVMX` | Evidence from decompile and caller context. |
| `0x82202310` | `FM2_HashTable_SetEntryWithRefAdd` | Evidence from decompile and caller context. |
| `0x82203918` | `FM2_HashTableList_EraseRangeIterators` | Evidence from decompile and caller context. |
| `0x822041f8` | `FM2_NetNotification_Ctor` | Evidence from decompile and caller context. |
| `0x821e6a10` | `FM2_RenderAdapter_GetFieldAt10036` | Evidence from decompile and caller context. |
| `0x821f0ff8` | `FM2_RenderAdapter_UpdateSceneNodeDrawState` | Evidence from decompile and caller context. |
| `0x821f0e30` | `FM2_RenderAdapter_MarkFrameTimingDirty` | Evidence from decompile and caller context. |
| `0x822034c0` | `FM2_HashTableList_ClearSentinelLinks` | Evidence from decompile and caller context. |
| `0x821ef130` | `FM2_ContentEntry_CopyDwordVector` | Evidence from decompile and caller context. |