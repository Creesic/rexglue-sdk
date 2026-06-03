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

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include <dispatch/dispatch.h>

#include <rex/ui/windowed_app_context_mac.h>

namespace rex::ui {

MacWindowedAppContext::~MacWindowedAppContext() = default;

void MacWindowedAppContext::RunMainLoop() {
  if (HasQuitFromUIThread()) {
    return;
  }
  [NSApp run];
  QuitFromUIThread();
}

void MacWindowedAppContext::NotifyUILoopOfPendingFunctions() {
  {
    std::lock_guard<std::mutex> lock(notify_mutex_);
    if (notify_pending_) {
      return;
    }
    notify_pending_ = true;
  }

  MacWindowedAppContext* context = this;
  dispatch_async(dispatch_get_main_queue(), ^{
    {
      std::lock_guard<std::mutex> lock(context->notify_mutex_);
      context->notify_pending_ = false;
    }
    context->ExecutePendingFunctionsFromUIThread();
  });
}

void MacWindowedAppContext::PlatformQuitFromUIThread() {
  [NSApp stop:nil];
  NSEvent* wakeup = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                       location:NSZeroPoint
                                  modifierFlags:0
                                      timestamp:0.0
                                   windowNumber:0
                                        context:nil
                                        subtype:0
                                          data1:0
                                          data2:0];
  [NSApp postEvent:wakeup atStart:YES];
}

}  // namespace rex::ui
