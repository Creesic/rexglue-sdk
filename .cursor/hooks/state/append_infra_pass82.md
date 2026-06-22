### Infrastructure pass 82 (33 functions)

Newly exposed callees from passes 80–81: presentation slot vector, car presentation dtor, render sort/visibility, D3D validate.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825a0298` | `FM2_Render_DrawPassMaterialSetupSharedHelper` | Evidence from decompile and caller context. |
| `0x825e5230` | `FM2_LiveryMask_OrRaceGhostSharedUtil` | Evidence from decompile and caller context. |
| `0x8272d7a0` | `FM2_Presentation_CopyCarDisplayBlockSharedAppend` | Evidence from decompile and caller context. |
| `0x821efe38` | `FM2_Render_HelperB3E8DrawPathInit` | Evidence from decompile and caller context. |
| `0x82227100` | `FM2_D3D_ValidateResourceHandlesCheckA` | Evidence from decompile and caller context. |
| `0x82227158` | `FM2_D3D_ValidateResourceHandlesCheckB` | Evidence from decompile and caller context. |
| `0x82369fa0` | `FM2_D3D_ValidateResourceHandlesRecoverSlot` | Evidence from decompile and caller context. |
| `0x82369ff0` | `FM2_D3D_ValidateResourceHandlesRecoverNoOp` | Evidence from decompile and caller context. |
| `0x8236ea80` | `FM2_Render_InstancePathWrapperCallThunk` | Evidence from decompile and caller context. |
| `0x82418630` | `FM2_HashName_InitSaltFieldA` | Evidence from decompile and caller context. |
| `0x82418650` | `FM2_HashName_InitSaltFieldB` | Evidence from decompile and caller context. |
| `0x82455100` | `FM2_Network_DispatchMessageQueueTail` | Evidence from decompile and caller context. |
| `0x824635e8` | `FM2_RaceGhost_BuildPlaybackSampleTableFinalize` | Evidence from decompile and caller context. |
| `0x824a76a8` | `FM2_RenderAdapter_DestroyChildClearThunk` | Evidence from decompile and caller context. |
| `0x8250f1c8` | `FM2_Presentation_CopyCarDisplayBlockSlotInitA` | Evidence from decompile and caller context. |
| `0x82510260` | `FM2_Presentation_CopyCarDisplayBlockSlotInitB` | Evidence from decompile and caller context. |
| `0x825104a8` | `FM2_PresentationSlotVector_Clear200ByteInnerA` | Evidence from decompile and caller context. |
| `0x82510910` | `FM2_PresentationSlotVector_Clear200ByteInnerB` | Evidence from decompile and caller context. |
| `0x82510ef8` | `FM2_Presentation_CopyCarDisplayBlockLinkNode` | Evidence from decompile and caller context. |
| `0x82511110` | `FM2_Presentation_CopyCarDisplayBlockSlotFinalize` | Evidence from decompile and caller context. |
| `0x82511170` | `FM2_PresentationSlotVector_Clear200ByteDtorChain` | Evidence from decompile and caller context. |
| `0x825145e8` | `FM2_CarPresentation_DtorReleaseFieldA` | Evidence from decompile and caller context. |
| `0x8251d540` | `FM2_CarPresentation_DtorReleaseFieldB` | Evidence from decompile and caller context. |
| `0x8251e270` | `FM2_CarPresentation_DtorClearOwnedLists` | Evidence from decompile and caller context. |
| `0x8251e410` | `FM2_Render_TestPassVisibilityVMXCore` | Evidence from decompile and caller context. |
| `0x82523020` | `FM2_Render_SortVisibleRenderablesPartitionTail` | Evidence from decompile and caller context. |
| `0x82526490` | `FM2_Render_SortVisibleRenderablesInitHeap` | Evidence from decompile and caller context. |
| `0x82526a88` | `FM2_Render_SortVisibleRenderablesInsertTail` | Evidence from decompile and caller context. |
| `0x82527c60` | `FM2_Render_SortVisibleRenderablesBodyTail` | Evidence from decompile and caller context. |
| `0x8252bbb8` | `FM2_Render_GetDistanceKeyFromPassSlotCore` | Evidence from decompile and caller context. |
| `0x8252d170` | `FM2_Render_UpdateObjectDistanceKeysTail` | Evidence from decompile and caller context. |
| `0x82587788` | `FM2_Network_DispatchMessageFromQueueLockedTail` | Evidence from decompile and caller context. |
| `0x8258cba0` | `FM2_Set_LowerBoundByKeyInTreeTail` | Evidence from decompile and caller context. |