### Infrastructure pass 79 (33 functions)

Livery/car audio, Lua call-depth overflow, profile/input, render object-pass, presentation.

| Address | New name | Reasoning |
| --- | --- | --- |
| `0x82432688` | `FM2_LiveryMask_ParseAndLoadEntryValidate` | Evidence from decompile and caller context. |
| `0x82494740` | `FM2_AIDriver_ResetRaceLineOnSectorChangeFinalizeBody` | Evidence from decompile and caller context. |
| `0x8249a868` | `FM2_CarAudio_DtorReleaseBindingA` | Evidence from decompile and caller context. |
| `0x8249a8d0` | `FM2_CarAudio_DtorReleaseBindingB` | Evidence from decompile and caller context. |
| `0x824a5998` | `FM2_LiveryMask_ParseAndLoadEntryParseLayer` | Evidence from decompile and caller context. |
| `0x824b80e8` | `FM2_Lua_IncrementCallDepthOrOverflowCheck` | Evidence from decompile and caller context. |
| `0x824bc5c8` | `FM2_Lua_IncrementCallDepthOrOverflowGrowStack` | Evidence from decompile and caller context. |
| `0x824bcd78` | `FM2_Lua_IncrementCallDepthOrOverflowDispatchA` | Evidence from decompile and caller context. |
| `0x824bce28` | `FM2_Lua_IncrementCallDepthOrOverflowDispatchB` | Evidence from decompile and caller context. |
| `0x824bcec8` | `FM2_Lua_IncrementCallDepthOrOverflowDispatchC` | Evidence from decompile and caller context. |
| `0x824bcf70` | `FM2_Lua_IncrementCallDepthOrOverflowDispatchD` | Evidence from decompile and caller context. |
| `0x824bd228` | `FM2_Lua_IncrementCallDepthOrOverflowDispatchE` | Evidence from decompile and caller context. |
| `0x824bedc8` | `FM2_Lua_IncrementCallDepthOrOverflowErrorHandler` | Evidence from decompile and caller context. |
| `0x824befa8` | `FM2_Lua_IncrementCallDepthOrOverflowGuard` | Evidence from decompile and caller context. |
| `0x824bf820` | `FM2_Lua_IncrementCallDepthOrOverflowCleanup` | Evidence from decompile and caller context. |
| `0x824d0ed8` | `FM2_Input_InitControllerDevicesParseBindingField` | Evidence from decompile and caller context. |
| `0x824d3e58` | `FM2_Lua_PushSslUnitStringsTableAppendA` | Evidence from decompile and caller context. |
| `0x824d3f00` | `FM2_Lua_PushSslUnitStringsTableAppendB` | Evidence from decompile and caller context. |
| `0x824d4608` | `FM2_Math_AllocForceVectorComPtrInitA` | Evidence from decompile and caller context. |
| `0x824d4628` | `FM2_Math_AllocForceVectorComPtrInitB` | Evidence from decompile and caller context. |
| `0x824db218` | `FM2_Profile_ParseUnsignedFromSubStringValidateDigit` | Evidence from decompile and caller context. |
| `0x824dcec0` | `FM2_Profile_ParseUnsignedFromSubStringValidateRange` | Evidence from decompile and caller context. |
| `0x824f26d8` | `FM2_SystemEventSubscriber_CtorFields` | Evidence from decompile and caller context. |
| `0x82502268` | `FM2_D3D_LazyInitPresentChainInit` | Evidence from decompile and caller context. |
| `0x82505d20` | `FM2_Audio_MLPMatrix_FormatErrorMessageBody` | Evidence from decompile and caller context. |
| `0x825065c8` | `FM2_Render_FramePipelineSubmitPassABody` | Evidence from decompile and caller context. |
| `0x82510a88` | `FM2_Memory_AllocTaggedSmallBlockFromPoolEntryBody` | Evidence from decompile and caller context. |
| `0x82510d20` | `FM2_PresentationSlotVector_Clear200ByteBody` | Evidence from decompile and caller context. |
| `0x82510e28` | `FM2_Render_BuildObjectPassCommandBufferInitA` | Evidence from decompile and caller context. |
| `0x82511038` | `FM2_Render_BuildObjectPassCommandBufferInitB` | Evidence from decompile and caller context. |
| `0x825110b0` | `FM2_Render_AppendObjectPassDrawEntryBody` | Evidence from decompile and caller context. |
| `0x825117a0` | `FM2_Presentation_CopyCarDisplayBlockToSlotBody` | Evidence from decompile and caller context. |
| `0x82511828` | `FM2_PresentationCarConfig_DeleteOptionalBodyA` | Evidence from decompile and caller context. |