# OpenWolf

@.wolf/OPENWOLF.md

This project uses OpenWolf for context management. Read and follow .wolf/OPENWOLF.md every session. Check .wolf/cerebrum.md before generating code. Check .wolf/anatomy.md before reading files.


# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

ReXGlue is an **ahead-of-time (AOT) static recompiler** for Xbox 360 executables. It does
not interpret or JIT PowerPC at runtime — it reads a XEX and emits portable C++23 source code
that is then compiled natively for modern platforms (Windows/Linux, amd64/arm64). Rooted in
Xenia's emulation research; codegen approach inspired by XenonRecomp and rexdex's recompiler.

This repo is both the **SDK** (the `rexglue` CLI + runtime libraries) and a **title project**,
`FM2/` (Forza Motorsport 2), which is the primary debugging target. `FM2infovault/` is a
separate notes/knowledge vault, not build input.

**`AGENTS.md` is the authoritative agent ruleset — read it.** This file summarizes structure
and commands; AGENTS.md governs how to work (evidence-first debugging, no unsolicited branches,
don't revert the dirty tree, generated-code policy, manifest conventions).

## Two-stage architecture (read this before editing)

1. **SDK build** → produces the `rexglue` CLI and the runtime libraries (`rexruntime*.dll`/`.so`).
   Source lives in `src/` (one CMake subdir per subsystem: `core`, `kernel`, `graphics`,
   `audio`, `filesystem`, `input`, `ui`, `system`, `ppc`, `codegen`, `rexglue`).
2. **Title build** (e.g. `FM2/`) → runs codegen from a TOML manifest to emit C++ into
   `FM2/generated/`, then compiles that against the installed SDK runtime into a playable exe.

`src/codegen/` is the recompiler engine. `src/graphics/` is a large Xenos GPU translation layer
(command processor, PM4 packet disassembler, shader translation, D3D12 + Vulkan backends) built
on **plume** (`plume/`), a vendored low-level RHI abstracting D3D12/Vulkan/Metal.

The SDK runtime must be **installed** (`--target install`) before a title links against it. A
**stale `rexruntimerd.dll` in the title build dir is the #1 cause of "my fix didn't work"** —
always copy the freshly-installed DLL into the title build dir after rebuilding the SDK.

## Generated code is not source

`FM2/generated/` (incl. `fm2_recomp.*.cpp`, `fm2_init.*`, `rexglue.cmake`) is codegen output.
**Do not make permanent fixes there.** Temporary edits are acceptable only to prove a theory;
once proven, move the fix to the SDK source, the manifest, or the codegen. Real fixes land in
one of three places:
- **SDK/source** (`src/`) — when behavior should match Xbox 360/Xenia semantics generally.
- **Manifest** (`FM2/fm2_manifest.toml`) — the source of truth for FM2 generation: function
  naming and `[[midasm_hook]]` injection. `FM2/fm2_config.toml` is legacy; cross-check only.
- **Title hooks** (`FM2/src/fm2_hooks.cpp`) — C++ for mid-asm hooks referenced by the manifest.

Manifest function-name form: `0x82000000 = { name = "FM2_MyFunction" }`. Use the `FM2_` prefix
for title-specific names; if an address is only a branch thunk / adjustor / EH landing pad, name
it as such rather than inventing a gameplay name. Mid-asm hooks are documented in
`docs/midasm-hooks-guide.md` and `docs/FM2-hook-workflow.md`.

## Build & dev commands

Toolchain is **enforced: Clang ≥ 18, C++23, 64-bit, Ninja Multi-Config**. On Windows the
presets use `clang-cl` from a Visual Studio install. Per-machine compiler/LIBPATH overrides go
in `CMakeUserPresets.json` (gitignored) — copy from `CMakeUserPresets.json.example`; never bake
your username into the committed `CMakePresets.json`.

Configure presets: `win-amd64`, `win-arm64`, `linux-amd64`, `linux-arm64`. Build/test presets
append a config: `<configure>-{debug,release,relwithdebinfo}`. RelWithDebInfo is the default
working config (debug-info builds get a `rd` suffix, debug a `d` suffix).

`thirdparty/` deps are **git submodules** (`plume/` is vendored in-tree, not a submodule). A fresh
clone needs `git submodule update --init --recursive` before configuring.

```powershell
# Configure + build + install the SDK (Windows, default working config)
cmake --preset win-amd64
cmake --build --preset win-amd64-relwithdebinfo --target install

# Build FM2 (from the FM2/ dir) — needs the installed SDK + fresh runtime DLL
cmake --build --preset win-amd64-relwithdebinfo --target fm2

# Regenerate FM2 C++ from the manifest (only when intentionally regenerating)
cmake --build --preset win-amd64-relwithdebinfo --target fm2_codegen
```

After rebuilding the SDK, sync the runtime DLL into the FM2 build dir before rebuilding FM2:
`out/install/win-amd64/bin/rexruntimerd.dll` → `FM2/out/build/win-amd64-relwithdebinfo/`.
The exact copy command is in `AGENTS.md`'s build workflow.

### PSReX (preferred Windows dev wrapper)

`scripts/PSReX` (PowerShell 7+) wraps these flows and **auto-repairs stale CMake caches / absolute
paths** after the tree is copied or moved (the workspace is designed to be relocatable — see
`docs/portable-workspace.md`).

```powershell
Import-Module .\scripts\PSReX -Force
rex-repair                       # scan + fix stale caches / absolute paths (rex-repair -WhatIf to preview)
rex-configure
rex-build -Config RelWithDebInfo
rex-install ; rex-test
rex-format ; rex-lint            # clang-format / clang-tidy (.clang-format, .clang-tidy at root)
```
`rex-configure` / `rex-build` / `rex-cmake` run `rex-repair` once and retry on cmake failure
(disable via `$env:REXGLUE_SKIP_AUTO_REPAIR = '1'`).

### Tests

Tests are **off by default** (`-DREXGLUE_BUILD_TESTS=ON` to enable). Two suites under `tests/`:
`tests/ppc/` (PPC instruction tests generated from assembly) and `tests/unit/` (Catch2 unit
tests per subsystem: `codegen`, `core`, `fm2`, `kernel`, `memory`, `ppc`, `system`, `rexglue`).

```powershell
cmake --preset win-amd64 -DREXGLUE_BUILD_TESTS=ON
cmake --build --preset win-amd64-relwithdebinfo
ctest --preset win-amd64-relwithdebinfo                 # all tests
ctest --preset win-amd64-relwithdebinfo -R <name>       # single test by regex
```

## `rexglue` CLI

The SDK binary `rexglue` (target `rex::rexglue`) requires a subcommand:
- `init` (+ `init module`) — scaffold a new title project / add a DLL module.
- `codegen` — analyze a XEX and generate C++.
- `recompile-tests` — generate Catch2 tests from PPC assembly.

## Build options (CMakeLists.txt)

- `REXGLUE_USE_D3D12` (Win default ON) / `REXGLUE_USE_VULKAN` (Linux default ON, Win OFF) —
  at least one graphics backend is required.
- `REXGLUE_ENABLE_TRACY` (ON) / `REXGLUE_ENABLE_PERF_COUNTERS` (ON) — profiling defines are
  compiled out in Release.
- `REXGLUE_ENABLE_SANITIZERS` (OFF) — UBSan only; ASan is incompatible with the custom memory
  map at `0x100000000`.
- `REXGLUE_ENABLE_FIDELITYFX` (OFF, experimental).

## FM2 debugging orientation

- Crashes in generated C++ map back to XEX addresses via `FM2_XXX` names — disassemble/name the
  original XEX function (IDA/x64dbg), then burn the name into the manifest.
- Common log path during debugging: `C:\temp\fm2-clean.log`. RenderDoc/Tracy captures live in
  `FM2/out/build/win-amd64-relwithdebinfo/`.
- Durable findings go in `docs/` (already covered: rendering UI overexposure, XAM/sign-in,
  audio pipeline, performance/CP starvation, IDA→TOML function notes, native renderer generator).

## IDA reverse-engineering workflow

Naming the XEX's `sub_` functions is a major ongoing activity (it produces the `FM2_` names that
make crash triage possible). **AGENTS.md is authoritative on this** — read its "Learned User
Preferences" and "Learned Workspace Facts" before doing rename work. Key points:

- Every rename is manual, evidence-based decompiler naming — no heuristic/placeholder names
  (`FM2_Helper_XXXX`, `*_Caller`). Prefer behavior-based `snake_case` over vague verbs.
- The IDA database is `default.xex.i64`; batch renames go through the IDA MCP server.
- Log renames in a dated `docs/FM2-ida-renames-*.md` and cross-reference from
  `docs/FM2-ida-toml-function-notes.md`; useful names get burned into `FM2/fm2_manifest.toml`.
- Tooling: `scripts/ida_fm2_*.py` (enumerate unnamed callees, batch rename) and the
  `.cursor/hooks/ida-rename-loop.ts` loop with per-pass artifacts in `.cursor/hooks/state/`.

## Conventions

- Search with `rg`. Don't reformat or clean up unrelated code. Don't touch `thirdparty/`
  (vendored, own rules) unless the task truly requires it.
- Avoid destructive git: never `git reset --hard` or checkout over user work unless explicitly
  asked. The working tree is intentionally kept dirty (in-flight debugging + generated output).
