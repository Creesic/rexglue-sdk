import json

RENAMES = [
    ("0x82377580", "FM2_GpuKick_SubmitFloatShaderConstantsPm4"),
    ("0x82377660", "FM2_GpuKick_SubmitFixedShaderConstantsPm4"),
    ("0x82377750", "FM2_GpuKick_BuildLinearGammaRampTable"),
    ("0x823777a8", "FM2_GpuKick_BuildPwlGammaRampTable"),
    ("0x82378d58", "FM2_GpuKick_ComputeScalerViewportRects"),
    ("0x8237c5e8", "FM2_GpuKick_CreatePixCaptureFileOnUsb"),
    ("0x82381750", "FM2_AudioPump_SubmitRingBufferMarkerPm4"),
    ("0x82381850", "FM2_AudioPump_WaitForBlockerCompletion"),
    ("0x8239f0d0", "FM2_Png_SetReadCallbacksOnStruct"),
    ("0x8239f0e0", "FM2_Png_FatalErrorShutdown"),
    ("0x8239f118", "FM2_Png_InvokeOldWriteFnIfSet"),
    ("0x823a93d8", "FM2_Png_AllocReadStructTagged"),
    ("0x823a9468", "FM2_Png_AllocChunkBufferTagged"),
    ("0x823a94d8", "FM2_Png_FreeChunkBufferIfOwner"),
    ("0x823a4ba0", "FM2_Png_DestroyReadStructFull"),
    ("0x823b0398", "FM2_Png_ReportError15"),
    ("0x823ab188", "FM2_Jpeg_ValidateDecompressState"),
    ("0x823b2db0", "FM2_Jpeg_InitSourceManager"),
    ("0x823b4010", "FM2_Jpeg_InitComponentInfoTable"),
    ("0x823b4e40", "FM2_Jpeg_InitEntropyDecoder"),
    ("0x823b5c88", "FM2_Jpeg_InitHuffmanDecodeTable"),
    ("0x823b6188", "FM2_Jpeg_InitSampleBufferTable"),
    ("0x823b6380", "FM2_Jpeg_InitUpsampler"),
    ("0x823b6a20", "FM2_Jpeg_InitColorConverter"),
    ("0x823b79a0", "FM2_Jpeg_InitColorSpaceConverter"),
    ("0x823b8270", "FM2_Jpeg_InitUpsampleBufferPaths"),
    ("0x823bf708", "FM2_Zlib_CopyInputToSlidingWindow"),
    ("0x823c4ff8", "FM2_ComObject_InvokeChildSyncCallback"),
    ("0x824152c8", "FM2_Stl_StringIterator_DecrementSafe"),
    ("0x82417f78", "FM2_Lua_MathTwoArgCompute"),
    ("0x82418210", "FM2_Lua_UnwindAndSetErrorStatus"),
    ("0x82419cb8", "FM2_Crt_UnlockHeap"),
    ("0x82413fa8", "FM2_FMOD_NormalizeSinLookupInput"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x82377580": "Emits PM4 float shader constant fetch bundle (6437/6434 packets).",
    "0x823b2db0": "JPEG decompress: allocates and wires libjpeg source manager.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass30.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 30 (33 functions)\n",
    "GPU shader constants/gamma, PIX USB capture, audio pump PM4, PNG/JPEG init, Lua unwind, FMOD sin.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass30.md", "w", encoding="utf-8").write("\n".join(md))
