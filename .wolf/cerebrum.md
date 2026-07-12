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
- **FM2→Unleashed convergence (2026-07-12):** Phase A–C done (usage flags, device-lost latch, present coalesce, sync `RenderQueue`, 2-frame lists). StretchRect pending + MSAA CreateSurface + create/SetTexture on RT. Continue: per-frame upload allocators, `RenderQueue::Enqueue` for state setters, PresentImpl owns present-source, dropped ClaimPresentOwner. Still missing: POD `RenderCommand`+`Proc*`, intermediary uploads, DestructResource.

## Do-Not-Repeat

<!-- Mistakes made and corrected. Each entry prevents the same mistake recurring. -->
<!-- Format: [YYYY-MM-DD] Description of what went wrong and what to do instead. -->
- [2026-07-12] Do not re-port `native_renderer/` or `FM2PlumeTrace*` when fixing black screen — port the missing SOURCE `render/` hooks into the cleaned direct-hook path instead.
- [2026-07-12] Do **not** assume FM2 skips `D3DDevice_SetTexture` in favor of fetch constants. IDA37: `D3DDevice_SetTexture` @ `0x8236C208` has 65 code xrefs from material/object/PM4 paths; `SetTextureFetchBitsLow/Mid` are tiny dirty-flag setters. Primary fix = Unleashed-style SetTexture hook.

## Decision Log

<!-- Significant technical decisions with rationale. Why X was chosen over Y. -->
- 2026-07-11/12: Native renderer transfer = architectural rebuild of plume integration into cleaned `FM2/src/render/`; diagnostic `native_renderer/` overlay left behind on purpose.
