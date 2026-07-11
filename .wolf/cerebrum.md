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

## Do-Not-Repeat

<!-- Mistakes made and corrected. Each entry prevents the same mistake recurring. -->
<!-- Format: [YYYY-MM-DD] Description of what went wrong and what to do instead. -->

## Decision Log

<!-- Significant technical decisions with rationale. Why X was chosen over Y. -->
