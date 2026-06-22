### Infrastructure pass 57 (33 functions)

Deferred tasks, livery render, FMOD/XAudio2/SQLite, D3D device, font/XTS/STL helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82767fe0` | `FM2_STL_VectorClearAndFreeRange` | Evidence from decompile and caller context. |
| `0x82777000` | `FM2_DeferredTask_SubmitPreloadingAnimTurnOnTail` | Evidence from decompile and caller context. |
| `0x8277d808` | `FM2_STL_WStringInsertCharsSlot` | Evidence from decompile and caller context. |
| `0x82782748` | `FM2_STL_CircularBuffer_PushBackEntry` | Evidence from decompile and caller context. |
| `0x825db070` | `FM2_LiveryRenderManager_TryFinalizeLayoutSlot0` | Evidence from decompile and caller context. |
| `0x825db0b8` | `FM2_LiveryRenderManager_TryFinalizeLayoutSlot1` | Evidence from decompile and caller context. |
| `0x825db9d0` | `FM2_LiveryRenderManager_TryFinalizeLayoutSlot2` | Evidence from decompile and caller context. |
| `0x82763488` | `FM2_DeferredTask_SubmitWithParamsLookupBody` | Evidence from decompile and caller context. |
| `0x824f5550` | `FM2_D3D_DeviceContext_DtorFields` | Evidence from decompile and caller context. |
| `0x8254d2b8` | `FM2_FindAndReplaceDelimitedTextRangeImpl` | Evidence from decompile and caller context. |
| `0x824b7448` | `FM2_Lua_RegisterBindingPairsModuleTail` | Evidence from decompile and caller context. |
| `0x825eacf8` | `FM2_SceneSerializer_DisposeAndClearChildren` | Evidence from decompile and caller context. |
| `0x82615988` | `FM2_SceneNode_InvokeVirtualMethod32Body` | Evidence from decompile and caller context. |
| `0x82617a60` | `FM2_HashNamePropertyList_EraseNode` | Evidence from decompile and caller context. |
| `0x82657fd8` | `FM2_CarDynamics_ComputeSuspensionDotsVMX` | Evidence from decompile and caller context. |
| `0x825dddb0` | `FM2_LiveryRenderManager_CreateLayerBinding` | Evidence from decompile and caller context. |
| `0x825edfb8` | `FM2_Lua_UISceneManager_GetFadeOutBeginEvent` | Evidence from decompile and caller context. |
| `0x8265ea78` | `FM2_FMOD_Channel_StopIfPlayingInner` | Evidence from decompile and caller context. |
| `0x826631a0` | `FM2_FMOD_Channel_StopIfPlayingAlt` | Evidence from decompile and caller context. |
| `0x826afa08` | `FM2_RenderAdapter_DecRefPresentationSwitch` | Evidence from decompile and caller context. |
| `0x826bd550` | `FM2_XAudio2_WorkerThread_MainLoopBody` | Evidence from decompile and caller context. |
| `0x826ee218` | `FM2_SQLite_Database_CloseField` | Evidence from decompile and caller context. |
| `0x827526f0` | `FM2_DeferredTaskQueue_AllocWorkItemBody` | Evidence from decompile and caller context. |
| `0x82535be8` | `FM2_DirectIface_SetVertexShaderFromHandle` | Evidence from decompile and caller context. |
| `0x82537270` | `FM2_DirectIface_SetVertexShaderFromHandleB` | Evidence from decompile and caller context. |
| `0x825b4188` | `FM2_DirectIface_SetPixelShaderFromHandle` | Evidence from decompile and caller context. |
| `0x825d3130` | `FM2_SceneGraph_UpdateNodeWithNotifyStateField` | Evidence from decompile and caller context. |
| `0x827bd050` | `FM2_FontRenderer_LayoutGlyphRunBody` | Evidence from decompile and caller context. |
| `0x827bec78` | `FM2_FontCache_InitSentinelList` | Evidence from decompile and caller context. |
| `0x82781e30` | `FM2_XtsClient_ProcessMessageQueueStep` | Evidence from decompile and caller context. |
| `0x82778780` | `FM2_STL_WStringInsertCharsAlt` | Evidence from decompile and caller context. |
| `0x8277c760` | `FM2_STL_WStringInsertCharsRange` | Evidence from decompile and caller context. |
| `0x827f73b0` | `FM2_STL_RbTree_InsertOrFindLeaf` | Evidence from decompile and caller context. |