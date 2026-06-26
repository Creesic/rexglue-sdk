// doax - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <cstdlib>
#include <filesystem>
#include <memory>

#include <rex/filesystem.h>
#include <rex/rex_app.h>
#include <rex/ui/window_listener.h>

#include "doax_hooks.h"

class DoaxApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<DoaxApp>(new DoaxApp(ctx, "doax",
        PPCImageConfig));
  }

  void OnPostSetup() override {
    InstallDoaxGuestPcFiber(PPCImageConfig);
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    rex::ReXApp::OnCreateDialogs(drawer);
    // Registered after ReXApp's listener so TerminateTitle runs first.
    if (window() && !close_exit_listener_) {
      close_exit_listener_ = std::make_unique<ForceProcessExitListener>();
      window()->AddListener(close_exit_listener_.get());
    }
  }

  // Portable game root: DOAX/assets relative to doax.exe in out/build/<preset>/.
  // Survives git checkouts and broken .vs/launch.vs.json (that file is gitignored).
  void OnConfigurePaths(rex::PathConfig& paths) override {
    const auto has_assets = [](const std::filesystem::path& root) {
      return !root.empty() && std::filesystem::is_directory(root) &&
             std::filesystem::exists(root / "default.xex");
    };

    if (has_assets(paths.game_data_root)) {
      return;
    }

    const auto exe_dir = rex::filesystem::GetExecutableFolder();
    const std::filesystem::path candidates[] = {
        exe_dir / ".." / ".." / ".." / "assets",
        exe_dir / "assets",
    };
    for (const auto& candidate : candidates) {
      std::error_code ec;
      const auto resolved = std::filesystem::weakly_canonical(candidate, ec);
      if (ec || !has_assets(resolved)) {
        continue;
      }
      paths.game_data_root = resolved;
      return;
    }
  }

 private:
  // DOAX guest-PC fiber swap can leave module_thread_->Wait blocked after
  // TerminateTitle; exit the process once the window close path has run.
  class ForceProcessExitListener final : public rex::ui::WindowListener {
   public:
    void OnClosing(rex::ui::UIEvent& e) override {
      (void)e;
      std::exit(EXIT_SUCCESS);
    }
  };

  std::unique_ptr<ForceProcessExitListener> close_exit_listener_;
};
