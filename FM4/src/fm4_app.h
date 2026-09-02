// fm4 - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <array>
#include <filesystem>

#include <rex/rex_app.h>

#include "gpu/native_video.h"
#include <rex/logging.h>

class Fm4App : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Fm4App>(new Fm4App(ctx, "fm4",
        PPCImageConfig));
  }

  // gpu_plugin = "native" in fm4.toml: run without an SDK GPU plugin
  // ("detached mode", rex_app.h) and let FM4 own the swapchain via Plume.
  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (config.gpu_plugin == "native") {
      config.gpu_plugin.clear();  // skips LoadGpuPlugin in ReXApp::SetupPresentation
      fm4::gpu::SetNativeRequested(true);
      REXLOG_INFO("fm4: native GPU path selected");
    }
  }

  void OnPostSetup() override {
    if (!fm4::gpu::NativeRequested()) {
      return;
    }
    REXLOG_INFO("fm4: window at OnPostSetup: {}", window() ? "present" : "null");
    if (!fm4::gpu::Video::Init(window())) {
      REXLOG_ERROR("fm4: native GPU init failed; running headless (guest waits are still neutralised, Present is a no-op)");
    }
  }

  void OnShutdown() override { fm4::gpu::Video::Shutdown(); }

  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (!paths.game_data_root.empty()) {
      return;
    }

    const auto exe_dir = paths.config_path.parent_path();
    const auto cwd = std::filesystem::current_path();
    const std::array<std::filesystem::path, 4> candidates = {
        cwd / "assets",
        cwd / "FM4" / "assets",
        exe_dir / "assets",
        exe_dir / ".." / ".." / ".." / "assets",
    };

    for (const auto& candidate : candidates) {
      if (std::filesystem::is_regular_file(candidate / "default.xex")) {
        paths.game_data_root = candidate;
        return;
      }
    }
  }
};
