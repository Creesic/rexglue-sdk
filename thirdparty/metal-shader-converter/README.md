# Metal Shader Converter Integration

This directory contains integration for Apple's Metal Shader Converter, which is used to convert DXIL bytecode to Metal shaders for Xbox 360 game shader translation in [Xenia](https://github.com/xenia-project/xenia). The dynamic library and headers are copied from the Apple installer so we can build out of the box on macOS.

## Installation


1. Install Metal Shader Converter from Apple Developer:
   https://developer.apple.com/metal/shader-converter/
2. The installer places the library and headers under `/usr/local/lib` and `/usr/local/include/metal_irconverter*`.
3. To refresh the copies in this repo:
   ```
   cp /usr/local/lib/libmetalirconverter.dylib third_party/metal-shader-converter/lib/
   rsync -a /usr/local/include/metal_irconverter* third_party/metal-shader-converter/include/
   ```
4. Run `xb premake` to regenerate build files.

## Requirements

- macOS 13+ (Ventura)
- Xcode 15+
- Apple Silicon or Intel Mac with Metal support
- Metal Shader Converter 2.0+

## Usage

The Metal Shader Converter is used exclusively for Xbox 360 game shader translation:
- Xbox 360 Microcode → DxbcShaderTranslator → DXIL → Metal Shader Converter → Metal Library

For Xenia's built-in shaders, use the existing XeSL → MSL pipeline via `xb buildshaders`.

### Licensing note
Apple’s EULA for Metal Shader Converter allows distributing the dynamic libraries **solely for shader conversion** (see Section 2.B). The library remains subject to Apple’s terms and must be used on Apple-branded hardware. Keep the bundled license/acknowledgements alongside the copied files.
