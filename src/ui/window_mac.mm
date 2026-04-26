/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Grien Gupta, 2026 - macOS Cocoa/Metal window for ReXGlue
 *
 * MacWindow implementation — Cocoa NSWindow + CAMetalLayer for Vulkan/MoltenVK.
 *
 * File layout
 * ───────────
 *  1. Anonymous-namespace helpers
 *       ResolveWindowWidth/Height — read window_width/height cvars, falling
 *         back to video_mode cvars, then to the caller's requested size.
 *       TranslateKeyCode — maps Carbon kVK_* hardware key codes (which are
 *         layout-independent) to ReXGlue VirtualKey values.
 *
 *  2. Objective-C objects
 *       RexMetalView  — NSView subclass whose backing layer is a CAMetalLayer.
 *         Handles all mouse and keyboard NSEvents; converts coordinates and
 *         calls MacWindow::Handle* methods.
 *       RexWindowDelegate — NSWindowDelegate forwarding window lifecycle
 *         callbacks (close / resize / focus) to MacWindow::Handle* methods.
 *
 *  3. C++ implementation of MacWindow
 *       Owns ns_window_, metal_view_, and delegate_.  Implements the Window
 *       virtual interface (OpenImpl, RequestCloseImpl, surface creation, DPI,
 *       fullscreen, title).
 *
 * ObjC ↔ C++ boundary
 * ────────────────────
 * The ObjC classes cannot inherit from rex::ui::Window and therefore cannot
 * call its protected On* / WindowDestructionReceiver members.  To bridge this,
 * MacWindow exposes public Handle* methods that are thin wrappers around the
 * protected base-class methods.  The ObjC classes hold a raw MacWindow* and
 * call only those public methods — the same "WndProc thunk" pattern Win32Window
 * uses.
 *
 * CAMetalLayer lifecycle
 * ──────────────────────
 * The CAMetalLayer is created by -[RexMetalView makeBackingLayer] and is owned
 * by the NSView backing-layer mechanism.  MacWindow holds a non-owning pointer
 * obtained via metal_view_.layer (cast to CAMetalLayer*).  The layer is valid
 * for as long as ns_window_ exists.  CreateSurfaceImpl wraps it in a
 * CAMetalLayerSurface, which also holds a non-owning pointer; the presenter
 * must be torn down before MacWindow is destroyed.
 *
 * drawableSize synchronisation
 * ────────────────────────────
 * CAMetalLayer.drawableSize must be kept equal to the view's backing-pixel
 * size at all times, otherwise MoltenVK's swapchain images will be wrong.
 * We sync it in two places:
 *   - HandleWindowDidResize  — after every live resize.
 *   - CreateSurfaceImpl      — at swapchain creation time, to handle the case
 *     where the surface is (re)created without an intervening resize event.
 */

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#import <QuartzCore/CAMetalLayer.h>

#include <algorithm>
#include <string>

#include <rex/assert.h>
#include <rex/cvar.h>
#include <rex/graphics/flags.h>
#include <rex/graphics/video_mode_util.h>
#include <rex/logging.h>
#include <rex/ui/surface_mac.h>
#include <rex/ui/virtual_key.h>
#include <rex/ui/window_mac.h>

// ─── helpers ─────────────────────────────────────────────────────────────────

