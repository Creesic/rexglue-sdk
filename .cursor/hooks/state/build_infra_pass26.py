import json

RENAMES = [
    ("0x82331710", "FM2_RaceGhost_CopyPlaybackUpdateArgs"),
    ("0x82331d90", "FM2_RaceGhost_EnqueueDeferredPlaybackTask"),
    ("0x82332508", "FM2_RaceGhost_SubmitPlaybackUpdateAsync"),
    ("0x82331f10", "FM2_RaceGhost_SchedulePlaybackUpdateTask"),
    ("0x82357638", "FM2_Stl_SlideStringRecords32Bytes"),
    ("0x82365670", "FM2_Stl_SlideRecords16Bytes"),
    ("0x82357c08", "FM2_Stl_Vector_EraseStringRangeAt"),
    ("0x82365a40", "FM2_Render_VectorEraseDrawRangeAt"),
    ("0x8235a728", "FM2_Input_SetWheelEnabledAndDetect"),
    ("0x8235f9d8", "FM2_Input_CopyRumbleDefaults88Bytes"),
    ("0x82363368", "FM2_Input_ControllerDevice_InitSslBindings"),
    ("0x82364020", "FM2_Memory_PoolHandlerCanFreeCategory"),
    ("0x82367328", "FM2_Memory_SetPhysicalAllocLockFlag"),
    ("0x82367338", "FM2_Memory_GetPhysicalAllocLockFlag"),
    ("0x82366460", "FM2_Memory_DeferredFreeRbTreeInsert"),
    ("0x8236c828", "FM2_GpuKick_SubmitShaderSyncPm4Bundle"),
    ("0x8236c948", "FM2_GpuKick_SubmitDrawSetupPm4Bundle"),
    ("0x8236bd00", "FM2_AudioRender_SubmitFrontBufferPath"),
    ("0x823748d0", "FM2_Render_ScopedBatch_FinalizeGpuKick"),
    ("0x82371250", "FM2_GpuKick_SubmitViewportConstantPm4"),
    ("0x82378940", "FM2_GpuKick_SubmitTextureFetchPm4"),
    ("0x823789d0", "FM2_GpuKick_RotateMultiDrawTargetPm4"),
    ("0x8237f358", "FM2_GpuKick_ToggleClockGatingPm4"),
    ("0x82356af8", "FM2_BufFile_SeekAndTestPathPrefixMatch"),
    ("0x8235ad90", "FM2_UI_GetMaxPropertyAbsValueHalfStep"),
    ("0x823815f0", "FM2_AudioPumpThread_SignalWorkerEvent"),
    ("0x82388cc8", "FM2_Image_SwapEndian128BitRow"),
    ("0x8239f1c8", "FM2_Png_CompareSignatureBytes"),
    ("0x823a4f90", "FM2_Png_SetInterlaceHandlingFlag"),
    ("0x823a4fa0", "FM2_Png_SetBitDepth16Flag"),
    ("0x823a4fc0", "FM2_Png_ClampBitDepthToAtLeast8"),
    ("0x823ae8e0", "FM2_Zlib_CopyPendingInputToWindow"),
    ("0x822fd1c8", "FM2_RaceGhost_MergeSortedKeyframeBufferSelfCheck"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x823748d0": "Scoped batch teardown: sync GPU, release perf counters, free kick tag.",
    "0x823ae8e0": "Zlib deflate: copy pending input bytes into sliding window.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass26.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 26 (33 functions)\n",
    "Race ghost async playback, STL vector erase, GPU PM4 kick helpers, PNG/zlib, audio pump.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass26.md", "w", encoding="utf-8").write("\n".join(md))
