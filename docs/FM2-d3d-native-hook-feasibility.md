# FM2 native-renderer feasibility: are FM2's D3D functions the same as Lost Odyssey's?

Verification of whether the ReOdyssey (Lost Odyssey) native-renderer hook approach
can be applied to FM2 — i.e. whether FM2's Direct3D functions are *actually* the
same as LO's and therefore hookable the same way.

**Date:** 2026-06-29
**Method:** structural fingerprint comparison + decompile spot-checks across the
IDA databases for both titles.

- LO database: `D:\Emulation\Games_Xbox_360\LostOdyssey\LostOdyssey.xex.i64` (MCP `ida40`)
- FM2 database: `D:\Emulation\Games_Xbox_360\Forza2\FM2.xex.i64` (MCP `ida38`)
- Cross-reference: `fm2graine.xex.i64` (MCP `ida39`) — same binary as ida38, FLIRT-named

## Verdict

**Yes — for the shared core D3D API, FM2's functions are genuinely the same as
LO's, and ReOdyssey's hooks transfer directly.** Two caveats: ~20 of ReOdyssey's
hooks are UE3-only (no FM2 equivalent), and ~8 essential entry points exist in FM2
but are not named yet (must be identified before they can be hooked).

## Why they're the same

Both games **statically link the same Microsoft Xbox 360 XDK Direct3D9 library**.
The `D3DDevice_*` / `D3D*` / `XG*` functions are Microsoft's runtime, compiled into
each title's XEX — not game code. So they are expected to be identical (modulo XDK
version / compiler differences). This is the same reason ReOdyssey can hook them:
there is concrete recompiled D3D code at each address.

For background on the ReOdyssey hook mechanism itself (REX_HOOK replaces the
recompiled guest symbol with a native plume implementation keyed on the function
**signature/ABI**, not the body), see the companion doc
`ReOdyssey-native-renderer.md` (DOAX workspace docs). The ABI-keyed nature is why
even functions whose bodies differ slightly still hook cleanly: only the signature
must match, and it does (same XDK).

## Evidence 1 — structural fingerprints (size / #instructions / #calls)

Comparison of every D3D function named in **both** databases. `sz` = byte size,
`ins` = instruction count, `call` = call-instruction count.

