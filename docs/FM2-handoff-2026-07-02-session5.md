# FM2 plume_native handoff — START HERE (session 5 entry)

Clean, self-contained handoff at the end of session 4. Detailed running notes
for session 4 are in `FM2-handoff-2026-07-02-session4.md` (ring-scan disproof,
write-hook disproof, full press-A investigation); session 3 is in
`FM2-handoff-2026-07-02-session3.md`. This doc is the authoritative summary —
you should not need to read the chain to resume.

## TL;DR

- **Build state = session-3 "good" baseline, verified restored by the user.**
  Two constant-transport experiments this session (ring-wrap-following, and
  write-time SetF hooks) both regressed and are **gated OFF**. Nothing in the
  working tree changes runtime behavior vs. the session-3 baseline.
- **Press-A screen is fully root-caused** (this session's main result). It is
  NOT a new bug, NOT textures, NOT memexport, NOT geometry — it is the SAME
  VS-constant transport problem, pinned to a specific mechanism (below).
- **A targeted, non-blind fix is designed and ready to implement** behind a
  toggle. It is materially different from the two reverts.

## Build / run

- Current `fm2.exe`: `FM2/out/build/win-amd64-relwithdebinfo/fm2.exe`
  (rebuilt this session, session-3 behavior).
- Build FM2 from a plain Claude shell needs the VS18 lib dir prepended to
  `$env:LIB` in the SAME call (else lld-link `__std_find_*` undefined):
  `$env:LIB = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\lib\x64;" + $env:LIB`
  then `cmake --build --preset win-amd64-relwithdebinfo --target fm2` from `FM2/`.
- Launch: `scripts/fm2/launch-fm2-plume-native.bat` (do NOT bare-launch fm2).
- Common log: `C:\temp\fm2-clean.log` (append-mode; tail from a recorded byte
  offset, never whole-file grep).

## Current toggles (all at session-3-good values)

`FM2/src/render/d3d_hooks.cpp`:
- `kFollowRingWrap = false` — ring scanner does NOT follow wraps (following
  them applied stale tail bytes → boot black + menu UI gone). Wrap DIAGNOSTIC
  (`FM2_PM4WRAP`) still logs.
- `kScannerApplies = true` — the ring scanner still feeds the PM4 constant
  shadow (session-3 behavior; per-draw pre-draw-delta application = the
  correct ORDERING, even though its parsing can read stale bytes on undetected
  wraps).
- `kSetFHooksFeedPm4Shadow = false` — the new SetF write-hooks
  (`sub_8236D958` VS / `sub_8236DA60` PS) do NOT feed the shadow. Enabling it
  regressed ("two days back"): the hooks apply at RECORD time but deferred
  draws execute later, so every draw saw the newest upload (smearing). The
  `FM2_SETPSCONST` logging + the PS chokepoint hook remain in place, dormant.

`FM2/src/render/render_state.cpp`:
- `kCpAccumulate3D = false` — 3D draws use fresh-per-delta PM4 overlay (2D
  uses accumulated). Accumulated-3D regressed under both experiments.

## PRESS-A ROOT CAUSE (the session's main result)

Captures: ours `renderdoccaps/fm2pressstart2.rdc` (user-provided); Xenia GT
`renderdoccaps/xeniafm2pressstart.rdc`.

1. **The "messed-up mesh" is a RenderDoc artifact, not a defect.** The 2D UI
   draws carry position in `TEXCOORD0` (half2), no `POSITION` semantic, so
   RenderDoc's VS-Input plot is meaningless (spiky). `decode_mesh_inputs`
   shows clean glyph geometry. Judge by VS-Output / rendered pixels only.
2. **Press-A composition:** the background / logo / A-button are plain
   `CopyTextureRegion` blits (they render fine — no shader/constants). The
   "empty white/black polygons" the user sees are the DRAWN UI text.
3. **The UI text samples NO texture.** PS `cd714e30` is a procedural
   **Loop-Blinn** vector-curve shader (`tc0.x*tc0.x - tc0.y` curve test):
   outputs a flat color `c0.xyz` with analytic coverage as alpha, discards
   low coverage. There is no font atlas — glyphs are math. "Which texture
   goes where" does not apply to the text.
4. **Placement + coverage both come from VS constants c0-c9.** VS `99bda386`
   builds position = `c0*(|tc0.x|+c4.x) + c1*tc0.y + c3`, projected by c5-c9
   (reads cbuffer0 at offsets 0/16/48/64/80/96/112/128/144).
5. **Log trace of writes to register c0** (THE mechanism): three writers per
   frame, and our shadow keeps the wrong one:
   - `SET_CONSTANT idx=0 regs=16` = the 4x4 placement matrix, `c0.x = 0.05 /
     0.040` — **correct** (matches Xenia ~0.033-0.059), and it IS in the
     stream immediately before each glyph draw.
   - `SET_CONSTANT idx=0 regs=3` = a different write, `c0.x = -0.0` — **this
     is what we wrongly apply** (our draw-time `c0 = (-0.0, -2.25, 0, 0)`).
   - `Type-0 ALU reg=0` bursts (XDK dirty-flush) — also stomp c0-c3.
   Our ACCUMULATED shadow keeps a later `regs=3`/Type-0 write instead of the
   per-draw `regs=16` matrix. Wrong c0 → glyphs mis-place (collapse to center)
   AND the coverage goes uniform → solid polygons.
6. **Xenia's press-A capture has zero indexed glyph draws** (`DrawIndexed`
   count 0 — it composes text via memexport, a different method/moment), so
   there is no comparable Xenia draw to read c0 from. Ground truth comes from
   OUR emitted stream (same game code → same PM4): the game emits `0.05`.

