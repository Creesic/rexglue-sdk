# FM2 plume_native handoff — 2026-07-02 session 4 (ring-scan transport DISPROVEN)

Continuation of `FM2-handoff-2026-07-02-session3.md`. Session 3's endgame plan
("CP reconstruction: scan the command buffer continuously, fix the wrap-skip,
apply everything into one stream-ordered register file") was implemented,
live-tested, and **disproven as a transport**. The evidence gathered here
redirects the fix to write-time emitter hooks. The user-visible state is
back to session 3's build (white-artifact boot, menu UI visible, car flash
present); all new scanner behavior is gated OFF.

## What was implemented (and left in-tree, gated)

1. `d3d_hooks.cpp` `ProcessPm4VsConstantsDiag`: the delta walk moved into
   `ScanPm4AluConstantRange(rangePhys, len)`; on `curPhys < lastPhys` the
   scanner can follow the ring wrap (scan `[lastPhys, ringEnd)` then
   `[ringBase, curPhys)`). **`kFollowRingWrap = false`** — see below.
   `FM2_PM4WRAP` diagnostic stays (logs ring geometry + sane check, first 8).
2. `render_state.cpp`: **`kCpAccumulate3D = false`** — 3D draws briefly
   consumed the accumulated PM4 shadow; reverted to fresh-per-delta.
3. Scan caps restored to session-3 values (len ≤ 0x40000, guard 8192).

## Ring geometry (IDA-recovered, live-verified — keep, it's real)

- The FM2 command stream for draw context `0x4004D100` is a **3 MB ring**:
  base = `*(ctx+14528)`, end = `*(ctx+14532)`, committed write ptr =
  `*(ctx+0x30)` (all phys-alias addrs; mask `0x1FFFFFFF`).
  `FM2_PM4WRAP ... ring=[09A49300,09D492FC) sane=1` on every wrap.
- Allocator: `D3D::RingBufferDeviceAllocate` @0x823721D8 (wrap: restart at
  base, bump lap counter +14544, flag `*(ctx+10814) |= 8`).
  `D3D_AllocateCommandBuffer` @0x823723E0; chained-CB segments (2008 bytes,
  header dword0 = next-segment phys, dword1 = used qwords) built by
  `FM2_RenderContext_SetShaderResourceState` @0x82375078 (misnamed; it's
  the make-space/segment-switch) — used by the locked-resource path, not the
  main draw ring.
- Only ONE context issues PM4 draws (`FM2_PM4GEO_IDX ctx=0x4004D100` is the
  only value ever logged).

## Why ring scanning is a dead transport (the disproof)

1. **The ring tail is NOT NOP-padded.** With wrap-following ON, the
   histograms filled with misparsed junk: `FM2_PM4SETC idx=0 regs=2501
   v=8200001D` (a PPC code address as "constant data"), idx up to 2044,
   `FM2_PM4LOADALU sz=3073`. Thousands of garbage dwords were applied into
   the shadow every wrap (Apply* clamps to the 0x400-dword file, so no host
   overflow — but the whole file got stomped). User-visible result: boot
   went white-artifact → BLACK, menu UI invisible. Confirmed regression,
   reverted.
2. **Even forward deltas misparse.** Yesterday's (session-3) logs ALSO
   contain garbage histogram entries (`type=95/112/166`, `regs=2486`...):
   when the ring wraps AND the new write ptr lands ABOVE the old sample,
   `curPhys > lastPhys` looks like a normal forward delta and the scanner
   walks stale bytes. Draw-time write-ptr sampling fundamentally cannot
   know which bytes are fresh. The session-3 shadow was already being
   poisoned at low frequency.

## The flash: root cause sharpened (red vs blue capture diff)

User captures `renderdoccaps/fm2redartifact.rdc` (session 3 build) and
`fm2blueartifact806.rdc` (wrap-following build), same 267-event frame:

- The car draw's (event 1841) VS float file c0–c3 holds THE SAME value
  family in both: rows with ±93.70 / ±70.85 / ±70.71 (=100·cos/sin 45°) and
  screen-space translations. That is the **boot spinner's rotating 2D
  placement matrix** (verified as the legitimate c0–c3 of a 2D spinner
  element draw in `scratchpad fm2cap_cpreco_capture_5.rdc` of session
  cc9c696e). It rotates every frame → the car's lighting math output flips
  sign patterns per frame → **the red/yellow/blue flashing IS the spinner
  rotation**, delivered into the car's constants by our own overlay (the
  spinner's `SET_CONSTANT idx=0 regs=16` lands in the car draw's pre-draw
  delta on the SAME single ring, so even the "fresh" 3D overlay applies it).
