# FM2 plume_native Black-Screen Investigation — Master Working Doc

> **Purpose:** capture EVERY detail of the FM2 `--fm2_plume_mode plume_native` black-screen
> investigation so nothing is lost across context compaction. This is the authoritative
> working record. Update it as we test. Cross-references the auto-memory file
> `project_plume_native_black_root_cause.md` (sessions 6a–6L) and
> `project_plume_native_black_screen.md`.

Last updated: 2026-06-26 (session 6L + ReOdyssey reference check).

---

## 0. The goal & the symptom

- **Goal:** get Forza Motorsport 2 (FM2) to render visibly under `--fm2_plume_mode plume_native`
  (UnleashedRecomp/ReOdyssey-style AOT native rendering via plume RHI), NOT Xenos GPU emulation.
- **Symptom:** the game boots to a **fully black menu**. User reports there is normally a 3D car
  preview + menu items that look 3D. The game is also **stuck on the loading screen just before a
  race** (a separate, known producer-guard hang). Because the menu is black, the user can't
  navigate to the car preview in our build.
- **Path used:** `plume_native` uses the `render_state.cpp` → `Video` `g_commandList` path
  (`ShouldMirrorPlumeRenderState() == !nr::WantsReXGraphics()`). `fm2_native_renderer` (the
  Xenos-emulation "direct draw replay" experiment) is OFF.

---

## 1. THE PARADOX (the core unresolved thing)

For the menu's content vertex shaders, **all three inputs check out individually, yet the geometry
collapses**, and the real game (Xenia) renders it:

- **Shader is FAITHFULLY translated.** Original Xenos microcode (disassembled this session) matches
  the generated DXIL exactly. Position math for shader `11460356`:
  ```
  oPos = c0*(|tc0.x| + c4.x) + c1*tc0.y + c3
  ```
- **Constant `c0.x = 1/640`** — confirmed 4 ways (x64dbg, RenderDoc reflection, our HUDCONST dump,
  the `+0x700` read). It is a **pixel→NDC ortho** (1/640 = 2/1280) that expects `tc0` in PIXELS.
- **Vertex `tc0 ≈ 0.4` normalized** — confirmed 3 ways (raw guest bytes, RenderDoc decode, multiple
  format interpretations). The bytes are `0x367D` etc. = 0.4 as half / 0.4257 as SHORT2N / 13949 as
  raw SHORT2 — **never ~640 pixels in any sane format.**

Result: `oPos.x = (1/640)·(|0.4|−0.045) ≈ 0.0006` → sub-pixel sliver at screen center → 0 samples →
black. **For the real game to render, either `c0` must be ~2 (normalized scale) or `tc0` must be
~640 (pixels). Neither is true in our pipeline. This contradiction is unresolved.**

---

## 2. Shader disassemblies (original Xenos microcode, this session)

Captured via `FM2_VSASM` diag (see §6). Hashes are the microcode hash (== `shaderCacheEntry->hash`).
RenderDoc resource names use the **high 32 bits** (e.g. `11460356`).

### 2a. `0x11460356B7C40577` — UI content (TEXCOORD0+TEXCOORD1, no POSITION), elems=2
```
vfetch_full r2.xy__, r0.x, vf0      ; r2 = TEXCOORD0
vfetch_full r0.xy__, r0.x, vf0      ; r0 = TEXCOORD1 (different offset, disasm shows vf0 for both)
; alloc position
mul   r3, r2.yyyy, c1.wzyx
add   r0.__z_, r_abs[2].xxxx, c4.xxxx   ; r0.z = |tc0.x| + c4.x
mul   r4, r0.zzzz, c0.wzyx              ; r4 = (|tc0.x|+c4.x) * c0 (swizzled)
add   r1.x___, r3.wwww, r4.wwww         ; r1.x = tc0.y*c1.x + r0.z*c0.x
add   r1._yzw, r4.yyxz, r3.yyxz
add   oPos, r1.xwyz, c3
; interpolators (texcoord out uses c8, c9):
... sgt/sgt/add (sign of tc0.x -> o0) ...
mul   r1, r2.yyyy, c8.wzyx
mul/mad r0 with c6,c5,c7
dp4   r0.x, r1, c9.wzyx
max   o1.xy, r0.xyyy
```
Decoded oPos (via swizzles):
- `oPos.x = tc0.y*c1.x + (|tc0.x|+c4.x)*c0.x + c3.x`
- `oPos.y = (|tc0.x|+c4.x)*c0.y + tc0.y*c1.y + c3.y`
- `oPos.z = (|tc0.x|+c4.x)*c0.z + tc0.y*c1.z + c3.z`
- `oPos.w = (|tc0.x|+c4.x)*c0.w + tc0.y*c1.w + c3.w`
- With c0=(1/640,0,0,-0.518), c1.x=0, c3=(0,0,0,1), c4.x=-0.045 → oPos≈(0.0006, ~0, ~0, ~0.8).

### 2b. `0x01E3852515B88874` — content, elems=1 (single packed attr)
Complex unpack: `frc/trunc/sgt/mad` with `c253/c254/c255` (recompiler helper consts?), then position
from `c0/c1/c3/c4`. Same constant family. Same collapse risk.

### 2c. `0x4E770147F2C809C4` — content, elems=1
Like 2b plus extra interpolators using `c10/c11/c13`.

### 2d. `0x292FF29403B1DDF8` — the FULLSCREEN COMPOSITE/BLIT (POSITION0+TEXCOORD0), elems=2
```
vfetch_full r1.xy1_, r0.x, vf0      ; r1 = (posX, posY, 1, _)
vfetch_full r0.xy__, r0.x, vf0      ; r0 = texcoord
; alloc position
max oPos, r1.xyzz, r1.xyzz          ; oPos = POSITION attr DIRECTLY (passthrough! no constants)
; interpolators
mul r0.__zw, r0.xxxy, c255.xxxy     ; texcoord scale only (c255)
max o0, r0, r0
```
**This is a passthrough.** Its POSITION attr is already NDC (verts `(-1,+1)`, `(-0.75,+1)`). It does
NOT read the float constants for position. RenderDoc: **draw 825 samples_passed=921600 (=1280×720,
full screen), NO depth target.** So the composite WORKS; it just blits a black content RT (323).

---

## 3. Exact constant values

### 3a. Live low-register dump (session 6j, x64dbg, host 0x14004D800 = device+0x700)
```
c[0]=[1/640, 0,       0,      -0.469]     (1/640 = 0.0015625 = pixel ortho scale)
c[1]=[0,     1/360,   0,      -0.229]
c[2]=[0,     0,       1e-4,    0.5]
c[3]=[0,     0,       0,       1]
c[4]=[-1.2e-4, 0,     1,       0]
c[5]=[640,   0,       0,       0]        (screen width)
c[8]=[640,   360,     1,       1]        (screen HALF-dims)
c[9]=[0,     0,       0,       99999]
c[10]=[-1.8e-4, 42.46, -124.5, 1]
c[11]=[1,    0,       0,       0]
```

### 3b. HUDCONST dump (this session, shader 11460356, +0x700, big-endian decoded)
- c0 = (1/640, 0, 0, −0.518); c1.x=0; c3=(0,0,0,1); c4.x≈−0.045 (varies per draw: −0.045, −0.049…)
- `nz@0x700=18` nonzero floats out of 1024 — sparse but populated.

### 3c. RenderDoc reflection (capture_3/4 draw 497/419, cbuffer0 raw, little-endian GPU bytes)
```
c0 = (1/640, 0, 0, -0.518)
c1 = (0, 1/360, 0, -0.641)
c2 = (0, 0, 1e-4, 0.5)
c3 = (0, 0, 0, 1)
c4 = (-0.0451, 0, 1, ...)
c5.x = 640
```

### 3d. 292FF294 (the blit): `nz@0x700=0` — ALL-ZERO constants. Irrelevant: it's a passthrough.

---

## 4. Exact vertex data

### 4a. Draw 497 (RenderDoc decode_mesh_inputs, FLOAT16_2)
- Vertex buffer `863`/`771`, stride 8, TEXCOORD0 @ off 0, TEXCOORD1 @ off 4.
- V0: TEXCOORD0=(0.0, 0.405), TEXCOORD1=(1.0, -1.0)
- V1: TEXCOORD0=(0.012, 0.456), TEXCOORD1=(-1.0, 0.0)
- V2: TEXCOORD0=(0.037, 0.49), TEXCOORD1=(1.0, 1.0)
- V3: TEXCOORD0=(0.109, -0.396), TEXCOORD1=(1.0, -1.0)
- TEXCOORD1 corners are clean ±1.0; TEXCOORD0 ∈ [−0.45, 0.5] normalized.

### 4b. Raw guest bytes (FM2_HUDVB, SetStreamSource mappedMemory, BIG-endian, stride 8)
Shader 11460356, several draws (offset into shared VB):
```
off=203552: 367D0000 BC003C00 374F2240 0000BC00 37E228C8 3C003C00 B6582F00 BC003C00
off=231776: 30562750 BC003C00 31022080 0000BC00 31F29700 3C003C00 33F6A240 BC003C00
off=283896: AB913AE7 00003C00 A8C43AE7 00003C00 A8C439E2 00003C00 AC480000 00000000
```
- V0 @ 203552: bytes `36 7D 00 00 | BC 00 3C 00` → big-endian halves TEXCOORD0=(0x367D=0.28, 0.0),
  TEXCOORD1=(0xBC00=-1.0, 0x3C00=+1.0). (After our 32-bit byteswap + swappedTexcoords x↔y, the
  shader sees TEXCOORD0≈(0.28,0); RenderDoc shows the post-byteswap (0.0, 0.405) for a different
  vertex. Net: normalized ~0.4, byteswap CONFIRMED correct.)

