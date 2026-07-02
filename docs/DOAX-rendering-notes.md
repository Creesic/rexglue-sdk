# DOAX Rendering Notes

## Pool Float / Near-Player Black Assets

Observed in the pool scene with `C:\Users\Tera\Documents\GitHub\renderdoccaps\doaxgood.rdc`:

- The black float draw samples `Texture 3608`, a 512x512 `k_8` texture backed by guest memory at `0x1BD7C000`.
- The texture is loaded from shared memory after a `Resolve Copy Full 8bpp` pass.
- The resolve source is a `k_8_8_8_8` render target at EDRAM base `0x0`; the source alpha channel contains the mask data while RGB is mostly zero.
- The old `resolve_full_8bpp` shader loaded only red or blue for R8 output, so the resolved texture became black.

The fix is to make the full 8bpp resolve write source alpha into the R8 destination. If the generated shader bytecode is refreshed from Xenia sources, apply the equivalent source change in `resolve_full_8bpp.xesli`: load eight RGBA pixels, build the two `float4` R8 pack vectors from `.a`, and regenerate both D3D12 and Vulkan `resolve_full_8bpp*` bytecode headers.

## Pool Scene Full-Screen Black Transition

This is separate from the K8 pool-float bug above. The K8 fix repairs black near-player assets; the full-screen blackout is a later guest-output publish problem.

Observed with `C:\Users\Tera\Documents\GitHub\renderdoccaps\doaxblack.rdc`:

- The final presenter draw sampled the normal guest-output texture, but that source texture was already black. The final presenter shader was not the source of the blackout.
- Runtime logs around the black transition dropped from normal scene frames in the 1200-2400 draw range to tiny transition frames: `draws=36/37/38`, `indexed=0`, `indices=63/127/191`, `copies=6`.
- Gamma diagnostics stayed neutral (`lvl=1.000`, `scale=0.000`, `bias=0.000`), so the black was not a gamma fade.

The D3D12 runtime has a DOAX-title-gated guard (`d3d12_doax_hold_low_draw_guest_output`) that keeps the previous presenter guest output when this low-draw signature appears after a full scene has already been published. This prevents publishing the black transient while the title-side source state is still being traced.

Verification run `doax_012*.log` showed:

- `full_scene_swaps=704`
- `low_signature_held_stats=136`
- `low_signature_published_before_first_full=113`
- `low_signature_published_after_first_full=0`
- First full scene at `2026-06-30 10:28:50.979`: `draws=1302`, `indexed=1154`, `d3d12_submit=4`, `swap_submit=1`.
- First held low frame at `2026-06-30 10:28:52.690`: `draws=36`, `indexed=0`, `indices=63`, `copies=6`, `swap_submit=0`.
- Last held low frame at `2026-06-30 10:29:05.420`: `draws=38`, `indexed=0`, `indices=191`, `copies=6`, `swap_submit=0`.

The current diagnostic build logs the held-frame frontbuffer pointer plus title menu/present bytes (`present`, `target`, `overlay`, `mcase`, `mloop`, `sphase`, `smode`, `sctdown`, `sff`), `sprite_busy`, and texture fetch 0 when the hold fires.

Run `doax_014*.log` captured the diagnostic fields:

- First held frame at `2026-06-30 10:56:31.477` (`doax_014.1.log:4201`): `draws=37`, `indexed=0`, `indices=127`, `copies=6`, `swap_submit=0`.
- Title state on the held frames: `present=4`, `target=4`, `overlay=0`, `mcase=3`, `mloop=255`, `sphase=2`, `smode=2`, `sctdown=0`, `sff=0`, `sprite_busy=1`.
- Texture fetch 0 alternated with the frontbuffer: `8A000002 1F577006 0059E4FF 00001414 00000000 00000200` and `8A000002 1F90F006 0059E4FF 00001414 00000000 00000200`.
- Gamma remained neutral around the event (`lvl=1.000`, `scale=0.000`, `bias=0.000`).

Capture `C:\Users\Tera\Documents\GitHub\renderdoccaps\doax_2026.06.30_16.03.47_frame5764.rdc` is a complete black-present capture:

- Present EID `3463` presents `ResourceId::316` (`Swapchain Image 316`), which is 99.4% near black.
- The swapchain image is written only in the presenter pass: EID `1849`, `1861`, and `1863`.
- EID `1849` is the full-viewport presenter draw. It samples `ResourceId::2311`, a 1280x720 `R10G10B10A2` texture, and writes `ResourceId::316`.
- `ResourceId::2311` is already 100% black at EID `1849`, and RenderDoc reports no writer for it inside this capture. The final presenter shader is not creating black; it is displaying a black guest-output texture.
- EIDs `1861` and `1863` draw small overlay/image content from `ResourceId::313`, which explains the tiny non-black region on the otherwise black present.
- Real scene content exists elsewhere in the capture: `ResourceId::2250` (`RT @ 0t, <16t>, 1xMSAA, k_8_8_8_8`) contains normal image content at EID `1825`. The break is between the resolved EDRAM render target and the presenter guest-output texture.