| Function | FM2 (sz/ins/call) | LO (sz/ins/call) | Result |
|---|---|---|---|
| D3DDevice_CreateVertexBuffer | 200/50/4 | 200/50/4 | ✅ identical |
| D3DDevice_CreateIndexBuffer | 172/43/4 | 172/43/4 | ✅ identical |
| D3DDevice_CreateTexture | 288/72/7 | 288/72/7 | ✅ identical |
| D3DDevice_CreateVertexShader | 240/60/9 | 240/60/9 | ✅ identical |
| D3DDevice_CreatePixelShader | 272/68/7 | 272/68/7 | ✅ identical |
| D3DDevice_SetVertexShader | 460/115/3 | 460/115/3 | ✅ identical |
| D3DDevice_SetPixelShader | 444/111/3 | 444/111/3 | ✅ identical |
| D3DDevice_SetVertexShaderConstantB | 92/23/0 | 92/23/0 | ✅ identical |
| D3DDevice_SetPixelShaderConstantB | 92/23/0 | 92/23/0 | ✅ identical |
| D3DDevice_SetVertexShaderConstantI | 84/21/0 | 84/21/0 | ✅ identical |
| D3DDevice_SetPixelShaderConstantI | 84/21/0 | 84/21/0 | ✅ identical |
| D3DDevice_SetStreamSource | 284/71/2 | 284/71/2 | ✅ identical |
| D3DDevice_SetIndices | 144/36/2 | 144/36/2 | ✅ identical |
| D3DDevice_SetViewport | 124/31/1 | 124/31/1 | ✅ identical |
| D3DDevice_SetScissorRect | 252/63/1 | 252/63/1 | ✅ identical |
| D3DDevice_ClearF | 292/73/1 | 292/73/1 | ✅ identical |
| D3DDevice_SetRenderState_AlphaBlendEnable | 128/32/0 | 128/32/0 | ✅ identical |
| D3DDevice_SetRenderState_AlphaTestEnable | 36/9/0 | 36/9/0 | ✅ identical |
| D3DDevice_SetRenderState_BlendOp | 124/31/0 | 124/31/0 | ✅ identical |
| D3DDevice_SetRenderState_ColorWriteEnable | 56/14/0 | 56/14/0 | ✅ identical |
| D3DDevice_SetRenderState_DepthBias | 152/38/0 | 152/38/0 | ✅ identical |
| D3DDevice_SetRenderState_DestBlend | 124/31/0 | 124/31/0 | ✅ identical |
| D3DDevice_SetRenderState_DestBlendAlpha | 92/23/0 | 92/23/0 | ✅ identical |
| D3DDevice_SetRenderState_SrcBlend | 124/31/0 | 124/31/0 | ✅ identical |
| D3DDevice_SetRenderState_SrcBlendAlpha | 92/23/0 | 92/23/0 | ✅ identical |
| D3DDevice_SetRenderState_ZEnable | 56/14/0 | 56/14/0 | ✅ identical |
| D3DDevice_SetRenderState_ViewportEnable | 64/16/0 | 64/16/0 | ✅ identical |
| D3DVertexBuffer_Lock | 80/20/1 | 80/20/1 | ✅ identical |
| D3DIndexBuffer_Lock | 72/18/1 | 72/18/1 | ✅ identical |
| D3DSurface_LockRect | 32/8/0 | 32/8/0 | ✅ identical |
| D3DDevice_SetTexture | 376/94/2 | 380/95/2 | ✅ near-identical |
| D3DDevice_SetDepthStencilSurface | 748/187/1 | 728/182/1 | ✅ near-identical |
| D3DDevice_BeginVertices | 1176/294/18 | 1184/296/18 | ✅ near-identical |
| D3DDevice_BlockUntilIdle | 116/29/2 | 120/30/2 | ✅ near-identical |
| D3DDevice_SetPredication | 388/97/1 | 364/91/1 | ✅ near-identical |
| D3DDevice_SetVertexShaderConstantFN | 264/66/1 | 232/58/0 | ✅ equivalent (decompile-verified) |
| D3DDevice_SetPixelShaderConstantFN | 264/66/1 | 232/58/0 | ✅ equivalent |
| D3DDevice_CreateVertexDeclaration | 224/56/4 | 132/33/2 | ✅ equivalent (FM2 inlines XGSetVertexDeclaration) |
| D3DDevice_CreateSurface | 296/74/6 | 356/89/8 | ⚠️ differs (XDK version; same role) |
| D3DDevice_SetShaderGPRAllocation | 460/115/3 | 232/58/3 | ⚠️ differs — irrelevant (ReOdyssey no-ops it) |

~33 identical, ~5 near-identical, ~3 equivalent-on-inspection, 2 differing (one
irrelevant). No behavioral conflicts found.

## Evidence 2 — decompile spot-checks on the divergent functions

**SetVertexShaderConstantFN — identical.** Same signature
`(device, startRegister, pConstantData, count, pendingMask)`, same destination
offset (`2·startRegister + const`), same `lvlx/lvrx/stvx` vector-copy loop
(4-at-a-time unrolled + remainder), same `m_Pending.m_Mask[0] |= HIDWORD(mask)`.
The fingerprint's "extra call" was a `dcbt` prefetch artifact, not a real call.
(FM2 decompiles with the typed `m_Constants.Fetch[26].Vertex[...]` because ida38
has the `D3DDevice` struct; LO shows the raw `result[2*a2+240]` — same byte offset.)

**CreateVertexDeclaration — functionally equivalent.** Both walk the
`D3DVERTEXELEMENT9[]` array to the `Stream == 0xFF` terminator, allocate
`12·count + 56` bytes with the **same GPU allocation tag `612368384`**, and build a
`D3DVertexDeclaration`. FM2's version is larger only because it **inlines** the
declaration-populate step that LO performs via a call to `XGSetVertexDeclaration`.
Same input format, same output structure → ReOdyssey's hook (which only reads the
input element array) works for both.

## Caveat 1 — ~20 ReOdyssey hooks are UE3-only (no FM2 equivalent)

Lost Odyssey is Unreal Engine 3; FM2 is Turn10's in-house engine. These ReOdyssey
hooks **do not exist in FM2** and must not be ported:

`RHISetDepthState`, `RHISetStencilState`, `RHICreateBoundShaderState`,
`RHIDrawIndexedPrimitiveUP`, `FXeVertexShader_Init`, `FXePixelShader_Init`, and the
`rex_LO_*` bound-shader-state / vertex-declaration-bind glue
(`CreateBoundShaderStateResource`, `SetBoundShaderState`, `SetVertexDeclarationBind`,
`ReleaseBoundShaderStateRef`, the `FBoundShaderStateRHIRef` ctor/assign/release).

