import json

RENAMES = [
    ("0x822a0710", "FM2_Lua_AppForza_UnpauseMedia"),
    ("0x822a0148", "FM2_Lua_PushSaveVolumeWrapperClosure"),
    ("0x822a01c0", "FM2_Lua_PushNetworkXuidClosure"),
    ("0x822a9db8", "FM2_Lua_PushLiveryHandleClosure"),
    ("0x822b4b18", "FM2_Lua_PushCarControlsClosure"),
    ("0x822aab40", "FM2_Lua_AuctionHouse_PushHighBidderDisplay"),
    ("0x822b5e88", "FM2_Lua_GameLibrary_GetCompressorATM"),
    ("0x822b6a60", "FM2_Lua_GameLibrary_GetSuspensionDamage"),
    ("0x822b70d8", "FM2_Lua_GameLibrary_GetDistUnderTire"),
    ("0x822b88f0", "FM2_Lua_PopCarControlsUserdata"),
    ("0x822bb2d0", "FM2_Lua_PopNumUnitStringUserdata"),
    ("0x822c8ae8", "FM2_Lua_Garage_CalcPerformanceIndex"),
    ("0x822cef20", "FM2_Lua_PopUICarListUserdata"),
    ("0x822d50c0", "FM2_Lua_Leaderboard_PushGotFirstPlaceLocal"),
    ("0x8222e340", "FM2_CarDb_GetGlobalSingleton3390"),
    ("0x82599458", "FM2_CarDb_LookupPerformanceIndexByRatio"),
    ("0x824ae760", "FM2_ComObject_AllocRefCountBlock72"),
    ("0x822dc120", "FM2_Lua_LiveryColor_PushFinishValue"),
    ("0x822da538", "FM2_Lua_LiveryEditor_PushHasOppositeSide"),
    ("0x82301c00", "FM2_Lua_ForzaProfile_PushUsingWheel"),
    ("0x82314ba0", "FM2_Lua_SaveVolume_PushOperationResult"),
    ("0x8231e988", "FM2_Lua_Tuning_GetFrontAccelValue"),
    ("0x823353b0", "FM2_Audio_VolumeListIteratorHasNext"),
    ("0x82335428", "FM2_Audio_VolumeListGetFloatAt40"),
    ("0x82339ea0", "FM2_LiveryRenderManager_InitListHead"),
    ("0x82334df0", "FM2_RaceGhostPlaybackState_Init"),
    ("0x8233ce70", "FM2_GameType_Ctor"),
    ("0x82340c10", "FM2_MultiscreenClientComponent_Ctor"),
    ("0x82331560", "FM2_HashName_RbTreeLowerBoundByKey"),
    ("0x8230abc8", "FM2_SavedReplay_Dtor"),
    ("0x823472f0", "FM2_LuaGarage_EnsureCarRecordField92"),
    ("0x822ddd78", "FM2_Lua_PopLiveryLayerUserdata"),
    ("0x82301cc8", "FM2_ProfileLua_UnwindBindingContext"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x822c8ae8": "Maps Lua ratio to car DB performance index table.",
    "0x82599458": "Piecewise-linear lookup in 10-segment PI curve.",
    "0x823472f0": "Lazy-init car record field at +92 before notify copy.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass22.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 22 (33 functions)\n", "Lua userdata closures/getters, car DB PI lookup, audio volume list, game type ctors.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass22.md", "w", encoding="utf-8").write("\n".join(md))
