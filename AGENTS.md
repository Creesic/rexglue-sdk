## Learned User Preferences

- When initializing this workspace or fixing CMake/build errors, borrow toolchain and preset settings from the sibling `ReXGlue080` repo instead of inventing new paths.
- User copies full game assets into `FM4/assets` manually; do not assume the folder is complete until the user confirms.
- Prefer SDK-level automation for the crash→stub→codegen→build bring-up loop rather than one-off manual edits only.
- PowerShell loop scripts should pause on exit and write logs so failures stay visible when launched by double-click.
- FM4 development uses the `win-amd64-relwithdebinfo` CMake preset as the default build configuration.
- During FM4 bootstrap bring-up, use `rexglue codegen ... --force` so unresolved branches become stubs instead of hard codegen failures.
- Prioritize gameplay bring-up over intro video rendering when choosing what to debug next.
- Do not hand-edit `FM4/generated/`; use `[[midasm_hook]]` in `fm4_config.toml` (impl under `FM4/src/`) or wiki-documented patches — see https://github.com/rexglue/rexglue-sdk/wiki/Mid-ASM-Hooks.
- Prefer midasm hooks over FH1-style post-codegen patcher scripts for targeted guest control-flow fixes.
- Do not wholesale-copy `FM4/San/fm4/generated/fm4_recomp.*.cpp` into `FM4/generated/` — San codegen splits differ and cause duplicate linker symbols; use SDK codegen fixes, `FM4/scripts/` patch scripts, or re-codegen.
- Bootstrap loop should support keypress advance (not only a fixed timer) so the user can continue when ready.

## Learned Workspace Facts

- This repo is `rexglue-sdk` at `ReXGlue080FM4`; companion workspace `C:\Users\Tera\Documents\ReXGlue080`; `FM4/San/` is an untracked collaborator snapshot (SDK + fm4 + FH1 reference) — compare file-by-file, not a git branch.
- The Forza Motorsport 4 title project lives in `FM4/` with entrypoint `default.xex` plus `XMediaFacade_default.xex` and `SpeechFacade_default.xex` modules.
- Runtime game data is under `FM4/assets/`; VS debug profiles in `FM4/.vs/launch.vs.json` include `fm4 (Debug)` and `fm4 (Bootstrap)` with `--game_data_root`.
- Two CMake trees: SDK at repo root (`out/win-amd64/rexglue.exe`), FM4 at `FM4/out/build/win-amd64-relwithdebinfo/`; run `rexglue codegen fm4_manifest.toml --force` from `FM4/` before building `fm4` if `generated/` is missing.
- Codegen is driven by `FM4/fm4_manifest.toml`; stubs and `[[midasm_hook]]` blocks live in `fm4_config.toml` / sibling configs; hook bodies in `FM4/src/fm4_midasm_hooks.cpp`.
- FM4 CRT hooks are configured as `setjmp_address = 0x82762EC0` and `longjmp_address = 0x82762A90` in manifest/config.
- FM4 runtime logs are under `FM4/out/build/win-amd64-relwithdebinfo/logs/fm4_NNN.log`; bootstrap loop script logs stay in `FM4/logs/`.
- Runtime mounts `cache:\` on the host cache directory (default `C:\Users\Tera\Documents\fm4\cache`); `HostPathDevice`'s third argument is read-only — the cache mount must pass `false`.
- SDK `rexruntimerd.dll` builds to `out/win-amd64/`; FM4 POST_BUILD copies it beside `fm4.exe` — rebuild `fm4` or copy manually after SDK-only `rexruntime` rebuilds.
- Guest cooperative fiber swap uses San-merge `guest_pc_fiber` (`src/ppc/guest_pc_fiber.cpp`) with `FM4/src/fm4_guest_pc_fiber.cpp` (`fiber_start_pc`, r13 repin, trampoline/stale-entry guards); do not enable global deny/preserve at entry LR `0x82C9EF3C` — it breaks Press Start menu (required yield site).
- FM4 in-game audio required fixing `vpkd3d128` case 5 in-place FLOAT16_4 pack aliasing in `src/codegen/builders/vector.cpp` (FMOD `setLevels`); until codegen re-run, `FM4/scripts/patch_vpkd3d128_case5.py` patches `fm4_recomp.2.cpp` and `fm4_recomp.72.cpp` — see `FM4/patcher/audio-vpkd3d128-fix.md`.
- Bootstrap bring-up uses `--bootstrap_unregistered_functions` and `--bootstrap_functions_log` (default `FM4/bootstrap_discovered.toml`); automated loop is `FM4/scripts/bootstrap-codegen-loop.ps1` (`.cmd` launcher; keypress or timed advance; stub counting includes nested `[functions.0x...]` sections).
