// pgr4_recompiled - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/rex_app.h>

#if PGR4_ENABLE_PLUME
#include <memory>

#include "render/guest_gpu.h"
#include "render/video.h"
#endif

class Pgr4RecompiledApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Pgr4RecompiledApp>(new Pgr4RecompiledApp(ctx, "pgr4_recompiled",
        PPCImageConfig));
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  // PGR4 needs three settings that would otherwise have to be passed on every
  // launch. Baked in here so the exe runs standalone. game_data_root and
  // gpu_plugin are only defaulted when unset, so a command-line value still wins.

  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (paths.game_data_root.empty()) {
      paths.game_data_root = R"(D:\Emulation\Games_Xbox_360\PGR4\Extracted)";
    }
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
#if PGR4_ENABLE_PLUME
    // The native renderer replaces the plugin outright: src/render/d3d_hooks.cpp
    // has already taken over D3DDevice_Swap at link time, so loading xenos here
    // would leave two things believing they own presentation. Leaving the plugin
    // name empty is what makes LoadGpuPlugin a no-op.
    config.gpu_plugin.clear();

    // ...but clearing it also leaves graphics_system() null, and the xboxkrnl
    // Vd* exports return early when it is -- silently dropping the guest's
    // graphics interrupt callback, so its frame loop never advances and
    // D3DDevice_Swap is never reached. Supplying our own IGraphicsSystem keeps
    // that surface alive without reloading xenos; ReXApp only loads a plugin
    // when this field is still null.
    config.graphics = std::make_unique<pgr4::render::Pgr4GraphicsSystem>();
#else
    if (config.gpu_plugin.empty()) {
      config.gpu_plugin = "xenos";
    }
#endif
  }

#if PGR4_ENABLE_PLUME
  void OnPreLaunchModule() override {
    // Runs after the window exists but before guest code executes, so the
    // swapchain is live before the first D3DDevice_Swap can arrive.
    if (auto* w = window()) {
      if (!Video::Init(w->GetNativeWindowHandle(), Video::s_viewportWidth,
                       Video::s_viewportHeight)) {
        REXLOG_ERROR("PGR4 native Plume renderer failed to initialize; nothing will be presented");
      }
    } else {
      REXLOG_ERROR("PGR4 native Plume renderer: no window at OnPreLaunchModule");
    }
  }
#endif

#if !PGR4_ENABLE_PLUME
  void OnPostSetup() override {
    // execute_unclipped_draw_vs_on_cpu is defined inside the GPU plugin DLL, so
    // it is not registered until LoadGpuPlugin runs. OnPostSetup is the first
    // hook after that, hence set by name rather than REXCVAR_SET (the exe does
    // not link the plugin). Unconditional: a bool cvar left at its false default
    // is indistinguishable from an explicit --execute_unclipped_draw_vs_on_cpu=false.
    if (!rex::cvar::SetFlagByName("execute_unclipped_draw_vs_on_cpu", "true")) {
      REXLOG_WARN("Could not set execute_unclipped_draw_vs_on_cpu; PGR4 may render incorrectly");
    }
  }
#endif  // !PGR4_ENABLE_PLUME -- that cvar lives in the xenos DLL, which the
        // native renderer never loads; setting it there only logs a false warning.

  // void OnPreSetup(rex::RuntimeConfig& config) override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostLoadXexImage() override {}
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  std::unique_ptr<rex::ui::AchievementNotificationDialog>
  CreateAchievementNotificationDialog() override {
    // Constructing this registers a permanent UI drawer at startup, so
    // ui_drawers_ is never empty and Presenter::GetDesiredPaintModeFromUIThread
    // always returns kUIThreadOnRequest - the paced path - instead of ever
    // reaching kGuestOutputThreadImmediately. Returning nullptr keeps the
    // fast path reachable.
    return nullptr;
  }
#if PGR4_ENABLE_PLUME
  void OnShutdown() override {
    // Drains the queue before tearing down device objects; safe to call even
    // if Init() failed, since Shutdown() is written to tolerate null state.
    Video::Shutdown();
  }
#endif
  // void OnConfigurePaths(rex::PathConfig& paths) override {}
};
