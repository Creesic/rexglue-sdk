
// fm2 - ReXGlue Recompiled Project
//
// This file is yours to edit. 'rexglue migrate' will NOT overwrite it.
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <array>
#include <cstdlib>
#include <filesystem>

#include <rex/audio/nop/nop_audio_system.h>
#include <rex/audio/sdl/sdl_audio_system.h>
#if REX_PLATFORM_WIN32
#include <rex/audio/xaudio2/xaudio2_audio_system.h>
#endif
#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/system.h>

#include "native_renderer/fm2_native_renderer.h"

class Fm2App : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Fm2App>(new Fm2App(ctx, "fm2",
        PPCImageConfig));
  }

  // Override virtual hooks for customization:
  void OnPreSetup(rex::RuntimeConfig& config) override {
#if REX_PLATFORM_WIN32
    config.audio_factory = REX_AUDIO_BACKEND(rex::audio::xaudio2::XAudio2AudioSystem);
#else
    config.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
#endif
    if (!fm2::native_renderer::WantsReXGraphics()) {
      config.graphics.reset();
    }
  }
  // void OnLoadXexImage(std::string& xex_image) override {}
  void OnPostSetup() override {
    const auto mode = fm2::native_renderer::GetMode();
    if (!fm2::native_renderer::Initialize(window())) {
      if (mode == fm2::native_renderer::Mode::kPlumeClear) {
        constexpr const char* kMessage =
            "FM2 Plume clear mode failed to initialize; startup cannot "
            "continue because ReXGlue graphics is disabled.";
        REXLOG_ERROR("{}", kMessage);
        rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, kMessage);
        std::exit(EXIT_FAILURE);
      }
      REXLOG_WARN(
          "FM2 Plume native renderer failed to initialize in mode={}; "
          "continuing with normal startup",
          fm2::native_renderer::GetModeName(mode));
    }
  }
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  void OnShutdown() override {
    fm2::native_renderer::Shutdown();
  }
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (!paths.game_data_root.empty()) {
      return;
    }

    const auto exe_dir = paths.config_path.parent_path();
    const auto cwd = std::filesystem::current_path();
    const std::array<std::filesystem::path, 4> candidates = {
        cwd / "assets",
        cwd / "FM2" / "assets",
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
