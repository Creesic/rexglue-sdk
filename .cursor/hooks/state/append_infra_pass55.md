### Infrastructure pass 55 (33 functions)

Audio/FMOD/XAudio2, car dynamics, profile/livery, deferred tasks, STL RB-tree, pass lighting helpers.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x8264eb78` | `FM2_CarDynamics_InitSubsystemField` | Evidence from decompile and caller context. |
| `0x826c2e90` | `FM2_XAudio2_Stream_SubmitBufferLockedTail` | Evidence from decompile and caller context. |
| `0x8258ec80` | `FM2_Lua_PushEngineTypeEnumToStack` | Evidence from decompile and caller context. |
| `0x82657fa0` | `FM2_CarDynamics_InitTireParamsField` | Evidence from decompile and caller context. |
| `0x82678f38` | `FM2_FMOD_DspConnectionPool_ProcessInputTail` | Evidence from decompile and caller context. |
| `0x826bc1b0` | `FM2_D3D_InitVoicePresentationField` | Evidence from decompile and caller context. |
| `0x82429f40` | `FM2_LiveryMask_MergeProfileRecordTail` | Evidence from decompile and caller context. |
| `0x824d3d48` | `FM2_Profile_ApplyTuningRecordField` | Evidence from decompile and caller context. |
| `0x825c5d38` | `FM2_LiveProfile_ReadWriteBufferTail` | Evidence from decompile and caller context. |
| `0x8267a158` | `FM2_FMOD_Dsp_ProcessReverbMixTail` | Evidence from decompile and caller context. |
| `0x82721878` | `FM2_Render_PrepareSceneSliceTransforms` | Evidence from decompile and caller context. |
| `0x82754c30` | `FM2_LiveryMask_SpawnWorkerThread` | Evidence from decompile and caller context. |
| `0x82756310` | `FM2_XAudio2_Pool_FreeWithCallback` | Evidence from decompile and caller context. |
| `0x82763300` | `FM2_DeferredTask_SubmitWithParamsLookup` | Evidence from decompile and caller context. |
| `0x8277c508` | `FM2_STL_RbTree_InsertOrFindNode` | Evidence from decompile and caller context. |
| `0x824f53b8` | `FM2_RenderState_ApplyFromContextBlock` | Evidence from decompile and caller context. |
| `0x825a25b0` | `FM2_AudioManager_InitSignalGateField` | Evidence from decompile and caller context. |
| `0x8266f620` | `FM2_FMOD_Channel_GetVolumeScalar` | Evidence from decompile and caller context. |
| `0x82766a10` | `FM2_CameraList_FindPrevByCamId` | Evidence from decompile and caller context. |
| `0x82767928` | `FM2_STL_RbTree_InsertOrFindAlt` | Evidence from decompile and caller context. |
| `0x8265ea30` | `FM2_FMOD_Channel_StopIfPlayingCheck` | Evidence from decompile and caller context. |
| `0x826743b0` | `FM2_FMOD_DspConnectionPool_ProcessInputCheck` | Evidence from decompile and caller context. |
| `0x826b54c8` | `FM2_XAudio2_StreamPool_UnlinkAndNotifyBody` | Evidence from decompile and caller context. |
| `0x826bbef0` | `FM2_D3D_InitVoicePresentationSubsystemTail` | Evidence from decompile and caller context. |
| `0x826efa08` | `FM2_SQLite_AppendLowercaseIdentifierMode1` | Evidence from decompile and caller context. |
| `0x825562b8` | `FM2_CarDynamics_InitTireParamsTail` | Evidence from decompile and caller context. |
| `0x82638d40` | `FM2_CarSetup_CtorFields` | Evidence from decompile and caller context. |
| `0x82673ea0` | `FM2_FMOD_Dsp_AdjustDelayLinePointers` | Evidence from decompile and caller context. |
| `0x826792d0` | `FM2_FMOD_Dsp_ProcessReverbMixBlockTail` | Evidence from decompile and caller context. |
| `0x82725d80` | `FM2_Render_SetObjectDistanceKeySlot` | Evidence from decompile and caller context. |
| `0x82724760` | `FM2_Render_BuildPassLightingFromCameraAngles` | Evidence from decompile and caller context. |
| `0x827243b0` | `FM2_Render_ApplyPassLightingCoeffsVMXWide` | Evidence from decompile and caller context. |
| `0x82725f80` | `FM2_Render_ComputeSinCosForPassLighting` | Evidence from decompile and caller context. |