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
Run `rex-repair` from `scripts/PSReX` (or let `rex-configure` / `rex-cmake` auto-repair
on failure). Title projects should use relative XEX paths and
`${sourceDir}/../out/install/...` for the SDK prefix.

## Build Workflow

Typical full runtime plus FM2 rebuild:

Paths below are **relative to the repo root** so they survive copy/move (this checkout is
`ReXGlue080plume`, not the older `ReXGlue080`). Run from the repo root unless noted; `$RepoRoot`
is wherever you cloned it.

```powershell
# from the repo root
cmake --build --preset win-amd64-relwithdebinfo --target install
Copy-Item -LiteralPath '.\out\install\win-amd64\bin\rexruntimerd.dll' -Destination '.\FM2\out\build\win-amd64-relwithdebinfo\rexruntimerd.dll' -Force
Set-Location '.\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2
```

Run FM2 codegen only when intentionally regenerating (from `FM2/`):

```powershell
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

- When renaming unnamed IDA functions, include explicit reasoning for each rename
  decision in the session documentation.
- Do not use automated heuristic/placeholder naming (e.g. `FM2_Helper_XXXX`,
  `*_Caller`); every rename must be manual evidence-based decompile naming.
- Log all IDA renames in a dated `docs/FM2-ida-renames-*.md` file and
  cross-reference it from `docs/FM2-ida-toml-function-notes.md`.
- When asked to work an IDA naming cluster or to loop/keep going until done,
  continue until all unnamed `sub_` callees in scope are exhausted (cluster
  closure or global `FM2_` infrastructure queue).
- Prefer specific behavior-based snake_case names over vague names like
  `handler`, `process_data`, or `do_stuff`.
- Cross-check repo docs and local Xbox 360 tech docs when naming unnamed IDA
  functions from already-named `FM2_` caller context.
- During IDA naming work, "go", "continue", or "keep going" means proceed with
  the next manual pass without asking for extra confirmation first.
- In the IDA rename loop, skip low-confidence candidates and record them in the
  scratchpad skipped table with reason; do not overwrite meaningful names.

## Learned Workspace Facts

- FM2 IDA database is `default.xex.i64`; use the `user-IDA` MCP server for batch
  renames and decompilation.
- Xbox 360 tech docs live at `D:\Emulation\Xbox360techdocs` (not
  `D:\Emulation\Xbox360 tech docs`).
- IDA naming workflow: enumerate `sub_` callees of named `FM2_` functions,
  decompile high-traffic clusters, name from decompiler behavior plus
  caller context.
- Primary repo docs for IDA naming:
  `docs/FM2-ida-toml-function-notes.md`,
  `docs/FM2-native-renderer-generator-notes.md`, `docs/FM2-performance-notes.md`,
  and `docs/FM2-audio-fmod-decode-cadence.md`.
- Render emit cluster BFS roots:
  `FM2_Render_EmitPassDrawWork`, `FM2_D3D_EmitDirtyStateAndDrawList`,
  `FM2_D3D_EmitDrawListStatePackets`, `FM2_D3D_EmitScissorRegionPackets`,
  `FM2_D3D_EmitSurfaceResolvePackets`, `FM2_D3D_BeginCommandBufferBatch`, and
  `FM2_D3D_FinalizeCommandBufferBatch`.
- IDA MCP Python rename scripts should use `ida_name.set_name(...,
  ida_name.SN_CHECK)` with `ida_name.SN_FORCE` fallback; do not use
  `idc.SN_FORCE`. `ida_name.set_name` returns boolean `True` on success (not
  integer `0`).
- IDA Python symbol lookup and free-name checks should use
  `ida_name.get_name_ea(ida_idaapi.BADADDR, name)` (i.e. `0xFFFFFFFFFFFFFFFF`
  when unused), not `get_name_ea(0, name)`.
- IDA MCP scripts should iterate functions with `idautils.Functions()`, not
  `ida_funcs.Functions()`.
- Infrastructure `sub_` naming queue: run `scripts/ida_fm2_list_unnamed_sub_callees.py`
  with `--outside-emit` (writes `.cursor/hooks/state/unnamed-sub-callees.json`),
  then decompile high-caller-count candidates for manual passes. Pass artifacts
  (`build_infra_passN.py`, `rename_infra_passN.json`, `append_infra_passN.md`)
  live in `.cursor/hooks/state/`.
- Lua binding registrars are often identifiable from embedded method-name strings
  and should be named `FM2_Lua_Register*`.
- CRT/XML static-init hooks commonly follow `FM2_Crt_AtexitRegister*` and
  `FM2_XmlStaticInit_CacheTypeHandle_<global>` patterns from embedded strings
  and type-handle globals.
- IDA rename loop (hook `.cursor/hooks/ida-rename-loop.ts`): at most 12
  `snake_case` renames per iteration logged in `.cursor/ida-rename-scratchpad.md`;
  set `IDA_RENAME_LOOP: done` when exhausted. Appends via
  `.cursor/hooks/state/merge_scratchpad.py`. Separate from `FM2_` infrastructure
  passes in `.cursor/hooks/state/`.
