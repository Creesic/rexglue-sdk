# FM2 Hook Workflow (Incremental)

This workflow is for adding title-specific FM2 patches/features without editing generated code.

## 1) Pick a target in IDA/Ghidra

- Identify the exact PPC address to hook (instruction-level site).
- Record:
  - Function start address
  - Hook site address
  - Desired branch destination (if condition is true)
  - Required registers at the hook site

## 2) Name the function in the manifest

In `FM2/fm2_manifest.toml`, add or update:

```toml
0x82697F08 = { name = "FM2_ApuMixRenderCore_82697F08" }
```

Use stable descriptive names with `FM2_` prefix.

## 3) Implement hook logic in `FM2/src/fm2_hooks.cpp`

Hook function styles:

1. `bool` hook (conditional jump)

```cpp
bool FM2MyConditionHook(PPCRegister& r3, PPCRegister& r31) {
  // return true => jump to jump_address_on_true
  // return false => continue normal flow
  return false;
}
```

2. `void` hook (observe/mutate registers, no jump decision)

```cpp
void FM2MyTraceHook(PPCRegister& r3) {
  // optional logging / register edits
}
```

## 4) Bind the hook in `FM2/fm2_manifest.toml`

```toml
[[entrypoint.midasm_hook]]
address = 0x82697F20
name = "FM2MyConditionHook"
registers = ["r3", "r31"]
after_instruction = true
jump_address_on_true = 0x82697F60
```

Notes:

- `registers` must match your C++ function signature and order.
- `after_instruction = true` means register values after the hooked PPC instruction.
- Omit `jump_address_on_true` for pure observe/mutate hooks.

## 5) Rebuild in the correct order

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080'
cmake --build --preset win-amd64-relwithdebinfo --target install
Copy-Item -LiteralPath 'C:\Users\Tera\Documents\GitHub\ReXGlue080\out\install\win-amd64\bin\rexruntimerd.dll' -Destination 'C:\Users\Tera\Documents\GitHub\ReXGlue080\FM2\out\build\win-amd64-relwithdebinfo\rexruntimerd.dll' -Force
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2
```

Run `fm2_codegen` only when intentionally regenerating.

## 6) Validate

- Confirm hook fires via log counters / targeted logs.
- Verify no regressions at:
  - boot
  - menu load
  - race start
  - race finish

## 7) Document durable patches

Add permanent patch notes to `FM2/PATCHES.md`:

- function purpose
- exact hook address
- jump target (if used)
- why this patch is needed
- rollback criteria (if temporary)

## Recommended first patch pattern

Start with a non-branching trace hook at function entry:

- no control-flow change
- confirms hook plumbing and register capture
- low-risk baseline before adding behavioral patches
