# ReXGlue080 Agent Guide

This file is for AI/code agents working in this repository. Treat it as a living
ruleset: update it when we learn a durable project lesson.

## Core Rules

- Work from evidence. For crashes, rendering bugs, and performance problems,
  gather logs/captures/disassembly first, then patch.
- Do not create branches, worktrees, or large structural rewrites unless the
  user explicitly asks. This workspace is intentionally being debugged in place.
- Do not revert user changes. The tree is often dirty because debugging and
  generated output are in flight.
- Prefer small, reversible patches. If a temporary experiment is needed, say so
  and document it.
- Keep notes in `docs/` as discoveries become durable.

## Generated Code Policy

- `FM2/generated/` is generated output. Do not make permanent fixes there.
- Temporary edits to generated files are acceptable only for proving a theory.
  Once proven, move the fix to the SDK, manifest/TOML, source code, or codegen.
- `FM2/generated/rexglue.cmake` runs codegen from
  `FM2/fm2_manifest.toml`. Treat the manifest as the source of truth for FM2
  generation.
- `FM2/fm2_config.toml` is older/legacy config context. Cross-check it when
  useful, but do not assume it drives the current FM2 build.

## Manifest/TOML Rules

- When adding known function names to FM2 generation, use the ReXGlue manifest
  form:

```toml
0x82000000 = { name = "MyFunctionName" }
```

- Keep generated symbol names stable and descriptive. Use an `FM2_` prefix for
  title-specific names.
- If an address is only a branch thunk, adjustor label, or EH landing pad, name
  it as such. Do not pretend it is a high-level gameplay function.
- When IDA names or comments are important, mirror them in
  `docs/FM2-ida-toml-function-notes.md` and, when useful, in
  `FM2/fm2_manifest.toml`.

## Portable Workspace

After copy/paste or moving the repo, stale CMake caches and absolute paths in
local presets/config are the usual failures. See `docs/portable-workspace.md`.
Run `rex-repair` from `scripts/PSReX` via PowerShell 7 (`pwsh`; Windows PowerShell
5.1 is insufficient), or let `rex-configure` / `rex-cmake` auto-repair on failure.
Title projects should use relative XEX paths and
`${sourceDir}/../out/install/...` for the SDK prefix.

## Build Workflow

Typical full runtime plus FM2 rebuild:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080'
cmake --build --preset win-amd64-relwithdebinfo --target install
Copy-Item -LiteralPath 'C:\Users\Tera\Documents\GitHub\ReXGlue080\out\install\win-amd64\bin\rexruntimerd.dll' -Destination 'C:\Users\Tera\Documents\GitHub\ReXGlue080\FM2\out\build\win-amd64-relwithdebinfo\rexruntimerd.dll' -Force
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2
```

Run FM2 codegen only when intentionally regenerating:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2_codegen
```

After rebuilding the SDK/runtime, make sure FM2 is using the fresh
`rexruntimerd.dll`. Stale DLLs can make fixes appear to fail.

## FM2 Debugging Notes

- Current FM2 log path commonly used during debugging:
  `C:\temp\fm2-clean.log`.
- Crashes in generated C++ stack traces map back to XEX addresses via function
  names like `FM2_XXX`. Prefer naming/disassembling the original
  XEX function in IDA, then burning useful names into the manifest.
- Visual investigation has used RenderDoc captures in:
  `FM2/out/build/win-amd64-relwithdebinfo/`.
- Performance investigation has used Tracy captures and CP stats log lines.
  Current command processor telemetry is documented in
  `docs/FM2-performance-notes.md`.
- x64dbg/IDA are valid tools for live debugging and disassembly. If IDA is open
  with the XEX, prefer it for naming and structural understanding.

## Known FM2 Fix Areas

- UI overexposure: documented in `docs/FM2-rendering-notes.md`. The key issue
  was a bad interpolator value (`TEXCOORD4` / VS output `o4`) causing UI pixel
  shaders to multiply textures by `(8,8,8,1)`.
- XAM/sign-in: documented in `docs/FM2-xam-notes.md`. FM2 should proceed
  without the "No Gamer Profiles" loop.
- Crash hooks and function names: documented in
  `docs/FM2-ida-toml-function-notes.md`.
- Performance/CP starvation diagnostics: documented in
  `docs/FM2-performance-notes.md`.

## Patching ReXGlue

- Prefer SDK/source fixes over title-specific hacks when behavior should match
  Xbox 360/Xenia semantics.
- Prefer title-specific manifest/TOML hooks only when the game is relying on a
  title quirk or when the root SDK behavior is not yet understood.
- For graphics behavior, compare against Xenia Canary when possible before
  inventing new behavior.
- For library/API behavior, check current official/project documentation when
  the schema or expected behavior may have changed.
- Keep diagnostic cvars hot-reloadable when practical and clearly mark temporary
  diagnostics.

## Documentation Discipline

- If a debugging session produces a durable finding, add it to `docs/`.
- If a temporary patch is introduced, document why it exists and how to remove
  or replace it.
- If a generated-code experiment proves useful, document the permanent home
  before moving on.
- Keep notes factual: include addresses, filenames, log fields, capture names,
  and exact observed behavior.

## Style And Safety

- Use `rg` / `rg --files` for searches.
- Use `apply_patch` for manual edits.
- Avoid destructive git commands. Never run `git reset --hard` or checkout over
  user work unless explicitly requested.
- Avoid sweeping reformatting or unrelated cleanup.
- Third-party directories may have their own rules. Do not modify `thirdparty/`
  unless the task truly requires it.

## Learned User Preferences

- Run `rexglue init` and `rexglue codegen` with the installed SDK binary
  (`../out/install/win-amd64/bin/rexglue.exe` from `DOAX/`), not in-tree build
  artifacts or absolute repo-root paths.
