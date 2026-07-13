## Learned User Preferences

- When debugging FM2, confirm the debuggee is actually `fm2.exe` (x64dbg may be attached to `runner.exe` or another binary).
- Treat first-chance `0xC0000005` stops as normal rexglue VEH guest-memory handling unless the process unhandled-exits; prioritize real stacks over every AV stop.
- When the user pastes a VS crash stack, chase the concrete null-deref / faulting site they report rather than re-deriving from scratch.
- Use UnleashedRecomp’s GPU/render-thread architecture as the reference when prioritizing FM2 native Plume work (align gaps; don’t invent a divergent pattern). Stay away from porting more ReXGlue080plume FM2 tiling machinery (`ResizeTileSurface`, band destY rebasing, `g_tileViewportOffsetY`, `native_renderer` overlay).
- When the user says “commit then continue,” commit the finished chunk first, then resume the remaining work.

## Learned Workspace Facts

- FM2 is a separate CMake project under `FM2/` (not part of the root SDK build); run with `--game_data_root` pointing at extracted Forza 2 content (helper launch scripts may live under the FM2 build tree).
- The native Plume renderer lives in `FM2/src/render/` (cleanup folded away the old `native_renderer/` layout; do not re-port the dropped diagnostic overlay from sibling `ReXGlue080plume`).
- FM2’s primary texture-bind path is `D3DDevice_SetTexture` @ `0x8236C208`; fetch-bit helpers are secondary dirty-flag setters, not a replacement for SetTexture. Non-FM2-magic guest textures need `TranslateGuestTexture` before bind.
- Plume `createBuffer` / `createTexture` can fail (including `DXGI_ERROR_DEVICE_REMOVED` / `0x887A0005`) and yield null or objects with null `d3d`; callers must null-check before `map` / `createTextureView`.
- FM2 Plume uses a dedicated render thread with slim `RenderQueue`, 2-frame pipelining, `g_presentBusy` present coalesce, and a device-lost latch (`IsDeviceLost` / `NoteDeviceLost`); Unleashed’s target dispatch is POD `RenderCommand` + `Proc*` in `UnleashedRecomp/UnleashedRecomp/gpu/video.cpp`, while FM2 still mostly uses `std::function` enqueue/`Run`.
- Present source is `GetCurrentColorRenderTarget` (live RT → sticky `g_lastPresentableRenderTarget` → implicit); sticky can latch onto 1280×256 EDRAM tile RTs and upscale a black/wrong strip — prefer full-frame / resolve-assembled frontbuffer (aperture lookup), not Unleashed’s always-`g_backBuffer` model for FM2 compositing. Do not solve this by porting 080plume’s predicated-tiling state machine.
- Xbox D3DFORMAT low 6 bits are `GPUTEXTURE_FORMAT`; value 10 is `k_8_8` → Plume `R8G8_UNORM` (e.g. `0x2D20014A`), not `ColorRenderTargetFormat` 10 (`k_2_10_10_10_AS_…`); do not default unknown formats to `R8G8B8A8` when bpp/pitch differ.
- Architectural maturity reference for the native path is `UnleashedRecomp/UnleashedRecomp/gpu/`. Sibling `ReXGlue080plume` is historical/transfer context only — not the ongoing design target.
- OpenWolf context applies here: follow `.wolf/OPENWOLF.md`; durable session notes may live under `.wolf/`.
