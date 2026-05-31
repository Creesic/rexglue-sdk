# FM2 Rendering Notes

## Overexposed UI Fixed By Scaling TEXCOORD4

Date found: 2026-05-19

FM2 reached gameplay, but the UI became heavily overexposed after the Turn10 and Microsoft intro videos. The Forza intro video, press-start screen, and later 2D UI were blown out. The 3D menu scene and gameplay rendering were not blown out.

RenderDoc evidence:

- In `mainmenu-both.rdc`, the overexposed draw was visible around EID `12475`.
- The pipeline name was `VS 47EF573F72E1163F, PS 0A9AB4E93B7E98B5`.
- The relevant pixel shader received `TEXCOORD4` as `(8, 8, 8, 1)`.
- The pixel shader copied that interpolator into `r4` and multiplied normal UI texture samples by it, pushing values above 1.0 and causing UNORM render target clipping.

The working SDK-side fix is to scale the effective pixel shader register after interpolation:

```cpp
if (REXCVAR_GET(dxbc_fm2_scale_overbright_texcoord4) &&
    current_shader().ucode_data_hash() == UINT64_C(0x0A9AB4E93B7E98B5) &&
    register_count() > 4 && param_gen_interpolator != 4) {
  a_.OpMul(uses_register_dynamic_addressing ? dxbc::Dest::X(0, 4, 0b0111)
                                            : dxbc::Dest::R(4, 0b0111),
           uses_register_dynamic_addressing ? dxbc::Src::X(0, 4, dxbc::Src::kXYZW)
                                            : dxbc::Src::R(4),
           dxbc::Src::LF(0.125f));
}
```

This lives in the DXBC translator, so it survives FM2 generated-code rebuilds. The translator version was also bumped to force shader/pipeline storage invalidation.

Files touched in the SDK:

- `include/rex/graphics/flags.h`
- `include/rex/graphics/pipeline/shader/dxbc_translator.h`
- `src/graphics/pipeline/shader/dxbc_translator.cpp`

Runtime config used for FM2:

```toml
d3d12_ignore_8bit_color_exp_bias = false
d3d12_invert_8bit_color_exp_bias = false
dxbc_fm2_scale_overbright_texcoord4 = true
```

Important RenderDoc gotcha: `TEXCOORD4` will still show `(8, 8, 8, 1)` as the PS input. That is expected because the fix does not alter the vertex shader output or the interpolator itself. It changes the copied pixel shader register, so verification should look for an early `mul r4.xyz, r4.xyz, 0.125` in the pixel shader disassembly, or simply confirm the UI is no longer blown out.

Build gotcha: after changing the SDK, rebuild and install the SDK, then relink FM2. In this session, the patch appeared not to work until `fm2.exe` was relinked against the patched installed SDK.

