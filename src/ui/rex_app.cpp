/**
 * @file        ui/rex_app.cpp
 * @brief       ReXApp implementation — compiled as part of the consumer executable
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <rex/rex_app.h>

#include <rex/cvar.h>
#include <rex/perf/counter.h>
#include <rex/ui/flags.h>
#include <rex/kernel/crt/heap.h>
#include <rex/filesystem.h>
#include <rex/logging/sink.h>
#include <rex/logging.h>
#include <rex/ui/overlay/audio_voices_overlay.h>
#include <rex/ui/overlay/console_overlay.h>
#include <rex/ui/overlay/debug_overlay.h>
#include <rex/ui/overlay/settings_overlay.h>
#include <rex/graphics/graphics_system.h>
#if REX_HAS_VULKAN
#include <rex/graphics/vulkan/graphics_system.h>
#endif
#if REX_HAS_D3D12
#include <rex/graphics/d3d12/graphics_system.h>
#endif
#include <rex/audio/audio_system.h>
#include <rex/audio/xma/decoder.h>
#include <rex/audio/nop/nop_audio_system.h>
#include <rex/audio/sdl/sdl_audio_system.h>
#if REX_PLATFORM_WIN32
#include <rex/audio/xaudio2/xaudio2_audio_system.h>
#include <windows.h>
#endif
#include <rex/input/input_system.h>
#include <rex/kernel/init.h>
#include <rex/system.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/ui/graphics_provider.h>
#include <rex/ui/keybinds.h>
#include <rex/version.h>

#include <fmt/format.h>
#include <imgui.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>

namespace rex {

REXCVAR_DEFINE_STRING(audio_backend, "sdl", "Audio", "Audio backend: sdl, xaudio2, nop")
    .allowed({"sdl", "xaudio2", "nop"});
REXCVAR_DEFINE_BOOL(show_fps_overlay, true, "UI", "Show compact FPS overlay on startup");

namespace {

std::string TrimCopy(const std::string& value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

std::string EscapeLayoutString(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char c : value) {
    if (c == '\\') {
      escaped += "\\\\";
    } else if (c == '\n') {
      escaped += "\\n";
    } else if (c == '\r') {
      escaped += "\\r";
    } else {
      escaped.push_back(c);
    }
  }
  return escaped;
}

std::string UnescapeLayoutString(const std::string& value) {
  std::string unescaped;
  unescaped.reserve(value.size());
  for (size_t i = 0; i < value.size(); ++i) {
    const char c = value[i];
    if (c == '\\' && i + 1 < value.size()) {
      const char next = value[++i];
      if (next == 'n') {
        unescaped.push_back('\n');
      } else if (next == 'r') {
        unescaped.push_back('\r');
      } else {
        unescaped.push_back(next);
      }
    } else {
      unescaped.push_back(c);
    }
  }
  return unescaped;
}

void ParseUintList(const std::string& value, std::unordered_set<uint32_t>& out_values) {
  std::stringstream ss(value);
  std::string token;
  while (std::getline(ss, token, ',')) {
    token = TrimCopy(token);
    if (token.empty()) {
      continue;
    }
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(token.c_str(), &end, 10);
    if (!end || *end != '\0') {
      continue;
    }
    out_values.insert(static_cast<uint32_t>(parsed));
  }
}

void ParseUintList(const std::string& value, std::vector<uint32_t>& out_values) {
  std::stringstream ss(value);
  std::string token;
  while (std::getline(ss, token, ',')) {
    token = TrimCopy(token);
    if (token.empty()) {
      continue;
    }
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(token.c_str(), &end, 10);
    if (!end || *end != '\0') {
      continue;
    }
    const uint32_t parsed_value = static_cast<uint32_t>(parsed);
    if (std::find(out_values.begin(), out_values.end(), parsed_value) == out_values.end()) {
      out_values.push_back(parsed_value);
    }
  }
}

void ParseUint64HexList(const std::string& value, std::unordered_set<uint64_t>& out_values) {
  std::stringstream ss(value);
  std::string token;
  while (std::getline(ss, token, ',')) {
    token = TrimCopy(token);
    if (token.empty()) {
      continue;
    }
    if (token.rfind("0x", 0) == 0 || token.rfind("0X", 0) == 0) {
      token = token.substr(2);
    }
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(token.c_str(), &end, 16);
    if (!end || *end != '\0') {
      continue;
    }
    out_values.insert(static_cast<uint64_t>(parsed));
  }
}

std::string JoinUintList(const std::unordered_set<uint32_t>& values) {
  std::vector<uint32_t> sorted(values.begin(), values.end());
  std::sort(sorted.begin(), sorted.end());
  std::string joined;
  for (size_t i = 0; i < sorted.size(); ++i) {
    if (i) {
      joined.push_back(',');
    }
    joined += std::to_string(sorted[i]);
  }
  return joined;
}

std::string JoinUint64HexList(const std::unordered_set<uint64_t>& values) {
  std::vector<uint64_t> sorted(values.begin(), values.end());
  std::sort(sorted.begin(), sorted.end());
  std::string joined;
  for (size_t i = 0; i < sorted.size(); ++i) {
    if (i) {
      joined.push_back(',');
    }
    joined += fmt::format("{:016X}", sorted[i]);
  }
  return joined;
}

std::string JoinUintList(const std::vector<uint32_t>& values) {
  std::string joined;
  bool first = true;
  for (uint32_t value : values) {
    if (!first) {
      joined.push_back(',');
    }
    first = false;
    joined += std::to_string(value);
  }
  return joined;
}

std::filesystem::path GetAudioVoicesLayoutPath(const std::filesystem::path& user_data_root,
                                               uint32_t title_id) {
  return user_data_root / "overlays" / "audio_voices" / fmt::format("{:08X}.cfg", title_id);
}

std::optional<ui::AudioVoicesLayoutState> LoadAudioVoicesLayoutState(
    const std::filesystem::path& user_data_root, uint32_t title_id) {
  if (user_data_root.empty() || title_id == 0) {
    return std::nullopt;
  }
  const auto path = GetAudioVoicesLayoutPath(user_data_root, title_id);
  std::ifstream file(path, std::ios::in);
  if (!file.is_open()) {
    return std::nullopt;
  }

  ui::AudioVoicesLayoutState state;
  std::string line;
  while (std::getline(file, line)) {
    line = TrimCopy(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    const size_t equals = line.find('=');
    if (equals == std::string::npos) {
      continue;
    }
    const std::string key = TrimCopy(line.substr(0, equals));
    const std::string value = TrimCopy(line.substr(equals + 1));

    if (key == "window_width") {
      state.window_width = std::strtof(value.c_str(), nullptr);
      state.has_window_size = true;
    } else if (key == "window_height") {
      state.window_height = std::strtof(value.c_str(), nullptr);
      state.has_window_size = true;
    } else if (key == "normalize_volume_panel") {
      state.normalize_volume_panel = value == "1" || value == "true";
    } else if (key == "ignore_persisted_controls") {
      state.ignore_persisted_controls = value == "1" || value == "true";
    } else if (key == "show_playing_contexts_only") {
      state.show_playing_contexts_only = value == "1" || value == "true";
    } else if (key == "meter_use_audible") {
      state.meter_use_audible = value == "1" || value == "true";
    } else if (key == "muted_contexts") {
      ParseUintList(value, state.muted_contexts);
    } else if (key == "muted_signatures") {
      ParseUint64HexList(value, state.muted_signatures);
    } else if (key == "tracked_contexts") {
      ParseUintList(value, state.tracked_volume_contexts);
    } else if (key.rfind("name_", 0) == 0) {
      const std::string index_text = key.substr(5);
      char* end = nullptr;
      const unsigned long parsed = std::strtoul(index_text.c_str(), &end, 10);
      if (!end || *end != '\0') {
        continue;
      }
      state.context_names[static_cast<uint32_t>(parsed)] = UnescapeLayoutString(value);
    } else if (key.rfind("volume_", 0) == 0) {
      const std::string index_text = key.substr(7);
      char* end = nullptr;
      const unsigned long parsed = std::strtoul(index_text.c_str(), &end, 10);
      if (!end || *end != '\0') {
        continue;
      }
      const float parsed_volume = std::strtof(value.c_str(), nullptr);
      state.context_volumes[static_cast<uint32_t>(parsed)] = std::clamp(parsed_volume, 0.0f, 1.0f);
    }
  }

  return state;
}

void SaveAudioVoicesLayoutState(const std::filesystem::path& user_data_root, uint32_t title_id,
                                const ui::AudioVoicesLayoutState& state) {
  if (user_data_root.empty() || title_id == 0) {
    return;
  }

  const auto path = GetAudioVoicesLayoutPath(user_data_root, title_id);
  std::filesystem::create_directories(path.parent_path());
  std::ofstream file(path, std::ios::out | std::ios::trunc);
  if (!file.is_open()) {
    return;
  }

  file << "# ReXGlue Audio Voices Overlay State\n";
  file << "window_width=" << state.window_width << "\n";
  file << "window_height=" << state.window_height << "\n";
  file << "normalize_volume_panel=" << (state.normalize_volume_panel ? "1" : "0") << "\n";
  file << "ignore_persisted_controls=" << (state.ignore_persisted_controls ? "1" : "0") << "\n";
  file << "show_playing_contexts_only=" << (state.show_playing_contexts_only ? "1" : "0") << "\n";
  file << "meter_use_audible=" << (state.meter_use_audible ? "1" : "0") << "\n";
  file << "muted_contexts=" << JoinUintList(state.muted_contexts) << "\n";
  file << "muted_signatures=" << JoinUint64HexList(state.muted_signatures) << "\n";
  file << "tracked_contexts=" << JoinUintList(state.tracked_volume_contexts) << "\n";

  std::vector<std::pair<uint32_t, std::string>> sorted_names(state.context_names.begin(),
                                                             state.context_names.end());
  std::sort(sorted_names.begin(), sorted_names.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  for (const auto& [index, name] : sorted_names) {
    if (name.empty()) {
      continue;
    }
    file << "name_" << index << "=" << EscapeLayoutString(name) << "\n";
  }

  std::vector<std::pair<uint32_t, float>> sorted_volumes(state.context_volumes.begin(),
                                                          state.context_volumes.end());
  std::sort(sorted_volumes.begin(), sorted_volumes.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  for (const auto& [index, volume] : sorted_volumes) {
    file << "volume_" << index << "=" << std::clamp(volume, 0.0f, 1.0f) << "\n";
  }
}

}  // namespace

// --- ReXApp ---

ReXApp::~ReXApp() = default;

ReXApp::ReXApp(ui::WindowedAppContext& ctx, std::string_view name, PPCImageInfo ppc_info,
               std::string_view usage)
    : WindowedApp(ctx, name, usage), ppc_info_(ppc_info) {}

bool ReXApp::OnInitialize() {
  if (!SetupEnvironment())
    return false;
  if (!SetupPresentation())
    return false;

  auto paths = OnFinalizePaths(resolved_defaults_, MakeResumeCallback());
  if (!paths) {
    // Async: consumer will invoke resume when ready. OnInitialize returns
    // true so the event loop keeps pumping (wizard dialogs render).
    return true;
  }

  if (!ConstructRuntime(*paths))
    return false;
  LaunchModule();
  return true;
}

bool ReXApp::SetupEnvironment() {
  auto exe_dir = rex::filesystem::GetExecutableFolder();

  std::filesystem::path game_dir;
  std::string game_data_cvar = REXCVAR_GET(game_data_root);
  if (!game_data_cvar.empty()) {
    game_dir = game_data_cvar;
  }

  // User data: cvar override, or platform user directory
  std::filesystem::path user_dir;
  std::string user_data_cvar = REXCVAR_GET(user_data_root);
  if (!user_data_cvar.empty()) {
    user_dir = user_data_cvar;
  } else {
    user_dir = rex::filesystem::GetUserFolder() / GetName();
  }

  // Update data: cvar override, or empty (opt-in)
  std::filesystem::path update_dir;
  std::string update_data_cvar = REXCVAR_GET(update_data_root);
  if (!update_data_cvar.empty()) {
    update_dir = update_data_cvar;
  }

  // Cache: cvar override, or user_dir/cache
  std::filesystem::path cache_dir;
  std::string cache_root_cvar = REXCVAR_GET(cache_root);
  if (!cache_root_cvar.empty()) {
    cache_dir = cache_root_cvar;
  } else {
    cache_dir = user_dir / "cache";
  }

  PathConfig path_config{game_dir, user_dir, update_dir, cache_dir,
                         exe_dir / (std::string(GetName()) + ".toml")};
  OnConfigurePaths(path_config);
  game_data_root_ = path_config.game_data_root;
  user_data_root_ = path_config.user_data_root;
  update_data_root_ = path_config.update_data_root;
  cache_root_ = path_config.cache_root;
  config_path_ = path_config.config_path;
  resolved_defaults_ = std::move(path_config);

  // Load config FIRST so log cvars have final values
  if (std::filesystem::exists(config_path_))
    rex::cvar::LoadConfig(config_path_);

  // Late-phase logging
  std::string log_file_cvar = REXCVAR_GET(log_file);
  std::string log_level_str = REXCVAR_GET(log_level);
  if (REXCVAR_GET(log_verbose) && log_level_str == "info")
    log_level_str = "trace";

  auto category_levels = rex::ParseCategoryLevelsFromConfig(config_path_);
  auto log_config = rex::BuildLogConfig(log_file_cvar.empty() ? nullptr : log_file_cvar.c_str(),
                                        log_level_str, category_levels);
  if (log_file_cvar.empty()) {
    log_config.app_name = std::string(GetName());
    log_config.log_dir = (exe_dir / "logs").string();
  }

  rex::InitLogging(log_config);
  rex::RegisterLogLevelCallback();

  log_sink_ = std::make_shared<rex::LogCaptureSink>();
  rex::AddSink(log_sink_);

  OnPostInitLogging();

  if (std::filesystem::exists(config_path_))
    REXLOG_INFO("Loaded config: {}", config_path_.filename().string());

  REXLOG_INFO("{} starting", GetName());
  if (!game_data_root_.empty()) {
    REXLOG_INFO("  Game directory: {}", game_data_root_.string());
  }
  if (!user_data_root_.empty()) {
    REXLOG_INFO("  User data:      {}", user_data_root_.string());
  }
  if (!update_data_root_.empty()) {
    REXLOG_INFO("  Update data:    {}", update_data_root_.string());
  }
  REXLOG_INFO("  Cache root:     {}", cache_root_.string());

  return true;
}

bool ReXApp::ConstructRuntime(const PathConfig& paths) {
  if (paths.game_data_root.empty()) {
    auto msg = std::string("--game_data_root was not provided.");
    REXLOG_ERROR("{}", msg);
    rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, msg);
    return false;
  }
  if (!std::filesystem::is_directory(paths.game_data_root)) {
    auto msg = fmt::format("--game_data_root does not exist: {}", paths.game_data_root.string());
    REXLOG_ERROR("{}", msg);
    rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, msg);
    return false;
  }

  runtime_ = std::make_unique<rex::Runtime>(paths.game_data_root, paths.user_data_root,
                                            paths.update_data_root, paths.cache_root);
  runtime_->set_app_context(&app_context());

  // Window and ImGui drawer already exist from SetupPresentation; publish them
  // to the runtime before Setup so hooks and native rendering see them.
  if (window_) {
    runtime_->set_display_window(window_.get());
  }
  if (imgui_drawer_) {
    runtime_->set_imgui_drawer(imgui_drawer_.get());
  }

  auto status = runtime_->Setup(ppc_info_, std::move(config_));
  if (XFAILED(status)) {
    REXLOG_ERROR("Runtime setup failed: {:08X}", status);
    return false;
  }

  if (window_ && runtime_->input_system()) {
    static_cast<rex::input::InputSystem*>(runtime_->input_system())->AttachWindow(window_.get());
  }

  if (ppc_info_.register_modules) {
    ppc_info_.register_modules(runtime_->kernel_state());
  }

  if (imgui_drawer_) {
    auto* input_sys = static_cast<rex::input::InputSystem*>(runtime_->input_system());
    if (input_sys) {
      input_sys->SetActiveCallback([this]() {
        if (!debug_overlay_ && !console_overlay_ && !audio_voices_overlay_ && !settings_overlay_)
          return true;
        return !imgui_drawer_->GetIO().WantCaptureMouse;
      });
    }
  }

  std::string xex_image = "game:\\default.xex";
  OnLoadXexImage(xex_image);

  // Mirrors the game:\ / d:\ -> game_data_root mapping in Runtime::SetupVfs.
  {
    constexpr std::string_view kGameDevice = "game:\\";
    constexpr std::string_view kDDevice = "d:\\";
    std::string_view tail = xex_image;
    if (tail.starts_with(kGameDevice)) {
      tail.remove_prefix(kGameDevice.size());
    } else if (tail.starts_with(kDDevice)) {
      tail.remove_prefix(kDDevice.size());
    }
    std::string host_tail{tail};
    std::replace(host_tail.begin(), host_tail.end(), '\\', '/');
    auto xex_host = paths.game_data_root / host_tail;
    if (!std::filesystem::is_regular_file(xex_host)) {
      auto msg = fmt::format("Entrypoint XEX not found: {}", xex_host.string());
      REXLOG_ERROR("{}", msg);
      rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, msg);
      return false;
    }
  }

  status = runtime_->LoadXexImage(xex_image);
  if (XFAILED(status)) {
    auto msg = fmt::format("Failed to load XEX ({}): {:08X}", xex_image, status);
    REXLOG_ERROR("{}", msg);
    rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, msg);
    return false;
  }

  OnPostLoadXexImage();

  if (ppc_info_.rexcrt_heap) {
    if (!rex::kernel::crt::InitHeap(REXCVAR_GET(rexcrt_heap_size_mb), runtime_->memory())) {
      REXLOG_ERROR("Failed to initialize rexcrt heap");
      return false;
    }
  }

  OnPostSetup();

  return true;
}

bool ReXApp::SetupPresentation() {
#if REX_HAS_D3D12
  config_.graphics = REX_GRAPHICS_BACKEND(rex::graphics::d3d12::D3D12GraphicsSystem);
#elif REX_HAS_VULKAN
  config_.graphics = REX_GRAPHICS_BACKEND(rex::graphics::vulkan::VulkanGraphicsSystem);
#endif
  const auto backend = REXCVAR_GET(audio_backend);
  REXLOG_INFO("Audio backend requested: {}", backend);
  if (backend == "nop") {
    REXLOG_INFO("Audio backend selected: nop");
    config_.audio_factory = REX_AUDIO_BACKEND(rex::audio::nop::NopAudioSystem);
  }
#if REX_PLATFORM_WIN32
  else if (backend == "xaudio2") {
    REXLOG_INFO("Audio backend selected: xaudio2");
    config_.audio_factory = REX_AUDIO_BACKEND(rex::audio::xaudio2::XAudio2AudioSystem);
  }
#endif
  else {
    REXLOG_INFO("Audio backend selected: sdl");
    config_.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
  }
  config_.input_factory = REX_INPUT_BACKEND(rex::input::CreateDefaultInputSystem);
  config_.kernel_init = rex::kernel::InitializeKernel;

  OnPreSetup(config_);

  if (config_.graphics) {
    X_STATUS status = config_.graphics->SetupPresentation(&app_context());
    if (XFAILED(status)) {
      REXLOG_ERROR("Graphics presentation setup failed: {:08X}", status);
      return false;
    }
  }

  // Create window
  window_ = rex::ui::Window::Create(app_context(), GetName(), 1280, 720);
  if (!window_) {
    REXLOG_ERROR("Failed to create window");
    return false;
  }

  // Set window title with SDK build stamp
  std::string title = std::string(GetName()) + " " + REXGLUE_BUILD_TITLE;
  window_->SetTitle(title);

  window_->AddListener(this);
  window_->AddInputListener(this, 0);

  if (REXCVAR_GET(fullscreen)) {
    window_->SetFullscreen(true);
  }
  window_->Open();

  auto* graphics_system = static_cast<rex::graphics::GraphicsSystem*>(config_.graphics.get());
  if (graphics_system && graphics_system->presenter()) {
    auto* presenter = graphics_system->presenter();
    auto* provider = graphics_system->provider();
    if (provider) {
      immediate_drawer_ = provider->CreateImmediateDrawer();
      if (immediate_drawer_) {
        immediate_drawer_->SetPresenter(presenter);
        imgui_drawer_ = std::make_unique<rex::ui::ImGuiDrawer>(
            window_.get(), 64, [this](ImFontAtlas* atlas) { OnConfigureFonts(atlas); });
        imgui_drawer_->SetPresenterAndImmediateDrawer(presenter, immediate_drawer_.get());
        rex::ui::RegisterBind("bind_debug_overlay", "F3", "Toggle debug overlay", [this] {
          if (debug_overlay_) {
            debug_overlay_.reset();
          } else {
            debug_overlay_ = std::make_unique<ui::DebugOverlayDialog>(imgui_drawer_.get(),
                                                                      frame_stats_provider_);
          }
        });
        rex::ui::RegisterBind("bind_fps_overlay", "F2", "Toggle FPS overlay", [this] {
          if (fps_overlay_) {
            fps_overlay_.reset();
          } else {
            fps_overlay_ = std::make_unique<ui::DebugOverlayDialog>(
                imgui_drawer_.get(), frame_stats_provider_, true);
          }
        });
        rex::ui::RegisterBind("bind_console", "Backtick", "Toggle console overlay", [this] {
          if (console_overlay_) {
            console_overlay_.reset();
          } else {
            console_overlay_ = std::make_unique<ui::ConsoleDialog>(imgui_drawer_.get(), log_sink_);
          }
        });
        rex::ui::RegisterBind("bind_settings", "F4", "Toggle settings overlay", [this] {
          if (settings_overlay_) {
            settings_overlay_.reset();
          } else {
            settings_overlay_ =
                std::make_unique<ui::SettingsDialog>(imgui_drawer_.get(), config_path_);
          }
        });
        rex::ui::RegisterBind("bind_audio_voices", "F5", "Toggle audio voices overlay", [this] {
          if (audio_voices_overlay_) {
            audio_voices_overlay_.reset();
          } else {
            audio_voices_overlay_ = std::make_unique<ui::AudioVoicesDialog>(
                imgui_drawer_.get(), [this]() -> ui::AudioVoicesSnapshot {
                  ui::AudioVoicesSnapshot out;
                  if (runtime_ && runtime_->audio_system()) {
                    auto* audio_system = dynamic_cast<audio::AudioSystem*>(runtime_->audio_system());
                    if (audio_system) {
                      auto snapshot = audio_system->GetDebugSnapshot();
                      out.valid = true;
                      out.paused = snapshot.paused;
                      out.queued_frames = snapshot.queued_frames;
                      out.render_callbacks_total = snapshot.render_callbacks_total;
                      for (size_t i = 0; i < audio::AudioSystem::kMaximumClientCount; ++i) {
                        const auto& client = snapshot.clients[i];
                        if (!client.in_use) {
                          continue;
                        }
                        out.voices.push_back(ui::AudioVoiceInfo{
                            static_cast<uint32_t>(i), client.driver_handle, client.callback,
                            client.callback_arg, client.submitted_frames});
                      }

                      if (auto* xma_decoder = audio_system->xma_decoder()) {
                        auto xma_snapshot = xma_decoder->GetDebugSnapshot();
                        out.xma_paused = xma_snapshot.paused;
                        out.vp.total_worker_time_us = xma_snapshot.vp.total_worker_time_us;
                        out.vp.sweeps_per_second = xma_snapshot.vp.sweeps_per_second;
                        out.vp.decode_iterations_per_second =
                            xma_snapshot.vp.decode_iterations_per_second;
                        out.vp.workers[0].num_voices = xma_snapshot.vp.workers[0].num_voices;
                        out.vp.workers[0].time_us = xma_snapshot.vp.workers[0].time_us;
                        out.gp.cycles = xma_snapshot.gp.cycles;
                        out.ep.cycles = xma_snapshot.ep.cycles;
                        out.xma_contexts.reserve(audio::XmaDecoder::kContextCount);
                        for (size_t i = 0; i < audio::XmaDecoder::kContextCount; ++i) {
                          const auto& in = xma_snapshot.contexts[i];
                          out.xma_contexts.push_back(ui::AudioXmaContextInfo{
                              static_cast<uint32_t>(i), in.allocated, in.enabled, in.input0_valid,
                              in.input1_valid, in.output_valid, in.stop_when_done,
                              in.interrupt_when_done, in.consume_only, in.stereo, in.muted, in.volume,
                              in.peak_level, in.rms_level, in.audible_peak_level,
                              in.audible_rms_level, in.current_buffer,
                              in.subframe_decode_count, in.output_buffer_block_count,
                              in.output_buffer_write_offset, in.output_buffer_read_offset,
                              in.sample_rate_id, in.loop_count, in.output_buffer_padding,
                              in.loop_subframe_start, in.loop_subframe_end,
                              in.loop_subframe_skip, in.packet_metadata,
                              in.input_buffer_0_packet_count, in.input_buffer_1_packet_count,
                              in.guest_ptr,
                              in.input_buffer_read_offset, in.input_buffer_0_ptr,
                              in.input_buffer_1_ptr, in.output_buffer_ptr});
                        }
                      }
                    }
                  }
                  return out;
                },
                [this](uint32_t context_id, bool muted) {
                  if (!runtime_ || !runtime_->audio_system()) {
                    return;
                  }
                  auto* audio_system = dynamic_cast<audio::AudioSystem*>(runtime_->audio_system());
                  if (!audio_system || !audio_system->xma_decoder()) {
                    return;
                  }
                  audio_system->xma_decoder()->SetContextMuted(context_id, muted);
                },
                [this](uint32_t context_id, float volume) {
                  if (!runtime_ || !runtime_->audio_system()) {
                    return;
                  }
                  auto* audio_system = dynamic_cast<audio::AudioSystem*>(runtime_->audio_system());
                  if (!audio_system || !audio_system->xma_decoder()) {
                    return;
                  }
                  audio_system->xma_decoder()->SetContextVolume(context_id, volume);
                },
                [this]() -> std::optional<ui::AudioVoicesLayoutState> {
                  if (!runtime_ || !runtime_->kernel_state()) {
                    return std::nullopt;
                  }
                  return LoadAudioVoicesLayoutState(user_data_root_, runtime_->kernel_state()->title_id());
                },
                [this](const ui::AudioVoicesLayoutState& state) {
                  if (!runtime_ || !runtime_->kernel_state()) {
                    return;
                  }
                  SaveAudioVoicesLayoutState(user_data_root_, runtime_->kernel_state()->title_id(),
                                             state);
                });
          }
        });
        if (REXCVAR_GET(show_fps_overlay)) {
          fps_overlay_ =
              std::make_unique<ui::DebugOverlayDialog>(imgui_drawer_.get(), frame_stats_provider_, true);
        }

        OnCreateDialogs(imgui_drawer_.get());
      }
    }
    window_->SetPresenter(presenter);
  }

  return true;
}

void ReXApp::LaunchModule() {
  app_context().CallInUIThreadDeferred([this]() {
    OnPreLaunchModule();

    auto main_thread = runtime_->PrepareModuleLaunch();
    if (!main_thread) {
      REXLOG_ERROR("Failed to launch module");
      app_context().QuitFromUIThread();
      return;
    }

    auto* graphics_system =
        static_cast<rex::graphics::GraphicsSystem*>(runtime_->graphics_system());
    if (graphics_system && !runtime_->cache_root().empty()) {
      uint32_t title_id = runtime_->kernel_state()->title_id();
      if (title_id != 0) {
        REXLOG_INFO("Initializing shader storage for title {:08X}...", title_id);
        graphics_system->InitializeShaderStorage(runtime_->cache_root(), title_id, true);
      }
    }

    OnPostLaunchModule(main_thread.get());
    main_thread->Resume();

    module_thread_ = std::thread([this, main_thread = std::move(main_thread)]() mutable {
      main_thread->Wait(0, 0, 0, nullptr);
      OnGuestThreadExit(main_thread.get());
      REXLOG_INFO("Execution complete");
      if (!shutting_down_.load(std::memory_order_acquire)) {
        app_context().CallInUIThread([this]() { app_context().QuitFromUIThread(); });
      }
    });
  });
}

std::function<void(PathConfig)> ReXApp::MakeResumeCallback() {
  return [this](PathConfig paths) {
    if (shutting_down_.load(std::memory_order_acquire))
      return;
    if (!ConstructRuntime(std::move(paths))) {
      app_context().QuitFromUIThread();
      return;
    }
    LaunchModule();
  };
}

void ReXApp::OnKeyDown(ui::KeyEvent& e) {
  rex::ui::ProcessKeyEvent(e);
}

void ReXApp::OnClosing(ui::UIEvent& e) {
  (void)e;
  REXLOG_INFO("Window closing, shutting down...");
  shutting_down_.store(true, std::memory_order_release);
  if (runtime_ && runtime_->kernel_state()) {
    runtime_->kernel_state()->TerminateTitle();
  }
  app_context().QuitFromUIThread();
#if REX_PLATFORM_WIN32
  ExitProcess(0);
#endif
}

void ReXApp::OnDestroy() {
  // Notify subclass before cleanup
  OnShutdown();

  // Unregister overlay keybinds before destroying dialogs
  rex::ui::UnregisterBind("bind_debug_overlay");
  rex::ui::UnregisterBind("bind_fps_overlay");
  rex::ui::UnregisterBind("bind_console");
  rex::ui::UnregisterBind("bind_settings");
  rex::ui::UnregisterBind("bind_audio_voices");

  // ImGui cleanup (reverse of setup)
  settings_overlay_.reset();
  audio_voices_overlay_.reset();
  console_overlay_.reset();
  fps_overlay_.reset();
  debug_overlay_.reset();
  if (imgui_drawer_) {
    imgui_drawer_->SetPresenterAndImmediateDrawer(nullptr, nullptr);
    imgui_drawer_.reset();
  }
  if (immediate_drawer_) {
    immediate_drawer_->SetPresenter(nullptr);
    immediate_drawer_.reset();
  }
  if (runtime_) {
    runtime_->set_display_window(nullptr);
    runtime_->set_imgui_drawer(nullptr);
  }
  // Window/runtime cleanup
  if (window_) {
    window_->SetPresenter(nullptr);
  }
  if (module_thread_.joinable()) {
    module_thread_.join();
  }
  if (window_) {
    window_->RemoveInputListener(this);
    window_->RemoveListener(this);
  }
  window_.reset();
  runtime_.reset();
}

void ReXApp::SetGuestFrameStats(ui::DebugOverlayDialog::FrameStatsProvider provider) {
  frame_stats_provider_ = provider;
  if (debug_overlay_) {
    debug_overlay_->SetStatsProvider(provider);
  }
  if (fps_overlay_) {
    fps_overlay_->SetStatsProvider(provider);
  }
}

}  // namespace rex