namespace {

uint32_t ResolveWindowWidth(uint32_t requested) {
  if (REXCVAR_GET(window_width) > 0) return uint32_t(REXCVAR_GET(window_width));
  if (!rex::cvar::HasNonDefaultValue("window_width")) {
    if (rex::cvar::HasNonDefaultValue("video_mode_width") && REXCVAR_GET(video_mode_width) > 0)
      return uint32_t(std::clamp(REXCVAR_GET(video_mode_width), 1, 8192));
    int32_t pw = 0, ph = 0;
    if (rex::graphics::video_mode_util::TryGetResolutionPresetFromCVar(pw, ph))
      return uint32_t(std::clamp(pw, 1, 8192));
  }
  return requested;
}

uint32_t ResolveWindowHeight(uint32_t requested) {
  if (REXCVAR_GET(window_height) > 0) return uint32_t(REXCVAR_GET(window_height));
  if (!rex::cvar::HasNonDefaultValue("window_height")) {
    if (rex::cvar::HasNonDefaultValue("video_mode_height") && REXCVAR_GET(video_mode_height) > 0)
      return uint32_t(std::clamp(REXCVAR_GET(video_mode_height), 1, 8192));
    int32_t pw = 0, ph = 0;
    if (rex::graphics::video_mode_util::TryGetResolutionPresetFromCVar(pw, ph))
      return uint32_t(std::clamp(ph, 1, 8192));
  }
  return requested;
}

// Translate a Carbon hardware key code to the Windows-style VirtualKey.
using rex::ui::VirtualKey;
VirtualKey TranslateKeyCode(unsigned short key_code) {
  switch (key_code) {
    case kVK_ANSI_A: return VirtualKey::kA;
    case kVK_ANSI_B: return VirtualKey::kB;
    case kVK_ANSI_C: return VirtualKey::kC;
    case kVK_ANSI_D: return VirtualKey::kD;
    case kVK_ANSI_E: return VirtualKey::kE;
    case kVK_ANSI_F: return VirtualKey::kF;
    case kVK_ANSI_G: return VirtualKey::kG;
    case kVK_ANSI_H: return VirtualKey::kH;
    case kVK_ANSI_I: return VirtualKey::kI;
    case kVK_ANSI_J: return VirtualKey::kJ;
    case kVK_ANSI_K: return VirtualKey::kK;
    case kVK_ANSI_L: return VirtualKey::kL;
    case kVK_ANSI_M: return VirtualKey::kM;
    case kVK_ANSI_N: return VirtualKey::kN;
    case kVK_ANSI_O: return VirtualKey::kO;
    case kVK_ANSI_P: return VirtualKey::kP;
    case kVK_ANSI_Q: return VirtualKey::kQ;
    case kVK_ANSI_R: return VirtualKey::kR;
    case kVK_ANSI_S: return VirtualKey::kS;
    case kVK_ANSI_T: return VirtualKey::kT;
    case kVK_ANSI_U: return VirtualKey::kU;
    case kVK_ANSI_V: return VirtualKey::kV;
    case kVK_ANSI_W: return VirtualKey::kW;
    case kVK_ANSI_X: return VirtualKey::kX;
    case kVK_ANSI_Y: return VirtualKey::kY;
    case kVK_ANSI_Z: return VirtualKey::kZ;
    case kVK_ANSI_0: return VirtualKey::k0;
    case kVK_ANSI_1: return VirtualKey::k1;
    case kVK_ANSI_2: return VirtualKey::k2;
    case kVK_ANSI_3: return VirtualKey::k3;
    case kVK_ANSI_4: return VirtualKey::k4;
    case kVK_ANSI_5: return VirtualKey::k5;
    case kVK_ANSI_6: return VirtualKey::k6;
    case kVK_ANSI_7: return VirtualKey::k7;
    case kVK_ANSI_8: return VirtualKey::k8;
    case kVK_ANSI_9: return VirtualKey::k9;
    case kVK_ANSI_Keypad0:        return VirtualKey::kNumpad0;
    case kVK_ANSI_Keypad1:        return VirtualKey::kNumpad1;
    case kVK_ANSI_Keypad2:        return VirtualKey::kNumpad2;
    case kVK_ANSI_Keypad3:        return VirtualKey::kNumpad3;
    case kVK_ANSI_Keypad4:        return VirtualKey::kNumpad4;
    case kVK_ANSI_Keypad5:        return VirtualKey::kNumpad5;
    case kVK_ANSI_Keypad6:        return VirtualKey::kNumpad6;
    case kVK_ANSI_Keypad7:        return VirtualKey::kNumpad7;
    case kVK_ANSI_Keypad8:        return VirtualKey::kNumpad8;
    case kVK_ANSI_Keypad9:        return VirtualKey::kNumpad9;
    case kVK_ANSI_KeypadPlus:     return VirtualKey::kAdd;
    case kVK_ANSI_KeypadMinus:    return VirtualKey::kSubtract;
    case kVK_ANSI_KeypadMultiply: return VirtualKey::kMultiply;
    case kVK_ANSI_KeypadDivide:   return VirtualKey::kDivide;
    case kVK_ANSI_KeypadDecimal:  return VirtualKey::kDecimal;
    case kVK_ANSI_KeypadEnter:    return VirtualKey::kReturn;
    case kVK_F1:  return VirtualKey::kF1;
    case kVK_F2:  return VirtualKey::kF2;
    case kVK_F3:  return VirtualKey::kF3;
    case kVK_F4:  return VirtualKey::kF4;
    case kVK_F5:  return VirtualKey::kF5;
    case kVK_F6:  return VirtualKey::kF6;
    case kVK_F7:  return VirtualKey::kF7;
    case kVK_F8:  return VirtualKey::kF8;
    case kVK_F9:  return VirtualKey::kF9;
    case kVK_F10: return VirtualKey::kF10;
    case kVK_F11: return VirtualKey::kF11;
    case kVK_F12: return VirtualKey::kF12;
    case kVK_F13: return VirtualKey::kF13;
    case kVK_F14: return VirtualKey::kF14;
    case kVK_F15: return VirtualKey::kF15;
    case kVK_F16: return VirtualKey::kF16;
    case kVK_F17: return VirtualKey::kF17;
    case kVK_F18: return VirtualKey::kF18;
    case kVK_F19: return VirtualKey::kF19;
    case kVK_F20: return VirtualKey::kF20;
    case kVK_LeftArrow:  return VirtualKey::kLeft;
    case kVK_RightArrow: return VirtualKey::kRight;
    case kVK_UpArrow:    return VirtualKey::kUp;
    case kVK_DownArrow:  return VirtualKey::kDown;
    case kVK_Home:       return VirtualKey::kHome;
    case kVK_End:        return VirtualKey::kEnd;
    case kVK_PageUp:     return VirtualKey::kPrior;
    case kVK_PageDown:   return VirtualKey::kNext;
    case kVK_Return:        return VirtualKey::kReturn;
    case kVK_Space:         return VirtualKey::kSpace;
    case kVK_Tab:           return VirtualKey::kTab;
    case kVK_Delete:        return VirtualKey::kBack;
    case kVK_ForwardDelete: return VirtualKey::kDelete;
    case kVK_Escape:        return VirtualKey::kEscape;
    case kVK_Shift:         return VirtualKey::kShift;
    case kVK_RightShift:    return VirtualKey::kShift;
    case kVK_Control:       return VirtualKey::kControl;
    case kVK_RightControl:  return VirtualKey::kControl;
    case kVK_Option:        return VirtualKey::kMenu;
    case kVK_RightOption:   return VirtualKey::kMenu;
    case kVK_Command:       return VirtualKey::kLWin;
    case kVK_RightCommand:  return VirtualKey::kRWin;
    case kVK_CapsLock:      return VirtualKey::kCapital;
    case kVK_ANSI_Grave:         return VirtualKey::kOem3;
    case kVK_ANSI_Minus:         return VirtualKey::kOemMinus;
    case kVK_ANSI_Equal:         return VirtualKey::kOemPlus;
    case kVK_ANSI_LeftBracket:   return VirtualKey::kOem4;
    case kVK_ANSI_RightBracket:  return VirtualKey::kOem6;
    case kVK_ANSI_Backslash:     return VirtualKey::kOem5;
    case kVK_ANSI_Semicolon:     return VirtualKey::kOem1;
    case kVK_ANSI_Quote:         return VirtualKey::kOem7;
    case kVK_ANSI_Comma:         return VirtualKey::kOemComma;
    case kVK_ANSI_Period:        return VirtualKey::kOemPeriod;
    case kVK_ANSI_Slash:         return VirtualKey::kOem2;
    default: break;
  }
  return VirtualKey::kNone;
}

}  // namespace

