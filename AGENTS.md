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
  names like `__imp__sub_82375ED0`. Prefer naming/disassembling the original
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
- Keep diagnostic cvars hot-reloadable when practical and clearly mark
  temporary diagnostics.

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
