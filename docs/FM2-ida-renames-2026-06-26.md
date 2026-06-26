# FM2 IDA renames + findings — 2026-06-26 (render/PM4/vertex-fetch dig)

Driven by the plume_native black-screen investigation (see
`docs/FM2-plume-native-black-investigation.md`). Goal: re-verify the FM2 function names/assumptions
the render path trusts, fix the wrong ones, and map the real PM4 vertex/constant pipeline. Names
ending in `?` are best-effort / not fully certain. IDA db = `default.xex.i64` (ida38). Do NOT idb_save.

## passId-mirror fix: TRIED then REVERTED (load-bearing band-aid) — 2026-06-26

- **passId IS a texture/shader-state token, NOT a declaration** (confirmed: `FM2_D3D_EmitIndexedDrawPacket`
  0x82383718 packs param `a5`=passId into `FM2_D3D_EmitTextureStageStatePackets`; the decl field is the
  separate GuestDevice+0x2E24). So `Fm2SetActivePassId`'s `device->vertexDeclaration = passId;` is
  conceptually wrong.
- **BUT removing the mirror REGRESSED everything to black.** With the mirror gone,
  `SyncVertexDeclarationFromDevice` had only `MatchDeclarationForShader` as a source, which returns
  null for ~EVERY draw -> `FM2_DRAW_OUTCOME ok=0, skip=1626, missing_decl=1626` (vs the prior
  executing-but-collapsing baseline). So the mirror is **empirically LOAD-BEARING**: passId!=0 is
  resolved via the alias path into a usable layout; passId==0 falls through to the matcher. **REVERTED**
  (mirror restored, comment corrected). Builds clean.
- **KEY LESSON for the whole render path:** plume_native's declaration sourcing depends ENTIRELY on
  the passId band-aid — `MatchDeclarationForShader` is NOT a reliable primary source (it only matched
  in old FM2_DECLMATCH logs for the rare passId==0 draws). The real fix is a proper per-draw
  declaration from FM2's engine (the fetch constants at ctx+0x670 / the bound resource). Until then,
  do NOT remove the passId mirror.
- **NOT changed: the matcher heuristic + `ConvertDeclType`** (the matched type `0x2C259F` decodes
  correctly to FLOAT16_2; FM2 uses non-canonical packed types so the bitfield decode is needed).

## §9b CENSUS RESULT (FM2_RTCENSUS2, plume_native menu) — 2026-06-26
Per-draw render-target + POSITION0 census proves: across the whole menu run, **only TWO distinct
vertex shaders reach the draw path** — `292FF294` (the fullscreen BLIT, POSITION0) and `11460356`
(a sprite, no POSITION0) — both to a **single RT `0x130C7F000`** (1280x720). NO 3D car, NO main UI
quads (uint4-POSITION0 like Lost Odyssey), not even the other sprite shaders (`c2dd4577`/`01E38525`/
`4E770147`) from the old captures. With the passId mirror reverted those draws execute+collapse; with
it removed they all skip missing_decl. Either way the menu only ever has sprite+blit -> RT is black.
**Implication:** the main content (3D car, UI quads) is either not issued by the game in this state
(stuck early, like the race-load hang) OR uses a path that never reaches our D3D hooks. NEXT: log the
SKIPPED draws' shader hashes + count guest draw entries before our render path, to tell "issued but
skipped" from "never issued".

## Renames applied (ida38)

| addr | old | new | evidence |
|------|-----|-----|----------|
| 0x82723D18 | FM2_RenderContext_UploadMatrixConstantsFromPass_0 | **FM2_Render_PreparePassDrawItemOrder?** | builds identity index list (0..n) or sorts back-to-front by `abs(projected depth)` via `FM2_RenderContext_SortIndicesByProjectedValue` (0x82723C40, confirmed bubble-sort on `abs(float)`). NOT a matrix/constant upload — old name was WRONG. |
| 0x82A41BEC | dword_82A41BEC | **g_FM2_ActivePassRenderContext?** | the `this` passed to every `FM2_RenderContext_*` (BindVertexStream/UploadMatrixConstants/SetVertexShaderState/SetActivePassId). `stw`-stored per pass in `FM2_Render_BuildFallbackPassCommandBuffers` (0x82508178/0x825084A4). Its +0x700 = unified ALU constant file; +0x6xx = vertex-fetch constants. |
| 0x825064A8 | sub_825064A8 | **FM2_Render_BuildAndUploadScreenTransformToC0?** | builds screen/2D matrices (MultiplyMatrix4x4*VMX from pass data +224 + TLS ctx mats) and uploads: `UploadMatrixConstants(ctx, reg=0, m, 3)` → **c0-c2**, then reg=3, reg=7. THIS is the source of the `c0=1/640` 2D-ortho the UI/sprite shaders read. |
| 0x825063D0 | sub_825063D0 | **FM2_Render_BindPassDrawListState?** | `FM2_RenderTls_GetMainContextPtr`, sets pass-state ptrs (A/B), `FM2_RenderTls_BindPassStateToContextInner(g_FM2_ActivePassRenderContext, 1)`. |

