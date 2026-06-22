import json

RENAMES = [
    ("0x82767fe0", "FM2_STL_VectorClearAndFreeRange"),
    ("0x82777000", "FM2_DeferredTask_SubmitPreloadingAnimTurnOnTail"),
    ("0x8277d808", "FM2_STL_WStringInsertCharsSlot"),
    ("0x82782748", "FM2_STL_CircularBuffer_PushBackEntry"),
    ("0x825db070", "FM2_LiveryRenderManager_TryFinalizeLayoutSlot0"),
    ("0x825db0b8", "FM2_LiveryRenderManager_TryFinalizeLayoutSlot1"),
    ("0x825db9d0", "FM2_LiveryRenderManager_TryFinalizeLayoutSlot2"),
    ("0x82763488", "FM2_DeferredTask_SubmitWithParamsLookupBody"),
    ("0x824f5550", "FM2_D3D_DeviceContext_DtorFields"),
    ("0x8254d2b8", "FM2_FindAndReplaceDelimitedTextRangeImpl"),
    ("0x824b7448", "FM2_Lua_RegisterBindingPairsModuleTail"),
    ("0x825eacf8", "FM2_SceneSerializer_DisposeAndClearChildren"),
    ("0x82615988", "FM2_SceneNode_InvokeVirtualMethod32Body"),
    ("0x82617a60", "FM2_HashNamePropertyList_EraseNode"),
    ("0x82657fd8", "FM2_CarDynamics_ComputeSuspensionDotsVMX"),
    ("0x825dddb0", "FM2_LiveryRenderManager_CreateLayerBinding"),
    ("0x825edfb8", "FM2_Lua_UISceneManager_GetFadeOutBeginEvent"),
    ("0x8265ea78", "FM2_FMOD_Channel_StopIfPlayingInner"),
    ("0x826631a0", "FM2_FMOD_Channel_StopIfPlayingAlt"),
    ("0x826afa08", "FM2_RenderAdapter_DecRefPresentationSwitch"),
    ("0x826bd550", "FM2_XAudio2_WorkerThread_MainLoopBody"),
    ("0x826ee218", "FM2_SQLite_Database_CloseField"),
    ("0x827526f0", "FM2_DeferredTaskQueue_AllocWorkItemBody"),
    ("0x82535be8", "FM2_DirectIface_SetVertexShaderFromHandle"),
    ("0x82537270", "FM2_DirectIface_SetVertexShaderFromHandleB"),
    ("0x825b4188", "FM2_DirectIface_SetPixelShaderFromHandle"),
    ("0x825d3130", "FM2_SceneGraph_UpdateNodeWithNotifyStateField"),
    ("0x827bd050", "FM2_FontRenderer_LayoutGlyphRunBody"),
    ("0x827bec78", "FM2_FontCache_InitSentinelList"),
    ("0x82781e30", "FM2_XtsClient_ProcessMessageQueueStep"),
    ("0x82778780", "FM2_STL_WStringInsertCharsAlt"),
    ("0x8277c760", "FM2_STL_WStringInsertCharsRange"),
    ("0x827f73b0", "FM2_STL_RbTree_InsertOrFindLeaf"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass57.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 57 (33 functions)\n",
    "Deferred tasks, livery render, FMOD/XAudio2/SQLite, D3D device, font/XTS/STL helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass57.md", "w", encoding="utf-8").write("\n".join(md))
