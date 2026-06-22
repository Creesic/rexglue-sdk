### Infrastructure pass 54 (33 functions)

Render pass lighting VMX cluster, frame pipeline pass cleanup, sort/visibility helpers, PNG/math.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x827c5e38` | `FM2_Render_SetPassTimingScalar` | Evidence from decompile and caller context. |
| `0x82725780` | `FM2_Render_MultiplyMatrix4x4VMX` | Evidence from decompile and caller context. |
| `0x821d7608` | `FM2_Render_ViewTraversalNormalizeBasisVMX` | Evidence from decompile and caller context. |
| `0x82724218` | `FM2_Render_SetPassLightingCoeffs` | Evidence from decompile and caller context. |
| `0x82724888` | `FM2_Render_ApplyPassLightingStateInner` | Evidence from decompile and caller context. |
| `0x82724958` | `FM2_Render_CopyPassViewMatrix4x4` | Evidence from decompile and caller context. |
| `0x82724988` | `FM2_Render_CopyPassProjMatrix4x4` | Evidence from decompile and caller context. |
| `0x827249a0` | `FM2_Render_SetPassLightingFromMatrix` | Evidence from decompile and caller context. |
| `0x827250f8` | `FM2_Render_TransformVectorByMatrix4x4` | Evidence from decompile and caller context. |
| `0x827255b0` | `FM2_Render_MultiplyMatrix4x4AccumVMX` | Evidence from decompile and caller context. |
| `0x827306c0` | `FM2_ConstantBuffer_UploadVector4Block` | Evidence from decompile and caller context. |
| `0x82725310` | `FM2_Render_BuildPassLightingMatrixVMX` | Evidence from decompile and caller context. |
| `0x82724568` | `FM2_Render_UpdatePassSortKeysFromBounds` | Evidence from decompile and caller context. |
| `0x82723d18` | `FM2_RenderContext_UploadMatrixConstantsFromPass` | Evidence from decompile and caller context. |
| `0x82723860` | `FM2_RenderTls_BatchSubmitDrawPacketsTail` | Evidence from decompile and caller context. |
| `0x825377e8` | `FM2_Render_InstancePathWrapperTraverse` | Evidence from decompile and caller context. |
| `0x8253cfd0` | `FM2_Render_FramePipelineNotifyPassState` | Evidence from decompile and caller context. |
| `0x8253d440` | `FM2_Render_FramePipelineCleanupPassSlots` | Evidence from decompile and caller context. |
| `0x8252b8d8` | `FM2_Render_SortVisibleRenderablesIntrosort` | Evidence from decompile and caller context. |
| `0x821e1d60` | `FM2_AudioRenderFrame_FlushLogBufferChunk` | Evidence from decompile and caller context. |
| `0x821efec0` | `FM2_Render_HelperB3E8ResetState` | Evidence from decompile and caller context. |
| `0x82515d58` | `FM2_Render_TestObjectPassOcclusionWrapped` | Evidence from decompile and caller context. |
| `0x82659258` | `FM2_Memory_AllocFromAllocatorContext` | Evidence from decompile and caller context. |
| `0x824df418` | `FM2_RenderAdapter_ResetPresentationStateBlock` | Evidence from decompile and caller context. |
| `0x825c5f48` | `FM2_ProfileDb_InitPropertyBagCritSec` | Evidence from decompile and caller context. |
| `0x82557428` | `FM2_BufferedFileRead_RandUnitFloat` | Evidence from decompile and caller context. |
| `0x8258b370` | `FM2_RbTree_FindLowerBoundNodeByKey` | Evidence from decompile and caller context. |
| `0x82461428` | `FM2_Crt_CreateSemaphoreA` | Evidence from decompile and caller context. |
| `0x8239f2b8` | `FM2_Png_AllocDecodeStateBuffer` | Evidence from decompile and caller context. |
| `0x82724078` | `FM2_RenderTls_BindPassStateToContext` | Evidence from decompile and caller context. |
| `0x82724160` | `FM2_Render_InitPassLightingStateBlock` | Evidence from decompile and caller context. |
| `0x82724270` | `FM2_Render_ApplyPassLightingCoeffsVMX` | Evidence from decompile and caller context. |
| `0x827260e8` | `FM2_Math_FastInvSqrtTaylor` | Evidence from decompile and caller context. |