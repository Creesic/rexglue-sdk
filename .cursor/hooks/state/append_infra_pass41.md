### Infrastructure pass 41 (33 functions)

Buffered file ring buffer, render presentation adapter, Lua stack grow, resource lock teardown.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82462020` | `FM2_BufferedFileRead_SubmitAsyncReadLocked` | Evidence from decompile and caller context. |
| `0x82462ba8` | `FM2_BufferedFileRead_GrowRingBufferCapacity` | Evidence from decompile and caller context. |
| `0x82462d70` | `FM2_BufferedFileRead_AppendRingBufferSlot` | Evidence from decompile and caller context. |
| `0x82481990` | `FM2_AIDriver_ResetRaceLineStateOnSectorChange` | Evidence from decompile and caller context. |
| `0x824a1448` | `FM2_ResourceLock_WalkFrameSlotsUntilMatch` | Evidence from decompile and caller context. |
| `0x824a7678` | `FM2_ResourceLock_GetNullFrameSlotSentinel` | Evidence from decompile and caller context. |
| `0x824a88b8` | `FM2_Memory_AllocArray32Checked` | Evidence from decompile and caller context. |
| `0x824a9b10` | `FM2_IntrusiveList_InitSentinelHead` | Evidence from decompile and caller context. |
| `0x824ae7c0` | `FM2_CarAudio_AppendVoiceIdAndInitBuffer` | Evidence from decompile and caller context. |
| `0x824b3498` | `FM2_RenderAdapter_DecRefPresentationCritSec` | Evidence from decompile and caller context. |
| `0x824b3430` | `FM2_RenderAdapter_IncRefPresentationCritSec` | Evidence from decompile and caller context. |
| `0x824b34f0` | `FM2_RenderAdapter_TryEnablePresentationSwitch` | Evidence from decompile and caller context. |
| `0x824b3630` | `FM2_RenderAdapter_EnterPresentationCritSecSingleton` | Evidence from decompile and caller context. |
| `0x824ba138` | `FM2_Lua_MarkObjectDuringStackGrow` | Evidence from decompile and caller context. |
| `0x824ba4c8` | `FM2_Lua_TraverseProtoUpvaluesForMark` | Evidence from decompile and caller context. |
| `0x824ba710` | `FM2_Lua_ComputeStackGrowSizeForObject` | Evidence from decompile and caller context. |
| `0x824b2d98` | `FM2_RenderAdapter_SwitchPresentationModePartial` | Evidence from decompile and caller context. |
| `0x826af9a0` | `FM2_D3D_ApplyPresentationThrottleGlobals` | Evidence from decompile and caller context. |
| `0x82412148` | `FM2_RenderAdapter_SetPresentationSlotMultiplier` | Evidence from decompile and caller context. |
| `0x8242d8a8` | `FM2_Lua_CreateComPtrFromThreeLuaNumbers` | Evidence from decompile and caller context. |
| `0x8236e320` | `FM2_AudioRender_AllocMixBufferRegion` | Evidence from decompile and caller context. |
| `0x82417950` | `FM2_Crt_HeapReallocOrSetErrno` | Evidence from decompile and caller context. |
| `0x824365d0` | `FM2_FileSys_ComparePathsCaseInsensitive` | Evidence from decompile and caller context. |
| `0x82455158` | `FM2_Network_AllocMessageListHeadNode` | Evidence from decompile and caller context. |
| `0x82453b18` | `FM2_CompressionStream_RbTreeLowerBoundByKey` | Evidence from decompile and caller context. |
| `0x82435ca0` | `FM2_AsyncOp_IntroSortInnerLoop` | Evidence from decompile and caller context. |
| `0x824a9910` | `FM2_IntrusiveList_AllocSentinelNode` | Evidence from decompile and caller context. |
| `0x82412160` | `FM2_RenderAdapter_GetPresentationSlotFromGlobals` | Evidence from decompile and caller context. |
| `0x82454230` | `FM2_Network_InitTimedMessageNodeFields` | Evidence from decompile and caller context. |
| `0x82453ed8` | `FM2_Network_CopyMessagePayloadQwords` | Evidence from decompile and caller context. |
| `0x824ae1b8` | `FM2_CarAudio_InitVoiceBufferRange` | Evidence from decompile and caller context. |
| `0x824a4108` | `FM2_ResourceLock_ReleaseFrameSlotsAndWalk` | Evidence from decompile and caller context. |
| `0x824a4488` | `FM2_ResourceLock_TeardownFrameSlotRange` | Evidence from decompile and caller context. |