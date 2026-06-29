# FM2 (Forza Motorsport 2) PM4 draw-list opcode map

Reverse-engineered 2026-06-24 from `FM2.xex.i64` (IDA) cross-referenced with the
Xbox 360 PM4 reference in `D:\Emulation\Xbox360techdocs\xbox-360` (`pm4/`,
`engines/forza-turn10.md`). This is the spec for translating Forza's rendering to
a native (Plume) renderer in `--fm2_plume_mode plume_native`.

## Why this matters

2026-06-29 correction: do **not** read this as "Forza has no Direct3D
library code." IDA shows the XDK D3D/XG code is present in the XEX: resource
creation and setup paths call functions such as `D3DDevice_CreateVertexBuffer`,
`D3DDevice_CreateTexture`, `D3DDevice_CreateSurface`, and
`D3DDevice_CreateVertexDeclaration`, and the binary contains D3D command-buffer,
PIX, D3DX, and XGraphics strings. The import table has no named `D3DDevice_*`
imports because this code is linked into the title image; the kernel-facing
graphics imports are the lower-level `Vd*` functions.

The important FM2-specific point is narrower: the hot draw/replay path does not
look like ReOdyssey or UnleashedRecomp's public immediate-mode hook surface with
dozens of `D3DDevice_Set*` / `D3DDevice_Draw*` calls. Forza builds its **own
structured draw-command list** and walks it with a private interpreter. The
opcode handlers update a D3D-style render context and then call internal D3D
command-buffer helpers that emit Xenos **PM4 packets**. Present goes through
`FM2_GpuCommandBuffer_BuildAndSubmit` and low-level `VdSwap` /
`PM4_XE_SWAP (0x64)`, not a clean `D3DDevice_Swap` hook. So a native renderer
for Forza must translate at this draw-list / D3D-internal PM4 layer, not at an
absent high-level `D3DDevice_DrawIndexedPrimitive` call layer.

## The interpreter

- `FM2_Render_WalkAndDispatchPm4DrawList` (0x82722FD8) — walks a linked list of
  command nodes: node size = `u16 @ node+0`, opcode = `u8 @ node+2`. Advances
  `node += *node`. Calls `FM2_Render_ApplyRenderStateCallbackBlock` per node.
- `FM2_Render_DispatchPm4DrawOpcode` (0x82722808) — jump-table dispatch:
  `idx = opcode - 1; if (idx <= 0x39) jump table[idx]`. 58 handlers, targets
  relative to `0x82722860`; offset table is `word_8211DF60` (58 × big-endian u16).
- Global render context pointer (the `r3`/"device" all handlers use):
  `dword_82A41BEC`. Pass-state dedup cache: `unk_82A41D38`.

## Opcode → handler (opcode = `*(node+2)`, raw value)

### Draws (the geometry payload)
| Opcode(s) | Handler |
|-----------|---------|
| `0x01, 0x30, 0x31, 0x38, 0x39, 0x3A` | `FM2_RenderContext_BindIndexBuffer` + `FM2_D3D_EmitIndexedDrawPm4PacketsWithGpuOffset` (indexed draw; addressing variants) |
| `0x06, 0x07, 0x08` | `FM2_Render_ObjectPassPrefetchDrawBatch` |
| `0x0E, 0x0F` | indirect `bctrl` (vtable +0x1D0) batched draw + writes pass-state cache |
| `0x20, 0x0C, 0x0D` | indirect `bctrl` callback (vtable +0x40 / dynamic) |

### Shader / vertex-stream / texture / sampler state
| Opcode(s) | Handler |
|-----------|---------|
| `0x02` | `FM2_RenderContext_SetPixelShaderState` |
| `0x2A` | set pixel shader = NULL |
| `0x09` | `SetActivePassId` + `SetVertexShaderState` |
| `0x0A, 0x2E, 0x36` | `FM2_RenderContext_BindVertexStream` (loops) [+ pass id + VS] |
| `0x2F` | `FM2_Render_SetupPassShaderAndVertexStreams` |
| `0x37` | `FM2_Render_BindPassVertexStreamsWithConstants` |
| `0x2C, 0x35` | `FM2_Render_GetTextureResourceGpuOffset` + `FM2_D3D_ApplyGpuMemoryPatches` (texture bind; magenta fallback via `LoadDefaultLookupTextureMagentaPtr`) |
| `0x2D` | `FM2_Render_ApplyPassSamplerBindings` |
| `0x32` | `FM2_Render_ApplyPassShaderConstantsAndTextureBindings` |
| `0x33` | `FM2_RenderContext_SetActivePassId` |

### Matrix / shader constants
| Opcode(s) | Handler |
|-----------|---------|
| `0x03, 0x04, 0x05` | `FM2_RenderContext_UploadMatrixConstants` |
| `0x0B` | `FM2_RenderContext_UploadMatrixConstantsFromPass` |
| `0x12` | `FM2_RenderTls_BatchSubmitDrawPackets` — **NOT a GPU submit**; batches per-instance transform matrices (`MultiplyMatrix4x4AccumVMX`) into the worker context |
| `0x13` | `FM2_Render_UploadBoneMatrices` |
| `0x14` | `FM2_Render_ApplyShaderConstantOffset` |
| `0x21` | `FM2_Render_ApplyPassMatrixWithBoneUpload` |
| `0x25` | `FM2_Render_UploadContextBoneMatricesAndPass` |
| `0x26` | `FM2_Render_PopShaderConstantSnapshot` + `FM2_Render_UploadPassMatrices` |
| `0x34` | `FM2_Render_ApplyIndexedPassMatrixUpload` |

### Constant-stack snapshots
`0x23` = push (`FM2_Render_PushShaderConstantSnapshot`); `0x24`, `0x2B` = pop.

### Control flow (skip/advance/stop; gated on `FM2_Render_TestPassStateBit`)
`0x15, 0x16, 0x17, 0x18, 0x19, 0x1B, 0x1C, 0x27, 0x28, 0x29` (conditional
skip/advance to next node); `0x1A, 0x1D` = stop list.

### Unhandled / no-op
`0x10, 0x11, 0x1E, 0x1F, 0x22` → default case (advance only).

## Key implication

The draw/state opcode handlers call **exactly the `FM2_` functions the native
renderer already hooks** (`BindVertexStream`, `BindIndexBuffer`,
`SetPixelShaderState`, `SetVertexShaderState`, `EmitIndexedDrawPm4PacketsWithGpuOffset`,
`ApplyGpuMemoryPatches`). So when `WalkAndDispatchPm4DrawList` runs, the existing
hooks fire and geometry *should* translate to Plume. Remaining unknowns / gaps:

1. Whether `WalkAndDispatchPm4DrawList` actually runs for the **3D scene** (the
   `FM2_FrameLoop_*` thread we observed drives the loading/wait screen, no 3D).
2. The indirect `bctrl` draws (`0x0E/0x0F/0x20/0x0C/0x0D`) are not captured by the
   named-function hooks — they dispatch through guest vtables.
3. EDRAM **resolve** translation (Forza is EDRAM-tiled; docs: "skipping resolve →
   black") and the `VdSwap`/`PM4_XE_SWAP` present hand-off (`FM2_GpuCommandBuffer_BuildAndSubmit`,
   0x8236CB28, driven by the FMOD pump + `FM2_AudioSubmitBridge_Process`).

See also: `memory/project_plume_native_black_screen.md`.
