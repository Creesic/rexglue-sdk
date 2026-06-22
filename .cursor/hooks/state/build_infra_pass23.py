import json

RENAMES = [
    ("0x8228bd18", "FM2_Audio_VolumeListFindByPrefixOrIterator"),
    ("0x82411900", "FM2_Timer_ReadTimeBase64"),
    ("0x8242a2b8", "FM2_ProfileState_GetControllerIdAt12"),
    ("0x824db4a0", "FM2_Audio_VolumeListInitIteratorPair"),
    ("0x82256398", "FM2_LuaLobbySort_CompareUpgradeModifierGt"),
    ("0x82256410", "FM2_LuaLobbySort_CompareStreamBytesReadLt"),
    ("0x82256858", "FM2_LuaLobbySort_CompareDisplayNameLt"),
    ("0x82256918", "FM2_LuaLobbySort_CompareGamertagThenName"),
    ("0x82256a38", "FM2_LuaLobbySort_CompareRewardTierThenClass"),
    ("0x82256390", "FM2_NetworkLobby_GetSeriesPlayerCount"),
    ("0x822643a0", "FM2_LobbyEntry_GetGamertagPrefixByte"),
    ("0x822643f0", "FM2_LobbyEntry_ClearSeriesPointsField28"),
    ("0x822575e8", "FM2_NetworkLobby_GetSeriesEntryAtIndex"),
    ("0x8221b5d0", "FM2_WString_CompareICaseFromComObjects"),
    ("0x82293360", "FM2_TuningRecord_GetFrontAccelPct100"),
    ("0x82277f08", "FM2_SceneCamera_GetStereoscopicModeVtable112"),
    ("0x82297358", "FM2_Profile_ApplyPendingTuningFromHeap"),
    ("0x822ba480", "FM2_Lua_PushNumUnitStringClosure"),
    ("0x822cb590", "FM2_Lua_PushUICarListClosure"),
    ("0x822dd5f0", "FM2_Lua_PushLiveryLayerClosure"),
    ("0x82295fb0", "FM2_TuningRecord_SetDecelIncrementScaled"),
    ("0x82356858", "FM2_Render_RbTreeIteratorDecrement"),
    ("0x823568e0", "FM2_Render_RbTreeLowerBoundBySortKey"),
    ("0x823569e8", "FM2_Render_InitDrawListRangeIterator"),
    ("0x8234b090", "FM2_Replay_GetDefaultWatchStreamPath"),
    ("0x82363628", "FM2_Memory_XPhysicalAllocTracked"),
    ("0x823637c8", "FM2_Memory_AllocViaPoolOrSmallBlock"),
    ("0x82363d18", "FM2_Boot_GetSubsystemTableEntry"),
    ("0x82363fa8", "FM2_Boot_ShutdownSubsystemByIndex"),
    ("0x823643f8", "FM2_AudioDevice_NotifyCategoryIfEnabled"),
    ("0x82369280", "FM2_Render_ScopedBatch_IncGpuKickDepth"),
    ("0x82369418", "FM2_Render_ScopedBatch_DecGpuKickDepthOrFree"),
    ("0x8236a2a0", "FM2_D3D_UnlockResourceFenceRegions"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x82256918": "Lobby sort mode 0: gamertag byte tie-break then display name.",
    "0x82256a38": "Lobby sort mode 4: reward tier then car class at +448.",
    "0x82411900": "Reads PPC time-base register into 64-bit out-param.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass23.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 23 (33 functions)\n",
    "Lobby sort comparators, audio volume list, boot/memory helpers, render scoped batch.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass23.md", "w", encoding="utf-8").write("\n".join(md))
