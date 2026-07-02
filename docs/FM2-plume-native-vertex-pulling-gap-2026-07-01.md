# FM2 plume_native black screen: traced to a vertex-shader-translation gap (2026-07-01)

> **CORRECTION (same day, later session — read before trusting the conclusion
> below):** the "translator gap" conclusion this doc originally reached was
> **retracted** after reading XenosRecomp's actual source. The evidence trail
> (sections below) remains valid and correct; only the final attribution was
> wrong. See the "Correction" section at the end for the revised state: the
> shader translation is faithful, and the real open question is **why the
> shared compressed-vertex buffer is never populated with real data under
> plume_native** — with a stale/bypassed vertex-stream-binding hook as the
> live (unconfirmed) suspect.

Same-day follow-up to `docs/FM2-ida-renames-2026-07-01.md`. After wiring up the
`D3DDevice_Resolve` hook (confirmed working — see that doc), the screen was
still black. This doc traces that specific remaining bug via RenderDoc,
comparing a plume_native capture against a same-scene Xenia (real Xenos GPU
emulation) capture, down to a likely root cause: **plume's Xenos-vertex-shader
translator emits fixed-function IA-style semantic inputs (e.g. `TEXCOORD0`)
for shaders whose original VFETCH addressing doesn't reduce to a simple IA
binding, silently losing the real fetch/addressing logic for compressed or
shared-pool ("streamed") geometry.** *(Retracted — see correction at end.)*

## Captures used

- plume_native (broken): `C:\Users\Tera\Documents\GitHub\renderdoccaps\fm2_2026.07.01_20.57.26_capture.rdc`
  (D3D12, 90 events, 58 draws)
- Xenia (working, same scene): `C:\Users\Tera\Documents\GitHub\renderdoccaps\xenia_edge_2026.07.01_21.23.41_frame1630.rdc`
  (D3D12, 12984 events, 955 draws, 8 dispatches — Xenia captures at raw PM4
  granularity, so draw counts aren't directly comparable 1:1 with plume's
  merged draw list)

## Evidence trail (plume_native capture)

1. **`get_frame_overview`**: 58 real draws (index counts up to 1728), but the
   only render target the overview flags as written is the depth target
   (`2D Depth Target 325`, 55 writes). No color target shows any writes at
   all, despite draws clearly binding color render targets.
2. Picked a representative big draw (event 317, 1512 indices).
   **`get_pipeline_state`**: real color target bound (`2D Render Target 323`),
   real pixel shader bound (`Shader {bf5ebeee}`), real vertex shader
   (`Shader {99bda386}`), viewport/scissor full-screen (1280x720). Nothing
   obviously wrong at the binding level. One anomaly flagged: `depth_test_disabled`.
3. **`analyze_texture`** on RT 323 at a late event (629): **100% near-black**
   — `min/max/mean_channels` all `[0,0,0,0]` across all 921,600 pixels.
   `get_debug_messages`: zero D3D12 validation errors/warnings.
4. **`pixel_history`** at screen-center (640,360) on RT 323: **zero entries**.
   Not a shader/blend/discard problem — no fragment ever reaches that pixel
   at all across the whole draw sequence.
5. **`decode_mesh_inputs`** on event 317: the bound vertex stream exposes only
   `TEXCOORD0`/`TEXCOORD1` (both `Float16x2`, 8-byte total stride) — **no
   `POSITION` attribute**, and the previewed values are `(0.0, nan)`. Every
   other vertex-buffer slot is `Null`.
