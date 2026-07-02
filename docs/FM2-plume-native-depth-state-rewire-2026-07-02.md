# FM2 plume_native: render-state rewire + depth fix — content now renders (2026-07-02)

Continuation of `docs/FM2-ida-renames-2026-07-01.md` and
`docs/FM2-plume-native-vertex-pulling-gap-2026-07-01.md`. Chronicles the
depth-rejection root cause, the render-state hook rewire that fixed it, and
the current state: **real content now renders into a 720p RT; the remaining
black screen is present-source selection, not rendering.**

## Root cause #4 (drift again): the whole packed-state hook cluster was mis-wired

RenderDoc (capture_5, post-vertex-fix) showed fragments rasterizing with real
HDR colors but **100% depth-rejected** (`samples_passed=0`, buffer cleared to
1.0, GREATER-family compare). Instrumentation (`FM2_ZFUNC`/`FM2_ZCLEAR`/
`FM2_ZVIEWPORT` in `render_state.cpp`) showed `D3DRS_ZFUNC` receiving `raw=0`
on every call → `ConvertCmpFunc(0)=D3DCMP_NEVER` → plume `NEVER` → everything
rejected.

IDA (after reconnect) revealed why: the June-18 "packed state" cluster at
`0x8236Exxx-0x8236Fxxx` is the ordinary XDK `D3DDevice_SetRenderState_*`
family writing RB_DEPTHCONTROL (`ctx+10420`) bit fields, and the manifest/hook
names had drifted from IDA's own later type-recovery (same failure mode as
`SetPending_Predicated`, third instance):

| addr | real identity | was hooked/mirrored as |
|---|---|---|
| `0x8236EAF8` | AlphaBlendEnable | → `D3DRS_ZENABLE` ❌ |
| `0x8236F1F0` | **ZFunc** (RB_DEPTHCONTROL bits 4-6) | → `D3DRS_ALPHABLENDENABLE` ❌ |
| `0x8236F268` | TwoSidedStencilMode (bit 7) | → `D3DRS_ALPHATESTENABLE` ❌ |
| `0x8236F2D0` | StencilFail (bits 11-13, always 0=KEEP) | → `D3DRS_ZFUNC` ❌ ← the NEVER bug |
| `0x8236F340` | StencilPass (bits 14-16) | → `D3DRS_COLORWRITEENABLE` ❌ ← historic colorWrite=0 bug |
| `0x8236F180` | **ZEnable** (bit 1) | not hooked |
| `0x8236F1C0` | **ZWriteEnable** (bit 2) | not hooked (IDA had it as SetZEnableBit) |
| `0x8236F718` | **ColorWriteEnable** (RB_COLOR_MASK, `Value&0xF`) | not hooked |
| `0x8236EAC0` | AlphaTestEnable | not hooked |
| `0x8236EB88-EE18` | BlendOp/SrcBlend/DestBlend/SrcBlendAlpha/DestBlendAlpha | not hooked |

**Fix (landed, rebuilt clean):** manifest renamed to true XDK names + 11
correct mirrors wired in `d3d_hooks.cpp` (`Fm2Rs*` handlers); the three
stencil-op setters are deliberately unhooked (originals run untouched).
`0x8236EF20` (AlphaRef) intentionally skipped — it has an existing manifest
name + midasm hook.

## Root cause #5: reverse-Z double-flip

FM2 is natively reverse-Z (D24FS8 scene depth `0x1A220197`; observed clears
`z=0.0` = far; game ZFunc `raw=4` = GREATER). `render_state.cpp` applied BOTH
schemes at once: `FlushViewport` kept the reversed depth range (minZ=1/maxZ=0)
while `Clear` flipped the clear value (`1.0f-z`) and `SetDepthState`/
`D3DRS_ZFUNC` flipped the compare (`FlipCmpFunc`) — a contradiction that
rejects everything under any consistent game input. **Fix:** faithful
passthrough — keep the reversed viewport, removed both flips. Verified live:
`FM2_ZFUNC raw=4 → GREATER (final=5, no flip)`, `FM2_ZCLEAR z=0 → applied=0`.

## Verified result (capture_6, post-everything)

- Pixel history at (638,128) on the tile RT: **zero `depth_test_failed`** (was
  100%); 16 fragments reach and write.
- **RT 349 (1280×720) contains real image content** — 25% of the frame
  non-black (green channel up to 0.53), tool description "Appears to contain
  normal image content". RT 324 (720p) has alpha coverage over half the frame.
- The presented RT (`0x130C41000` = the 1280×256 EDRAM tile, host RT 317) is
  still black → **the remaining black screen is PRESENT-SOURCE SELECTION /
  tile-composite, no longer geometry/state/shading.**

## Next session: the present/composite chain

1. `GetLastDrawnColorRenderTarget()` returns `g_scenePresentRT`, which the PM4
   draw hooks overwrite on every draw — the last drawn RT is the 1280×256
   tile, so present shows the (black) tile. Map which guest RT actually holds
   the final composite (RT 349's guest address — census candidates:
   `0x130CC1000`/`0x130C7F000` 720p) and present that; principled route =
   follow the swap fetch (VdSwap frontbuffer base) through the now-working
   `D3DDevice_Resolve`/`StretchRect` chain instead of the last-drawn heuristic.
2. FM2 uses EDRAM predicated tiling (1280×256 tiles × ~3 for 720p; that's why
   `SetPending_Predicated` exists on every draw). Tile passes need per-tile
   placement (window offset / resolve destPoint) for the 720p composite to
   assemble correctly.
3. Cleanup queued: `ApplyLiveColorWriteFromContext` reads ctx+10420 bits 14-16
   (= STENCILZPASS, wrong) and the `FlushRenderState` colorWrite force-hack
   compensates — both should be retired now that the real ColorWriteEnable
   mirror exists (real mask lives at the RB_COLOR_MASK ctx word, `Value&0xF`
   via 0x8236F718).
4. RT 349's content is green-channel-only (R=B=0) — possibly a channel-mapping
   or format-alias issue to check once it's being presented.
5. TEMP diagnostics still in code: FM2_ZCLEAR/FM2_ZFUNC/FM2_ZVIEWPORT
   (render_state.cpp), FM2_LOCK_WRITETHROUGH/FM2_BINDSTREAM/FM2_RESOLVE_HOOK
   (d3d_hooks.cpp) — remove once stable.

## Fix ledger for the whole 2026-07-01→02 arc

1. D3D draw/swap/resolve primitives identified (compiler-inlined, renamed).
2. Manifest/IDA drift #1: `SetPending_Predicated` mis-hooked as resolve emitter (fixed).
3. `D3DDevice_Resolve`/`ClearF`/`DrawVerticesUP` orphan handlers wired (mode-gated RAW).
4. Lock write-through: game vertex data now reaches guest physical RAM
   (`FM2_LOCK_WRITETHROUGH`, byte-verified at draw time).
5. `DrawVertices` non-indexed correction + `baseVertexIndex` passthrough.
6. Render-state cluster rewire (this doc) — depth/blend/alpha/colorwrite real.
7. Reverse-Z double-flip removed — fragments pass depth for the first time.
