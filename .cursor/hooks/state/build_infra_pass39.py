import json

RENAMES = [
    ("0x82453e70", "FM2_Network_AllocTimedMessageNode32"),
    ("0x824554f0", "FM2_Network_IncrementTimedMessageListCount"),
    ("0x82455fb8", "FM2_Network_InsertTimedMessageNodeBeforeIter"),
    ("0x82456ae8", "FM2_Network_QueueMessageWithDeadline"),
    ("0x82459228", "FM2_Network_DispatchMessageFromQueue"),
    ("0x824bfc68", "FM2_LuaSyntax_PeekNextLexerChar"),
    ("0x824c2e88", "FM2_LuaSyntax_LoadStringLiteralChunk"),
    ("0x824c36a0", "FM2_LuaSyntax_LoadBinaryPrecompiledChunk"),
    ("0x824a0e00", "FM2_ResourceLock_CompareFrameSlotOrder"),
    ("0x82462e28", "FM2_BufferedFileRead_CompleteAsyncReadLocked"),
    ("0x82463050", "FM2_BufferedFileRead_SignalAsyncReadComplete"),
    ("0x824630e8", "FM2_BufferedFileRead_AsyncReadFileLocked"),
    ("0x82436060", "FM2_AsyncOp_MatchAndInvokeCallbackRange"),
    ("0x823297c0", "FM2_Lua_XStorage_PushCarSetupUserdataFromProfile"),
    ("0x82365f20", "FM2_Memory_SearchDeferredMapBlockByTag"),
    ("0x82368048", "FM2_Memory_AllocOrFallbackToPool"),
    ("0x82271d60", "FM2_ComObject_InitRefCountFieldsFromSource"),
    ("0x822721c8", "FM2_ComObject_CopyConstructFieldBlock"),
    ("0x823a5248", "FM2_Png_EnsureRgbThenDecodeRow"),
    ("0x8242fa60", "FM2_AsyncOp_AcquireRefAndAllocTask"),
    ("0x82436b00", "FM2_FileSys_VectorEraseFromIterator"),
    ("0x82437060", "FM2_AsyncOp_QuickSortPartitionRecursive"),
    ("0x8243c758", "FM2_ResourceLock_CtorFileChunkHandle"),
    ("0x82453900", "FM2_CompressionStream_RbTreeRotateRight"),
    ("0x8245a330", "FM2_D3D_InitPresentThrottleSingleton"),
    ("0x82467bc8", "FM2_LapTracker_UpdateCarPosition"),
    ("0x82491fb8", "FM2_AudioVoice_ResizeChannelArray"),
    ("0x824a1398", "FM2_ResourceLock_WalkFrameSlotsWithCallback"),
    ("0x824a1538", "FM2_D3D_ResolveSubsystemFromState"),
    ("0x824a2620", "FM2_ResourceLock_CopyNextFrameSlotPair"),
    ("0x824a3448", "FM2_ResourceManager_AtomicIncPendingLoadCount"),
    ("0x824a35c8", "FM2_D3D_DecRefGpuFrameSlot"),
    ("0x824ac388", "FM2_RenderAdapter_ApplyPresentationModeSwitch"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass39.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 39 (33 functions)\n",
    "Network timed messages, Lua lexer/chunk load, buffered async read, resource lock, D3D frame slots.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass39.md", "w", encoding="utf-8").write("\n".join(md))
