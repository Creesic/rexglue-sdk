import json

RENAMES = [
    ("0x826ec7f0", "FM2_CircularBuffer_GetCapacityField8"),
    ("0x82461bf8", "FM2_BufferedFileRead_MaybeRaiseDiscError"),
    ("0x82461ed0", "FM2_BufferedFileRead_UnpackHandleDescriptor"),
    ("0x8228b240", "FM2_AsyncOp_AllocTuningListNodePair"),
    ("0x8230f2a8", "FM2_RewardReveal_CreatePendingCompatTaskWrapper"),
    ("0x8240cbd0", "FM2_SystemTimeFields_FromKeQuerySystemTime"),
    ("0x8240d950", "FM2_Memory_NtAllocateVirtualMemoryWrapped"),
    ("0x824607f0", "FM2_AsyncQueue_InitSentinelListHead"),
    ("0x82461b50", "FM2_BufferedFileRead_GetOverlappedResult"),
    ("0x824618e8", "FM2_BufferedFileRead_FatalSecuredFileError"),
    ("0x82464f60", "FM2_AIDriver_GetPathBufferLength"),
    ("0x82492be0", "FM2_CircularBuffer_IsSingleSlotMode"),
    ("0x82453290", "FM2_Input_XamInputGetCapabilitiesEx"),
    ("0x82453330", "FM2_Input_MapXamStatusToWheelError"),
    ("0x82453de8", "FM2_HashName_ClonePropertyTreeRecursive"),
    ("0x82453ac8", "FM2_Network_RbTreeLowerBoundByMessageKey"),
    ("0x82453f88", "FM2_Network_InitMessageInsertContext"),
    ("0x82455958", "FM2_Network_EraseMessageTreeNode"),
    ("0x82455ca0", "FM2_Network_LowerBoundInsertMessageNode"),
    ("0x8245db98", "FM2_CmdLine_InitCircularListHead"),
    ("0x8245ef70", "FM2_ContentDb_RbTreeLowerBoundByKey"),
    ("0x8245efd0", "FM2_ContentDb_InitHashLookupContext"),
    ("0x8245f290", "FM2_RbTree_InsertNodeAndRebalance"),
    ("0x82430930", "FM2_AsyncOp_TryPopFreeBlockFromQueue"),
    ("0x82431280", "FM2_AsyncOp_SpliceIntrusiveListHead"),
    ("0x824a4768", "FM2_D3D_InitGlobalDeviceSingletonA"),
    ("0x824a47b0", "FM2_D3D_InitGlobalDeviceSingletonB"),
    ("0x824a47f8", "FM2_D3D_InitGlobalDeviceSingletonC"),
    ("0x824a4840", "FM2_D3D_InitGlobalDeviceSingletonD"),
    ("0x824ac470", "FM2_RaceEntry_GetVisibilityChangeVtable"),
    ("0x824a7410", "FM2_CarAudioComponent_Dtor"),
    ("0x824a7698", "FM2_CarAudio_GetStaticMetaPointer"),
    ("0x824b1700", "FM2_AudioSample_InitOutputPairDescriptor"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x824618e8": "Logs secured-file error and spins forever (noreturn debug trap).",
    "0x826ec7f0": "Returns *(obj+8) capacity used by circular buffer select/resize.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass35.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 35 (33 functions)\n",
    "Buffered file read, network RB-tree, async queue, D3D singletons, car audio, input wheel.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass35.md", "w", encoding="utf-8").write("\n".join(md))
