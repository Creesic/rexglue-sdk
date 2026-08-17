# Migration from ReXGlue080plume

`ReXFM2P` is a clean-slate checkout on the current upstream SDK. The sibling repo
`C:\Users\Tera\Documents\GitHub\ReXGlue080plume` (branch `FM2_WIN_Plume`, remote
`Creesic/rexglue-sdk`) is an older SDK snapshot with ~2 months of FM2-specific
debugging piled on top. That work is **not automatically compatible** with this
repo's SDK — achievements, the `gpu_plugin` runtime-loadable graphics
architecture, and SDL-only windowing all landed here *after* the plume repo
forked. Some of what's in the old repo is superseded, some is dead weight
(logs, one-off scripts, a different game's config), and some is load-bearing
and needs to come back. This doc triages which is which.

Rule of thumb: **read before you port.** Nothing here should be bulk-copied —
the old tree is dirty by its own admission (see its `AGENTS.md`: "the tree is
often dirty because debugging and generated output are in flight").

## Tier 0 — Must port (FM2 will misbehave without it)

- **`FM2/src/fm2_hooks.cpp`** and the `[[midasm_hook]]` entries it backs.
  **`FM2/PATCHES.md` is stale and undercounts this badly** — it documents 8-9
  guards, but the old repo's `fm2_manifest.toml` actually registers **126**
  `[[entrypoint.midasm_hook]]` entries, and `fm2_hooks.cpp` is 3,645 lines, not
  a small patch file. A full audit (function-by-function, cross-checked
  against the manifest) found:
  - **9 genuine crash guards** — real fixes, backed only by clean guest-memory
    helpers (`GuestBase`/`GuestReadableRange`/`HasCallableVtableSlot`), no
    plume/graphics dependency. These are the ones `PATCHES.md` documents and
    the only ones ported so far.
  - **94 diagnostic-only hooks** (FMOD thread instrumentation, producer-guard/
    APU-mix counters, allocator/string/dispatch-table tracing) — all log to a
    hardcoded `C:\temp\fm2-clean.log`, none change guest behavior in the
    shipped config. Correctly excluded.
  - **18 `FM2PlumeTrace*` hooks** — not diagnostics despite the name; they
    forward guest render state into the native_renderer/plume stack
    (`FM2PlumeTraceVdSwap` literally drives `Video::Present()`). Excluded per
    Tier 1 below — do not port without the rendering integration.
  - **3 gameplay/QoL tweaks, not yet ported, needs a decision**:
    `FM2SkipStartupIntroWait`, `FM2FastForwardSplashTiming` (both inert unless
    hooked, unconditional once wired), and **`FM2FmodPumpWaitPrep82381DE4`** —
    ⚠️ this one silently overrides the guest FMOD pump wait-timeout to a
    hardcoded 8ms by default (env-var gated to disable, not cvar-gated, and
    **not mentioned anywhere in `PATCHES.md`**). Treat it as a real behavior
    change hiding in what looks like a trace hook — don't port it without
    understanding why it's there.
  - **2 inert no-ops** (`FM2SpinWaitYield`, `FM2FmodPumpForceGpuBit82381DBC`)
    and **12 orphaned hook-shaped functions with no manifest entry** (dead
    code — includes an entire abandoned producer-wait-loop instrumentation
    set at `sub_82372A38/A68/A70/A78`). Don't port either group.
  - [x] Port the 9 crash-guard hook functions + their 4 shared helpers →
        `FM2/src/fm2_hooks.cpp` (clean ~180-line file, not the 3,645-line
        original)
  - [x] Wire `src/fm2_hooks.cpp` into `FM2/CMakeLists.txt`
  - [x] Re-add the corresponding 9 `[[entrypoint.midasm_hook]]` entries to
        `fm2_manifest.toml`
  - [x] Re-add the `[entrypoint.rexcrt]` fiber mappings (`ConvertThreadToFiber`,
        `ConvertFiberToThread`, `CreateFiber`, `DeleteFiber`, `SwitchToFiber`)
        — this fixed a post-intro hang. Verified unchanged: TOML schema
        (`src/codegen/config.cpp`), `PPCRegister`/`REX_STORE_U32`/
        `REX_RAW_ADDR`/`rex::system::kernel_state()` all match this SDK
        snapshot as-is.
  - [x] Regenerated codegen (`cmake --build --target fm2_codegen`) so the
        hooks are wired into `FM2/generated/`, then rebuilt `fm2` — confirmed
        the crash-guard, FMOD, and intro/splash hooks all show up as
        `extern` forward-declarations + calls at their exact guest addresses
        in the regenerated `fm2_recomp.*.cpp` shards.
  - [x] Decided on the 3 Tier-0-adjacent gameplay tweaks, revised after
        first playtest: initially skipped `FM2SkipStartupIntroWait` and
        `FM2FastForwardSplashTiming` as unnecessary, but the intro/splash
        screens turned out not to be skippable without them (they're not
        auto-timed — the guest code genuinely waits on the condition these
        hooks short-circuit) — ported both afterward. `FM2FmodPumpWaitPrep82381DE4`
        was kept from the start, with its original `REX_FM2_PUMP_WAIT_MS`
        env-var override preserved (unset/`>0` = 8ms default override active,
        `<=0` = disabled, matching upstream behavior). All 12 hooks (9 crash
        guards + FMOD override + 2 QoL skips) are now in `fm2_hooks.cpp` and
        wired into the manifest.
  - [x] Fixed an unrelated but load-bearing build gap found while getting FM2
        to actually render: `FM2/CMakeLists.txt` called
        `rexglue_setup_target(fm2)` with no `GPU_PLUGINS` argument, so
        `rexgpu-xenos` was never staged next to `fm2.exe` — the game ran with
        no GPU backend loaded at all (black screen, `gpu_plugin not set` in
        the log). Fixed to `rexglue_setup_target(fm2 GPU_PLUGINS xenos)`;
        launch also needs `--gpu_plugin=xenos`.

## Tier 1 — High-value, but verify against the new architecture first

Don't port these mechanically. The SDK grew a `gpu_plugin` runtime-loadable
graphics architecture and other changes after this work was done — decide
per-item whether to port as-is or re-home it in the new structure.

- **Rendering.** Decided (2026-07-11/12): keep as an FM2-local D3D hook layer
  against vendored `thirdparty/plume`, not a `gpu_plugin` backend. Rebuilt into
  cleaned `FM2/src/render/` (Phases 1–4); the old `native_renderer/` diagnostic
  overlay was intentionally **not** ported. Transfer is incomplete for full
  rendering — see the audit for remaining gaps (texture bind, viewport,
  Clear/Resolve, alternate constant uploads).
  - Read first: `docs/FM2-native-renderer-transfer-audit.md` (this repo),
    then sibling-repo notes if needed: `docs/FM2-native-renderer-gap-analysis.md`,
    `docs/FM2-native-renderer-not-wired.md`,
    `docs/native-renderer-architecture-comparison.md`,
    `docs/FM2-plume-native-black-investigation.md`.
  - [x] Decide: FM2-local Plume hooks (not `gpu_plugin`)
  - [x] Port cleaned production `render/` path (device/present/resources/state/draws)
  - [x] Leave `native_renderer/` diagnostic overlay behind
  - [ ] Finish load-bearing SOURCE `render/` gaps listed in the transfer audit

- **Audio.** `src/audio/fm2_native/` (`codec`, `diag`, `runtime`, `scheduler`),
  `src/kernel/xboxkrnl/xboxkrnl_lzx.cpp` (LZX decompression), `xma_gap_diag*.h`.
  - Check whether `AudioSystem`/`AudioDriver` interfaces
    (`include/rex/audio/audio_system.h`, `audio_driver.h`) changed shape in
    this SDK snapshot before assuming these compile as-is.
  - Read first: `docs/FM2-audio-decode-throughput.md`,
    `docs/FM2-audio-producer-gate-instrumentation-2026-05-30.md`.
  - [x] Do NOT port SOURCE's `src/audio/xaudio2/` backend. It worked around an
    audio problem rexglue fixed in 0.9.0; the SDL backend is the only path.
  - [ ] Confirm audio interfaces are unchanged, then port `fm2_native/`
  - [ ] Port `xboxkrnl_lzx.cpp` if FM2 content decompression needs it

- **Input automation.** `src/input/automation/automation_input_driver.cpp` +
  header + its unit test (`tests/unit/input/automation_input_driver_test.cpp`).
  Small, self-contained (scripted/replay input for smoke testing) — low risk,
  port directly once `InputDriver` interface is confirmed unchanged.
  - [ ] Port automation input driver + test

- **PPC fiber execution model.** Top-level `src/ppc/guest_pc_fiber.cpp` +
  `include/rex/ppc/{guest_pc_fiber,static_recomp_fiber,image_info,legacy_macros}.h`.
  Unclear yet whether this was a genuine alternate execution model needed for
  FM2's fiber usage, or an abandoned experiment — `guest_pc_fiber` doesn't
  appear referenced by the `PATCHES.md` fiber fix (that one just maps
  `Create/SwitchToFiber` through `[rexcrt]`, no C++ fiber runtime needed).
  - [ ] Investigate whether this is actually load-bearing before porting —
        may be redundant with the `[rexcrt]` fiber mapping approach above

## Tier 2 — Reference only, do not bulk-copy

Valuable as institutional memory, not as source to merge in.

- **`FM2infovault/`** — full Obsidian vault (audio, rendering, xam, ppc,
  performance notes, an experiment board). Skim for facts relevant to whatever
  you're currently debugging; don't import wholesale — it documents dead ends
  as often as it documents conclusions.
- **`docs/*.md`** (~49 files) — dated handoff notes, IDA/Ghidra rename logs,
  `docs/superpowers/{plans,specs}/` design docs, shader-translation audits.
  Same treatment: read the specific doc relevant to whatever you're porting
  from Tier 0/1 (cross-references are called out above), don't copy the
  directory.
- **`AGENTS.md`** — a more detailed AI-agent ruleset than this repo's
  `CLAUDE.md` (evidence-first debugging norms, "don't hand-edit generated
  code" policy, don't create branches/worktrees mid-debug). Worth stealing
  the *rules*, not the file — fold anything still true into `CLAUDE.md`
  by hand.

## Tier 3 — Skip (noise, obsolete, or not ours)

- `.cursor/hooks/state/*` (500+ files) — accumulated log output from an IDA
  symbol-rename automation loop (`.cursor/hooks/ida-rename-loop.ts`). The logs
  are pure noise; the script itself might be worth a look if you want to
  rebuild IDA rename tooling, but it's Cursor-specific and not something to
  drag in as-is.
- `.omo/`, `.wolf/backups/`, `.debug-journal.md`, `.vs/` — session/tool
  internals from the old workspace, not project content.
- `rexglue_companion_workspace.json` — configured for a *different* game
  (`init_app_name: "FH1"`, Forza Horizon), not FM2. Irrelevant here.
- `tmp_show.py` — a throwaway script with hardcoded IDA addresses from a past
  debugging session. Not reusable.
- `FM2/fm2_config.toml` — the legacy pre-manifest single-file config format,
  already superseded by `fm2_manifest.toml` in both repos. Don't port the
  file; if it has function overrides not yet in the manifest, port those
  entries into `fm2_manifest.toml` instead.
- `src/ui/window_win.cpp`, `window_gtk.cpp` (+ their `windowed_app_context_*`
  variants) — native Win32/GTK windowing. This SDK snapshot's windowing is
  SDL-only (`window_sdl.cpp`, already present here) and includes `gpu_plugin`,
  which supersedes the old approach. **Exception**: if the Tier 1 rendering
  decision requires a raw native `HWND` for D3D12/plume interop that SDL
  doesn't expose, `window_win.cpp` may need to come back for that reason
  specifically — don't restore it preemptively.

## What ReXFM2P already has that the old repo doesn't

Confirms this repo's SDK is genuinely ahead in places, not just different —
these don't need backporting, and Tier 1 decisions above should build on them
rather than route around them:

- **Achievements system** — `achievement_manager/store/achievements.*`,
  overlay/toast UI, unit test.
- **`gpu_plugin` / `gpu_plugin_loader`** — the runtime-loadable graphics
  plugin architecture (see root `CLAUDE.md`). This is the thing the old
  repo's plume integration predates; any new rendering work should probably
  target this instead of the old direct-hook approach.
- **SDL-only windowing** — `window_sdl.cpp`, `windowed_app_*_sdl.cpp`,
  `image_decode.cpp` (stb_image), `console_commands` overlay.
