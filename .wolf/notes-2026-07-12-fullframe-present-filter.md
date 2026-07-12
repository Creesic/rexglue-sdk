# Full-frame present filter (2026-07-12)

## Follow-up to sticky present-source
Sticky RT fixed null-at-Swap, but Present then stuck on FM2's 1280x256 EDRAM tile binds (stretched to swapchain).

## Fix
`IsFramebufferSizedPresentSource`: reject width==viewportW && height<viewportH; require >= viewport WxH.
Only sticky-update and Present-select those.

## Verify (fm2_111.log)
Present source stays 1280x720 through call 1501 (was 1280x256 after ~300). Device lost 0.

## Next
Resolve-aperture / frontbuffer Swap fetch + tile ResizeTileSurface from ReXGlue080plume if content still wrong.
