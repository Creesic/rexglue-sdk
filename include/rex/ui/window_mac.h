#pragma once
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <memory>
#include <string>
#include <utility>

#include <rex/ui/menu_item.h>
#include <rex/ui/virtual_key.h>
#include <rex/ui/window.h>

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

namespace rex::ui {

class MacWindow final : public Window {
 public:
  MacWindow(WindowedAppContext& app_context, std::string_view title,
            uint32_t desired_logical_width, uint32_t desired_logical_height);
  ~MacWindow() override;

  void* GetNativeWindowHandle() const override;
  uint32_t GetMediumDpi() const override { return 96; }
  uint32_t GetLatestDpiImpl() const override;

  void HandleWindowShouldClose();
  void HandleWindowDidResize();
  void HandleWindowDidBecomeKey();
  void HandleWindowDidResignKey();
  void HandlePaint();

  void HandleMouseButton(int32_t x, int32_t y, MouseEvent::Button button, bool is_down);
  void HandleMouseMove(int32_t x, int32_t y);
  void HandleMouseScroll(int32_t x, int32_t y, int32_t scroll_x, int32_t scroll_y);
  void HandleKeyDown(VirtualKey vk, uint32_t key_char, bool shift, bool ctrl, bool alt,
                     bool super_key);
  void HandleKeyUp(VirtualKey vk, bool shift, bool ctrl, bool alt, bool super_key);

 protected:
  bool OpenImpl() override;
  void RequestCloseImpl() override;

  void ApplyNewFullscreen() override;
  void ApplyNewTitle() override;

  std::unique_ptr<Surface> CreateSurfaceImpl(Surface::TypeFlags allowed_types) override;
  void RequestPaintImpl() override;

 private:
  NSWindow* ns_window_ = nullptr;
  RexMetalView* metal_view_ = nullptr;
  RexWindowDelegate* delegate_ = nullptr;
};

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

}  // namespace rex::ui
