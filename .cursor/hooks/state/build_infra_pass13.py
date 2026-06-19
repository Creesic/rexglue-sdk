import json

RENAMES = [
    ("0x821e7760", "FM2_Stl_StringIter_InitFromBufferBounds"),
    ("0x824aee08", "FM2_RenderAdapter_GetFrameTimingVtablePtr"),
    ("0x821f1488", "FM2_RenderAdapter_ApplyFrameTimingDeltaFlagFB"),
    ("0x8221b4e8", "FM2_PropertyBag_AllocNodePool128xCount"),
    ("0x82369470", "FM2_D3D_SyncRingBufferSlotAfterGpuError"),
    ("0x8240bff8", "FM2_Xam_ShowDirtyDiscAndRelaunch"),
    ("0x82413490", "FM2_Crt_VsprintfS_L"),
    ("0x824615e8", "FM2_D3D_SuspendLowPriorityWorkerThreads"),
    ("0x824a68f0", "FM2_ResourceLock_WaitReadyState3"),
    ("0x8258b2f8", "FM2_AllocPoolAcquire44xCount"),
    ("0x8253f108", "FM2_HashNamePropertyList_EraseNodeRebalance"),
    ("0x821d7980", "FM2_Render_MatrixMultiplyVMX128_From16Byte"),
    ("0x821d9ef8", "FM2_BufFile32768_FlushWriteBuffer"),
    ("0x821df050", "FM2_BufFile_CompareRangeSubstr"),
    ("0x821df128", "FM2_BufFile32768_DtorInner"),
    ("0x821e6238", "FM2_Render_TransformVec4x4VMX128"),
    ("0x821e6e40", "FM2_Stl_SnprintfPartNames128"),
    ("0x821e7678", "FM2_RenderAdapter_SetVblankWaitState"),
    ("0x821e82f8", "FM2_SceneObject_ReleaseRefFields78_80_88"),
    ("0x821ec2d0", "FM2_ContentEntry_CopyTailFields384"),
    ("0x821ef7f8", "FM2_ContentEntry_CopyVec4AndSubrecord"),
    ("0x821f13b8", "FM2_RenderAdapter_ApplyFrameTimingDeltaFlagFD"),
    ("0x821f1f70", "FM2_RenderAdapter_QueueFrameTimingUpdateInner"),
    ("0x821f2330", "FM2_RenderAdapter_TogglePresentInterval"),
    ("0x821f3b08", "FM2_Animation_ClampKeyframeWeightVMX"),
    ("0x82202310", "FM2_HashTable_SetEntryWithRefAdd"),
    ("0x82203918", "FM2_HashTableList_EraseRangeIterators"),
    ("0x822041f8", "FM2_NetNotification_Ctor"),
    ("0x821e6a10", "FM2_RenderAdapter_GetFieldAt10036"),
    ("0x821f0ff8", "FM2_RenderAdapter_UpdateSceneNodeDrawState"),
    ("0x821f0e30", "FM2_RenderAdapter_MarkFrameTimingDirty"),
    ("0x822034c0", "FM2_HashTableList_ClearSentinelLinks"),
    ("0x821ef130", "FM2_ContentEntry_CopyDwordVector"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x821e7760": "Init string iterator with bounds check against STL string buffer.",
    "0x824aee08": "Returns static frame-timing vtable pointer `off_8299B8C4`.",
    "0x8240bff8": "Shows dirty-disc UI then `XLaunchNewImage` (GPU hang path).",
    "0x82413490": "Thin `vsprintf_s_l` CRT wrapper.",
    "0x8221b4e8": "Pool alloc `0x80 * count` for property-bag nodes.",
    "0x8258b2f8": "Pool alloc `44 * count` (FMOD/network RB-tree nodes).",
    "0x8253f108": "RB-tree erase/rebalance for hash-name property list.",
    "0x821df128": "Tears down buf-file stream vtables and decrefs camera script.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass13.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = ["### Infrastructure pass 13 (33 functions)\n", "Render adapter timing, D3D hang path, buf-file, hash-table RB-tree, content entry.\n", "| Address | New name | Reasoning |", "| --- | --- | --- |"]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass13.md", "w", encoding="utf-8").write("\n".join(md))
