### Infrastructure pass 26 (33 functions)

Race ghost async playback, STL vector erase, GPU PM4 kick helpers, PNG/zlib, audio pump.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82331710` | `FM2_RaceGhost_CopyPlaybackUpdateArgs` | Evidence from decompile and caller context. |
| `0x82331d90` | `FM2_RaceGhost_EnqueueDeferredPlaybackTask` | Evidence from decompile and caller context. |
| `0x82332508` | `FM2_RaceGhost_SubmitPlaybackUpdateAsync` | Evidence from decompile and caller context. |
| `0x82331f10` | `FM2_RaceGhost_SchedulePlaybackUpdateTask` | Evidence from decompile and caller context. |
| `0x82357638` | `FM2_Stl_SlideStringRecords32Bytes` | Evidence from decompile and caller context. |
| `0x82365670` | `FM2_Stl_SlideRecords16Bytes` | Evidence from decompile and caller context. |
| `0x82357c08` | `FM2_Stl_Vector_EraseStringRangeAt` | Evidence from decompile and caller context. |
| `0x82365a40` | `FM2_Render_VectorEraseDrawRangeAt` | Evidence from decompile and caller context. |
| `0x8235a728` | `FM2_Input_SetWheelEnabledAndDetect` | Evidence from decompile and caller context. |
| `0x8235f9d8` | `FM2_Input_CopyRumbleDefaults88Bytes` | Evidence from decompile and caller context. |
| `0x82363368` | `FM2_Input_ControllerDevice_InitSslBindings` | Evidence from decompile and caller context. |
| `0x82364020` | `FM2_Memory_PoolHandlerCanFreeCategory` | Evidence from decompile and caller context. |
| `0x82367328` | `FM2_Memory_SetPhysicalAllocLockFlag` | Evidence from decompile and caller context. |
| `0x82367338` | `FM2_Memory_GetPhysicalAllocLockFlag` | Evidence from decompile and caller context. |
| `0x82366460` | `FM2_Memory_DeferredFreeRbTreeInsert` | Evidence from decompile and caller context. |
| `0x8236c828` | `FM2_GpuKick_SubmitShaderSyncPm4Bundle` | Evidence from decompile and caller context. |
| `0x8236c948` | `FM2_GpuKick_SubmitDrawSetupPm4Bundle` | Evidence from decompile and caller context. |
| `0x8236bd00` | `FM2_AudioRender_SubmitFrontBufferPath` | Evidence from decompile and caller context. |
| `0x823748d0` | `FM2_Render_ScopedBatch_FinalizeGpuKick` | Scoped batch teardown: sync GPU, release perf counters, free kick tag. |
| `0x82371250` | `FM2_GpuKick_SubmitViewportConstantPm4` | Evidence from decompile and caller context. |
| `0x82378940` | `FM2_GpuKick_SubmitTextureFetchPm4` | Evidence from decompile and caller context. |
| `0x823789d0` | `FM2_GpuKick_RotateMultiDrawTargetPm4` | Evidence from decompile and caller context. |
| `0x8237f358` | `FM2_GpuKick_ToggleClockGatingPm4` | Evidence from decompile and caller context. |
| `0x82356af8` | `FM2_BufFile_SeekAndTestPathPrefixMatch` | Evidence from decompile and caller context. |
| `0x8235ad90` | `FM2_UI_GetMaxPropertyAbsValueHalfStep` | Evidence from decompile and caller context. |
| `0x823815f0` | `FM2_AudioPumpThread_SignalWorkerEvent` | Evidence from decompile and caller context. |
| `0x82388cc8` | `FM2_Image_SwapEndian128BitRow` | Evidence from decompile and caller context. |
| `0x8239f1c8` | `FM2_Png_CompareSignatureBytes` | Evidence from decompile and caller context. |
| `0x823a4f90` | `FM2_Png_SetInterlaceHandlingFlag` | Evidence from decompile and caller context. |
| `0x823a4fa0` | `FM2_Png_SetBitDepth16Flag` | Evidence from decompile and caller context. |
| `0x823a4fc0` | `FM2_Png_ClampBitDepthToAtLeast8` | Evidence from decompile and caller context. |
| `0x823ae8e0` | `FM2_Zlib_CopyPendingInputToWindow` | Zlib deflate: copy pending input bytes into sliding window. |
| `0x822fd1c8` | `FM2_RaceGhost_MergeSortedKeyframeBufferSelfCheck` | Evidence from decompile and caller context. |