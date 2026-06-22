### Infrastructure pass 53 (33 functions)

Render/audio/Lua/FMOD/RB-tree helpers: presentation slots, Lua assert/math, file cache, boot parse.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82509468` | `FM2_Render_SpliceAndReleaseNotifyList` | Evidence from decompile and caller context. |
| `0x8250cd60` | `FM2_DeferredCommand_ClearStringField` | Evidence from decompile and caller context. |
| `0x82505ec8` | `FM2_Render_ComputeInstancePathBlendMatrix` | Evidence from decompile and caller context. |
| `0x82505d70` | `FM2_AudioVoice_GrowChannelArrayCapacity` | Evidence from decompile and caller context. |
| `0x82512358` | `FM2_PresentationCarConfig_DeleteOptional` | Evidence from decompile and caller context. |
| `0x825127a0` | `FM2_PresentationSlotVector_Clear200Byte` | Evidence from decompile and caller context. |
| `0x8254e318` | `FM2_Lua_AssertionFailedTypeError` | Evidence from decompile and caller context. |
| `0x8254e450` | `FM2_Lua_MathTwoArgDispatch` | Evidence from decompile and caller context. |
| `0x8254d220` | `FM2_Lua_PushMetatableWithGcAndProps` | Evidence from decompile and caller context. |
| `0x82551ab0` | `FM2_FindAndReplaceDelimitedTextRange` | Evidence from decompile and caller context. |
| `0x82555fc8` | `FM2_Animation_ClampKeyframeWeightVMX` | Evidence from decompile and caller context. |
| `0x8255c210` | `FM2_Render_TestObjectPassDrawVisibility` | Evidence from decompile and caller context. |
| `0x8255b1c8` | `FM2_Render_SubmitSortedDrawListsTail` | Evidence from decompile and caller context. |
| `0x8255fb58` | `FM2_Render_ObjectPassDrawTraversalInner` | Evidence from decompile and caller context. |
| `0x82567060` | `FM2_TModel_InitVMXBounds` | Evidence from decompile and caller context. |
| `0x82568590` | `FM2_Render_HelperB3E8PathA` | Evidence from decompile and caller context. |
| `0x82572ca8` | `FM2_WString_GrowHeapCapacity` | Evidence from decompile and caller context. |
| `0x82572dc8` | `FM2_AudioManager_InitSignalGateField` | Evidence from decompile and caller context. |
| `0x82579bb8` | `FM2_Render_SubmitObjectDrawConstantsTail` | Evidence from decompile and caller context. |
| `0x8257ccf8` | `FM2_HashName_CtorEmpty` | Evidence from decompile and caller context. |
| `0x8257cdc0` | `FM2_PropertyBag_AllocRbTreeNode` | Evidence from decompile and caller context. |
| `0x82582188` | `FM2_XmlElement_Dtor` | Evidence from decompile and caller context. |
| `0x82586f40` | `FM2_FMOD_Build3DAttributesPairA` | Evidence from decompile and caller context. |
| `0x82587738` | `FM2_UI_GetMaxPropertyAbsValueHalfStep` | Evidence from decompile and caller context. |
| `0x8258a480` | `FM2_FileInfoCache_AllocateEntry` | Evidence from decompile and caller context. |
| `0x8258b0f8` | `FM2_RenderAdapter_DestroyChildAndClearList` | Evidence from decompile and caller context. |
| `0x8258b3c0` | `FM2_IntrusiveListNode_InitWithOffset` | Evidence from decompile and caller context. |
| `0x8258b4d0` | `FM2_RbTree_InitIteratorWithHint` | Evidence from decompile and caller context. |
| `0x8258c208` | `FM2_Presentation_InitMediaFoundationField` | Evidence from decompile and caller context. |
| `0x8258ebe0` | `FM2_CarDb_QueryStockPartByOrdinal` | Evidence from decompile and caller context. |
| `0x825a11f8` | `FM2_Render_ViewTraversalNotifyHook` | Evidence from decompile and caller context. |
| `0x825a31e0` | `FM2_AudioRenderFrame_LogSaveFrontBufferBody` | Evidence from decompile and caller context. |
| `0x825a3ca8` | `FM2_Boot_ParseCommandLineTokenBuffer` | Evidence from decompile and caller context. |