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

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#import <Metal/Metal.h>
#import <QuartzCore/CADisplayLink.h>
#import <QuartzCore/CAMetalLayer.h>

#include <algorithm>
#include <string>

#include <dispatch/dispatch.h>

#include <rex/cvar.h>
#include <rex/graphics/flags.h>
#include <rex/graphics/video_mode_util.h>
#include <rex/logging.h>
#include <rex/ui/surface_mac.h>
#include <rex/ui/virtual_key.h>
#include <rex/ui/window_mac.h>

namespace {

uint32_t ResolveWindowWidth(uint32_t requested_width) {
  if (REXCVAR_GET(window_width) > 0) {
    return uint32_t(REXCVAR_GET(window_width));
  }
  if (!rex::cvar::HasNonDefaultValue("window_width")) {
    if (rex::cvar::HasNonDefaultValue("video_mode_width") && REXCVAR_GET(video_mode_width) > 0) {
      return uint32_t(std::clamp(REXCVAR_GET(video_mode_width), 1, 8192));
    }
    int32_t preset_width = 0;
    int32_t preset_height = 0;
    if (rex::graphics::video_mode_util::TryGetResolutionPresetFromCVar(preset_width,
                                                                       preset_height)) {
      return uint32_t(std::clamp(preset_width, 1, 8192));
    }
  }
  return requested_width;
}

uint32_t ResolveWindowHeight(uint32_t requested_height) {
  if (REXCVAR_GET(window_height) > 0) {
    return uint32_t(REXCVAR_GET(window_height));
  }
  if (!rex::cvar::HasNonDefaultValue("window_height")) {
    if (rex::cvar::HasNonDefaultValue("video_mode_height") && REXCVAR_GET(video_mode_height) > 0) {
      return uint32_t(std::clamp(REXCVAR_GET(video_mode_height), 1, 8192));
    }
    int32_t preset_width = 0;
    int32_t preset_height = 0;
    if (rex::graphics::video_mode_util::TryGetResolutionPresetFromCVar(preset_width,
                                                                       preset_height)) {
      return uint32_t(std::clamp(preset_height, 1, 8192));
    }
  }
  return requested_height;
}

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
    case kVK_ANSI_Keypad0: return VirtualKey::kNumpad0;
    case kVK_ANSI_Keypad1: return VirtualKey::kNumpad1;
    case kVK_ANSI_Keypad2: return VirtualKey::kNumpad2;
    case kVK_ANSI_Keypad3: return VirtualKey::kNumpad3;
    case kVK_ANSI_Keypad4: return VirtualKey::kNumpad4;
    case kVK_ANSI_Keypad5: return VirtualKey::kNumpad5;
    case kVK_ANSI_Keypad6: return VirtualKey::kNumpad6;
    case kVK_ANSI_Keypad7: return VirtualKey::kNumpad7;
    case kVK_ANSI_Keypad8: return VirtualKey::kNumpad8;
    case kVK_ANSI_Keypad9: return VirtualKey::kNumpad9;
    case kVK_ANSI_KeypadPlus: return VirtualKey::kAdd;
    case kVK_ANSI_KeypadMinus: return VirtualKey::kSubtract;
    case kVK_ANSI_KeypadMultiply: return VirtualKey::kMultiply;
    case kVK_ANSI_KeypadDivide: return VirtualKey::kDivide;
    case kVK_ANSI_KeypadDecimal: return VirtualKey::kDecimal;
    case kVK_ANSI_KeypadEnter: return VirtualKey::kReturn;
    case kVK_F1: return VirtualKey::kF1;
    case kVK_F2: return VirtualKey::kF2;
    case kVK_F3: return VirtualKey::kF3;
    case kVK_F4: return VirtualKey::kF4;
    case kVK_F5: return VirtualKey::kF5;
    case kVK_F6: return VirtualKey::kF6;
    case kVK_F7: return VirtualKey::kF7;
    case kVK_F8: return VirtualKey::kF8;
    case kVK_F9: return VirtualKey::kF9;
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
    case kVK_LeftArrow: return VirtualKey::kLeft;
    case kVK_RightArrow: return VirtualKey::kRight;
    case kVK_UpArrow: return VirtualKey::kUp;
    case kVK_DownArrow: return VirtualKey::kDown;
    case kVK_Home: return VirtualKey::kHome;
    case kVK_End: return VirtualKey::kEnd;
    case kVK_PageUp: return VirtualKey::kPrior;
    case kVK_PageDown: return VirtualKey::kNext;
    case kVK_Return: return VirtualKey::kReturn;
    case kVK_Space: return VirtualKey::kSpace;
    case kVK_Tab: return VirtualKey::kTab;
    case kVK_Delete: return VirtualKey::kBack;
    case kVK_ForwardDelete: return VirtualKey::kDelete;
    case kVK_Escape: return VirtualKey::kEscape;
    case kVK_Shift: return VirtualKey::kShift;
    case kVK_RightShift: return VirtualKey::kShift;
    case kVK_Control: return VirtualKey::kControl;
    case kVK_RightControl: return VirtualKey::kControl;
    case kVK_Option: return VirtualKey::kMenu;
    case kVK_RightOption: return VirtualKey::kMenu;
    case kVK_Command: return VirtualKey::kLWin;
    case kVK_RightCommand: return VirtualKey::kRWin;
    case kVK_CapsLock: return VirtualKey::kCapital;
    case kVK_ANSI_Grave: return VirtualKey::kOem3;
    case kVK_ANSI_Minus: return VirtualKey::kOemMinus;
    case kVK_ANSI_Equal: return VirtualKey::kOemPlus;
    case kVK_ANSI_LeftBracket: return VirtualKey::kOem4;
    case kVK_ANSI_RightBracket: return VirtualKey::kOem6;
    case kVK_ANSI_Backslash: return VirtualKey::kOem5;
    case kVK_ANSI_Semicolon: return VirtualKey::kOem1;
    case kVK_ANSI_Quote: return VirtualKey::kOem7;
    case kVK_ANSI_Comma: return VirtualKey::kOemComma;
    case kVK_ANSI_Period: return VirtualKey::kOemPeriod;
    case kVK_ANSI_Slash: return VirtualKey::kOem2;
    default: break;
  }
  return VirtualKey::kNone;
}

}  // namespace