// ─── Objective-C objects ─────────────────────────────────────────────────────
//
// RexMetalView and RexWindowDelegate both hold a raw MacWindow* and forward
// all events via MacWindow's *public* Handle* methods, which in turn call the
// Window base-class protected On* methods.  This keeps all protected access
// inside the C++ class where it is legal.

@interface RexMetalView : NSView
- (instancetype)initWithFrame:(NSRect)frame cppWindow:(rex::ui::MacWindow*)win;
@end

@implementation RexMetalView {
  rex::ui::MacWindow* cpp_window_;
}

- (instancetype)initWithFrame:(NSRect)frame cppWindow:(rex::ui::MacWindow*)win {
  self = [super initWithFrame:frame];
  if (self) {
    cpp_window_ = win;
    self.wantsLayer = YES;
  }
  return self;
}

// Tell Cocoa that this view's backing layer class is CAMetalLayer so that
// -makeBackingLayer is invoked with the correct layer type.
+ (Class)layerClass { return [CAMetalLayer class]; }

- (CALayer*)makeBackingLayer {
  CAMetalLayer* layer = [CAMetalLayer layer];
  // BGRA8 is the most widely supported Metal swapchain format and matches the
  // VkFormat (VK_FORMAT_B8G8R8A8_UNORM) negotiated by the Vulkan presenter.
  layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  // framebufferOnly=YES tells Metal the layer is used exclusively as a render
  // target, enabling GPU driver optimisations (e.g. lossless compression).
  layer.framebufferOnly = YES;
  return layer;
}

