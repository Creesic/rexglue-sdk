### Infrastructure pass 61 (33 functions)

Livery mask tail, com-object ref-count, profile/garage, D3D texture upload, pass-lighting subscribers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8224c240` | `FM2_LiveryMask_FinalizePendingEntrySlot` | Evidence from decompile and caller context. |
| `0x8224e230` | `FM2_LiveryMask_ProcessPendingEntryBatch` | Evidence from decompile and caller context. |
| `0x8224ee38` | `FM2_LiveryMask_ResetPendingEntryState` | Evidence from decompile and caller context. |
| `0x822515f0` | `FM2_XmlReader_GrowAttrTableCapacity` | Evidence from decompile and caller context. |
| `0x82252468` | `FM2_LiveryMask_MergePendingEntryLists` | Evidence from decompile and caller context. |
| `0x82254a58` | `FM2_RaceGhost_TraversePlaybackNodeTree` | Evidence from decompile and caller context. |
| `0x822551b8` | `FM2_RaceGhost_GetPlaybackNodeChildCount` | Evidence from decompile and caller context. |
| `0x82255c50` | `FM2_ComObject_InitRefCountSubobjectFields` | Evidence from decompile and caller context. |
| `0x82257148` | `FM2_ComObject_BindRefCountVtableChain` | Evidence from decompile and caller context. |
| `0x82257eb0` | `FM2_ComObject_InitRefCountCallbackFields` | Evidence from decompile and caller context. |
| `0x8225ef80` | `FM2_ComObject_InitRefCountFieldsFromSourceCore` | Evidence from decompile and caller context. |
| `0x822939b0` | `FM2_Profile_SetTuningDisplayNameParseBody` | Evidence from decompile and caller context. |
| `0x822943a0` | `FM2_Profile_SetTuningDisplayNameValidateBody` | Evidence from decompile and caller context. |
| `0x822963d8` | `FM2_Profile_SetTuningDisplayNameCommitBody` | Evidence from decompile and caller context. |
| `0x823428f0` | `FM2_ComObject_InitRefCountAggregateBody` | Evidence from decompile and caller context. |
| `0x82345880` | `FM2_LuaGarage_EnsureCarRecordLookupBody` | Evidence from decompile and caller context. |
| `0x82345960` | `FM2_LuaGarage_EnsureCarRecordFieldCopy` | Evidence from decompile and caller context. |
| `0x823468b0` | `FM2_LuaGarage_EnsureCarRecordFieldInit` | Evidence from decompile and caller context. |
| `0x823b1ca0` | `FM2_Image_LoadPngFromMemory_ReadIdatHeader` | Evidence from decompile and caller context. |
| `0x823b1e10` | `FM2_Image_LoadPngFromMemory_DecompressIdatChunk` | Evidence from decompile and caller context. |
| `0x823b2048` | `FM2_Image_LoadPngFromMemory_ValidateChunkCrc` | Evidence from decompile and caller context. |
| `0x823bfbb0` | `FM2_D3D_ConvertSurfaceFormatToD3d` | Evidence from decompile and caller context. |
| `0x823bfcd8` | `FM2_D3D_CreateTextureFromSurfaceLevelInner` | Evidence from decompile and caller context. |
| `0x823c04d0` | `FM2_D3D_UploadTextureSurfaceLevels` | Evidence from decompile and caller context. |
| `0x823c0ae8` | `FM2_D3D_BuildTextureUploadDescriptor` | Evidence from decompile and caller context. |
| `0x823c0dc8` | `FM2_D3D_ComputeTexturePitchAndSize` | Evidence from decompile and caller context. |
| `0x8240dcc8` | `FM2_Render_GetPassLightingGlobalStatePtr` | Evidence from decompile and caller context. |
| `0x8242a3f0` | `FM2_Render_AppendPassLightingSubscriber` | Evidence from decompile and caller context. |
| `0x82457710` | `FM2_Render_BindPassLightingSubscriberParams` | Evidence from decompile and caller context. |
| `0x82469be8` | `FM2_Render_InsertPassLightingTreeNode` | Evidence from decompile and caller context. |
| `0x824806e8` | `FM2_Render_UpdatePassLightingCoeffSlot` | Evidence from decompile and caller context. |
| `0x82480780` | `FM2_Render_InterpolatePassLightingScalar` | Evidence from decompile and caller context. |
| `0x82484bf0` | `FM2_Render_ResetPassLightingSlotState` | Evidence from decompile and caller context. |