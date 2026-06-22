import json

RENAMES = [
    ("0x824a3268", "FM2_ResourceLock_ResolveFrameStateAndWalk"),
    ("0x824a32e0", "FM2_ResourceLock_EnterCritSecOrResolve"),
    ("0x824a4b98", "FM2_ResourceLock_WaitForReadyOrTimeout"),
    ("0x824ab3a0", "FM2_CompressionStream_DestroyRbTreeRoot"),
    ("0x824ab238", "FM2_CompressionStream_EraseRbTreeNode"),
    ("0x824ae5f0", "FM2_ComObject_InitRefCountBlockFields"),
    ("0x824ae658", "FM2_CarAudio_InitVoiceBufferBase"),
    ("0x824ae6b0", "FM2_CarAudio_AssignVoiceBufferVtables"),
    ("0x824adaa8", "FM2_CarAudio_InitVoiceBufferFromSelf"),
    ("0x824adda0", "FM2_CarAudio_AllocVoiceBufferNode"),
    ("0x824adec0", "FM2_CarAudio_ComputeUtf8CharWidth"),
    ("0x824a8868", "FM2_CarAudio_AllocStreamBufferAligned"),
    ("0x824a7920", "FM2_RenderAdapter_SwitchPresentationModePartial"),
    ("0x824a8b20", "FM2_CarAudio_TryStopStreamDecRef"),
    ("0x824b36e0", "FM2_RbTreeNode_LowerBoundByKey"),
    ("0x824b6840", "FM2_Lua_ShiftStackSlotDownAndCopy"),
    ("0x824b6d18", "FM2_Lua_TryCoerceStackSlotToNumber"),
    ("0x824b6f80", "FM2_Lua_GetBooleanFromStackSlot"),
    ("0x824b7060", "FM2_Lua_PushIntegerAsNumberSlot"),
    ("0x824b7318", "FM2_Lua_PushLightUserdataSlot"),
    ("0x824b8280", "FM2_Lua_PushInternedStringSlot"),
    ("0x824b8788", "FM2_Lua_RestoreSavedStackValue"),
    ("0x824b88a8", "FM2_Lua_GrowValueStackSlots"),
    ("0x824b89e8", "FM2_Lua_GrowCallInfoStack"),
    ("0x824b9148", "FM2_Lua_SetErrHandlerAndRestoreSlot"),
    ("0x824b9848", "FM2_LuaIO_ProtectedOpenFileCall"),
    ("0x824bb218", "FM2_LuaIO_DispatchFileOpStub"),
    ("0x824b81a0", "FM2_Lua_ParseStringToDouble"),
    ("0x8245ced8", "FM2_DeferredTaskParams_ReleaseChildCallback"),
    ("0x8243c5e8", "FM2_ResourceLock_FileChunkDtor"),
    ("0x824b9f00", "FM2_Lua_UpdateObjectGcMark"),
    ("0x824b9268", "FM2_Lua_IncrementCallDepthOrOverflow"),
    ("0x824b8550", "FM2_LuaSyntax_VaFormatExpectedToken"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x824bb218": "Lua IO file-op dispatcher: grow buffer then finalize read/write stub.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass36.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 36 (33 functions)\n",
    "Resource lock frame state, compression RB-tree, car audio voice, Lua stack/IO helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass36.md", "w", encoding="utf-8").write("\n".join(md))
