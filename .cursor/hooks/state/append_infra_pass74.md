### Infrastructure pass 74 (33 functions)

XML attr table, D3D texture-desc/JPEG, career race, livery grow, com-object aggregate, audio render enqueue.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82232090` | `FM2_XmlReader_CopyAttrTableEntryFields` | Evidence from decompile and caller context. |
| `0x823900a8` | `FM2_D3D_TextureDesc_ComputeFormatBlockSizeA` | Evidence from decompile and caller context. |
| `0x82390a70` | `FM2_D3D_TextureDesc_AllocFormatConversionBuffer` | Evidence from decompile and caller context. |
| `0x82390d70` | `FM2_D3D_TextureDesc_ReleaseFormatChain` | Evidence from decompile and caller context. |
| `0x823aaf88` | `FM2_Image_DecodeJpegFromMemory_OutputReadyCallback` | Evidence from decompile and caller context. |
| `0x823cc398` | `FM2_D3D_CreateTextureFromSurfaceLevelBodyB_CopyPixels` | Evidence from decompile and caller context. |
| `0x821d3c20` | `FM2_DeferredAudioManagerUpdate_DtorFields` | Evidence from decompile and caller context. |
| `0x821e7ff8` | `FM2_ComObject_CompareStringFieldPrefix` | Evidence from decompile and caller context. |
| `0x821f0f20` | `FM2_CareerRace_GetPlaybackFrameTimingPtr` | Evidence from decompile and caller context. |
| `0x821f15a8` | `FM2_CareerRace_UpdatePlaybackTimerInner` | Evidence from decompile and caller context. |
| `0x821f23c8` | `FM2_CareerRace_GetEndRaceTimerFieldPtr` | Evidence from decompile and caller context. |
| `0x8221e520` | `FM2_GraphicsStreamList_DeleteQueryDispatch` | Evidence from decompile and caller context. |
| `0x8221f8f8` | `FM2_GraphicsStreamList_DeleteQueryCallbackBody` | Evidence from decompile and caller context. |
| `0x8223d990` | `FM2_RaceGhost_AttachUpgradeNodeFinalize` | Evidence from decompile and caller context. |
| `0x8224a6a0` | `FM2_LiveryMask_GrowPendingEntryCheckBounds` | Evidence from decompile and caller context. |
| `0x8224b530` | `FM2_LiveryMask_GrowPendingEntryAppendSlot` | Evidence from decompile and caller context. |
| `0x8224c100` | `FM2_LiveryMask_GrowPendingEntryShiftTail` | Evidence from decompile and caller context. |
| `0x82255eb0` | `FM2_ComObject_InitRefCountAggregateBindField` | Evidence from decompile and caller context. |
| `0x82262fb8` | `FM2_ComObject_InitRefCountAggregateSetFlag` | Evidence from decompile and caller context. |
| `0x822643b0` | `FM2_ComObject_RefCountAggregateIncrementOne` | Evidence from decompile and caller context. |
| `0x822699b0` | `FM2_ComObject_InitRefCountAggregateFromCarRecord` | Evidence from decompile and caller context. |
| `0x82270228` | `FM2_ComObject_InitRefCountAggregateLinkNode` | Evidence from decompile and caller context. |
| `0x82270a20` | `FM2_ComObject_InitCarRecordFromDataQuery` | Evidence from decompile and caller context. |
| `0x8229a6d8` | `FM2_AudioSample_BuildOutputPairDescriptorValidate` | Evidence from decompile and caller context. |
| `0x8229ae88` | `FM2_AudioSample_BuildOutputPairDescriptorReleaseIter` | Evidence from decompile and caller context. |
| `0x823357d0` | `FM2_Audio_VolumeListInsertNodeRebalance` | Evidence from decompile and caller context. |
| `0x8236c1e8` | `FM2_D3D_GatherVolumeMetadataFromResourceDesc` | Evidence from decompile and caller context. |
| `0x8237cac8` | `FM2_AudioMix_SubmitPendingOutputInitPacket` | Evidence from decompile and caller context. |
| `0x823852f8` | `FM2_D3D_CreateTextureResourceFromFormat` | Evidence from decompile and caller context. |
| `0x82386dc8` | `FM2_AudioRenderFrame_EnqueueD3DCommandInitA` | Evidence from decompile and caller context. |
| `0x82386e58` | `FM2_AudioRenderFrame_EnqueueD3DCommandInitB` | Evidence from decompile and caller context. |
| `0x823892f0` | `FM2_AudioRenderFrame_EnqueueD3DCommandBindSurface` | Evidence from decompile and caller context. |
| `0x8238e490` | `FM2_AudioRenderFrame_EnqueueD3DCommandEmitPackets` | Evidence from decompile and caller context. |