- (BOOL)wantsUpdateLayer   { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)isOpaque { return YES; }

// ── Mouse helpers ─────────────────────────────────────────────────────────────

- (void)handleMouseEvent:(NSEvent*)event
                  button:(rex::ui::MouseEvent::Button)button
                    down:(BOOL)is_down {
  if (!cpp_window_) return;
  NSPoint loc = [self convertPoint:event.locationInWindow fromView:nil];
  int32_t x = static_cast<int32_t>(loc.x);
  // NSView origin is bottom-left; subtract from height to flip to top-left
  // origin, matching ReXGlue's coordinate convention (and Win32 client coords).
  int32_t y = static_cast<int32_t>(self.bounds.size.height - loc.y);
  cpp_window_->HandleMouseButton(x, y, button, (bool)is_down);
}

- (void)mouseMoved:(NSEvent*)event {
  if (!cpp_window_) return;
  NSPoint loc = [self convertPoint:event.locationInWindow fromView:nil];
  cpp_window_->HandleMouseMove(static_cast<int32_t>(loc.x),
                               static_cast<int32_t>(self.bounds.size.height - loc.y));
}
- (void)mouseDragged:(NSEvent*)e      { [self mouseMoved:e]; }
- (void)rightMouseDragged:(NSEvent*)e { [self mouseMoved:e]; }
- (void)otherMouseDragged:(NSEvent*)e { [self mouseMoved:e]; }

