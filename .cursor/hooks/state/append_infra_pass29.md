### Infrastructure pass 29 (33 functions)

Livery mask workers, compression stream, race ghost sort, buf-file refs, audio pump ring, GPU pass alloc.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82439190` | `FM2_CompressionStream_ReleasePendingRef` | Evidence from decompile and caller context. |
| `0x824614d8` | `FM2_LiveryMask_InitPendingRecordHeader` | Evidence from decompile and caller context. |
| `0x82461568` | `FM2_LiveryMask_SpawnWorkerThread` | Spawns XAP worker thread for livery mask pending-record processing. |
| `0x8237b978` | `FM2_Render_EndPixCaptureAndRestoreDisplay` | PIX capture end: restore display mode and release capture surfaces. |
| `0x823a9450` | `FM2_Png_FreeTaggedReadStruct` | Evidence from decompile and caller context. |
| `0x8242a8f0` | `FM2_CompressionStream_ClearPendingLists` | Evidence from decompile and caller context. |
| `0x824cc0b0` | `FM2_BufFile_BindGlobalModuleRef` | Evidence from decompile and caller context. |
| `0x824cca60` | `FM2_BufFile_ResolveOrLoadModuleRef` | Evidence from decompile and caller context. |
| `0x824cc5e8` | `FM2_BufFile_EnsureCapacityForLength` | Evidence from decompile and caller context. |
| `0x8228e4f8` | `FM2_RaceGhost_CompareAndSwapKeyframeTriple` | Evidence from decompile and caller context. |
| `0x8228e5d0` | `FM2_RaceGhost_SiftDownKeyframeHeap` | Evidence from decompile and caller context. |
| `0x8228e6c8` | `FM2_RaceGhost_HeapifyAndRestoreRange` | Evidence from decompile and caller context. |
| `0x8228e9e8` | `FM2_RaceGhost_HeapsortPartitionThreeWay` | Evidence from decompile and caller context. |
| `0x8228ea88` | `FM2_RaceGhost_HeapifyKeyframeRange` | Evidence from decompile and caller context. |
| `0x8228eaf8` | `FM2_RaceGhost_InsertionSortKeyframeRange` | Insertion sort for small race-ghost keyframe subranges. |
| `0x8228ec20` | `FM2_RaceGhost_HeapSortRecursive` | Evidence from decompile and caller context. |
| `0x822f9ec0` | `FM2_RaceGhost_SplicePlaybackListNodes` | Evidence from decompile and caller context. |
| `0x823313a8` | `FM2_RaceGhost_InitPlaybackTaskWrapper` | Evidence from decompile and caller context. |
| `0x8235d070` | `FM2_UI_GetPropertyMaskByteAtOffset` | Evidence from decompile and caller context. |
| `0x82366100` | `FM2_Memory_AllocDeferredFreeMapNode24` | Evidence from decompile and caller context. |
| `0x82366090` | `FM2_Memory_AllocDeferredMapBlockLocked` | Evidence from decompile and caller context. |
| `0x8236ded0` | `FM2_Render_AddRefPassSurfaceAt12412` | Evidence from decompile and caller context. |
| `0x8236e1e0` | `FM2_Render_AddRefPassSurfaceAt12416` | Evidence from decompile and caller context. |
| `0x8236e538` | `FM2_Render_AllocGpuPassMemoryBlock` | Evidence from decompile and caller context. |
| `0x8242a708` | `FM2_CompressionStream_AllocListSentinel` | Evidence from decompile and caller context. |
| `0x8230f1a8` | `FM2_ReplayPendingString_Ctor` | Evidence from decompile and caller context. |
| `0x823296d8` | `FM2_Lua_PopLuaCarSetupPointerUserdata` | Evidence from decompile and caller context. |
| `0x82381428` | `FM2_AudioPump_ComputeRingBufferMarker` | Evidence from decompile and caller context. |
| `0x82381490` | `FM2_AudioPump_CopyWaveChunkToRing` | Evidence from decompile and caller context. |
| `0x82381590` | `FM2_AudioPump_FlushPendingWaveChunks` | Evidence from decompile and caller context. |
| `0x8236a460` | `FM2_D3D_ComputeSurfaceCopyPitch` | Evidence from decompile and caller context. |
| `0x82369a50` | `FM2_AudioRender_CopySurfaceRegionToBuffer` | Evidence from decompile and caller context. |
| `0x8236a8f0` | `FM2_D3D_ComputeSurfaceBlitRegion` | Evidence from decompile and caller context. |