- The car's OWN material/light constants never appear in the scanned
  stream at the registers the PS reads. Part-6's "PS outputs ±90 garbage"
  values were literally these c0/c2 spinner rows.

## NEXT BIG ROCK (revised): write-time emitter hooks, not ring parsing

Mirror constants at WRITE time, where index/count/data are exact and call
order = stream order by construction:

1. Find the XDK emitters that write the ALU packets into the ring:
   - the `SET_CONSTANT` (op 0x2D) builder(s),
   - the draw-time dirty-constant flush that emits the Type-0 reg-0 bursts
     (448–1008 dwords — this is the path that should carry the car's WVP +
     material file),
   - `D3DDevice_SetVertexShaderConstantFN` @0x8236D958 is already hooked
     (the g_passVsConstants mirror) — extend the same pattern.
2. Hook them to apply into the PM4 shadow directly (per packet, in call
   order). Delete the ring parser once parity is confirmed (its histograms
   remain useful for cross-checking).
3. Then re-run the 3D=accumulated experiment (`kCpAccumulate3D=true`) — with
   a trustworthy shadow it may just work; the per-draw ordering question
   (spinner c0-c3 vs car c0-c3) is then about WHICH emissions happen between
   the spinner element and the car draw, which the hooks will show directly.

## Part 2: write-time emitter hooks — TRIED, REGRESSED, GATED OFF

**User verdict: "regressed two days".** Root cause of the regression: for
deferred/recorded render paths the SetF hook applies constants at RECORD
time while the draws execute later (drain/CB playback), so every draw reads
the NEWEST upload instead of its own — the same smearing failure as the old
kUseUiGlyphModelViewOverlay experiment. The ring scanner's per-draw
pre-draw-delta application is the CORRECT ordering; its only defect is
parsing staleness across undetected wraps. Final state of this session:
**session-3 behavior restored exactly** (`kScannerApplies=true`,
`kSetFHooksFeedPm4Shadow=false`, `kFollowRingWrap=false`,
`kCpAccumulate3D=false`). Everything below kept for the record + the
identified chokepoints remain valuable.

**Lesson for next session: change ONE variable at a time and get eyes on the
screen after each.** The sound next increment is to keep the scanner (right
ordering) and fix its parsing trustworthiness only — e.g. detect wraps
robustly via the ring lap counter at ctx+14544 (increments per wrap,
D3D::RingBufferDeviceAllocate 0x823721D8) instead of write-ptr comparison;
on ANY lap-count change, resync lastPhys=curPhys and skip (never walk stale
bytes), losing that one delta but never poisoning. Then, separately,
evaluate accumulated-3D.

Implemented the revised transport the same session (ReOdyssey precedent:
it wholesale-replaces `D3DDevice_Set{Vertex,Pixel}ShaderConstantFN` and
memcpys into the host device file — see its `src/render/d3d_hooks.cpp:576`):

- IDA: `0x8236D958` = `D3DDevice_SetVertexShaderConstantFN` (writes CPU file
  base+0x10+16·reg, ORs dirty bit into m_Pending.m_Mask[0]);
  **`0x8236DA60` = `D3DDevice_SetPixelShaderConstantFN`** (PS twin, adjacent).
  Both have dozens of FM2_Render_* callers (the FF material/light uploads;
  PS side called from FM2_Render_DispatchPm4DrawOpcode etc.).
- `d3d_hooks.cpp`: the existing `REX_HOOK_RAW(sub_8236D958)` now ALSO feeds
  `rr::ApplyPm4VsConstants(destReg*4, src, count*4)`; new
  `REX_HOOK_RAW(sub_8236DA60)` feeds `rr::ApplyPm4PsConstants` (+
  `FM2_SETPSCONST` log). REX_HOOK_RAW = strong-symbol override, no manifest
  change needed.
- `ScanPm4AluConstantRange`: `kScannerApplies=false` — ring scanner is
  diagnostic-only now (histograms remain).
- `render_state.cpp`: `kCpAccumulate3D=true` re-enabled — the earlier
  accumulated-3D regression is attributed to scanner garbage, not policy.
- Smoke test: both hooks fire with sane traffic (`FM2_SETPSCONST destReg=0
  count=5` material blocks cycling per-element source addrs, lr=0x825DC118
  in sub_825DBF50). Game stable.
- Open risks if visuals regress: (a) cross-device/record-vs-play ordering
  (SetF calls on recording contexts mirror at RECORD time, not CB playback
  time); (b) game threads calling SetF concurrently with drain-thread draws
  (shadow writes are unsynchronized); (c) constants written by direct
  packet builders that bypass the SetF pair would now be missed entirely.

## Part 3: press-A screen investigation (capture-based, no code changes)

