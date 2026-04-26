#pragma once
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay & Rien Gupta, 2026 - macOS Cocoa run-loop context for ReXGlue
 *
 * MacWindowedAppContext — Cocoa run-loop implementation of WindowedAppContext.
 *
 * Each platform has exactly one WindowedAppContext subclass that owns the UI
 * run loop and bridges the ReXGlue pending-function queue into the platform's
 * own event dispatch mechanism:
 *
 *   Windows → Win32WindowedAppContext  (hidden HWND + WM_APP)
 *   Linux   → GTKWindowedAppContext    (gdk_threads_add_idle)
 *   macOS   → MacWindowedAppContext    (dispatch_async main queue)   ← this
 *
 * Pending-function bridging
 * ─────────────────────────
 * ReXGlue non-UI threads call CallInUIThreadDeferred() which pushes into the
 * pending-function queue and then calls NotifyUILoopOfPendingFunctions().
 * Our implementation posts a dispatch_async block to the main queue which
 * calls ExecutePendingFunctionsFromUIThread().  A mutex-guarded flag prevents
 * flooding the main queue with duplicate blocks.
 *
 * Quit
 * ────
 * PlatformQuitFromUIThread() calls [NSApp stop:nil] and posts a synthetic
 * NSEventTypeApplicationDefined event so the run loop unblocks immediately.
 *
 * Usage
 * ─────
 *   MacWindowedAppContext ctx;
 *   // ... create windows, initialise app ...
 *   ctx.RunMainLoop();    // blocks until QuitFromUIThread() is called
 *   // ... app teardown ...
 */

#include <mutex>

#include <rex/ui/windowed_app_context.h>

namespace rex {
namespace ui {

class MacWindowedAppContext final : public WindowedAppContext {
 public:
  MacWindowedAppContext() = default;
  ~MacWindowedAppContext() override;

  // Calls [NSApp run].  Blocks until QuitFromUIThread() is called (from a
  // window-close handler or any other shutdown path).
  void RunMainLoop();

 protected:
  // Posts a one-shot dispatch_async block to the main queue that will drain
  // the pending-function queue.  Idempotent: at most one block in flight.
  void NotifyUILoopOfPendingFunctions() override;

  // Calls [NSApp stop:nil] and posts a dummy event to unblock the run loop
  // immediately rather than waiting for the next real user-input event.
  void PlatformQuitFromUIThread() override;

 private:
  // Prevents redundant dispatch_async blocks from being queued when the
  // pending-function queue is already being serviced.
  std::mutex notify_mutex_;
  bool notify_pending_ = false;
};

}  // namespace ui
}  // namespace rex
