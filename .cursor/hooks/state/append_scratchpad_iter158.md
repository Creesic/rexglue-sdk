## Iteration 158

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x827294D8 | sub_827294D8 | FM2_Render_DrawObjectPassToSurface | 0.91 | Resolves/creates render surface; lazy VS/PS GPU blocks `dword_82A41E64/68/6C`; binds shaders/textures; viewport setup; object draw emit. Called from `FM2_Render_SetupObjectDrawPassState`. |
| 0x82728CE0 | sub_82728CE0 | FM2_Render_ApplyPassShaderConstantsAndTextureBindings | 0.90 | Swaps TLS pass-state ptr; `FM2_Render_ApplyPrimitiveTypeGpuState`; uploads pass constants; binds textures via `FM2_Render_GetTextureResourceGpuOffset` + `FM2_D3D_ApplyGpuMemoryPatches`. |
| 0x82728980 | sub_82728980 | FM2_Render_ApplyPassSamplerBindings | 0.90 | Loads pass-state A/B; walks sampler binding table `dword_82A41D88`; invokes GPU patch callbacks; applies texture memory patches per slot. |
| 0x8272F650 | sub_8272F650 | FM2_Render_EmitIndexedTriangleFanDrawPm4 | 0.92 | Allocates PM4 buffer `21*a2` dwords; writes draw packets `0x2200/0x2208/0xC0003601`; per-vertex triangle-fan position/color data with 0.5 texel bias. |
| 0x8272F940 | sub_8272F940 | FM2_Render_EmitTiledAlignedTriangleFanDrawPm4 | 0.89 | Aligns dims to tile boundaries (40/8 byte grids); invokes `FM2_Render_EmitIndexedTriangleFanDrawPm4` up to 3 times when `dword_829A7FA0` set. |
| 0x8272F5C8 | sub_8272F5C8 | FM2_RenderResource_DecompressPackedTransportBlocks | 0.90 | Walks length-prefixed blocks; memcpy or `sub_8294FC70` decompress via `dword_82A42640`; used in XTS transport load path. |
| 0x8272AE80 | sub_8272AE80 | FM2_RenderResource_SetTextureGpuBaseOffset | 0.88 | Stores GPU base offset at +52 and resets texture type field +28 to 0. |
| 0x82721930 | sub_82721930 | FM2_Render_GetTlsMainContextPassIndex | 0.87 | `FM2_RenderTls_GetMainContextPtr` then returns dword at TLS context +348. |
| 0x82725C60 | sub_82725C60 | FM2_Math_CrossProduct3 | 0.91 | Standard 3-vector cross product into output vector; used by car/physics math callers. |
| 0x82725CC8 | sub_82725CC8 | FM2_Math_ComputeVectorDistanceSquared | 0.90 | Sum of squared deltas for 3D vectors `(a-b)·(a-b)`. |
| 0x82728438 | sub_82728438 | FM2_Render_LoadGlobalPassStatePtrB | 0.90 | Stores `dword_82A41D30` (pair of `FM2_RenderTls_SetGlobalPassStatePtrB`); used by pass sampler binding path. |
| 0x82728448 | sub_82728448 | FM2_Render_LoadPassSamplerBindingTablePtr | 0.88 | Stores `dword_829A77A0` (set by `sub_82728428`); indexes sampler binding table in pass apply paths. |

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
| 0x82722808 | sub_82722808 | Large PM4 opcode dispatch (~2KB); needs dedicated pass. |
| 0x82722FD8 | sub_82722FD8 | Large PM4 draw-list walker (~1.9KB); needs dedicated pass. |
| 0x82723B30 | sub_82723B30 | 8-byte wrapper → `ReleaseGpuResourceArray` only. |
| 0x82723C28 | sub_82723C28 | Thin dispatch to memcpy helpers only (24 bytes). |
| 0x82725D68 | sub_82725D68 | Zeros 3 floats only (24 bytes). |
| 0x82725EF0 | sub_82725EF0 | Float reciprocal only (16 bytes). |
| 0x82728428 | sub_82728428 | Stores `dword_829A77A0` only (12B); defer TLS setter cluster. |
| 0x8272A538 | sub_8272A538 | PM4 opcode sub-dispatch jump table (~648B); defer with `sub_82722808` cluster. |
| 0x8272FBA0 | sub_8272FBA0 | Large tiled blit orchestrator (~1.2KB); defer GPU emit cluster. |
| 0x827218D8 | sub_827218D8 | Writes dword to indexed array only (16B). |
| 0x827218E8 | sub_827218E8 | Writes dword to nested array only (16B). |