### 4c. 292FF294 (blit) raw bytes (stride 16, POSITION0 FLOAT32_2 @ 0)
```
off=0: BF800000 3F800000 00000000 00000000 BF400000 3F800000 3E000000 00000000
```
- V0 POSITION = (0xBF800000=-1.0, 0x3F800000=+1.0) big-endian float32 → NDC corner. V1 = (-0.75, +1).

---

## 5. RenderDoc captures inventory & key findings

### Our captures (ReXGlue plume_native), in `C:\Users\Tera\Documents\GitHub\renderdoccaps\`
- `fm2_..._capture_3.rdc` (13MB) / `capture_4..8`, `..._15.20.48_capture_*` — **content frames**:
  ~58 DrawIndexedInstanced (events 480–811 / 402–805) + 2 DrawInstanced blits (825/870).
  **ALL 58 are the no-POSITION stride-8 TEXCOORD0/TEXCOORD1 UI type. All collapse
  (samples_passed=0).** Color RT 323 = 100% black; only depth 325 targeted; blit (825) renders
  full-screen but blits the black RT 323.
- `fm2_..._frame900.rdc` / `..._frame597.rdc` (~7MB) — **present-only frames**: 4 events (one blit +
  present). Most presented frames are like this → content draws are sporadic in the present stream
  (threading symptom; see §8). RT 323 black in these.
- **Forced-scale probe verification (capture_4, draw 419 = shader c2dd4577):** even after the probe
  set c0.x=2.0/c1.y=2.0 (RenderDoc-confirmed cbuffer0[0]=2.0), **samples_passed STILL 0**,
  "fully clipped", anomaly `depth_test_disabled`. BUT the probe used 11460356's swizzle model on
  c2dd4577 (not disassembled) → INCONCLUSIVE.

### Pipeline state for capture_4 draw 419 (= our collapsing UI draw)
- VS=ResourceId::1636 (RenderDoc "Invalid Shader Specified" for disasm), PS=1637 bound.
- Color target 323, depth target 325 bound. Viewport **mindepth=1, maxdepth=0 (reverse-Z)**.
  Scissor full 1280×720. `topology: null`. Index buffer 774 stride 2 (16-bit). VB 771 slot 0 stride 8.
- Input attributes all report `location:0` (D3D12 uses semanticName+index, so this is a RenderDoc
  artifact — see §7, semantics actually match).

### Xenia captures (the ORACLE — Xenia renders FM2), in `C:\Users\Tera\Documents\GitHub\xenia-edge\`
- `xenia_edge_..._frame1248.rdc` — **Vulkan, 5632 events, 3D RACE scene** (huge triangle strips,
  depth prepass `ps:0`, color draws later). Proof Xenia drives FM2 into 3D (which our build can't).
- `xenia_edge_..._frame517.rdc` — **D3D12, 5701 events, 3D scene + 2D HUD overlay** at the tail
  (event ~7319+): `rectangle list` / `quad list` native Xenos prims with real pixel shaders.
- `..._frame464.rdc`, `..._frame833.rdc` — not yet inspected.
- **Key Xenia facts:** Xenia does **vertex fetch IN-SHADER** from a 512MB shared-memory SRV
  (`Buffer 343`), NOT a D3D input layout. Its system cbuffer (slot 0) has **ndc_scale = 2.0** (Xenos
  clip-space → host NDC). Guest ALU float constants are in cbuffer **slot 1** (a UI draw's cbuffer1
  c0 = `(-0.308, 0.035, -2.77, 1434.8)`, c2.w=515.8 — a DIFFERENT shader than ours; the captures are
  a 3D race, not our exact menu, so NOT a clean same-draw comparison).
- Even with Xenia's ndc_scale=2.0, our `(1/640)·0.4·2 = 0.001` still collapses → the equivalent
  Xenia shader MUST be fed a different c0 or tc0. **Only a same-draw (same menu) capture resolves it.**

---

## 6. Diagnostics currently in the tree (capped, write to `C:\temp\fm2-clean.log`)

| Tag | File / function | What it logs |
|-----|-----------------|--------------|
| `FM2_VSASM` | `d3d_resource_hooks.cpp` `CreateShaderFromFunction` | Disassembles original Xenos microcode for no-POSITION VS (+ dedicated slot for hash 0x292FF294…). Uses `Shader.physicalOffset/size` (vs[0]/vs[1]) at `function+shaderOffset`, byteswap to host, `rex::graphics::Shader::AnalyzeUcode` → `StringBuffer.to_string()`. |
| `FM2_HUDVB` | `render_state.cpp` `SetStreamSource` | First 32 raw big-endian guest bytes of bound VB slot 0 for no-POSITION (+ 292FF294) shaders. |
| `FM2_HUDCONST` | `render_state.cpp` `FlushRenderState` | c0/c1/c3/c4 (from `device+0x700`) + `nz@0x700` count, for no-POSITION (+ 292FF294) shaders. |
| `FM2_DECLMATCH` | `render_state.cpp` `SyncVertexDeclarationFromDevice` | Matched decl: shader header usage set + decl elements `u<usage>.<idx>@<off>/t<type>/s<stream>`. |
| `FM2_SYNC_DECL` | `render_state.cpp` `SyncVertexDeclarationFromDevice` | `device->vertexDeclaration` (always 0). |

Observed matches:
- `FM2_DECLMATCH hash=0x292FF294... stride=16 decl[u0.0@0/t2892709/s0 u5.0@8/t2892709/s0]` →
  type 0x2C25E5 = fmt 0x25 = **FLOAT32_2** for POSITION0+TEXCOORD0.
- `FM2_DECLMATCH hash=0x11460356... stride=8 decl[u5.0@0/t2892639/s0 u5.1@4/t2892639/s0]` →
  type **0x2C259F** = fmt 0x1F = FLOAT16_2 for TEXCOORD0+TEXCOORD1. **NOTE: 0x2C259F is NOT a
  canonical D3DDECLTYPE** (ReOdyssey FLOAT16_2 = 0x2C235F). See §9.

**The forced-scale PROBE (c0.x*=1280, c1.y*=720 for no-POSITION shaders) was REVERTED.** Tree builds
clean. Diagnostics remain.

---

## 7. RULED OUT (do not re-investigate without new evidence)

- **Shader mistranslation** — Xenos asm == DXIL. Faithful.
- **Input semantic/location mismatch** — XenosRecomp `shader_recompiler.cpp:1933` emits
  `i{var}{usageIndex} : {SEMANTIC}{usageIndex}` → TEXCOORD0 = semantic **"TEXCOORD0"**. The
  `[[vk::location(13)]]` is Vulkan-only. plume `plume_d3d12.cpp:3202-3203` matches D3D12 by
  `semanticName`+`semanticIndex` (ignores `RenderInputElement.location`). Our IA
  (`CompleteVertexDeclaration`: `semanticIndex=usageIndex`) declares "TEXCOORD0" → **MATCHES**. Inputs
  DO reach the shader. (POSITION0 works because semanticIndex 0 == location 0.)
- **Vertex byteswap** — UnlockVertexBuffer 32-bit byteswap + `swappedTexcoords` x↔y for FLOAT16_2 =
  net correct. RenderDoc decode agrees with raw-byte decode.
- **Vertex is pixels** — NO format interpretation of the bytes yields ~640. Genuinely ~0.4 normalized.
- **Constant source = stale** — `+0x700` is where FM2's engine (`UploadMatrixConstants`, IDA
  0x8236D958, writes `result+0x700+a2*16`) puts ALU constants; `result==device`. c0=1/640 is real.
- **Present-thread race** — session 6: forced single present, still black. `open=0` at present is
  benign.
- **292FF294 zero constants** — it's a passthrough blit; zero constants don't affect it.

---

## 8. Other established facts / separate issues

- **Threading:** ~44 host tids touch `Video::Present()` (likely fiber/thread-pool migration of FM2's
  ONE render thread, `FM2_CRenderThread_Ctor` 0x82289670 → `FM2_RenderThread_Main` 0x82289640 →
  `FM2_RenderThread_RunFrame` 0x82288948). Most presented frames are blit-only → content draws are
  sporadic in the present stream. `876,605 draws OK vs 16,011 skipped` (DRAW_OUTCOME) = content IS
  recorded, but RenderDoc proves the captured GPU output is black (every captured content draw
  collapses). Present source = `130C7F000`.
- **Loading hang before race:** separate producer-guard issue (game stuck just before race start).
- **Reverse-Z:** `FlushViewport` (render_state.cpp:2010) sets minDepth=1/maxDepth=0 when
  `SceneReverseZ()`. Content draws bind depth 325 + reverse-Z; the working blit (825) has NO depth
  target. Not yet ruled in/out as a second gate.
- **Earlier real fixes (session 6i, kept):** (1) declaration-matching recovery
  (`MatchDeclarationForShader`/`SnapshotGameDeclarations`/`RegisterGameDeclaration`); (2) 360 format
  decode rewrite of `ConvertDeclType`/`ConvertPositionDeclType` (was returning UNKNOWN for all real
  decls). These target POSITION (3D) shaders which aren't in the menu captures.

