
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

#if FM2_HAS_PLUME
#include "native_renderer/fm2_native_renderer.h"
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

  void OnPreSetup(rex::RuntimeConfig& config) override {
#if REX_PLATFORM_WIN32
    config.audio_factory = REX_AUDIO_BACKEND(rex::audio::xaudio2::XAudio2AudioSystem);
#else
    config.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
#endif
#if FM2_HAS_PLUME
    // ReOdyssey-style Plume ownership only applies to Plume-only mode.
    // Shadow/xenos modes still need the legacy ReX graphics backend alive.
    if (!fm2::native_renderer::WantsReXGraphics()) {
      config.graphics.reset();
    }
    config.mount_cache_root = true;
#endif
  }

  void OnPreLaunchModule() override {
#if FM2_HAS_PLUME
    if (auto* w = window()) {
      if (fm2::native_renderer::WantsReXGraphics()) {
        if (!fm2::native_renderer::Initialize(w)) {
          REXLOG_ERROR("FM2 native Plume renderer failed to initialize");
        }
      } else if (!Video::Init(w->GetNativeWindowHandle(), 1280, 720)) {
        REXLOG_ERROR("FM2 native Plume renderer failed to initialize");
      }
    }
#endif
  }

  void OnShutdown() override {
#if FM2_HAS_PLUME
    if (fm2::native_renderer::WantsReXGraphics()) {
      fm2::native_renderer::Shutdown();
    } else {
      Video::Shutdown();
    }
#endif
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
