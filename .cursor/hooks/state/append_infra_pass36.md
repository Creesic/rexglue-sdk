### Infrastructure pass 36 (33 functions)

Resource lock frame state, compression RB-tree, car audio voice, Lua stack/IO helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x824a3268` | `FM2_ResourceLock_ResolveFrameStateAndWalk` | Evidence from decompile and caller context. |
| `0x824a32e0` | `FM2_ResourceLock_EnterCritSecOrResolve` | Evidence from decompile and caller context. |
| `0x824a4b98` | `FM2_ResourceLock_WaitForReadyOrTimeout` | Evidence from decompile and caller context. |
| `0x824ab3a0` | `FM2_CompressionStream_DestroyRbTreeRoot` | Evidence from decompile and caller context. |
| `0x824ab238` | `FM2_CompressionStream_EraseRbTreeNode` | Evidence from decompile and caller context. |
| `0x824ae5f0` | `FM2_ComObject_InitRefCountBlockFields` | Evidence from decompile and caller context. |
| `0x824ae658` | `FM2_CarAudio_InitVoiceBufferBase` | Evidence from decompile and caller context. |
| `0x824ae6b0` | `FM2_CarAudio_AssignVoiceBufferVtables` | Evidence from decompile and caller context. |
| `0x824adaa8` | `FM2_CarAudio_InitVoiceBufferFromSelf` | Evidence from decompile and caller context. |
| `0x824adda0` | `FM2_CarAudio_AllocVoiceBufferNode` | Evidence from decompile and caller context. |
| `0x824adec0` | `FM2_CarAudio_ComputeUtf8CharWidth` | Evidence from decompile and caller context. |
| `0x824a8868` | `FM2_CarAudio_AllocStreamBufferAligned` | Evidence from decompile and caller context. |
| `0x824a7920` | `FM2_RenderAdapter_SwitchPresentationModePartial` | Evidence from decompile and caller context. |
| `0x824a8b20` | `FM2_CarAudio_TryStopStreamDecRef` | Evidence from decompile and caller context. |
| `0x824b36e0` | `FM2_RbTreeNode_LowerBoundByKey` | Evidence from decompile and caller context. |
| `0x824b6840` | `FM2_Lua_ShiftStackSlotDownAndCopy` | Evidence from decompile and caller context. |
| `0x824b6d18` | `FM2_Lua_TryCoerceStackSlotToNumber` | Evidence from decompile and caller context. |
| `0x824b6f80` | `FM2_Lua_GetBooleanFromStackSlot` | Evidence from decompile and caller context. |
| `0x824b7060` | `FM2_Lua_PushIntegerAsNumberSlot` | Evidence from decompile and caller context. |
| `0x824b7318` | `FM2_Lua_PushLightUserdataSlot` | Evidence from decompile and caller context. |
| `0x824b8280` | `FM2_Lua_PushInternedStringSlot` | Evidence from decompile and caller context. |
| `0x824b8788` | `FM2_Lua_RestoreSavedStackValue` | Evidence from decompile and caller context. |
| `0x824b88a8` | `FM2_Lua_GrowValueStackSlots` | Evidence from decompile and caller context. |
| `0x824b89e8` | `FM2_Lua_GrowCallInfoStack` | Evidence from decompile and caller context. |
| `0x824b9148` | `FM2_Lua_SetErrHandlerAndRestoreSlot` | Evidence from decompile and caller context. |
| `0x824b9848` | `FM2_LuaIO_ProtectedOpenFileCall` | Evidence from decompile and caller context. |
| `0x824bb218` | `FM2_LuaIO_DispatchFileOpStub` | Lua IO file-op dispatcher: grow buffer then finalize read/write stub. |
| `0x824b81a0` | `FM2_Lua_ParseStringToDouble` | Evidence from decompile and caller context. |
| `0x8245ced8` | `FM2_DeferredTaskParams_ReleaseChildCallback` | Evidence from decompile and caller context. |
| `0x8243c5e8` | `FM2_ResourceLock_FileChunkDtor` | Evidence from decompile and caller context. |
| `0x824b9f00` | `FM2_Lua_UpdateObjectGcMark` | Evidence from decompile and caller context. |
| `0x824b9268` | `FM2_Lua_IncrementCallDepthOrOverflow` | Evidence from decompile and caller context. |
| `0x824b8550` | `FM2_LuaSyntax_VaFormatExpectedToken` | Evidence from decompile and caller context. |