---

## 9. ReOdyssey REFERENCE (the big lead — FM2's render-code PARENT)

ReOdyssey = an UnleashedRecomp-style AOT recompiler using **our exact architecture** (XenosRecomp +
native RHI). FM2's `src/render` is a fork of it. **Identical file set + byte-identical `GuestDevice`.**

- Source: `C:/Users/Tera/Documents/GitHub/ReOdyssey/src/render/` (`render_state.cpp`,
  `d3d_resource_hooks.cpp`, `guest_device.h`, `guest_resources.h`, `pipeline.cpp`, `video.cpp`, …).
- XenosRecomp source: `C:/Users/Tera/Documents/GitHub/ReOdyssey/thirdparty/XenosRecomp/XenosRecomp/`
  (`shader_recompiler.cpp` — input gen line 1933; location table line 77+; isPosition0 uint input
  line ~1866).

### Divergences found (FM2 vs ReOdyssey) — TOP SUSPECTS
1. **VS constant upload.** ReOdyssey (`render_state.cpp:1908`):
   `allocateCopy<true>(device->vertexShaderFloatConstants, sizeof(...), 0x100)` → **+0x780** (struct
   field). FM2 (`FlushRenderState`): `allocateCopy<true>(devBytes + 0x700, ...)` → **hardcoded +0x700**
   (8 registers BEFORE the struct field). FM2's own `SetVertexShaderConstantFN` (d3d_hooks.cpp:1580)
   WRITES `device->vertexShaderFloatConstants` = +0x780. So FM2's D3D9 write path (+0x780) and read
   path (+0x700) are 8 registers apart; FM2's game uses the engine path (+0x700) instead. **Needs
   verification that +0x700 (not +0x780/+0x680) is truly correct for the UI shaders.**
2. **Vertex declaration.** ReOdyssey uses the **bound** `device->vertexDeclaration` (offset 0x2E24,
   canonical types). FM2's `vertexDeclaration` is ALWAYS 0 → FM2 heuristically MATCHES. The matched
   UI decl type `0x2C259F` is **NOT canonical** (ReOdyssey `D3DDECLTYPE_FLOAT16_2 = 0x2C235F`; differs
   in middle/format bits — bit 2 of middle byte set, fmt still 0x1F). **May be matching a WRONG decl.**
3. **`ConvertDeclType`.** ReOdyssey switches on FULL packed values (FLOAT2=0x2C23A5, FLOAT16_2=0x2C235F,
   SHORT2=0x2C2359, SHORT2N=0x2C2159, USHORT2N=0x2C2059, FLOAT16_4=0x1A2360, …). FM2 REWROTE it
   (session 6i) to a bitfield decode (`fmt=type&0x3F, numfmt=(type>>8)&3, bit11=BGRA`). If the bitfield
   maps some real type wrong, vertex decodes wrong.

### Canonical D3DDECLTYPE values (from ReOdyssey guest_device.h, for reference)
```
FLOAT1=0x2C83A4 FLOAT2=0x2C23A5 FLOAT3=0x2A23B9 FLOAT4=0x1A23A6
D3DCOLOR=0x182886 UBYTE4=0x1A2286 UBYTE4_2=0x1A2386
SHORT2=0x2C2359 SHORT4=0x1A235A UBYTE4N=0x1A2086 UBYTE4N_2=0x1A2186
SHORT2N=0x2C2159 SHORT4N=0x1A215A USHORT2N=0x2C2059 USHORT4N=0x1A205A
UINT1=0x2C82A1 UDEC3=0x2A2287 DEC3N=0x2A2187 DEC3N_2=0x2A2190 DEC3N_3=0x2A2390
FLOAT16_2=0x2C235F FLOAT16_4=0x1A2360 UNUSED=0xFFFFFFFF
```

---

## 9b. LOST ODYSSEY oracle (ReOdyssey rendering it in OUR architecture) — DECISIVE convention

Capture: `C:\Users\Tera\Documents\GitHub\renderdoccaps\lostodysseymenu.rdc` (D3D12, 174 events =
ReOdyssey/plume rendering Lost Odyssey's menu). Lost Odyssey XEX also open in IDA.

**Draw 489 (UI quad, 708 idx, RENDERS — samples_passed=16116):**
- VS `Shader {ce2840f8}` (hash ce2840f82b204571…). Input signature:
  ```
  uint4 POSITION;  float4 TEXCOORD;  float4 COLOR0;  float4 COLOR1;   (stride 48, buffer 360)
  ```
  POSITION is **uint4** → `asfloat()` bitcast in-shader (XenosRecomp `useUintInput`). Values are
  **FLOAT32 PIXELS**: V0 POSITION = (1136885760, 1141997568, 0, 1065353216) = **(390.0, 582.0, 0.0,
  1.0)**.
- Math: full 4×4 matrix `oPos = POSITION.x*c0 + POSITION.y*c1 + POSITION.z*c2 + w*c3`, constants from
  **cbuffer0 register b0 space4 = `_VertexShaderConstants float4[256]`** at byte offsets 0/16/32/48.
  cbuffer0 c0 = `(1/640, 0, 0, 0)` — **c0.x = 1/640, IDENTICAL to FM2**.
  (Raw c0..c3: c0=(1/640,0,0,0) c1=(-1/360,0,0,1.0) c2=(0,-1.0008,1.0014,0) c3=(1.0,-0,0.0017,0.010)
  — a perspective-ish 2D matrix; oPos.w≈POSITION.y.)
- cbuffer0 = `b0 space4`, cbuffer1 = `b2 space4` (`_SharedConstants`, same ABI as ours). Shader reads
  `cbuffer1.Load4(256)` bit 0 for the swappedTexcoords swap of TEXCOORD.x/y.
- Viewport **mindepth=0, maxdepth=1 (NORMAL)** — vs FM2 content draws reverse-Z (1→0). Color RT 317,
  NO depth target. `depth_test_disabled`.

**CONCLUSION — convention is unambiguous:** working UI feeds **large positions (hundreds) via a
raw-`uint4` POSITION0**, and `c0=1/640` (×matrix) maps them to NDC. FM2's collapsing shaders instead
have **no POSITION0**; they take a **float `TEXCOORD0`=0.4** and do `oPos=c0*(|tc0.x|+c4.x)+c1*tc0.y+c3`
with `TEXCOORD1=±1` corners → that is **point-sprite/billboard expansion**, a DIFFERENT shader class.
So either (a) FM2's real menu UI quads (POSITION0 raw-uint, large coords) are NOT in our 58-draw
captures, or (b) FM2's sprite shaders are normalized and read the wrong c0. **NEXT: get FM2's actual
VS input signature for a collapsing draw — does it declare uint4 POSITION0 (we mishandle) or float
TEXCOORD0 (genuine sprite)?** Also: ReOdyssey's input-layout build sets POSITION0 to a raw-uint
format; confirm FM2's `ConvertPositionDeclType`/`useUintInput` path matches.

**Lost Odyssey IDA is open** — use it to confirm how LO writes VS constants (D3D9
`SetVertexShaderConstantF` → device->vertexShaderFloatConstants @ +0x780) and binds vertex
declarations (`SetVertexDeclaration`), establishing what FM2's engine/PM4 path diverges from.

### 9b-CONCLUSION (MAJOR REFRAME, 2026-06-26): we've been analyzing the wrong draws
- FM2 shader `11460356` header = `{TEXCOORD0, TEXCOORD1}` (usage=5) → XenosRecomp generates **float
  TEXCOORD inputs**, NOT a `uint4` POSITION0. So it is a **genuine sprite/billboard shader** (the
  `|abs(tc0)|` + `TEXCOORD1=±1 corner` pattern), not a mishandled UI quad. Option (a), not (b).
- **All 58 draws in our captures are this sprite/packed-attr subpass.** Lost Odyssey-style **main UI
  quads (uint4 POSITION0, large coords, stride ~48) are ABSENT from every ReXGlue capture.** So are
  the **3D car** draws (POSITION0 + c28/c36 matrices). `FM2_DRAW_OUTCOME` shows **876k draws** happen,
  but a captured frame contains only ~58 (sprite subpass) or 0 (blit-only present frame).
- **Therefore: the black menu is NOT (only) the sprite collapse — FM2's MAIN content (3D car + UI
  quads) is being drawn (876k draws) but is NOT reaching the captured/presented frame.** This points
  back at the **threading / command-list / submit-present path** (FM2's parallel job system records
  draws across many command lists; only a sprite-subpass list reaches the presented frame). The
  sprite-subpass collapse is likely a real-but-secondary bug (and those off-screen sprites may even
  be *meant* to be tiny). RT 323 is cleared each captured frame and only gets the collapsed sprites
  → black; the blit (292FF294) faithfully shows that black RT.
- **PIVOT:** stop chasing the sprite c0/vertex paradox as THE blocker. Find WHERE FM2's main content
  draws (3D car, UI quads) go and why they don't reach the presented frame. Confirm by capturing a
  frame that DOES contain a uint4-POSITION0 UI quad or the 3D car (may require capturing a different
  point, or instrumenting which command list / RT the POSITION0 draws target vs what is presented).

## 10. THINGS TO TEST (checklist — leave nothing untested)

