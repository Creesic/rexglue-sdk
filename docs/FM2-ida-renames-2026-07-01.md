# FM2 IDA renames — 2026-07-01 (D3D native-hook: locating the "8 unnamed" primitives)

Continuation of `docs/FM2-d3d-native-hook-feasibility.md` (2026-06-29). That doc's
"Next step" was to locate 8 XDK D3D functions present in FM2's binary but not yet
named, matching them against Lost Odyssey's confirmed structural fingerprints.

**Corrected finding: these functions are not unnamed — they are compiler-inlined**
into FM2's engine at their call sites, and those call sites were already named in
earlier sessions (June 18/22/26 passes) under purpose-based `FM2_*` names that
didn't reveal their underlying XDK identity. This also corrects the "Practical
interpretation" paragraph at the end of the June 29 section in
`docs/FM2-ida-toml-function-notes.md` — the functions it names
(`FM2_D3D_EmitIndexedDrawPm4Packets*`, `FM2_GpuCommandBuffer_BuildAndSubmit`) are
not merely *adjacent to* the D3DDevice_Draw*/Swap primitives, they **are** those
primitives.

IDA dbs: FM2 = `default.xex.i64` / `Forza2\FM2.xex.i64` (MCP `ida37`), Lost Odyssey
= `LostOdyssey.xex.i64` (MCP `ida38`). Do NOT `idb_save`.

## Method

For each LO reference function (`rex_D3DDevice_*`, already named by ReOdyssey),
found the FM2 call site via caller-tracing from already-named neighbors (e.g. the
sole caller of the `VdSwap` import, the sole caller of the resolve-parameter
helpers `D3D::SetTileAndDepthClear`/`D3D::SetColorClear`), then confirmed identity
with a line-for-line decompile diff against the LO original (same
`SetPending_AluConstants/Shaders/Predicated/RenderStates/FetchConstants` call
sequence, same PM4 packet opcode/constant layout, same ring-buffer drain checks).
Exact-size search alone was not reliable — most candidate sizes (64/72/192/236B)
collide with hundreds of unrelated functions; the two largest, most distinctive
sizes (1816B for Swap, 3656B for Resolve) turned up **zero or false-positive**
exact matches because FM2's copies are compiler-specialized (different byte
counts) and the coincidental size matches were unrelated code (e.g. XAudio2 voice
logic).

## Renames applied (ida37 / FM2)

| addr | old | new | evidence |
|------|-----|-----|----------|
| `0x827313b0` | `FM2_D3D_EmitIndexedDrawPm4Packets` | **`D3DDevice_DrawVertices`** | Line-for-line match vs LO `rex_D3DDevice_DrawVertices` (0x827b56b0, 1000B vs FM2 1004B). Despite the old name saying "Indexed", this is the **non-indexed** primitive — no index-buffer/GPU-offset params, PM4 tag `\|0x80`. |
| `0x827317a0` | `FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset` | **`D3DDevice_DrawIndexedVertices`** | Line-for-line match vs LO `rex_D3DDevice_DrawIndexedVertices` (0x823c6860, 1112B vs FM2 1116B). |
| `0x82731c00` | `FM2_D3D_EmitIndexedDrawPm4WithVertexFormatSetup` | **`D3DDevice_DrawIndexedVertices_WithVertexFormatSetup`** | Same DrawIndexedVertices PM4 core as above, plus an inline vertex-format table-select branch (`(*(a1+10488)&3)==2`) absent from LO's copy — a second, larger inlined specialization (1500B) rather than a byte-identical duplicate, so kept a distinguishing suffix instead of a bare `D3DDevice_DrawIndexedVertices` collision. |
| `0x8237d158` | `FM2_AudioMix_SubmitPendingOutputBody` | **`D3DDevice_Resolve`** | Line-for-line match vs LO `rex_D3DDevice_Resolve` (0x823d45e8, 3656B vs FM2 3572B): identical `SetTileAndDepthClear`/`SetColorClear`/`SetSurfaceClip` call order and PM4 constants. FM2 reuses the EDRAM-resolve path for GPU-side audio mixing (see also already-named `FM2_Render_BlitAudioMixResolveRegionsPm4`), which is why an earlier pass read it as audio code. The function already carried a stale auto-comment ("Misnamed: shared resolve/clear/output PM4 flush...") flagging the mismatch without anyone following through. |
| `0x8236cb28` | `FM2_GpuCommandBuffer_BuildAndSubmit` | **`D3DDevice_Swap`** | Line-for-line match vs LO `rex_D3DDevice_Swap` (0x827b4e50, 1816B vs FM2 1432B — the largest size delta of the set, still same call skeleton). Sole caller of the `VdSwap` kernel import in the whole FM2 binary. |
| `0x8236b010` | `FM2_AudioRender_SampleFrontBufferRegionBody` | **`D3DBaseTexture_FindSurfaceWithinTexture`** | Sole caller is `D3DDevice_Resolve` (above), same `D3DBaseTexture*`/offset/multi-out-param signature as LO's `rex_FindSurfaceWithinTexture_D3D_...` (0x823f05c8). A Resolve helper, not audio code. |
| `0x82730d60` | `FM2_Render_AllocSurfaceAndMemcpyPixels` | **`D3DDevice_DrawVerticesUP`** | Matches LO `rex_D3DDevice_DrawVerticesUP` (0x823e9a30) exactly: `D3DDevice_BeginVertices` → `CopyToWriteCombinedMemory`/`FM2_MemcpyAligned` → same offset-restore write. |

