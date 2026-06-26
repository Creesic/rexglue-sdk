# FM2 plume_native — Not-Yet-Wired Checklist

Date started: 2026-06-25
Scope: tracking what remains unimplemented in the `--fm2_plume_mode plume_native`
native renderer, after the session that took FM2 from a fully black screen to
menu item panels rendering.

This complements `FM2-native-renderer-gap-analysis.md` (architecture) and the
auto-memory note `project_plume_native_black_screen.md` (debugging narrative).

## What now works (baseline, this session)
- **Texture data residency** — read streamed texture data via the *physical*
  memory view (`Memory::TranslatePhysical`), gated on the physical heap's commit
  state — NOT the protected virtual alias `ghp::ToHost` (which is no-access).
  This is how the xenos backend reads all guest GPU memory. (d3d_resource_hooks.cpp
  `UploadGuestTextureData`.)
- **Shader constants** — read VS constants from `device + 0x700` and PS from
  `device + 0x1700` (Forza's real ALU constant file base), not the GuestDevice
  struct fields at `+0x780/+0x1780` which are 8 registers too high. Fixed the
  zeroed transform matrices that collapsed 3D geometry. (render_state.cpp
  `FlushRenderState` constant upload.)
- **Resolve-dest aliasing** — `ctx+10652` (RB_COPY_DEST_BASE) gives the resolve
  destination guest address; aliased to the source plume render target so
  fetch-constant samples bind rendered surfaces. (d3d_hooks.cpp `Fm2EmitSurfaceResolve`
  + `g_surfaceAperture`.)

## NOT wired yet — rendering (priority order for visible/navigable menus)

1. **Multi-pass composite execution.** ← TOP BLOCKER for menus (definitive).
   The menu renders in LAYERS (a 1280x256 panel strip `317` + a text surface +
   a background) that the game composites into a final image via GPU resolve/blit
   we don't execute. RULED OUT: present-source selection CANNOT fix this — the
   composited image does not exist on ANY surface at present time (layers are
   recycled/overwritten; only the strip retains content). Tried preferring the
   VdSwap front buffer (aperture lookup + physical read) — no effect, because
   (a) VdSwap isn't the active present path here (the present source is set in the
   GpuCmdBuf hook d3d_hooks.cpp:~590 = `GetLastDrawnColorRenderTarget`), and more
   fundamentally (b) the front buffer / composite surface is never populated (the
   composite/resolve doesn't run). THE FIX: actually run the composite — snapshot
   each rendered layer surface at resolve time (so it isn't recycled) and let the
   composite draws (which sample those layers) produce the final image, then
   present that. This is the EDRAM-resolve work: `SnapshotSurfaceForResolve`
   exists but crashed doing a mid-frame copy; needs a safe-timing redo (copy when
   no render pass is active / correct thread). Everything else (textures,
   constants, geometry, font/glyph data) is ready and feeds into this.

   NOTE: Font/text is NOT the blocker. The glyph atlases (256x128, 1 byte/px,
   tiled) go through the fetch-constant path and ARE populated + uploaded via the
   physical-view read (confirmed: 1628 `FM2_TEX_SRC` hits, ~50-95% non-zero glyph
   data; `FM2_FONT_UNLOCK`=0 so Lock/Unlock isn't used). The earlier
   "FontTex (ResourceId::249) is black" was a stale/default texture, not the real
   atlas. Text will appear once the composite presents the text layer.

2. **(was multi-pass composite — merged into #1 above)**
   The menu renders in pieces (a 1280x256 UI strip + 720p surfaces) that the game
   composites into a final image via GPU resolve/blit we don't translate. We
   present whatever was last drawn (`GetLastDrawnColorRenderTarget`) — often a
   partial strip — stretched to fullscreen. Need to present the final composited
   display buffer (VdSwap front buffer) and/or perform the composite.

3. **EDRAM resolve — snapshot/freeze.**
   Resolve aliasing currently points at the *live* plume RT, which the game
   reuses/overwrites across passes, so a post-process samples the wrong (later)
   contents. `SnapshotSurfaceForResolve` (render_state.cpp) exists to freeze a
   copy at resolve time but CRASHED doing a mid-frame copy from a bound RT; it's
   reverted/unused and needs a safe-timing redo (copy at a frame boundary / when
   no render pass is active).

4. **Created-texture (Lock/Unlock) data upload.**
   Textures created via `D3DDevice_CreateTexture` + `Lock`/`Unlock` (incl.
   FontTex) don't reliably get their CPU-uploaded data. Likely needs the
   physical-view read and/or correct staging->plume copy.

5. **Colorspace / gamma.**
   Present blit (`copy_ps.hlsl`) is a plain copy to an `R8G8B8A8_UNORM`
   swapchain — no gamma/sRGB handling. User observed it "looks like we're
   treating it as linear." Direction (encode vs decode) is ambiguous; pin with a
   capture before changing. Note the prior "UI overexposure" fix
   (`docs/FM2-rendering-notes.md`) was a TEXCOORD4 scale, a different issue.

6. **Tiled detiling correctness across Xenos formats.**
   `UploadGuestTextureData` handles a subset of formats (k_8, k_8_8_8_8, DXT1/2/3/
   4/5, k_16_16_16_16, k_16_16_16_16_FLOAT) and a tiled path; unverified for all
   formats/endian/packed-mips the game uses.

7. **Texture upload caching / performance.**
   Fetch-constant textures are translated/uploaded per draw; ring-buffered
   surfaces (base changes per frame) miss the alias cache and re-upload every
   frame (heavy GPU traffic). Needs reuse/invalidation by content, not just base.

8. **MSAA resolve.** FM2 renders 720p 2xMSAA (per `FM2-rendering-notes.md`);
   MSAA surface creation + resolve handling not verified in the native path.

9. **Bink video playback.** Intro videos (Turn10 / Microsoft / Forza) render
   black — no Bink decode -> texture -> blit path in plume_native.

10. **3D scene / gameplay rendering.** Only menu screens partially render;
    in-game 3D (car/track) not reached or verified.

## Session update (2026-06-25, late) — composite snapshot working, fidelity gaps remain
- **Snapshot-on-resolve now works WITHOUT crashing.** Root cause of the earlier
  crash: (a) `setFramebuffer(nullptr)` mid-frame corrupted the command list, and
  (b) the MSAA `resolveTexture` path caused device-removed on a sample/format
  mismatch. Fix: removed the `setFramebuffer` call (barriers end the pass like
  `ExecutePendingStretchRects`) and guard `SnapshotSurfaceForResolve` to only
  snapshot **single-sampled** sources via `copyTexture` (MSAA sources fall back to
  live aliasing). Result: no crash, **384 aperture hits** (composite passes DO
  sample the frozen snapshots). But the visible output is unchanged ("same strip")
  — the composite is connected but its result still isn't what we present.
- **Present analysis:** dominant render target AND present source is a single
  host surface `130C7F000` (895 draws). The menu renders to it; we present it.
  So it's not a simple wrong-surface pick — the full menu content isn't landing on
  the presented surface (or the presented surface IS a sub-layer strip).
- **DXT5 UI textures load fine:** the 1628 populated 256x128 textures are
  **BC3_UNORM/DXT5** (RenderFormat 61), i.e. UI icon/images — NOT the font. They
  upload via the physical-read path.
- **Font (FontTex, 256x128 R8) still black** and filled by a path we don't
  handle: NOT Lock/Unlock (FM2_FONT_UNLOCK=0), NOT the fetch-constant upload
  (no R8 256x128 in FM2_TEX_SRC). `CreateTexture` already makes it a RENDER_TARGET,
  so glyph-render draws *could* target it, but they're not reaching it — the font
  fill path is unidentified (needs the glyph-render draws traced).
- **Tooling blocker:** menu DRAW frames aren't being captured — RenderDoc lands on
  the blit-only present frame because draws (tid 94488) and present (tid 48944)
  are on different threads. Need a capture that spans the menu draws to make
  efficient progress on text/fidelity.

## NOT wired yet — game progression / non-render

11. **Producer-guard loading hang.** `FM2_ProducerProgressGuard_82369340` is a
    consumer spin-wait that can starve the producer/streaming thread. Mitigation
    applied: `fm2_prod_guard_wait_yield_interval=1` (fm2_hooks.cpp) so the
    consumer yields. Streaming works (assets load) but reliable progression to
    gameplay is unverified; may need the real producer fix, not just yielding.

12. **Sign-in / XAM / save / profile.** Not confirmed as blockers but classic
    boot gates (see `docs/` XAM/sign-in notes). Revisit if a screen waits on them.

13. **Audio (FMOD / XAudio2).** Separate subsystem with its own in-progress work
    (`FM2_FMOD_*` telemetry, `FM2-xaudio2-conversion-guide.md`); not part of the
    render path but listed for completeness.

## Diagnostics left in the tree (toggles)
- `g_showPresentTestGrid` (video.cpp) — on-screen test grid overlay (off).
- `g_wireframeMode` (render_state.cpp) — wireframe all geometry (off).
- `fm2_prod_guard_stats` / `fm2_prod_guard_trace` cvars — producer-guard logging (off).
- Many `FM2_*` clean-log traces (FM2_TEX_UPLOAD/SRC, FM2_LIVE_TEX, FM2_RESOLVE_ALIAS,
  FM2_VSCONST2, etc.) — throttled, write to `C:\temp\fm2-clean.log`.
