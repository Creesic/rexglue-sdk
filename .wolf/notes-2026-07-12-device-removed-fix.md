# 2026-07-12 — DEVICE_REMOVED root cause (fm2_105/106 → fixed in 107)

## Evidence
- `GetDeviceRemovedReason: 0x887A0001` = `DXGI_ERROR_INVALID_CALL`
- D3D12 InfoQueue (forced debug layer):
  1. RT/DS used before Discard/Clear/Copy (CREATE_NOT_ZEROED + clearRect)
  2. `ResolveSubresourceRegion` 1x→1x (illegal; need MSAA src ≥2)
  3. `ExecuteCommandLists` on an **unclosed** command list
  4. `RemoveDevice` INVALID_CALL

## Fixes
- `ConvertFormat` / Translate: `0x2D20014A` / gpu fmt 10 → `R8G8_UNORM`
- StretchRect / Resolve 1x path: `copyTextureRegion`, not resolve
- `ProcWaitForGpu`: close+submit open CL before dummy begin/end; use slot 0
- `EnsureAttachmentInitialized`: `discardTexture` before first clear/draw

## Result (`fm2_107.log`)
- `GPU device lost: 0`, stderr clean, Translates continue (tiled fmt 55/61)
- New gap: after ~300 presents, `still no render target` / no blit
  (black may persist for SetRT/present-source tracking — separate from device death)
