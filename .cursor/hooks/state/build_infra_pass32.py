import json

RENAMES = [
    ("0x823a4660", "FM2_Png_EnsureRgbThenDecodeRow"),
    ("0x823a4e80", "FM2_Png_ValidateAndSetupRowDecode"),
    ("0x823b01b0", "FM2_Png_InvokeCustomDestroyCallback"),
    ("0x823b0238", "FM2_Png_AllocDecompressStateForChunk"),
    ("0x823b26a8", "FM2_Jpeg_InitDctCoefficientBuffers"),
    ("0x823b6d10", "FM2_Jpeg_InitIdctLookupTables"),
    ("0x823b7c68", "FM2_Jpeg_InitFastIdctLookupTables"),
    ("0x82413d68", "FM2_FMOD_SelectSignOrMagnitude"),
    ("0x82414bf0", "FM2_Stl_DequeIterator_CtorFromIterator"),
    ("0x82414c60", "FM2_Stl_DequeIterator_CtorFromPtr"),
    ("0x82414de8", "FM2_Stl_DequeIterator_AdvanceByBlock"),
    ("0x82414ee8", "FM2_Stl_DequeIterator_FindBlockForIndex"),
    ("0x82415090", "FM2_Stl_DequeIterator_CopyRangeBlocks"),
    ("0x82421120", "FM2_Lua_StoreUnwindErrorGlobals"),
    ("0x8242a768", "FM2_CompressionStream_InitSentinelHead"),
    ("0x8242ccf8", "FM2_Profile_IsContentDeviceReady"),
    ("0x8242cd98", "FM2_Lua_GetOverlappedAsyncResult"),
    ("0x8242edc8", "FM2_Storage_InitFileVolumeFromPath"),
    ("0x8242f5c8", "FM2_AsyncQueue_InitSemaphores"),
    ("0x8242f758", "FM2_ContentList_AssignRecordAndFreeBuffer"),
    ("0x824302b8", "FM2_ContentList_HeapSiftDownByCompare"),
    ("0x8242fb48", "FM2_AsyncOp_ReleasePlatformBuffer"),
    ("0x824353c8", "FM2_ContentRecord_AssignFromCopy"),
    ("0x82435df0", "FM2_ContentVector_MoveEraseRange36"),
    ("0x82436e80", "FM2_FileSys_Ctor"),
    ("0x82436ef0", "FM2_FileSys_Dtor"),
    ("0x824343a8", "FM2_FileSysWorker_CloseHandlesAndOptionalFree"),
    ("0x824381d0", "FM2_RaceGhost_EraseIntrusiveNodeFromList"),
    ("0x824391b0", "FM2_ContentManager_SnapshotChildListToBuffer"),
    ("0x82464768", "FM2_ContentBuffer_AllocTaggedCopyBuffer"),
    ("0x824538b0", "FM2_DeferredQueue_SampleElapsedTimestamp"),
    ("0x82453a18", "FM2_Set_IncrementIterator"),
    ("0x8240c348", "FM2_NtCloseHandleOrSetLastError"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x82413d68": "fsel-based sign/magnitude selection for FMOD sin lookup path.",
    "0x8242a768": "Resets compression stream intrusive list sentinel head.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass32.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 32 (33 functions)\n",
    "PNG/JPEG helpers, STL deque iterators, compression/content/file-sys, async queue, FMOD.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass32.md", "w", encoding="utf-8").write("\n".join(md))