## NEXT STEP: the proposed targeted fix

**STATUS 2026-07-02 (session 5): IMPLEMENTED as designed, built into
`FM2/out/build/win-amd64-relwithdebinfo/fm2.exe` (22:28). Awaiting user
on-screen A/B against the success criterion below.** Pieces:
- Scanner capture: `d3d_hooks.cpp` `ScanPm4AluConstantRange`, SET_CONSTANT
  branch — `idx==0 && cnt==17` (= regs=16) calls
  `rr::SetGlyphPlacementMatrix(payload)`. regs=3 / Type-0 never touch it.
- Shadow + overlay: `render_state.cpp` — `g_glyphPlacementMatrix[16]` next to
  `g_uiGlyphModelView`; in `FlushRenderState`, after the accumulated 2D
  overlay, `kUseGlyphPlacementMatrixShadow = true` copies the shadow over
  c0-c3 for no-POSITION shaders only (so it WINS over the regs=3/Type-0
  stomps already merged). 3D path untouched.
- Declared in `render_internal.h` (`SetGlyphPlacementMatrix`).
- Revert = flip `kUseGlyphPlacementMatrixShadow` to false (render_state.cpp).

Track the glyph placement matrix SPECIFICALLY, keyed on the distinctive
packet — different from the two blind rewrites that regressed:

- Add a small dedicated shadow: "last `SET_CONSTANT idx=0 regs=16` payload"
  (the 4x4 matrix), updated ONLY by regs=16 idx=0 writes in the scanner
  (ignore `regs=3` and Type-0 for c0-c3).
- In `FlushRenderState`, for no-POSITION (2D) shaders only, overlay c0-c3
  from that shadow. 3D untouched.
- Toggle `kUseGlyphPlacementMatrixShadow`; fully reversible.
- This dodges BOTH failure modes: the smearing (regs=3/Type-0) and the
  fresh-delta wrap-skip that made session 3 fall back to accumulated.
- **Success criterion (verify on screen, before/after):** "PRESS A" carves
  real letters instead of solid polygons; glyphs sized/placed correctly.

If it works for 2D, then reconsider whether the same regs=16-keyed approach
helps the 3D car (its per-object WVP has the same multi-writer register
contention).

## Other durable findings this session

- **Memexport is a real plume_native gap but a red herring for the press-A
  UI.** Xenia's press-A DOES run ~24 memexport passes (VS+GS+PS UAV-writes to
  a 32KB buffer, viewport 8192², reading shared mem). The SDK Xenos CP
  implements memexport (`spirv/dxbc_translator_memexport`, `command_processor`
  readback) but plume_native bypasses the CP, so those passes don't stream
  back. The press-A TEXT, however, is CPU-generated glyph geometry (Buffer
  342), not memexport output. Memexport remains a gap for whatever those
  passes feed (skinning/particles/3D) — a separate, larger future item.