@interface RexMetalView : NSView
- (instancetype)initWithFrame:(NSRect)frame cppWindow:(rex::ui::MacWindow*)win;
@end

@implementation RexMetalView {
  rex::ui::MacWindow* cpp_window_;
  CADisplayLink* display_link_;
}

- (void)dealloc {
  [self stopDisplayLink];
  [super dealloc];
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  if (self.window) {
    [self startDisplayLinkIfNeeded];
  } else {
    [self stopDisplayLink];
  }
}

- (void)startDisplayLinkIfNeeded {
  if (display_link_ || !self.window) {
    return;
  }
  display_link_ = [self.window.screen displayLinkWithTarget:self
                                                   selector:@selector(onDisplayLink:)];
  [display_link_ addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)stopDisplayLink {
  [display_link_ invalidate];
  display_link_ = nil;
}

- (void)onDisplayLink:(CADisplayLink*)link {
  (void)link;
  if (cpp_window_) {
    cpp_window_->HandlePaint();
  }
}

- (instancetype)initWithFrame:(NSRect)frame cppWindow:(rex::ui::MacWindow*)win {
  self = [super initWithFrame:frame];
  if (self) {
    cpp_window_ = win;
    self.wantsLayer = YES;
  }
  return self;
}

+ (Class)layerClass { return [CAMetalLayer class]; }

- (CALayer*)makeBackingLayer {
  CAMetalLayer* layer = [CAMetalLayer layer];
  layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  layer.framebufferOnly = YES;
  layer.displaySyncEnabled = YES;
  return layer;
}

- (void)layout {
  [super layout];
  CALayer* backing = self.layer;
  if (!backing) {
    return;
  }
  backing.frame = self.bounds;
  if ([backing isKindOfClass:[CAMetalLayer class]]) {
    CAMetalLayer* metal_layer = (CAMetalLayer*)backing;
    CGFloat scale = self.window ? self.window.backingScaleFactor : 1.0;
    metal_layer.contentsScale = scale;
    CGSize size = self.bounds.size;
    if (size.width > 0.0 && size.height > 0.0) {
      metal_layer.drawableSize =
          CGSizeMake(size.width * scale, size.height * scale);
    }
  }
}

- (BOOL)wantsUpdateLayer { return NO; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)isOpaque { return YES; }

- (void)updateLayer {
  // Drawing is driven by CADisplayLink; avoid layer-backing paint conflicts.
}

- (void)handleMouseEvent:(NSEvent*)event button:(rex::ui::MouseEvent::Button)button down:(BOOL)is_down {
  if (!cpp_window_) {
    return;
  }
  NSPoint loc = [self convertPoint:event.locationInWindow fromView:nil];
  int32_t x = static_cast<int32_t>(loc.x);
  int32_t y = static_cast<int32_t>(self.bounds.size.height - loc.y);
  cpp_window_->HandleMouseButton(x, y, button, bool(is_down));
}

- (void)mouseMoved:(NSEvent*)event {
  if (!cpp_window_) {
    return;
  }
  NSPoint loc = [self convertPoint:event.locationInWindow fromView:nil];
  cpp_window_->HandleMouseMove(static_cast<int32_t>(loc.x),
                               static_cast<int32_t>(self.bounds.size.height - loc.y));
}

- (void)mouseDragged:(NSEvent*)event { [self mouseMoved:event]; }
- (void)rightMouseDragged:(NSEvent*)event { [self mouseMoved:event]; }
- (void)otherMouseDragged:(NSEvent*)event { [self mouseMoved:event]; }

- (void)mouseDown:(NSEvent*)event {
  [self handleMouseEvent:event button:rex::ui::MouseEvent::Button::kLeft down:YES];
}

- (void)mouseUp:(NSEvent*)event {
  [self handleMouseEvent:event button:rex::ui::MouseEvent::Button::kLeft down:NO];
}

- (void)rightMouseDown:(NSEvent*)event {
  [self handleMouseEvent:event button:rex::ui::MouseEvent::Button::kRight down:YES];
}

- (void)rightMouseUp:(NSEvent*)event {
  [self handleMouseEvent:event button:rex::ui::MouseEvent::Button::kRight down:NO];
}

- (void)scrollWheel:(NSEvent*)event {
  if (!cpp_window_) {
    return;
  }
  NSPoint loc = [self convertPoint:event.locationInWindow fromView:nil];
  int32_t x = static_cast<int32_t>(loc.x);
  int32_t y = static_cast<int32_t>(self.bounds.size.height - loc.y);
  int32_t sx = static_cast<int32_t>(event.scrollingDeltaX * rex::ui::MouseEvent::kScrollPerDetent);
  int32_t sy = static_cast<int32_t>(event.scrollingDeltaY * rex::ui::MouseEvent::kScrollPerDetent);
  cpp_window_->HandleMouseScroll(x, y, sx, sy);
}

- (void)keyDown:(NSEvent*)event {
  if (!cpp_window_) {
    return;
  }
  NSEventModifierFlags mods = event.modifierFlags;
  bool shift = (mods & NSEventModifierFlagShift) != 0;
  bool ctrl = (mods & NSEventModifierFlagControl) != 0;
  bool alt = (mods & NSEventModifierFlagOption) != 0;
  bool super_key = (mods & NSEventModifierFlagCommand) != 0;
  VirtualKey vk = TranslateKeyCode(event.keyCode);
  uint32_t key_char = 0;
  NSString* chars = event.characters;
  if (chars.length > 0) {
    unichar ch = [chars characterAtIndex:0];
    if (ch >= 0x20 && ch != 0x7F) {
      key_char = ch;
    }
  }
  cpp_window_->HandleKeyDown(vk, key_char, shift, ctrl, alt, super_key);
}

- (void)keyUp:(NSEvent*)event {
  if (!cpp_window_) {
    return;
  }
  NSEventModifierFlags mods = event.modifierFlags;
  bool shift = (mods & NSEventModifierFlagShift) != 0;
  bool ctrl = (mods & NSEventModifierFlagControl) != 0;
  bool alt = (mods & NSEventModifierFlagOption) != 0;
  bool super_key = (mods & NSEventModifierFlagCommand) != 0;
  cpp_window_->HandleKeyUp(TranslateKeyCode(event.keyCode), shift, ctrl, alt, super_key);
}

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
  if (self) {
    cpp_window_ = win;
  }
  return self;
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
  (void)sender;
  if (cpp_window_) {
    cpp_window_->HandleWindowShouldClose();
  }
  return NO;
}