- (void)mouseDown:(NSEvent*)e {
  [self handleMouseEvent:e button:rex::ui::MouseEvent::Button::kLeft  down:YES];
}
- (void)mouseUp:(NSEvent*)e {
  [self handleMouseEvent:e button:rex::ui::MouseEvent::Button::kLeft  down:NO];
}
- (void)rightMouseDown:(NSEvent*)e {
  [self handleMouseEvent:e button:rex::ui::MouseEvent::Button::kRight down:YES];
}
- (void)rightMouseUp:(NSEvent*)e {
  [self handleMouseEvent:e button:rex::ui::MouseEvent::Button::kRight down:NO];
}
- (void)scrollWheel:(NSEvent*)e {
  if (!cpp_window_) return;
  NSPoint loc = [self convertPoint:e.locationInWindow fromView:nil];
  int32_t x = static_cast<int32_t>(loc.x);
  int32_t y = static_cast<int32_t>(self.bounds.size.height - loc.y);  // flip Y
  // Scale Cocoa's continuous scroll delta to ReXGlue's detent-based units.
  // scrollingDeltaX/Y is already in "points" for both trackpad and mouse wheel.
  int32_t sx = static_cast<int32_t>(e.scrollingDeltaX * rex::ui::MouseEvent::kScrollPerDetent);
  int32_t sy = static_cast<int32_t>(e.scrollingDeltaY * rex::ui::MouseEvent::kScrollPerDetent);
  cpp_window_->HandleMouseScroll(x, y, sx, sy);
}

// ── Key helpers ───────────────────────────────────────────────────────────────

- (void)keyDown:(NSEvent*)event {
  if (!cpp_window_) return;
  NSEventModifierFlags mods = event.modifierFlags;
  bool shift = (mods & NSEventModifierFlagShift)   != 0;
  bool ctrl  = (mods & NSEventModifierFlagControl) != 0;
  bool alt   = (mods & NSEventModifierFlagOption)  != 0;
  bool super_key = (mods & NSEventModifierFlagCommand) != 0;
  VirtualKey vk = TranslateKeyCode(event.keyCode);
  uint32_t key_char = 0;
  NSString* chars = event.characters;
  if (chars.length > 0) {
    unichar ch = [chars characterAtIndex:0];
    // Only forward printable Unicode code points; skip control chars (< 0x20)
    // and DEL (0x7F) which are not suitable for OnKeyChar text input.
    if (ch >= 0x20 && ch != 0x7F) key_char = ch;
  }
  cpp_window_->HandleKeyDown(vk, key_char, shift, ctrl, alt, super_key);
}

- (void)keyUp:(NSEvent*)event {
  if (!cpp_window_) return;
  NSEventModifierFlags mods = event.modifierFlags;
  bool shift = (mods & NSEventModifierFlagShift)   != 0;
  bool ctrl  = (mods & NSEventModifierFlagControl) != 0;
  bool alt   = (mods & NSEventModifierFlagOption)  != 0;
  bool super_key = (mods & NSEventModifierFlagCommand) != 0;
  cpp_window_->HandleKeyUp(TranslateKeyCode(event.keyCode), shift, ctrl, alt, super_key);
}

// performKeyEquivalent: is called by Cocoa for Cmd+Key combinations before the
// normal responder chain processes them.  Forwarding to keyDown: ensures that
// Cmd+key shortcuts (e.g. Cmd+Q captured by the emulator) reach the game.
// Returning YES prevents AppKit from handling the event a second time.
- (BOOL)performKeyEquivalent:(NSEvent*)event {
  [self keyDown:event];
  return YES;
}

@end

@interface RexWindowDelegate : NSObject<NSWindowDelegate>
- (instancetype)initWithCppWindow:(rex::ui::MacWindow*)win;
@end

@implementation RexWindowDelegate {
  rex::ui::MacWindow* cpp_window_;
}

- (instancetype)initWithCppWindow:(rex::ui::MacWindow*)win {
  self = [super init];
  if (self) cpp_window_ = win;
  return self;
}