- **RT 313 is a shared EDRAM scratch surface.** Its usage repeats in ~7
  blocks (clear → draws → 3 CopySrc); block 2's first draw is 3D, block 1 is
  2D — so the "repeats" the user noticed are separate render→resolve cycles
  (bg, logo, button, glyphs, 3D...), NOT 7 frames and NOT 7 tiles. There is
  no single "final composite" RT — the screen = the copied pieces + the drawn
  polygons.

## Debugging workflow / gotchas learned

- **fm2 silent-death under RenderDoc**: dies ~10-15s in, no WER, no log
  shutdown banner, process just vanishes. Repeatedly killed hands-off capture
  attempts. Not a crash we can catch via WER. **User will provide captures to
  `renderdoccaps/` rather than have Claude launch-and-capture.**
- **RenderDoc MCP device reset**: opening several captures triggers
  `DXGI_ERROR_DEVICE_RESET` (aggravated by the qrenderdoc GUI holding the
  GPU). Recover by `close_capture` on all but one; keep ≤1-2 open at a time.
- **Texture images from the MCP exceed the token cap**: `get_texture_image`
  with `include_image=true` saves to a file; decode the base64 to PNG in
  PowerShell (`[IO.File]::WriteAllBytes(out,[Convert]::FromBase64String(...))`)
  and `Read` the PNG (Read renders images).
- **WER offset → symbol**: `llvm-symbolizer --obj=fm2.exe --relative-address
  <off>` (x64 build under VS18 `VC\Tools\Llvm\x64\bin`; the ARM64 one won't
  run). One crash this session: `sub_825E0678` (mesh-section dispatcher,
  a1[20] recycled mid-vcall) — the known deferred-pool payload-recycle race.
- **`FM2_DRAWSEQ` c0=gray/zero is EXPECTED** — it samples the raw issuing-ctx
  file (`ctx+0x700`), which lacks the placement matrix by design; the overlay
  supplies it at upload time. Do not read it as the final constant.
- `FM2_CRASH code=0x40010006` log lines are benign `DBG_PRINTEXCEPTION_C`.

## SESSION 5 CONTINUATION (2026-07-03): UnleashedRecomp-guided transport pivot

User directive: use `C:\Users\Tera\Documents\GitHub\UnleashedRecomp` as the guide
(instead of ReOdyssey-style PM4 interception). Findings + state:

1. **Ring-scan window root cause (proven)**: the sampled write pointer trails
   the final pre-draw packet (FM2_C0WRITE inb=0 off=12 len=80 every glyph;
   FM2_C0DUMP shows the payload complete just past the window). Strict bounds
   rejected THE LAST CONSTANT PACKET BEFORE EVERY DRAW. Fixes landed:
   glyph-capture slack, then general `kApplyTailSlack=true` (+64B) in
   `ScanPm4AluConstantRange`.
2. **Why the pointer trails — the real emitter found**:
   `D3DDevice_GpuBeginShaderConstantF4` (0x82803358, decompiled) allocates ring
   space, writes the SET_CONSTANT op-0x2D header + leading 0x80000000 NOPs,
   bumps m_pRing, and returns the payload ptr for the CALLER to fill after
   return. Per-draw constants (glyph matrices; likely per-object data) travel
   through it, bypassing the device constant file entirely.
3. **UnleashedRecomp model verified** (gpu/video.cpp): guest D3D wrappers write
   GuestDevice file + dirty flags; each draw snapshots dirty constants into an
   intermediary allocator and enqueues ordered commands; host replays. FM2's
   XDK equivalents are all named in IDA: `D3DDevice_SetVertexShaderConstantFN`
   (0x8236D958, device file + m_Pending.m_Mask — decompiled, same model),
   `D3D::SetPending_AluConstants` (0x82382cc8, dirty flush),
   `D3DCommandBuffer_SetShaderConstantF`/`CreateShaderConstantFFixup`
   (0x823767b8/0x823766e0, precompiled-CB fixups),
   `FM2_D3D_EmitShaderConstantsBatch` (0x82730dc0).
