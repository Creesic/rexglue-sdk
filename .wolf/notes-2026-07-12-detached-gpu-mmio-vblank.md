# 2026-07-12 — Swap-1 hang: detached GPU MMIO vblank gate

## Cause
FM2 GameLoop waits on `dword_829C24C0`, woken by `FM2_VBlank_SignalGameLoopGate` via
`D3D::VerticalBlankInterrupt` ← `GraphicsInterruptCallback` when `r3==0`.

That path **also** requires `*(u32*)0x7FC86544 & 1` (GPU interrupt status, reg `0x1951`).
With no `GraphicsSystem`, that MMIO range is never registered. `REX_MM_LOAD_U32` then
called `CheckLoad` into an **uninitialized** `_v` → intermittent bit0 → intermittent
Swap-1 hangs.

## Fix
1. Register stub GPU MMIO `0x7FC80000` in detached mode (`EnsureDetachedGpuMmio` in
   `xboxkrnl_video.cpp`); return `1` for reg `0x1951` (same as `GraphicsSystem::ReadRegister`).
2. Zero-init `REX_MM_LOAD_*` temps on CheckLoad miss (template + current `fm2_init.h`).
3. StretchRect/Resolve: erase pending surfaces on destroy; gate copies with
   `IsLiveHostTexture`; plume `toD3D12` null-check before `desc` read.

## Smoke (2026-07-12)
After fix, runs routinely reach `Swap hook: called 301` (~11s). Occasional AV remains
in Resolve/StretchRect null-host paths (hardening reduces rate; not 100% yet).
Hang-at-Swap-1 from missing VBlank is the fixed issue.