// Return NO to prevent Cocoa from closing the window immediately; the C++ side
// decides whether to actually close via OnBeforeClose / OnAfterClose.
- (BOOL)windowShouldClose:(NSWindow*)sender {
  if (cpp_window_) cpp_window_->HandleWindowShouldClose();
  return NO;
}
- (void)windowDidResize:(NSNotification*)note {
  if (cpp_window_) cpp_window_->HandleWindowDidResize();
}
- (void)windowDidBecomeKey:(NSNotification*)note {
  if (cpp_window_) cpp_window_->HandleWindowDidBecomeKey();
}
- (void)windowDidResignKey:(NSNotification*)note {
  if (cpp_window_) cpp_window_->HandleWindowDidResignKey();
}

@end

// ─── C++ implementation ───────────────────────────────────────────────────────

namespace rex {
namespace ui {

std::unique_ptr<Window> Window::Create(WindowedAppContext& app_context,
                                       const std::string_view title,
                                       uint32_t desired_logical_width,
                                       uint32_t desired_logical_height) {
  desired_logical_width  = ResolveWindowWidth(desired_logical_width);
  desired_logical_height = ResolveWindowHeight(desired_logical_height);
  return std::make_unique<MacWindow>(app_context, title,
                                     desired_logical_width, desired_logical_height);
}

MacWindow::MacWindow(WindowedAppContext& app_context, const std::string_view title,
                     uint32_t desired_logical_width, uint32_t desired_logical_height)
    : Window(app_context, title, desired_logical_width, desired_logical_height) {}

MacWindow::~MacWindow() {
  // Notify the base class early so that no further On* callbacks fire during
  // teardown (e.g. if a resize event arrives while we're destroying objects).
  EnterDestructor();
  if (ns_window_) {
    // Detach the delegate before closing so -windowShouldClose: and -windowDidResize:
    // are not called back into a partially-destroyed MacWindow.
    [ns_window_ setDelegate:nil];
    delegate_ = nullptr;
    metal_view_ = nullptr;  // released when the window's content view is cleared
    [ns_window_ close];
    ns_window_ = nullptr;
  }
}

bool MacWindow::OpenImpl() {
  NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                     NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
  NSRect content_rect = NSMakeRect(0, 0,
                                   CGFloat(GetDesiredLogicalWidth()),
                                   CGFloat(GetDesiredLogicalHeight()));

  ns_window_ = [[NSWindow alloc] initWithContentRect:content_rect
                                           styleMask:style
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
  if (!ns_window_) {
    REXLOG_ERROR("MacWindow: Failed to create NSWindow");
    return false;
  }

  ns_window_.title = [NSString stringWithUTF8String:GetTitle().c_str()];
  [ns_window_ center];

  metal_view_ = [[RexMetalView alloc] initWithFrame:content_rect cppWindow:this];
  if (!metal_view_) {
    REXLOG_ERROR("MacWindow: Failed to create RexMetalView");
    [ns_window_ close];
    ns_window_ = nullptr;
    return false;
  }
  ns_window_.contentView = metal_view_;

  delegate_ = [[RexWindowDelegate alloc] initWithCppWindow:this];
  [ns_window_ setDelegate:delegate_];
  [ns_window_ setAcceptsMouseMovedEvents:YES];

  if (int32_t mon = REXCVAR_GET(monitor); mon > 0) {
    NSArray<NSScreen*>* screens = [NSScreen screens];
    if (mon <= static_cast<int32_t>(screens.count)) {
      NSScreen* target = screens[mon - 1];
      NSRect sr = target.visibleFrame;
      [ns_window_ setFrameOrigin:NSMakePoint(
          sr.origin.x + (sr.size.width  - content_rect.size.width)  / 2.0,
          sr.origin.y + (sr.size.height - content_rect.size.height) / 2.0)];
    }
  }

  if (IsFullscreen()) {
    // If the app was started with --fullscreen, enter fullscreen immediately.
    // toggleFullScreen: is asynchronous; a windowDidResize: callback will fire
    // once the transition completes and update the actual size.
    [ns_window_ toggleFullScreen:nil];
  }

  // makeKeyAndOrderFront: makes the window visible and gives it keyboard focus.
  // Must be called after all setup (delegate, content view, fullscreen) is done.
  [ns_window_ makeKeyAndOrderFront:nil];

  {
    // WindowDestructionReceiver guards against re-entrant window destruction:
    // any On* callback below could trigger app code that calls RequestClose(),
    // which sets a destroyed/closed flag checked after each call.
    WindowDestructionReceiver dr(this);
    // Sync the initial backing-pixel size so the base class knows the real
    // drawable dimensions before the first paint.
    NSSize backing = [metal_view_ convertSizeToBacking:metal_view_.bounds.size];
    OnActualSizeUpdate(static_cast<uint32_t>(backing.width),
                       static_cast<uint32_t>(backing.height), dr);
    if (dr.IsWindowDestroyedOrClosed()) return true;
    if ([ns_window_ isKeyWindow]) {
      OnFocusUpdate(true, dr);
      if (dr.IsWindowDestroyedOrClosed()) return true;
    }
  }
  return true;
}

void MacWindow::RequestCloseImpl() {
  [ns_window_ performClose:nil];
}

void MacWindow::HandleWindowShouldClose() {
  WindowDestructionReceiver dr(this);
  OnBeforeClose(dr);
  if (dr.IsWindowDestroyed()) return;
  if (ns_window_) {
    [ns_window_ setDelegate:nil];
    delegate_ = nullptr;
    metal_view_ = nullptr;
    [ns_window_ close];
    ns_window_ = nullptr;
  }
  OnAfterClose();
}

void MacWindow::HandleWindowDidResize() {
  if (!metal_view_) return;
  // bounds.size is in logical (DIP) points; convertSizeToBacking: scales by the
  // display's backingScaleFactor to yield physical pixel dimensions.
  NSSize backing = [metal_view_ convertSizeToBacking:metal_view_.bounds.size];
  // Keep drawableSize in sync with the physical pixel size.  If we don't do
  // this here, MoltenVK will present into a swapchain sized for the old window
  // and produce stretched or clipped output until the next surface recreation.
  CAMetalLayer* layer = reinterpret_cast<CAMetalLayer*>(metal_view_.layer);
  layer.drawableSize = CGSizeMake(backing.width, backing.height);
  WindowDestructionReceiver dr(this);
  OnActualSizeUpdate(static_cast<uint32_t>(backing.width),
                     static_cast<uint32_t>(backing.height), dr);
}

void MacWindow::HandleWindowDidBecomeKey() {
  WindowDestructionReceiver dr(this);
  OnFocusUpdate(true, dr);
}

void MacWindow::HandleWindowDidResignKey() {
  WindowDestructionReceiver dr(this);
  OnFocusUpdate(false, dr);
}

void MacWindow::HandleMouseButton(int32_t x, int32_t y,
                                   MouseEvent::Button button, bool is_down) {
  MouseEvent e(this, button, x, y, 0, 0);
  WindowDestructionReceiver dr(this);
  if (is_down) {
    OnMouseDown(e, dr);
  } else {
    OnMouseUp(e, dr);
  }
}

void MacWindow::HandleMouseMove(int32_t x, int32_t y) {
  MouseEvent e(this, MouseEvent::Button::kNone, x, y, 0, 0);
  WindowDestructionReceiver dr(this);
  OnMouseMove(e, dr);
}

void MacWindow::HandleMouseScroll(int32_t x, int32_t y, int32_t scroll_x, int32_t scroll_y) {
  MouseEvent e(this, MouseEvent::Button::kNone, x, y, scroll_x, scroll_y);
  WindowDestructionReceiver dr(this);
  OnMouseWheel(e, dr);
}

void MacWindow::HandleKeyDown(VirtualKey vk, uint32_t key_char,
                               bool shift, bool ctrl, bool alt, bool super_key) {
  KeyEvent e(this, vk, 1, false, shift, ctrl, alt, super_key);
  WindowDestructionReceiver dr(this);
  OnKeyDown(e, dr);
  if (dr.IsWindowDestroyedOrClosed()) return;
  if (key_char > 0) {
    KeyEvent ce(this, VirtualKey(key_char), 1, false, shift, ctrl, alt, super_key);
    OnKeyChar(ce, dr);
  }
}

void MacWindow::HandleKeyUp(VirtualKey vk,
                             bool shift, bool ctrl, bool alt, bool super_key) {
  KeyEvent e(this, vk, 1, true, shift, ctrl, alt, super_key);
  WindowDestructionReceiver dr(this);
  OnKeyUp(e, dr);
}

void MacWindow::ApplyNewFullscreen() {
  if (!ns_window_) return;
  // Query the current Cocoa fullscreen state via styleMask and toggle only if
  // it disagrees with the desired state set by the base class.  toggleFullScreen:
  // is asynchronous; windowDidResize: will fire when the transition completes.
  bool currently_fullscreen = (ns_window_.styleMask & NSWindowStyleMaskFullScreen) != 0;
  if (IsFullscreen() != currently_fullscreen) {
    [ns_window_ toggleFullScreen:nil];
  }
}

void MacWindow::ApplyNewTitle() {
  if (!ns_window_) return;
  ns_window_.title = [NSString stringWithUTF8String:GetTitle().c_str()];
}

std::unique_ptr<Surface> MacWindow::CreateSurfaceImpl(Surface::TypeFlags allowed_types) {
  if (!(allowed_types & Surface::kTypeFlag_CAMetalLayer)) {
    REXLOG_ERROR("MacWindow: CAMetalLayer surface type not supported by Vulkan provider");
    return nullptr;
  }
  if (!metal_view_ || !metal_view_.layer) {
    REXLOG_ERROR("MacWindow: metal_view_ or its layer is null");
    return nullptr;
  }
  // Sync drawableSize at surface-creation time in case a window resize happened
  // between the last HandleWindowDidResize and this call (e.g. when the Vulkan
  // swapchain is recreated after a VK_ERROR_OUT_OF_DATE_KHR without a matching
  // resize notification).
  NSSize backing = [metal_view_ convertSizeToBacking:metal_view_.bounds.size];
  CAMetalLayer* layer = reinterpret_cast<CAMetalLayer*>(metal_view_.layer);
  layer.drawableSize = CGSizeMake(backing.width, backing.height);
  // CAMetalLayerSurface holds a non-owning pointer; the layer's lifetime is
  // guaranteed by ns_window_ / metal_view_ which outlive the surface.
  return std::make_unique<CAMetalLayerSurface>(layer);
}

void MacWindow::RequestPaintImpl() {
  if (metal_view_) [metal_view_ setNeedsDisplay:YES];
}

uint32_t MacWindow::GetLatestDpiImpl() const {
  if (!ns_window_) return GetMediumDpi();
  // backingScaleFactor is 1.0 on standard displays, 2.0 on 2× Retina, etc.
  // Multiplying the Windows-baseline DPI (96) gives 96 or 192, matching what
  // the Win32 path returns for equivalent display configurations.
  return static_cast<uint32_t>(ns_window_.backingScaleFactor * GetMediumDpi());
}

void* MacWindow::GetNativeWindowHandle() const {
  // __bridge cast transfers the ObjC object pointer to a void* without changing
  // ARC ownership — safe because MacWindow outlives any caller of this method.
  return (__bridge void*)ns_window_;
}

// ─── MenuItem::Create ────────────────────────────────────────────────────────

std::unique_ptr<ui::MenuItem> MenuItem::Create(Type type, const std::string& text,
                                               const std::string& hotkey,
                                               std::function<void()> callback) {
  return std::make_unique<MacMenuItem>(type, text, hotkey, std::move(callback));
}

}  // namespace ui
}  // namespace rex
