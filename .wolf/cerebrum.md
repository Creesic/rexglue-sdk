# Cerebrum

> OpenWolf's learning memory. Updated automatically as the AI learns from interactions.
> Do not edit manually unless correcting an error.
> Last updated: 2026-07-06

## User Preferences

<!-- How the user likes things done. Code style, tools, patterns, communication. -->

## Key Learnings

- **Project:** ReXFM2P — "ReXGlue" static recompiler (Xbox 360 PPC → portable C++, Xenia/XenonRecomp-derived) plus `FM2/`, a downstream recompiled build of Forza Motorsport 2. Full architecture now documented in root `CLAUDE.md`.
- `FM2/` is untracked in git (on-disk only, no commits) — `FM2/generated/**` is codegen build output, not hand-written source.
- Build requires Clang >= 18, C++23, RelWithDebInfo-only (Debug/Release presets are disabled in CMakePresets.json). Day-to-day commands go through the `PSReX` PowerShell module (`rex-configure`/`rex-build`/`rex-test`/etc.), not raw cmake.
- **FM2 native Plume renderer (branch `plume`):** intentional rebuild into `FM2/src/render/`, NOT a port of `ReXGlue080plume/FM2/src/native_renderer/`. That overlay (~5k lines: debug replay, compare window, `FM2PlumeTrace*` midasm hooks, PM4 scanner) was deliberately dropped. Production path is direct `REX_HOOK` D3D replacements against vendored `thirdparty/plume`. Commits: `d3748a68` (Phases 1–3), `34a4bf8a` (Phase 4 draws), `64d7a2a8` (present/audio/vsync fixes; still black).
- **Cleanup left real gaps vs SOURCE `render/`:** `D3DDevice_SetTexture` is now hooked (2026-07-12) into `rr::SetTexture`/`SetTextureBase`; XG-header textures still bind null until TranslateGuestTexture. Resolve hook exists; float constants mostly land via unhooked `SetVertexShaderConstantFN` into GuestDevice. Clear/viewport are wired.
- **FM2→Unleashed convergence (2026-07-12):** POD queue complete. DEVICE_REMOVED (`INVALID_CALL`) fixed: no 1x Resolve, WaitForGpu must not Reset an open CL, Discard uninit RTs before clearRect/draw, map `k_8_8`/`0x2D20014A`→R8G8. Still: present sometimes loses `g_renderTarget` (~300 presents → no blit).
- Guest `D3DResource_Release`/`AddRef` (@ `0x82369E08`/`0x82369D90`) BE-atomic `ReferenceCount` at +4 — must fully hook FM2 `GuestResource` (host-LE atomic) or refcount/free paths corrupt. On zero call `ScheduleResourceDestruction`, never guest `sub_82369868`.
- **Upload helpers linkage:** `UploadFrameData` / `RetainTempUploadBuffer` must live in `fm2::render` (not anonymous namespace) — other TUs (`d3d_resource_hooks`) link them.
- **D3D12 NOT_ZEROED:** partial `clearColor(..., &rect)` does **not** initialize CREATE_NOT_ZEROED RT/DS — `discardTexture` (or full clear) required first.

## Do-Not-Repeat

<!-- Mistakes made and corrected. Each entry prevents the same mistake recurring. -->
<!-- Format: [YYYY-MM-DD] Description of what went wrong and what to do instead. -->
- [2026-07-12] Do not re-port `native_renderer/` or `FM2PlumeTrace*` when fixing black screen — port the missing SOURCE `render/` hooks into the cleaned direct-hook path instead.
- [2026-07-12] Do **not** assume FM2 skips `D3DDevice_SetTexture` in favor of fetch constants. IDA37: `D3DDevice_SetTexture` @ `0x8236C208` has 65 code xrefs from material/object/PM4 paths; `SetTextureFetchBitsLow/Mid` are tiny dirty-flag setters. Primary fix = Unleashed-style SetTexture hook.
- [2026-07-12] Never let guest `D3DResource_Release`/`AddRef` run on FM2 `GuestResource` — BE atomics vs host-LE `refCount`; on zero use `ScheduleResourceDestruction`, not guest free.
- [2026-07-12] Do not define `UploadFrameData`/`RetainTempUploadBuffer` inside an anonymous namespace in `render_state.cpp` — link errors from `d3d_resource_hooks.cpp`.
- [2026-07-12] Never `resolveTextureRegion` for 1-sample→1-sample StretchRect — D3D12 requires MSAA source; use `copyTextureRegion` (or Unleashed shader blit).
- [2026-07-12] `ProcWaitForGpu` must not `begin()` the currently open recording list (NDEBUG strips the assert) — close/submit first or use a closed slot.
- [2026-07-12] Do not Clear RT/DS with a scissor rect before Discard on CREATE_NOT_ZEROED heaps — debug layer DEVICE_REMOVED INVALID_CALL.

## Decision Log

<!-- Significant technical decisions with rationale. Why X was chosen over Y. -->
- 2026-07-11/12: Native renderer transfer = architectural rebuild of plume integration into cleaned `FM2/src/render/`; diagnostic `native_renderer/` overlay left behind on purpose.
