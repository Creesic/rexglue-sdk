import json

RENAMES = [
    ("0x82494668", "FM2_AIDriver_ResetRaceLineOnSectorChangeInterpA"),
    ("0x824946c8", "FM2_AIDriver_ResetRaceLineOnSectorChangeInterpB"),
    ("0x824947f0", "FM2_AIDriver_ResetRaceLineOnSectorChangeBlend"),
    ("0x824948d0", "FM2_AIDriver_ResetRaceLineOnSectorChangeFinalize"),
    ("0x8249a920", "FM2_CarAudio_DtorBody"),
    ("0x824a6758", "FM2_LiveryMask_ParseAndLoadEntryBody"),
    ("0x824aacb8", "FM2_CompressionStream_EraseRbTreeNodeBody"),
    ("0x824bd360", "FM2_Lua_IncrementCallDepthOrOverflowBody"),
    ("0x824cdca0", "FM2_LiveryMask_ProcessPendingLayerEntryInitA"),
    ("0x824cdde0", "FM2_LiveryMask_ProcessPendingLayerEntryInitB"),
    ("0x824cde98", "FM2_LiveryMask_ProcessPendingLayerEntryInitC"),
    ("0x824d0560", "FM2_Render_ResetPassLightingSlotStateCopy"),
    ("0x824d07b8", "FM2_Input_InitControllerDevicesParseControllerName"),
    ("0x824d2df8", "FM2_Input_InitControllerDevicesParseBindingA"),
    ("0x824d2f50", "FM2_Input_InitControllerDevicesParseBindingB"),
    ("0x824d3670", "FM2_Scene_GetNotifyStateFromParamNormalizeUtf16"),
    ("0x824d56c8", "FM2_Lua_PushSslUnitStringsTableBodyA"),
    ("0x824d59a0", "FM2_Lua_PushSslUnitStringsTableBodyB"),
    ("0x824d5f28", "FM2_Lua_PushDampingFromKeyframeDoubleBody"),
    ("0x824d8030", "FM2_Math_AllocForceVectorComPtrBody"),
    ("0x824daca8", "FM2_Audio_VolumeListFindOrInsertByPrefixWalk"),
    ("0x824dd2d8", "FM2_Profile_ParseUnsignedFromSubStringValidateBody"),
    ("0x824e30c0", "FM2_ComObject_AllocSharedStateBufferInit"),
    ("0x824e9e98", "FM2_RenderAdapter_InitPresentationVtablesClearStateBody"),
    ("0x824ef8a8", "FM2_ExceptionFilter_OnCppExceptionLogBodyA"),
    ("0x824efc50", "FM2_ExceptionFilter_OnCppExceptionLogBodyB"),
    ("0x824f0758", "FM2_Render_NotifyChainInsertSubscriberSortedInit"),
    ("0x824f2630", "FM2_Memory_LookupFrameAllocNotifyStateBody"),
    ("0x823d1d10", "FM2_D3D_CreateTextureFromSurfaceLevelBodyE_InitDesc"),
    ("0x824b1168", "FM2_ComObject_GetAggregateFieldAt60Thunk"),
    ("0x824fe578", "FM2_D3D_LazyInitPresentChainBody"),
    ("0x825025a0", "FM2_D3D_Subscriber_EnableDeviceJournalBody"),
    ("0x82503388", "FM2_Memory_LookupFrameAllocNotifyStateInit"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass77.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 77 (33 functions)\n",
    "AI race line, car audio/livery, Lua SSL/input, profile, exception filter, D3D present chain.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass77.md", "w", encoding="utf-8").write("\n".join(md))
