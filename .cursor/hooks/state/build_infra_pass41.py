import json

RENAMES = [
    ("0x82462020", "FM2_BufferedFileRead_SubmitAsyncReadLocked"),
    ("0x82462ba8", "FM2_BufferedFileRead_GrowRingBufferCapacity"),
    ("0x82462d70", "FM2_BufferedFileRead_AppendRingBufferSlot"),
    ("0x82481990", "FM2_AIDriver_ResetRaceLineStateOnSectorChange"),
    ("0x824a1448", "FM2_ResourceLock_WalkFrameSlotsUntilMatch"),
    ("0x824a7678", "FM2_ResourceLock_GetNullFrameSlotSentinel"),
    ("0x824a88b8", "FM2_Memory_AllocArray32Checked"),
    ("0x824a9b10", "FM2_IntrusiveList_InitSentinelHead"),
    ("0x824ae7c0", "FM2_CarAudio_AppendVoiceIdAndInitBuffer"),
    ("0x824b3498", "FM2_RenderAdapter_DecRefPresentationCritSec"),
    ("0x824b3430", "FM2_RenderAdapter_IncRefPresentationCritSec"),
    ("0x824b34f0", "FM2_RenderAdapter_TryEnablePresentationSwitch"),
    ("0x824b3630", "FM2_RenderAdapter_EnterPresentationCritSecSingleton"),
    ("0x824ba138", "FM2_Lua_MarkObjectDuringStackGrow"),
    ("0x824ba4c8", "FM2_Lua_TraverseProtoUpvaluesForMark"),
    ("0x824ba710", "FM2_Lua_ComputeStackGrowSizeForObject"),
    ("0x824b2d98", "FM2_RenderAdapter_SwitchPresentationModePartial"),
    ("0x826af9a0", "FM2_D3D_ApplyPresentationThrottleGlobals"),
    ("0x82412148", "FM2_RenderAdapter_SetPresentationSlotMultiplier"),
    ("0x8242d8a8", "FM2_Lua_CreateComPtrFromThreeLuaNumbers"),
    ("0x8236e320", "FM2_AudioRender_AllocMixBufferRegion"),
    ("0x82417950", "FM2_Crt_HeapReallocOrSetErrno"),
    ("0x824365d0", "FM2_FileSys_ComparePathsCaseInsensitive"),
    ("0x82455158", "FM2_Network_AllocMessageListHeadNode"),
    ("0x82453b18", "FM2_CompressionStream_RbTreeLowerBoundByKey"),
    ("0x82435ca0", "FM2_AsyncOp_IntroSortInnerLoop"),
    ("0x824a9910", "FM2_IntrusiveList_AllocSentinelNode"),
    ("0x82412160", "FM2_RenderAdapter_GetPresentationSlotFromGlobals"),
    ("0x82454230", "FM2_Network_InitTimedMessageNodeFields"),
    ("0x82453ed8", "FM2_Network_CopyMessagePayloadQwords"),
    ("0x824ae1b8", "FM2_CarAudio_InitVoiceBufferRange"),
    ("0x824a4108", "FM2_ResourceLock_ReleaseFrameSlotsAndWalk"),
    ("0x824a4488", "FM2_ResourceLock_TeardownFrameSlotRange"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass41.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 41 (33 functions)\n",
    "Buffered file ring buffer, render presentation adapter, Lua stack grow, resource lock teardown.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass41.md", "w", encoding="utf-8").write("\n".join(md))
