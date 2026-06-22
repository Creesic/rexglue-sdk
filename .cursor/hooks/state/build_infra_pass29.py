import json

RENAMES = [
    ("0x82439190", "FM2_CompressionStream_ReleasePendingRef"),
    ("0x824614d8", "FM2_LiveryMask_InitPendingRecordHeader"),
    ("0x82461568", "FM2_LiveryMask_SpawnWorkerThread"),
    ("0x8237b978", "FM2_Render_EndPixCaptureAndRestoreDisplay"),
    ("0x823a9450", "FM2_Png_FreeTaggedReadStruct"),
    ("0x8242a8f0", "FM2_CompressionStream_ClearPendingLists"),
    ("0x824cc0b0", "FM2_BufFile_BindGlobalModuleRef"),
    ("0x824cca60", "FM2_BufFile_ResolveOrLoadModuleRef"),
    ("0x824cc5e8", "FM2_BufFile_EnsureCapacityForLength"),
    ("0x8228e4f8", "FM2_RaceGhost_CompareAndSwapKeyframeTriple"),
    ("0x8228e5d0", "FM2_RaceGhost_SiftDownKeyframeHeap"),
    ("0x8228e6c8", "FM2_RaceGhost_HeapifyAndRestoreRange"),
    ("0x8228e9e8", "FM2_RaceGhost_HeapsortPartitionThreeWay"),
    ("0x8228ea88", "FM2_RaceGhost_HeapifyKeyframeRange"),
    ("0x8228eaf8", "FM2_RaceGhost_InsertionSortKeyframeRange"),
    ("0x8228ec20", "FM2_RaceGhost_HeapSortRecursive"),
    ("0x822f9ec0", "FM2_RaceGhost_SplicePlaybackListNodes"),
    ("0x823313a8", "FM2_RaceGhost_InitPlaybackTaskWrapper"),
    ("0x8235d070", "FM2_UI_GetPropertyMaskByteAtOffset"),
    ("0x82366100", "FM2_Memory_AllocDeferredFreeMapNode24"),
    ("0x82366090", "FM2_Memory_AllocDeferredMapBlockLocked"),
    ("0x8236ded0", "FM2_Render_AddRefPassSurfaceAt12412"),
    ("0x8236e1e0", "FM2_Render_AddRefPassSurfaceAt12416"),
    ("0x8236e538", "FM2_Render_AllocGpuPassMemoryBlock"),
    ("0x8242a708", "FM2_CompressionStream_AllocListSentinel"),
    ("0x8230f1a8", "FM2_ReplayPendingString_Ctor"),
    ("0x823296d8", "FM2_Lua_PopLuaCarSetupPointerUserdata"),
    ("0x82381428", "FM2_AudioPump_ComputeRingBufferMarker"),
    ("0x82381490", "FM2_AudioPump_CopyWaveChunkToRing"),
    ("0x82381590", "FM2_AudioPump_FlushPendingWaveChunks"),
    ("0x8236a460", "FM2_D3D_ComputeSurfaceCopyPitch"),
    ("0x82369a50", "FM2_AudioRender_CopySurfaceRegionToBuffer"),
    ("0x8236a8f0", "FM2_D3D_ComputeSurfaceBlitRegion"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x8237b978": "PIX capture end: restore display mode and release capture surfaces.",
    "0x82461568": "Spawns XAP worker thread for livery mask pending-record processing.",
    "0x8228eaf8": "Insertion sort for small race-ghost keyframe subranges.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass29.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 29 (33 functions)\n",
    "Livery mask workers, compression stream, race ghost sort, buf-file refs, audio pump ring, GPU pass alloc.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass29.md", "w", encoding="utf-8").write("\n".join(md))
