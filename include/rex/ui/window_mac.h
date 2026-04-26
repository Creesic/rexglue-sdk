#pragma once
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay & Rien Gupta, 2026 - macOS Cocoa/Metal window for ReXGlue
 *
 * MacWindow — Cocoa NSWindow + CAMetalLayer implementation of rex::ui::Window.
 *
 * Architecture
 * ────────────
 * MacWindow owns three Objective-C objects (held as raw pointers, with ARC
 * disabled — the NSWindow and delegate are retained by the ObjC runtime once
 * set up; we release them in the destructor or in HandleWindowShouldClose):
 *
 *   ns_window_   — the top-level NSWindow
 *   metal_view_  — RexMetalView (NSView subclass with a CAMetalLayer backing
 *                  layer), fills the entire content area.  All input events
 *                  originate here.
 *   delegate_    — RexWindowDelegate (NSWindowDelegate) forwards lifecycle
 *                  callbacks (close / resize / focus) back to MacWindow.
 *
 * Protected method access
 * ───────────────────────
 * The ObjC delegate and view classes cannot call Window's protected On* /
 * WindowDestructionReceiver members directly (they don't inherit from Window).
 * Instead, they call the public Handle* methods defined here, which are thin
 * wrappers that invoke the protected base-class methods.  This is the same
 * pattern Win32Window uses with its WndProc thunk.
 *
 * Surface creation
 * ────────────────
 * CreateSurfaceImpl() returns a CAMetalLayerSurface wrapping metal_view_'s
 * CAMetalLayer.  The presenter (VulkanPresenter) uses this to create a
 * VkSurfaceKHR via vkCreateMetalSurfaceEXT (MoltenVK).
 *
 * DPI / Retina
 * ────────────
 * GetMediumDpi() returns 96 (Windows convention used throughout the codebase).
 * GetLatestDpiImpl() returns 96 × backingScaleFactor, so a 2× Retina display
 * reports 192 DPI.  Physical pixel dimensions are obtained via
 * -[NSView convertSizeToBacking:] and stored in OnActualSizeUpdate.
 *
 * MenuItem stub
 * ─────────────
 * MacMenuItem is a minimal stub that satisfies MenuItem::Create's platform
 * requirement.  Native NSMenu integration can be added later without changing
 * the Window or Surface interfaces.
 */

#include <memory>
#include <string>

#include <rex/ui/menu_item.h>
#include <rex/ui/virtual_key.h>
#include <rex/ui/window.h>

// Forward-declare ObjC types so this header remains plain C++.
// The implementations are Objective-C++ objects defined in window_mac.mm.
#ifdef __OBJC__
@class NSWindow;
@class RexMetalView;
@class RexWindowDelegate;
#else
struct objc_object;
typedef struct objc_object NSWindow;
typedef struct objc_object RexMetalView;
typedef struct objc_object RexWindowDelegate;
#endif

namespace rex {
namespace ui {

class MacWindow final : public Window {
  using super = Window;

 public:
  MacWindow(WindowedAppContext& app_context, const std::string_view title,
            uint32_t desired_logical_width, uint32_t desired_logical_height);
  ~MacWindow() override;

  // Returns the underlying NSWindow* as void*.  Valid after Open() succeeds.
  void* GetNativeWindowHandle() const override;

  // ── DPI ────────────────────────────────────────────────────────────────────
  // Medium DPI is 96 (the Windows baseline used by the rest of the codebase).
  uint32_t GetMediumDpi() const override { return 96; }
  // Returns 96 × NSWindow.backingScaleFactor (192 on 2× Retina).
  uint32_t GetLatestDpiImpl() const override;

  // ── Lifecycle callbacks (called by RexWindowDelegate) ─────────────────────
  // These are the only public entry points for ObjC → C++ event flow.
  // Internally they call the protected On* methods of rex::ui::Window.
  void HandleWindowShouldClose();   // user clicked the close button
  void HandleWindowDidResize();     // window was resized; syncs drawableSize
  void HandleWindowDidBecomeKey();  // window gained focus
  void HandleWindowDidResignKey();  // window lost focus

  // ── Input callbacks (called by RexMetalView) ──────────────────────────────
  // Coordinates are already converted to view-local logical pixels with the
  // Y axis flipped to top-left origin.
  void HandleMouseButton(int32_t x, int32_t y, MouseEvent::Button button, bool is_down);
  void HandleMouseMove(int32_t x, int32_t y);
  // scroll_x / scroll_y are in MouseEvent::kScrollPerDetent units.
  void HandleMouseScroll(int32_t x, int32_t y, int32_t scroll_x, int32_t scroll_y);
  // key_char is the Unicode code point (0 if not printable); fires OnKeyChar.
  void HandleKeyDown(VirtualKey vk, uint32_t key_char,
                     bool shift, bool ctrl, bool alt, bool super_key);
  void HandleKeyUp(VirtualKey vk, bool shift, bool ctrl, bool alt, bool super_key);

 protected:
  // ── Window overrides ───────────────────────────────────────────────────────
  bool OpenImpl() override;
  void RequestCloseImpl() override;

  void ApplyNewFullscreen() override;
  void ApplyNewTitle() override;

  // Returns a CAMetalLayerSurface wrapping metal_view_'s backing CAMetalLayer.
  std::unique_ptr<Surface> CreateSurfaceImpl(Surface::TypeFlags allowed_types) override;
  // Marks metal_view_ as needing display (triggers a Vulkan present request).
  void RequestPaintImpl() override;

 private:
  NSWindow*         ns_window_  = nullptr;  // the top-level Cocoa window
  RexMetalView*     metal_view_ = nullptr;  // CAMetalLayer-backed content view
  RexWindowDelegate* delegate_  = nullptr;  // NSWindowDelegate adapter
};

// ── MacMenuItem ───────────────────────────────────────────────────────────────
// Minimal stub satisfying the platform-specific MenuItem::Create() requirement.
// Does not create native NSMenuItems; native menu support can be added later
// without changing the Window or Surface interfaces.
class MacMenuItem final : public MenuItem {
 public:
  MacMenuItem(Type type, const std::string& text, const std::string& hotkey,
              std::function<void()> callback)
      : MenuItem(type, text, hotkey, std::move(callback)) {}
  ~MacMenuItem() override = default;

 protected:
  void OnChildAdded(MenuItem*) override {}
  void OnChildRemoved(MenuItem*) override {}
};

}  // namespace ui
}  // namespace rex
