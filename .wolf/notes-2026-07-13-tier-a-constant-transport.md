# Tier A constant transport and object replay — 2026-07-13

- Added flush-time color-write, implicit-depth, and single-sample PSO guards.
- Bound declaration-derived shader specialization constants and shared swizzle metadata.
- Added guest sampler-state conversion, caching, descriptor binding, and per-draw sampler indices.
- Added pass/object/scene VS constant snapshots plus live VS/PS register-file sourcing and curated
  merge order in `FlushRenderState`.
- Added minimal command-buffer batch draw capture keyed by clone, EmitDirty replay, traversal
  constant overlays, SetPending flush snapshots, and guest-context scribble restoration.
- Recorded draws now use their real pixel shaders; no PM4 scanner, tiling state machine,
  diagnostic file logging, shader probe, or constant-mode cycler was ported.
- Verified with the FM2 RelWithDebInfo `fm2` target build.

## 2026-07-13 hotfix — startup crash

`fm2_313.log` died mid-startup right after first PSO / StretchRect drain.
Suspect: object-pass record/replay scribbling guest context (`RestoreGuestRange`
+ sync `DrawIndexedVertices` from `EmitDirty`).

Mitigation: `kObjPassRecordReplay = false` — Begin/Finalize/CreateClone are
passthrough; EmitDirty still takes the scene3d WVP overlay only. Restored
placeholder PS for `IsInsideRecordedBatch`. Kept flush guards, samplers, decl
SPEC, UploadMatrix mirror, live float files, SetPending snapshots.
