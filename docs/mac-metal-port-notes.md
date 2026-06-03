# macOS Metal Port Notes

## 2026-06-02 build baseline

- `thirdparty/metal-shader-converter/lib/libmetalirconverter.dylib` and
  `thirdparty/dxilconv/lib/libdxilconv.dylib` must be exposed to consumers with
  explicit link directories and Darwin `rpath` entries. On this tree, relying
  on transitive `-lmetalirconverter` / `-ldxilconv` resolution was not enough
  for the final `rexruntime` link.
- `src/core/CMakeLists.txt` cannot treat all `UNIX` platforms like Linux.
  macOS needs `Threads::Threads` plus `${CMAKE_DL_LIBS}`, but must not link
  `rt`.
- The local non-Windows DXC shim currently empties `__declspec`, so
  `DxbcConverter.h`'s `CLSID_DxbcConverter` definition loses its select-any
  behavior. Keep `src/graphics/metal/iunknown_id_stub.cpp` on `dxcapi.h` only;
  including `DxbcConverter.h` there causes duplicate-symbol link failures on
  macOS.
- A basic smoke check for this baseline is:

```sh
cmake --build out/build/mac-arm64 --target rexruntime rexglue -j8
./out/mac-arm64/Debug/rexglue --help
```