Comments also set on: 0x82723D18, 0x82A41BEC, 0x82370E48 (fetch-constant layout), 0x825064A8.

## Key verifications / corrections (the "wrongly trusted" names)

- **`FM2_RenderContext_UploadMatrixConstants` (0x8236D958) = CORRECT.** `_R10 = &result[2*a2+224]`
  → writes `result + 0x700 + a2*16` (a2=start register). `result` = `g_FM2_ActivePassRenderContext`,
  which the x64dbg evidence shows IS the GuestDevice. So the render-path `device+0x700` read is right
  and `c0=1/640` is genuinely FM2's value. (The `+0x780` struct field is the unused D3D9-setter path.)
- **`FM2_RenderContext_SetActivePassId` (0x8236E228): passId is NOT a vertex declaration.** Writes a2
  to `result+11540`=`+0x2D14` (a pass-id / shader-state-dirty token consumed by
  `FM2_D3D_EmitDrawListStatePackets` 0x82383A70 → `FM2_D3D_EmitIndexedDrawPacket` +
  `FM2_D3D_CompareAndMarkShaderStateDirty`), NOT the declaration field `+0x2E24`. **BUG: our
  `Fm2SetActivePassId` hook (d3d_hooks.cpp:794) does `device->vertexDeclaration = passId`, corrupting
  the decl field with a flag every draw.** d3d_hooks.cpp:97-103/786-793 comments are FALSE. FIX:
  remove that mirror.
- **`FM2_RenderContext_SetVertexShaderState` (0x8236E010):** installs VS at ctx+0x3080 and copies the
  shader's compiled GPU-register state table (shader+0x37C → entries `{offset:u16,count:u16}` +
  payload) into the context register block at ctx+0x400 via FM2_MemcpyAligned + AND/OR masks. This is
  where the shader's GPU register defaults (incl. some fetch/format state) are applied.
- **`FM2_RenderContext_BindVertexStream` (0x82370E48) = the REAL stream bind** (FM2 PM4 engine, not
  D3D9 SetStreamSource). For stream s: fetch base+size → `ctx_word[2*(17-s)+412 / +413]`
  (ctx+0x670..0x6FC, right before ALU consts @ +0x700); resource ptr → `ctx_word[s+3045]`; stride/4
  byte → `ctx+0x2FD8+s`. base=D3DResource+0x18, size=+0x1C, stride from iface desc w08. **FORMAT is
  NOT here** — it comes from the D3DVERTEXELEMENT9 declaration.
- **FM2 creates CANONICAL declarations via `D3DDevice_CreateVertexDeclaration` (0x8236E240).**
  `FM2_Render_CreateCrowdSpriteVertexDeclaration` (0x82537598) builds a real D3DVERTEXELEMENT9 array
  ("CrowdSpriteDecl"): elem0 = POSITION0, **Type=0x2A23B9 (FLOAT3)** @0; others 0x2C23D9, 0x2A23D0,
  0x1A2306, 0x1A2107 — all canonical. So **our render-path matched type `0x2C259F` is NOT canonical**
  → the matching hack matched a wrong/synthetic decl (or mis-recorded the type byteswap), and the
  session-6i `ConvertDeclType` bitfield rewrite was probably unnecessary (ReOdyssey's full-value
  switch handles canonical types). The crowd-sprite VS uses POSITION0 FLOAT3 — so the collapsing
  TEXCOORD0/1 stride-8 sprites are a DIFFERENT subsystem.

## Architecture summary (FM2 render = PM4 engine, NOT D3D9-immediate)

`FM2_Render_SetupPassShaderAndVertexStreams` (0x827225A0) configures the global context
`g_FM2_ActivePassRenderContext` (BindVertexStream / UploadMatrixConstants / SetVertexShaderState /
SetActivePassId), then `FM2_D3D_EmitDrawListStatePackets` emits raw PM4 push-buffer packets
(`D3D_SubmitAndDrainCommands`). Our `render_state.cpp` is forked from ReOdyssey's **D3D9-immediate**
model (Lost Odyssey genuinely uses SetVertexDeclaration/SetVertexShaderConstantF). That mismatch is
why declaration recovery is a heuristic band-aid.

