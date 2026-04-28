#pragma once
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay & Rien Gupta, 2026 - macOS CAMetalLayer surface for ReXGlue
 *
 * CAMetalLayerSurface — the macOS implementation of rex::ui::Surface.
 *
 * Vulkan on macOS is provided by MoltenVK, which translates Vulkan API calls
 * into Metal.  The corresponding WSI extension is VK_EXT_metal_surface (#217);
 * it accepts a CAMetalLayer* via VkMetalSurfaceCreateInfoEXT::pLayer.
 *
 * This class wraps a non-owning pointer to the CAMetalLayer that lives as the
 * backing layer of MacWindow's RexMetalView.  The presenter (vulkan_presenter)
 * calls GetType() to detect the surface type and layer() to retrieve the
 * pointer when creating the VkSurfaceKHR.
 *
 * CAMetalLayer typedef note
 * ─────────────────────────
 * CAMetalLayer is an Objective-C class.  To keep this header usable from both
 * C++ and Objective-C++ translation units without pulling in all of AppKit, we
 * forward-declare it using the same idiom Vulkan's own vulkan_metal.h uses:
 *   - In ObjC/ObjC++ (@class) the type is properly declared.
 *   - In plain C++ (typedef void) it becomes an opaque void* alias, which is
 *     compatible with the pLayer field in VkMetalSurfaceCreateInfoEXT.
 */

#include <rex/ui/surface.h>

// Forward-declare CAMetalLayer safely in both C++ and ObjC++ translation units.
// Matches the pattern used by <vulkan/vulkan_metal.h> so the pointer is
// assignment-compatible with VkMetalSurfaceCreateInfoEXT::pLayer in all TUs.
#ifdef __OBJC__
@class CAMetalLayer;
#else
typedef struct objc_object CAMetalLayer;
#endif

namespace rex {
namespace ui {

// Wraps a CAMetalLayer* as a rex::ui::Surface for Vulkan presentation.
// The layer is owned by RexMetalView (MacWindow's NSView backing layer) —
// this class holds only a non-owning pointer and must not release it.
class CAMetalLayerSurface final : public Surface {
 public:
  explicit CAMetalLayerSurface(CAMetalLayer* layer) : layer_(layer) {}

  TypeIndex GetType() const override { return kTypeIndex_CAMetalLayer; }

  // Returns the wrapped CAMetalLayer*.  In C++ TUs the static type is void*;
  // cast via reinterpret_cast<CAMetalLayer*> inside .mm files when needed.
  // When passing to VkMetalSurfaceCreateInfoEXT::pLayer, the types are
  // compatible without a cast because both use the same void typedef.
  CAMetalLayer* layer() const { return layer_; }

 protected:
  // Returns drawable size in physical backing pixels (set by
  // MacWindow::HandleWindowDidResize and MacWindow::CreateSurfaceImpl).
  bool GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const override;

 private:
  // Non-owning. Lifetime guaranteed by MacWindow — the surface is destroyed
  // before the MacWindow (and therefore the view + layer) is closed.
  CAMetalLayer* layer_;
};

}  // namespace ui
}  // namespace rex