- [ ] **T11. [NEW TOP PRIORITY per §9b-CONCLUSION] Find FM2's main content draws.** Instrument the
  draw path to log, per draw: the VS header usage set (does it have POSITION0?), the bound color RT,
  and the command list / thread. Determine whether uint4-POSITION0 UI quads + the 3D car are drawn at
  all in plume_native, which RT they target, and whether that RT/command-list reaches Present. This
  is the likely real blocker (threading/command-list), reframed from the sprite collapse.
- [ ] **T12. Capture a frame containing a POSITION0 UI quad or the 3D car.** Our 58-draw captures are
  a sprite subpass only. Get a capture (different timing / different menu screen) that includes a
  uint4-POSITION0 draw; confirm whether it renders or collapses, and compare to Lost Odyssey draw 489.
- [ ] **T1. Systematic ReOdyssey diff** of FM2 `render_state.cpp` + `d3d_resource_hooks.cpp` (and
  `pipeline.cpp`, `video.cpp`) vs `C:/Users/Tera/Documents/GitHub/ReOdyssey/src/render/*`. Enumerate
  EVERY divergence; classify each as (legit FM2 difference) vs (introduced bug). **[recommended first]**
- [ ] **T2. Constant offset.** Log c0 (and the c28/c36 matrix region) from `+0x680`, `+0x700`, `+0x780`
  for BOTH UI (no-POSITION) and 3D (POSITION) draws. Determine the single correct base. Test uploading
  `device->vertexShaderFloatConstants` (+0x780, ReOdyssey way) and see if UI renders / 3D breaks.
- [ ] **T3. Matched declaration correctness.** Is `0x2C259F` a real FM2 decl or did
  `RegisterGameDeclaration`/the matcher pick a wrong one? Dump ALL registered game declarations +
  their types. Check if a canonical `0x2C235F` decl exists and should have matched instead.
- [ ] **T4. ConvertDeclType.** Diff FM2's bitfield decode vs ReOdyssey's full-value switch for the
  exact types FM2 produces. Confirm FLOAT16_2 path → R16G16_FLOAT either way.
- [ ] **T5. Reverse-Z / depth gate.** Does disabling depth test (or fixing reverse-Z compare/clear)
  on content draws change samples_passed? Content binds depth 325 + inverted viewport; blit doesn't.
- [ ] **T6. Re-run the scale probe CORRECTLY** on a draw whose shader we disassembled (11460356), not
  c2dd4577. Confirm whether spreading x/y actually rasterizes (rules collapse-vs-depth).
- [ ] **T7. Xenia same-menu capture.** Run `xenia_edge.exe` on FM2, navigate to the SAME main menu
  that's black in our build, RenderDoc-capture it. Find the UI shader by its `oPos=c0·(|tc0|+c4)`
  signature; read Xenia's c0 (cbuffer1) + the fetched vertex (from Buffer 343 shared memory at the
  fetch offset). Diff against ours (c0=1/640, tc0=0.4). **[definitive but needs user to run Xenia]**
- [ ] **T8. Xenia/ReXGlue instrumentation ("spill everything").** Add log lines to Xenia's command
  processor (xenia-edge source) OR our in-tree `src/graphics/command_processor.cpp` to dump per-FM2-
  draw the **vertex-fetch constant (format/stride/base/exp)** and **low ALU constants c0–c8**. The
  fetch constant may carry an exponent/scale we don't apply (we use a plain FLOAT16_2 IA).
- [ ] **T9. PM4 vertex-fetch decode.** FM2 defines vertex format via Xenos vfetch constants (PM4), not
  D3D9 decls. `FM2/src/native_renderer/fm2_direct_draw_decode.h` + `fm2_shader_analysis.h` already
  decode these (used only in the OFF native_renderer replay). Wire/compare the real fetch format.
- [ ] **T10. NDC scale/offset.** Confirm whether XenosRecomp bakes the Xenos→host NDC transform into
  the generated VS or expects the runtime to apply it (Xenia applies ndc_scale=2.0 via system cbuffer).
  The blit renders, so it's probably baked — but verify for the content shaders.

---

## 11. Build / run / debug environment (CRITICAL constraints)

- **Build ONLY with VS18** (Clang from VS18). Exact command (the link error
  `__std_find_first_not_of_trivial_pos_1` happens if VS2022's MSVC 14.44 libs shadow VS18's 14.51):
  ```
  cmd /c 'call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 && cd /d "C:\Users\Tera\Documents\GitHub\ReXGlue080plume\FM2" && cmake --build --preset win-amd64-relwithdebinfo --target fm2'
  ```
  (Must `cd` into `FM2/` inside the vcvars cmd, else `--preset` resolves to the SDK preset → "unknown
  target fm2".) `render_state.cpp`/`d3d_resource_hooks.cpp` compile into `fm2.exe` directly (NOT the
  runtime DLL) — no DLL sync needed for those.
- **Log path:** `C:\temp\fm2-clean.log`. Archive before each run.
- **x64dbg:** guest memory at host membase `0x100000000`. GuestDevice guest `0x4004D100` → host
  `0x14004D100`; VS const file `device+0x700` → host `0x14004D800`. **NEVER kill x64dbg** (use `stop`
  in x64dbg MCP if connected; only `Stop-Process fm2` if x64dbg NOT connected). Do NOT use
  `ida38 idb_save`.
- **Working tree is intentionally dirty** (in-flight debugging + generated output). Avoid destructive
  git. User commits ("I'll commit, you build").
- **User runs FM2** (it needs interaction to reach menus). I cannot launch it to the menu myself.

## 11b. IDA RE-VERIFICATION (2026-06-26) — code-diff + IDA dig (user: "we wrongly named functions")

Code diff (FM2 vs ReOdyssey `src/render/`) found FM2 overrides ReOdyssey's working logic in exactly
3 places, all resting on IDA assumptions: (1) constant upload `device->vertexShaderFloatConstants`
(+0x780) → hardcoded `+0x700`; (2) bound declaration → matching hack; (3) `ConvertDeclType`
full-value switch → bitfield rewrite. Re-verified each in IDA (ida38 = FM2 default.xex.i64):

- **CONSTANT OFFSET +0x700 = CORRECT (not a bug).** `FM2_RenderContext_UploadMatrixConstants`
  (0x8236D958): `_R10 = &result[2*a2+224]` → writes **`result + 0x700 + a2*16`** (a2=start reg). The
  `result` is the GLOBAL render context `dword_82A41BEC`. By the x64dbg evidence (session 6j: device+
  0x700 host 0x14004D800 = 1/640) the render context **IS** the GuestDevice (0x4004D100). So reading
  device+0x700 is right; **c0=1/640 is genuinely FM2's value.** (The ReOdyssey struct field at +0x780
  is the unused D3D9-setter path; FM2 writes via the engine to +0x700.)
- **passId IS NOT A VERTEX DECLARATION — our hook is WRONG (real bug).** `FM2_RenderContext_SetActivePassId`
  (0x8236E228) writes a2 to `result+11540` = **`+0x2D14`** (passId field), NOT the declaration field
  `+0x2E24`. `FM2_D3D_EmitDrawListStatePackets` (0x82383A70) reads `+11540` as `v8` and feeds it to
  `FM2_D3D_EmitIndexedDrawPacket` + `FM2_D3D_CompareAndMarkShaderStateDirty` — it's a **pass-id /
  shader-state-dirty token**. Our `Fm2SetActivePassId` hook (d3d_hooks.cpp:794) does
  `device->vertexDeclaration = passId` → **corrupts +0x2E24** with a flag (0 most calls, sometimes a
  garbage value like 0x2E0086E0 that the alias path may wrongly resolve). The d3d_hooks.cpp:97-103 /
  786-793 comments ("passId is a D3DVERTEXELEMENT9 declaration") are conceptually FALSE (passId is a
  texture/shader-state token: EmitIndexedDrawPacket packs it into EmitTextureStageStatePackets). **BUT
  the mirror is EMPIRICALLY LOAD-BEARING — removing it made MatchDeclarationForShader the sole decl
  source, which returns null for ~every draw -> ok=0, all missing_decl, total black. TRIED then
  REVERTED (2026-06-26). Do NOT remove it until a real per-draw declaration source exists. The whole
  plume_native decl path depends on this band-aid. See docs/FM2-ida-renames-2026-06-26.md §passId.**
- **FM2 IS A PM4-BASED ENGINE (the root architectural mismatch).** `FM2_Render_SetupPassShaderAndVertexStreams`
  (0x827225A0) configures all state on the **global render context `dword_82A41BEC`**:
  `FM2_RenderContext_BindVertexStream(ctx, stream, resource, ...)` (0x82370E48, the REAL stream bind —
  "D3DResource+0x18=base, +0x1C=size, stride from iface desc w08", stores into ctx at
  +413*4/+412*4/+3045*4/+12248), `FM2_RenderContext_UploadMatrixConstants(ctx, ...)`,
  `FM2_RenderContext_SetVertexShaderState(ctx, ...)`, then `FM2_D3D_EmitDrawListStatePackets` emits
  raw **PM4 push-buffer packets** (e.g. `*v17 = 0xC0xxxxxx` headers, `D3D_SubmitAndDrainCommands`).
  FM2 does NOT use the D3D9-immediate API the way ReOdyssey's game (Lost Odyssey) does. Our
  render_state.cpp is forked from ReOdyssey's **D3D9-immediate** model, so the vertex declaration +
  format are recovered by heuristic matching — a band-aid over FM2's real PM4 vfetch-constant setup.
  The real vertex FORMAT lives in the PM4 vertex-fetch constants set by `BindVertexStream` /
  `SetVertexShaderState`, not a D3D9 declaration.