4. **Implemented: emitter hook transport** (`kGpuBeginHookFeedsShadow=true`,
   d3d_hooks.cpp): REX_HOOK_RAW(sub_82803358) records (payload,reg,count,ps)
   in call order; `DrainGpuBeginConstants()` applies them via
   rr::ApplyPm4{Vs,Ps}Constants + the glyph shadow at the top of
   `ProcessPm4VsConstantsDiag`, before the ring scan. VERIFIED live:
   FM2_GPUBEGINCONST fires (all ps=0 reg=0 count4=4, lr=0x827BB01C = text
   renderer), and per-draw glyph c3 translations now VARY per draw (ring scan
   had been smearing one stale matrix). PS alpha path: PS c0=white; fade
   constant c9.w=1e5 at steady state (capture-time c9=0 was early-boot).
5. **Press-A text carve chain fully understood**: PS cd714e30 alpha =
   smoothstep(sat(|dot(pos,c9)|*0.02/w)) * coverage * c0.w — c9 zero ⇒
   invisible glyphs even though they rasterize (4668 samples passed).
6. **Next steps (Unleashed-guided, in order)**: (a) user A/B of the emitter
   hook build; (b) turn OFF ring-scanner applies (kScannerApplies=false) once
   (a) holds — the scanner's stale-ring misparses still poison other regs;
   (c) hook `D3DCommandBuffer_SetShaderConstantF` + `CreateShaderConstantFFixup`
   for the precompiled-CB constant path (car per-object WVP/materials
   candidates); (d) hook `SetPending_AluConstants` to mirror the dirty-mask
   flush instead of parsing its output. Car MODEL absence also gated by the
   separate unpopulated-vertex-pool finding (2026-07-01) — constants alone
   won't make the car appear.
7. Temp diagnostics still in tree: FM2_GLYPHMTX_CAP/_DRAW (with c9/c10/cov),
   FM2_C0WRITE/_C0DUMP, FM2_GPUBEGINCONST, FM2_RANGESNAP. Strip once
   transport settles.

### 2026-07-03 later: glyph-mesh jumble root-caused + ranged snapshot (parked)

- **User-verified: GpuBegin hook fixed glyph placement** (letters sit where
  they belong). Remaining: each letter's own mesh jumbled/not legible.
- **Root cause of the jumble (proven from fm2pressstart3.rdc)**: geometry
  reconstruction of glyph draw 146 from the captured buffer produces garbage
  under EVERY half-swap/alignment hypothesis => the captured bytes don't
  match their own indices. The pool is written INCREMENTALLY through the
  frame while `SetStreamSourceGuestData`/`SetIndicesGuestData` cache the
  upload ONCE PER FRAME keyed by pool pointer -- every draw after the first
  renders from a stale snapshot. Same mechanism very likely = the car pool
  "wholesale unpopulated at draw time" finding (2026-07-01).

### 2026-07-03 latest: draw-time ranged snapshot (built 12:10, awaiting A/B)

- **fm2pressstartbadglyphs.rdc (10:47) analyzed in depth** — it is the
  PRE-fix state and confirms the mechanism quantitatively: all 13 glyph
  draws (EID 135..202) bind the SAME whole-pool VB window (arena offset
  368384, 869040 B, stride 8) with 0-based per-glyph index blocks
  (startIndex per draw, NOT monotonic in draw order => the game caches
  per-glyph index topology persistently while rewriting VERTEX data per
  frame). Plotting each draw's indices against the captured window is
  scrambled for EVERY draw (even the last-drawn S), and a coherence search
  over the first 64KB of the pool finds NO shift that reproduces any
  letter => the snapshot content is a mid-frame mixture that matches no
  glyph. The quad draws (127/210-245) in the same capture DO show small
  576-B per-draw windows = the bind-side FM2_RANGESNAP covered quads but
  the glyphs bypass it (API-level SetStreamSource*Data path, not the PM4
  bind hook).
- **Fix moved to draw time**: `TryDrawTimeRangedSnapshot`
  (render_state.cpp, kEnabled=true) re-slices LIVE guest bytes per
  indexed draw (single-stream slot 0, guest-data-backed), uploads only the
  referenced window + rebased indices, draws startIndex=0/baseVertex=0.
  Bind-side variant in BindPm4GeometryFromContext gated OFF
  (kPerDrawIndexedRangeSnapshot=false) so only one mechanism runs.
  Built into the 12:10 exe; log-verified firing (FM2_RANGESNAP2, incl.
  glyph-shaped windows whose first record = the T-block first vertex
  AC12 3A83 = BE halves (-0.0635, 0.8139)). **Verification pending: user
  press-A legibility check on the >=12:10 build; a fresh capture should
  show glyph draws with ~1-6KB VB windows and startIndex=0 instead of the
  869KB whole-pool bind.** Known scope gap: draws bound via the
  SetStreamSource OBJECT path (g_guestStreamRef cleared) and multi-stream
  draws still use the per-frame cache and can still be stale (car pool).
