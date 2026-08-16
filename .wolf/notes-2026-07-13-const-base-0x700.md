# 2026-07-13 — VS/PS constant base + FLOAT16_4 decl tie-break

## Changes (plume)

1. **FlushRenderState** uploads float constants from `device+0x700` (VS) and
   `device+0x1700` (PS), not `GuestDevice::vertexShaderFloatConstants` (+0x780).
   FM2’s live ALU file matches 080plume / SetPending_AluConstants; the struct
   field is 0x80 past the real file and was skipping transform regs → HUD VS
   `SV_Position.w=0` (fm2mmgrok8 draw 68).

2. **MatchDeclarationForShader** boosts `FLOAT16_4` (+5000) / `FLOAT16_2`
   (+2500) POSITION when scoring stride-8 ties so FLOAT2 does not win and feed
   `R32G32_UINT` into a float16 unpack (fm2mmgrok8 draw 199).

3. **guest_device.h** documents the +0x780 vs +0x700 mismatch; static_asserts
   for float-constant field offsets.

## Verify (next capture: fm2mmgrok9)

- Binary: `FM2/out/build/win-amd64-relwithdebinfo/fm2.exe` (built RelWithDebInfo).
- Draw ~68 / VS `{407ccac3}`: CBV c0–c4 should look like real transforms;
  `SV_Position.w` ≠ 0.
- Draw ~199 / VS `{7e9ad136}`: POSITION debug as full float16_4 UINT unpack;
  `SamplesPassed` > 0 if depth path is OK.

## Still open

- Reverse-Z depth (`GREATER` + z≈0) if SamplesPassed still 0 after correct POSITION.