- Callers of SetActivePassId (= the per-pass setup sites, useful map): `FM2_Render_UploadPassTransformConstants`
  (0x82515528), `FM2_Render_BuildDirectIndexedDrawBuffers` (0x825380B8), `FM2_Render_InstanceHybridDrawPath`
  (0x82539650), `render_pass_execute_draw_batch_with_state_save` (0x825AF3C0),
  `FM2_Render_SetupPassShaderAndVertexStreams` (0x827225A0), `FM2_Render_DispatchPm4DrawOpcode` (0x82722808).
- `dword_82A41BEC` reads are all over `FM2_Render_EmitPassDrawWork` (0x8250F7C0),
  `FM2_Render_BuildFallbackPassCommandBuffers` (0x825080E0), `FM2_Render_PrepareSceneSliceTransforms`
  (0x825078D0) — the per-pass PM4 build path. **NEXT IDA:** confirm where `dword_82A41BEC` is
  initialized (is it the D3D device or a wrapper?), and decode the vertex-fetch constants
  `BindVertexStream`/`SetVertexShaderState` write into the context (the real per-attribute format).

## 12. Key IDA functions (named)
- `FM2_RenderContext_UploadMatrixConstants` 0x8236D958 — writes `result+0x700+a2*16` (a2=start reg).
- `FM2_Render_UploadDrawListMatrixConstants` 0x82522418 — calls with a2=36 (3D WVP at c36); launch
  bat default `c36_mul_c28` (3D shaders read c28/c36; UI shaders read c0).
- `FM2_RenderContext_SetActivePassId` 0x8236E228 — `*(ctx+11540)=passId; *(ctx+16)|=0x80000` (passId
  is a FLAG, NOT a vertex declaration — the old +11540 hook assumption was WRONG).
- Render thread: `FM2_CRenderThread_Ctor` 0x82289670, `FM2_RenderThread_Main` 0x82289640,
  `FM2_RenderThread_RunFrame` 0x82288948, `FM2_GpuCommandBuffer_BuildAndSubmit` 0x8236cb28.

---

## 13. UNIMPLEMENTED / DIAGNOSTIC-ONLY / BAND-AID GAPS — full audit (2026-06-26)

This is the authoritative list of every place the plume_native render path is incomplete. It explains
why content is missing: FM2 routes draws through SEVERAL emitters; only SOME are translated to native
Plume draws. All file:line are in `FM2/src/render/` unless noted.

### 13a. The draw-emit dispatch map (THE key table)
FM2 (a PM4 engine) emits draws through multiple guest functions. Each is REX_IMPORT-hooked in
`d3d_hooks.cpp` (decls at lines 167-193). Status in plume_native:

| Guest emitter (IDA) | hook (d3d_hooks.cpp) | plume_native behavior | TRANSLATED? |
|---|---|---|---|
| `FM2_D3D_EmitIndexedDrawPm4Packets` (ctx,prim,ibBase,idxCount) | `Fm2EmitIndexedDrawPm4Base` :2200 | `ApplyLiveColorWriteFromContext` + `ApplyLiveTexturesFromContext` + `DrawIndexedVertices(...)` | **YES** ✅ |
| `FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset` | `Fm2EmitIndexedDrawPm4WithGpuOffset` :2492 | → `SubmitNativeIndexedDrawPm4` → `DrawIndexedVertices` | **YES** ✅ |
| `FM2_D3D_EmitIndexedDrawPm4WithVertexFormatSetup` | `Fm2EmitIndexedDrawPm4WithVertexFormatSetup` :2506 | → `SubmitNativeIndexedDrawPm4` | **YES** ✅ |
| `FM2_D3D_EmitDirtyStateAndDrawList` (ctx,drawNode,flags) 0x82375ED0 | `Fm2EmitDirtyStateAndDrawList` :2525 | calls original + DECODES `drawNode+116` list, logs `FM2_DIRTY_DRAW`. **Never calls DrawIndexedVertices.** | **NO** ❌ (GAP 1) |
| `FM2_Render_WalkAndDispatchPm4DrawList` 0x82722FD8 | `REX_HOOK_RAW` :2736 | counts "WalkDispatch", calls original | **NO** ❌ (GAP 2, count-only) |
| `FM2_D3D_EmitSurfaceResolvePackets` | `Fm2EmitSurfaceResolve` :2745 | aliases resolve-dest (`ctx+10652`) → snapshot of plume surface | **PARTIAL** ⚠️ (GAP 3) |

The draws we DO see in plume (`FM2_RTCENSUS2`: sprite `11460356` + blit `292FF294`, RT `0x130C7F000`)
come from the `EmitIndexedDrawPm4*` path (✅). **The forward COLOR pass + UI go through
`EmitDirtyStateAndDrawList` (❌) — untranslated — so the color RT is never written → black.**

### 13b. GAP 1 (BIGGEST): `FM2_D3D_EmitDirtyStateAndDrawList` forward-color/UI not translated
- **Where:** hook `Fm2EmitDirtyStateAndDrawList` `d3d_hooks.cpp:2525-2548`; guest 0x82375ED0; decl
  comment :176-180 ("forward COLOR pass goes through here and is NOT translated to native draws ->
  the color RT never gets written -> black. Diagnostic hook to decode the draw list before
  translating.").
- **What it currently does:** calls original (builds PM4 into the DISABLED guest ring → nothing
  renders), then logs `FM2_DIRTY_DRAW` decoding: `nodeFlags=drawNode+108`, `listHead=drawNode+116`,
  `count=listHead+4`, per-draw entries at `listHead+8,12,16,20,24,28`.
- **What it MUST do:** walk the draw list (count + entries), extract per-draw (primType, startIndex,
  indexCount) + bound state, and `DrawIndexedVertices(...)` each into plume (mirror the `Pm4Base`
  pattern). Callers: `FM2_Render_UiOrScreenDrawListSubmit` (0x825B8A60, uploads the 2D ortho to c0
  then calls this) + the forward/opaque object-pass submit.
- **NEW (2026-06-26):** in the current black-menu state this hook is **NOT EVEN CALLED** —
  `FM2_PRESENT_DIAG` shows `DrawEmit` fires but `DirtyDraw`=0. So there is ALSO a second problem
  (13g): the render thread isn't reaching the forward pass in plume_native. BOTH must be fixed.

### 13c. GAP 2: `FM2_Render_WalkAndDispatchPm4DrawList` (PM4 interpreter) count-only
- `d3d_hooks.cpp:2736-2740` (RAW hook), guest 0x82722FD8. Only `CountPresentDiag("WalkDispatch")` +
  original. The recorded-PM4 draw-list REPLAY path (`DispatchPm4DrawOpcode` 0x82722808 jump table) is
  not interpreted into native draws. If the 3D scene replays through here, it's lost too.

### 13d. GAP 3: `FM2_D3D_EmitSurfaceResolvePackets` (EDRAM resolve) — PARTIAL workaround
- `d3d_hooks.cpp:2745-2776`. Does NOT translate the EDRAM→texture resolve. Instead reads the
  resolve-dest guest base from `ctx+10652` (RB_COPY_DEST_BASE) + color surf from `ctx+12160`, snapshots
  the plume surface (`SnapshotSurfaceForResolve`) and `RegisterSurfaceAperture(destBase, snap)` so a
  later sampling pass hits the rendered content instead of black resolve memory (`FM2_RESOLVE_ALIAS`).
  Works for the cases where the aperture address matches; a real resolve translation is absent.

### 13e. GAP 4: `render_patches.cpp` is a placeholder
- `render_patches.cpp:1` "FM2-specific render patches (placeholder; ReOdyssey LO patches are not
  applicable)." No FM2 patches implemented.

### 13f. GAP 5 / experiments: native renderer init + direct-debug-replay (OFF)
- `native_renderer/fm2_native_renderer.cpp:1819` logs "FM2 Plume initialization incomplete mode={}".
- `TryBuildAndSubmitDebugReplayForPm4Draw` (`d3d_hooks.cpp:2251`) + `nr::WantsDirectDebugReplay()` —
  the side-by-side "direct draw replay" experiment (uses `NativeStateSnapshot` from midasm hooks).
  OFF by default; separate from the main path. The shader-analysis/vfetch-decode infra it relies on
  (`fm2_shader_analysis.h`, `fm2_direct_draw_decode.h`) is the proper place to source the REAL vertex
  format/fetch constants (see band-aids below) but is not wired into the main render_state path.

### 13g. LOAD-BEARING BAND-AIDS (work, but fragile — do NOT remove without a replacement)
1. **passId → `device->vertexDeclaration` mirror** (`Fm2SetActivePassId` d3d_hooks.cpp:~800).
   passId is a texture/shader-state token (NOT a decl), but the mirror is the ONLY thing that hands
   `SyncVertexDeclarationFromDevice` a usable handle. Removing it → `ok=0`, all `missing_decl`, total
   black (proven + reverted 2026-06-26). The WHOLE decl path depends on it.