- **Quad texture note (bindless)**: the A-button/background/logo quads'
  PS (ResourceId 894) has no SRVs in reflection — textures ride as
  descriptor indices in SharedConstants (238: [33,25,26]) per the
  UnleashedRecomp-style bindless layout, so "no SRV bound" is NORMAL here;
  whether index 33 resolves to the expected texture (user expects 1094 for
  the A button, 903 bg, 898 logo) needs its own check in a fresh capture.

### 2026-07-03 part 3: GLYPH JUMBLE TRUE ROOT CAUSE — dropped per-draw
### vertex-fetch base (OffsetInBytes). FIX BUILT 14:11, log-verified.

- User reported the draw-time snapshot changed nothing and provided the
  GOOD reference `fm2pressstartxenosgoodglyphs.rdc` (same build, xenos CP
  backend). Its translated VS pulls vertices via fetch constant CB1[47]
  (= vfetch 95): addr = idx*8 + (CB1[47].z & ~3). Dumping every draw's
  fetch table shows **each glyph has its own persistent vertex base**
  (T=0xA939028, O=0xA933648, S=0xA937788, A=0xA92A7F0, R=0xA9361B0,
  P=0xA9345E8, E=0xA92E260; repeated letters SHARE their base across draws
  and frames) — a persistent per-glyph cache, index blocks byte-identical
  to plume's.
- **The plume pool snapshot was never stale**: the good capture's T vertex
  block was found BYTE-IDENTICAL inside the bad capture's own uploaded
  pool at +164,488 bytes past the bound base (arena 532872 vs bound
  368384), and plotting it with the T's indices yields a perfect letter T
  (box-minus-counterform carving). Only the BASE was wrong.
- **Mechanism** (IDA, 0x82370E48 D3DDevice_SetStreamSource): the glyph
  renderer FM2_RenderContext_ComputeVertexLighting (0x827BAC98, the
  lr=0x827BB01C GpuBegin caller) calls SetStreamSource PER GLYPH with a
  per-glyph OffsetInBytes into the shared pool VB. The XDK writes the live
  fetch constant base = vb->format0 + OffsetInBytes to **device word
  +0x6F8** (stream 0; +0x6FC = shrunken size; stream s at word
  412+2*(17-s), i.e. 0x670..0x6FC), but the resource pointer at +0x2F94
  keeps only the OFFSET-LESS header. Both plume transports read the
  offset-less base (SetStreamSourceNative: header format0; PM4 hook:
  vbRes+0x18) => every glyph bound the pool base. The xenos CP consumes
  the real fetch constant => correct.
- **FIX** (render_state.cpp TryDrawTimeRangedSnapshot,
  `kUseLiveVertexFetchBase=true`): read the LIVE stream-0 fetch constant
  from device+0x6F8/+0x6FC at draw time, TranslatePhysical it, window from
  THAT base. FM2_RANGESNAP2 now logs fetchBase; smoke run (fm2.exe 14:11)
  shows glyph-sized draws with VARYING per-draw bases (0x0A56CC58,
  0x0A529348, ...) vs the single frozen base before. **Awaiting user
  press-A legibility check.**
- Likely same fix family applies to the car pool: BindPm2GeometryFromContext
  reads vbRes+0x18 (offset-less) — switching it to the ctx+0x670..0x6FC
  fetch words is the analogous follow-up (not done yet; one variable at a
  time).
- Textures-not-visible on the quads: still open, untouched by this fix;
  needs a fresh capture to chase the bindless descriptor indices.

### 2026-07-03 part 4: car-pool live-fetch + texture aperture-hijack fixes

- **Car pool**: BindPm4GeometryFromContext's whole-pool fallback now prefers
  the live ctx fetch words (ctx + 1648 + 8*(17-s), toggle
  kUseCtxLiveVertexFetch) over the offset-less vbRes+0x18;
  TryDrawTimeRangedSnapshot extended to MULTI-STREAM (windows every
  guest-backed stream from its own live fetch constant; bails if any bound
  stream is object-backed). Log-verified per-draw varying fetchBase0
  including float32 geometry.
