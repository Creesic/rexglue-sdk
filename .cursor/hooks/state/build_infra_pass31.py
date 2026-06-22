import json

RENAMES = [
    ("0x8228e0e0", "FM2_RaceGhost_SiftUpKeyframeHeap"),
    ("0x8228e1a0", "FM2_RaceGhost_IntroSortPartitionGap"),
    ("0x822a17f0", "FM2_Lua_PopSaveVolumeWrapperUserdata"),
    ("0x822a1848", "FM2_Lua_PopNetworkXuidUserdata"),
    ("0x822a8fe8", "FM2_Lua_AuctionHouse_GetCarSlotFromArgs"),
    ("0x822aa0a8", "FM2_Lua_PopLiveryHandleUserdata"),
    ("0x822db290", "FM2_Lua_LiveryEditor_LoadDecalsFromArg"),
    ("0x822db340", "FM2_Lua_LiveryEditor_SetDecalTabFromArg"),
    ("0x822eb190", "FM2_Lua_PlayerChoices_SetShiftingFromArg"),
    ("0x822f9058", "FM2_Lua_PhotoModeCamera_PushFocusToStack"),
    ("0x822fb0a8", "FM2_Lua_PhotoModeCamera_GetDepthAtScreenCoord"),
    ("0x82301700", "FM2_Lua_GetGarageUserdataOrRaise"),
    ("0x82304e28", "FM2_Lua_ForzaProfile_GetThrottleDeadzoneWheel"),
    ("0x8230f0d8", "FM2_ReplayPendingString_CopyConstruct"),
    ("0x8230f200", "FM2_ReplayPendingString_Dtor"),
    ("0x8230f7c0", "FM2_Replay_CreatePendingStringTask"),
    ("0x8230f878", "FM2_Lua_PopPendingStringUserdata"),
    ("0x8230fec0", "FM2_Lua_RewardReveal_GetCarLevelInfoFromArgs"),
    ("0x82314fc0", "FM2_Lua_PopSaveEnumeratorUserdata"),
    ("0x8231edc8", "FM2_Lua_TuningSetRearDecelFromArg"),
    ("0x8231f658", "FM2_Lua_PopLuaTuningClassUserdata"),
    ("0x82329548", "FM2_Lua_BuildCarSetupPointerUserdata"),
    ("0x8232d758", "FM2_Career_PendingBoolSslWrapper_Ctor"),
    ("0x82341e60", "FM2_RaceGhost_MergePlaybackKeyframeSlice"),
    ("0x82363e58", "FM2_AudioDevice_AllocPhysicalCategoryBuffer"),
    ("0x82371ea8", "FM2_D3D_WritePrimaryRingBufferWords"),
    ("0x82372aa8", "FM2_GpuKick_AppendSchedulerPm4Packets"),
    ("0x82372c00", "FM2_D3D_WaitForGpuCommandCompletion"),
    ("0x823744c8", "FM2_GpuCommandBuffer_SetDisplayModeAndQuery"),
    ("0x82378070", "FM2_D3D_AllocGpuCaptureForPix"),
    ("0x8237b090", "FM2_Render_ReleasePixCaptureSurfaces"),
    ("0x8237b8c0", "FM2_Render_AllocateEdramScratchSlice"),
    ("0x8239f358", "FM2_Png_ZeroPaletteBlock64"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x82372aa8": "Appends PM4 scheduler packets (66940/1400 opcodes) to kick buffer.",
    "0x82378070": "Allocates D3D GPU capture object after PIXBeginCapture succeeds.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass31.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 31 (33 functions)\n",
    "Race ghost sort, Lua userdata pop closures, replay pending strings, GPU kick/PIX, audio alloc.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass31.md", "w", encoding="utf-8").write("\n".join(md))
