import json

RENAMES = [
    ("0x82432688", "FM2_LiveryMask_ParseAndLoadEntryValidate"),
    ("0x82494740", "FM2_AIDriver_ResetRaceLineOnSectorChangeFinalizeBody"),
    ("0x8249a868", "FM2_CarAudio_DtorReleaseBindingA"),
    ("0x8249a8d0", "FM2_CarAudio_DtorReleaseBindingB"),
    ("0x824a5998", "FM2_LiveryMask_ParseAndLoadEntryParseLayer"),
    ("0x824b80e8", "FM2_Lua_IncrementCallDepthOrOverflowCheck"),
    ("0x824bc5c8", "FM2_Lua_IncrementCallDepthOrOverflowGrowStack"),
    ("0x824bcd78", "FM2_Lua_IncrementCallDepthOrOverflowDispatchA"),
    ("0x824bce28", "FM2_Lua_IncrementCallDepthOrOverflowDispatchB"),
    ("0x824bcec8", "FM2_Lua_IncrementCallDepthOrOverflowDispatchC"),
    ("0x824bcf70", "FM2_Lua_IncrementCallDepthOrOverflowDispatchD"),
    ("0x824bd228", "FM2_Lua_IncrementCallDepthOrOverflowDispatchE"),
    ("0x824bedc8", "FM2_Lua_IncrementCallDepthOrOverflowErrorHandler"),
    ("0x824befa8", "FM2_Lua_IncrementCallDepthOrOverflowGuard"),
    ("0x824bf820", "FM2_Lua_IncrementCallDepthOrOverflowCleanup"),
    ("0x824d0ed8", "FM2_Input_InitControllerDevicesParseBindingField"),
    ("0x824d3e58", "FM2_Lua_PushSslUnitStringsTableAppendA"),
    ("0x824d3f00", "FM2_Lua_PushSslUnitStringsTableAppendB"),
    ("0x824d4608", "FM2_Math_AllocForceVectorComPtrInitA"),
    ("0x824d4628", "FM2_Math_AllocForceVectorComPtrInitB"),
    ("0x824db218", "FM2_Profile_ParseUnsignedFromSubStringValidateDigit"),
    ("0x824dcec0", "FM2_Profile_ParseUnsignedFromSubStringValidateRange"),
    ("0x824f26d8", "FM2_SystemEventSubscriber_CtorFields"),
    ("0x82502268", "FM2_D3D_LazyInitPresentChainInit"),
    ("0x82505d20", "FM2_Audio_MLPMatrix_FormatErrorMessageBody"),
    ("0x825065c8", "FM2_Render_FramePipelineSubmitPassABody"),
    ("0x82510a88", "FM2_Memory_AllocTaggedSmallBlockFromPoolEntryBody"),
    ("0x82510d20", "FM2_PresentationSlotVector_Clear200ByteBody"),
    ("0x82510e28", "FM2_Render_BuildObjectPassCommandBufferInitA"),
    ("0x82511038", "FM2_Render_BuildObjectPassCommandBufferInitB"),
    ("0x825110b0", "FM2_Render_AppendObjectPassDrawEntryBody"),
    ("0x825117a0", "FM2_Presentation_CopyCarDisplayBlockToSlotBody"),
    ("0x82511828", "FM2_PresentationCarConfig_DeleteOptionalBodyA"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass79.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 79 (33 functions)\n",
    "Livery/car audio, Lua call-depth overflow, profile/input, render object-pass, presentation.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass79.md", "w", encoding="utf-8").write("\n".join(md))
