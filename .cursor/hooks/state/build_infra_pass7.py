import json

RENAMES = [
    ("0x826bbda8", "FM2_XAudio2_Pool_FreeWithCallback"),
    ("0x824b8a70", "FM2_Lua_GrowStackSlots"),
    ("0x8240c028", "FM2_Win32_WaitForSingleObject"),
    ("0x82762320", "FM2_LiveConnection_ValidateXtsSession"),
    ("0x821e6bc8", "FM2_SceneNode_GetPostRaceCameraDataOffset"),
    ("0x82204898", "FM2_AsyncOp_DecRefAndCloseWaitHandle"),
    ("0x8220a5a8", "FM2_SceneProp_GetSslObjectBindingOffset"),
    ("0x822271d8", "FM2_IntrusiveList_InitCritSecSentinelNode"),
    ("0x8223da40", "FM2_Memory_BindFrameAllocatorForCategory"),
    ("0x822553b0", "FM2_GraphicsStream_NotifyListenersProfileChange"),
    ("0x826fcc10", "FM2_SQLite_Vdbe_GrowOpcodeArray"),
    ("0x826f10c0", "FM2_SQLite_ParseStack_PushEntry"),
    ("0x826ee4e0", "FM2_SQLite_CheckSoftHeapLimitAvailable"),
    ("0x826eefc0", "FM2_SQLite_Statement_FinalizeViaUserCallback"),
    ("0x826f29a8", "FM2_SQLite_TriggerList_DestroyAll"),
    ("0x826f0e58", "FM2_SQLite_ParseTriggerDestroyDeferred"),
    ("0x826f27f0", "FM2_SQLite_IndexList_DestroyAll"),
    ("0x826c3690", "FM2_XAudio2_Stream_SubmitBufferLockedInner"),
    ("0x82682b10", "FM2_FMOD_Event_LookupDescriptorById"),
    ("0x82677e80", "FM2_FMOD_System_ValidateChannelInGlobalList"),
    ("0x82671c98", "FM2_FMOD_HeapAllocMaybeZero"),
    ("0x82671ba8", "FM2_FMOD_Dsp_ReverbProcessDelayLine"),
    ("0x826714c8", "FM2_FMOD_BitBuffer_WriteBits"),
    ("0x82674278", "FM2_FMOD_CriticalSection_Leave"),
    ("0x82642510", "FM2_AudioMix_ComputeEnvelopeSampleAtPhase"),
    ("0x8263de58", "FM2_LuaHashTable_GetFloatFieldAt8"),
    ("0x827b01b0", "FM2_DeferredTask_ResetCallbackHolder"),
    ("0x82782030", "FM2_STL_Map_FindBucketWithComAllocator"),
    ("0x8277d8a8", "FM2_Stl_ThrowLengthError_WithThreadAssert"),
    ("0x827f6180", "FM2_STL_Map_DisposeNodeWithComAllocator"),
    ("0x82454640", "FM2_NetworkMessage_InitRedBlackTreeHeader"),
    ("0x82434738", "FM2_ContentEntry36_DtorAndReleaseChild"),
    ("0x82464ac8", "FM2_CircularBuffer_PushFrontNode"),
]

REASONS = {
    "0x826bbda8": "XAudio2 pool free: optional pre-free callback, dec ref, return slot to free list.",
    "0x824b8a70": "Grow Lua stack slots; rebase stack/GC refs via `sub_824B88A8`.",
    "0x8240c028": "Thin wrapper around `Nt_WaitForSingleObject`.",
    "0x82762320": "Validate live XTS COM session is initialized and signed in.",
    "0x821e6bc8": "Returns scene-node post-race camera payload at `this+10056`.",
    "0x82204898": "Interlocked dec ref; close wait handle when secondary count hits zero.",
    "0x8220a5a8": "Returns scene-prop SSL binding subobject at `this+168`.",
    "0x822271d8": "Allocate/init self-linked intrusive-list sentinel for critsec-backed list.",
    "0x8223da40": "Bind memory category to current frame allocator kind (except kind 9).",
    "0x822553b0": "Notify graphics-stream listeners after profile/state change.",
    "0x826fcc10": "Grow SQLite Vdbe opcode record array (20-byte entries).",
    "0x826f10c0": "Push entry onto SQLite parse stack via alloc helper.",
    "0x826ee4e0": "Check SQLite soft heap limit before allocation.",
    "0x826eefc0": "Finalize SQLite statement through user callback when present.",
    "0x826f29a8": "Destroy all SQLite trigger definitions and owned parse trees.",
    "0x826f0e58": "Deferred destroy of SQLite parse trigger list when refcount zero.",
    "0x826f27f0": "Free SQLite index-name list arrays.",
    "0x826c3690": "Locked XAudio2 stream buffer submit: queue packet under voice critsec.",
    "0x82682b10": "Lookup FMOD event descriptor record by packed event id.",
    "0x82677e80": "Verify FMOD channel pointer is in global channel linked list.",
    "0x82671c98": "FMOD heap alloc wrapper; zero-fill when flag at +8 clear.",
    "0x82671ba8": "FMOD reverb DSP delay-line process under critsec.",
    "0x826714c8": "Write bit field into FMOD bit buffer (set/clear masked bits).",
    "0x82674278": "Leave FMOD critical section; FMOD error code 36 if null.",
    "0x82642510": "Compute audio envelope sample from phase tables (float interp).",
    "0x8263de58": "Read float from Lua hash table field at offset +8.",
    "0x827b01b0": "Reset deferred-task callback holder vtable and invoke cleanup.",
    "0x82782030": "STL map bucket find thunk -> `sub_827F6100`.",
    "0x8277d8a8": "Throw `vector<T> too long` then assert current thread id.",
    "0x827f6180": "STL map node dispose via COM allocator or tagged free.",
    "0x82454640": "Init network-message red-black tree header/sentinel links.",
    "0x82434738": "Content entry dtor: clear string field and release child COM ref.",
    "0x82464ac8": "Push new node at front of circular buffer intrusive list.",
}

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass7.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 7 (33 functions)\n",
    "Callee list refreshed (**1162** remaining). XAudio2 pool, SQLite Vdbe/parse, FMOD, live-connection, deferred-task.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass7.md", "w", encoding="utf-8").write("\n".join(md))
