## Iteration 163

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x82745DC8 | sub_82745DC8 | FM2_Render_ScaleSphericalHarmonicBasisChannels | 0.92 | Calls `ComputeSphericalHarmonicBasis`; scales `order²` coeffs by `pi/normalization` and three channel weights into up to three output buffers. |
| 0x827464A8 | sub_827464A8 | FM2_Render_ConvolveEnvironmentCubeMapSphericalHarmonics | 0.91 | Iterates cube-map faces/levels; `ComputeSphericalHarmonicBasis` + scale/add float buffers; GPU texture alloc; caller `render_car_pass_bind_environment_texture_samplers`. |
| 0x82745F00 | sub_82745F00 | FM2_Render_BuildSphericalHarmonicVisibilityMatrix | 0.89 | VMX matrix setup; calls `BuildSphericalHarmonicBasisMatrix`; used from `FM2_Render_TestPassVisibilityVMXBody`. |
| 0x82748E38 | sub_82748E38 | FM2_AtiSSM_EmitShaderMicrocodeFromToken | 0.90 | Recursively walks compiled shader tokens; encodes ALU ops via `unk_82000D10` into `byte_8200E4CC`; callee of `WalkCompiledShaderTokens`. |
| 0x82749BF0 | sub_82749BF0 | FM2_AtiSSM_GetArrayStateCompiledFloat | 0.92 | Array-state bounds checks (`Internal_AS.h:670`); returns compiled float via `GetCompiledStateFloat`. |
| 0x82749D68 | sub_82749D68 | FM2_DeferredTaskQueue_ResetBumpRegionOnRewind | 0.90 | When rewind ptr hits region start, `memset` remainder and resets bump cursor/stats; used by deferred-task emitters. |
| 0x82752490 | sub_82752490 | FM2_AtiSSM_IRInstBindArgumentOperand | 0.91 | Binds IR operand pointers into instruction arg slots (`irinst.cpp:251`, `arg < 6` assert); 29 xrefs in shader compiler. |
| 0x8274E858 | sub_8274E858 | FM2_DeferredTaskQueue_ReallocBufferChunk | 0.91 | Reallocates queue buffer (min 12248 bytes); resets head/tail pointers; caller `BumpAllocAligned`. |
| 0x827499C0 | sub_827499C0 | FM2_AtiSSM_MemPoolInitFreelistChunk | 0.90 | Allocates aligned freelist chunk and chains nodes; called when `MemPoolAllocFromFreelist` empty. |
| 0x827495A8 | sub_827495A8 | FM2_AtiSSM_LinkedListInsertBefore | 0.91 | Splices new node before `pNextItem` (`linkedlist.cpp:163-168`); used by `LinkedListInsertItem`. |
| 0x82749DE8 | sub_82749DE8 | FM2_AtiSSM_AppendShaderDumpLogLine | 0.90 | Appends `"@@ "` prefixed `vsnprintf` line to 8KB RSLOG buffer (`compiler.cpp:1585`). |
| 0x82749E98 | sub_82749E98 | FM2_AtiSSM_ShaderDumpLogVprintf | 0.88 | Varargs wrapper forwarding to `AppendShaderDumpLogLine`. |

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
| 0x827218D8 | sub_827218D8 | Writes dword to indexed array only (16B). |
| 0x827218E8 | sub_827218E8 | Writes dword to nested array only (16B). |
| 0x8270DA40 | sub_8270DA40 | SQLite correlation walker; defer expr cluster. |
| 0x82718B28 | sub_82718B28 | SQLite parse reduce helper; defer codegen cluster. |
| 0x82747778 | sub_82747778 | SH kernel dispatch jump table; needs paired `sub_827477AC` analysis. |
| 0x827477AC | sub_827477AC | Returns constant only (12B); insufficient alone. |
| 0x8274EA38 | sub_8274EA38 | Large XGRAPHICS CFG IR walker (~1.4KB); defer compiler cluster. |
