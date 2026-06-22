import json

RENAMES = [
    ("0x823743d0", "FM2_Render_EndCaptureReleaseSurfaces"),
    ("0x823816c8", "FM2_Render_WaitForGpuWorkerEvents"),
    ("0x8237f2e8", "FM2_GpuKick_SubmitViewportConstant3841"),
    ("0x8237a888", "FM2_GpuKick_SubmitVdScalerCommandBuffer"),
    ("0x8237ab08", "FM2_GpuKick_RetrainEdramAndFlushPm4"),
    ("0x8237c988", "FM2_GpuKick_NotifyPixCaptureFileEnded"),
    ("0x823818d8", "FM2_AudioPumpThread_DispatchPm4Commands"),
    ("0x82389210", "FM2_D3DXTex_Image_DtorReleaseLevels"),
    ("0x8238ed18", "FM2_Image_ConvertFloatRowTo565BE"),
    ("0x8238ee10", "FM2_Image_ConvertFloatRowTo565LE"),
    ("0x823a41e0", "FM2_Png_CreateReadStructFromCallbacks"),
    ("0x823a4cf0", "FM2_Png_DestroyReadStructsTriple"),
    ("0x823a4fe8", "FM2_Png_SetPaletteToRgbFlag"),
    ("0x823a5018", "FM2_Png_SetGrayToRgbAndScale"),
    ("0x823a51f8", "FM2_Png_SetAspectRatioMismatchFlag"),
    ("0x823a5238", "FM2_Png_SetRgbToGrayFlag"),
    ("0x823a7038", "FM2_Png_SetWriteFnAndClearOld"),
    ("0x823ab428", "FM2_Shader_InitHuffmanCallbackTable"),
    ("0x823aee90", "FM2_Zlib_ReadBitsFromInput"),
    ("0x823af088", "FM2_Zlib_FillDeflateWindowFromInput"),
    ("0x823b4538", "FM2_Png_HuffmanDecodeSymbol"),
    ("0x823c1590", "FM2_ComObject_SyncChildProperties"),
    ("0x823c81a8", "FM2_Render_BindPassSurfacesForKick"),
    ("0x823c8328", "FM2_Render_ResolvePassGpuMemoryBlocks"),
    ("0x823cdc20", "FM2_D3D_BltRegionToSurface"),
    ("0x823d3b38", "FM2_Image_ConvertFloatRowTo555BE"),
    ("0x823d3c38", "FM2_Image_ConvertFloatRowTo555LE"),
    ("0x823d3d30", "FM2_Image_ConvertFloatRowTo4444"),
    ("0x82412470", "FM2_Metrics_InsertOrRemoveGlobalNode"),
    ("0x82413fa8", "FM2_FMOD_InitSinLookupTable"),
    ("0x8242a7c0", "FM2_CompressionStream_InitListHead"),
    ("0x8242ac50", "FM2_CompressionStream_ResetAndClearPending"),
    ("0x8242b7f0", "FM2_CompressionStream_Dtor"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x8237a888": "Builds VdInitializeScaler PM4 packet for viewport blit.",
    "0x823af088": "Zlib deflate: slides window and copies input from next_in.",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass28.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 28 (33 functions)\n",
    "GPU kick/scaler, audio pump PM4, PNG/zlib/image convert, compression stream, metrics/FMOD.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass28.md", "w", encoding="utf-8").write("\n".join(md))
