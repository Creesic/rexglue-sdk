import json

RENAMES = [
    ("0x824cc940", "FM2_BufFile_InitRefCountedStringPath"),
    ("0x824d0288", "FM2_BufFile_OpenAndReadArchiveEntry"),
    ("0x824cc020", "FM2_BufFile_GetGlobalModuleSingleton"),
    ("0x824cc640", "FM2_BufFile_AssignPathStringRefCounted"),
    ("0x824cc128", "FM2_BufFile_SwapModuleRefAndReleaseOld"),
    ("0x824ce8e0", "FM2_XmlReader_CtorWithBufferSizes"),
    ("0x824cf538", "FM2_XmlReader_LoadFromBufFileStream"),
    ("0x824ce138", "FM2_XmlReader_FindChildElementByPath"),
    ("0x8220e880", "FM2_RefCount_DecrementAndFreePoolBlock"),
    ("0x82221770", "FM2_Profile_MergeStringListFromOther"),
    ("0x82220098", "FM2_Profile_SpliceStringListRange"),
    ("0x8223d7e0", "FM2_CareerRaceCoordinator_ClearField33Thunk"),
    ("0x8223d750", "FM2_CareerRaceCoordinator_FreeOptionalBlockAt1"),
    ("0x822941e8", "FM2_Profile_MergeTuningRecordsFromComObject"),
    ("0x82294b50", "FM2_TuningDb_AllocLinkedListNode"),
    ("0x82294c20", "FM2_TuningRecord_AdjustScrollSliderUp"),
    ("0x82294cf0", "FM2_TuningRecord_AdjustScrollSliderDown"),
    ("0x8220a5a0", "FM2_TuningUi_GetScrollSliderObjectAt56"),
    ("0x82277b78", "FM2_SceneNodeManager_GetStateVtable100"),
    ("0x82277cc8", "FM2_SceneCamera_ApplyPhotoModeEffectParams"),
    ("0x822a7c00", "FM2_BootConfigEntry_DtorAtexit"),
    ("0x822a7ba8", "FM2_BootConfigEntry_DestroyLuaBindingArray"),
    ("0x824db1b8", "FM2_Audio_VolumeListLowerBoundByPrefix"),
    ("0x82249778", "FM2_LiveryEditor_LoadDecalsForTabIndex"),
    ("0x8223ded8", "FM2_LiveryEditor_SetCurrentDecalTabId"),
    ("0x8226fd08", "FM2_PlayerChoices_SetAssistShiftingValue"),
    ("0x823611f8", "FM2_Input_ParseControllerRumbleXmlSection"),
    ("0x82330eb0", "FM2_RaceGhost_AccumulateRotationalKeyframeDeltas"),
    ("0x82330f38", "FM2_RaceGhost_CopyPlaybackTransformBlock"),
    ("0x82330ff0", "FM2_RaceGhost_InterpolateExtendedPlaybackState"),
    ("0x823325a0", "FM2_RaceGhost_LookupAiPlayerFeeFromSql"),
    ("0x82334d48", "FM2_CareerCircuitRaceCoordinator_DestroyField2"),
    ("0x82334e48", "FM2_CareerCircuitRaceCoordinator_ResetBaseVtable"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x824d0288": "Shared buf-file open/read path for camera scripts and rumble XML.",
    "0x823611f8": "Parses ControllerRumble.xml motor sections into float rumble table.",
    "0x823325a0": "SQL lookup of AI player fee for ghost playback timing.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass25.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 25 (33 functions)\n",
    "BufFile/XML reader cluster, profile/tuning merge, race ghost playback, input rumble parse.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass25.md", "w", encoding="utf-8").write("\n".join(md))