Comments with the above reasoning were also appended at each address in IDA.

## Still open — ClipPlaneEnable and LockTail (tried harder, still not found)

Follow-up attempt (same session, later pass) to trace these via caller context
instead of size, all inconclusive:

- **`D3DDevice_SetRenderState_ClipPlaneEnable`**: LO's caller is a "reset all
  render state to defaults" function (`sub_82811F00`) that also calls
  `SetRenderState_ZEnable/AlphaBlendEnable/StencilEnable/...` with hardcoded 0
  values. FM2 has no equivalent single reset function — its already-named
  `FM2_RenderContext_SetClipPlane0-3Enable` (0x8236F440-0x8236F4A0) looked like a
  promising lead but decompiled to a *different* data layout (writes one byte per
  plane to fixed offsets, dirty bit `0x10000000`) than LO's ClipPlaneEnable
  (packs a 6-bit mask into `+10564`, dirty bits `0x80`/`ROR(1,20)`) — a distinct,
  legitimately FM2-specific per-plane cache, not a disguised copy of the XDK
  primitive. Also tried tracing from already-confirmed sibling
  `SetRenderState_*` functions (AlphaBlendEnable, StencilEnable) — their FM2
  callers are per-pass draw-state appliers (`FM2_Render_EmitPassDrawWork`, etc.),
  not a single defaults-reset function, so there's no equivalent chokepoint to
  search from.
- **`D3DBaseTexture_LockTail`**: LO's caller checks format flags + calls
  `XGGetMipTailDesc`, then branches to either `LockTail` or a regular
  `LockSurface`. FM2 does have `XGGetMipTailDesc` (0x823cd8d8) named, but all 7
  callers are `D3DXLoadSurfaceFromMemory`/`FM2_RenderResource_Setup*MipLayout`
  texture-layout/format-conversion code that computes mip-tail geometry
  directly — none of them call a lock-like primitive matching LO's shape.

Neither is on the Draw/Swap/Resolve critical path, so parked here rather than
continued — revisit if a concrete crash/bug points at one of them specifically.
- `XGSetVertexDeclaration` — per the 2026-06-29 doc, already confirmed inlined
  into `D3DDevice_CreateVertexDeclaration`; no separate FM2 symbol expected.

## Later same-day follow-up: manifest/IDA drift found in the resolve investigation

While reviewing the plume_native black-screen present logic against ReOdyssey
(same day, later session), found that the entire resolve/present-source
heuristic stack in `d3d_hooks.cpp` (`RegisterSurfaceAperture`,
`SnapshotSurfaceForResolve`, `g_sceneResolveSource`) was hooked onto
**`0x82382590`, believed to be `FM2_D3D_EmitSurfaceResolvePackets`** (an EDRAM
resolve-packet emitter) but actually **`D3D::SetPending_Predicated`** — a
per-draw GPU predication-state flush called from `BeginVertices`/
`DrawVertices`/`DrawIndexedVertices`/`Resolve` alike, not resolve-specific.

The IDA database itself already had the *correct* identity for this function
(Hex-Rays showed clean `D3D::CDevice` struct field decompiles and the mangled
type `?SetPending_Predicated@D3D@@YA_KPAVCDevice@1@_KK@Z`) — a previous session
had evidently done a full `D3D::CDevice` struct/type-recovery pass and typed it
correctly, but never propagated that correction to the **manifest** (which is
what actually drives codegen/hook naming), so the shipped code kept operating
under the old wrong name and a false "this call means a resolve is happening"
assumption. A sibling, `0x82382928` (manifest: `FM2_D3D_CountLeadingDirtyBits`,
also stale), turned out to be `D3D::SetPending_RenderStates` by the same
mechanism — confirming this is a repeatable failure mode (IDA type-correction
without a matching manifest/hook update), not a one-off.

