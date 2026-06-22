### Infrastructure pass 39 (33 functions)

Network timed messages, Lua lexer/chunk load, buffered async read, resource lock, D3D frame slots.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82453e70` | `FM2_Network_AllocTimedMessageNode32` | Evidence from decompile and caller context. |
| `0x824554f0` | `FM2_Network_IncrementTimedMessageListCount` | Evidence from decompile and caller context. |
| `0x82455fb8` | `FM2_Network_InsertTimedMessageNodeBeforeIter` | Evidence from decompile and caller context. |
| `0x82456ae8` | `FM2_Network_QueueMessageWithDeadline` | Evidence from decompile and caller context. |
| `0x82459228` | `FM2_Network_DispatchMessageFromQueue` | Evidence from decompile and caller context. |
| `0x824bfc68` | `FM2_LuaSyntax_PeekNextLexerChar` | Evidence from decompile and caller context. |
| `0x824c2e88` | `FM2_LuaSyntax_LoadStringLiteralChunk` | Evidence from decompile and caller context. |
| `0x824c36a0` | `FM2_LuaSyntax_LoadBinaryPrecompiledChunk` | Evidence from decompile and caller context. |
| `0x824a0e00` | `FM2_ResourceLock_CompareFrameSlotOrder` | Evidence from decompile and caller context. |
| `0x82462e28` | `FM2_BufferedFileRead_CompleteAsyncReadLocked` | Evidence from decompile and caller context. |
| `0x82463050` | `FM2_BufferedFileRead_SignalAsyncReadComplete` | Evidence from decompile and caller context. |
| `0x824630e8` | `FM2_BufferedFileRead_AsyncReadFileLocked` | Evidence from decompile and caller context. |
| `0x82436060` | `FM2_AsyncOp_MatchAndInvokeCallbackRange` | Evidence from decompile and caller context. |
| `0x823297c0` | `FM2_Lua_XStorage_PushCarSetupUserdataFromProfile` | Evidence from decompile and caller context. |
| `0x82365f20` | `FM2_Memory_SearchDeferredMapBlockByTag` | Evidence from decompile and caller context. |
| `0x82368048` | `FM2_Memory_AllocOrFallbackToPool` | Evidence from decompile and caller context. |
| `0x82271d60` | `FM2_ComObject_InitRefCountFieldsFromSource` | Evidence from decompile and caller context. |
| `0x822721c8` | `FM2_ComObject_CopyConstructFieldBlock` | Evidence from decompile and caller context. |
| `0x823a5248` | `FM2_Png_EnsureRgbThenDecodeRow` | Evidence from decompile and caller context. |
| `0x8242fa60` | `FM2_AsyncOp_AcquireRefAndAllocTask` | Evidence from decompile and caller context. |
| `0x82436b00` | `FM2_FileSys_VectorEraseFromIterator` | Evidence from decompile and caller context. |
| `0x82437060` | `FM2_AsyncOp_QuickSortPartitionRecursive` | Evidence from decompile and caller context. |
| `0x8243c758` | `FM2_ResourceLock_CtorFileChunkHandle` | Evidence from decompile and caller context. |
| `0x82453900` | `FM2_CompressionStream_RbTreeRotateRight` | Evidence from decompile and caller context. |
| `0x8245a330` | `FM2_D3D_InitPresentThrottleSingleton` | Evidence from decompile and caller context. |
| `0x82467bc8` | `FM2_LapTracker_UpdateCarPosition` | Evidence from decompile and caller context. |
| `0x82491fb8` | `FM2_AudioVoice_ResizeChannelArray` | Evidence from decompile and caller context. |
| `0x824a1398` | `FM2_ResourceLock_WalkFrameSlotsWithCallback` | Evidence from decompile and caller context. |
| `0x824a1538` | `FM2_D3D_ResolveSubsystemFromState` | Evidence from decompile and caller context. |
| `0x824a2620` | `FM2_ResourceLock_CopyNextFrameSlotPair` | Evidence from decompile and caller context. |
| `0x824a3448` | `FM2_ResourceManager_AtomicIncPendingLoadCount` | Evidence from decompile and caller context. |
| `0x824a35c8` | `FM2_D3D_DecRefGpuFrameSlot` | Evidence from decompile and caller context. |
| `0x824ac388` | `FM2_RenderAdapter_ApplyPresentationModeSwitch` | Evidence from decompile and caller context. |