### Infrastructure pass 35 (33 functions)

Buffered file read, network RB-tree, async queue, D3D singletons, car audio, input wheel.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x826ec7f0` | `FM2_CircularBuffer_GetCapacityField8` | Returns *(obj+8) capacity used by circular buffer select/resize. |
| `0x82461bf8` | `FM2_BufferedFileRead_MaybeRaiseDiscError` | Evidence from decompile and caller context. |
| `0x82461ed0` | `FM2_BufferedFileRead_UnpackHandleDescriptor` | Evidence from decompile and caller context. |
| `0x8228b240` | `FM2_AsyncOp_AllocTuningListNodePair` | Evidence from decompile and caller context. |
| `0x8230f2a8` | `FM2_RewardReveal_CreatePendingCompatTaskWrapper` | Evidence from decompile and caller context. |
| `0x8240cbd0` | `FM2_SystemTimeFields_FromKeQuerySystemTime` | Evidence from decompile and caller context. |
| `0x8240d950` | `FM2_Memory_NtAllocateVirtualMemoryWrapped` | Evidence from decompile and caller context. |
| `0x824607f0` | `FM2_AsyncQueue_InitSentinelListHead` | Evidence from decompile and caller context. |
| `0x82461b50` | `FM2_BufferedFileRead_GetOverlappedResult` | Evidence from decompile and caller context. |
| `0x824618e8` | `FM2_BufferedFileRead_FatalSecuredFileError` | Logs secured-file error and spins forever (noreturn debug trap). |
| `0x82464f60` | `FM2_AIDriver_GetPathBufferLength` | Evidence from decompile and caller context. |
| `0x82492be0` | `FM2_CircularBuffer_IsSingleSlotMode` | Evidence from decompile and caller context. |
| `0x82453290` | `FM2_Input_XamInputGetCapabilitiesEx` | Evidence from decompile and caller context. |
| `0x82453330` | `FM2_Input_MapXamStatusToWheelError` | Evidence from decompile and caller context. |
| `0x82453de8` | `FM2_HashName_ClonePropertyTreeRecursive` | Evidence from decompile and caller context. |
| `0x82453ac8` | `FM2_Network_RbTreeLowerBoundByMessageKey` | Evidence from decompile and caller context. |
| `0x82453f88` | `FM2_Network_InitMessageInsertContext` | Evidence from decompile and caller context. |
| `0x82455958` | `FM2_Network_EraseMessageTreeNode` | Evidence from decompile and caller context. |
| `0x82455ca0` | `FM2_Network_LowerBoundInsertMessageNode` | Evidence from decompile and caller context. |
| `0x8245db98` | `FM2_CmdLine_InitCircularListHead` | Evidence from decompile and caller context. |
| `0x8245ef70` | `FM2_ContentDb_RbTreeLowerBoundByKey` | Evidence from decompile and caller context. |
| `0x8245efd0` | `FM2_ContentDb_InitHashLookupContext` | Evidence from decompile and caller context. |
| `0x8245f290` | `FM2_RbTree_InsertNodeAndRebalance` | Evidence from decompile and caller context. |
| `0x82430930` | `FM2_AsyncOp_TryPopFreeBlockFromQueue` | Evidence from decompile and caller context. |
| `0x82431280` | `FM2_AsyncOp_SpliceIntrusiveListHead` | Evidence from decompile and caller context. |
| `0x824a4768` | `FM2_D3D_InitGlobalDeviceSingletonA` | Evidence from decompile and caller context. |
| `0x824a47b0` | `FM2_D3D_InitGlobalDeviceSingletonB` | Evidence from decompile and caller context. |
| `0x824a47f8` | `FM2_D3D_InitGlobalDeviceSingletonC` | Evidence from decompile and caller context. |
| `0x824a4840` | `FM2_D3D_InitGlobalDeviceSingletonD` | Evidence from decompile and caller context. |
| `0x824ac470` | `FM2_RaceEntry_GetVisibilityChangeVtable` | Evidence from decompile and caller context. |
| `0x824a7410` | `FM2_CarAudioComponent_Dtor` | Evidence from decompile and caller context. |
| `0x824a7698` | `FM2_CarAudio_GetStaticMetaPointer` | Evidence from decompile and caller context. |
| `0x824b1700` | `FM2_AudioSample_InitOutputPairDescriptor` | Evidence from decompile and caller context. |