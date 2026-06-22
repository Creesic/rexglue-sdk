### Infrastructure pass 58 (33 functions)

Pass-lighting VMX/math tail, race ghost playback, car-parts bounds, livery/XML/com/audio helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8223e0e0` | `FM2_Render_BuildPassLightingBasisVMX` | Evidence from decompile and caller context. |
| `0x827f6100` | `FM2_STL_AllocViaComGpuAllocator` | Evidence from decompile and caller context. |
| `0x821e8280` | `FM2_RaceGhost_InitPlaybackContext` | Evidence from decompile and caller context. |
| `0x82232180` | `FM2_RaceGhost_RebuildPlaybackFromSamples` | Evidence from decompile and caller context. |
| `0x822361a0` | `FM2_RaceGhost_SplicePlaybackNodeList` | Evidence from decompile and caller context. |
| `0x822356b0` | `FM2_RaceGhost_FindPlaybackNodeByKey` | Evidence from decompile and caller context. |
| `0x82236fb0` | `FM2_RaceGhost_LoadPlaybackSslBindingChain` | Evidence from decompile and caller context. |
| `0x8223f6a0` | `FM2_CarParts_ApplyUpgradeSlotBoundsTransform` | Evidence from decompile and caller context. |
| `0x821d76f0` | `FM2_Render_ComputePassLightingBasisVectors` | Evidence from decompile and caller context. |
| `0x827261c8` | `FM2_Render_SinDegreesFloat` | Evidence from decompile and caller context. |
| `0x82726350` | `FM2_Render_ClampAbsFloat220M` | Evidence from decompile and caller context. |
| `0x82726548` | `FM2_Render_TruncateDoubleToFloat` | Evidence from decompile and caller context. |
| `0x82726440` | `FM2_Render_ClampAbsFloat220MAlt` | Evidence from decompile and caller context. |
| `0x827265b0` | `FM2_Render_TruncateDoubleToFloatAlt` | Evidence from decompile and caller context. |
| `0x82726618` | `FM2_Render_InitGlobalLightingTlsState` | Evidence from decompile and caller context. |
| `0x82726da0` | `FM2_Render_EnsureGlobalLightingTlsInit` | Evidence from decompile and caller context. |
| `0x82726c78` | `FM2_Render_AdvancePassLightingCycleIndex` | Evidence from decompile and caller context. |
| `0x82726e18` | `FM2_Render_AdvancePassLightingCycleIndexTls` | Evidence from decompile and caller context. |
| `0x82726ec0` | `FM2_Render_DotProduct4WithBias` | Evidence from decompile and caller context. |
| `0x82726f60` | `FM2_Render_LerpVec4` | Evidence from decompile and caller context. |
| `0x82727080` | `FM2_Render_ComputeVec4LengthSq` | Evidence from decompile and caller context. |
| `0x82727158` | `FM2_Render_ComputePassLightingSlotStride48` | Evidence from decompile and caller context. |
| `0x82727180` | `FM2_Render_AllocPassLightingSlotArray` | Evidence from decompile and caller context. |
| `0x82727200` | `FM2_Render_ClearPassLightingSlotVMX` | Evidence from decompile and caller context. |
| `0x827272b0` | `FM2_Render_ComputePassLightingSlotOffset` | Evidence from decompile and caller context. |
| `0x827272c8` | `FM2_Render_BindPassLightingResourcePair` | Evidence from decompile and caller context. |
| `0x82727390` | `FM2_Render_UpdatePassLightingSlotFields` | Evidence from decompile and caller context. |
| `0x82727410` | `FM2_Render_ProcessPassLightingBatchA` | Evidence from decompile and caller context. |
| `0x827277f0` | `FM2_Render_ProcessPassLightingBatchB` | Evidence from decompile and caller context. |
| `0x8224eef8` | `FM2_LiveryMask_ProcessPendingEntryUpdatesBody` | Evidence from decompile and caller context. |
| `0x822516d0` | `FM2_XmlReader_InsertAttrEntrySorted` | Evidence from decompile and caller context. |
| `0x82260188` | `FM2_ComObject_InitRefCountFieldsBody` | Evidence from decompile and caller context. |
| `0x82284608` | `FM2_AudioRenderFrame_PathBInner` | Evidence from decompile and caller context. |