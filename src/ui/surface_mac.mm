/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Grien Gupta, 2026 - macOS CAMetalLayer surface for ReXGlue
 *
 * CAMetalLayerSurface is the macOS implementation of rex::ui::Surface.
 *
 * On macOS, Vulkan presentation is handled by MoltenVK which translates
 * Vulkan calls into Metal.  The WSI extension used is VK_EXT_metal_surface
 * (#217), which wraps a CAMetalLayer* rather than an HWND or XCB window.
 *
 * The CAMetalLayer is created by RexMetalView (see window_mac.mm) as the
 * NSView's backing layer.  This class simply wraps a non-owning pointer to
 * that layer and reports drawable size for the presenter's swapchain logic.
 *
 * Drawable-size vs bounds-size:
 *   - NSView.bounds are in logical (DIP) pixels.
 *   - CAMetalLayer.drawableSize is in physical (backing) pixels.
 *   - MacWindow::HandleWindowDidResize() keeps drawableSize in sync with the
 *     backing pixel dimensions after every resize event.
 *   - GetSizeImpl() reads drawableSize, so the presenter always sees physical
 *     pixel counts and never needs to apply a DPI scale factor itself.
 */

#import <QuartzCore/CAMetalLayer.h>

#include <rex/ui/surface_mac.h>

namespace rex {
namespace ui {

// Returns the current drawable size in physical backing pixels.
// Returns false (and zeros) if the layer is null or zero-area, which tells
// the presenter not to attempt presenting to this surface yet — the same
// contract as the XCB and Win32 implementations.
bool CAMetalLayerSurface::GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const {
  if (!layer_) {
    width_out = 0;
    height_out = 0;
    return false;
  }

  // drawableSize is set explicitly by MacWindow::HandleWindowDidResize() and
  // MacWindow::CreateSurfaceImpl() so it matches the backing pixel dimensions.
  CGSize size = reinterpret_cast<CAMetalLayer*>(layer_).drawableSize;
  if (size.width <= 0.0 || size.height <= 0.0) {
    width_out = 0;
    height_out = 0;
    return false;
  }

  width_out  = static_cast<uint32_t>(size.width);
  height_out = static_cast<uint32_t>(size.height);
  return true;
}

}  // namespace ui
}  // namespace rex
