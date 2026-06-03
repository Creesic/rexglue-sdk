#pragma once
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

#include <rex/ui/surface.h>

#ifdef __OBJC__
@class CAMetalLayer;
#else
typedef struct objc_object CAMetalLayer;
#endif

namespace rex::ui {

class CAMetalLayerSurface final : public Surface {
 public:
  explicit CAMetalLayerSurface(CAMetalLayer* layer) : layer_(layer) {}

  TypeIndex GetType() const override { return kTypeIndex_CAMetalLayer; }

  CAMetalLayer* layer() const { return layer_; }

 protected:
  bool GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const override;

 private:
  CAMetalLayer* layer_;
};

}  // namespace rex::ui
