/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Grien Gupta, 2026 - macOS Cocoa entry point for ReXGlue
 *
 * This file is the macOS counterpart to windowed_app_main_win.cpp (Win32) and
 * windowed_app_main_posix.cpp (Linux/GTK).  It is NOT compiled into rexui.a —
 * instead, rexglue_configure_target() injects it into each consumer executable
 * via target_sources(), the same way the Win32 and POSIX entry points work.
 *
 * Startup sequence
 * ────────────────
 *  1. [NSApplication sharedApplication] — must be called first on the main
 *     thread before any AppKit objects are created or any Cocoa framework is
 *     touched.  Setting NSApplicationActivationPolicyRegular ensures the app
 *     appears in the Dock and can receive keyboard focus.
 *
 *  2. rex::cvar::Init — parse argv, populating console variables.
 *     rex::InitLoggingEarly — bring up spdlog before any ReXGlue subsystem
 *     tries to log.
 *
 *  3. MacWindowedAppContext — the Cocoa run-loop adapter (see
 *     windowed_app_context_mac.mm).  Constructed on the stack so its lifetime
 *     brackets the entire app session.
 *
 *  4. GetWindowedAppCreator()(app_context) — instantiate the application
 *     object registered by the SDK consumer (e.g. the emulator front-end).
 *
 *  5. app->OnInitialize() — let the app open its window, load assets, etc.
 *     If this returns false, we skip the run loop and exit with failure.
 *
 *  6. [NSApp activateIgnoringOtherApps:YES] — bring the window to the front.
 *     app_context.RunMainLoop() — spin [NSApp run] until the window closes.
 *
 *  7. app->InvokeOnDestroy() — orderly shutdown (saves config, releases GPU
 *     resources, etc.) while logging is still alive.
 *
 *  8. rex::ShutdownLogging() — flush and tear down spdlog.
 *
 * Positional arguments
 * ────────────────────
 * The app may declare positional options (e.g. "target_path").  Any leftover
 * argv tokens that cvar::Init did not consume are matched positionally to
 * those names and forwarded as a string map via SetParsedArguments().
 */

#import <AppKit/AppKit.h>

#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/windowed_app.h>
#include <rex/ui/windowed_app_context_mac.h>

int main(int argc, char** argv) {
  // ── 1. AppKit initialisation ────────────────────────────────────────────────
  // Must happen before any Cocoa objects are created.
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

  // ── 2. ReXGlue init (cvars + early logging) ─────────────────────────────────
  auto remaining = rex::cvar::Init(argc, argv);
  rex::cvar::ApplyEnvironment();
  rex::InitLoggingEarly();

  int result;

  {
    // ── 3. Run-loop context (RAII — destructor drains pending functions) ───────
    rex::ui::MacWindowedAppContext app_context;

    // ── 4. Application object ─────────────────────────────────────────────────
    std::unique_ptr<rex::ui::WindowedApp> app =
        rex::ui::GetWindowedAppCreator()(app_context);

    // ── 5. Positional argument mapping ────────────────────────────────────────
    // cvar::Init strips --flag=value tokens; anything left is positional.
    const auto& option_names = app->GetPositionalOptions();
    std::map<std::string, std::string> parsed;
    size_t count = std::min(remaining.size(), option_names.size());
    for (size_t i = 0; i < count; ++i) {
      parsed[option_names[i]] = remaining[i];
    }
    app->SetParsedArguments(std::move(parsed));

    // ── 6. Main loop ──────────────────────────────────────────────────────────
    if (app->OnInitialize()) {
      // Bring the window to the foreground before spinning the run loop.
      [NSApp activateIgnoringOtherApps:YES];
      app_context.RunMainLoop();
      result = EXIT_SUCCESS;
    } else {
      result = EXIT_FAILURE;
    }

    // ── 7. Shutdown ───────────────────────────────────────────────────────────
    // InvokeOnDestroy() must be called while logging and the run-loop context
    // are still alive so that any shutdown logging works correctly.
    app->InvokeOnDestroy();
  }  // MacWindowedAppContext destructor runs here

  // ── 8. Logging teardown ───────────────────────────────────────────────────
  rex::ShutdownLogging();
  return result;
}
