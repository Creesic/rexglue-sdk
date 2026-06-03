# macOS codegen notes

## Standalone XEX loader

On macOS, `rexglue codegen` now bypasses `Runtime` setup inside
`ProjectRecompiler` and parses XEX files directly into `BinaryView`.

Why:
- the normal tool-mode runtime path still aborts inside guest-memory setup on
  Apple Silicon before `ProjectRecompiler` can recover
- FM2 regeneration needs a reliable non-interactive path from manifest to
  generated C++

Current tradeoff:
- the standalone loader now creates a local `ExportResolver` and registers the
  static `xboxkrnl` and `xam` export tables before Register phase runs
- import thunks in ranges such as `0x8294E2A8` to `0x8294EE08` now resolve to
  named imports like `__imp__RtlInitializeCriticalSectionAndSpinCount` and
  `__imp__RtlImageXexHeaderField` instead of `sub_<addr>` placeholders
- this still is not equivalent to full runtime-backed import resolution because
  standalone codegen does not construct kernel state or runtime variable
  mappings; it only resolves static function exports
- if a future title depends on import variables rather than function exports,
  standalone codegen may still need additional resolver plumbing

Current scope:
- entrypoint and manifest-listed DLL XEX files can be loaded from disk
- XEX decryption, decompression, PE section extraction, import thunk discovery,
  and execution metadata reporting all happen without guest-memory allocation