- (void)windowDidResize:(NSNotification*)notification {
  (void)notification;
  if (cpp_window_) {
    cpp_window_->HandleWindowDidResize();
  }
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
  (void)notification;
  if (cpp_window_) {
    cpp_window_->HandleWindowDidBecomeKey();
  }
}

- (void)windowDidResignKey:(NSNotification*)notification {
  (void)notification;
  if (cpp_window_) {
    cpp_window_->HandleWindowDidResignKey();
  }
}

@end

namespace rex::ui {

std::unique_ptr<Window> Window::Create(WindowedAppContext& app_context, std::string_view title,
                                       uint32_t desired_logical_width,
                                       uint32_t desired_logical_height) {
  desired_logical_width = ResolveWindowWidth(desired_logical_width);
  desired_logical_height = ResolveWindowHeight(desired_logical_height);
  return std::make_unique<MacWindow>(app_context, title, desired_logical_width,
                                     desired_logical_height);
}

MacWindow::MacWindow(WindowedAppContext& app_context, std::string_view title,
                     uint32_t desired_logical_width, uint32_t desired_logical_height)
    : Window(app_context, title, desired_logical_width, desired_logical_height) {}

MacWindow::~MacWindow() {
  EnterDestructor();
  if (ns_window_) {
    [ns_window_ setDelegate:nil];
    delegate_ = nullptr;
    metal_view_ = nullptr;
    [ns_window_ close];
    ns_window_ = nullptr;
  }
}

bool MacWindow::OpenImpl() {
  NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                     NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
  NSRect content_rect =
      NSMakeRect(0, 0, CGFloat(GetDesiredLogicalWidth()), CGFloat(GetDesiredLogicalHeight()));

  ns_window_ = [[NSWindow alloc] initWithContentRect:content_rect
                                           styleMask:style
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
  if (!ns_window_) {
    REXLOG_ERROR("MacWindow: Failed to create NSWindow");
    return false;
  }

  [ns_window_ setTitle:[NSString stringWithUTF8String:GetTitle().c_str()]];
  [ns_window_ center];

  metal_view_ = [[RexMetalView alloc] initWithFrame:content_rect cppWindow:this];
  if (!metal_view_) {
    REXLOG_ERROR("MacWindow: Failed to create RexMetalView");
    [ns_window_ close];
    ns_window_ = nullptr;
    return false;
  }
  [ns_window_ setContentView:metal_view_];
  [metal_view_ setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  [ns_window_ makeFirstResponder:metal_view_];

  delegate_ = [[RexWindowDelegate alloc] initWithCppWindow:this];
  [ns_window_ setDelegate:delegate_];
  [ns_window_ setAcceptsMouseMovedEvents:YES];

  if (int32_t mon = REXCVAR_GET(monitor); mon > 0) {
    NSArray<NSScreen*>* screens = [NSScreen screens];
    if (mon <= static_cast<int32_t>(screens.count)) {
      NSScreen* target = screens[mon - 1];
      NSRect sr = target.visibleFrame;
      [ns_window_ setFrameOrigin:NSMakePoint(
                       sr.origin.x + (sr.size.width - content_rect.size.width) / 2.0,
                       sr.origin.y + (sr.size.height - content_rect.size.height) / 2.0)];
    }
  }

  if (IsFullscreen()) {
    [ns_window_ toggleFullScreen:nil];
  }

  [ns_window_ makeKeyAndOrderFront:nil];

  WindowDestructionReceiver destruction_receiver(this);
  NSSize backing = [metal_view_ convertSizeToBacking:[metal_view_ bounds].size];
  OnActualSizeUpdate(static_cast<uint32_t>(backing.width), static_cast<uint32_t>(backing.height),
                     destruction_receiver);
  if (destruction_receiver.IsWindowDestroyedOrClosed()) {
    return true;
  }
  if ([ns_window_ isKeyWindow]) {
    OnFocusUpdate(true, destruction_receiver);
  }
  return true;
}

void MacWindow::RequestCloseImpl() {
  if (ns_window_) {
    [ns_window_ performClose:nil];
  }
}

void MacWindow::HandleWindowShouldClose() {
  WindowDestructionReceiver destruction_receiver(this);
  OnBeforeClose(destruction_receiver);
  if (destruction_receiver.IsWindowDestroyed()) {
    return;
  }
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
  if (!metal_view_) {
    return;
  }
  NSSize backing = [metal_view_ convertSizeToBacking:[metal_view_ bounds].size];
  CAMetalLayer* layer = reinterpret_cast<CAMetalLayer*>([metal_view_ layer]);
  if (layer) {
    layer.drawableSize = CGSizeMake(backing.width, backing.height);
  }
  WindowDestructionReceiver destruction_receiver(this);
  OnActualSizeUpdate(static_cast<uint32_t>(backing.width), static_cast<uint32_t>(backing.height),
                     destruction_receiver);
}

void MacWindow::HandleWindowDidBecomeKey() {
  WindowDestructionReceiver destruction_receiver(this);
  OnFocusUpdate(true, destruction_receiver);
}

void MacWindow::HandleWindowDidResignKey() {
  WindowDestructionReceiver destruction_receiver(this);
  OnFocusUpdate(false, destruction_receiver);
}

void MacWindow::HandlePaint() {
  if (REXCVAR_QUERY(bool, metal_present_probe)) {
    static uint32_t handle_paint_probe_count = 0;
    if (handle_paint_probe_count < 16) {
      ++handle_paint_probe_count;
      REXLOG_WARN("MacWindow::HandlePaint view={} window={}", fmt::ptr(metal_view_),
                  fmt::ptr(ns_window_));
    }
  }
  OnPaint();
}

void MacWindow::HandleMouseButton(int32_t x, int32_t y, MouseEvent::Button button, bool is_down) {
  MouseEvent event(this, button, x, y, 0, 0);
  WindowDestructionReceiver destruction_receiver(this);
  if (is_down) {
    OnMouseDown(event, destruction_receiver);
  } else {
    OnMouseUp(event, destruction_receiver);
  }
}

void MacWindow::HandleMouseMove(int32_t x, int32_t y) {
  MouseEvent event(this, MouseEvent::Button::kNone, x, y, 0, 0);
  WindowDestructionReceiver destruction_receiver(this);
  OnMouseMove(event, destruction_receiver);
}

void MacWindow::HandleMouseScroll(int32_t x, int32_t y, int32_t scroll_x, int32_t scroll_y) {
  MouseEvent event(this, MouseEvent::Button::kNone, x, y, scroll_x, scroll_y);
  WindowDestructionReceiver destruction_receiver(this);
  OnMouseWheel(event, destruction_receiver);
}

void MacWindow::HandleKeyDown(VirtualKey vk, uint32_t key_char, bool shift, bool ctrl, bool alt,
                              bool super_key) {
  KeyEvent event(this, vk, 1, false, shift, ctrl, alt, super_key);
  WindowDestructionReceiver destruction_receiver(this);
  OnKeyDown(event, destruction_receiver);
  if (destruction_receiver.IsWindowDestroyedOrClosed()) {
    return;
  }
  if (key_char > 0) {
    KeyEvent char_event(this, VirtualKey(key_char), 1, false, shift, ctrl, alt, super_key);
    OnKeyChar(char_event, destruction_receiver);
  }
}

void MacWindow::HandleKeyUp(VirtualKey vk, bool shift, bool ctrl, bool alt, bool super_key) {
  KeyEvent event(this, vk, 1, true, shift, ctrl, alt, super_key);
  WindowDestructionReceiver destruction_receiver(this);
  OnKeyUp(event, destruction_receiver);
}

void MacWindow::ApplyNewFullscreen() {
  if (!ns_window_) {
    return;
  }
  bool currently_fullscreen = ([ns_window_ styleMask] & NSWindowStyleMaskFullScreen) != 0;
  if (IsFullscreen() != currently_fullscreen) {
    [ns_window_ toggleFullScreen:nil];
  }
}

void MacWindow::ApplyNewTitle() {
  if (ns_window_) {
    [ns_window_ setTitle:[NSString stringWithUTF8String:GetTitle().c_str()]];
  }
}

std::unique_ptr<Surface> MacWindow::CreateSurfaceImpl(Surface::TypeFlags allowed_types) {
  if (!(allowed_types & Surface::kTypeFlag_CAMetalLayer)) {
    return nullptr;
  }
  if (!metal_view_) {
    REXLOG_ERROR("MacWindow: Metal view is not ready");
    return nullptr;
  }
  CAMetalLayer* layer = reinterpret_cast<CAMetalLayer*>([metal_view_ layer]);
  if (!layer) {
    REXLOG_ERROR("MacWindow: CAMetalLayer is not available");
    return nullptr;
  }
  NSSize backing = [metal_view_ convertSizeToBacking:[metal_view_ bounds].size];
  layer.drawableSize = CGSizeMake(backing.width, backing.height);
  return std::make_unique<CAMetalLayerSurface>(layer);
}

void MacWindow::RequestPaintImpl() {
  if (!metal_view_) {
    return;
  }
  if (REXCVAR_QUERY(bool, metal_present_probe)) {
    static uint32_t request_paint_probe_count = 0;
    if (request_paint_probe_count < 16) {
      ++request_paint_probe_count;
      REXLOG_WARN("MacWindow::RequestPaintImpl view={}", fmt::ptr(metal_view_));
    }
  }
  RexMetalView* view = metal_view_;
  MacWindow* window = this;
  dispatch_async(dispatch_get_main_queue(), ^{
    window->HandlePaint();
    [view setNeedsDisplay:YES];
  });
}

uint32_t MacWindow::GetLatestDpiImpl() const {
  if (!ns_window_) {
    return GetMediumDpi();
  }
  return static_cast<uint32_t>([ns_window_ backingScaleFactor] * GetMediumDpi());
}

void* MacWindow::GetNativeWindowHandle() const {
  return (void*)ns_window_;
}

std::unique_ptr<ui::MenuItem> MenuItem::Create(Type type, const std::string& text,
                                               const std::string& hotkey,
                                               std::function<void()> callback) {
  return std::make_unique<MacMenuItem>(type, text, hotkey, std::move(callback));
}

}  // namespace rex::ui