- **Texture root cause (press-A background/A-button/logo black)**: sampler
  slot 0's fetch (the BC1 art at 0x10578000-style bases, IDENTICAL fetch
  words to the good capture's tfetch0) was hijacked by a MISREGISTERED
  resolve destination: FM2_RESOLVE_APERTURE dest=0x2E023900
  dataBase=0x0AAE0000 registered the art's page -> ApplyLiveTexturesFromContext
  bound a black RT surface instead of the art, every frame. The good capture
  proves no resolve ever writes the art page (the sampled bg is BC1; resolves
  cannot produce BC). Fixes: (1) aperture registry entries now carry
  resolveDest (only Resolve-hook exact bases; broad Fm2BindSurface scan
  entries serve the present path only via LookupSurfaceAperture;
  LookupResolveSurfaceAperture for sampling); (2) **BC gate**: the texture
  binder translates the fetch first and NEVER lets an aperture entry shadow
  a block-compressed texture; (3) Resolve hook refuses BC-format
  destinations (FM2_RESOLVE_BC_DEST_SKIP). Note: the early-frame
  CopyTextureRegion events into BC textures in captures are OUR OWN texture
  uploads (benign), not resolve corruption -- earlier theory retracted.
- **Verified via FM2_LIVE_TEX (now periodically re-armed, logs bc= flag)**:
  slot 0 = bc=1 tex bound at 0x10578000 (art) at press-A steady state.
  Glyphs legible on screen. **Background STILL black** => remaining break is
  downstream of binding: SharedConstants descriptor index at draw time,
  BC1 alpha/blend (punch-through alpha=0 * blend?), or per-draw state
  timing. Next: fresh capture on this build -- check bg draw's
  SharedConstants tex index, pixel-history shader output (art color vs
  black), and blend state.
- ALSO: the misidentified Resolve dests (0x2E023900 etc. giving art pages,
  and 0x4001Dxxxx = render-context fields) mean the Resolve hook's
  destination decode (+32 word) is unreliable for some call shapes --
  worth RE'ing D3DDevice_Resolve's parameter forms in IDA.

### 2026-07-03 part 5: quad PIXEL SHADER is wrong/smeared (textures still
### black with correct binding)

- fm2pressstart5.rdc (BC-gate build): bg draw 132 binds the art correctly
  (SharedConstants [34,29,30]) but the PS STILL outputs (0,0,0,1).
- **The good capture proves the quads use DIFFERENT pixel shaders per
  draw**: bg + shadow quads = {d88fa0db} (textured tint-chain multiplying
  by PS float constants CB4[0..26]); A-button = {2e856b2a} (glyph family).
  Plume binds ONE PS (ResourceId 894, declares NO float-constant block) for
  ALL quad draws => background/A-button/logo compute black; shadow quads
  only look right because black is their correct output.
- **Live-PS sync implemented** (d3d_hooks.cpp ApplyLivePixelShaderFromContext,
  kApplyLivePixelShader=true): D3DDevice_SetPixelShader (0x8236DD10)
  installs the current PS object at ctx+0x307C (IDA-confirmed); the sync
  reads it per draw in both SubmitNativeIndexedDrawPm4 and
  Fm2EmitIndexedDrawPm4Base and calls SetPixelShaderNative. VERIFIED
  firing with per-draw VARYING pointers (0x2E0034E0 / 0x2E017500 /
  0x4005D1B0) -- but screen unchanged (text fine, textures still black).
- ~~Next hypothesis: ResolveShader returns NULL~~ WRONG — the aliases
  resolve fine (FM2_LIVE_PS res= non-null, guest objs 0x2E017500/0x4005D1B0
  -> shaders hash 0x4A548329074CC3CA / 0x9E93B37448CA0172).
- **TRUE CAUSE OF THE STUB PS: SHADER CACHE MISS.** The resolved
  GuestShaders had shaderCacheEntry==nullptr -- their translations were in
  `missed_shaders/` (156 dumps incl. 4A548329074CC3CA = the background
  tint-chain PS), so LoadShader fell back to the stub (PS 894).
  **FIX APPLIED**: copied build-dir `missed_shaders/*.bin` into
  `FM2/assets/missed_shaders/`, reran
  `scripts/fm2/Update-FM2ShaderCache.ps1 -SkipBuild` (XenosRecomp over
  FM2\assets; 3 dumps skipped w/ recompiler exceptions; cache = 213
  entries incl. the quad shaders -- most missed hashes were already IN the
  regenerated cpp, the RUNNING EXE had an older embedded table), rebuilt
  fm2. VERIFIED: **zero "Shader cache MISS" lines** in the new run;
  FM2_DRAW_OUTCOME ok=okPS, skip=0, create_fail=0.
