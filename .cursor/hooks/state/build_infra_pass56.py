import json

RENAMES = [
    ("0x8265e8a0", "FM2_FMOD_Channel_GetEventParameterValue"),
    ("0x826ef890", "FM2_SQLite_ExprSetAffinityAndFlags"),
    ("0x821d65f0", "FM2_Render_HelperB3E8ClearStringVector"),
    ("0x821e9a88", "FM2_Render_HelperB3E8InitStringFields"),
    ("0x82228560", "FM2_Render_NotifyChainInsertSubscriberSorted"),
    ("0x821d6538", "FM2_Render_HelperB3E8InitStringRange"),
    ("0x826ff938", "FM2_SQLite_ExprAppendLowercaseToken"),
    ("0x826eedc0", "FM2_SQLite_ExprGrowTokenBuffer"),
    ("0x8237ef10", "FM2_D3D_ReadGpuResourceFloatData"),
    ("0x8237ed10", "FM2_D3D_ReleaseGpuResourceRef"),
    ("0x8253d3d0", "FM2_Render_FramePipelineResolvePassSlot"),
    ("0x82237d90", "FM2_RaceGhost_SelectPlaybackNode"),
    ("0x82364140", "FM2_Memory_AllocViaPoolHandler"),
    ("0x827283f8", "FM2_RenderTls_SetGlobalPassStatePtrA"),
    ("0x82728418", "FM2_RenderTls_SetGlobalPassStatePtrB"),
    ("0x82724a50", "FM2_Render_SetPassLightingFromInverseMatrix"),
    ("0x82724ce0", "FM2_Render_TransformPointByLightingMatrix"),
    ("0x82725210", "FM2_Render_CopyLightingMatrixColumns"),
    ("0x827258d0", "FM2_Render_TestPassBoundsVMX"),
    ("0x8236f180", "FM2_RenderTls_BindPassStateToContextInner"),
    ("0x82682168", "FM2_FMOD_Channel_GetVolumeFromEventTable"),
    ("0x82684938", "FM2_FMOD_Event_GetUserDataPtrImpl"),
    ("0x824f2e48", "FM2_ForzaTV_InitSubscriberVtables"),
    ("0x824f2ef0", "FM2_ForzaTV_EnsureSingletonInit"),
    ("0x827253c8", "FM2_Render_BuildPassLightingMatrixFromAngles"),
    ("0x827254c0", "FM2_Render_SetPassLightingScaleMatrix"),
    ("0x82725698", "FM2_Render_MultiplyPassMatrixVMXVariant"),
    ("0x827259e0", "FM2_Render_SetPassLightingModeScalar"),
    ("0x82725b70", "FM2_Render_ComparePassMatrixBytesVMX"),
    ("0x82724d68", "FM2_Render_TransformDirByLightingMatrix"),
    ("0x82724e40", "FM2_Render_ProjectPassBoundsToScreen"),
    ("0x82725160", "FM2_Render_SetPassLightingDiagonalMatrix"),
    ("0x827261f8", "FM2_Render_ApplyPassLightingMatrixToState"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass56.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 56 (33 functions)\n",
    "Render pass-lighting VMX tail, FMOD/SQLite, race ghost playback, D3D GPU resource read, ForzaTV singleton.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass56.md", "w", encoding="utf-8").write("\n".join(md))