Target = bring up the press-A screen. Ground truth: Xenia
`renderdoccaps/xeniafm2pressstart.rdc`; our build
`renderdoccaps/fm2pressstart2.rdc` (user-captured; 161 draws, EIDs→~2118).

Findings (all capture-verified):
1. **The "messed-up mesh" is a RenderDoc VS-Input artifact, NOT broken
   geometry.** The 2D UI draws (e.g. EID 148, idx 222) carry position in
   `TEXCOORD0` (half2) with NO `POSITION` semantic, so RenderDoc's VS-Input
   3D plot is meaningless (spiky). `decode_mesh_inputs` shows clean local
   coords (-0.15..0.81) + proper UVs + shared indices. Judge these by VS
   OUTPUT / rendered pixels, never VS-In.
2. **Memexport is a red herring for the press-A UI.** Xenia's press-A frame
   DOES run ~24 memexport passes (VS+GS+PS UAV-writes to a 32KB buffer,
   viewport 8192², reading shared-mem), and plume_native has zero memexport
   support (the SDK Xenos CP implements it — spirv/dxbc_translator_memexport,
   command_processor readback — but plume_native bypasses the CP). BUT the
   press-A UI itself is CPU-generated 8-byte glyph geometry in a bump buffer
   (Buffer 342), not memexport output. Memexport remains a real gap for
   whatever those passes feed (skinning/particles/3D), just not the UI text.
3. **The session-3 PM4 placement fix DOES reach press-A.** The first capture
   the user showed (`fm2pressstart.rdc`, 14:45) was STALE — pre-fix: draw
   145 c0=(0.502 gray) c2-c3=0. Current build (fm2pressstart2): c0 is a real
   matrix row. `FM2_DRAWSEQ` logging gray/zero is EXPECTED — it samples the
   raw issuing-ctx file (ctx+0x700), which lacks the matrix; the overlay
   supplies it at upload time.
4. **RT 313 is a SHARED EDRAM scratch surface.** Its usage repeats in ~7
   blocks (clear → ~20 ColorTarget draws → 3 CopySrc), and block 2's first
   draw is a 3D shader (fb3c0c13, 32-byte verts) while block 1 is 2D glyphs
   (99bda386) — so the 7 "repeats" the user noticed are SEPARATE
   render→resolve cycles (background, logo, A-button, glyphs, 3D...), each
   rendered into RT 313 then CopyTextureRegion'd to its final home. NOT 7
   frames, NOT 7 tiles. The composition itself is CopyTextureRegion blits:
   user maps EID 25=background, 103=A-button, 112=logo (each a Copy in its
   own Reset/Copy/Close command list).
