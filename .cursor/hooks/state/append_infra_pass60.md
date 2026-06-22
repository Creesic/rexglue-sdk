### Infrastructure pass 60 (33 functions)

PNG read/validate, pass-lighting offsets/VMX, D3D texture helpers, race ghost, livery mask.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823b1500` | `FM2_Image_LoadPngReadChunkBytesToBuffer` | Evidence from decompile and caller context. |
| `0x827281d0` | `FM2_Render_ComputePassLightingResourceOffset64B` | Evidence from decompile and caller context. |
| `0x8239f2f8` | `FM2_Image_LoadPngValidateStreamAfterRead` | Evidence from decompile and caller context. |
| `0x82725ec0` | `FM2_Render_ReciprocalSinScaleFloat` | Evidence from decompile and caller context. |
| `0x8238efc0` | `FM2_D3D_ReleaseTextureSurfacePairTagged` | Evidence from decompile and caller context. |
| `0x82391c80` | `FM2_D3D_InitTextureDescFromFormat` | Evidence from decompile and caller context. |
| `0x823cd8d0` | `FM2_D3D_GetDeviceCapsThunk` | Evidence from decompile and caller context. |
| `0x823cd8d8` | `FM2_D3D_QueryTextureResourceType` | Evidence from decompile and caller context. |
| `0x823cdbc8` | `FM2_D3D_ComputeMipLevelCount` | Evidence from decompile and caller context. |
| `0x823d3680` | `FM2_D3D_ClearComPtrPair` | Evidence from decompile and caller context. |
| `0x8240b998` | `FM2_Render_ZeroPassLightingCacheLinesVMX` | Evidence from decompile and caller context. |
| `0x824a72b8` | `FM2_Render_HasPassLightingResourceBound` | Evidence from decompile and caller context. |
| `0x824d1178` | `FM2_Input_BuildControllerSslBindingEntry` | Evidence from decompile and caller context. |
| `0x824e31b0` | `FM2_ComObject_AllocSharedStateBuffer` | Evidence from decompile and caller context. |
| `0x826115d8` | `FM2_Vector_ReallocGrow16ByteElements` | Evidence from decompile and caller context. |
| `0x82727dd0` | `FM2_Render_ComputePassLightingSlotOffset64B` | Evidence from decompile and caller context. |
| `0x82728088` | `FM2_Render_GetPassLightingWorkerSlotIndex` | Evidence from decompile and caller context. |
| `0x827280a0` | `FM2_Render_CopyPassLightingPairHead` | Evidence from decompile and caller context. |
| `0x82728280` | `FM2_Render_TestPassLightingSlotIndexValid` | Evidence from decompile and caller context. |
| `0x82728378` | `FM2_Render_GetPassLightingSlotDataPtr` | Evidence from decompile and caller context. |
| `0x827261f0` | `FM2_Render_SinRadiansDouble` | Evidence from decompile and caller context. |
| `0x821d28f8` | `FM2_Input_ControllerSslBindingInitField` | Evidence from decompile and caller context. |
| `0x821f4c50` | `FM2_ComObject_GetRefCountField` | Evidence from decompile and caller context. |
| `0x8221cd00` | `FM2_RaceGhost_ComparePlaybackNodeKey` | Evidence from decompile and caller context. |
| `0x8222f5e8` | `FM2_RaceGhost_LoadPlaybackResourcePath` | Evidence from decompile and caller context. |
| `0x82231848` | `FM2_RaceGhost_ParsePlaybackMetadataBlock` | Evidence from decompile and caller context. |
| `0x82236250` | `FM2_RaceGhost_BuildPlaybackSampleTable` | Evidence from decompile and caller context. |
| `0x82249ae0` | `FM2_LiveryMask_UpdateEntryFlagsField` | Evidence from decompile and caller context. |
| `0x8224a7b8` | `FM2_LiveryMask_ProcessPendingLayerEntry` | Evidence from decompile and caller context. |
| `0x8224b400` | `FM2_LiveryMask_ReleasePendingEntryRef` | Evidence from decompile and caller context. |
| `0x8224b850` | `FM2_LiveryMask_QueuePendingEntryUpdate` | Evidence from decompile and caller context. |
| `0x8224b910` | `FM2_LiveryMask_ApplyPendingEntryTransform` | Evidence from decompile and caller context. |
| `0x8224c0a8` | `FM2_LiveryMask_ClearPendingEntrySlot` | Evidence from decompile and caller context. |