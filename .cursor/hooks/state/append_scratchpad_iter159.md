## Iteration 159

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82722808 | sub_82722808 | FM2_Render_DispatchPm4DrawOpcode | 0.93 | PM4 packet walker: reads opcode byte at +2 minus 1; 57-entry jump table (`cmplwi 0x39`, `lhzx`/`bctr`); handlers call `DrawIndexedPrimitive`, pass shader/sampler/matrix upload, bone matrices; unknown opcodes advance by packet length. |
| 0x82722FD8 | sub_82722FD8 | FM2_Render_WalkAndDispatchPm4DrawList | 0.92 | Called from `FM2_Render_ExecuteBoundDrawPass`; loads PM4 list at context+236; `ApplyRenderStateCallbackBlock`; prefetch loop with same 57-opcode jump dispatch as `FM2_Render_DispatchPm4DrawOpcode`. |
| 0x8272A538 | sub_8272A538 | FM2_Render_DispatchObjectPassPm4Opcode | 0.90 | 23-entry opcode jump table (`a4-1 <= 0x16`, `bctr`); handlers invoke `SetupObjectDrawPassState`, `DrawObjectPassToSurface`, `ObjectPassPrefetchDrawBatch`, GPU pass alloc. |
| 0x827299B0 | sub_827299B0 | FM2_Render_DispatchBlurLightingPm4Opcode | 0.87 | 23-entry PM4 opcode jump table; handlers use `FM2_Math_ComputeGaussianKernelWeight` and `FM2_Render_AdvancePassLightingCycleIndexTls`; paired with object-pass dispatch in `sub_8250C120`. |
| 0x8272FBA0 | sub_8272FBA0 | FM2_Render_BlitTiledRegionTriangleFanPm4 | 0.90 | Tile-aligned region blit; calls `FM2_Render_EmitTiledAlignedTriangleFanDrawPm4`, `FM2_D3D_EmitSurfaceResolvePackets`, `D3D_SubmitAndDrainCommands`. |
| 0x82730068 | sub_82730068 | FM2_Render_BlitClampedRegionToSurface | 0.89 | Clamps blit rect to surface bounds (+12368/+12372 dims); scissor/dirty-bit packets; invokes `FM2_Render_BlitTiledRegionTriangleFanPm4`; called from constant-buffer upload path. |
| 0x82723D70 | sub_82723D70 | FM2_Render_TransformPassMatrixIntoTlsContexts | 0.88 | VMX matrix multiply from draw-packet batch base into TLS worker context; then `FM2_Render_PrepareSceneSliceTransforms_0`; called from `FM2_RenderTls_BindPassStateToContext`. |
| 0x8272AD98 | sub_8272AD98 | FM2_Render_ApplyTextureGpuPatchesByType | 0.89 | Branches on texture type field at +28; types 0-3 and 5 patch via `FM2_D3D_ApplyGpuMemoryPatches` with +32 offset; type 4 uses alternate patch path. |
| 0x82725C28 | sub_82725C28 | FM2_Math_AddVector3 | 0.92 | Component-wise float add of two 3-vectors into output (`fadds` on X/Y/Z). |
| 0x82725D68 | sub_82725D68 | FM2_Math_ZeroVector3 | 0.91 | Stores 0.0 to three floats; used by shader scratch/sampler init paths (`InitPassScratchBufferFromShaderPool`, etc.). |
| 0x82725D38 | sub_82725D38 | FM2_Math_CopyVector3 | 0.91 | Copies three dwords (12-byte vector) src→dst. |
| 0x82728428 | sub_82728428 | FM2_RenderTls_SetPassSamplerBindingTablePtr | 0.90 | Stores arg to `dword_829A77A0`; pair of `FM2_Render_LoadPassSamplerBindingTablePtr`. |

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x826F0E48 | sub_826F0E48 | Clears single flag bit at +8 only; too trivial alone. |
| 0x826F14A8 | sub_826F14A8 | Sets FK action byte on last constraint only (44 bytes); too thin alone. |
| 0x826F2628 | sub_826F2628 | Sets single byte on nested parse object; too thin alone. |
| 0x826F6300 | sub_826F6300 | Thin pager callback setter; callee unknown. |
| 0x826F6360 | sub_826F6360 | Thin wrapper → `Pager_SetJournalModeFlags` only. |
| 0x826F6390 | sub_826F6390 | Thin wrapper (12 bytes). |
| 0x826F6440 | sub_826F6440 | Too trivial alone. |
| 0x826F6458 | sub_826F6458 | Too trivial alone. |
| 0x826F72F0 | sub_826F72F0 | Thin wrapper only. |
| 0x826F7310 | sub_826F7310 | Thin wrapper only. |
| 0x826FA1D0 | sub_826FA1D0 | Reads single byte from mem cell; too trivial alone. |
| 0x826FABB0 | sub_826FABB0 | Thin wrapper (12 bytes). |
| 0x826FABC0 | sub_826FABC0 | Thin wrapper (12 bytes). |
| 0x826FABD0 | sub_826FABD0 | Thin wrapper (12 bytes). |
| 0x826FAC88 | sub_826FAC88 | Single-line wrapper; too thin alone. |
| 0x826FE880 | sub_826FE880 | Too trivial alone. |
| 0x826FE898 | sub_826FE898 | Too trivial alone. |
| 0x826EB8E8 | sub_826EB8E8 | 8-byte db+32 reader; too trivial alone. |
| 0x826EC510 | sub_826EC510 | 12-byte Mem_SetRowid wrapper; too thin alone. |
| 0x82707100 | j_FM2_SQLite_ReallocOrAllocZeroed | Jump thunk only (4 bytes). |
| 0x827072F8 | j_FM2_SQLite_LookupKeywordToken | Jump thunk only (4 bytes). |
| 0x82713520 | sub_82713520 | Pager page-size field accessor; too trivial alone. |
| 0x82716080 | sub_82716080 | Zeros 3 dwords only; too trivial alone. |
| 0x82723B30 | sub_82723B30 | 8-byte wrapper → `ReleaseGpuResourceArray` only. |
| 0x82723C28 | sub_82723C28 | Thin dispatch to memcpy helpers only (24 bytes). |
| 0x82725EF0 | sub_82725EF0 | Float reciprocal only (16 bytes). |
| 0x827218D8 | sub_827218D8 | Writes dword to indexed array only (16B). |
| 0x827218E8 | sub_827218E8 | Writes dword to nested array only (16B). |
| 0x8272A968 | sub_8272A968 | 12-byte global store only. |
| 0x8272B918 | sub_8272B918 | 12-byte global store only. |
| 0x8272AB00 | sub_8272AB00 | 44-byte PM4 handler stub; defer inner opcode cluster. |
| 0x8272AB30 | sub_8272AB30 | 44-byte PM4 handler stub; defer inner opcode cluster. |
| 0x8272AB60 | sub_8272AB60 | 44-byte PM4 handler stub; defer inner opcode cluster. |
