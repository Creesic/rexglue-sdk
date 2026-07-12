// fm2 - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <array>
#include <filesystem>

#include <rex/rex_app.h>

#if FM2_HAS_PLUME
#include "render/video.h"
#endif

class Fm2App : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Fm2App>(new Fm2App(ctx, "fm2",
        PPCImageConfig));
  }

  void OnPreLaunchModule() override {
#if FM2_HAS_PLUME
    if (auto* w = window()) {
      if (!Video::Init(w->GetNativeWindowHandle(), 1280, 720)) {
        REXLOG_ERROR("FM2 native Plume renderer failed to initialize");
      }
    }
#endif
  }

  void OnShutdown() override {
#if FM2_HAS_PLUME
    Video::Shutdown();
#endif
  }

  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (!paths.game_data_root.empty()) {
      return;
    }

    const auto exe_dir = paths.config_path.parent_path();
    const auto cwd = std::filesystem::current_path();
    const std::array<std::filesystem::path, 5> candidates = {
        cwd / "assets",
        cwd / "FM2" / "assets",
        exe_dir / "assets",
        // FM2/out/build/<preset>/fm2.exe -> repo root is 3 levels up.
        exe_dir / ".." / ".." / ".." / "assets",
        // <repo>/FM2/out/build/<preset>/fm2.exe -> repo root is 4 levels up.
        exe_dir / ".." / ".." / ".." / ".." / "assets",
    };

    for (const auto& candidate : candidates) {
      if (std::filesystem::is_regular_file(candidate / "default.xex")) {
        paths.game_data_root = candidate;
        return;
      }
    }
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  // void OnPreSetup(rex::RuntimeConfig& config) override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostLoadXexImage() override {}
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
};
