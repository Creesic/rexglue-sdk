import json

RENAMES = [
    ("0x824a16b0", "FM2_Presentation_GetCarResourceLoadCountB"),
    ("0x824a2338", "FM2_LiveryMask_ResourceManager_Ctor"),
    ("0x824a0e20", "FM2_LiveryRenderManager_ClearFinalizeEvent"),
    ("0x824a0ec0", "FM2_CriticalSection_SetRawEventSlot"),
    ("0x824a0fd8", "FM2_ResourceLock_DecrementRefUnderLock"),
    ("0x824530f0", "FM2_StreamRead_ReleasePriorAndGetCurrent"),
    ("0x82453400", "FM2_Input_DetectWheelSubtypeFromCaps"),
    ("0x824540b0", "FM2_HashName_LinkPropertyTreeNode"),
    ("0x82454370", "FM2_Network_AppendMessagePayloadNodes"),
    ("0x82455eb0", "FM2_Network_ClearMessageTreeRoot"),
    ("0x824562f8", "FM2_Network_InsertSortedMessageNode"),
    ("0x8245b560", "FM2_PropertyBag_InitRbTreeNodeFromKey"),
    ("0x8245b1a8", "FM2_HashName_EraseRbTreeNodeAndRebalance"),
    ("0x8245eec0", "FM2_RbTree_IncrementIteratorPastSentinel"),
    ("0x8245f030", "FM2_ContentDb_CountHashRangeNodes"),
    ("0x8245f0b8", "FM2_ContentDb_InitHashRangeIterator"),
    ("0x824609c8", "FM2_AsyncQueue_GlobalStaticInit"),
    ("0x8245e898", "FM2_CmdLineGlobal_StaticInit"),
    ("0x8242fce8", "FM2_FontSystem_DecrementRefAndMaybeClose"),
    ("0x82430a58", "FM2_AsyncOp_AllocAlignedPlatformBuffer"),
    ("0x824321d0", "FM2_AsyncOp_EnqueueUnderGlobalLock"),
    ("0x82439470", "FM2_Input_EraseControllerStateListNode"),
    ("0x82439a68", "FM2_D3D_Subscriber_TryEnableDeviceLocked"),
    ("0x824621a8", "FM2_BufferedFileRead_SyncOrAsyncRead"),
    ("0x82463280", "FM2_BufferedFileRead_SyncOrAsyncReadFile"),
    ("0x824920e0", "FM2_Sort_HeapSortDwordArrayInPlace"),
    ("0x82494608", "FM2_CircularBuffer_SelectSlotAndResize"),
    ("0x8248f9c0", "FM2_CircularBuffer_WrapReadIndices"),
    ("0x8247dd38", "FM2_AIDriver_ClearPathSegmentFlags"),
    ("0x8247e2e8", "FM2_AIDriver_SampleSteeringFromPath"),
    ("0x82482290", "FM2_AIDriver_ResetRaceLineState"),
    ("0x824f1650", "FM2_AuctionHouse_Ctor"),
    ("0x8243c640", "FM2_ResourceLock_AssignHandleAndWaitReady"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x824f1650": "Constructs Forza2::CAuctionHouse with intrusive-list sentinel nodes.",
    "0x824621a8": "Routes read through async wrapper or direct NtRead path.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass34.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 34 (33 functions)\n",
    "Network/hash RB-tree, async queue, buffered file read, AI driver, presentation/livery, auction house.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass34.md", "w", encoding="utf-8").write("\n".join(md))