Renamed in manifest + IDA (comments added at both addresses):

| addr | old (manifest/hook) | new | real identity confirmed via |
|------|------|-----|------|
| `0x82382590` | `FM2_D3D_EmitSurfaceResolvePackets` | `D3D_SetPending_Predicated` | IDA's own type-propagated mangled name + caller set (Begin/Draw/DrawIndexed/Resolve) |
| `0x82382928` | `FM2_D3D_CountLeadingDirtyBits` | `D3D_SetPending_RenderStates` | same mechanism; not currently hooked in source |

Stripped the resolve-surface-aliasing logic out of the `D3D_SetPending_Predicated`
hook (now a plain passthrough) since it fired on every draw, not just resolves,
with stale `context+10652`/`context+12160` reads. `RegisterSurfaceAperture`,
`SnapshotSurfaceForResolve`, and `g_sceneResolveSource` are left defined but
now unpopulated — they're the right building blocks for hooking the *actual*
`D3DDevice_Resolve` (0x8237D158) directly, which is a separate, larger change
(mirroring ReOdyssey's `D3DDevice_Resolve`/`StretchRect` deferred-resolve
pattern) intentionally deferred to a follow-up session. Verified with a clean
`fm2_codegen` + `fm2` rebuild (no errors) after the rename.

## Later still same day: D3DDevice_Resolve hook actually wired up

The deferred "Step 2" above (hook `D3DDevice_Resolve` directly, mirroring
ReOdyssey's `D3DDevice_Resolve`/`StretchRect` deferred-resolve pattern) turned
out to already be **written and sitting orphaned** in `d3d_hooks.cpp`: a
`Resolve(GuestDevice*, uint32_t flags, const rr::GuestRect*, GuestBaseTexture*,
const rr::GuestPoint*, ...)` function, a verbatim port of ReOdyssey's `Resolve`
hook handler calling `rr::StretchRect`, plus the full deferred-resolve
machinery in `render_state.cpp` (`StretchRect`, `ExecutePendingStretchRects`,
`FlushPendingStretchRects`, wired into `Video::Present()` via
`FlushPendingResolvesForPresent`) — all present, all unused. It was simply
missing a `REX_HOOK(D3DDevice_Resolve, Resolve);` line, because nothing had
correctly identified/named `D3DDevice_Resolve` in the manifest until earlier
today. Same story for `ClearF` — also a ready, ported, never-hooked function
(spotted but not wired this session; low priority, revisit later).

Added the missing `REX_HOOK` (no `REX_IMPORT` needed — this handler never
falls back to the original guest body, matching ReOdyssey). Added temporary
`FM2_RESOLVE_HOOK` trace logging to confirm it actually fires. Verified live:
6 distinct resolve operations fire repeatedly during real gameplay, all
translating valid non-null destination textures, with `srcRT=0x130C7F000`
(the scene RT referenced in older black-screen notes) showing real content.
**However**: the RT actually being presented to screen (`0x130C41000`) never
appears in any resolve traffic — it's populated by direct draws
(`SetScenePresentRT` from the Draw hooks), not by resolve. So this fix is
real, verified, durable progress, but it does **not** by itself explain or
fix the persistent black screen — that's a separate, deeper bug, chased in
`docs/FM2-plume-native-vertex-pulling-gap-2026-07-01.md`.

## Implication for the native-renderer-hook plan

ReOdyssey hooks a single shared `D3DDevice_DrawIndexedVertices` symbol because
LO's UE3 RHI calls through one shared function — hook that address, every draw
call is intercepted. FM2's compiler inlined the same XDK logic into **at least
three separate call sites** for the indexed-draw case alone
(`D3DDevice_DrawIndexedVertices`, `..._WithVertexFormatSetup`, plus whatever
non-`FM2_D3D_Emit*`-named call sites may still exist). There is no single
low-level chokepoint symbol to swap a `REX_HOOK` onto the way ReOdyssey does for
LO. This confirms (with concrete decompiled evidence, for the first time) the
hook boundary already recorded in `AGENTS.md`'s Learned Workspace Facts: hook at
the `FM2_RenderContext_*`/command-buffer layer (first candidate
`FM2_Render_BuildDirectIndexedDrawBuffers`), not at a raw `D3DDevice_Draw*`
symbol.
