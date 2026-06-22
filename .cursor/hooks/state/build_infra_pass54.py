import json

RENAMES = [
    ("0x827c5e38", "FM2_Render_SetPassTimingScalar"),
    ("0x82725780", "FM2_Render_MultiplyMatrix4x4VMX"),
    ("0x821d7608", "FM2_Render_ViewTraversalNormalizeBasisVMX"),
    ("0x82724218", "FM2_Render_SetPassLightingCoeffs"),
    ("0x82724888", "FM2_Render_ApplyPassLightingStateInner"),
    ("0x82724958", "FM2_Render_CopyPassViewMatrix4x4"),
    ("0x82724988", "FM2_Render_CopyPassProjMatrix4x4"),
    ("0x827249a0", "FM2_Render_SetPassLightingFromMatrix"),
    ("0x827250f8", "FM2_Render_TransformVectorByMatrix4x4"),
    ("0x827255b0", "FM2_Render_MultiplyMatrix4x4AccumVMX"),
    ("0x827306c0", "FM2_ConstantBuffer_UploadVector4Block"),
    ("0x82725310", "FM2_Render_BuildPassLightingMatrixVMX"),
    ("0x82724568", "FM2_Render_UpdatePassSortKeysFromBounds"),
    ("0x82723d18", "FM2_RenderContext_UploadMatrixConstantsFromPass"),
    ("0x82723860", "FM2_RenderTls_BatchSubmitDrawPacketsTail"),
    ("0x825377e8", "FM2_Render_InstancePathWrapperTraverse"),
    ("0x8253cfd0", "FM2_Render_FramePipelineNotifyPassState"),
    ("0x8253d440", "FM2_Render_FramePipelineCleanupPassSlots"),
    ("0x8252b8d8", "FM2_Render_SortVisibleRenderablesIntrosort"),
    ("0x821e1d60", "FM2_AudioRenderFrame_FlushLogBufferChunk"),
    ("0x821efec0", "FM2_Render_HelperB3E8ResetState"),
    ("0x82515d58", "FM2_Render_TestObjectPassOcclusionWrapped"),
    ("0x82659258", "FM2_Memory_AllocFromAllocatorContext"),
    ("0x824df418", "FM2_RenderAdapter_ResetPresentationStateBlock"),
    ("0x825c5f48", "FM2_ProfileDb_InitPropertyBagCritSec"),
    ("0x82557428", "FM2_BufferedFileRead_RandUnitFloat"),
    ("0x8258b370", "FM2_RbTree_FindLowerBoundNodeByKey"),
    ("0x82461428", "FM2_Crt_CreateSemaphoreA"),
    ("0x8239f2b8", "FM2_Png_AllocDecodeStateBuffer"),
    ("0x82724078", "FM2_RenderTls_BindPassStateToContext"),
    ("0x82724160", "FM2_Render_InitPassLightingStateBlock"),
    ("0x82724270", "FM2_Render_ApplyPassLightingCoeffsVMX"),
    ("0x827260e8", "FM2_Math_FastInvSqrtTaylor"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass54.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 54 (33 functions)\n",
    "Render pass lighting VMX cluster, frame pipeline pass cleanup, sort/visibility helpers, PNG/math.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass54.md", "w", encoding="utf-8").write("\n".join(md))
