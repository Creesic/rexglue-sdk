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

#include <mutex>

#include <rex/ui/windowed_app_context.h>

namespace rex::ui {

class MacWindowedAppContext final : public WindowedAppContext {
 public:
  MacWindowedAppContext() = default;
  ~MacWindowedAppContext() override;

  void RunMainLoop();

 protected:
  void NotifyUILoopOfPendingFunctions() override;
  void PlatformQuitFromUIThread() override;

 private:
  std::mutex notify_mutex_;
  bool notify_pending_ = false;
};

}  // namespace rex::ui
