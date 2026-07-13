# Resolve-aperture present (2026-07-12)

## Cause of black blit
Present was blitting sticky full-frame color RTs. FM2 composites via D3DDevice_Resolve into XDK frontbuffer textures (not FM2R). Those resolves were previously skipped (IsFm2Resource-only).

## Fix
- Register resolve dest page (XDK header +32) -> host texture
- Swap reads frontbuffer fetch (arg4+28+4), LookupResolveSurfaceAperture, SetFrontbufferPresentSource
- ExecuteCommandList prefers aperture over sticky RT
- TranslateGuestTexture(dest, upload=false) for non-FM2 resolve destinations
- Do NOT SetFrontbufferPresentSource from Resolve (stole composite / stalled boot); Swap owns it

## Verify (fm2_118)
presentKind=aperture fbBase=0x096A7000 1280x720; Swap 601+; device lost 0.
