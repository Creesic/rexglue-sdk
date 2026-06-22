### Infrastructure pass 40 (33 functions)

Lap tracker spline, network message queue/RB-tree, content-record sort, com-object field blocks.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825872b8` | `FM2_CircularList_InitSentinelHead28` | Evidence from decompile and caller context. |
| `0x826107d8` | `FM2_Network_RbTreeLowerBoundInsertHint` | Evidence from decompile and caller context. |
| `0x82467c98` | `FM2_LapTracker_AdvanceSplineSampleIndex` | Evidence from decompile and caller context. |
| `0x82465838` | `FM2_LapTracker_InterpolateCarPositionOnSpline` | Evidence from decompile and caller context. |
| `0x822032b8` | `FM2_ComObject_DestroyFieldBlockRecursive` | Evidence from decompile and caller context. |
| `0x822712a8` | `FM2_ComObject_FindVectorIterByFieldAndCopy` | Evidence from decompile and caller context. |
| `0x82271b00` | `FM2_ComObject_CopyConstructPairFromVectorIter` | Evidence from decompile and caller context. |
| `0x82201940` | `FM2_ComObject_RemoveFieldSlotAndCompact` | Evidence from decompile and caller context. |
| `0x82365ea0` | `FM2_Memory_AllocTaggedSmallBlockFromPoolEntry` | Evidence from decompile and caller context. |
| `0x823d1610` | `FM2_BinaryReader_ReadAndByteSwapScalar` | Evidence from decompile and caller context. |
| `0x8242ab60` | `FM2_CompressionStream_RbTreeLowerBoundInsertHint` | Evidence from decompile and caller context. |
| `0x82453fe8` | `FM2_Network_RbTreeInitLowerBoundHint` | Evidence from decompile and caller context. |
| `0x82435370` | `FM2_ContentRecord_CopyConstructFrom` | Evidence from decompile and caller context. |
| `0x82435428` | `FM2_ContentRecord_SwapViaTemp` | Evidence from decompile and caller context. |
| `0x82435a20` | `FM2_Script_RegisterBindingNormalizePath` | Evidence from decompile and caller context. |
| `0x82435bc8` | `FM2_AsyncOp_MedianOfThreeContentRecords` | Evidence from decompile and caller context. |
| `0x82435d88` | `FM2_AsyncOp_QuickSortPartitionRange` | Evidence from decompile and caller context. |
| `0x82435e48` | `FM2_AsyncOp_IntroSortContentRecords` | Evidence from decompile and caller context. |
| `0x82435ee8` | `FM2_AsyncOp_HeapifyDownContentRecord` | Evidence from decompile and caller context. |
| `0x82436338` | `FM2_AsyncOp_PartitionContentRecords` | Evidence from decompile and caller context. |
| `0x82436460` | `FM2_AsyncOp_SortContentRecordSubrange` | Evidence from decompile and caller context. |
| `0x824563f8` | `FM2_Network_InitMessageChannelWithVectorReserve` | Evidence from decompile and caller context. |
| `0x824565f8` | `FM2_Network_InitMessageChannelWithDwordVector` | Evidence from decompile and caller context. |
| `0x82456e48` | `FM2_Network_InitMessageQueueFromSource` | Evidence from decompile and caller context. |
| `0x82458538` | `FM2_Network_DispatchDueMessagesFromTree` | Evidence from decompile and caller context. |
| `0x824586b0` | `FM2_Network_InitDeadlineTimerState` | Evidence from decompile and caller context. |
| `0x82455a58` | `FM2_Network_AssignMessageListFromSource` | Evidence from decompile and caller context. |
| `0x824546e0` | `FM2_Network_InitEmptyTimedMessageList` | Evidence from decompile and caller context. |
| `0x824556a0` | `FM2_Network_ReserveMessagePayloadVector` | Evidence from decompile and caller context. |
| `0x8229f6f8` | `FM2_Stl_VectorReserveDwordCapacity` | Evidence from decompile and caller context. |
| `0x8245dcb0` | `FM2_ComObject_FindListNodeByFieldOffset` | Evidence from decompile and caller context. |
| `0x82461a60` | `FM2_BufferedFileRead_InitAsyncReadRequest` | Evidence from decompile and caller context. |
| `0x824b8108` | `FM2_Lua_CountLeadingZeroBits8` | Evidence from decompile and caller context. |