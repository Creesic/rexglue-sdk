import json

RENAMES = [
    ("0x825c5d78", "FM2_CareerRace_LookupXmlIntByRewardId"),
    ("0x825c5958", "FM2_CareerRace_QueryGameOptionValueInt"),
    ("0x82255ff8", "FM2_CareerRace_IsAssistOverrideRaceMode"),
    ("0x8240d918", "FM2_Thread_NtClearEventOrFail"),
    ("0x8245ce10", "FM2_ComObject_InitBaseVtable423C0"),
    ("0x824b9550", "FM2_Lua_IncrementCallDepthOrOverflow"),
    ("0x824bc470", "FM2_Lua_TypeErrorCorruptValue"),
    ("0x824bc6b8", "FM2_Lua_ProtectedCallMarkYieldable"),
    ("0x824bc718", "FM2_Lua_ResolveUpvalueOrConstant"),
    ("0x824beb38", "FM2_Lua_FindTableSlotForValue"),
    ("0x824beae0", "FM2_Lua_HashLookupClosureSlot"),
    ("0x82277cb0", "FM2_SceneCamera_CallVfunc20"),
    ("0x8222e350", "FM2_RaceGhost_GetCareerCarPropertyTable"),
    ("0x8222e838", "FM2_RaceGhost_TableExistsQuery"),
    ("0x8222f400", "FM2_RaceGhost_GetOrBuildMainCareerNode"),
    ("0x82253ef8", "FM2_ProfileDb_RbTreeInsertNode"),
    ("0x82251980", "FM2_ProfileDb_RbTreeIncrementIterator"),
    ("0x82249fe0", "FM2_LiveryMask_ParseColorKeyString"),
    ("0x8224a628", "FM2_AllocPoolAcquire292xCount"),
    ("0x8224c160", "FM2_LiveryMask_CopyPendingUpdateNode"),
    ("0x8224b6a8", "FM2_LiveryMask_ReleasePendingUpdateRefs"),
    ("0x82266908", "FM2_Vector48Record_CopyAssign"),
    ("0x8226a0f8", "FM2_Crt_MemmoveDwordRange"),
    ("0x8245b828", "FM2_Crt_SnprintfBufferVa"),
    ("0x8221a8b0", "FM2_HashName_LookupAltModuleProperty"),
    ("0x824a3398", "FM2_ResourceLock_AppendWaiterEntry"),
    ("0x82277b38", "FM2_LiveryMask_GetFieldAt2156"),
    ("0x82278e00", "FM2_AudioSignalGate_Ctor_EC5C"),
    ("0x82279e18", "FM2_AudioSignalGate_Ctor_EEBC"),
    ("0x8227d5b0", "FM2_AudioResource_RegisterHook_EB94"),
    ("0x8227d618", "FM2_AudioResource_RegisterHook_EBB4"),
    ("0x8227d680", "FM2_AudioResource_RegisterHook_EBD4"),
    ("0x8227d6e8", "FM2_AudioResource_RegisterHook_EBF4"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x825c5958": "SQL `SELECT Value FROM GameOptionValues WHERE Id=%u`.",
    "0x82255ff8": "True when profile race mode at +80 is 1, 7, or 8.",
    "0x824b9550": "Increments Lua call depth; throws at 200 frames.",
    "0x8227d5b0": "Static audio resource hook: alloc vtable + register with resource manager.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass18.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 18 (33 functions)\n", "Career XML/Lua, race ghost, livery mask, profile RB-tree, audio resource hooks.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass18.md", "w", encoding="utf-8").write("\n".join(md))
