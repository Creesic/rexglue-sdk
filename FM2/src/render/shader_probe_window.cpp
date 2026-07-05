// Small always-on-top status window for the interactive shader probe
// (--fm2_shader_probe 1, driven from render_state.cpp). Kept in its own TU so
// <windows.h> stays out of the large render_state.cpp. The window runs on its
// own thread with a simple message loop; the render thread only pushes a text
// string under a mutex, and a repaint timer redraws it.

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>

#include "render/render_internal.h"

namespace fm2::render {

namespace {

std::mutex g_textMutex;
std::string g_text = "FM2 Shader Probe\n(waiting for first update...)";
std::atomic<bool> g_started{false};

LRESULT CALLBACK ProbeWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam) {
  switch (msg) {
  case WM_TIMER:
    InvalidateRect(hwnd, nullptr, TRUE);
    return 0;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    std::string text;
    {
      std::lock_guard<std::mutex> lock(g_textMutex);
      text = g_text;
    }
    RECT rc;
    GetClientRect(hwnd, &rc);
    FillRect(dc, &rc, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    rc.left += 10;
    rc.top += 8;
    SetBkMode(dc, TRANSPARENT);
    HFONT font = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             FIXED_PITCH | FF_MODERN, "Consolas");
    HGDIOBJ oldFont = SelectObject(dc, font);
    DrawTextA(dc, text.c_str(), -1, &rc, DT_LEFT | DT_TOP | DT_NOPREFIX);
    SelectObject(dc, oldFont);
    DeleteObject(font);
    EndPaint(hwnd, &ps);
    return 0;
  }
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void ProbeWindowThread() {
  HINSTANCE inst = GetModuleHandleA(nullptr);
  WNDCLASSEXA wc = {};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = ProbeWndProc;
  wc.hInstance = inst;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszClassName = "FM2ShaderProbeWindow";
  RegisterClassExA(&wc);

  HWND hwnd = CreateWindowExA(
      WS_EX_TOPMOST | WS_EX_TOOLWINDOW, wc.lpszClassName, "FM2 Shader Probe",
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 40, 40, 560, 235, nullptr,
      nullptr, inst, nullptr);
  if (hwnd == nullptr)
    return;
  ShowWindow(hwnd, SW_SHOWNOACTIVATE);
  SetTimer(hwnd, 1, 120, nullptr); // ~8 Hz repaint

  MSG msg;
  while (GetMessageA(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }
}

} // namespace

void UpdateShaderProbeWindow(int selDraw, int total, bool solo, int psIndex,
                             int psCount, uint64_t hash, const char *file,
                             bool psoOk, uint64_t boundHash, bool highlight,
                             bool filterSeen, int seenCount, bool testIsSeen) {
  char buf[900];
  std::snprintf(
      buf, sizeof(buf),
      "Draw:   %d / %d      solo: %s   highlight: %s\n"
      "bound:  0x%016llX   (what plume assigns; 0 = not in cache)\n"
      "test:   0x%016llX   psoOk: %d   [%d / %d]   %s\n"
      "file:   %s\n"
      "shaders:%s  (%d used on-screen)\n"
      "\n"
      "[F6/F7] draw  [F8/F9] shader  [F5] solo  [F10] filter  [F11] hilite",
      selDraw, total, solo ? "ON " : "off", highlight ? "ON " : "off",
      static_cast<unsigned long long>(boundHash),
      static_cast<unsigned long long>(hash), psoOk ? 1 : 0, psIndex, psCount,
      testIsSeen ? "<on-screen>" : "", file ? file : "",
      filterSeen ? "ON-SCREEN only" : "FULL cache", seenCount);
  {
    std::lock_guard<std::mutex> lock(g_textMutex);
    g_text = buf;
  }
  bool expected = false;
  if (g_started.compare_exchange_strong(expected, true))
    std::thread(ProbeWindowThread).detach();
}

} // namespace fm2::render
