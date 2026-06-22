## Infrastructure pass 71 (2026-06-18)

Small helper and accessor pass (high-confidence naming by direct decompile + caller context).

| Address | New name | Evidence |
| --- | --- | --- |
| 0x8222E7B8 | FM2_RaceGhost_GetRarityDescriptorName | Called by FM2_RaceGhost_QueryRarityByOrdinal; decomp returns table pointer and embedded string from rarity-global registry when index is in range, otherwise fallback constant string. |
| 0x8229BA30 | FM2_AudioSample_BuildOutputPairDescriptorFieldBody | Called by FM2_AudioSample_BuildOutputPairDescriptorField08/20; function builds output pair descriptor, validates with sub_82454290, finalizes with sub_8229B6A8, and returns status byte. |
| 0x8230B038 | FM2_CReplayStats_Ctor | Called by FM2_LuaGarage_EnsureCarRecordLookupBody; decomp initializes Forza2::CReplayStats vtable and zeroes key members, matching constructor behavior. |
| 0x8236A2B8 | FM2_D3DResource_UnlockForRelease | Called from FM2_D3DTexture_ReleaseResourceContext and FM2_D3D_ReleaseResourceSlot; decomp calls D3D::UnlockResource using addresses derived from a1 + 24. |
| 0x8237DF50 | FM2_D3DTexture_InitDefaultDescriptorCopy | FM2_D3D_CreateTextureFromSurfaceLevelBodyC calls this helper; callee performs FM2_MemcpyAligned from unk_82024690 into output with fixed 304-byte copy. |
| 0x8240E3B0 | FM2_D3DTexture_InitSurfaceDesc | FM2_D3D_CreateTextureFromSurfaceLevelBody passes unpacked dimensions/steps and this helper stores them into four dwords of a local descriptor structure and returns success. |
| 0x824EF788 | FM2_RaceGhost_GetInterpolationModeByte | Used by FM2_RaceGhost_ComputePlaybackInterpolationWeight; decomp returns *(u8*)(a1+100) which is consumed as interpolation mode in caller math. |
| 0x824FB0E8 | FM2_DeferredTask_InitPreloadAnimRecord | FM2_DeferredTask_SubmitPreloadingAnimTurnOn repeatedly initializes same struct layout via this helper (field0, field4, and byte8 cleared). |
| 0x824FD258 | FM2_RaceGhost_GetRarityByte | FM2_RaceGhost_QueryRarityByOrdinal feeds this accessor (*(u8*)(a1+8)) into rarity SQL-query string construction. |
| 0x824FD260 | FM2_AsyncQueue_HasPendingOp | FM2_AsyncQueue_FindPendingOp uses this direct boolean check (*a1 != 0) before queue operations. |
| 0x82503688 | FM2_PresentationCar_DtorBody | Invoked by FM2_PresentationCar_Dtor; sets base vtable and delegates to FM2_Object_AssignBaseVtable_82000E18. |
| 0x825A3B58 | FM2_Render_HasPassFlag | FM2_Render_CompileMissingPassBuffers checks (a2 == (*a1 & a2)) through this helper, so it is a pass-flag test predicate. |
| 0x825AD5E8 | FM2_Render_GetPassConstantDesc | Called by FM2_RenderPass_BindSurfaceAndConstants; returns indexed unk_829A2BAC entry for current pass and passes it into bind call as constant-table value. |
| 0x825B36A0 | FM2_AudioRenderFrame_StoreFenceAndDrain | FM2_AudioRenderFrame_PathB forwards *(a1+2136) into this helper, which calls FM2_GpuKick_NotifyPixCapture_StoreFenceAndDrain. |
| 0x825DB058 | FM2_LuaGarage_HasProfileManagerHeap | FM2_LuaGarage_EnsureCarRecordField92Body uses this check to gate manager-heap access ((a1+4) != 0). |
| 0x82615A50 | FM2_ComObject_DeleteOptionalBody | FM2_ComObject_DeleteOptional invokes this tiny init step that writes off_820423C0 into object vtable slot. |
| 0x8264EAF0 | FM2_CarDynamics_InitScalarPair | FM2_CarDynamics_InitSubsystems repeatedly calls this reset helper; it writes 0.0f and 0 into object scalar/payload fields. |
| 0x8264ECD0 | FM2_CarDynamics_InvokeUpdateHook | FM2_CarDynamics_UpdateSimulationStep dispatches this helper; decomp calls virtual slot +60 on a subobject at a1 + 24. |