## Declaration object layout (D3DDevice_CreateVertexDeclaration 0x8236E240)
- 360 `D3DVERTEXELEMENT9` = **12 bytes** {Stream:u16@0, Offset:u16@2, Type:u32@4 (packed
  GPUVERTEXFETCHFORMAT), Method:u8@8, Usage:u8@9, UsageIndex:u8@10, pad@11}. Terminator Stream==255.
- Alloc `12*count + 56`; header memset 0x38; count @ decl[1].Common, maxStream @ decl[1].ReferenceCount;
  elements copied to **decl+0x38**. Our `GuestVertexElement` is already 12-byte (type=u32) → parse size
  is CORRECT. So the non-canonical matched type `0x2C259F` = we matched the WRONG declaration for the
  sprite shader (usage-set collision), not a parse bug. The matching heuristic needs the real per-draw
  decl (FM2 sets it via the engine/fetch constants, not device->vertexDeclaration).
- `FM2_RenderContext_SetVertexFetchModeBit` (0x82370080): sets ctx+10428 bit 0x10 from a2 + dirty
  bit 0x200 (a vertex-fetch mode toggle). Name OK.

## Pass 2 — draw-list submission + screen-ortho origin (2026-06-26)

Renames applied:
| addr | old | new |
|------|-----|-----|
| 0x825B6F60 | sub_825B6F60 | **FM2_Render_BuildScreenOrthoAndComposeMatrices?** — builds `v59[0]=2/W, v59[1]=-2/H` screen→NDC ortho from viewport w/h, composes with pass matrices (vmsum4fp128). **The ORIGIN of c0.x=2/1280=1/640.** |
| 0x825B7478 | sub_825B7478 | **FM2_Render_ConvertScreenPointPixelNdc?** — 2D point pixel↔NDC (`px/W*2-1`). |
| 0x8257E9F8 | ~~FM2_HashName_GetPropertyTablePtr~~ | **FM2_ReadObjDwordAt8?** — generic `return *(a1+8)`; prior name was a WRONG heuristic guess (here = viewport width). |
| 0x8242A2B8 | ~~FM2_ProfileState_GetControllerIdAt12~~ | **FM2_ReadObjDwordAt12?** — generic `return *(a1+12)`; prior name WRONG (here = viewport height). |

Findings:
- **`FM2_Render_UiOrScreenDrawListSubmit` (0x825B8A60) = the menu/UI submit path:** builds a 4×4 2D
  transform (vmsum4fp128), `UploadMatrixConstants(ctx=*(pass+164), reg=0, m, 4)` → **c0-c3**, then
  `FM2_D3D_EmitDirtyStateAndDrawList(ctx, drawList, 0)` (0x82375ED0). So UI draws upload the 2D ortho
  to c0 and emit the draw list. The c0=1/640 is genuinely the screen ortho — CONFIRMED at the source.
- **`FM2_Render_DispatchPm4DrawOpcode` (0x82722808) = a PM4 opcode jump table** (`bctr` switch on
  packet opcode); `FM2_Render_WalkAndDispatchPm4DrawList` (0x82722FD8) walks the recorded PM4 draw
  list and dispatches each opcode. This is FM2's draw-list REPLAY (records PM4, replays via this
  walker → opcode handlers → D3DDevice_* which we hook). Draw-list submission map (already-named):
  `ExecuteSortedDrawLists` (0x8252FF00), `SubmitSortedObjectDrawLists` (0x8251BC40),
  `ViewTraversalSortDrawLists` (0x8250CFF0), `EmitDirtyStateAndDrawList` (0x82375ED0),
  `UiOrScreenDrawListSubmit` (0x825B8A60). **NEXT (§9b):** trace which of these submission roots run
  in plume_native and which RT/command-list each targets — to find why the main content (3D car, UI
  quads) isn't in the captured/presented frame.
- The screen-ortho math confirms the paradox is real (c0=1/640 is correct), reinforcing §9b: the
  collapsing sprites are secondary; the missing MAIN content is the blocker.

## NEXT (continue the dig)
- Map `D3DDevice_CreateVertexDeclaration` (0x8236E240) + `FM2_D3D_CreateVertexDeclarationFromElements`
  (0x8259F008): capture ALL named declarations + their canonical element formats; fix matching to use
  the right one per draw (or read the fetch constants ctx+0x670 + the bound resource directly).
- Trace `FM2_Render_DispatchPm4DrawOpcode` (0x82722808) + `FM2_Render_EmitPassDrawWork` (0x8250F7C0)
  → how draws reach our D3D9 hooks, and whether the MAIN content passes (3D car, UI quads) are
  emitted to a command list we capture (the §9b dominant-blocker question).
- `FM2_RenderContext_SetVertexFetchModeBit` (0x82370080) — small, decode it.
- Re-name the remaining `sub_*` in the pass-build cluster (callees tool returned empty — use
  decompile refs).
