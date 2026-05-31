# Mid-Assembly Hooks in ReXGlue: A Guide

## What is a mid-asm hook?

When ReXGlue recompiles an Xbox 360 game, it translates every PowerPC instruction into C++. A **mid-asm hook** lets you inject your own C++ function call at any specific instruction address in the recompiled code. This is useful for:

- **Logging** — trace what the game is doing at a specific point
- **Crash prevention** — check for bad pointers before the game dereferences them
- **Behavior override** — skip, redirect, or modify game logic on the fly

You don't modify generated code. You just add an entry to a TOML file and write a small C++ function.

## How it works

The recompiler reads your TOML manifest, finds the `[[midasm_hook]]` entries, and injects a function call at the specified PPC address in the generated C++ code. The codegen also auto-generates the `extern` declaration for your function — no manual declarations needed.

## Step-by-step

### 1. Create your hooks file

In your game project's `src/` directory, create a file like `game_hooks.cpp`. This is a regular C++ file:

```cpp
#include "generated/game_init.h"

#include <rex/ppc.h>
#include <rex/logging.h>

// Example: log a value at a specific point in the game
void MyGame_LogAddress(PPCRegister& lr) {
  REXKRNL_ERROR("Reached target code, LR=0x{:08X}", lr.u32);
}

// Example: prevent a crash from a bad pointer
bool MyGame_SkipBadPointer(PPCRegister& r3) {
  // Return true to make codegen skip the bad code path
  // Return false to let normal execution continue
  return r3.u32 == 0;
}
```

### 2. Add it to your CMakeLists.txt

```cmake
set(GAME_SOURCES
    src/main.cpp
    src/game_hooks.cpp     # <-- add your hooks file
)
```

### 3. Add entries to your manifest TOML

In your `game_manifest.toml`, add `[[entrypoint.midasm_hook]]` blocks:

```toml
# Simple logging hook — called every time the game hits address 0x82123456
[[entrypoint.midasm_hook]]
address = 0x82123456
name = "MyGame_LogAddress"
registers = ["lr"]

# Conditional skip hook — if function returns true, jump elsewhere
[[entrypoint.midasm_hook]]
address = 0x82123ABC
name = "MyGame_SkipBadPointer"
registers = ["r3"]
after_instruction = true
return_on_true = false
jump_address_on_true = 0x82123AE0
```

### 4. Rebuild

Run codegen (to regenerate the C++ with your hook injected), then rebuild:

```powershell
cmake --build --preset win-amd64-relwithdebinfo --target game_codegen
cmake --build --preset win-amd64-relwithdebinfo --target game
```

That's it. The generated code will contain something like:

```cpp
extern bool MyGame_SkipBadPointer(PPCRegister& r3);  // auto-generated

// ... inside the recompiled function ...
// lwz r3,0(r4)          <- the original PPC instruction
if (MyGame_SkipBadPointer(ctx.r3)) {
    goto loc_82123AE0;   // your jump target
}
// ... continues normal execution ...
```

## TOML options

| Field | Type | What it does |
|---|---|---|
| `address` | hex | PPC instruction address to hook |
| `name` | string | Your C++ function name |
| `registers` | array | PPC registers to pass as `PPCRegister&` args |
| `after_instruction` | bool | Hook fires after the instruction (default: before) |
| `return` | bool | Return from the recompiled function after your hook |
| `return_on_true` | bool | Return if your hook returns `true` |
| `return_on_false` | bool | Return if your hook returns `false` |
| `jump_address` | hex | Jump to this PPC address after your hook |
| `jump_address_on_true` | hex | Jump here if your hook returns `true` |
| `jump_address_on_false` | hex | Jump here if your hook returns `false` |

## Available registers

Pass these as strings in the `registers` array. Your function receives `PPCRegister&` references:

- **GPRs**: `"r0"` through `"r31"`
- **FPRs**: `"f0"` through `"f31"`
- **VMXs**: `"v0"` through `"v127"`
- **Special**: `"lr"`, `"ctr"`, `"cr0"`–`"cr7"`, `"xer"`, `"fpscr"`

## Common patterns

### Log a value (no side effects)

```toml
[[entrypoint.midasm_hook]]
address = 0x82123456
name = "MyGame_LogSomething"
registers = ["r3", "lr"]
```

```cpp
void MyGame_LogSomething(PPCRegister& r3, PPCRegister& lr) {
  REXKRNL_ERROR("r3=0x{:08X} from LR=0x{:08X}", r3.u32, lr.u32);
}
```

### Skip a crash (null/bad pointer guard)

```toml
[[entrypoint.midasm_hook]]
address = 0x82123456
name = "MyGame_GuardNullDeref"
registers = ["r3"]
after_instruction = true
jump_address_on_true = 0x82123480
```

```cpp
bool MyGame_GuardNullDeref(PPCRegister& r3) {
  return r3.u32 == 0;  // true = skip the crash
}
```

### Override a return value

```toml
[[entrypoint.midasm_hook]]
address = 0x82123456
name = "MyGame_ForceReturnZero"
registers = ["r3"]
after_instruction = true
return_on_true = true
```

```cpp
bool MyGame_ForceReturnZero(PPCRegister& r3) {
  r3.u32 = 0;       // set return value
  return true;       // true = return from function
}
```

## How the codegen handles declarations

You do **not** need to create a header file or manually declare your hook functions. When codegen processes your manifest and finds a `[[midasm_hook]]` entry, it automatically emits an `extern` declaration at the call site in the generated `.cpp` file.

For example, given this TOML:

```toml
[[entrypoint.midasm_hook]]
address = 0x82603C28
name = "FM2SkipBadChildSlot"
registers = ["r11", "r31"]
after_instruction = true
jump_address_on_true = 0x82603C48
```

The codegen produces:

```cpp
extern bool FM2SkipBadChildSlot(PPCRegister& r11, PPCRegister& r31);

DEFINE_REX_FUNC(FM2_ReleaseOwnedChildObjects) {
    // ... recompiled PPC code ...
    if (FM2SkipBadChildSlot(ctx.r11, ctx.r31)) {
        goto loc_82603C48;
    }
    // ... continues ...
}
```

The declaration, the call, and the conditional jump are all generated for you. You only write the function body in your hooks file.

## Tips

- **Finding addresses**: Use IDA or Ghidra to find the PPC address where you want to hook. Look for the instruction immediately before the point of interest (or use `after_instruction = true` to hook after it).
- **Register values are live**: When your hook is called, the register values reflect the PPC state at that exact point in execution. Modifying a `PPCRegister&` changes the value the generated code sees.
- **`after_instruction = true`** is almost always what you want for crash guards — it lets the original instruction execute first, then your hook checks the result.
- **Conditional jumps**: Use `jump_address_on_true` / `jump_address_on_false` to redirect execution to an existing PPC code path (like an error handler or early-return).
- **Keep hooks minimal**: Hook functions run on the game thread. Heavy work (allocations, I/O, complex logging) can cause performance issues or deadlocks.
