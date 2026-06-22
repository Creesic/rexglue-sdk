### Infrastructure pass 37 (33 functions)

Network/compression iterators, resource lock frame resolve, async alloc, Lua IO, D3D singletons.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82453bf0` | `FM2_Network_IncrementMessageTreeIterator` | Evidence from decompile and caller context. |
| `0x824a31a8` | `FM2_ResourceLock_ResolveFrameAllocatorState` | Evidence from decompile and caller context. |
| `0x824bafa0` | `FM2_Lua_GrowStackForFileOp` | Evidence from decompile and caller context. |
| `0x824bf910` | `FM2_Lua_UnlinkOpenUpvaluesAbove` | Evidence from decompile and caller context. |
| `0x82534638` | `FM2_AsyncQueue_AllocSentinelNodeArray40` | Evidence from decompile and caller context. |
| `0x82586b78` | `FM2_CmdLine_AllocCircularListNodeArray28` | Evidence from decompile and caller context. |
| `0x826177a8` | `FM2_FrameAllocMap_AllocNode24` | Evidence from decompile and caller context. |
| `0x82621210` | `FM2_RbTree_RotateLeftAtChild` | RB-tree left rotation used during insert rebalance. |
| `0x827e7f70` | `FM2_Stl_VectorEmplacePairAtEndOrRealloc` | Evidence from decompile and caller context. |
| `0x82299dc0` | `FM2_AudioSample_RbTreeLowerBoundByTotals` | Evidence from decompile and caller context. |
| `0x82424bf0` | `FM2_Lua_GetCharClassTable` | Evidence from decompile and caller context. |
| `0x824301b8` | `FM2_AsyncOp_FindMatchingQueueBlock` | Evidence from decompile and caller context. |
| `0x82430718` | `FM2_AsyncOp_TryAcquireFreeBlockByIndex` | Evidence from decompile and caller context. |
| `0x82453b68` | `FM2_CompressionStream_IncrementTreeIterator` | Evidence from decompile and caller context. |
| `0x82454150` | `FM2_Network_InitEmptyMessageTreeHead` | Evidence from decompile and caller context. |
| `0x82453f28` | `FM2_Network_DestroyMessageTreeRecursive` | Evidence from decompile and caller context. |
| `0x82455848` | `FM2_Network_RbTreeLowerBoundByMessageKeyByte` | Evidence from decompile and caller context. |
| `0x8245ccb8` | `FM2_DeferredTaskParams_FreeIfOutsidePool` | Evidence from decompile and caller context. |
| `0x824bad00` | `FM2_LuaIO_ResetLexStateForNextOp` | Evidence from decompile and caller context. |
| `0x824bb320` | `FM2_Lua_LinkProtoToGcObject` | Evidence from decompile and caller context. |
| `0x824a2298` | `FM2_ResourceLock_WalkFrameSlotRange` | Evidence from decompile and caller context. |
| `0x824a37b0` | `FM2_D3D_RegisterGlobalDeviceSingletonD` | Evidence from decompile and caller context. |
| `0x824a38f8` | `FM2_D3D_RegisterGlobalDeviceSingletonA` | Evidence from decompile and caller context. |
| `0x824a3a30` | `FM2_D3D_RegisterGlobalDeviceSingletonB` | Evidence from decompile and caller context. |
| `0x824a3b68` | `FM2_D3D_RegisterGlobalDeviceSingletonC` | Evidence from decompile and caller context. |
| `0x824b3570` | `FM2_RenderAdapter_DecRefPresentationSwitch` | Evidence from decompile and caller context. |
| `0x824b35b8` | `FM2_RenderAdapter_GetPresentationSwitchFlag` | Evidence from decompile and caller context. |
| `0x824b3658` | `FM2_RenderAdapter_InitPresentationCritSec` | Evidence from decompile and caller context. |
| `0x824adb08` | `FM2_CarAudio_ComputeUtf8EncodedSize` | Evidence from decompile and caller context. |
| `0x82454558` | `FM2_Network_FindSchedulerNodeByDeadline` | Evidence from decompile and caller context. |
| `0x82456818` | `FM2_Network_InsertTimedMessageBefore` | Evidence from decompile and caller context. |
| `0x8242f658` | `FM2_AsyncQueue_DecRefAndMaybeDestroy` | Evidence from decompile and caller context. |
| `0x824ae128` | `FM2_CircularBuffer_MoveEraseRange8` | Evidence from decompile and caller context. |