FM2 instead calls the raw `D3DDevice_*` API directly (no RHI wrapper) — which is
*simpler* to hook, but means FM2's state/draw flow goes through the core D3D
functions, not a wrapper layer.

## Caveat 2 — UPDATE 2026-07-01: these were not unnamed, they were misnamed

**Correction — see `docs/FM2-ida-renames-2026-07-01.md` for full evidence.** These
functions are not missing/unnamed: the compiler **inlined** the XDK primitive
directly into FM2's PM4-emission call sites, and those call sites were already
named under purpose-based `FM2_*` names in earlier sessions that didn't recognize
them as the XDK primitives. Confirmed via line-for-line decompile diff against LO
for 6 of 8. This also means the "hook one shared symbol" premise this doc opened
with does **not** hold for FM2's indexed-draw path — see that doc's "Implication
for the native-renderer-hook plan" section.

| Function | LO size | FM2 status |
|---|---|---|
| D3DDevice_DrawVertices | 1000 | ✅ found+renamed 2026-07-01 (was `FM2_D3D_EmitIndexedDrawPm4Packets`, 0x827313b0) |
| D3DDevice_DrawIndexedVertices | 1112 | ✅ found+renamed 2026-07-01 (was `FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset`, 0x827317a0) |
| D3DDevice_DrawVerticesUP | 72 | ✅ found+renamed 2026-07-01 (was `FM2_Render_AllocSurfaceAndMemcpyPixels`, 0x82730d60) |
| D3DDevice_Swap | 1816 | ✅ found+renamed 2026-07-01 (was `FM2_GpuCommandBuffer_BuildAndSubmit`, 0x8236cb28) |
| D3DDevice_Resolve | 3656 | ✅ found+renamed 2026-07-01 (was `FM2_AudioMix_SubmitPendingOutputBody`, 0x8237d158) |
| D3DDevice_SetRenderState_ClipPlaneEnable | 64 | ❌ still not located (too common a size to disambiguate) |
| D3DBaseTexture_LockTail | 192 | ❌ still not located (too common a size to disambiguate) |
| XGSetVertexDeclaration | 236 | confirmed inlined into `D3DDevice_CreateVertexDeclaration`; no separate FM2 symbol expected |

A second inlined copy of DrawIndexedVertices with fused vertex-format setup was
also found and renamed `D3DDevice_DrawIndexedVertices_WithVertexFormatSetup`
(0x82731c00), plus a Resolve helper `D3DBaseTexture_FindSurfaceWithinTexture`
(was `FM2_AudioRender_SampleFrontBufferRegionBody`, 0x8236b010).

Note: FM2's *named* draw functions (e.g. `FM2_Render_EmitIndexedTriangleFanDrawPm4`)
are higher-level engine wrappers, **not** these D3D primitives. The primitives sit
below them — as of this update, mostly identified, but each exists as 2-3 inlined
copies rather than one shared function, which changes the hook strategy (see
above).

## Bottom line

| Bucket | Count (approx) | Action |
|---|---|---|
| Shared core D3D — verified same behavior | ~40 named (+ more unnamed) | Hook directly, ReOdyssey impls port over |
| UE3-only (RHI/FXe/BoundShaderState) | ~20 | N/A — do not port; FM2 hooks core D3D directly |
| Draw/Swap/Resolve primitives | 6 of 8 found+renamed 2026-07-01 (compiler-inlined, not missing) | Hook boundary must move up a layer — see below |

"Can we just hook them?" — **for the resource / state / constant / lock API, yes,
confidently.** For the Draw/Swap/Resolve primitives, **no** — see "Implication for
the native-renderer-hook plan" above: they exist as multiple compiler-inlined
copies per primitive rather than one shared symbol, so a ReOdyssey-style single
`REX_HOOK` swap doesn't have a clean target. The hook boundary needs to be the
`FM2_RenderContext_*`/command-buffer layer instead (per `AGENTS.md`'s Learned
Workspace Facts).

## Next step

~~Locate the 8 unnamed FM2 functions~~ — done for 6/8, see
`docs/FM2-ida-renames-2026-07-01.md`. Remaining work:
1. Locate `D3DDevice_SetRenderState_ClipPlaneEnable` and `D3DBaseTexture_LockTail`
   (low priority — neither is on the Draw/Swap/Resolve critical path).
2. Design the actual hook points at the `FM2_RenderContext_*`/PM4-emission layer
   now that the inlining is confirmed, rather than continuing to assume a single
   `D3DDevice_Draw*` symbol per primitive.
