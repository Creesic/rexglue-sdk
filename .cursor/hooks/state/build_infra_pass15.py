import json

RENAMES = [
    ("0x8220c6f8", "FM2_ProfileLua_UnwindBindingStackSlot"),
    ("0x8240c618", "FM2_Thread_YieldExecution"),
    ("0x8220c758", "FM2_ProfileLua_PushStackMarkerLink"),
    ("0x8220c7c0", "FM2_ProfileLua_InitStackMarker"),
    ("0x8220c890", "FM2_ProfileLua_PushBindingKeyAndPop"),
    ("0x8221bf40", "FM2_WString_GrowHeapCapacity"),
    ("0x8221cf10", "FM2_IntrusiveList_EraseNodeRebalance122"),
    ("0x8221d328", "FM2_IntrusiveList_ClearSentinelLinks122"),
    ("0x822246c0", "FM2_Boot_VsprintfBuffer260"),
    ("0x82225578", "FM2_IntrusiveList_AllocSentinelNode24"),
    ("0x82437508", "FM2_BufferedStream_InitCore"),
    ("0x824b7668", "FM2_Lua_InvokeProtectedCall32"),
    ("0x824f51d8", "FM2_Profile_GetFieldAt40"),
    ("0x82572948", "FM2_AudioManager_GetAltSingleton24"),
    ("0x82509400", "FM2_Presentation_GetManagerSingleton8"),
    ("0x822dc9a8", "FM2_ConfigEntry_ReleaseRefOptionalFree"),
    ("0x827e4ca0", "FM2_AllocPoolAcquire24xCount"),
    ("0x824d3580", "FM2_ComObject_SetUtf8NameWide"),
    ("0x824a51a0", "FM2_ResourceLock_EnterCritSecOrResolve"),
    ("0x824a4f68", "FM2_D3D_WaitGpuFrameSlotWithTimeout"),
    ("0x822272f0", "FM2_DeferredCommand_DtorReleaseRef"),
    ("0x82228478", "FM2_DeferredCommand_CopyAssign"),
    ("0x8222ed38", "FM2_WString_EraseSubrangeInPlace"),
    ("0x8223db98", "FM2_SceneProp_GetWideCharAtIndex"),
    ("0x8224c5d0", "FM2_LiveryMask_CheckListLengthLimit"),
    ("0x82251b10", "FM2_ProfileDb_CompareStringRecordsLess"),
    ("0x82251c48", "FM2_ProfileDb_InitBindingContexts"),
    ("0x822520f0", "FM2_ProfileDb_ReleaseBindingContexts"),
    ("0x82252170", "FM2_ProfileDb_DtorReleaseAll"),
    ("0x822529b0", "FM2_ProfileDb_CopyAssignRecord"),
    ("0x82255488", "FM2_Profile_ClearOptionsChangedFlag"),
    ("0x82256028", "FM2_CareerRace_IsRaceModeType2"),
    ("0x82256040", "FM2_CareerRace_IsRaceModeType6"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x822246c0": "Boot path `vsprintf_s` into 0x104-byte buffer.",
    "0x8240c618": "Thin `NtYieldExecution` wrapper.",
    "0x8221cf10": "RB-tree erase for 122-byte intrusive-list nodes.",
    "0x82255488": "Clears profile options-changed bit at +744.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass15.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 15 (33 functions)\n", "Profile Lua stack markers, wstring, intrusive-list RB-tree, profile DB, career race.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass15.md", "w", encoding="utf-8").write("\n".join(md))