- **Remaining (final layer): PS float constants are ZERO.** The real
  tint-chain PS now runs and still outputs black => its CB4[0..26]
  multipliers never arrive. This is the long-planned precompiled-CB
  constant transport: hook D3DCommandBuffer_SetShaderConstantF 0x823767b8 +
  CreateShaderConstantFFixup 0x823766e0 + SetPending_AluConstants
  0x82382cc8. A fresh capture (with the real PS bound, RenderDoc can now
  decode its float CB slot) will show exactly which registers are zero.
- WORKFLOW: when adding new title-side shaders, remember BOTH steps:
  regenerate shader_cache.cpp AND rebuild fm2 (the cache is embedded in
  the exe; a stale exe keeps missing even when the cpp has the entries).

### 2026-07-03 part 6: UI quads are IMMEDIATE (UP) draws; live state now
### applied there too. Final remaining gap = PS float constants.

- fm2pressstart7.rdc still showed the stub-reflection PS on the bg draw:
  because the UI quads are NOT indexed PM4 draws at all -- they are
  BeginVertices/EndVertices IMMEDIATE triangle fans (18 verts, 48
  generated fan indices, VB = uploaded UP staging) flowing through
  FlushImmediateVertices -> rr::DrawPrimitiveUP, which bypassed ALL the
  per-draw live-ctx appliers.
- FIX: FlushImmediateVertices now runs ApplyLiveTexturesFromContext +
  ApplyLivePixelShaderFromContext on the pending draw's device before
  DrawPrimitiveUP (re-entrancy safe: p.device nulled first).
- Current state after the fix build: text pulses with fade (constant
  modulation live), background/A-button/logo still black. All PSOs load
  (FM2_PS_LOAD ps_ok, zero failures; zero shader-cache misses).
- ~~NEXT: wire the three constant hooks~~ SUPERSEDED by pt7 below (the
  transport is the PLAYED-COMMAND-BUFFER channel, hooked at the submit
  chokepoint instead).

### 2026-07-03 part 7: played-command-buffer transport decoded + constants
### scanner wired through nested INDIRECT_BUFFERs. PS program = IM_LOAD.

- **Mechanism fully decoded (IDA + live packet dumps)**:
  - D3DCommandBuffer_SetShaderConstantF (0x823767b8) PATCHES float values
    into a precompiled PM4 blob via FixupRecords (dst = blob payload base +
    fixup offset). The blob later executes via
    D3D_SubmitCommandBuffer (sub_82372920: r5=segment guest addr, r6=size
    dwords) -> FM2_D3D_SubmitCommandBufferChain, which writes a PM4
    **INDIRECT_BUFFER (0xC0013F00)** into the primary ring. The pre-draw
    ring scanner NEVER followed IBs => everything inside played buffers
    was invisible to the constant shadow. Capture-8-era conclusion that
    ctx+0x307C holds the wrong PS for UI quads: correct -- the real
    material PS never goes through D3DDevice_SetPixelShader at all.
  - Submitted segments contain FURTHER nested IB calls (FM2_CBDUMP
    n=16397: three 0xC0013F00 packets = the material buffers).
  - Inner material buffers carry the SHADER PROGRAMS as
    **IM_LOAD_IMMEDIATE (op 0x2B, INLINE Xenos ucode; dword0 0=VS 1=PS)**
    (+ SQ_PROGRAM_CNTL type-0 writes at 0x2180) -- FM2_CBDUMP_IN k=4098.
    Presumably IM_LOAD (op 0x27, by address) for larger shaders.
- **Landed (fm2.exe ~16:3X)**: REX_HOOK_RAW(sub_82372920) +
  ScanSubmittedSegment (d3d_hooks.cpp): packet-structured walk of every
  submitted segment, recursing into nested IBs (depth<=3, <=256KB), calling
  ScanPm4AluConstantRange per level so SET_CONSTANT floats (VS+PS) land in
  the shadow in record order. Verified firing (FM2_CBSUBMIT n>81k, real
  segments 11-31 dwords; nested dumps FM2_CBDUMP_IN). Screen: text crisp,
  background still black.