2. **`MatchDeclarationForShader`** (render_state.cpp:2253) is an UNRELIABLE primary source — returns
   null for ~every draw when used alone. Only works because the passId mirror covers most draws.
3. **Vertex FORMAT is heuristic** — matched a D3DVERTEXELEMENT9 by usage-set; real per-draw format
   lives in FM2's engine fetch constants (`BindVertexStream` writes them to ctx+0x670; see
   FM2-ida-renames-2026-06-26.md). Decoded type `0x2C259F`→FLOAT16_2 is *probably* fine but unverified
   against the engine source.
4. **VS constants read from `device+0x700`** — VERIFIED CORRECT (the render context IS the device;
   `UploadMatrixConstants` writes there). c0=1/640 is the real 2D screen ortho
   (`FM2_Render_BuildScreenOrthoAndComposeMatrices?` 0x825064A8 computes 2/W). The sprite collapse
   (c0=1/640 × normalized vertex) is a *separate*, secondary paradox; sprites are not the main content.
5. **Frame-sync gate pulse** (`StartGatePulseThreadOnce` d3d_hooks.cpp:434, cvar
   `fm2_plume_gate_pulse_hz`) — a host thread pulses the guest frame-sync gate so the game loop
   advances. A workaround for a sync the render thread waits on; likely related to 13g/13b-NEW (the
   render thread not reaching the forward pass).

### 13h. WHAT IS IMPLEMENTED (the working spine, for reference)
- `EmitIndexedDrawPm4*` → `DrawIndexedVertices`/`SubmitNativeIndexedDrawPm4` → render_state.cpp
  `DrawIndexedPrimitive`/`GetPipeline`/`FlushRenderState` → plume command list.
- State mirroring: `ApplyLiveColorWriteFromContext`, `ApplyLiveTexturesFromContext`,
  `SetVertexShaderNative`/`SetPixelShaderNative`/`SetStreamSourceNative`/`SetIndicesNative`/
  `SetRenderTargetNative`, `MirrorFm2RenderState`.
- Resource aliasing: `CreateVertexBufferAliased` + `RegisterBufferAlias`; surface aperture aliasing
  for resolves; `SetActivePassId` decl mirror.
- Present: `FM2PlumeTraceVdSwap` picks a present source (aperture → fbfetch → last-drawn RT) and
  `SetPresentSource` (cvar `fm2_plume_vdswap_present`).
- `GpuCommandBuffer_BuildAndSubmit` body SKIPPED in plume_native (correct: it kicks the disabled ring).

### 13i. CONCLUSION — the two-fold blocker
The black screen is the SUM of two gaps, both on the forward-color path:
1. **`FM2_D3D_EmitDirtyStateAndDrawList` is not translated** (GAP 1) — even if it ran, its draws
   wouldn't reach plume.
2. **It isn't running** in the current state (13b-NEW) — the render thread only executes the minimal
   `EmitIndexedDrawPm4*` (sprite/blit) pass, never the forward pass — while the game logic/audio/input
   advance normally (the game is NOT stuck; the render thread is gated into the minimal loop).
NEXT (per plan): wire path-logging into `UiOrScreenDrawListSubmit` / the object-pass submit /
`WalkAndDispatchPm4DrawList` and trace in IDA the condition that diverts the render thread away from
the forward pass in plume_native; then implement the GAP 1 translation.

---

## 14. PATH-LOGGING RESULT + REAL ROOT CAUSE (2026-06-26, session 6N)

Added present-diag counters (`d3d_hooks.cpp`, REX_HOOK_RAW) on the whole forward/object/scene pass
chain. Ran plume_native at the black menu. **Result — the ENTIRE scene/object/forward chain is
DORMANT:**

```
FIRED:    RenderWorker, GateThread, GpuCmdBuf, DrawEmit, DrawSubmit
ABSENT:   ExecSorted, SubmitSorted, ObjPassTrav, PrepWalkObj, UiScreenSubmit, WalkDispatch, DirtyDraw
```

So the render thread never enters `ExecuteSortedDrawLists` or anything below it. There are TWO render
systems and only the minimal one runs:

- **SCENE render (DORMANT)** = `FM2_Render_FramePipeline` (0x82518DC0, invoked via vtable — only data
  xrefs) + `FM2_Render_ViewTraversal` (0x8250D950 ← sub_82519CF0) + `FM2_Render_SceneSliceEntry`
  (0x82509148) + `FM2_Render_SubmitPassWrapper` (0x825181A8) → `ExecuteSortedDrawLists` (0x8252FF00) →
  object passes → `EmitDirtyStateAndDrawList`. **None of this executes in plume_native.**
- **MINIMAL render (RUNS)** = the render thread's `RunFrame` (0x82288948) calls
  `FM2_RenderFrame_CompositeBlitOrClearPath?` (0x8227FF20, was "AudioRenderFrame_PathA") which does a
  1280x720 blit-one-tex (the `292FF294` blit) OR a black clear (0xFF000000); and
  `FM2_RenderFrame_PostProcessPath?` (0x82287400, was "AudioRenderFrame_PathB") drives the
  post-process passes (`downsample16x`/`gaussian_blur`/`blit_one_tex`/`final_combine_post_effects` →
  `render_pass_execute_draw_batch_with_state_save` 0x825AF3C0 → `EmitIndexedDrawPm4*`) — the source of
  the sprite `11460356` draws.

### 14a. MAJOR MISNAMING (heuristic-mislabeled render cluster)
The ENTIRE `FM2_AudioRenderFrame_*` / `audio_render_*` / `FM2_AudioMix_*` family is the **RENDER frame
/ composite / post-process system**, NOT audio. e.g. `FM2_AudioMix_WriteOutputSamplePair(...,1280,720,
...)` writes a screen-size render quad; `audio_render_setup_*_pass` are post-process passes. Renamed
the two central paths (0x8227FF20, 0x82287400, `?`); the rest of the cluster needs a dedicated pass.

### 14b. ROOT CAUSE (unifies menu-black + race-load hang)
In plume_native the render thread is stuck running ONLY the minimal composite+post-process render
(blit a black/empty texture). It **never advances to the SCENE render** (`FramePipeline`/object
passes). So there's no scene content; post-process operates on black; the blit shows black. This is a
**wait/loading-state hang** — the game logic/audio/input advance (you can "play" it), but the render
pipeline never leaves the minimal "composite/wait" path. Same root as "stuck before race loads."
The earlier `EmitDirtyStateAndDrawList` translation gap (§13 GAP 1) is real but SECONDARY — that path
isn't even reached. **The primary blocker is upstream: the scene render (`FM2_Render_FramePipeline`
0x82518DC0) is never invoked in plume_native.**

### 14c. Scene-entry chain ALSO dormant — scene render never TRIGGERED (extended path-log)
Added counters for `FramePipe`/`SubmitPass`/`SceneSlice`/`ViewTraverse`: **all absent.** `ViewTraversal`
(0x8250D950) is DIRECT-called by `sub_82519CF0` (0x82519CF0, the scene-frame orchestrator) yet never
fires — so the scene render is **never invoked at all**, not gated low. The whole FramePipeline pipeline
(`FramePipeline` 0x82518DC0, `sub_82519CF0`, ViewTraversal, …) is **vtable/table-dispatched** (entries in
`0x8218fXXX` / `0x82044XXX` / `0x82045XXX`), and that pipeline simply isn't run.

- `sub_82519CF0` (scene orchestrator) calls `ViewTraversal` only if `(*(scene+4035)||*(scene+4084)) &&
  sub_82517010(a1)` (sub_82517010 = passCount<=2 OR flags at scene+4293/4294/+924). It also calls
  `FM2_GpuKick_NotifyPixCapture_StoreFenceAndDrain` (0x82373160) repeatedly = store fence (ctx+10780 ->
  +10800) + `D3D_SubmitAndDrainCommands`.
- **`D3D_SubmitAndDrainCommands` (0x82373078)** = submit pending PM4 (`D3D_SubmitCommandBuffer`) +
  `D3D_FlushCommandQueue` + **`D3D_CommandWaitForCompletion(ctx, *(ctx+10780)-2, 0)`** (CPU waits for the
  GPU fence). The wait is gated by `dword_829F0258` (likely "GPU/ring enabled" — probably 0 in
  plume_native, so the wait is SKIPPED -> not the hang). So the GPU drain is NOT what blocks; the scene
  pipeline is simply never dispatched.

### 14d. ROOT (refined): the SCENE-RENDER PIPELINE is never dispatched in plume_native
The render thread (`RunFrame`) runs only the minimal composite (PathA blit/clear) + post-process (PathB).
The scene-render pipeline (`FM2_Render_FramePipeline` family, table-dispatched) is **never run**, so no
scene PM4 is built -> RT black. The decision of WHICH to run is upstream (the producer / main-thread
render-frame build that enqueues the scene-pipeline command, OR the render-pipeline object that should be
created/dispatched). The game stays in the minimal/wait/loading render state in plume_native; without it
the scene pipeline runs. **NEXT:** trace what creates/dispatches the FramePipeline pipeline object (the
vtable owner) + the wait-screen state (`RunFrame` `a1+1824`, `FM2_WaitScreen_FlushPendingEntries`
0x822884C8) to find the condition that keeps plume_native in the minimal render and never starts the
scene pipeline. The path-counters (ExecSorted/…/FramePipe etc.) are in the tree (capped, present-diag).

---

