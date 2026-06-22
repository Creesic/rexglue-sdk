import json

RENAMES = [
    ("0x8223e0e0", "FM2_Render_BuildPassLightingBasisVMX"),
    ("0x827f6100", "FM2_STL_AllocViaComGpuAllocator"),
    ("0x821e8280", "FM2_RaceGhost_InitPlaybackContext"),
    ("0x82232180", "FM2_RaceGhost_RebuildPlaybackFromSamples"),
    ("0x822361a0", "FM2_RaceGhost_SplicePlaybackNodeList"),
    ("0x822356b0", "FM2_RaceGhost_FindPlaybackNodeByKey"),
    ("0x82236fb0", "FM2_RaceGhost_LoadPlaybackSslBindingChain"),
    ("0x8223f6a0", "FM2_CarParts_ApplyUpgradeSlotBoundsTransform"),
    ("0x821d76f0", "FM2_Render_ComputePassLightingBasisVectors"),
    ("0x827261c8", "FM2_Render_SinDegreesFloat"),
    ("0x82726350", "FM2_Render_ClampAbsFloat220M"),
    ("0x82726548", "FM2_Render_TruncateDoubleToFloat"),
    ("0x82726440", "FM2_Render_ClampAbsFloat220MAlt"),
    ("0x827265b0", "FM2_Render_TruncateDoubleToFloatAlt"),
    ("0x82726618", "FM2_Render_InitGlobalLightingTlsState"),
    ("0x82726da0", "FM2_Render_EnsureGlobalLightingTlsInit"),
    ("0x82726c78", "FM2_Render_AdvancePassLightingCycleIndex"),
    ("0x82726e18", "FM2_Render_AdvancePassLightingCycleIndexTls"),
    ("0x82726ec0", "FM2_Render_DotProduct4WithBias"),
    ("0x82726f60", "FM2_Render_LerpVec4"),
    ("0x82727080", "FM2_Render_ComputeVec4LengthSq"),
    ("0x82727158", "FM2_Render_ComputePassLightingSlotStride48"),
    ("0x82727180", "FM2_Render_AllocPassLightingSlotArray"),
    ("0x82727200", "FM2_Render_ClearPassLightingSlotVMX"),
    ("0x827272b0", "FM2_Render_ComputePassLightingSlotOffset"),
    ("0x827272c8", "FM2_Render_BindPassLightingResourcePair"),
    ("0x82727390", "FM2_Render_UpdatePassLightingSlotFields"),
    ("0x82727410", "FM2_Render_ProcessPassLightingBatchA"),
    ("0x827277f0", "FM2_Render_ProcessPassLightingBatchB"),
    ("0x8224eef8", "FM2_LiveryMask_ProcessPendingEntryUpdatesBody"),
    ("0x822516d0", "FM2_XmlReader_InsertAttrEntrySorted"),
    ("0x82260188", "FM2_ComObject_InitRefCountFieldsBody"),
    ("0x82284608", "FM2_AudioRenderFrame_PathBInner"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass58.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 58 (33 functions)\n",
    "Pass-lighting VMX/math tail, race ghost playback, car-parts bounds, livery/XML/com/audio helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass58.md", "w", encoding="utf-8").write("\n".join(md))
