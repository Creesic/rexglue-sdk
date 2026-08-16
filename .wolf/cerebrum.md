# Cerebrum

> OpenWolf's learning memory. Updated automatically as the AI learns from interactions.
> Do not edit manually unless correcting an error.
> Last updated: 2026-07-06

## User Preferences

<!-- How the user likes things done. Code style, tools, patterns, communication. -->
- [2026-07-12] Prefer **UnleashedRecomp-clean** FM2 renderer patterns (deferred StretchRect, render-thread queue, Present split, SetTexture). Stay away from porting more **ReXGlue080plume** FM2-specific tiling machinery (`ResizeTileSurface`, band destY rebasing, `g_tileViewportOffsetY`, native_renderer overlay).
- [2026-07-12] Unbind active framebuffer before draining deferred StretchRect (1x copy / shader blit); Resolve may need last-presentable RT when `g_renderTarget` is already null.
- [2026-07-13] Tired of RenderDoc nibble-fixes; prefer **tranche ports** of known missing subsystems (constant transport / EmitDirty / samplers / decl→spec) over one-decl-at-a-time chases.

## Key Learnings

- **Detached VBlank (2026-07-12):** FM2 GameLoop waits on `dword_829C24C0`; wake path is `GraphicsInterruptCallback(r3=0)` → `VerticalBlankInterrupt` only if GPU MMIO `0x7FC86544` bit0 set (reg `0x1951`). Without `GraphicsSystem`, that range was unmapped and `REX_MM_LOAD_U32` left `_v` uninitialized → intermittent Swap-1 hangs. Fix: stub MMIO returning `1` for `0x1951` + zero-init MM loads.
- **Project:** ReXFM2P — "ReXGlue" static recompiler (Xbox 360 PPC → portable C++, Xenia/XenonRecomp-derived) plus `FM2/`, a downstream recompiled build of Forza Motorsport 2. Full architecture now documented in root `CLAUDE.md`.
- `FM2/` hand-written sources (`FM2/src/**`, `FM2/tests/**`, CMake/manifest) ARE tracked in git; `FM2/generated/**` and `FM2/out/**` are codegen/build output and stay untracked.
- Build requires Clang >= 18, C++23, RelWithDebInfo-only (Debug/Release presets are disabled in CMakePresets.json). Day-to-day commands go through the `PSReX` PowerShell module (`rex-configure`/`rex-build`/`rex-test`/etc.), not raw cmake.
- **FM2 native Plume renderer (branch `plume`):** intentional rebuild into `FM2/src/render/`, NOT a port of `ReXGlue080plume/FM2/src/native_renderer/`. That overlay (~5k lines: debug replay, compare window, `FM2PlumeTrace*` midasm hooks, PM4 scanner) was deliberately dropped. Production path is direct `REX_HOOK` D3D replacements against vendored `thirdparty/plume`. Commits: `d3748a68` (Phases 1–3), `34a4bf8a` (Phase 4 draws), `64d7a2a8` (present/audio/vsync fixes; still black).
- **Cleanup left real gaps vs SOURCE `render/`:** `D3DDevice_SetTexture` is now hooked (2026-07-12) into `rr::SetTexture`/`SetTextureBase`; XG-header textures still bind null until TranslateGuestTexture. Resolve hook exists; float constants mostly land via unhooked `SetVertexShaderConstantFN` into GuestDevice. Clear/viewport are wired.
- **2026-07-13 restructure gap (vs 080plume `render/`, not native_renderer):** clean path kept queue/StretchRect/`+0x700` but dropped deferred constant overlays (`SetLiveFloatConstantFiles`, pass/scene/obj WVP mirrors), `EmitDirtyStateAndDrawList` replay, `FlushSamplerStates`, decl-driven SPEC_CONSTANT_* at bind, colorWrite/MSAA flush guards, scene present-RT tracking. Hook count ~79 vs ~181. grok9: w≠0 fixed but RT still black / SamplesPassed=0; PS often has zero textures.
- **2026-07-13 Tier A landed then rolled back for crash bisect:** curated constant transport + EmitDirty object-pass replay + FlushSamplerStates + decl SPEC bits + flush guards caused mid-startup death (fm2_313/314). After pure RAW passthrough hooks + pre-Tier-A FlushRenderState (kept `+0x700` upload), user confirmed startup runs again. See `.wolf/notes-2026-07-13-tier-a-crash-bisect.md`.
- **2026-07-13 decl stride (grok10):** `ResolveVertexDeclaration` must not `max()` stale `vertexStrides` over live `inputSlots.stride`. Replay must rebind host streams from restored `ctx+0x2F94`/`+0x2FD8`. See `.wolf/notes-2026-07-13-decl-stride-fix.md`.
- **2026-07-13 grok11:** Decl fix confirmed (stride-8 → FLOAT16 TEXCOORD pair, not 32B FLOAT3). Still SamplesPassed=0: VS outputs collapse to **x≈y≈z** (degenerate). Capture is TEXCOORD-only UI draws only — no 3D POSITION mesh. Next: constants/transform or later-frame capture. Notes: `.wolf/notes-2026-07-13-fm2mmgrok11.md`.
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
- [2026-07-12] Do not mid-CL `ResizeTileSurface` for FM2 1280x256 tiles — DEVICE_REMOVED INVALID_CALL. Grow at `ProcCreateSurfaceHost` instead; expand VP/scissor on flush.
- [2026-07-12] Do not release `RecordingMutex` across DXGI present/fence wait, and do not exempt `CopyTextureFromUpload` from that mutex / call it on the guest thread — both made Swap-1 hangs/crashes worse. Keep CreateTranslatedTextureHost mutex-exempt only.
- [2026-07-12] Do not StretchRect/Resolve via raw `texture != nullptr` alone — use `IsLiveHostTexture` (holder match) and drop destroyed surfaces from `g_pendingSurfaceCopies` or plume `toD3D12` AVs on dangling `RenderTexture*`.
- [2026-07-13] Do not let `MatchDeclarationForShader` accept decls whose stream-0 elements overflow the bound VB stride — exact-count FLOAT3 layouts beat FLOAT16_4 and leave TEXCOORDs on slot-15 dummies → `SV_Position.w=0` → SamplesPassed=0 / black RT.
- [2026-07-13] Do not trust `device->vertexDeclaration` from SetActivePassId before matching — passId can alias an FM2 decl and bypass `MatchDeclarationForShader` entirely (grok7 still had 32B layout after stride reject). Prefer Match first; only accept device decl if it fits the live VB stride.
- [2026-07-13] Do not enable FM2 object-pass EmitDirty record/replay (`kObjPassRecordReplay`) without proving it — first enable crashed mid-startup (fm2_313) via guest-context scribble/replay. Keep Begin/Finalize/CreateClone passthrough until guarded.
- [2026-07-13] Do **not** wrap `FM2_D3D_EmitDirtyStateAndDrawList` in `REX_HOOK`/HostToGuest marshalling — wrong/fragile ABI kills startup. Use `REX_HOOK_RAW` + `g_orig*.fn(ctx, base)` only; re-enable Tier A flush extras one host-side piece at a time.
- [2026-07-13] Do **not** resolve decl stride via `max(vertexStrides, inputSlots.stride)` — a stale 32B pipeline stride accepts FLOAT3 while the live VB stays at 8 (fm2mmgrok10). Prefer `inputSlots` (what SetVertexBuffers binds), then guest `device+0x2FD8*4`. Never treat `streamStride==0` as “fits all”. Object-pass replay must re-`SetStreamSource` after restoring guest VB/strides.

## Decision Log

<!-- Significant technical decisions with rationale. Why X was chosen over Y. -->
- 2026-07-11/12: Native renderer transfer = architectural rebuild of plume integration into cleaned `FM2/src/render/`; diagnostic `native_renderer/` overlay left behind on purpose.
- 2026-07-12: User direction — stay on **Unleashed-clean** patterns; do not keep mining ReXGlue080plume for tiling/band machinery. FM2-only surface quirks (e.g. create-time 1280x256→720 host size) OK if minimal; no `g_tileViewportOffsetY` / band rebasing / mid-CL resize ports.
- 2026-07-12: Create-time host grow for 1280x256 tiles is a one-shot size override (not a 080plume tiling state machine).
- 2026-07-31: ReXGlue is a genuine source-AOT PPC recompiler, but its
  runtime is still an emulator-derived compatibility layer rooted in Xenia.
  Its architectural advantages over xrecomp are authoritative XEX PDATA
  function ranges, explicit PPCContext state, ordered graph analysis with
  fail-closed validation, typed host/guest ABI bridges, and assembled PPC
  fixtures recompiled through the production CLI. FM2 remains heavily
  title-integrated through manifest mappings, hooks, and renderer code; do
  not use it as evidence that ReXGlue projects require no manual integration.
