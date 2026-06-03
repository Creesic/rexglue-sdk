/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#import <QuartzCore/CAMetalLayer.h>

#include <rex/ui/surface_mac.h>

namespace rex::ui {

bool CAMetalLayerSurface::GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const {
  if (!layer_) {
    width_out = 0;
    height_out = 0;
    return false;
  }

  CGSize size = reinterpret_cast<CAMetalLayer*>(layer_).drawableSize;
  if (size.width <= 0.0 || size.height <= 0.0) {
    width_out = 0;
    height_out = 0;
    return false;
  }

  width_out = static_cast<uint32_t>(size.width);
  height_out = static_cast<uint32_t>(size.height);
  return true;
}

}  // namespace rex::ui
