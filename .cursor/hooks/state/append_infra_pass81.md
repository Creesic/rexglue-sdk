### Infrastructure pass 81 (33 functions)

Scene graph/STL, render object-pass/draw setup, FMOD/network, race ghost playback table.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8253d970` | `FM2_SpliceResultObjectsIntoListInitA` | Evidence from decompile and caller context. |
| `0x8253db30` | `FM2_IntrusiveList_InitSentinelBody` | Evidence from decompile and caller context. |
| `0x8253efa0` | `FM2_SpliceResultObjectsIntoListInitB` | Evidence from decompile and caller context. |
| `0x825422b0` | `FM2_LuaGarage_EnsureCarRecordLookupTail` | Evidence from decompile and caller context. |
| `0x82544700` | `FM2_SceneGraph_DestroySubtreeAndFreeBody` | Evidence from decompile and caller context. |
| `0x82545fe0` | `FM2_IntrusiveList_ResetToSelfBody` | Evidence from decompile and caller context. |
| `0x82548ae8` | `FM2_Set_InsertUniqueSortedBody` | Evidence from decompile and caller context. |
| `0x8254a4e0` | `FM2_LuaParser_GetTokenOrAdvanceLineBody` | Evidence from decompile and caller context. |
| `0x82551508` | `FM2_FindAndReplaceDelimitedTextRangeBody` | Evidence from decompile and caller context. |
| `0x82551a00` | `FM2_FindAndReplaceDelimitedTextRangeTail` | Evidence from decompile and caller context. |
| `0x82556080` | `FM2_CarDynamics_ComputeSuspensionDotsVMXBody` | Evidence from decompile and caller context. |
| `0x8255bc00` | `FM2_Render_ObjectPassDrawTraversalBody` | Evidence from decompile and caller context. |
| `0x8255d430` | `FM2_Render_ObjectPassShouldDrawVisibleCheck` | Evidence from decompile and caller context. |
| `0x8255d4b8` | `FM2_Render_ObjectPassShouldDrawVisibleBody` | Evidence from decompile and caller context. |
| `0x82561208` | `FM2_Render_DrawPassMaterialSetupBodyA` | Evidence from decompile and caller context. |
| `0x82561f58` | `FM2_D3D_ValidateResourceHandlesOrRecoverBody` | Evidence from decompile and caller context. |
| `0x82563b68` | `FM2_Render_DrawPassMaterialSetupBodyB` | Evidence from decompile and caller context. |
| `0x825687c8` | `FM2_Render_ObjectPassDrawSetupBody` | Evidence from decompile and caller context. |
| `0x8256ac18` | `FM2_Render_HelperB3E8DrawPathTail` | Evidence from decompile and caller context. |
| `0x8257cbe8` | `FM2_HashName_CtorEmptyBody` | Evidence from decompile and caller context. |
| `0x8257cf90` | `FM2_AIDriver_ResetRaceLineInterpBScalar` | Evidence from decompile and caller context. |
| `0x82586de0` | `FM2_FMOD_Build3DAttributesPairBodyA` | Evidence from decompile and caller context. |
| `0x82587048` | `FM2_FMOD_Build3DAttributesPairBodyB` | Evidence from decompile and caller context. |
| `0x82587b88` | `FM2_Network_DispatchMessageFromQueueLockedBody` | Evidence from decompile and caller context. |
| `0x82589ae0` | `FM2_FileInfoCache_AllocateEntryBody` | Evidence from decompile and caller context. |
| `0x8258b060` | `FM2_RenderAdapter_DestroyChildAndClearListBody` | Evidence from decompile and caller context. |
| `0x8258c008` | `FM2_Presentation_InitMediaFoundationFieldBody` | Evidence from decompile and caller context. |
| `0x8258d0b8` | `FM2_Set_LowerBoundByKeyInTreeBody` | Evidence from decompile and caller context. |
| `0x825977a8` | `FM2_ComObject_InitCarRecordFromDataQueryBody` | Evidence from decompile and caller context. |
| `0x82598048` | `FM2_RaceGhost_BuildPlaybackSampleTableCore` | Evidence from decompile and caller context. |
| `0x8259dc20` | `FM2_RaceGhost_BuildPlaybackSampleTableParse` | Evidence from decompile and caller context. |
| `0x82503668` | `FM2_Memory_LookupFrameAllocNotifyStateHelper` | Evidence from decompile and caller context. |
| `0x824cd5a0` | `FM2_STL_WStringInsertCharsRange_LenThunk` | Evidence from decompile and caller context. |