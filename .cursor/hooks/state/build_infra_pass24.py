import json

RENAMES = [
    ("0x8236ae10", "FM2_D3D_ComputeResourceBindingFlags"),
    ("0x8236ef20", "FM2_Render_SetClearColorByteAndDirtyFlag"),
    ("0x823700b0", "FM2_Render_SetObjectPassLayerShift24"),
    ("0x82372c20", "FM2_D3D_WaitRingBufferCompletion5"),
    ("0x823732e8", "FM2_D3D_SyncRingBufferAfterGpuError"),
    ("0x82376828", "FM2_D3D_CreateShaderConstantFFixupWr"),
    ("0x823764b0", "FM2_Render_EmitDrawRangeCountPm4"),
    ("0x82365b68", "FM2_Render_EmitDrawRangeFromVector"),
    ("0x8236c8c8", "FM2_GpuKick_SubmitFenceOrInlinePm4"),
    ("0x8235fb58", "FM2_Input_DetectWheelSubtypeFromCapabilities"),
    ("0x82362418", "FM2_Input_InitAxisDefaultsTriple"),
    ("0x82362570", "FM2_Input_LoadControllerRumbleXmlThunk"),
    ("0x82362480", "FM2_Input_LoadControllerRumbleXml"),
    ("0x82332c30", "FM2_RaceGhost_ComputePlaybackWindow"),
    ("0x82332ec8", "FM2_CareerCircuitRaceCoordinator_DtorPartial"),
    ("0x8233b8c8", "FM2_CareerRace_SlideGhostReplayRecordsLeft"),
    ("0x8233bad0", "FM2_CareerRace_FillGhostReplayRecords"),
    ("0x823654a8", "FM2_Memory_DeferredFreePopListHead"),
    ("0x82364078", "FM2_Memory_TryFreeViaPoolHandler"),
    ("0x82363c78", "FM2_Memory_XPhysicalAllocUnderCriticalSection"),
    ("0x8230f3e8", "FM2_Lua_PushPendingStringClosure"),
    ("0x82314f40", "FM2_Lua_PushSaveEnumeratorClosure"),
    ("0x8231c168", "FM2_Lua_PushLuaTuningClassClosure"),
    ("0x822cb240", "FM2_LuaGarage_CopyUICarListUserdata"),
    ("0x822ec0c0", "FM2_Lua_NetworkLobby_ClearSeriesPoints"),
    ("0x822d5f28", "FM2_Lua_Leaderboard_EnumerateByGamertagIndex"),
    ("0x822dea50", "FM2_Lua_Livery_CreateNewLayerAtArgs"),
    ("0x822fa5b0", "FM2_Lua_PhotoMode_ApplyCameraEffectParams"),
    ("0x8235a778", "FM2_Input_ControllerDevice_InitFromTemplate"),
    ("0x8234cff0", "FM2_SceneNodeTree_DetachAndReleaseRefs"),
    ("0x8234d988", "FM2_SceneNode_CopyAssignWithResourceLock"),
    ("0x82370318", "FM2_Render_SubmitObjectDrawConstantsSlot"),
    ("0x823704a0", "FM2_Render_SubmitObjectDrawConstantsSlotAlt"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x82362480": "Loads ControllerRumble.xml wheel/controller defs into input state.",
    "0x82332c30": "Accumulates ghost playback tick window from keyframe helpers.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass24.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 24 (33 functions)\n",
    "GPU PM4 kick paths, input rumble XML, race ghost replay, scene node copy, Lua closures.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass24.md", "w", encoding="utf-8").write("\n".join(md))
