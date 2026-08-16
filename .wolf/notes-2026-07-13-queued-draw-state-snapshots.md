# 2026-07-13 — queued draw-state snapshots

## Original XEX mapping (IDA37)

- `D3DDevice_SetVertexShaderConstantFN` @ `0x8236DA48` copies into `device+0x700` and ORs VS float dirty groups into `device+0x3B10` (`dirtyFlags[0]`).
- `D3DDevice_SetPixelShaderConstantFN` @ `0x8236DB08` copies into `device+0x1700` and ORs PS float dirty groups into `device+0x3B18` (`dirtyFlags[1]`).
- `sub_8236DB68` / `sub_8236DBC8` update packed VS/PS booleans and set the boolean dirty bit represented in host order as `1ull << 56` in `dirtyFlags[4]`.
- `D3DDevice_SetSamplerState_ParameterCheck` @ `0x82370408`, `D3DDevice_SetSamplerState_MipFilter` @ `0x82370628`, and `D3DDevice_SetSamplerState_AddressU` @ `0x82370A48` update the 24-byte sampler record at `device+0x400+sampler*24` and set sampler bits 31..16 in `dirtyFlags[3]`.
- `sub_82382CC8` is the Xenos pending-ALU flush consumer, not a durable render-thread snapshot. The native path must consume these dirty files before asynchronously queueing a draw.

## Native implementation

- Added POD render commands for VS constants, PS constants, booleans, and individual samplers.
- `QueueDrawStateSnapshots` copies guest data through the intermediary allocator before the guest thread can mutate it.
- Float dirty bits are converted from MSB-first 64-byte groups into one bounded copy range (64 VS groups, 56 PS groups).
- First draw on a device is seeded in full. Recorded-object replay forces a full state snapshot because original `EmitDirty` has already consumed its dirty masks.
- Snapshot commands and the consuming draw are atomically appended with `RenderQueue::EnqueueBulk`, preventing cross-thread interleaving.
- `FlushRenderState` now uploads only persistent render-thread constant files. It no longer dereferences live guest constant files or merges heuristic pass/scene/WVP overlays.

## Verification completed

- RelWithDebInfo `fm2` target builds successfully.
- `fm2_render_snapshot_test` covers MSB-first range calculation, PS clamping, and command payloads.
- Full FM2 CTest suite: 2/2 passed.
- Direct launch confirmed the debuggee was `fm2.exe` (PID 118576, correct FM2 window title). It remained responsive/running for roughly 115 seconds and produced no Windows Application Error/WER event, then exited with process code `0xB00`; this exit still needs classification during an interactive run.

## Next RenderDoc capture checklist

1. Capture a later garage/showroom or track frame containing real POSITION mesh draws and texture-bound PS draws, not only the 13 TEXCOORD-only UI draws from `fm2mmgrok11`.
2. On consecutive draws, inspect root CBV b0/b1: values must match each draw's guest snapshot, change where expected, and remain stable after later guest work is queued.
3. Re-check the former grok11 UI shader: transformed vertices should no longer collapse to `x≈y≈z`; verify non-degenerate NDC triangles and `SamplesPassed > 0`.
4. For a textured draw, verify SharedConstants b2 has non-null `texture*Indices` and valid `samplerIndices`, and that descriptor-set slots resolve to the expected SRV/sampler.
5. Include at least one recorded object-pass replay draw. Confirm its VS/PS constants, booleans, and all 16 sampler snapshots belong to that restored draw rather than the preceding direct draw.
6. Keep the process running beyond the previous ~115-second exit point and record the x64dbg stack/exit reason if `0xB00` repeats.
