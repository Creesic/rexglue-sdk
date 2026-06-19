import json

RENAMES = [
    ("0x825720c8", "FM2_ConfigEntryVector_FreeBuffer"),
    ("0x821e6ab0", "FM2_RenderAdapter_SetPresentIntervalMode"),
    ("0x821ea1d8", "FM2_ContentEntry_CopyAssignHead304"),
    ("0x821eecc0", "FM2_ContentEntry_ReserveDwordVector"),
    ("0x821f1a48", "FM2_Animation_NormalizeKeyframeWeightVMX"),
    ("0x82203420", "FM2_HashTableList_DestroySubtreeNodes"),
    ("0x82203518", "FM2_HashTableList_EraseNodeRebalance"),
    ("0x82206100", "FM2_ConfigEntryVector_DestroyAndFree"),
    ("0x824ce030", "FM2_FileSysStream_DestroyNested"),
    ("0x8242cb90", "FM2_RedirectStream_Dtor"),
    ("0x82411d20", "FM2_Thread_SleepMilliseconds"),
    ("0x824d3730", "FM2_ComObject_Ctor16BytePool"),
    ("0x8245a478", "FM2_D3D_GetGlobalPresentThrottleSingleton"),
    ("0x824a5608", "FM2_ResourceLock_WaitForReadyOrTimeout"),
    ("0x824cfe00", "FM2_BufFile_TrySeekPosition"),
    ("0x824cfeb8", "FM2_BufFile_GetStreamTell"),
    ("0x824cfff8", "FM2_BufFile_ReleaseRefCount"),
    ("0x82206208", "FM2_FileSys_DestroyEntryRange"),
    ("0x8220b658", "FM2_SceneGraph_CompareNodeNamePrefix"),
    ("0x8220c8e8", "FM2_ProfileLua_InvokeManagerCallback"),
    ("0x8220c9d8", "FM2_Lua_BindingVector_DecrementIterByIndex"),
    ("0x8220cb60", "FM2_ProfileLua_InitBindingContext"),
    ("0x822023e8", "FM2_HashTableNode_DtorOptionalFree"),
    ("0x82252f40", "FM2_ConfigEntry_DestroyRange"),
    ("0x821e7218", "FM2_ContentEntry_CopyMemcpyBlock128"),
    ("0x825dcd20", "FM2_AllocPoolAcquire4xCount"),
    ("0x8242bc68", "FM2_RefCountedThreadSafe_AssignBaseVtable"),
    ("0x8220e3a0", "FM2_ProfileLua_IsRegistryValueString"),
    ("0x8220e408", "FM2_ProfileLua_RegisterManagerClosure"),
    ("0x8221a870", "FM2_AudioManager_RouteInitByCmdlineFlag"),
    ("0x8221b688", "FM2_Profile_GetManagerHeapIfAlloc"),
    ("0x82225058", "FM2_BufferedStream_CtorRetainSource"),
    ("0x82204d08", "FM2_FileSysEntry_ReleaseRefAndClearString"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x825720c8": "Frees config-entry vector buffer via destroy-range helper.",
    "0x82411d20": "Wraps `KeDelayExecutionThread` for millisecond sleep.",
    "0x82203518": "RB-tree erase/rebalance sibling to hash-name list helper.",
    "0x8221a870": "Branches audio init on Forza cmdline flag at +1048.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass14.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 14 (33 functions)\n", "FileSys/config vectors, hash-table RB-tree, profile Lua, resource lock, buf-file.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass14.md", "w", encoding="utf-8").write("\n".join(md))
