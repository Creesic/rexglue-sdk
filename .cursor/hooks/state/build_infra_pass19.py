import json

RENAMES = [
    ("0x82253888", "FM2_ProfileDb_RbTreeInsertNodeImpl"),
    ("0x8228ef18", "FM2_RaceGhost_MergeSortedKeyframeBuffer"),
    ("0x824a4888", "FM2_D3D_WaitGpuFrameSlotSpinLoop"),
    ("0x82540420", "FM2_RenderAdapter_InitChildListFromRange"),
    ("0x821d9aa8", "FM2_LiveryMask_SetPendingRecordField12"),
    ("0x821d9db0", "FM2_RaceEntry_GetGhostTableEntryAtIndex"),
    ("0x821da240", "FM2_LiveryMask_SetPendingRecordField8"),
    ("0x821e9ae0", "FM2_ContentEntry_CopyCarSetupHead304"),
    ("0x8220a9a8", "FM2_SQLite_FormatQueryVprintfShort"),
    ("0x8221bd30", "FM2_RaceEntry_ShouldProcessGhostForClass"),
    ("0x82224e18", "FM2_RaceEntry_SpawnGhostSceneNodeAtSlot"),
    ("0x82249f80", "FM2_LiveryMask_GenerateUniqueColorKeyString"),
    ("0x8224a290", "FM2_LiveryMask_DtorReleaseColorKeyNode"),
    ("0x8224a718", "FM2_LiveryMask_BinarySearchRecordById"),
    ("0x822531a8", "FM2_Lua_LiveryEditor_ApplyColorFromRegistry"),
    ("0x8225e3f8", "FM2_LuaLobbySort_SortMode0"),
    ("0x8225e610", "FM2_LuaLobbySort_SortMode1"),
    ("0x8225e828", "FM2_LuaLobbySort_SortMode2"),
    ("0x8225ea40", "FM2_LuaLobbySort_SortMode3"),
    ("0x8225ec58", "FM2_LuaLobbySort_SortMode4"),
    ("0x82266888", "FM2_Vector48Record_ReleaseRefFields"),
    ("0x822669a0", "FM2_Vector48Record_MoveConstruct"),
    ("0x82266d10", "FM2_Vector48Iterator_ShiftRecordsBackward"),
    ("0x82266dc0", "FM2_Vector48Iterator_FillFromRecord"),
    ("0x822695d0", "FM2_RaceEntry_UpdateGhostVisibilityFlag"),
    ("0x8226b390", "FM2_RaceGhostWorldState_Ctor"),
    ("0x8226b840", "FM2_CarAudioStreamDefaults_Ctor"),
    ("0x82277c98", "FM2_SceneCamera_CallVfunc12"),
    ("0x8227b618", "FM2_AudioSignalGate_Ctor_F0A4"),
    ("0x8227b780", "FM2_AudioSignalGate_Ctor_F0F0"),
    ("0x825489c8", "FM2_RenderAdapter_CopyChildListFromRange"),
    ("0x8242a258", "FM2_FileStream_Ctor36714"),
    ("0x82460670", "FM2_D3D_InitGpuWaitTimerState"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x8228ef18": "Merges sorted dword keyframe ranges in ghost playback buffer.",
    "0x824a4888": "Spins/waits on GPU frame slot with timeout (D3D resource lock path).",
    "0x8225e3f8": "Lobby sort dispatch for context mode 0 at profile +760.",
    "0x8226b840": "Initializes car-audio stream defaults with wide `Default` name.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass19.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 19 (33 functions)\n", "Profile RB-tree, race ghost/entry, livery mask, lobby sort modes, D3D wait, render adapter.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass19.md", "w", encoding="utf-8").write("\n".join(md))
