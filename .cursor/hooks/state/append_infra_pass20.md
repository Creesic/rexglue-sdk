### Infrastructure pass 20 (33 functions)

Race entry ghost path, audio resource hooks, render pass resource, profile tuning, scene graph.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82224ce8` | `FM2_RaceEntry_NotifyGhostRenderState` | Evidence from decompile and caller context. |
| `0x821f10d0` | `FM2_SceneNode_ClearDrawFlag400` | Evidence from decompile and caller context. |
| `0x822691e8` | `FM2_RaceEntry_PostGhostVisibilityChange` | Evidence from decompile and caller context. |
| `0x82256058` | `FM2_CareerRace_IsArcadeRaceMode` | True when profile race mode at +80 is 3–6 (arcade/time trial family). |
| `0x821d9e30` | `FM2_RaceEntry_IsGhostPlaybackComplete` | Evidence from decompile and caller context. |
| `0x821d9c68` | `FM2_RaceEntry_CreateGhostSceneNode` | Evidence from decompile and caller context. |
| `0x8227dfb0` | `FM2_AudioResource_RegisterHook_EDE8` | Evidence from decompile and caller context. |
| `0x8227d750` | `FM2_AudioResource_RegisterHook_EC14` | Evidence from decompile and caller context. |
| `0x8227d7b8` | `FM2_AudioResource_RegisterHook_EC34` | Evidence from decompile and caller context. |
| `0x8227c250` | `FM2_AudioRenderFrame_WriteFrontBufferMix` | Evidence from decompile and caller context. |
| `0x8227c4a8` | `FM2_AudioRenderFrame_LogSaveFrontBuffer` | Debug path logging `SAVE FRONT BUFFER` during audio render. |
| `0x8227e018` | `FM2_AudioResource_RegisterHook_EE08` | Evidence from decompile and caller context. |
| `0x8227e080` | `FM2_AudioResource_RegisterHook_EE28` | Evidence from decompile and caller context. |
| `0x8227e0e8` | `FM2_AudioResource_RegisterHook_EE48` | Evidence from decompile and caller context. |
| `0x8227e470` | `FM2_AudioFrameService_QueryDeviceCaps` | Evidence from decompile and caller context. |
| `0x8227ed30` | `FM2_AudioSignalGate_Ctor_F3E4` | Evidence from decompile and caller context. |
| `0x8227ee98` | `FM2_AudioSignalGate_CtorFromCopy_F4C4` | Evidence from decompile and caller context. |
| `0x8227f008` | `FM2_AudioResource_RegisterHook_F34C` | Evidence from decompile and caller context. |
| `0x8227f5c0` | `FM2_DeferredTask_NotifyStateChangeA` | Evidence from decompile and caller context. |
| `0x8227fb60` | `FM2_DeferredTask_NotifyStateChangeB` | Evidence from decompile and caller context. |
| `0x822802e8` | `FM2_RenderPassResource_Dtor` | Evidence from decompile and caller context. |
| `0x82282b90` | `FM2_RenderPassResource_CtorWithLock` | Evidence from decompile and caller context. |
| `0x82284d08` | `FM2_WString_AssignFromWideStringView` | Evidence from decompile and caller context. |
| `0x82284d60` | `FM2_FxlResourceType_StaticInit24` | Static init 24-byte CFXLResourceType singleton for audio resources. |
| `0x8228b2f0` | `FM2_SpliceResultList_CheckLengthLimit` | Evidence from decompile and caller context. |
| `0x8228bc88` | `FM2_FileInfoCache_GetTransferNotifyVtable` | Evidence from decompile and caller context. |
| `0x8228d6d0` | `FM2_RaceGhost_CopyPlaybackState200` | Evidence from decompile and caller context. |
| `0x8228f4e0` | `FM2_AudioManager_SetFrameCounterField80308` | Evidence from decompile and caller context. |
| `0x822905a0` | `FM2_SceneGraph_ClearChildSlotByType` | Evidence from decompile and caller context. |
| `0x82292018` | `FM2_IntrusiveList_ShiftNodes248Byte` | Evidence from decompile and caller context. |
| `0x82293f58` | `FM2_Lua_InterpolateFloatField432To436` | Evidence from decompile and caller context. |
| `0x82296f00` | `FM2_Profile_DtorReleaseRefs` | Evidence from decompile and caller context. |
| `0x822979b8` | `FM2_Profile_SetTuningDisplayName` | Evidence from decompile and caller context. |