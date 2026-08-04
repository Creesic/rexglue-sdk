/**
 * @file        ui/windowed_app_main_sdl.cpp
 * @brief       Entry point for windowed applications (SDL3 windowing)
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/platform.h>
#include <rex/ui/windowed_app.h>
#include <rex/ui/windowed_app_context_sdl.h>

#if REX_PLATFORM_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <winternl.h>
#endif

namespace {

#if REX_PLATFORM_WIN32
void RequestWin32HighResolutionTimer();
void RequestWin32MMCSS();
#endif

int RunWindowedApp(int argc, char** argv) {
  auto remaining = rex::cvar::Init(argc, argv);
  rex::cvar::ApplyEnvironment();
  rex::InitLoggingEarly();
#if REX_PLATFORM_WIN32
  RequestWin32HighResolutionTimer();
  RequestWin32MMCSS();
#endif

  int result;
  {
    rex::ui::SDLWindowedAppContext app_context;
    if (!app_context.Initialize()) {
      return EXIT_FAILURE;
    }

#if REX_PLATFORM_WIN32
    // Apartment-threaded COM for shell dialogs.
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
      return EXIT_FAILURE;
    }
#endif

    std::unique_ptr<rex::ui::WindowedApp> app = rex::ui::GetWindowedAppCreator()(app_context);

    // Match remaining positional args to the app's expected options.
    const auto& option_names = app->GetPositionalOptions();
    std::map<std::string, std::string> parsed;
    size_t count = std::min(remaining.size(), option_names.size());
    for (size_t i = 0; i < count; ++i) {
      parsed[option_names[i]] = remaining[i];
    }
    app->SetParsedArguments(std::move(parsed));

    result = app->OnInitialize() ? app_context.RunMainMessageLoop() : EXIT_FAILURE;

    app->InvokeOnDestroy();
  }

#if REX_PLATFORM_WIN32
  CoUninitialize();
#endif

  return result;
}

#if REX_PLATFORM_WIN32
// Convert wide argv from CommandLineToArgvW to UTF-8 for cvar::Init.
std::vector<std::string> WideArgsToUtf8(int argc, wchar_t** wargv) {
  std::vector<std::string> args;
  args.reserve(static_cast<size_t>(argc));
  for (int i = 0; i < argc; ++i) {
    std::wstring wide(wargv[i]);
    if (wide.empty()) {
      args.emplace_back();
      continue;
    }
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr,
                                   0, nullptr, nullptr);
    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), size,
                        nullptr, nullptr);
    args.push_back(std::move(utf8));
  }
  return args;
}
void RequestWin32HighResolutionTimer() {
  auto write_timer_diag = [](const char* line) {
    if (FILE* f = std::fopen("C:\\temp\\fm2-timer.log", "a")) {
      std::fputs(line, f);
      std::fputs("\n", f);
      std::fclose(f);
    }
  };

  HMODULE ntdll_module = GetModuleHandleW(L"ntdll.dll");
  if (!ntdll_module) {
    REXLOG_WARN("Win32 timer: ntdll.dll not found, skipping high-resolution request");
    write_timer_diag("Win32 timer: ntdll.dll not found");
    return;
  }

  // Match Xenia startup: ask NT for maximum timer resolution so guest 10 ms
  // sleeps are not quantized to ~15.6 ms on default Windows timer settings.
  using NtQueryTimerResolutionFn = LONG(NTAPI*)(PULONG, PULONG, PULONG);
  using NtSetTimerResolutionFn = LONG(NTAPI*)(ULONG, BOOLEAN, PULONG);

  auto nt_query_timer_resolution = reinterpret_cast<NtQueryTimerResolutionFn>(
      GetProcAddress(ntdll_module, "NtQueryTimerResolution"));
  auto nt_set_timer_resolution = reinterpret_cast<NtSetTimerResolutionFn>(
      GetProcAddress(ntdll_module, "NtSetTimerResolution"));
  if (!nt_query_timer_resolution || !nt_set_timer_resolution) {
    REXLOG_WARN("Win32 timer: NtQuery/NtSetTimerResolution not available");
    write_timer_diag("Win32 timer: NtQuery/NtSetTimerResolution not available");
    return;
  }

  ULONG minimum_resolution = 0;
  ULONG maximum_resolution = 0;
  ULONG current_resolution = 0;
  auto query_status = nt_query_timer_resolution(&minimum_resolution, &maximum_resolution,
                                                &current_resolution);
  if (query_status >= 0) {
    REXLOG_ERROR("Win32 timer: query ok min={} max={} cur={}", minimum_resolution,
                 maximum_resolution, current_resolution);
    auto set_status =
        nt_set_timer_resolution(maximum_resolution, TRUE, &current_resolution);
    REXLOG_ERROR("Win32 timer: set status={} new_cur={}", set_status, current_resolution);
    char buf[256] = {};
    std::snprintf(buf, sizeof(buf),
                  "Win32 timer: query_status=%ld min=%lu max=%lu cur=%lu set_status=%ld new_cur=%lu",
                  query_status, minimum_resolution, maximum_resolution, current_resolution,
                  set_status, current_resolution);
    write_timer_diag(buf);
  } else {
    REXLOG_WARN("Win32 timer: query failed status={}", query_status);
    char buf[128] = {};
    std::snprintf(buf, sizeof(buf), "Win32 timer: query failed status=%ld", query_status);
    write_timer_diag(buf);
  }
}

void RequestWin32MMCSS() {
  HMODULE dwmapi_module = LoadLibraryW(L"dwmapi.dll");
  if (!dwmapi_module) {
    return;
  }
  using DwmEnableMMCSSFn = HRESULT(STDAPICALLTYPE*)(BOOL);
  auto dwm_enable_mmcss = reinterpret_cast<DwmEnableMMCSSFn>(
      GetProcAddress(dwmapi_module, "DwmEnableMMCSS"));
  if (dwm_enable_mmcss) {
    dwm_enable_mmcss(TRUE);
  }
  FreeLibrary(dwmapi_module);
}
#endif

}  // namespace

#if REX_PLATFORM_WIN32

int WINAPI wWinMain(HINSTANCE hinstance, HINSTANCE hinstance_prev, LPWSTR command_line,
                    int show_cmd) {
  (void)hinstance;
  (void)hinstance_prev;
  (void)command_line;
  (void)show_cmd;

  int wargc = 0;
  wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
  auto utf8_args = WideArgsToUtf8(wargc, wargv);
  LocalFree(wargv);

  std::vector<char*> argv_ptrs;
  argv_ptrs.reserve(utf8_args.size());
  for (auto& s : utf8_args) {
    argv_ptrs.push_back(s.data());
  }
  return RunWindowedApp(static_cast<int>(argv_ptrs.size()), argv_ptrs.data());
}

#else

int main(int argc, char* argv[]) {
  return RunWindowedApp(argc, argv);
}

#endif