## 15. UNIFIED ROOT CAUSE (2026-06-26, session 6N) — stuck on the WAIT/LOADING screen

Both leads converge. Confirmed by decompiling the two functions:

- **`FM2_Render_FramePipeline` (0x82518DC0) = the FULL SCENE RENDER** — applies pass/camera state,
  `FM2_Render_CompileMissingPassBuffers`, then submits the object passes via `FM2_Render_SubmitPassWrapper`
  (depth/opaque/transparent + `FramePipelineDrawObjects`/`SubmitPassA/B`). Entire body is `if (v4){...}`
  (v4 = the scene/pass-state object). It is **vtable-dispatched and NEVER invoked in plume_native**
  (path-counters FramePipe/SubmitPass/ViewTraverse/… all 0).
- **`FM2_WaitScreen_FlushPendingEntries` (0x822884C8) = the WAIT/LOADING screen builder** — instantiates
  `CWaitTexture` / `CWaitMovie` / `CWaitModel` and enqueues them into the **movie renderer** (renderThread
  `+0x2524`) via `FM2_MovieRenderer_EnqueuePlaylistEntry`. `RunFrame`'s composite (`PathA`
  0x8227FF20) is gated on `+0x2524` valid — so **the screen shows the wait/movie renderer output, not the
  scene.** The minimal path that DOES run (PathA composite + PathB post-process → sprite 11460356 + blit
  292FF294) is the WAIT-screen render.

**ROOT CAUSE:** in plume_native the game is **stuck rendering the WAIT/LOADING screen** and never
transitions to `FM2_Render_FramePipeline` (the scene render). So no scene PM4 is built → RT black → blit
shows black. Game logic/audio/input still advance (the wait screen is interactive / the loop runs), which
is why it "plays" but is blank. **This UNIFIES the menu-black and the "stuck before race loads" hang —
both are the same: the render pipeline never leaves the wait/loading state in plume_native.** Without
plume_native the wait state clears and `FramePipeline` runs.

Everything below `FramePipeline` (§13 GAP 1 `EmitDirtyStateAndDrawList` translation, the decl band-aids,
the sprite c0 paradox) is downstream and SECONDARY — none of it is reached.

### 15a. NEXT (the actual fix target)
Find what **dismisses the wait screen / completes loading** and transitions to `FramePipeline`, and why
that condition is met WITHOUT plume_native but not WITH it. Leads:
- The wait/movie renderer (renderThread `+0x2524`) and its completion (a `CWaitMovie` finishing, or the
  pending-entry/loading state at `+0x1824`/`+0x2466`/`+0x2467`). What clears it?
- Almost certainly a GPU-completion / present-complete / resource-ready signal that the DISABLED guest
  ring would normally raise. plume_native disables the ring (`GpuCommandBuffer_BuildAndSubmit` skipped;
  `D3D_SubmitAndDrainCommands` GPU wait gated by `dword_829F0258`). The game's loading likely polls a GPU
  fence / VdSwap-complete that never advances in plume_native. Relate to the gate-pulse workaround
  (`fm2_plume_gate_pulse_hz`) which already pulses ONE frame-sync gate but evidently not this one.
- Check whether `D3D_SubmitAndDrainCommands` / the swap/fence (`a1+10780`/`+10800`, VdSwap path) signal
  the completion the loading state machine waits on; if the plume present/VdSwap hook doesn't advance that
  fence, the wait never clears.

## 16. PRODUCER-GUARD BYPASS IS CORRECT; LOG CONFIRMS NO CONTENT PATH RUNS (session 6O)

Two results this session that refine §15 and define the next decisive test.

### 16a. The GPU-fence bypass (`Fm2ProducerProgressGuard` returns 0) is NOT the bug — it is correct
`D3D_CommandCheckCompletion` (0x82369340, our `Fm2ProducerProgressGuard` hook) is the GPU completed-fence
poll: reads `D3D_GetFrameCounter` (= `device.gpuState[+256]+332`, the GPU completed-frame counter) and
returns `(submitted-completed) < 0x1388`. Its blocking-wait callers loop **while it returns non-zero**:
- `FM2_D3D_WaitForPrimaryOverrun` (0x82371DF8): `while(1){ if(!CheckCompletion(v)){destroy; return;} ... }`
- `FM2_AudioPump_WaitForBlockerCompletion` (0x82381850): `while(CheckCompletion(v6) && **(v2+16)!=v3);`

So our bypass `return 0` makes these waits **exit immediately** (don't block on the dead ring) — the
intended behaviour. The bypass is correct, NOT the gate. Producer is not GPU-blocked.

`D3D_GetFrameCounter`'s other readers (`resource_manager_async_load_by_hash` 0x82432310,
`FM2_IOSys_EnqueueFileOperation`) use the counter only to pick a **load priority** (`a4 = 393215`) and then
call `resource_handle_trigger_load_and_resolve` **regardless** — they do NOT hard-block on the counter. So
a frozen frame-counter does not by itself stall loading. The "loading waits on the GPU fence" sub-theory is
**ruled out** as a hard gate.

### 16b. LOG BASELINE (plume_native, fm2-clean.log 2026-06-26 15:47): render thread presents an EMPTY RT
Per-second `FM2_PRESENT_DIAG` tallies in the current plume_native log:
- `RenderWorker` = **144/sec**, `with_source`=144, src=`0x130C7F000` → presents the RT every frame.
- `GateThread` ~520/sec (frame-sync gate pulse), `GpuCmdBuf` sparse (1–29).
- **ZERO** for every content path: `FramePipe`, `SubmitPass`, `ViewTraverse`, `SceneSlice`, `ExecSorted`,
  `SubmitSorted`, `ObjPassTrav`, `UiScreenSubmit`, **AND** `WalkDispatch`, `DrawEmit`, `DrawSubmit`,
  `DirtyDraw`.

So neither the **scene chain** NOR the **PM4 draw-list walker** runs. The render thread spins presents of
the RT while **nothing ever draws into it**. (Sharper than "stuck on wait screen": no content-generation
path executes at all. The 2-sprite RTCENSUS seen earlier was a different/earlier run state.)

### 16c. DECISIVE EXPERIMENT (zero code change): Xenos-mode baseline of the SAME counters
`CountPresentDiag`→`LogReplayDbg` is **not** mode-gated and the `REX_HOOK_RAW` hooks install in **both**
modes (`__imp__` thunk replacements, independent of `--fm2_plume_mode`). So the same counters fire in Xenos
mode. Comparing Xenos vs plume_native for the SAME screen resolves the central fork:
- If `FramePipe`/`WalkDispatch`/`DrawEmit` are **>0 in Xenos** → that path renders the screen and
  plume_native suppresses it → bisect which hook/bypass suppresses it (gate pulse, prod-guard, GpuCmdBuf
  body-skip, Irq skip, `dword_829F0258`/`dword_829C24C0`).
- If they are **0 in Xenos too** → the menu is NOT scene/PM4-rendered; it is the minimal PathA/PathB
  composite and the bug is in that translation (back to the sprite-collapse / decl band-aid line).

Procedure: clear/rename `C:\temp\fm2-clean.log`; run FM2 **without** `--fm2_plume_mode plume_native` to the
same screen; then summarise the per-hook counters. No rebuild required.

## 17. RESOLVED FORK + CALL-CHAIN CLIMB (session 6O) — scene producer is silent in plume_native

Decisive Xenos vs plume_native comparison (same screen, same RAW-hook counters, both modes):

| hook | Xenos /sec | plume_native /sec |
|---|---|---|
| `FramePipe` (scene render) | **360–410** | **0** |
| `SubmitPass` | 1805–2050 | 0 |
| `ObjPassTrav` | 1441–1482 | 0 |
| `SubmitSorted` | 330–366 | 0 |

**Fork RESOLVED:** `FM2_Render_FramePipeline` IS the on-screen render; plume_native suppresses it. Not a
shader-collapse, not a wait-screen problem. `FramePipe`'s counter is **0, not 1** — never entered once.

### 17a. The climb (each level via guest `ctx.lr` capture in the RAW hook, Xenos run)
1. **`FramePipe` caller** = `sub_825E8BF8` (`0x825E8F38`/`0x825E8FB8`), which calls FramePipe via
   `(*(this+20))->vtable[+44]()`, 4×/frame across 2 pass objects. Whole body gated by `if(*(this+2379))`.
   Probe in plume_native: **`sub_825E8BF8` is never reached** (`FM2_SCENEVIEW_DISP` absent). In Xenos it
   runs with `enable2379=1` — so **the +2379 gate is a red herring** (set in both / not the issue).
2. **`sub_825E8BF8` caller** = `0x8245D13C` inside **`sub_8245D048`** — a **deferred-callback QUEUE drain**
   (`for(i=*v4; i; i=*(i+20)) (*(i))(i[1], i[2])`). The scene render is one registered node
   (`node[0]=sub_825E8BF8`, `node[4]=this=sceneView 0x4193xxxx`). `sub_8245D048` is called by
   `FM2_RenderThread_RunFrame` (`sub_82288948`) only when `v11(=*(queue+120) submitted) > *(rt+2100)
   processed` && `sub_8245D448()`.
3. **plume_native probe of `sub_8245D048`**: it **runs heavily** (~288/sec, draining 7–110-node queues on
   tids 82056 & 58272) — but **`sceneNode=0` in every queue**. So the consumer/frame-gate is FINE; the
   **scene-render callback is never enqueued**. (RenderWorker fires 144/sec and presents the RT regardless.)

