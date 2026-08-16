# 2026-07-13 — fm2mmgrok6 still black after viewport fix

Capture: `C:\Users\Tera\Documents\GitHub\renderdoccaps\fm2mmgrok6.rdc`

## Confirmed fixed from grok5
- Viewport now `minDepth=0, maxDepth=1` (EID 64).

## Still broken
- Scene RT 325 stays 100% black; draws `SamplesPassed=0` with depth test **off**.
- VS debug (EID 64, v0): inputs `TEXCOORD0/1 = (0,0,0,1)` (defaults); `SV_Position = (8.77, 0, 0, 0)` — **w=0** → full clip.
- Bound VB stride **8**; input layout float3@0 + float3@12 + float2@24 (32B) — mismatch.
- VB bytes at draw offset look like float16_4 (`… 00 3c = 1.0 half`).

## Root cause
`MatchDeclarationForShader` could pick an exact-count 32B FLOAT3 layout that covers the shader header even when stream-0 elements overflow the bound stride. Shader TEXCOORDs then bind to slot-15 dummies → zero inputs → w=0.

## Fix
Reject decls whose stream-0 `offset+DeclTypeByteSize` exceeds `vertexStrides[0]`; prefer packedEnd == stride.

Rebuild: `FM2/out/build/win-amd64-relwithdebinfo` fm2.exe (render_state.cpp).
Next: new RDC — expect non-zero SamplesPassed and non-black RT after draws.

## grok8 (2026-07-13)
Capture after Match-first + stride gate.

### Progress
- Draw 68 (VS `{407ccac3}`): input layout is now **float16×2 @0/@4** (8B) — stride match works; TEXCOORD inputs are real (`0.814, -0.103` / `1, 0`).
- Viewport still `0..1`.

### Still black
- Draw 68: `SV_Position.w = 0` still (VS transform `_122` from c0/c1/c3; `vteFlags=8` skips `1/w`). SamplesPassed=0 via clip.
- Draw 199 (VS `{7e9ad136}`): **still 32B float3 layout** on stride-8 VB; POSITION comes through as UINT bits; `SV_Position≈(0,0,0,0.99)` but **depth Greater + z=0** → SamplesPassed=0.
- RT 325 / swapchain still 100% black.

### Next
1. Why some draws still bind 32B decls with stride 8 (stride 0 at match?).
2. Reverse-Z depth clear vs Greater with z=0.
3. VS W=0 on `{407ccac3}` (constants / VTE).


