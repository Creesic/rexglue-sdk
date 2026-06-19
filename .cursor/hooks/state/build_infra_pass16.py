import json

RENAMES = [
    ("0x8242cc38", "FM2_RedirectStream_CtorFromSource"),
    ("0x824a4d20", "FM2_ResourceLock_ResolveFrameAllocatorState"),
    ("0x824bca78", "FM2_Lua_ProtectedCallDispatchLoop"),
    ("0x824d32d8", "FM2_WideString_ReserveCapacityChars"),
    ("0x82610330", "FM2_IntrusiveList_IncrementIteratorPastErase122"),
    ("0x8220c688", "FM2_ProfileLua_InitBindingNilMarker"),
    ("0x8221cc88", "FM2_IntrusiveList_DestroySubtreeNodes122"),
    ("0x82226958", "FM2_RaceEntry_OnFinishedNotifyGhostReplay"),
    ("0x82230368", "FM2_CarDb_QueryBaseCostByCarId"),
    ("0x82231740", "FM2_WString_AssignFromWideCStrChecked"),
    ("0x82238928", "FM2_RaceGhost_SelectOrRebuildPlaybackNode"),
    ("0x8223d910", "FM2_Render_PackDrawMatrixVMX128"),
    ("0x82241830", "FM2_Lua_LiveryEditor_RevertLayerApplyUpgrade"),
    ("0x82242458", "FM2_CarParts_ApplyUpgradeSlotGroupImpl"),
    ("0x82242b88", "FM2_Lua_LiveryEditor_GetCurrentLayerRecord"),
    ("0x8224c428", "FM2_LiveryMask_UpdatePendingEntryAt16"),
    ("0x8224c4d8", "FM2_LiveryMask_UpdatePendingEntryAt28"),
    ("0x8224cdf8", "FM2_LiveryMask_SetActiveCarMediaPath"),
    ("0x8224d168", "FM2_LiveryMask_BuildPendingUpdateRecord96"),
    ("0x8224d6a0", "FM2_LiveryMask_AllocPendingUpdateNode"),
    ("0x8224e878", "FM2_LiveryMask_EraseListNodeAndIter"),
    ("0x822521c0", "FM2_ProfileDb_InitRbTreeIterator"),
    ("0x82252718", "FM2_LiveryMask_FindProfileRecordByKey"),
    ("0x82252928", "FM2_LiveryMask_InsertOrUpdateProfileRecord"),
    ("0x82252bf8", "FM2_CarParts_LookupUpgradePathByName"),
    ("0x82254c80", "FM2_Lua_LiveryEditor_SetColorKeyValue"),
    ("0x8225ae70", "FM2_RaceGhost_MergeSortedKeyframeRanges"),
    ("0x8225e330", "FM2_CareerRace_CopyRewardsBlockAt760"),
    ("0x8225ee70", "FM2_LuaLobbySort_RunSortByContextMode"),
    ("0x822624f8", "FM2_RenderAdapter_DestroyChildAndClearList"),
    ("0x82264450", "FM2_GraphicsStream_IsLinkedListEmpty"),
    ("0x8226a8a0", "FM2_SceneGraph_CopyContentEntryDwordVector"),
    ("0x8226b1e0", "FM2_CareerRace_CopyGhostReplayRecord"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x82230368": "SQLite `SELECT BaseCost FROM Data_Car WHERE Id=%u`.",
    "0x8224cdf8": "Builds `GAME:\\Media\\cars\\%s` path for active livery car.",
    "0x82264450": "Returns true when graphics-stream linked list at +12 is empty.",
    "0x8226b1e0": "Deep copy of ghost replay record with VMX128 matrix blocks.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass16.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 16 (33 functions)\n", "IO streams, Lua protected call, livery mask, race ghost, career race, render adapter.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass16.md", "w", encoding="utf-8").write("\n".join(md))
