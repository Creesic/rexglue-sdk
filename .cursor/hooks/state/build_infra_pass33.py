import json

RENAMES = [
    ("0x8240bde0", "FM2_D3D_ReadKernelTickCountImport"),
    ("0x82414b60", "FM2_Crt_CopyConstructRefCountedCStr"),
    ("0x824f16e8", "FM2_AuctionHouse_LazyInitStaticData"),
    ("0x82277d60", "FM2_Lua_PhotoModeCamera_ReadDepthBufferSample"),
    ("0x822a1690", "FM2_Lua_GetNetworkXuidUserdataOrRaise"),
    ("0x8230efe0", "FM2_Replay_CreateGaragePendingStringTask"),
    ("0x8230f140", "FM2_RewardReveal_PendingUpgradeCompat_Ctor"),
    ("0x8230f470", "FM2_Lua_BuildPendingUpgradeCompatUserdata"),
    ("0x8230f9a0", "FM2_RewardReveal_CreatePendingCompatTask"),
    ("0x8230fa90", "FM2_Lua_PopPendingUpgradeCompatUserdata"),
    ("0x823418e8", "FM2_RaceGhost_ComputePlaybackInterpolationWeight"),
    ("0x82363580", "FM2_ContentBuffer_AllocWithTrackingNotify"),
    ("0x823b0138", "FM2_Png_ResetDecompressStateAndSyncChild"),
    ("0x823c1658", "FM2_Png_AllocInflateStateBuffers"),
    ("0x8242a630", "FM2_CompressionStream_FreeChildListRecursive"),
    ("0x8242fe88", "FM2_ContentList_HeapSiftUpByCompare"),
    ("0x8242db78", "FM2_Lua_GetAppForzaStateOffset8"),
    ("0x82461500", "FM2_LiveryMask_CloseWorkerThreadHandle"),
    ("0x82461508", "FM2_LiveryMask_IsWorkerThreadRunning"),
    ("0x824615c8", "FM2_LiveryMask_CopyPendingRecordName128"),
    ("0x824603c8", "FM2_AudioFrameService_InvokeVtableUpdate"),
    ("0x82460580", "FM2_AudioFrameService_InitTimingBaseline"),
    ("0x82463698", "FM2_Math_PositiveModulo"),
    ("0x82464088", "FM2_CmdLine_InitParamsVtable"),
    ("0x824662b8", "FM2_AIOvertake_GetAssistValueAtIndex172"),
    ("0x824662d0", "FM2_AIOvertake_GetAssistValueAtIndex192"),
    ("0x82466a78", "FM2_CareerRace_GetElapsedRaceTimeFloat"),
    ("0x82466a88", "FM2_CareerRace_GetTotalRaceTimeFloat"),
    ("0x82466b80", "FM2_CareerRace_SubtractPhotoModeDeltaTime"),
    ("0x8246c4d0", "FM2_AIOvertake_IsHornThresholdExceeded"),
    ("0x8246d020", "FM2_AIOvertake_GetGlobalRaceTimeFloat"),
    ("0x824804d8", "FM2_AIDriver_IsAssistModeActive"),
    ("0x824a1688", "FM2_Presentation_GetCarResourceLoadCountA"),
]

REASONS = {a: "Evidence from decompile and caller context." for a, _ in RENAMES}
REASONS.update({
    "0x8240bde0": "Returns kernel tick count via import slot MEMORY[0xBD].",
    "0x82461508": "GetExitCodeThread check for STILL_ACTIVE (259).",
})

base = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state"
open(base + r"\rename_infra_pass33.json", "w", encoding="utf-8").write(
    json.dumps([{"addr": a, "name": n} for a, n in RENAMES])
)
md = [
    "### Infrastructure pass 33 (33 functions)\n",
    "Tick count import, Lua/replay/reward userdata, livery worker, race ghost interp, AI/career helpers.\n",
    "| Address | New name | Reasoning |",
    "| --- | --- | --- |",
]
for a, n in RENAMES:
    md.append(f"| `{a}` | `{n}` | {REASONS[a]} |")
open(base + r"\append_infra_pass33.md", "w", encoding="utf-8").write("\n".join(md))
