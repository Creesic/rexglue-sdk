import json

RENAMES = [
    ("0x8264eb78", "FM2_CarDynamics_InitSubsystemField"),
    ("0x826c2e90", "FM2_XAudio2_Stream_SubmitBufferLockedTail"),
    ("0x8258ec80", "FM2_Lua_PushEngineTypeEnumToStack"),
    ("0x82657fa0", "FM2_CarDynamics_InitTireParamsField"),
    ("0x82678f38", "FM2_FMOD_DspConnectionPool_ProcessInputTail"),
    ("0x826bc1b0", "FM2_D3D_InitVoicePresentationField"),
    ("0x82429f40", "FM2_LiveryMask_MergeProfileRecordTail"),
    ("0x824d3d48", "FM2_Profile_ApplyTuningRecordField"),
    ("0x825c5d38", "FM2_LiveProfile_ReadWriteBufferTail"),
    ("0x8267a158", "FM2_FMOD_Dsp_ProcessReverbMixTail"),
    ("0x82721878", "FM2_Render_PrepareSceneSliceTransforms"),
    ("0x82754c30", "FM2_LiveryMask_SpawnWorkerThread"),
    ("0x82756310", "FM2_XAudio2_Pool_FreeWithCallback"),
    ("0x82763300", "FM2_DeferredTask_SubmitWithParamsLookup"),
    ("0x8277c508", "FM2_STL_RbTree_InsertOrFindNode"),
    ("0x824f53b8", "FM2_RenderState_ApplyFromContextBlock"),
    ("0x825a25b0", "FM2_AudioManager_InitSignalGateField"),
    ("0x8266f620", "FM2_FMOD_Channel_GetVolumeScalar"),
    ("0x82766a10", "FM2_CameraList_FindPrevByCamId"),
    ("0x82767928", "FM2_STL_RbTree_InsertOrFindAlt"),
    ("0x8265ea30", "FM2_FMOD_Channel_StopIfPlayingCheck"),
    ("0x826743b0", "FM2_FMOD_DspConnectionPool_ProcessInputCheck"),
    ("0x826b54c8", "FM2_XAudio2_StreamPool_UnlinkAndNotifyBody"),
    ("0x826bbef0", "FM2_D3D_InitVoicePresentationSubsystemTail"),
    ("0x826efa08", "FM2_SQLite_AppendLowercaseIdentifierMode1"),
    ("0x825562b8", "FM2_CarDynamics_InitTireParamsTail"),
    ("0x82638d40", "FM2_CarSetup_CtorFields"),
    ("0x82673ea0", "FM2_FMOD_Dsp_AdjustDelayLinePointers"),
    ("0x826792d0", "FM2_FMOD_Dsp_ProcessReverbMixBlockTail"),
    ("0x82725d80", "FM2_Render_SetObjectDistanceKeySlot"),
    ("0x82724760", "FM2_Render_BuildPassLightingFromCameraAngles"),
    ("0x827243b0", "FM2_Render_ApplyPassLightingCoeffsVMXWide"),
    ("0x82725f80", "FM2_Render_ComputeSinCosForPassLighting"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass55.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 55 (33 functions)\n",
    "Audio/FMOD/XAudio2, car dynamics, profile/livery, deferred tasks, STL RB-tree, pass lighting helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass55.md", "w", encoding="utf-8").write("\n".join(md))
