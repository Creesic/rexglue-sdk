# Reclone Notes

This repository intentionally does not track title asset payloads, generated
recompiler output, build trees, logs, IDE state, or compiled executables.

After a fresh clone:

1. Initialize submodules with `git submodule update --init --recursive`.
2. Restore local title assets into `FM2/assets/` or `FH1/assets/` as needed.
3. Regenerate title output from the manifests/configs before building title
   targets.

The Mac/Metal build dependencies vendored under `thirdparty/` are tracked when
the SDK build references them directly.
