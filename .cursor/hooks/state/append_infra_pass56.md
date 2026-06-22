### Infrastructure pass 56 (33 functions)

Render pass-lighting VMX tail, FMOD/SQLite, race ghost playback, D3D GPU resource read, ForzaTV singleton.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8265e8a0` | `FM2_FMOD_Channel_GetEventParameterValue` | Evidence from decompile and caller context. |
| `0x826ef890` | `FM2_SQLite_ExprSetAffinityAndFlags` | Evidence from decompile and caller context. |
| `0x821d65f0` | `FM2_Render_HelperB3E8ClearStringVector` | Evidence from decompile and caller context. |
| `0x821e9a88` | `FM2_Render_HelperB3E8InitStringFields` | Evidence from decompile and caller context. |
| `0x82228560` | `FM2_Render_NotifyChainInsertSubscriberSorted` | Evidence from decompile and caller context. |
| `0x821d6538` | `FM2_Render_HelperB3E8InitStringRange` | Evidence from decompile and caller context. |
| `0x826ff938` | `FM2_SQLite_ExprAppendLowercaseToken` | Evidence from decompile and caller context. |
| `0x826eedc0` | `FM2_SQLite_ExprGrowTokenBuffer` | Evidence from decompile and caller context. |
| `0x8237ef10` | `FM2_D3D_ReadGpuResourceFloatData` | Evidence from decompile and caller context. |
| `0x8237ed10` | `FM2_D3D_ReleaseGpuResourceRef` | Evidence from decompile and caller context. |
| `0x8253d3d0` | `FM2_Render_FramePipelineResolvePassSlot` | Evidence from decompile and caller context. |
| `0x82237d90` | `FM2_RaceGhost_SelectPlaybackNode` | Evidence from decompile and caller context. |
| `0x82364140` | `FM2_Memory_AllocViaPoolHandler` | Evidence from decompile and caller context. |
| `0x827283f8` | `FM2_RenderTls_SetGlobalPassStatePtrA` | Evidence from decompile and caller context. |
| `0x82728418` | `FM2_RenderTls_SetGlobalPassStatePtrB` | Evidence from decompile and caller context. |
| `0x82724a50` | `FM2_Render_SetPassLightingFromInverseMatrix` | Evidence from decompile and caller context. |
| `0x82724ce0` | `FM2_Render_TransformPointByLightingMatrix` | Evidence from decompile and caller context. |
| `0x82725210` | `FM2_Render_CopyLightingMatrixColumns` | Evidence from decompile and caller context. |
| `0x827258d0` | `FM2_Render_TestPassBoundsVMX` | Evidence from decompile and caller context. |
| `0x8236f180` | `FM2_RenderTls_BindPassStateToContextInner` | Evidence from decompile and caller context. |
| `0x82682168` | `FM2_FMOD_Channel_GetVolumeFromEventTable` | Evidence from decompile and caller context. |
| `0x82684938` | `FM2_FMOD_Event_GetUserDataPtrImpl` | Evidence from decompile and caller context. |
| `0x824f2e48` | `FM2_ForzaTV_InitSubscriberVtables` | Evidence from decompile and caller context. |
| `0x824f2ef0` | `FM2_ForzaTV_EnsureSingletonInit` | Evidence from decompile and caller context. |
| `0x827253c8` | `FM2_Render_BuildPassLightingMatrixFromAngles` | Evidence from decompile and caller context. |
| `0x827254c0` | `FM2_Render_SetPassLightingScaleMatrix` | Evidence from decompile and caller context. |
| `0x82725698` | `FM2_Render_MultiplyPassMatrixVMXVariant` | Evidence from decompile and caller context. |
| `0x827259e0` | `FM2_Render_SetPassLightingModeScalar` | Evidence from decompile and caller context. |
| `0x82725b70` | `FM2_Render_ComparePassMatrixBytesVMX` | Evidence from decompile and caller context. |
| `0x82724d68` | `FM2_Render_TransformDirByLightingMatrix` | Evidence from decompile and caller context. |
| `0x82724e40` | `FM2_Render_ProjectPassBoundsToScreen` | Evidence from decompile and caller context. |
| `0x82725160` | `FM2_Render_SetPassLightingDiagonalMatrix` | Evidence from decompile and caller context. |
| `0x827261f8` | `FM2_Render_ApplyPassLightingMatrixToState` | Evidence from decompile and caller context. |