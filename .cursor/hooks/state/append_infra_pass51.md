### Infrastructure pass 51 (33 functions)

Render frame pipeline / view traversal cluster, PNG/bitstream/image, Lua binding register, FMOD geometry.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x823b1278` | `FM2_Png_EnsureRgbThenDecodeRow` | Evidence from decompile and caller context. |
| `0x823c1190` | `FM2_Bitstream_ReadVariableBits` | Evidence from decompile and caller context. |
| `0x823d6eb0` | `FM2_Image_PixelBuffer_Init` | Evidence from decompile and caller context. |
| `0x82251c98` | `FM2_LiveryMask_MergeProfileRecordFromNode` | Evidence from decompile and caller context. |
| `0x824930a0` | `FM2_AIDriver_ComputeSectorIndexFromProgress` | Evidence from decompile and caller context. |
| `0x8250a820` | `FM2_Render_WaitResourceLockForPassCompile` | Evidence from decompile and caller context. |
| `0x8250ce18` | `FM2_Render_ViewTraversalCullNodes` | Evidence from decompile and caller context. |
| `0x8250cff0` | `FM2_Render_ViewTraversalSortDrawLists` | Evidence from decompile and caller context. |
| `0x8250d238` | `FM2_Render_ViewTraversalEmitDrawCalls` | Evidence from decompile and caller context. |
| `0x8250e400` | `FM2_Presentation_AllocCarInstanceSlot` | Evidence from decompile and caller context. |
| `0x8250f590` | `FM2_Memory_AllocDeferredMapBlockLocked` | Evidence from decompile and caller context. |
| `0x82512038` | `FM2_Presentation_CopyCarDisplayBlockToSlot` | Evidence from decompile and caller context. |
| `0x82513280` | `FM2_Render_BuildObjectPassCommandBuffer` | Evidence from decompile and caller context. |
| `0x82513340` | `FM2_Render_AppendObjectPassDrawEntry` | Evidence from decompile and caller context. |
| `0x825135b8` | `FM2_Vector_EraseBegin20ByteElements` | Evidence from decompile and caller context. |
| `0x825151a0` | `FM2_Render_SubmitObjectDrawConstantsBlock` | Evidence from decompile and caller context. |
| `0x82516e30` | `FM2_Render_UpdatePassVisibilitySortKeysA` | Evidence from decompile and caller context. |
| `0x82516ed8` | `FM2_Render_UpdatePassVisibilitySortKeysB` | Evidence from decompile and caller context. |
| `0x82517778` | `FM2_Render_FramePipelineSubmitPassA` | Evidence from decompile and caller context. |
| `0x82517870` | `FM2_Render_FramePipelineSubmitPassB` | Evidence from decompile and caller context. |
| `0x825179e0` | `FM2_Render_SubmitPassWrapperInner` | Evidence from decompile and caller context. |
| `0x82517b18` | `FM2_Render_FramePipelineDrawObjects` | Evidence from decompile and caller context. |
| `0x82518228` | `FM2_Render_FramePipelineFinalizePass` | Evidence from decompile and caller context. |
| `0x8251a010` | `FM2_Render_SubmitSortedObjectDrawListsInner` | Evidence from decompile and caller context. |
| `0x8251c290` | `FM2_Presentation_ApplyCarCameraVMX` | Evidence from decompile and caller context. |
| `0x82521a98` | `FM2_Render_InstanceHybridDrawPathInner` | Evidence from decompile and caller context. |
| `0x825222b0` | `FM2_Render_ExecuteSortedDrawListsCore` | Evidence from decompile and caller context. |
| `0x8254d6e8` | `FM2_Lua_RegisterBindingPairsInModuleTable` | Evidence from decompile and caller context. |
| `0x8254e278` | `FM2_Lua_AssertionFailedVprintf` | Evidence from decompile and caller context. |
| `0x82553568` | `FM2_Lua_AllocUpvalueClosure` | Evidence from decompile and caller context. |
| `0x82556170` | `FM2_CarDynamics_InitSuspensionFromTireBlock` | Evidence from decompile and caller context. |
| `0x82559680` | `FM2_FMOD_Geometry_AddPolygonFromVMX` | Evidence from decompile and caller context. |
| `0x82559d50` | `FM2_Render_ObjectPassDrawSetupMaterialPass` | Evidence from decompile and caller context. |