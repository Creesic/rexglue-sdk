import json

RENAMES = [
    ("0x82453bf0", "FM2_Network_IncrementMessageTreeIterator"),
    ("0x824a31a8", "FM2_ResourceLock_ResolveFrameAllocatorState"),
    ("0x824bafa0", "FM2_Lua_GrowStackForFileOp"),
    ("0x824bf910", "FM2_Lua_UnlinkOpenUpvaluesAbove"),
    ("0x82534638", "FM2_AsyncQueue_AllocSentinelNodeArray40"),
    ("0x82586b78", "FM2_CmdLine_AllocCircularListNodeArray28"),
    ("0x826177a8", "FM2_FrameAllocMap_AllocNode24"),
    ("0x82621210", "FM2_RbTree_RotateLeftAtChild"),
    ("0x827e7f70", "FM2_Stl_VectorEmplacePairAtEndOrRealloc"),
    ("0x82299dc0", "FM2_AudioSample_RbTreeLowerBoundByTotals"),
    ("0x82424bf0", "FM2_Lua_GetCharClassTable"),
    ("0x824301b8", "FM2_AsyncOp_FindMatchingQueueBlock"),
    ("0x82430718", "FM2_AsyncOp_TryAcquireFreeBlockByIndex"),
    ("0x82453b68", "FM2_CompressionStream_IncrementTreeIterator"),
    ("0x82454150", "FM2_Network_InitEmptyMessageTreeHead"),
    ("0x82453f28", "FM2_Network_DestroyMessageTreeRecursive"),
    ("0x82455848", "FM2_Network_RbTreeLowerBoundByMessageKeyByte"),
    ("0x8245ccb8", "FM2_DeferredTaskParams_FreeIfOutsidePool"),
    ("0x824bad00", "FM2_LuaIO_ResetLexStateForNextOp"),
    ("0x824bb320", "FM2_Lua_LinkProtoToGcObject"),
    ("0x824a2298", "FM2_ResourceLock_WalkFrameSlotRange"),
    ("0x824a37b0", "FM2_D3D_RegisterGlobalDeviceSingletonD"),
    ("0x824a38f8", "FM2_D3D_RegisterGlobalDeviceSingletonA"),
    ("0x824a3a30", "FM2_D3D_RegisterGlobalDeviceSingletonB"),
    ("0x824a3b68", "FM2_D3D_RegisterGlobalDeviceSingletonC"),
    ("0x824b3570", "FM2_RenderAdapter_DecRefPresentationSwitch"),
    ("0x824b35b8", "FM2_RenderAdapter_GetPresentationSwitchFlag"),
    ("0x824b3658", "FM2_RenderAdapter_InitPresentationCritSec"),
    ("0x824adb08", "FM2_CarAudio_ComputeUtf8EncodedSize"),
    ("0x82454558", "FM2_Network_FindSchedulerNodeByDeadline"),
    ("0x82456818", "FM2_Network_InsertTimedMessageBefore"),
    ("0x8242f658", "FM2_AsyncQueue_DecRefAndMaybeDestroy"),
    ("0x824ae128", "FM2_CircularBuffer_MoveEraseRange8"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS["0x82621210"] = "RB-tree left rotation used during insert rebalance."

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass37.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 37 (33 functions)\n",
    "Network/compression iterators, resource lock frame resolve, async alloc, Lua IO, D3D singletons.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS.get(a, REASONS[a])} |")
open(base + r"\append_infra_pass37.md", "w", encoding="utf-8").write("\n".join(md))