Correlated with `doax_015*.log`, this capture filename's `frame5764` lands near CP stats frames `5748`/`5781`, which are low-draw published frames (`draws=35`, `indexed=0`, `indices=61`, `copies=5`, `swap_submit=1`) before the first full scene. In that run the first full scene is `frame=18185` at `2026-06-30 12:04:06.325`, and the post-full held low-draw transition begins at `frame=19704` at `2026-06-30 12:04:07.799`.

Capture comparison `C:\Users\Tera\Documents\GitHub\renderdoccaps\doax_2026.06.30_16.03.47_frame5631.rdc` vs `C:\Users\Tera\Documents\GitHub\renderdoccaps\doaxgood.rdc` pins the branch point:

- The captures are effectively aligned through the last common draw: EID `5235` in `frame5631.rdc` corresponds to EID `4832` in `doaxgood.rdc`.
- After that draw, `frame5631.rdc` does not continue into the normal scene path. It closes the pass and runs only a tiny presenter/overlay tail: reset/begin pass, one `DrawInstanced`, two small indexed draws, then `Present`.
- In the good capture, the matching point is followed by the missing depth/scene branch: command list close, setup/copy work, EID `4895` `ClearDepthStencilView`, then EID `4904` begins a depth-only pass into `RT @ 736t, <8t>, 2xMSAA` using the earlier `RT @ 736t, <8t>, 4xMSAA` depth as a pixel resource.
- That 2xMSAA depth target exists in `doaxgood.rdc` (`640x2048`, `D24S8`, first write at EID `4904`) and is absent from `frame5631.rdc`.
- The later large indexed scene pass in `doaxgood.rdc` depends on this branch. The bad capture is therefore a skipped command-stream branch, not a final-present shader failure.

Occlusion-query hypothesis, 2026-06-30: DOAX may gate some rendering on occlusion query visibility, but the latest `doax_003*.log` run did not support this as the cause of the observed full-screen black. Neither `--no-d3d12_doax_fake_occlusion_queries` nor `--no-occlusion_query_enable` changed the behavior, no `EVENT_WRITE_ZPD` / `ZPD` path appeared in the latest logs, and the DOAX fake-query diagnostic did not log.

Relevant runtime path:

- `PM4_EVENT_WRITE_ZPD` reaches `D3D12CommandProcessor::ExecutePacketType3_EVENT_WRITE_ZPD`.
- With `occlusion_query_enable=true`, D3D12 uses host occlusion queries and writes the real host sample count via `EndGuestOcclusionQuery`.
- With `occlusion_query_enable=false`, D3D12 falls back to `CommandProcessor::ExecutePacketType3_EVENT_WRITE_ZPD`, which writes `query_occlusion_fake_sample_count` (`1000` by default) when the guest query has the finished sentinel.
- The DOAX-only D3D12 cvar `d3d12_doax_fake_occlusion_queries` is diagnostic-only and defaults off. It routes DOAX title ID `0x544307D2` through the same fake-count base path without disabling host occlusion queries globally, but it is not implicated by the latest logs.

Test toggles:

- DOAX fake-query diagnostic on: `--d3d12_doax_fake_occlusion_queries`.
- Global confirmation switch: `--no-occlusion_query_enable`.

Latest `doax_003*.log` evidence:

- First full scene: `2026-06-30 17:30:07.968`, `frame=7729`, `draws=1302`, `indexed=1154`, `swap_submit=1`.
- One post-full zero-indexed frame still published before the hold: `2026-06-30 17:30:10.829`, `frame=10672`, `draws=73`, `indexed=0`, `indices=235`, `copies=15`, `swap_submit=1`.
- The original low-draw hold thresholds (`draws<=64`, `indices<=256`, `copies<=8`) caught the later `draws=37/38` frames but missed `frame=10672` because both `draws` and `copies` were slightly above the limits.
- The D3D12 low-draw hold thresholds are now widened to `draws<=128`, `indices<=512`, `copies<=32` so that zero-indexed transition frame is held after a good full scene.

Latest `doax_001*.log` evidence from the pool scene showed a second full-screen black signature:

- Black intervals occurred with normal full-scene draw volume, not the low-draw signature: about `draws=1241-1246`, `indexed=1083-1091`, `indices=2239615-2256749`, `copies=31`.
- `DOAXSWAPSIG` reported the published guest output as fully black (`nz=0`, `rgb3min=0`) even though the scene command volume stayed high.
- The black frames were missing the late 1280-wide fullscreen composite draws seen on adjacent good frames. Normal good frames include 64-index draws using VS hash `5DA9246A2E51D5DF` and PS hash `9567C79307ACC6F5` or `E2CEF62EAE21A6AB`; recovery frames can use the same VS and PS `9567C79307ACC6F5` with a 4-index restore composite sampling fetch dword1 `18D9E006`.
- The D3D12 runtime has an opt-in DOAX-title-gated diagnostic guard (`d3d12_doax_hold_missing_final_composite`, default off). After a known-good composite has been seen, a full-scene swap that omits that final composite can be held for up to 120 frames. The log marker is `DOAX missing-final-composite hold`. This is not enabled by default because these frames can be intentional title-side black transitions.

