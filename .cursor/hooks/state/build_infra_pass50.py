import json

RENAMES = [
    ("0x824e9c78", "FM2_RenderAdapter_InitSwitchModeSharedFields"),
    ("0x825c5320", "FM2_LiveProfile_ReadWriteBufferScoped"),
    ("0x8222e4e0", "FM2_XmlWriter_AppendVsnprintf"),
    ("0x82758250", "FM2_Crt_WcsncpyChecked"),
    ("0x824eb090", "FM2_Lua_BindingPairSiftDown"),
    ("0x821fcb38", "FM2_IntrusiveList_SpliceNodeRange"),
    ("0x8223f108", "FM2_CarParts_ApplyUpgradeSlotFromDescriptor"),
    ("0x82480be0", "FM2_AIDriver_UpdateRaceLineFromSector"),
    ("0x824810a8", "FM2_AIDriver_ResetRaceLineOnSectorChange"),
    ("0x824a52c0", "FM2_D3D_InitGlobalDeviceSingleton"),
    ("0x824efcb8", "FM2_ExceptionFilter_OnCppException"),
    ("0x824f0fb0", "FM2_D3D_LazyInitPresentChainNotify"),
    ("0x824f69f8", "FM2_D3D_Subscriber_InitVtables"),
    ("0x824fa8b0", "FM2_Memory_LookupFrameAllocNotifyState"),
    ("0x82502aa8", "FM2_D3D_Subscriber_EnableDeviceJournal"),
    ("0x8250a178", "FM2_Render_ViewTraversalUpdateNodes"),
    ("0x825105e8", "FM2_Render_CompileMissingPassBuffers"),
    ("0x82514e58", "FM2_Render_DecodeAndSubmitDrawKey"),
    ("0x82516700", "FM2_Render_ObjectPassDrawSetupCore"),
    ("0x82517520", "FM2_Render_UpdatePassVisibilityState"),
    ("0x824df738", "FM2_RenderAdapter_ClearPresentationBinding"),
    ("0x825c5290", "FM2_LiveProfile_ReadWriteBufferBody"),
    ("0x82429e08", "FM2_BinaryStream_InitReadScope"),
    ("0x82429e60", "FM2_BinaryStream_DtorReadScope"),
    ("0x821fc718", "FM2_Render_NotifyChainInsertSubscriber"),
    ("0x8250ffd0", "FM2_Render_InitPassCompileLock"),
    ("0x8250fbd8", "FM2_Render_DtorPassCompileLock"),
    ("0x82493680", "FM2_AIDriver_LookupTrackWidthSample"),
    ("0x82333430", "FM2_Stl_IntrosortMedianOfThreeFloats"),
    ("0x82360e38", "FM2_Input_InitAxisDefaultsFromTable"),
    ("0x8236aa48", "FM2_D3D_ComputeResourceBindingFlags"),
    ("0x823780e0", "FM2_GpuCommandBuffer_BeginPerfCaptureOrKick"),
    ("0x823a4348", "FM2_Image_LoadPngFromMemory"),
]

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass50.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 50 (33 functions)\n",
    "Render adapter/D3D init, view traversal draw setup, AI race line, XML writer, Lua binding sort, image/PNG load.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | Evidence from decompile and caller context. |")
open(base + r"\append_infra_pass50.md", "w", encoding="utf-8").write("\n".join(md))
