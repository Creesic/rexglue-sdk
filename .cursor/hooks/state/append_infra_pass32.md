### Infrastructure pass 32 (33 functions)

PNG/JPEG helpers, STL deque iterators, compression/content/file-sys, async queue, FMOD.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823a4660` | `FM2_Png_EnsureRgbThenDecodeRow` | Evidence from decompile and caller context. |
| `0x823a4e80` | `FM2_Png_ValidateAndSetupRowDecode` | Evidence from decompile and caller context. |
| `0x823b01b0` | `FM2_Png_InvokeCustomDestroyCallback` | Evidence from decompile and caller context. |
| `0x823b0238` | `FM2_Png_AllocDecompressStateForChunk` | Evidence from decompile and caller context. |
| `0x823b26a8` | `FM2_Jpeg_InitDctCoefficientBuffers` | Evidence from decompile and caller context. |
| `0x823b6d10` | `FM2_Jpeg_InitIdctLookupTables` | Evidence from decompile and caller context. |
| `0x823b7c68` | `FM2_Jpeg_InitFastIdctLookupTables` | Evidence from decompile and caller context. |
| `0x82413d68` | `FM2_FMOD_SelectSignOrMagnitude` | fsel-based sign/magnitude selection for FMOD sin lookup path. |
| `0x82414bf0` | `FM2_Stl_DequeIterator_CtorFromIterator` | Evidence from decompile and caller context. |
| `0x82414c60` | `FM2_Stl_DequeIterator_CtorFromPtr` | Evidence from decompile and caller context. |
| `0x82414de8` | `FM2_Stl_DequeIterator_AdvanceByBlock` | Evidence from decompile and caller context. |
| `0x82414ee8` | `FM2_Stl_DequeIterator_FindBlockForIndex` | Evidence from decompile and caller context. |
| `0x82415090` | `FM2_Stl_DequeIterator_CopyRangeBlocks` | Evidence from decompile and caller context. |
| `0x82421120` | `FM2_Lua_StoreUnwindErrorGlobals` | Evidence from decompile and caller context. |
| `0x8242a768` | `FM2_CompressionStream_InitSentinelHead` | Resets compression stream intrusive list sentinel head. |
| `0x8242ccf8` | `FM2_Profile_IsContentDeviceReady` | Evidence from decompile and caller context. |
| `0x8242cd98` | `FM2_Lua_GetOverlappedAsyncResult` | Evidence from decompile and caller context. |
| `0x8242edc8` | `FM2_Storage_InitFileVolumeFromPath` | Evidence from decompile and caller context. |
| `0x8242f5c8` | `FM2_AsyncQueue_InitSemaphores` | Evidence from decompile and caller context. |
| `0x8242f758` | `FM2_ContentList_AssignRecordAndFreeBuffer` | Evidence from decompile and caller context. |
| `0x824302b8` | `FM2_ContentList_HeapSiftDownByCompare` | Evidence from decompile and caller context. |
| `0x8242fb48` | `FM2_AsyncOp_ReleasePlatformBuffer` | Evidence from decompile and caller context. |
| `0x824353c8` | `FM2_ContentRecord_AssignFromCopy` | Evidence from decompile and caller context. |
| `0x82435df0` | `FM2_ContentVector_MoveEraseRange36` | Evidence from decompile and caller context. |
| `0x82436e80` | `FM2_FileSys_Ctor` | Evidence from decompile and caller context. |
| `0x82436ef0` | `FM2_FileSys_Dtor` | Evidence from decompile and caller context. |
| `0x824343a8` | `FM2_FileSysWorker_CloseHandlesAndOptionalFree` | Evidence from decompile and caller context. |
| `0x824381d0` | `FM2_RaceGhost_EraseIntrusiveNodeFromList` | Evidence from decompile and caller context. |
| `0x824391b0` | `FM2_ContentManager_SnapshotChildListToBuffer` | Evidence from decompile and caller context. |
| `0x82464768` | `FM2_ContentBuffer_AllocTaggedCopyBuffer` | Evidence from decompile and caller context. |
| `0x824538b0` | `FM2_DeferredQueue_SampleElapsedTimestamp` | Evidence from decompile and caller context. |
| `0x82453a18` | `FM2_Set_IncrementIterator` | Evidence from decompile and caller context. |
| `0x8240c348` | `FM2_NtCloseHandleOrSetLastError` | Evidence from decompile and caller context. |