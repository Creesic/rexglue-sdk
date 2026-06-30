# DOAX Rendering Notes

## Pool Float / Near-Player Black Assets

Observed in the pool scene with `C:\Users\Tera\Documents\GitHub\renderdoccaps\doaxgood.rdc`:

- The black float draw samples `Texture 3608`, a 512x512 `k_8` texture backed by guest memory at `0x1BD7C000`.
- The texture is loaded from shared memory after a `Resolve Copy Full 8bpp` pass.
- The resolve source is a `k_8_8_8_8` render target at EDRAM base `0x0`; the source alpha channel contains the mask data while RGB is mostly zero.
- The old `resolve_full_8bpp` shader loaded only red or blue for R8 output, so the resolved texture became black.

The fix is to make the full 8bpp resolve write source alpha into the R8 destination. If the generated shader bytecode is refreshed from Xenia sources, apply the equivalent source change in `resolve_full_8bpp.xesli`: load eight RGBA pixels, build the two `float4` R8 pack vectors from `.a`, and regenerate both D3D12 and Vulkan `resolve_full_8bpp*` bytecode headers.