6. **`read_buffer`** on the actual vertex buffer (`Buffer 362`) at two very
   different offsets (the buffer's true start, and a location ~522KB in,
   corresponding to a completely different vertex index in the same draw's
   index range): **both read the identical uniform repeating pattern
   `00 00 00 FF`**, not real geometry. This exactly matches a warning already
   written into `BindPm4GeometryFromContext`'s own diagnostic comment
   ("Raw guest source bytes ... If this is uniform fill (e.g. FF000000) the
   geometry is not actually there") — confirmed happening, live.
7. **`get_shader`** (VS `99bda386`, disassembly): this is **not** a broken or
   placeholder shader. It's a legitimate, well-formed dequantization shader:
   reads `TEXCOORD0.xy`/`TEXCOORD1.xy`, does a per-vertex axis-swap based on a
   flag bit from a constant buffer, reconstructs a signed value via
   `abs()`/sign-comparison, then runs that through what is structurally a
   scale+bias matrix multiply (multiple `cbuffer0.Load4` calls at matrix-row
   offsets) to produce `SV_Position`. This is a classic
   "quantized/compressed position, expanded via a per-draw bounding-box
   transform" pattern — a real, sane technique, not garbage codegen. This
   *rules out* "wrong declaration/shader mismatch" as the cause: the 8-byte
   Float16x2×2 binding is exactly what this shader is built to consume.
   Also checked: this shader hash is **not** present in
   `FM2/out/build/.../missed_shaders/` (the fallback-dump directory for
   shaders plume's cache/lookup couldn't find), so it went through normal
   successful translation — not a translation-failure artifact.

**Conclusion from the plume capture alone**: the shader and its declared
input binding are correct and self-consistent; the actual bytes at the guest
physical address `BindPm4GeometryFromContext` resolves for this vertex stream
are not real geometry (a uniform fill pattern instead). Something that's
supposed to populate that memory with real compressed-vertex data either
never runs, or we're reading the wrong address, under plume_native.

## Comparison against ReOdyssey (why this class of bug can't happen there)

ReOdyssey's `SetStreamSource`/`SetIndices` hooks only ever operate on a
`GuestBuffer*` — a native buffer created via `CreateVertexBuffer` and
populated through the standard `Lock`/`Unlock` hook pair (the CPU writes real
data into it before the GPU ever reads it). There is **no**
`TranslatePhysical`/raw-guest-memory-aliasing anywhere in ReOdyssey's render
code. FM2's `BindPm4GeometryFromContext`, by contrast, must call
`mem->TranslatePhysical<const void*>(physBase)` and alias guest RAM directly
at draw time, because FM2's compiler-inlined PM4 draws don't go through
`CreateVertexBuffer`/`Lock`/`Unlock` at all (established in
`docs/FM2-ida-renames-2026-07-01.md`: the XDK draw primitives are inlined
directly into PM4-emission call sites). This is inherently more fragile: it's
only correct if whatever's supposed to write real data to that physical
address has actually done so by the time the draw reads it.

## The decisive comparison: Xenia's working capture

Picked a representative draw in the Xenia capture and pulled its pipeline
state (`get_pipeline_state`). The difference from plume is fundamental, not
incidental:

```
"vertex_inputs": { "vertex_buffers": [], "attributes": [] }   <- EMPTY

"bindings_by_stage.vertex.readonly": [
  { "resource_id": "Buffer 343", "byte_offset": 0, "byte_size": 536870912 }
]
```

**536,870,912 bytes = exactly 512MB — the entire Xbox 360 address space.**
Xenia binds the *whole emulated guest RAM* as one giant read-only SRV buffer
directly to the vertex shader stage, and **the vertex shader itself computes
addresses and pulls vertex data via explicit buffer loads** — because that's
literally what Xenos VFETCH is: an instruction *inside* vertex shader
microcode that computes an address and loads from memory. There is no
hardware fixed-function input-assembler stage on Xenos the way there is on
D3D12; Xenia's shader translator (in `src/graphics/pipeline/shader/`)
faithfully preserves that as raw SRV loads in the translated DXIL/SPIR-V.

Plume's shader for the broken content family has **no SRV bound to the
vertex stage at all** (confirmed via `get_shader_reflection`:
`readonly_bindings: []`) — it only has plain `TEXCOORD0`/`TEXCOORD1` IA
inputs. That means whatever translated this VS chose to convert its original
VFETCH instructions into **fixed-function IA vertex-attribute reads** rather
than preserving them as shader-side raw buffer loads.

## Why this explains the specific symptom (and not others)

Converting VFETCH → IA binding is a valid simplification *when the fetch
reduces to something IA hardware can express directly* ("read vertex N from
stream S at a fixed stride") — and that's exactly the class of content that
already works in plume_native today (UI, depth prepass, simple meshes).
It silently breaks for vertex fetches that need real *address computation*
inside the shader — which is exactly this content: a huge (219,787-vertex,
1.75MB) **shared vertex pool** reused across many draws, fed through
compressed/quantized position math with a per-draw dequantization matrix.
Whatever indexing/addressing scheme the original Xenos shader used to pull
the *correct* compressed vertex for each draw out of that shared pool gets
thrown away when the translator collapses it to "bind stream 0 at offset X,
stride 8" — so `BindPm4GeometryFromContext` ends up pointed at a physical
address that was never populated the way the simplified binding assumes.

## Bottom line / next step

This is not a small runtime-hook bug fixable in `d3d_hooks.cpp` or
`render_state.cpp` — those are working exactly as designed, faithfully
reading whatever fetch-constant address the (mis-simplified) shader
translation implies. The gap is upstream, in whatever component decides how
to translate a given Xenos vertex shader's VFETCH instructions for
plume_native — it needs a path (mirroring what Xenia's translator already
does) to preserve raw shader-side buffer addressing for shaders that need it,
instead of unconditionally emitting IA-style semantic inputs.

**Not yet done**: locating that translator component and confirming exactly
where/how it decides IA-vs-raw-buffer binding (likely somewhere under
`src/graphics/pipeline/shader/`, shared with or adapted from the same
codebase Xenia-style translation uses — `dxbc_translator.cpp`,
`spirv_translator.cpp`, and friends — needs an FM2/plume_native-specific
entry point audited next).

## CORRECTION (same day, later session): translator-gap theory retracted

Followed up on the "Bottom line" above by going to fix the translator — and
found the theory doesn't survive contact with the actual code.

**What disproved it:** FM2's shader cache is not built by the SDK's runtime
translator (`src/graphics/pipeline/shader/`) at all — per
`docs/native-renderer-architecture-comparison.md` Phase 2, it's built offline
by **XenosRecomp** (vendored at `ReOdyssey/thirdparty/XenosRecomp/`, driven by
`scripts/fm2/Update-FM2ShaderCache.ps1`). Reading
`XenosRecomp/shader_recompiler.cpp` directly:
`ShaderRecompiler::recompile(const VertexFetchInstruction&)` (~line 208)
resolves each VFETCH to an HLSL input via a `vertexElements` map that is
populated (~lines 1844-1892) from **`vertexShader->vertexElementsAndInterpolators`
— declaration metadata embedded in the game's own compiled shader object.**
The `TEXCOORD0`/`TEXCOORD1` Float16x2 IA inputs are exactly what FM2 itself
declared for this shader. XenosRecomp is faithfully reproducing the game's
declaration, not guessing or simplifying. There is no lossy IA-collapse to fix.

**Why the Xenia comparison misled:** Xenia's whole-512MB-SRV vertex pulling is
its *universal* mechanism for every draw, simple or complex — its presence on
this draw doesn't prove this shader needs computed addressing. That was
over-read.

**Additional evidence gathered:** two more `read_buffer` probes on the plume
capture's vertex buffer (offsets 870848 and 1900000) show the identical
`00 00 00 FF` fill — now 4/4 sampled locations across ~1.9MB are uniform.
Too broad and too uniform for a partially-caught-up streaming system; the
buffer is wholesale unpopulated at the location the runtime reads (or the
runtime is reading the wrong location entirely).

**Revised open question:** why is the shared compressed-vertex pool never
populated with real data under plume_native? Live hypotheses, in rough order:

1. **Stale/bypassed stream binding (the "incorrect d3d hooks again" suspicion)**:
   this content may bind its vertex pool once per pass (or via a path our
   hooks never see) rather than per draw, so `BindPm4GeometryFromContext`'s
   read of `ctx+0x2F94` at draw time picks up a stale resource from an
   unrelated earlier bind. IDA note: `FM2_RenderContext_BindVertexStream`
   (0x82370E48) *is* called from the generic PM4 dispatch machinery
   (`FM2_Render_DispatchPm4DrawOpcode`, `FM2_Render_WalkAndDispatchPm4DrawList`),
   so some PM4 draws do bind through it — that neither confirms nor rules out
   a bypass/once-per-pass pattern for this specific pool.
2. **The populating writer never runs in plume_native** (a CPU streaming job or
   GPU-side write whose trigger is dead in this mode).
3. **Wrong physical-address decode** for this particular fetch constant.

**Instrumentation added (compiled into the current build):** `FM2_BINDSTREAM`
logging in `Fm2BindVertexStream` (`d3d_hooks.cpp`) — logs
ctx/slot/resource/decoded physBase/offset/stride for the first 200 calls, to
cross-correlate against `FM2_PM4GEO_BIG`'s physBase for the same scene.
**Status: awaiting a race/track-scene repro** — the one test run only reached
the menu (`FM2_PM4GEO_BIG` count = 0), so no correlating data exists yet. Next
session: get into a race with this build, then diff the two log channels; if
the big draws' physBase never appears in any `FM2_BINDSTREAM` line, hypothesis
1 is confirmed.
