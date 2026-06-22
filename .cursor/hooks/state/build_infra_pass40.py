import json

RENAMES = [
    ("0x825872b8", "FM2_CircularList_InitSentinelHead28"),
    ("0x826107d8", "FM2_Network_RbTreeLowerBoundInsertHint"),
    ("0x82467c98", "FM2_LapTracker_AdvanceSplineSampleIndex"),
    ("0x82465838", "FM2_LapTracker_InterpolateCarPositionOnSpline"),
    ("0x822032b8", "FM2_ComObject_DestroyFieldBlockRecursive"),
    ("0x822712a8", "FM2_ComObject_FindVectorIterByFieldAndCopy"),
    ("0x82271b00", "FM2_ComObject_CopyConstructPairFromVectorIter"),
    ("0x82201940", "FM2_ComObject_RemoveFieldSlotAndCompact"),
    ("0x82365ea0", "FM2_Memory_AllocTaggedSmallBlockFromPoolEntry"),
    ("0x823d1610", "FM2_BinaryReader_ReadAndByteSwapScalar"),
    ("0x8242ab60", "FM2_CompressionStream_RbTreeLowerBoundInsertHint"),
    ("0x82453fe8", "FM2_Network_RbTreeInitLowerBoundHint"),
    ("0x82435370", "FM2_ContentRecord_CopyConstructFrom"),
    ("0x82435428", "FM2_ContentRecord_SwapViaTemp"),
    ("0x82435a20", "FM2_Script_RegisterBindingNormalizePath"),
    ("0x82435bc8", "FM2_AsyncOp_MedianOfThreeContentRecords"),
    ("0x82435d88", "FM2_AsyncOp_QuickSortPartitionRange"),
    ("0x82435e48", "FM2_AsyncOp_IntroSortContentRecords"),
    ("0x82435ee8", "FM2_AsyncOp_HeapifyDownContentRecord"),
    ("0x82436338", "FM2_AsyncOp_PartitionContentRecords"),
    ("0x82436460", "FM2_AsyncOp_SortContentRecordSubrange"),
    ("0x824563f8", "FM2_Network_InitMessageChannelWithVectorReserve"),
    ("0x824565f8", "FM2_Network_InitMessageChannelWithDwordVector"),
    ("0x82456e48", "FM2_Network_InitMessageQueueFromSource"),
    ("0x82458538", "FM2_Network_DispatchDueMessagesFromTree"),
    ("0x824586b0", "FM2_Network_InitDeadlineTimerState"),
    ("0x82455a58", "FM2_Network_AssignMessageListFromSource"),
    ("0x824546e0", "FM2_Network_InitEmptyTimedMessageList"),
    ("0x824556a0", "FM2_Network_ReserveMessagePayloadVector"),
    ("0x8229f6f8", "FM2_Stl_VectorReserveDwordCapacity"),
    ("0x8245dcb0", "FM2_ComObject_FindListNodeByFieldOffset"),
    ("0x82461a60", "FM2_BufferedFileRead_InitAsyncReadRequest"),
    ("0x824b8108", "FM2_Lua_CountLeadingZeroBits8"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass40.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 40 (33 functions)\n",
    "Lap tracker spline, network message queue/RB-tree, content-record sort, com-object field blocks.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass40.md", "w", encoding="utf-8").write("\n".join(md))
