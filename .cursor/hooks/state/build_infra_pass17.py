import json

RENAMES = [
    ("0x8242bc48", "FM2_BinaryStream_InitBaseVtable"),
    ("0x8222e490", "FM2_WString_IsPointerInsideBuffer"),
    ("0x821e9bf0", "FM2_ContentEntry_CopyHeadFields384"),
    ("0x8226ab58", "FM2_LapTrackerInit_CopyAssign"),
    ("0x822196f8", "FM2_SQLite_FormatQueryVprintf"),
    ("0x8224bcb8", "FM2_LiveryMask_FindPendingEntryInList"),
    ("0x8224ccc0", "FM2_LiveryMask_QueryMediaNameByCarId"),
    ("0x8224c748", "FM2_Lua_LiveryEditor_BuildLayerMaterialList"),
    ("0x8223fc00", "FM2_Lua_LiveryEditor_IsSlotIndexValid"),
    ("0x82251b88", "FM2_ProfileDb_RbTreeLowerBound"),
    ("0x82251fc0", "FM2_LiveryMask_MergeProfileRecordFromNode"),
    ("0x82254548", "FM2_LiveryMask_FindOrInsertColorKey"),
    ("0x8242bb88", "FM2_CompressionStream_CtorFromSource"),
    ("0x824bec00", "FM2_Lua_ProtectedCallSetupFrame"),
    ("0x824d3190", "FM2_WideString_ReleaseHeapBuffer"),
    ("0x822540f8", "FM2_ProfileDb_RbTreeInsertOrFind"),
    ("0x82254eb0", "FM2_Lua_LiveryEditor_ApplyLayerFromArgs"),
    ("0x82267428", "FM2_Vector48Iterator_InsertRangeFromSource"),
    ("0x8226b7e0", "FM2_RaceGhost_GetWorldStateSingleton"),
    ("0x8226b8c0", "FM2_CarAudio_GetStreamBufferSingleton"),
    ("0x8226d360", "FM2_RenderAdapter_GetDeviceContextFromOffset"),
    ("0x8226fd20", "FM2_CareerRace_GetAssistSuggestLineEnabled"),
    ("0x8226fd78", "FM2_CareerRace_GetAssistAbsEnabled"),
    ("0x8226fdd0", "FM2_CareerRace_GetAssistTcsEnabled"),
    ("0x8226fe28", "FM2_CareerRace_GetAssistStmEnabled"),
    ("0x8226fe80", "FM2_CareerRace_GetAssistManualTransEnabled"),
    ("0x822708d8", "FM2_CareerRace_GetUpgradeModifierOrStockTune"),
    ("0x82272010", "FM2_GraphicsStreamList_CtorInit"),
    ("0x822737a8", "FM2_Render_SetFramePipelineGlobalPtr"),
    ("0x82276570", "FM2_Render_MatchShaderPassKeyword"),
    ("0x822766c8", "FM2_Render_WritePassConstantSlot"),
    ("0x822768d0", "FM2_AudioSignalGate_Ctor_E734"),
    ("0x82279000", "FM2_SQLite_VfsReadSchemaCallback"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x822196f8": "Varargs format into query buffer (lap-tracker/car-db SQL).",
    "0x8224ccc0": "SQL `SELECT MediaName FROM Data_Car WHERE id = ...`.",
    "0x8226fd20": "Returns field +416 unless profile forces `ForceOffSuggLine`.",
    "0x822708d8": "Stock-tune override via `ForceStockUpgradesAndTuning` XML flag.",
    "0x822766c8": "Writes pass-constant float into PM4 bitfield slot.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass17.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 17 (33 functions)\n", "Stream/Lua/livery helpers, career assist getters, render pass setup, audio/SQLite.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass17.md", "w", encoding="utf-8").write("\n".join(md))