- ~~REMAINING: map played buffers' programs~~ Route (a) IMPLEMENTED and
  the result DISPROVES the theory: the ucode-container registry +
  FindShaderByInlineUcode/FindShaderByUcodeAddress + IM_LOAD(0x27)/
  IM_LOAD_IMMEDIATE(0x2B) handling in ScanSubmittedSegment all landed
  (with a g_cbPsFresh newest-writer gate vs the ctx+0x307C apply), and a
  full boot->press-A run shows the ONLY program loads in played buffers
  are ONE tiny 24-dw VS + 9-dw PS pair (the XDK's runtime-generated EDRAM
  resolve helpers, correctly matching no container; zero op-0x27 at all).
  **Played command buffers do NOT carry the UI material shader programs.**
- **pt8 conclusion / next session:** the quads' PS in the plume path is
  whatever ctx+0x307C holds (guest objs 0x2E017500/0x4005D1B0 -> cached
  translations incl. hash 0x4A548329074CC3CA) -- and its translation shows
  NO float-constant block, while xenia's same-position draw runs the
  CB4-heavy tint chain d88fa0db. Capture-8 pixel history: the plume "bg"
  draw (127) outputs constant (0,0,0,1) even over the art's brightest
  pixels => it behaves as a SOLID FILL, not a sampler. Hypotheses to
  discriminate NEXT: (1) plume's 21-draw frame vs the good pass's 19 draws
  -- plume's 127 may correspond to the good backend's black UNDERLAY quad
  and the true art-sampling draw is a different 48-idx quad (compare
  SharedConstants texture indices per quad draw against the FM2_LIVE_TEX
  slot->texture log; find which plume draw has the art's descriptor and
  what IT outputs); (2) plume forces FM2 down a SIMPLIFIED direct-draw
  path whose shaders differ from the xenos material path by design --
  check whether 0x4A54.../0x9E93... containers correspond to
  'fallback pass' shaders (FM2_Render_BuildFallbackPassCommandBuffers).
  (3) If the simple PS modulates by an interpolated vertex color, black
  may come from the VS constants/vertex color -- shader-debug the bg
  pixel in RenderDoc (qrenderdoc GUI, user-driven) for ground truth.
  TEMP diags to strip later: FM2_CBDUMP, FM2_CBDUMP_IN, FM2_CB_IMLOADIMM
  signature histogram.
- **Fix implemented: ranged per-draw snapshot** in
  `BindPm4GeometryFromContext` (`kPerDrawIndexedRangeSnapshot`): scan the
  draw's index slice, upload only the referenced vertex window
  (SetStreamSourceHostWindow) + rebased indices (SetIndicesPreparedHost),
  draw with startIndex=0/baseVertex=0. Verified engaging (FM2_RANGESNAP,
  windows 576B-8KB). Whole-pool per-draw re-upload was tried first and
  OOM-killed the process; the per-frame caches stay for the fallback path.
- **"Intermittent silent death" RESOLVED as user-closes**: the game windows
  my headless verification runs popped on the user's desktop were being
  closed BY THE USER -- the survival A/B statistics were meaningless.
  `src/ui/window_win.cpp` WM_CLOSE/WM_DESTROY now logs
  `REXUI_WINDOW_CLOSED_BY_USER ... NOT a crash` to the debug log (verified by
  simulated X-press); always check for it before calling a vanished process
  a crash. Also learned: after an SDK install, fm2.exe MUST be rebuilt or it
  exits instantly at boot (fresh-DLL/old-exe mismatch).
- **`kPerDrawIndexedRangeSnapshot` RE-ENABLED (true)** with the small-window
  caps and the DrainGpuBeginConstants QueryRangeAccess guard (a real hazard
  fix regardless). Awaiting user press-A letter-legibility verdict.

## Parked / open

- Implement + verify the glyph-placement-matrix-shadow fix (next step above).
- 3D car flash (same multi-writer register contention on the per-object WVP).
- Memexport emulation in plume_native (large; gates whatever the 24 passes
  feed — likely skinning/particles/3D geometry generation).
- Memory leak (RT/depth churn), `FM2_Audio*` misname cluster.
- Clean up temp diagnostics once the constant path is settled.