### 17b. ROOT (refined): a PRODUCER/SUBMISSION failure, not consumer/GPU
The deferred-callback queue is a **producer/consumer with a circular buffer**: `sub_8245D448` pops a frame's
batch from the input ring (`queue+32`) into the consume list (`queue+56`); `sub_8245D048` drains+dispatches;
`sub_8245D740` recycles via `FM2_CircularBuffer_PushFrontNode`. The render queue lives at
**`renderThreadObj+2224`**. In plume_native the consumer drains fine but the **scene producer never pushes
the scene-render command** into it. (`sub_8245CED8`/`ReleaseChildCallback` — which appends to a *different*
list at `queue+52` — is NOT the scene's enqueue; filtering `fn==0x825E8BF8` there never fired, even in
Xenos. The scene node is a persistent/cross-thread push into the `+2224` input ring.)

So: in plume_native the **scene-render producer thread is silent** — it never enqueues the per-frame scene
command. Everything downstream (drain → `sub_825E8BF8` → `FramePipe` → passes) is reached only when the
command is enqueued. The producer-guard bypass, the GPU-fence waits, and the `+2379` enable flag are all
ruled out. Most likely the **scene-producer thread is parked on a frame-sync/present signal the disabled
ring never raises** (same family as the `dword_829C24C0` gate we already pulse — but a DIFFERENT event/
thread). Reconnects to §15a.

### 17c. NEXT
Find the producer that pushes `(fn=sub_825E8BF8, this=sceneView)` into `renderThreadObj+2224`, and why its
thread is silent in plume_native. Cleanest: attach x64dbg to a **plume_native** run on the black screen and
inspect thread states — locate the parked render-producer thread (the one that in Xenos runs on the
scene tid and pushes to `+2224`) and read what it waits on. Alternatively a HW read-bp on the vtable slot
holding `sub_825E8BF8` (`0x8210c74c` / `0x821950e8`) during a Xenos run reveals the producer call site.
Diagnostic hooks added this session (all in `d3d_hooks.cpp`, log to `fm2-clean.log`, mode-independent):
`FM2_FRAMEPIPE_CALLER`, `FM2_SCENEVIEW_DISP` (sub_825E8BF8), `FM2_CBQUEUE_DISP` (sub_8245D048),
`FM2_SCENE_ENQUEUE` (sub_8245CED8 — wrong queue, can be removed).

## 18. x64dbg: PRODUCER PINPOINTED — scene-view entry never SUBMITTED in plume_native (session 6P)

Used x64dbg (HW read-bp on the scene-render vtable slot, guest `0x8210c74c`/`0x821950e8`, both hold
fn `0x825E8BF8`; host = membase `0x100000000` + guest). **Xenos run:** the bp fired instantly; call
stack (render thread "XThread1CA20"): `XThread::Execute → sub_82756CE0 → sub_82289640
(RenderThread_Main) → sub_82288948 (RunFrame) → sub_822884C8 (FM2_WaitScreen_FlushPendingEntries) →
… → sub_82279610` (a CParams execute that loads scene `vtable[11]`, `bswap`s it, builds the render
command). So the scene render command is **built on the render thread inside `sub_822884C8`**, which
RunFrame calls when `*(renderThread+1824)` is set.

**Plume_native run** (relaunched under x64dbg, broke at RunFrame entry): render-thread obj
`a1 = ctx.r3 = 0x4001C170` (queue `a1+2224 = 0x4001CA20` ✓). Two HW bps:
- WRITE-bp on `+1824` (host `0x14001C890`) **fired, value=1**, written by a **GAME thread**
  ("XThread4124": `sub_82214CF0 → sub_8220EAC8 → sub_822892C0`) that queues a render entry. So
  **`+1824` toggles normally in plume** — game threads DO queue entries and the render thread DOES
  flush them. The gate is NOT the bug (an earlier single read of `0` was just a cleared moment).
- READ-bp on the scene vtable slot **never fired in plume_native**, even after the game reached the
  front-end (audio/UI/music loaded). So `sub_822884C8` flushes OTHER entries but **never builds the
  scene-view command** — the scene-view entry is never queued.

### 18a. REFINED ROOT CAUSE
The `+1824`/flush/queue/drain/dispatch machinery all WORK in plume_native. What's missing is the
**game-thread submission of the scene-view render entry** — the 3D scene render request is simply
never made in plume_native (other render entries are). This is a **game-logic-level divergence**, not
a render-pipeline bug. So ALL the prior render-pipeline theories (decl/constants/collapse, +1824
gate, GPU fences, producer-guard, present race) are downstream/irrelevant — the scene is never even
requested.

### 18b. SUBMITTER CHAIN + THE GATE (x64dbg Xenos, bp on the renderer-CParams ctor)
Broke on `sub_82279630` (`CCarRendererDeferred::CParams1ICarRendererRenderCarReflection` ctor) in
Xenos → submitter (game thread "XThread15FE8"):
```
sub_822172E8 (MAIN GAME LOOP) → sub_825DCE08 → sub_82609AB8 (render-pass schedule loop)
  → FM2_TriggerMatchingListEntryActions (0x8234d5a8) → sub_82279630 (build car-render CParams)
```
Render entries are typed CParams (`CCarRendererDeferred`/`COverlayRendererDeferred`/…) queued onto
the render thread via `sub_822892C0(rt, entry)` = `sub_82288460(rt+16, entry)` + set `rt+1824=1`.

**THE GATE** — `FM2_TriggerMatchingListEntryActions` walks the render-pass list and fires each entry's
render-action vfunc ONLY if ALL of:
```
*(DWORD*)(entry+36)==a2  &&  *(BYTE*)(entry+202)  &&  *(float*)(entry+180)!=0.0
  &&  (*(BYTE*)(entry+184) || *(BYTE*)(entry+185))
```
The car/scene render is one entry's action. **In plume_native that entry fails one of these** — most
likely `+180` (pass weight/fade-alpha, set by `sub_82609AB8`'s `FM2_Render_SetPassTimingScalar`/level
calls) == 0, or the `+202`/`+184/185` enables. So the render-action never fires → command never built
→ drain empty → `FramePipe` never dispatched → black.

### 18c. FIX REGION + NEXT
The bug is in the **render-pass activation / transition-weight system**: the car/scene pass entry is
never activated (weight 0 / disabled) in plume_native, even though the game reaches the front-end.
NEXT: read the car-render pass entry's `+180/+202/+184/+185` in plume vs Xenos to pin the exact field,
then trace what sets it (`sub_82609AB8` pass-weight/transition, or a higher game-state flag) that
plume_native leaves incomplete. x64dbg facts: recompiled guest fns resolve by **bare** name
(`sub_82288948`, not `fm2.`); at entry `RCX=&PPCContext`, `ctx.r3=[RCX+0]`; render-thread obj fixed at
guest `0x4001C170` (queue +2224, wait-flag +1824); bps persist in the `.dd64` db across relaunch.

### 17d. DEFINITIVE: scene is NOT enqueued via the deferred-callback API (`sub_8245CED8`)
`sub_8245CED8` = `FM2_DeferredTaskParams_ReleaseChildCallback(queue, fn=a2, this=a3, arg2=a4, flag=a5)`:
either immediate-executes (`if(*(queue+144)) a2(a3,a4)`) or allocs an 0x18 node (`node[0]=a2 …`) and appends
to the list at **`queue+52`**. Probe with fire-counter + fn-range capture: `SchedCb` fires **354,752/sec**
(hook definitely installed) but **ZERO** enqueues with `fn` in `[0x825E0000,0x825F0000)` — so the scene
render is **never scheduled through this API**. Reason: the drain `sub_8245D048` processes `queue+56`
(filled by `sub_8245D448` from the **input ring `queue+32`**), which is a DIFFERENT list from the `queue+52`
that `ReleaseChildCallback` appends to. The render-thread command system uses C++ command objects
**`CRenderThreadLink::CParams1IRenderThread*`** (`BeginRenderInternal` 0x82276940, `EndRenderModeInternal`
0x82276B30, …) enqueued via `GetField4`+`AllocPoolBumpAllocate`+`ReleaseChild`. But the scene node is a
**raw-fn** node (`node[0]=sub_825E8BF8`, called directly by the drain, caller `0x8245D13C`, no thunk), NOT a
CParams object — so it reaches the input ring `queue+32` via a per-frame **render-command submission** path
that is layered/inlined (no single clean hookable enqueue). Top-down is also nested vtable dispatch
(`FM2_SceneCamera_CallVfunc12/20` → `(*(*(*(scene+2392)+8)+12))()`).

**STATUS:** diagnosis is solid (scene producer silent in plume_native; consumer/queue/GPU all fine). The
exact producer function is elusive via static+hook climbing due to the engine's layered render-command
architecture. Cleanest remaining options: (1) x64dbg HW write-bp on the scene queue node `[0]` (catch the
writer = producer) or thread inspection of the parked producer on a plume_native black-screen run; (2)
test the main-loop branch hypothesis: in `sub_822172E8` the render path is `if(*(a1+8)) (*(*(a1+8)+40))(dt)`
else the loading branch (`FM2_Render_NotifyManagerStateChange`) — count a loading-branch marker plume vs
Xenos to see if the main loop is stuck pre-render (`*(a1+8)==0`).
