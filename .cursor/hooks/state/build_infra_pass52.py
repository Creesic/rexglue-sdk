import json

RENAMES = [
    ("0x825d62e0", "FM2_Render_ViewTraversalGetDrawListEntryPtr"),
    ("0x82724200", "FM2_Render_CopyPassLightingStateBlock"),
    ("0x82513108", "FM2_Render_AllocObjectPassDrawSlotLocked"),
    ("0x82516d08", "FM2_Render_ApplyPassEnvironmentIfDeferred"),
    ("0x8251b520", "FM2_Render_TestPassOcclusionBounds"),
    ("0x8251d030", "FM2_Render_UpdateDrawCullFlagsIfVisible"),
    ("0x8251ff88", "FM2_Render_TestPassVisibilityVMX"),
    ("0x825a39a8", "FM2_Presentation_InitCarSlotTransformZeros"),
    ("0x8251b4f0", "FM2_Render_ClearPassDrawOverride"),
    ("0x8251bbb0", "FM2_Render_HasPassDrawListForSortKey"),
    ("0x8251b1a0", "FM2_Render_ComputePassDrawOverrideVMX"),
    ("0x82761080", "FM2_Render_GetPassEnvConstantSlotA"),
    ("0x827610c0", "FM2_Render_GetPassEnvConstantSlotB"),
    ("0x82761100", "FM2_Render_SetPassEnvConstantSlotA"),
    ("0x82761120", "FM2_Render_SetPassEnvConstantSlotB"),
    ("0x82512f40", "FM2_Render_GrowObjectPassDrawVector"),
    ("0x8252d060", "FM2_Render_SortVisibleRenderablesThunk"),
    ("0x8252dba0", "FM2_Render_GetDistanceKeyFromPassSlot"),
    ("0x825276b0", "FM2_Render_ExecuteSortedDrawListsPassA"),
    ("0x82527878", "FM2_Render_ExecuteSortedDrawListsPassB"),
    ("0x8252ac00", "FM2_Render_CompilePassIfStaleLocked"),
    ("0x82559df0", "FM2_Render_ObjectPassDrawSetupMaterialSlot"),
    ("0x8255b828", "FM2_Render_SubmitSortedObjectDrawListVMX"),
    ("0x8255d798", "FM2_Render_ObjectPassShouldDrawVisible"),
    ("0x8255fa28", "FM2_Render_ObjectPassEmitDrawIfVisible"),
    ("0x8272d5a8", "FM2_Render_TestFrustumOcclusionVMX"),
    ("0x82564588", "FM2_Render_AssignResourceLockFromPassData"),
    ("0x825094c0", "FM2_Render_NotifyGlobalManagerStateChange"),
    ("0x82522418", "FM2_Render_UploadDrawListMatrixConstants"),
    ("0x82528750", "FM2_Stl_Vector_EraseRangeAtCopy"),
    ("0x82538990", "FM2_Render_InstanceHybridDrawPathSort"),
    ("0x82539398", "FM2_Render_InstancePathWrapperInner"),
    ("0x8250f708", "FM2_Render_IsPassCompileResourceReady"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass52.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 52 (33 functions)\n",
    "Render view-traversal / object-pass draw cluster: visibility VMX, pass env constants, draw-list submit.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass52.md", "w", encoding="utf-8").write("\n".join(md))
