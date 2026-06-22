### Infrastructure pass 34 (33 functions)

Network/hash RB-tree, async queue, buffered file read, AI driver, presentation/livery, auction house.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824a16b0` | `FM2_Presentation_GetCarResourceLoadCountB` | Evidence from decompile and caller context. |
| `0x824a2338` | `FM2_LiveryMask_ResourceManager_Ctor` | Evidence from decompile and caller context. |
| `0x824a0e20` | `FM2_LiveryRenderManager_ClearFinalizeEvent` | Evidence from decompile and caller context. |
| `0x824a0ec0` | `FM2_CriticalSection_SetRawEventSlot` | Evidence from decompile and caller context. |
| `0x824a0fd8` | `FM2_ResourceLock_DecrementRefUnderLock` | Evidence from decompile and caller context. |
| `0x824530f0` | `FM2_StreamRead_ReleasePriorAndGetCurrent` | Evidence from decompile and caller context. |
| `0x82453400` | `FM2_Input_DetectWheelSubtypeFromCaps` | Evidence from decompile and caller context. |
| `0x824540b0` | `FM2_HashName_LinkPropertyTreeNode` | Evidence from decompile and caller context. |
| `0x82454370` | `FM2_Network_AppendMessagePayloadNodes` | Evidence from decompile and caller context. |
| `0x82455eb0` | `FM2_Network_ClearMessageTreeRoot` | Evidence from decompile and caller context. |
| `0x824562f8` | `FM2_Network_InsertSortedMessageNode` | Evidence from decompile and caller context. |
| `0x8245b560` | `FM2_PropertyBag_InitRbTreeNodeFromKey` | Evidence from decompile and caller context. |
| `0x8245b1a8` | `FM2_HashName_EraseRbTreeNodeAndRebalance` | Evidence from decompile and caller context. |
| `0x8245eec0` | `FM2_RbTree_IncrementIteratorPastSentinel` | Evidence from decompile and caller context. |
| `0x8245f030` | `FM2_ContentDb_CountHashRangeNodes` | Evidence from decompile and caller context. |
| `0x8245f0b8` | `FM2_ContentDb_InitHashRangeIterator` | Evidence from decompile and caller context. |
| `0x824609c8` | `FM2_AsyncQueue_GlobalStaticInit` | Evidence from decompile and caller context. |
| `0x8245e898` | `FM2_CmdLineGlobal_StaticInit` | Evidence from decompile and caller context. |
| `0x8242fce8` | `FM2_FontSystem_DecrementRefAndMaybeClose` | Evidence from decompile and caller context. |
| `0x82430a58` | `FM2_AsyncOp_AllocAlignedPlatformBuffer` | Evidence from decompile and caller context. |
| `0x824321d0` | `FM2_AsyncOp_EnqueueUnderGlobalLock` | Evidence from decompile and caller context. |
| `0x82439470` | `FM2_Input_EraseControllerStateListNode` | Evidence from decompile and caller context. |
| `0x82439a68` | `FM2_D3D_Subscriber_TryEnableDeviceLocked` | Evidence from decompile and caller context. |
| `0x824621a8` | `FM2_BufferedFileRead_SyncOrAsyncRead` | Routes read through async wrapper or direct NtRead path. |
| `0x82463280` | `FM2_BufferedFileRead_SyncOrAsyncReadFile` | Evidence from decompile and caller context. |
| `0x824920e0` | `FM2_Sort_HeapSortDwordArrayInPlace` | Evidence from decompile and caller context. |
| `0x82494608` | `FM2_CircularBuffer_SelectSlotAndResize` | Evidence from decompile and caller context. |
| `0x8248f9c0` | `FM2_CircularBuffer_WrapReadIndices` | Evidence from decompile and caller context. |
| `0x8247dd38` | `FM2_AIDriver_ClearPathSegmentFlags` | Evidence from decompile and caller context. |
| `0x8247e2e8` | `FM2_AIDriver_SampleSteeringFromPath` | Evidence from decompile and caller context. |
| `0x82482290` | `FM2_AIDriver_ResetRaceLineState` | Evidence from decompile and caller context. |
| `0x824f1650` | `FM2_AuctionHouse_Ctor` | Constructs Forza2::CAuctionHouse with intrusive-list sentinel nodes. |
| `0x8243c640` | `FM2_ResourceLock_AssignHandleAndWaitReady` | Evidence from decompile and caller context. |