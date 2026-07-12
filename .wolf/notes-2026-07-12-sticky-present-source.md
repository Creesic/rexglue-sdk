# Sticky present-source RT (2026-07-12)

## Problem
After ~300 presents, Present logged still no render target / no blit. Guest often unbound or destroyed the current RT before Swap; dangling g_renderTarget was cleared by IsLiveHostTexture in ProcClear.

## Fix
- Track g_lastPresentableRenderTarget on live SetRenderTarget
- Clear sticky/current/implicit pointers in DestructTempResources
- GetCurrentColorRenderTarget: live current -> last presentable -> implicit

## Verify (fm2_109.log)
Blit alive through present 1501; device lost 0; no no-blit.
Present source size drops 1280x720 -> 1280x256 after ~300 — next content issue.