- Pass relative paths when initializing a title from its project directory
  (`assets/default.xex`, `--game-root assets`).
- Run PSReX repair/setup scripts with PowerShell 7 (`pwsh`); Windows PowerShell
  5.1 is insufficient.
- Use `${projectDir}/assets` for `--game_data_root` in `DOAX/.vs/launch.vs.json`,
  not `${workspaceRoot}\assets`.
- Use `DOAX/scripts/bootstrap-codegen-loop.cmd` to run the bootstrap/codegen loop
  from Explorer; the script pauses on exit unless `-NoPause` is passed.
- Rebuild DOAX intro/movie skips from IDA-disassembly midasm hooks; bulk legacy
  skip code was intentionally stripped from `doax_hooks.cpp` before re-adding skips.
- Prefer portable `game_data_root` via `doax_app.h` `OnConfigurePaths` (exe-relative
  `../../../assets`); `.vs/launch.vs.json` is gitignored and breaks on checkouts.
- Manual `cmake --build` and launch from `DOAX/out/build/win-amd64-relwithdebinfo/doax.exe`
  is fine during boot-hook debugging; VS is not required.
- Diagnose Press Start / four-menu input with SDK and guest input probes; `user_dismiss=0`
  alone does not prove the user did not press A/Start.

## Learned Workspace Facts

- Active title project is `DOAX/` (Dead or Alive Xtreme); FM2 paths and notes in
  this guide are legacy from the parent `ReXGlue080` copy.
- After copying or moving the repo, run portable workspace repair before building
  titles.
- DOAX generated output lives in `DOAX/generated/`; manifest source is
  `DOAX/doax_manifest.toml`; use a `DOAX_` prefix for title-specific symbol
  names.
- Title executables in the parent `DOAX/CMakeLists.txt` do not inherit SDK root
  `add_compile_options()` from `add_subdirectory`; Windows `/EHsc` and related
  flags are applied via `rexglue_apply_target_settings()` in
  `cmake/rexglue_helpers.cmake`.
- Stock `rexglue init` CMake presets use plain `clang++` without SSSE3; this
  workspace's `DOAX/CMakePresets.json` needs local `clang-cl` and
  `-march=x86-64-v3` on Windows — re-running `init --force` resets that preset.
- DOAX build output is `DOAX/out/build/win-amd64-relwithdebinfo/doax.exe`; sync
  fresh `rexruntimerd.dll` after SDK rebuilds.
- Game assets and `default.xex` live under `DOAX/assets/`; manifest `game_root`
  is `assets`.
- Bootstrap bring-up (ported from ReXGlue080FM4): `--bootstrap_unregistered_functions=1`
  and `--bootstrap_functions_log=DOAX/bootstrap_discovered.toml`; `rexglue codegen`
  merges into `[entrypoint.functions]` via line-based `bootstrap_merge.cpp` (toml++
  `out << tbl` emits dotted `[entrypoint.functions.0x...]` headers). Loop:
  `DOAX/scripts/bootstrap-codegen-loop.ps1` runs the game first, reports new unique
  addresses after the window closes, then merge/codegen/build **only** when the
  session recorded new addresses (or `-ForceCodegen`). It does not codegen just
  because manifest timestamps changed.
- After manifest or codegen changes, rebuild `doax.exe`; entrypoint registration
  is `PPCFuncMappings` in `doax_init.cpp` (not `doax_RegisterFunctions`).
  `[entrypoint.functions]` entries need `name` or bare stubs can crash codegen
  Write; `codegen_command.cpp` repairs/merges manifest and compacts duplicate
  `bootstrap_discovered.toml` before parse.
- DOAX boot cinematics: warning → ninja (`ninja_vi_hd.sfd`, table index 4) →
  promotion_video.sfd (index 1). `opening.sfd` (index 0) is post–Travel confirm via
  `DOAX_TravelOpeningHandler` (`0x826E1978`), not boot. Warning is `spWarn.xpr`
  sprite UI (scheduler mode 2), not SFD playback. Boot promotion runs via UI handlers
  `DOAX_MenuTransitionPlayMovie` (`0x824C1208`) and poll
  `DOAX_MenuTransitionMoviePoll` (`0x824C12D0`). `DOAX_MovieFilenameTable` at
  `0x82E71DC0` (40-byte stride).
- DOAX boot hooks (reference: `DOAX/archive/fiber-hooks-2026-06-24/`): license warn
  midasm `0x8250AAA8`→`0x8250AAFC`; ninja `0x8250AB1C`→`0x8250ABA4`. Promotion **replay**
  midasm `0x824C12B8`→`0x824C12BC` when `r3==1` and `g_boot_promotion_play_attempts>0`.
  Block drain-rearm menu-kick during active promotion or Press Start preview (`overlay==0`).
  Four-menu needs scheduler `flag2` via menu-kick; latch `g_boot_promotion_finished` or
  menu-kick stays blocked. Press Start → 4-menu dismiss uses LABEL_34 when `overlay==1`
  (LABEL_12 midasm-blocked at sched `2/2`); block LABEL_34 only for auto `dismiss_f9` from
  movie-poll-done, not user A/Start. Do not arm travel suppress on LABEL_12/LABEL_34
  during Press Start hold; `g_press_start_overlay_dismiss` + fade snap avoid travel_arm
  black flash. Do not global-skip `DOAX_PlayMovie(0)`. Travel confirm needs idle hub
  `5/1`, `overlay==1`, `item==15`.
- `DoaxApp` in `doax_app.h` calls `std::exit` on window close after `TerminateTitle`;
  guest fiber swap can otherwise leave `doax.exe` running past window close.
