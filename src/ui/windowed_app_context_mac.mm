/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Grien Gupta, 2026 - macOS Cocoa run-loop context for ReXGlue
 *
 * MacWindowedAppContext adapts rex::ui::WindowedAppContext to Cocoa's NSRunLoop.
 *
 * Threading model
 * ───────────────
 * The Cocoa run loop ([NSApp run]) and all UI callbacks execute on the main
 * thread.  ReXGlue's pending-function queue lets non-UI threads post work back
 * to the UI thread; the two virtual methods below bridge that queue into
 * Cocoa's dispatch / event mechanisms.
 *
 * NotifyUILoopOfPendingFunctions
 * ──────────────────────────────
 * Called from any thread when something is added to the pending-function queue.
 * We dispatch a block to the main queue that drains the queue by calling
 * ExecutePendingFunctionsFromUIThread().  A mutex-guarded flag (notify_pending_)
 * prevents flooding the main queue with redundant dispatches — the same pattern
 * GTK uses with gdk_threads_add_idle / a pending_functions_idle_pending_ guard.
 *
 * PlatformQuitFromUIThread
 * ────────────────────────
 * Called on the UI thread when the app requests shutdown.  [NSApp stop:nil]
 * schedules a quit but does not take effect until the run loop processes the
 * next event; we therefore post a synthetic NSEventTypeApplicationDefined
 * event so the run loop unblocks immediately rather than waiting for user input.
 *
 * RunMainLoop
 * ───────────
 * Simply calls [NSApp run].  All window and input callbacks fire from inside
 * this call via Cocoa's normal event delivery to NSWindow / NSView delegates.
 */

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include <rex/ui/windowed_app_context_mac.h>

namespace rex {
namespace ui {

MacWindowedAppContext::~MacWindowedAppContext() {
  // The common WindowedAppContext destructor calls
  // ExecutePendingFunctionsFromUIThread(is_final=true) via a non-virtual path.
  // We don't need extra work here, but logging / ARC object teardown must
  // complete before the base class destructor runs, so keep this body present.
}

void MacWindowedAppContext::RunMainLoop() {
  // Guard against the edge case where QuitFromUIThread() was called before
  // RunMainLoop() returned (e.g. the app failed to initialise and called quit
  // from OnInitialize).
  if (HasQuitFromUIThread()) {
    return;
  }
  [NSApp run];
  // [NSApp run] may return if something outside our code calls gtk_main_quit
  // or an equivalent.  Ensure the context knows about the quit so no new
  // pending functions are enqueued pointlessly after this point.
  QuitFromUIThread();
}

void MacWindowedAppContext::NotifyUILoopOfPendingFunctions() {
  // Only dispatch one block at a time.  The block resets the flag before
  // draining the queue so that any notifications posted *during* draining
  // will schedule a new block rather than being silently dropped.
  {
    std::lock_guard<std::mutex> lock(notify_mutex_);
    if (notify_pending_) {
      return;
    }
    notify_pending_ = true;
  }

  // Capture raw pointer — safe because MacWindowedAppContext outlives all
  // dispatched blocks (the destructor's base-class path drains remaining
  // functions before the object is freed).
  MacWindowedAppContext* ctx = this;
  dispatch_async(dispatch_get_main_queue(), ^{
    // Reset the flag first so notifications that arrive while we're executing
    // pending functions will correctly enqueue a new block.
    {
      std::lock_guard<std::mutex> lock(ctx->notify_mutex_);
      ctx->notify_pending_ = false;
    }
    ctx->ExecutePendingFunctionsFromUIThread();
  });
}

void MacWindowedAppContext::PlatformQuitFromUIThread() {
  // [NSApp stop:] sets an internal flag but does not immediately interrupt the
  // run loop — it only takes effect after the *next* event is processed.
  // Posting a dummy application-defined event forces an immediate wakeup so
  // RunMainLoop() returns without waiting for real user input.
  [NSApp stop:nil];
  NSEvent* wakeup =
      [NSEvent otherEventWithType:NSEventTypeApplicationDefined
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

}  // namespace ui
}  // namespace rex