5. **Visible defects in RT 313 @ EID 518** (block-1 fully rendered): a
   collapsed/distorted gray glyph near center-bottom (matches old "#13
   glyphs collapse to center") + large structural black/white regions
   (left-half black, right-half white with a black rect) — a surface
   clear/resolve/region problem distinct from the glyph collapse.

Still needed (asked user): which RT is the FINAL press-A composite (310/320
are unused here; dest of the bg/logo/button copies not yet located — likely
340/347/366/369/796 or a guest texture / present surface), and the bindless
texture mapping (PS at EID 148 samples indices [28,19,20]; FontTex=242).
NO code changed this part — root cause not yet pinned; avoiding another
premature change.

Debug note: fm2 has a **silent death under RenderDoc** (~10-15s, no WER, no
log banner) that repeatedly killed capture attempts; also opening several
captures via the RenderDoc MCP triggered `DXGI_ERROR_DEVICE_RESET` (close
extra captures / the qrenderdoc GUI to recover). User will provide captures
to `renderdoccaps/` rather than have Claude launch them.

### CONCLUSION (shader-level, decisive): press-A text = constant-driven
### vector shader; it's the SAME PM4-constant problem, not a new bug

User clarified there is NO final composite RT — press-A on screen = the
copied pieces (bg/logo/button, which render fine because they're plain
`CopyTextureRegion` blits) PLUS "empty white/black polygons" = the DRAWN
UI text.

Root cause pinned by reading the shaders (fm2pressstart2 EID 148):
- **The UI text samples NO texture** (PS `readonly_bindings: []`,
  `samplers: []`). PS `cd714e30` is a procedural **Loop-Blinn** curve shader:
  `_37 = tc0.x*tc0.x - tc0.y` (quadratic-curve inside test), outputs a flat
  color `c0.xyz` with an analytically-computed coverage as alpha, and
  `Discard`s low coverage. So "which texture goes where" does NOT apply to
  the text — there is no font atlas; the glyphs are vector curves.
- The VS `99bda386` builds BOTH the glyph position AND the curve params
  from `cbuffer0` (the VS float file) at offsets 0/16/48/64/80/96/112/128/144
  = **c0,c1,c3,c4,c5-c9**. Position = `c0*(|tc0.x|+c4.x) + c1*tc0.y + c3`,
  then projected by c5-c9 (matches the session-3 formula exactly).
- In our capture `c0 = (-0.0, -2.25, 0, 0)`; Xenia's ground-truth glyph
  placement matrix is ~`[0.033,0,0,0][0,0.059,0,0][-0.59,-0.75,0.118,1]`.
  Our c0 is wrong (−2.25 vs ~0.033), so glyphs both mis-place (collapse to
  center, per old #13) AND the coverage degenerates uniform → each glyph
  quad fills solid (white/black) instead of carving a letter.

**Therefore press-A text is gated on the SAME VS-constant transport problem
this whole session has been about** (c0-c9 per-glyph placement matrices
arriving wrong through the PM4 path) — NOT textures, NOT memexport, NOT
geometry. Do NOT treat it as a separate bug.

### TRACE RESULT (log-proven, the actual mechanism)

Grepping the run log for PM4 writes to register c0 shows THREE distinct
writers, and our shadow keeps the wrong one:
- `FM2_PM4SETC type=0 idx=0 regs=16` = the 4x4 PLACEMENT MATRIX. c0.x =
  `0x3D4CCCCD (0.05)` / `0x3D246757 (0.040)` — small scales matching
  Xenia's ~0.033-0.059. **This is the correct per-glyph matrix and it IS
  in the stream, right before the glyph draw.**
- `FM2_PM4SETC type=0 idx=0 regs=3` = a DIFFERENT 3-register write, c0.x =
  `0x80000000 (-0.0)`. This is the value we wrongly apply (our draw-time
  c0 = (-0.0, -2.25, 0, 0)).
- `FM2_PM4T0ALU reg=0` bursts (count 448/480/2498...) = the XDK dirty-flush,
  also stomping c0-c3.

Root mechanism: **register c0-c3 is written by multiple unrelated sources
per frame; the correct glyph matrix is the `regs=16` SET_CONSTANT emitted
immediately before each glyph draw, but our ACCUMULATED shadow keeps a later
`regs=3` / Type-0 write.** (Fresh-per-delta would be right IF the scanner
reliably captured the pre-draw delta — but its wrap-skip drops deltas, so
fresh falls back to the gray/zero live file, which is why session 3 chose
accumulated-as-less-bad.) Xenia's press-A capture has NO indexed glyph draw
(`DrawIndexed` count = 0; it's memexport-composed), so ground truth comes
from OUR emitted stream (same game code → same PM4), not a Xenia draw.

### PROPOSED TARGETED FIX (not yet implemented — distinct from the 2 failed
### blind rewrites; keys on the distinctive packet, behind a toggle)

Track the glyph placement matrix SPECIFICALLY: keep a small "last
SET_CONSTANT idx=0 **regs=16** payload" shadow (the 4x4 matrix), updated
only by regs=16 idx=0 writes (ignore regs=3 and Type-0 for c0-c3), and for
no-POSITION (2D) shaders overlay c0-c3 from THAT shadow. This dodges both
the smearing (regs=3/Type-0) and the fresh-delta wrap-skip. Success =
press-A "PRESS A" text carves real letters instead of solid polygons; verify
on screen before/after, one toggle (`kUseGlyphPlacementMatrixShadow`).

## Also this session

- **Crash** (unattended, ~80 s): c0000005 in `sub_825E0678`
  (fm2_recomp.33.cpp:68350, guest 0x825E0720 `lwzx r29,r11,r29`) — a
  mesh-section draw dispatcher; it read `a1[20]` fine BEFORE a virtual call
  and got garbage re-reading it AFTER ⇒ CParams payload recycled mid-call.
  Same deferred-pool lifetime-race family as the 17:41 IsFm2Resource crash
  (that one was guarded via IsReadableHostPtr; this dispatcher is another
  entry point of the same class). WER: offset 0x21762e8, module fm2.exe.
  Symbolize WER offsets with:
  `llvm-symbolizer --obj=fm2.exe --relative-address <offset>` (x64 build in
  VS18 `VC\Tools\Llvm\x64\bin`).
- RenderDoc hands-off workflow that worked: `renderdoccmd capture -d FM2
  -c <template> fm2.exe <bat args>` + focus window + SendKeys `{F12}`.
  Captures in session cc9c696e scratchpad (`fm2cap_cpreco_*.rdc`).
  NOTE: "RT black at end of capture" is normal for some frames (EDRAM-style
  resolve/clear cadence) — judge content mid-frame or on the user's screen.
- The `FM2_CRASH code=0x40010006` log lines are DBG_PRINTEXCEPTION_C
  (OutputDebugString), benign, present in every run.
