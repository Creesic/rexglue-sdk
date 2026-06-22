import json

RENAMES = [
    ("0x82224ce8", "FM2_RaceEntry_NotifyGhostRenderState"),
    ("0x821f10d0", "FM2_SceneNode_ClearDrawFlag400"),
    ("0x822691e8", "FM2_RaceEntry_PostGhostVisibilityChange"),
    ("0x82256058", "FM2_CareerRace_IsArcadeRaceMode"),
    ("0x821d9e30", "FM2_RaceEntry_IsGhostPlaybackComplete"),
    ("0x821d9c68", "FM2_RaceEntry_CreateGhostSceneNode"),
    ("0x8227dfb0", "FM2_AudioResource_RegisterHook_EDE8"),
    ("0x8227d750", "FM2_AudioResource_RegisterHook_EC14"),
    ("0x8227d7b8", "FM2_AudioResource_RegisterHook_EC34"),
    ("0x8227c250", "FM2_AudioRenderFrame_WriteFrontBufferMix"),
    ("0x8227c4a8", "FM2_AudioRenderFrame_LogSaveFrontBuffer"),
    ("0x8227e018", "FM2_AudioResource_RegisterHook_EE08"),
    ("0x8227e080", "FM2_AudioResource_RegisterHook_EE28"),
    ("0x8227e0e8", "FM2_AudioResource_RegisterHook_EE48"),
    ("0x8227e470", "FM2_AudioFrameService_QueryDeviceCaps"),
    ("0x8227ed30", "FM2_AudioSignalGate_Ctor_F3E4"),
    ("0x8227ee98", "FM2_AudioSignalGate_CtorFromCopy_F4C4"),
    ("0x8227f008", "FM2_AudioResource_RegisterHook_F34C"),
    ("0x8227f5c0", "FM2_DeferredTask_NotifyStateChangeA"),
    ("0x8227fb60", "FM2_DeferredTask_NotifyStateChangeB"),
    ("0x822802e8", "FM2_RenderPassResource_Dtor"),
    ("0x82282b90", "FM2_RenderPassResource_CtorWithLock"),
    ("0x82284d08", "FM2_WString_AssignFromWideStringView"),
    ("0x82284d60", "FM2_FxlResourceType_StaticInit24"),
    ("0x8228b2f0", "FM2_SpliceResultList_CheckLengthLimit"),
    ("0x8228bc88", "FM2_FileInfoCache_GetTransferNotifyVtable"),
    ("0x8228d6d0", "FM2_RaceGhost_CopyPlaybackState200"),
    ("0x8228f4e0", "FM2_AudioManager_SetFrameCounterField80308"),
    ("0x822905a0", "FM2_SceneGraph_ClearChildSlotByType"),
    ("0x82292018", "FM2_IntrusiveList_ShiftNodes248Byte"),
    ("0x82293f58", "FM2_Lua_InterpolateFloatField432To436"),
    ("0x82296f00", "FM2_Profile_DtorReleaseRefs"),
    ("0x822979b8", "FM2_Profile_SetTuningDisplayName"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x82256058": "True when profile race mode at +80 is 3–6 (arcade/time trial family).",
    "0x8227c4a8": "Debug path logging `SAVE FRONT BUFFER` during audio render.",
    "0x82284d60": "Static init 24-byte CFXLResourceType singleton for audio resources.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass20.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 20 (33 functions)\n", "Race entry ghost path, audio resource hooks, render pass resource, profile tuning, scene graph.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass20.md", "w", encoding="utf-8").write("\n".join(md))
