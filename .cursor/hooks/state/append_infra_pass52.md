### Infrastructure pass 52 (33 functions)

Render view-traversal / object-pass draw cluster: visibility VMX, pass env constants, draw-list submit.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x825d62e0` | `FM2_Render_ViewTraversalGetDrawListEntryPtr` | Evidence from decompile and caller context. |
| `0x82724200` | `FM2_Render_CopyPassLightingStateBlock` | Evidence from decompile and caller context. |
| `0x82513108` | `FM2_Render_AllocObjectPassDrawSlotLocked` | Evidence from decompile and caller context. |
| `0x82516d08` | `FM2_Render_ApplyPassEnvironmentIfDeferred` | Evidence from decompile and caller context. |
| `0x8251b520` | `FM2_Render_TestPassOcclusionBounds` | Evidence from decompile and caller context. |
| `0x8251d030` | `FM2_Render_UpdateDrawCullFlagsIfVisible` | Evidence from decompile and caller context. |
| `0x8251ff88` | `FM2_Render_TestPassVisibilityVMX` | Evidence from decompile and caller context. |
| `0x825a39a8` | `FM2_Presentation_InitCarSlotTransformZeros` | Evidence from decompile and caller context. |
| `0x8251b4f0` | `FM2_Render_ClearPassDrawOverride` | Evidence from decompile and caller context. |
| `0x8251bbb0` | `FM2_Render_HasPassDrawListForSortKey` | Evidence from decompile and caller context. |
| `0x8251b1a0` | `FM2_Render_ComputePassDrawOverrideVMX` | Evidence from decompile and caller context. |
| `0x82761080` | `FM2_Render_GetPassEnvConstantSlotA` | Evidence from decompile and caller context. |
| `0x827610c0` | `FM2_Render_GetPassEnvConstantSlotB` | Evidence from decompile and caller context. |
| `0x82761100` | `FM2_Render_SetPassEnvConstantSlotA` | Evidence from decompile and caller context. |
| `0x82761120` | `FM2_Render_SetPassEnvConstantSlotB` | Evidence from decompile and caller context. |
| `0x82512f40` | `FM2_Render_GrowObjectPassDrawVector` | Evidence from decompile and caller context. |
| `0x8252d060` | `FM2_Render_SortVisibleRenderablesThunk` | Evidence from decompile and caller context. |
| `0x8252dba0` | `FM2_Render_GetDistanceKeyFromPassSlot` | Evidence from decompile and caller context. |
| `0x825276b0` | `FM2_Render_ExecuteSortedDrawListsPassA` | Evidence from decompile and caller context. |
| `0x82527878` | `FM2_Render_ExecuteSortedDrawListsPassB` | Evidence from decompile and caller context. |
| `0x8252ac00` | `FM2_Render_CompilePassIfStaleLocked` | Evidence from decompile and caller context. |
| `0x82559df0` | `FM2_Render_ObjectPassDrawSetupMaterialSlot` | Evidence from decompile and caller context. |
| `0x8255b828` | `FM2_Render_SubmitSortedObjectDrawListVMX` | Evidence from decompile and caller context. |
| `0x8255d798` | `FM2_Render_ObjectPassShouldDrawVisible` | Evidence from decompile and caller context. |
| `0x8255fa28` | `FM2_Render_ObjectPassEmitDrawIfVisible` | Evidence from decompile and caller context. |
| `0x8272d5a8` | `FM2_Render_TestFrustumOcclusionVMX` | Evidence from decompile and caller context. |
| `0x82564588` | `FM2_Render_AssignResourceLockFromPassData` | Evidence from decompile and caller context. |
| `0x825094c0` | `FM2_Render_NotifyGlobalManagerStateChange` | Evidence from decompile and caller context. |
| `0x82522418` | `FM2_Render_UploadDrawListMatrixConstants` | Evidence from decompile and caller context. |
| `0x82528750` | `FM2_Stl_Vector_EraseRangeAtCopy` | Evidence from decompile and caller context. |
| `0x82538990` | `FM2_Render_InstanceHybridDrawPathSort` | Evidence from decompile and caller context. |
| `0x82539398` | `FM2_Render_InstancePathWrapperInner` | Evidence from decompile and caller context. |
| `0x8250f708` | `FM2_Render_IsPassCompileResourceReady` | Evidence from decompile and caller context. |