Follow-up `doax_001.log` / `doax_001.1.log` from `2026-06-30 21:28` showed all logged swap memory signatures black (`frame=239` through `frame=792`, `nz=0`). That initially made the frontbuffer signature look like the best baseline, but later evidence showed this CPU-side memory sample is not synchronized with the GPU resolve and can read zero even when the command stream emits the final composite.

Follow-up `doax_001.log` from `2026-06-30 21:40` started already black and stayed black for all logged swap memory signatures (`381/381`, `frame=1088` through `1468`, `draws=1154-1257`, `indexed=997-1097`, `nz=0`). No hold fired because no baseline had been latched.

Follow-up `doax_001.log` from `2026-06-30 21:45` again started on a full-scene zero CPU-memory signature: first logged swap `frame=1084`, `draws=1238`, `indexed=1083`, `indices=2238103`, `copies=31`, `nz=0`, `rgb3min=0`. The 1280x720 color resolves to the frontbuffer still occurred. The baseline/no-baseline diagnostics now run even when `d3d12_doax_hold_missing_final_composite` is disabled; only the actual hold decision remains gated by the cvar.

Follow-up `doax_001.log` from `2026-06-30 21:51` showed the log can begin after the first black frames: `291/291` swap memory signatures in the retained file were zero, from `frame=1081` through `1371`, with no nonzero CPU-memory baseline in the retained window. The missing-final diagnostic now emits a periodic compact `DOAXMISSCHK` line and repeats the no-baseline marker every 120 persistent no-baseline frames after the initial burst, so a tail-only log still captures the guard state.

Follow-up `doax_001.log` from `2026-06-30 21:57` clarified the baseline problem: `DOAXMISSCHK` showed `final_comp_seen=1` / `final_comp_draws=7` on some full-scene frames while the CPU frontbuffer signature was still all zero. The frontbuffer signature is therefore diagnostic-only, not a valid good-output latch. The missing-final-composite guard now uses the command stream: a full-scene frame with the known final-composite draw latches the baseline (`DOAX final-composite baseline`), and later full-scene frames without that draw are held (`DOAX missing-final-composite hold`).

Follow-up `doax_001.log` from `2026-06-30 22:03` confirmed the command-stream guard was active in the retained window: `109/297` CP-stat frames had `swap_submit=0`, with transitions at frames `40343`, `40632`, `42629`, `44235`, `46163`, and `47763`. The compact `DOAXMISSCHK` samples showed `baseline=1`, `hold_enabled=1`, `final_comp_seen=0`, and `hold_frames=33/43`, so the held windows were full-scene frames missing the final composite. `DOAXSWAPSIG` remained all zero for `188/188` published swaps, reinforcing that the CPU frontbuffer signature is not a reliable visible-output signal.

Follow-up `doax_001.log` from `2026-06-30 22:17` again showed two full-scene held intervals (`130/367` CP-stat frames with `swap_submit=0`, starting at frames `44896` and `49471`). The retained file did not include the explicit `DOAX missing-final-composite hold` lines because that marker only emitted during the first 32 held frames of the process. The diagnostic now repeats the hold marker every 30 held frames and uses a separate cap-log counter so truncated logs still show the active suppression path.

Follow-up `doax_001.log` from `2026-06-30 22:28` confirmed the repeated marker: three intervals were held as missing-final-composite frames (`151/370` CP-stat frames with `swap_submit=0`, starting at frames `41495`, `44718`, and `48378`). Each interval emitted `DOAX missing-final-composite hold: keeping previous guest output frame=30` with `final_comp_seen=0`, `final_comp_draws=0`, and `present=4 target=4 overlay=1`, then recovered to `DOAXGATE black=0` before the 120-frame cap.

Visual check after the `22:28` run showed the hold produced a frozen last-good frame where the title should go black. The missing-final-composite hold is therefore a diagnostic/workaround only and now defaults off; the command-stream evidence remains useful for identifying these transition windows, but normal output should publish them.

Follow-up `doax_001.log` from `2026-06-30 22:34` verified the default-off change: `446/446` CP-stat frames had `swap_submit=1` and there were no `DOAX missing-final-composite hold` lines. Missing-final windows still appeared (`DOAXGATE black=1`, `DOAXMISSCHK hold_enabled=0 final_comp_seen=0`), but they were published instead of freezing the previous good frame.
