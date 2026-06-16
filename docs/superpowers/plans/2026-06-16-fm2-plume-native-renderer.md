# FM2 Plume Native Renderer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first FM2-native Plume integration slice: build wiring, a native renderer facade, optional Plume clear/present, and shadow packet capture hooks.

**Architecture:** FM2 gets a small handwritten native-renderer layer under `FM2/src/native_renderer/`. Default `xenos` mode keeps current behavior unchanged; `shadow` initializes Plume without owning presentation and records FM2 render metadata; `plume_clear` disables the ReXGlue graphics system and uses Plume for a one-shot native clear/present smoke test.

**Tech Stack:** C++23 for FM2 handwritten code, ReXGlue runtime/cvars/logging, FM2 midasm hooks, CMake/Ninja presets, Plume static library with D3D12 on Windows.

## Global Constraints

- Work in `C:\Users\Tera\Documents\GitHub\ReXGlue080plume` on branch `FM2_WIN_Plume`.
- Do not revert unrelated dirty files.
- `FM2/generated/` is generated output; do not make permanent fixes there.
- `FM2/fm2_manifest.toml` is the source of truth for FM2 generation.
- Run FM2 codegen only when intentionally regenerating hook glue.
- Use `apply_patch` for manual edits.
- Keep the first slice FM2-specific; do not force other title projects to link Plume.
- Default runtime behavior must stay unchanged unless `fm2_plume_mode` is set away from `xenos`.
- The first slice must not remove the ReXGlue Xenos backend.

---

## File Structure

- Create `FM2/src/native_renderer/fm2_native_renderer.h`: narrow public facade used by `Fm2App` and FM2 hook adapters. This header contains no Plume types.
- Create `FM2/src/native_renderer/fm2_native_renderer.cpp`: owns Plume objects, renderer mode cvars, clear/present smoke test, and packet counters.
- Modify `FM2/CMakeLists.txt`: add the native-renderer source and link `plume` only for Windows FM2 builds when `../plume` exists.
- Modify `FM2/src/fm2_app.h`: call the native renderer during setup/shutdown and disable ReXGlue graphics only for the explicit `plume_clear` smoke-test mode.
- Modify `FM2/src/fm2_hooks.cpp`: add thin hook adapters that forward register snapshots into the native renderer facade.
- Modify `FM2/fm2_manifest.toml`: add non-branching midasm hooks for the first two render capture sites.
- Generated files in `FM2/generated/` may change only as the result of `fm2_codegen`.
- Update `docs/FM2-native-renderer-generator-notes.md` after verification with the exact cvars, hook addresses, and observed build/runtime behavior.

---

### Task 1: Build-Wired Native Renderer Skeleton

**Files:**
- Create: `FM2/src/native_renderer/fm2_native_renderer.h`
- Create: `FM2/src/native_renderer/fm2_native_renderer.cpp`
- Modify: `FM2/CMakeLists.txt`
- Modify: `FM2/src/fm2_app.h`

**Interfaces:**
- Consumes: `rex::ui::Window`, `rex::RuntimeConfig`, ReXGlue cvars/logging.
- Produces:
  - `fm2::native_renderer::Mode GetMode()`
  - `const char* GetModeName(Mode mode)`
  - `bool WantsReXGraphics()`
  - `bool Initialize(rex::ui::Window* window)`
  - `void Shutdown()`
  - `bool RenderClearOnce()`
  - `void RecordBuildObjectPassEntry(const GuestArgs& args)`
  - `void RecordDirectIndexedDrawEntry(const GuestArgs& args)`
  - `Stats GetStatsSnapshot()`

- [ ] **Step 1: Confirm the current FM2 target state**

Run:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2
```

Expected: either a successful build, or an unrelated pre-existing failure that must be recorded before continuing. Do not modify generated files for this step.

- [ ] **Step 2: Add the native renderer facade header**

Create `FM2/src/native_renderer/fm2_native_renderer.h`:

```cpp
#pragma once

#include <cstdint>

namespace rex::ui {
class Window;
}

namespace fm2::native_renderer {

enum class Mode : uint8_t {
  kXenos,
  kShadow,
  kPlumeClear,
};

struct GuestArgs {
  uint32_t r3 = 0;
  uint32_t r4 = 0;
  uint32_t r5 = 0;
  uint32_t r6 = 0;
  uint32_t r7 = 0;
  uint32_t r8 = 0;
  uint32_t r9 = 0;
  uint32_t r10 = 0;
};

struct Stats {
  bool initialized = false;
  bool plume_available = false;
  bool plume_device_ready = false;
  bool swapchain_ready = false;
  uint64_t build_object_pass_entries = 0;
  uint64_t direct_indexed_draw_entries = 0;
  uint32_t last_hook_address = 0;
  GuestArgs last_args;
};

Mode GetMode();
const char* GetModeName(Mode mode);
bool WantsReXGraphics();

bool Initialize(rex::ui::Window* window);
void Shutdown();
bool RenderClearOnce();

void RecordBuildObjectPassEntry(const GuestArgs& args);
void RecordDirectIndexedDrawEntry(const GuestArgs& args);
Stats GetStatsSnapshot();

}  // namespace fm2::native_renderer
```

- [ ] **Step 3: Add the skeleton implementation**

Create `FM2/src/native_renderer/fm2_native_renderer.cpp`:

```cpp
#include "native_renderer/fm2_native_renderer.h"

#include <atomic>
#include <mutex>
#include <string>

#include <rex/cvar.h>
#include <rex/logging.h>

namespace {

REXCVAR_DEFINE_STRING(
    fm2_plume_mode, "xenos", "FM2",
    "FM2 Plume renderer mode: xenos, shadow, plume_clear")
    .allowed({"xenos", "shadow", "plume_clear"});

REXCVAR_DEFINE_BOOL(
    fm2_plume_trace_packets, false, "FM2",
    "Log FM2 Plume shadow packet capture samples");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_log_interval, 120, "FM2",
    "Log one FM2 Plume packet sample every N captures when tracing is enabled");

std::atomic<bool> g_initialized{false};
std::atomic<bool> g_plume_available{false};
std::atomic<bool> g_plume_device_ready{false};
std::atomic<bool> g_swapchain_ready{false};
std::atomic<uint64_t> g_build_object_pass_entries{0};
std::atomic<uint64_t> g_direct_indexed_draw_entries{0};
std::atomic<uint32_t> g_last_hook_address{0};

std::mutex g_last_args_mutex;
fm2::native_renderer::GuestArgs g_last_args;

fm2::native_renderer::Mode ParseMode(const std::string& value) {
  if (value == "shadow") {
    return fm2::native_renderer::Mode::kShadow;
  }
  if (value == "plume_clear") {
    return fm2::native_renderer::Mode::kPlumeClear;
  }
  return fm2::native_renderer::Mode::kXenos;
}

void StoreLastArgs(uint32_t hook_address, const fm2::native_renderer::GuestArgs& args) {
  g_last_hook_address.store(hook_address, std::memory_order_relaxed);
  std::scoped_lock lock(g_last_args_mutex);
  g_last_args = args;
}

void MaybeLogPacket(const char* name, uint32_t hook_address, uint64_t count,
                    const fm2::native_renderer::GuestArgs& args) {
  if (!REXCVAR_GET(fm2_plume_trace_packets)) {
    return;
  }
  const uint32_t interval = REXCVAR_GET(fm2_plume_trace_log_interval);
  if (interval == 0 || (count % interval) != 1) {
    return;
  }
  REXLOG_INFO(
      "{} count={} hook={:08X} r3={:08X} r4={:08X} r5={:08X} r6={:08X} "
      "r7={:08X} r8={:08X} r9={:08X} r10={:08X}",
      name, count, hook_address, args.r3, args.r4, args.r5, args.r6, args.r7, args.r8, args.r9,
      args.r10);
}

}  // namespace

namespace fm2::native_renderer {

Mode GetMode() {
  return ParseMode(REXCVAR_GET(fm2_plume_mode));
}

const char* GetModeName(Mode mode) {
  switch (mode) {
    case Mode::kXenos:
      return "xenos";
    case Mode::kShadow:
      return "shadow";
    case Mode::kPlumeClear:
      return "plume_clear";
  }
  return "xenos";
}

bool WantsReXGraphics() {
  return GetMode() != Mode::kPlumeClear;
}

bool Initialize(rex::ui::Window* window) {
  (void)window;
  const Mode mode = GetMode();
  if (mode == Mode::kXenos) {
    return true;
  }

  g_initialized.store(true, std::memory_order_relaxed);
  REXLOG_INFO("FM2 Plume native renderer skeleton initialized mode={}", GetModeName(mode));
  return true;
}

void Shutdown() {
  const bool was_initialized = g_initialized.exchange(false, std::memory_order_relaxed);
  g_plume_available.store(false, std::memory_order_relaxed);
  g_plume_device_ready.store(false, std::memory_order_relaxed);
  g_swapchain_ready.store(false, std::memory_order_relaxed);
  if (was_initialized) {
    REXLOG_INFO("FM2 Plume native renderer skeleton shut down");
  }
}

bool RenderClearOnce() {
  REXLOG_WARN("FM2 Plume clear requested before Plume device support is implemented");
  return false;
}

void RecordBuildObjectPassEntry(const GuestArgs& args) {
  const uint64_t count =
      g_build_object_pass_entries.fetch_add(1, std::memory_order_relaxed) + 1;
  StoreLastArgs(0x82531370u, args);
  MaybeLogPacket("FM2_PLUME_BUILD_OBJECT_PASS", 0x82531370u, count, args);
}

void RecordDirectIndexedDrawEntry(const GuestArgs& args) {
  const uint64_t count =
      g_direct_indexed_draw_entries.fetch_add(1, std::memory_order_relaxed) + 1;
  StoreLastArgs(0x825380B8u, args);
  MaybeLogPacket("FM2_PLUME_DIRECT_INDEXED_DRAW", 0x825380B8u, count, args);
}

Stats GetStatsSnapshot() {
  Stats out;
  out.initialized = g_initialized.load(std::memory_order_relaxed);
  out.plume_available = g_plume_available.load(std::memory_order_relaxed);
  out.plume_device_ready = g_plume_device_ready.load(std::memory_order_relaxed);
  out.swapchain_ready = g_swapchain_ready.load(std::memory_order_relaxed);
  out.build_object_pass_entries = g_build_object_pass_entries.load(std::memory_order_relaxed);
  out.direct_indexed_draw_entries = g_direct_indexed_draw_entries.load(std::memory_order_relaxed);
  out.last_hook_address = g_last_hook_address.load(std::memory_order_relaxed);
  {
    std::scoped_lock lock(g_last_args_mutex);
    out.last_args = g_last_args;
  }
  return out;
}

}  // namespace fm2::native_renderer
```

- [ ] **Step 4: Wire the source and Plume target in FM2 CMake**

Modify `FM2/CMakeLists.txt` so `FM2_SOURCES` includes the new implementation and the FM2 target links Plume only when available on Windows:

```cmake
set(FM2_SOURCES
    src/main.cpp
    src/fm2_hooks.cpp
    src/native_renderer/fm2_native_renderer.cpp
)
```

Add this after `rexglue_setup_target(fm2)`:

```cmake
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../plume/CMakeLists.txt")
    set(PLUME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    if(NOT TARGET plume)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../plume" "${CMAKE_CURRENT_BINARY_DIR}/plume")
    endif()
    target_link_libraries(fm2 PRIVATE plume)
    target_compile_definitions(fm2 PRIVATE FM2_HAS_PLUME=1)
else()
    target_compile_definitions(fm2 PRIVATE FM2_HAS_PLUME=0)
endif()
```

- [ ] **Step 5: Call the skeleton from `Fm2App`**

Modify `FM2/src/fm2_app.h`:

```cpp
#include "native_renderer/fm2_native_renderer.h"
```

Update `OnPreSetup` so only `plume_clear` disables ReXGlue graphics:

```cpp
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
```

Add post-setup and shutdown hooks:

```cpp
  void OnPostSetup() override {
    fm2::native_renderer::Initialize(window());
  }

  void OnShutdown() override {
    fm2::native_renderer::Shutdown();
  }
```

- [ ] **Step 6: Build the skeleton**

Run:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2
```

Expected: `fm2` builds and the default `fm2_plume_mode=xenos` does not initialize Plume at runtime.

- [ ] **Step 7: Commit Task 1**

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume'
git add -- FM2/CMakeLists.txt FM2/src/fm2_app.h FM2/src/native_renderer/fm2_native_renderer.h FM2/src/native_renderer/fm2_native_renderer.cpp
git commit -m "feat: add FM2 Plume renderer skeleton"
```

---

### Task 2: Plume Device And One-Shot Clear/Present

**Files:**
- Modify: `FM2/src/native_renderer/fm2_native_renderer.cpp`

**Interfaces:**
- Consumes: Task 1 facade and `rex::ui::Window::GetNativeWindowHandle()`.
- Produces: working Plume D3D12 interface/device/queue/swapchain setup in `shadow` or `plume_clear` mode; `RenderClearOnce()` returns true only after a Plume frame is submitted and presented.

- [ ] **Step 1: Replace the skeleton implementation with Plume-backed state**

Replace `FM2/src/native_renderer/fm2_native_renderer.cpp` with:

```cpp
#include "native_renderer/fm2_native_renderer.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/platform.h>
#include <rex/ui/window.h>

#ifndef FM2_HAS_PLUME
#define FM2_HAS_PLUME 0
#endif

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
#include <plume_render_interface.h>

namespace plume {
std::unique_ptr<RenderInterface> CreateD3D12Interface();
}
#endif

namespace {

REXCVAR_DEFINE_STRING(
    fm2_plume_mode, "xenos", "FM2",
    "FM2 Plume renderer mode: xenos, shadow, plume_clear")
    .allowed({"xenos", "shadow", "plume_clear"});

REXCVAR_DEFINE_BOOL(
    fm2_plume_trace_packets, false, "FM2",
    "Log FM2 Plume shadow packet capture samples");

REXCVAR_DEFINE_UINT32(
    fm2_plume_trace_log_interval, 120, "FM2",
    "Log one FM2 Plume packet sample every N captures when tracing is enabled");

REXCVAR_DEFINE_BOOL(
    fm2_plume_clear_on_init, true, "FM2",
    "In fm2_plume_mode=plume_clear, issue one Plume clear/present during FM2 setup");

std::atomic<bool> g_initialized{false};
std::atomic<bool> g_plume_available{false};
std::atomic<bool> g_plume_device_ready{false};
std::atomic<bool> g_swapchain_ready{false};
std::atomic<uint64_t> g_build_object_pass_entries{0};
std::atomic<uint64_t> g_direct_indexed_draw_entries{0};
std::atomic<uint32_t> g_last_hook_address{0};

std::mutex g_last_args_mutex;
fm2::native_renderer::GuestArgs g_last_args;

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
struct PlumeState {
  std::unique_ptr<plume::RenderInterface> render_interface;
  std::unique_ptr<plume::RenderDevice> device;
  std::unique_ptr<plume::RenderCommandQueue> command_queue;
  std::unique_ptr<plume::RenderCommandList> command_list;
  std::unique_ptr<plume::RenderCommandFence> fence;
  std::unique_ptr<plume::RenderSwapChain> swapchain;
  std::unique_ptr<plume::RenderCommandSemaphore> acquire_semaphore;
  std::vector<std::unique_ptr<plume::RenderCommandSemaphore>> release_semaphores;
  std::vector<std::unique_ptr<plume::RenderFramebuffer>> framebuffers;
};

std::mutex g_plume_mutex;
PlumeState g_plume;

constexpr uint32_t kSwapchainBufferCount = 2;
constexpr plume::RenderFormat kSwapchainFormat = plume::RenderFormat::B8G8R8A8_UNORM;
#endif

fm2::native_renderer::Mode ParseMode(const std::string& value) {
  if (value == "shadow") {
    return fm2::native_renderer::Mode::kShadow;
  }
  if (value == "plume_clear") {
    return fm2::native_renderer::Mode::kPlumeClear;
  }
  return fm2::native_renderer::Mode::kXenos;
}

void StoreLastArgs(uint32_t hook_address, const fm2::native_renderer::GuestArgs& args) {
  g_last_hook_address.store(hook_address, std::memory_order_relaxed);
  std::scoped_lock lock(g_last_args_mutex);
  g_last_args = args;
}

void MaybeLogPacket(const char* name, uint32_t hook_address, uint64_t count,
                    const fm2::native_renderer::GuestArgs& args) {
  if (!REXCVAR_GET(fm2_plume_trace_packets)) {
    return;
  }
  const uint32_t interval = REXCVAR_GET(fm2_plume_trace_log_interval);
  if (interval == 0 || (count % interval) != 1) {
    return;
  }
  REXLOG_INFO(
      "{} count={} hook={:08X} r3={:08X} r4={:08X} r5={:08X} r6={:08X} "
      "r7={:08X} r8={:08X} r9={:08X} r10={:08X}",
      name, count, hook_address, args.r3, args.r4, args.r5, args.r6, args.r7, args.r8, args.r9,
      args.r10);
}

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
void ResetPlumeStateLocked() {
  g_plume.framebuffers.clear();
  g_plume.release_semaphores.clear();
  g_plume.acquire_semaphore.reset();
  g_plume.swapchain.reset();
  g_plume.fence.reset();
  g_plume.command_list.reset();
  g_plume.command_queue.reset();
  g_plume.device.reset();
  g_plume.render_interface.reset();
  g_swapchain_ready.store(false, std::memory_order_relaxed);
  g_plume_device_ready.store(false, std::memory_order_relaxed);
  g_plume_available.store(false, std::memory_order_relaxed);
}

bool CreateFramebuffersLocked() {
  g_plume.framebuffers.clear();
  if (!g_plume.device || !g_plume.swapchain) {
    return false;
  }

  for (uint32_t i = 0; i < g_plume.swapchain->getTextureCount(); ++i) {
    const plume::RenderTexture* color_attachment = g_plume.swapchain->getTexture(i);
    plume::RenderFramebufferDesc fb_desc;
    fb_desc.colorAttachments = &color_attachment;
    fb_desc.colorAttachmentsCount = 1;
    fb_desc.depthAttachment = nullptr;
    auto framebuffer = g_plume.device->createFramebuffer(fb_desc);
    if (!framebuffer) {
      g_plume.framebuffers.clear();
      return false;
    }
    g_plume.framebuffers.push_back(std::move(framebuffer));
  }
  return !g_plume.framebuffers.empty();
}

bool CreatePlumeDeviceLocked() {
  g_plume.render_interface = plume::CreateD3D12Interface();
  if (!g_plume.render_interface) {
    REXLOG_WARN("FM2 Plume failed to create D3D12 render interface");
    return false;
  }
  g_plume_available.store(true, std::memory_order_relaxed);

  g_plume.device = g_plume.render_interface->createDevice();
  if (!g_plume.device) {
    REXLOG_WARN("FM2 Plume failed to create render device");
    return false;
  }

  g_plume.command_queue = g_plume.device->createCommandQueue(plume::RenderCommandListType::DIRECT);
  g_plume.command_list = g_plume.command_queue ? g_plume.command_queue->createCommandList() : nullptr;
  g_plume.fence = g_plume.device->createCommandFence();
  g_plume.acquire_semaphore = g_plume.device->createCommandSemaphore();

  if (!g_plume.command_queue || !g_plume.command_list || !g_plume.fence ||
      !g_plume.acquire_semaphore) {
    REXLOG_WARN("FM2 Plume failed to create command resources");
    return false;
  }

  g_plume_device_ready.store(true, std::memory_order_relaxed);
  REXLOG_INFO("FM2 Plume device initialized backend=D3D12");
  return true;
}

bool CreateSwapchainLocked(rex::ui::Window* window) {
  if (!window || !g_plume.command_queue) {
    REXLOG_WARN("FM2 Plume swapchain creation skipped because window or queue is missing");
    return false;
  }

  void* native_window = window->GetNativeWindowHandle();
  if (!native_window) {
    REXLOG_WARN("FM2 Plume swapchain creation skipped because native window handle is null");
    return false;
  }

  auto render_window = reinterpret_cast<plume::RenderWindow>(native_window);
  g_plume.swapchain = g_plume.command_queue->createSwapChain(
      plume::RenderSwapChainDesc(render_window, kSwapchainFormat, kSwapchainBufferCount));
  if (!g_plume.swapchain || !g_plume.swapchain->resize()) {
    REXLOG_WARN("FM2 Plume failed to create or resize swapchain");
    return false;
  }

  if (!CreateFramebuffersLocked()) {
    REXLOG_WARN("FM2 Plume failed to create swapchain framebuffers");
    return false;
  }

  g_swapchain_ready.store(true, std::memory_order_relaxed);
  REXLOG_INFO("FM2 Plume swapchain initialized size={}x{} textures={}",
              g_plume.swapchain->getWidth(), g_plume.swapchain->getHeight(),
              g_plume.swapchain->getTextureCount());
  return true;
}

bool RenderClearOnceLocked() {
  if (!g_plume.device || !g_plume.command_queue || !g_plume.command_list || !g_plume.swapchain ||
      !g_plume.acquire_semaphore || !g_plume.fence) {
    return false;
  }

  if (g_plume.swapchain->needsResize()) {
    g_plume.framebuffers.clear();
    if (!g_plume.swapchain->resize() || !CreateFramebuffersLocked()) {
      REXLOG_WARN("FM2 Plume failed to resize swapchain before clear");
      return false;
    }
  }

  uint32_t image_index = 0;
  if (!g_plume.swapchain->acquireTexture(g_plume.acquire_semaphore.get(), &image_index)) {
    REXLOG_WARN("FM2 Plume failed to acquire swapchain texture");
    return false;
  }
  if (image_index >= g_plume.framebuffers.size()) {
    REXLOG_WARN("FM2 Plume acquired invalid swapchain image index {}", image_index);
    return false;
  }

  g_plume.command_list->begin();
  plume::RenderTexture* texture = g_plume.swapchain->getTexture(image_index);
  g_plume.command_list->barriers(
      plume::RenderBarrierStage::GRAPHICS,
      plume::RenderTextureBarrier(texture, plume::RenderTextureLayout::COLOR_WRITE));
  g_plume.command_list->setFramebuffer(g_plume.framebuffers[image_index].get());

  const uint32_t width = g_plume.swapchain->getWidth();
  const uint32_t height = g_plume.swapchain->getHeight();
  g_plume.command_list->setViewports(plume::RenderViewport(0.0f, 0.0f, float(width), float(height)));
  g_plume.command_list->setScissors(plume::RenderRect(0, 0, width, height));
  g_plume.command_list->clearColor(0, plume::RenderColor(0.02f, 0.0f, 0.08f, 1.0f));
  g_plume.command_list->barriers(
      plume::RenderBarrierStage::NONE,
      plume::RenderTextureBarrier(texture, plume::RenderTextureLayout::PRESENT));
  g_plume.command_list->end();

  while (g_plume.release_semaphores.size() < g_plume.swapchain->getTextureCount()) {
    g_plume.release_semaphores.emplace_back(g_plume.device->createCommandSemaphore());
  }

  const plume::RenderCommandList* command_list = g_plume.command_list.get();
  plume::RenderCommandSemaphore* wait_semaphore = g_plume.acquire_semaphore.get();
  plume::RenderCommandSemaphore* signal_semaphore = g_plume.release_semaphores[image_index].get();

  g_plume.command_queue->executeCommandLists(&command_list, 1, &wait_semaphore, 1,
                                             &signal_semaphore, 1, g_plume.fence.get());
  const bool presented = g_plume.swapchain->present(image_index, &signal_semaphore, 1);
  g_plume.command_queue->waitForCommandFence(g_plume.fence.get());

  REXLOG_INFO("FM2 Plume clear/present result={} image={} size={}x{}", presented, image_index,
              width, height);
  return presented;
}
#endif

}  // namespace

namespace fm2::native_renderer {

Mode GetMode() {
  return ParseMode(REXCVAR_GET(fm2_plume_mode));
}

const char* GetModeName(Mode mode) {
  switch (mode) {
    case Mode::kXenos:
      return "xenos";
    case Mode::kShadow:
      return "shadow";
    case Mode::kPlumeClear:
      return "plume_clear";
  }
  return "xenos";
}

bool WantsReXGraphics() {
  return GetMode() != Mode::kPlumeClear;
}

bool Initialize(rex::ui::Window* window) {
  const Mode mode = GetMode();
  if (mode == Mode::kXenos) {
    return true;
  }

  g_initialized.store(true, std::memory_order_relaxed);

#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  std::scoped_lock lock(g_plume_mutex);
  ResetPlumeStateLocked();

  if (!CreatePlumeDeviceLocked()) {
    REXLOG_WARN("FM2 Plume initialization incomplete mode={}", GetModeName(mode));
    return mode != Mode::kPlumeClear;
  }

  if (mode == Mode::kPlumeClear) {
    if (!CreateSwapchainLocked(window)) {
      REXLOG_WARN("FM2 Plume clear mode could not create swapchain");
      return false;
    }
    if (REXCVAR_GET(fm2_plume_clear_on_init)) {
      return RenderClearOnceLocked();
    }
  }

  REXLOG_INFO("FM2 Plume native renderer initialized mode={}", GetModeName(mode));
  return true;
#else
  g_plume_available.store(false, std::memory_order_relaxed);
  REXLOG_WARN("FM2 Plume mode {} requested, but this build has no Plume support",
              GetModeName(mode));
  return mode != Mode::kPlumeClear;
#endif
}

void Shutdown() {
  const bool was_initialized = g_initialized.exchange(false, std::memory_order_relaxed);
#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  {
    std::scoped_lock lock(g_plume_mutex);
    ResetPlumeStateLocked();
  }
#else
  g_plume_available.store(false, std::memory_order_relaxed);
  g_plume_device_ready.store(false, std::memory_order_relaxed);
  g_swapchain_ready.store(false, std::memory_order_relaxed);
#endif
  if (was_initialized) {
    REXLOG_INFO("FM2 Plume native renderer shut down");
  }
}

bool RenderClearOnce() {
#if FM2_HAS_PLUME && REX_PLATFORM_WIN32
  std::scoped_lock lock(g_plume_mutex);
  return RenderClearOnceLocked();
#else
  REXLOG_WARN("FM2 Plume clear requested, but this build has no Plume support");
  return false;
#endif
}

void RecordBuildObjectPassEntry(const GuestArgs& args) {
  const uint64_t count =
      g_build_object_pass_entries.fetch_add(1, std::memory_order_relaxed) + 1;
  StoreLastArgs(0x82531370u, args);
  MaybeLogPacket("FM2_PLUME_BUILD_OBJECT_PASS", 0x82531370u, count, args);
}

void RecordDirectIndexedDrawEntry(const GuestArgs& args) {
  const uint64_t count =
      g_direct_indexed_draw_entries.fetch_add(1, std::memory_order_relaxed) + 1;
  StoreLastArgs(0x825380B8u, args);
  MaybeLogPacket("FM2_PLUME_DIRECT_INDEXED_DRAW", 0x825380B8u, count, args);
}

Stats GetStatsSnapshot() {
  Stats out;
  out.initialized = g_initialized.load(std::memory_order_relaxed);
  out.plume_available = g_plume_available.load(std::memory_order_relaxed);
  out.plume_device_ready = g_plume_device_ready.load(std::memory_order_relaxed);
  out.swapchain_ready = g_swapchain_ready.load(std::memory_order_relaxed);
  out.build_object_pass_entries = g_build_object_pass_entries.load(std::memory_order_relaxed);
  out.direct_indexed_draw_entries = g_direct_indexed_draw_entries.load(std::memory_order_relaxed);
  out.last_hook_address = g_last_hook_address.load(std::memory_order_relaxed);
  {
    std::scoped_lock lock(g_last_args_mutex);
    out.last_args = g_last_args;
  }
  return out;
}

}  // namespace fm2::native_renderer
```

- [ ] **Step 2: Build the Plume-backed renderer**

Run:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2
```

Expected: `plume` static library builds under the FM2 build tree and `fm2` links successfully.

- [ ] **Step 3: Smoke-test default mode**

Run FM2 with the existing config and no `fm2_plume_mode` override.

Expected log behavior:

```text
FM2 Plume native renderer initialized
```

does not appear, because `xenos` mode returns without initializing Plume.

- [ ] **Step 4: Smoke-test shadow mode**

Set `fm2_plume_mode = "shadow"` in the FM2 config or through the existing cvar mechanism, then launch FM2.

Expected log behavior:

```text
FM2 Plume device initialized backend=D3D12
FM2 Plume native renderer initialized mode=shadow
```

Expected rendering behavior: current ReXGlue/Xenos output remains the visible renderer.

- [ ] **Step 5: Smoke-test native Plume clear mode**

Set `fm2_plume_mode = "plume_clear"` and `fm2_plume_clear_on_init = true`, then launch FM2.

Expected log behavior:

```text
Runtime initialized without graphics system (native rendering mode)
FM2 Plume device initialized backend=D3D12
FM2 Plume swapchain initialized size=<width>x<height> textures=2
FM2 Plume clear/present result=true image=<index> size=<width>x<height>
```

Expected rendering behavior: the current Xenos-backed renderer is disabled for this run, and the window receives one dark-purple Plume clear.

- [ ] **Step 6: Commit Task 2**

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume'
git add -- FM2/src/native_renderer/fm2_native_renderer.cpp
git commit -m "feat: initialize Plume for FM2 native renderer"
```

---

### Task 3: Shadow Packet Capture Hooks

**Files:**
- Modify: `FM2/src/fm2_hooks.cpp`
- Modify: `FM2/fm2_manifest.toml`
- Generated by codegen: `FM2/generated/*`

**Interfaces:**
- Consumes: Task 1 facade `RecordBuildObjectPassEntry` and `RecordDirectIndexedDrawEntry`.
- Produces:
  - `void FM2PlumeTraceBuildObjectPassEntry(PPCRegister& r3, ..., PPCRegister& r10)`
  - `void FM2PlumeTraceDirectIndexedDrawEntry(PPCRegister& r3, ..., PPCRegister& r10)`
  - Two non-branching midasm hooks that capture guest argument registers at function entry.

- [ ] **Step 1: Confirm generated tree state before codegen**

Run:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume'
git status --short -- FM2/generated FM2/fm2_manifest.toml FM2/src/fm2_hooks.cpp
```

Expected: note any pre-existing generated changes. Do not revert them.

- [ ] **Step 2: Include the native renderer facade from `fm2_hooks.cpp`**

Add this include near the top of `FM2/src/fm2_hooks.cpp`, after `#include "generated/fm2_init.h"`:

```cpp
#include "native_renderer/fm2_native_renderer.h"
```

- [ ] **Step 3: Add the hook adapter functions**

Add these functions near the other FM2 hook functions in `FM2/src/fm2_hooks.cpp`:

```cpp
void FM2PlumeTraceBuildObjectPassEntry(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                                       PPCRegister& r6, PPCRegister& r7, PPCRegister& r8,
                                       PPCRegister& r9, PPCRegister& r10) {
  fm2::native_renderer::RecordBuildObjectPassEntry({
      r3.u32,
      r4.u32,
      r5.u32,
      r6.u32,
      r7.u32,
      r8.u32,
      r9.u32,
      r10.u32,
  });
}

void FM2PlumeTraceDirectIndexedDrawEntry(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                                         PPCRegister& r6, PPCRegister& r7, PPCRegister& r8,
                                         PPCRegister& r9, PPCRegister& r10) {
  fm2::native_renderer::RecordDirectIndexedDrawEntry({
      r3.u32,
      r4.u32,
      r5.u32,
      r6.u32,
      r7.u32,
      r8.u32,
      r9.u32,
      r10.u32,
  });
}
```

- [ ] **Step 4: Add non-branching midasm hooks to the manifest**

Append these entries at the end of `FM2/fm2_manifest.toml` so they are easy to remove or adjust as a pair:

```toml
[[entrypoint.midasm_hook]]
address = 0x82531370
name = "FM2PlumeTraceBuildObjectPassEntry"
registers = ["r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"]
after_instruction = false

[[entrypoint.midasm_hook]]
address = 0x825380B8
name = "FM2PlumeTraceDirectIndexedDrawEntry"
registers = ["r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"]
after_instruction = false
```

- [ ] **Step 5: Regenerate FM2 code intentionally**

Run:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2_codegen
```

Expected: generated recompilation files update to include `extern void FM2PlumeTraceBuildObjectPassEntry(...)` and `extern void FM2PlumeTraceDirectIndexedDrawEntry(...)` at the chosen hook sites.

- [ ] **Step 6: Build the hooked FM2 target**

Run:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2
```

Expected: `fm2` builds with the new hook adapter symbols resolved.

- [ ] **Step 7: Runtime-check shadow packet logs**

Set:

```toml
fm2_plume_mode = "shadow"
fm2_plume_trace_packets = true
fm2_plume_trace_log_interval = 120
```

Launch FM2 and reach the first scene or menu point that exercises the render pipeline.

Expected log samples:

```text
FM2_PLUME_BUILD_OBJECT_PASS count=1 hook=82531370 ...
FM2_PLUME_DIRECT_INDEXED_DRAW count=1 hook=825380B8 ...
```

If one hook does not fire, keep the other hook and record which function did not execute in the docs.

- [ ] **Step 8: Commit Task 3**

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume'
git add -- FM2/src/fm2_hooks.cpp FM2/fm2_manifest.toml FM2/generated
git commit -m "feat: capture FM2 render packets for Plume shadow mode"
```

Before committing, inspect `git diff --cached --stat` and confirm only FM2 hook, manifest, and intended generated files are staged.

---

### Task 4: Verification Notes And First Replay Gate

**Files:**
- Modify: `docs/FM2-native-renderer-generator-notes.md`
- Optional modify: `docs/FM2-ida-toml-function-notes.md` only if hook addresses or function names change after IDA review.

**Interfaces:**
- Consumes: build/runtime results from Tasks 1-3.
- Produces: durable notes for continuing toward the first real Plume draw replay.

- [ ] **Step 1: Record the build and runtime results**

Append a `June 16 Plume integration slice` section to `docs/FM2-native-renderer-generator-notes.md` with this structure:

```markdown
## June 16 Plume Integration Slice

- Build mode:
  - Branch: `FM2_WIN_Plume`
  - Plume linked into FM2: yes/no
  - FM2 build command: `cmake --build --preset win-amd64-relwithdebinfo --target fm2`
- Runtime modes:
  - `fm2_plume_mode=xenos`: current backend only.
  - `fm2_plume_mode=shadow`: Plume device initialization plus current backend rendering.
  - `fm2_plume_mode=plume_clear`: ReXGlue graphics disabled; Plume one-shot clear/present.
- Hook capture:
  - `0x82531370` / `FM2_Render_BuildObjectPassCommandBuffer`: observed count and first sample.
  - `0x825380B8` / `FM2_Render_BuildDirectIndexedDrawBuffers`: observed count and first sample.
- Next replay gate:
  - Pick the hook with stable visible arguments.
  - Identify vertex/index buffer fields in IDA before attempting native draw replay.
  - Keep original Xenos draw enabled until a Plume debug draw presents from a captured packet.
```

Replace `yes/no`, observed counts, and first sample values with the exact results from the run.

- [ ] **Step 2: Final verification build**

Run:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume\FM2'
cmake --build --preset win-amd64-relwithdebinfo --target fm2
```

Expected: build succeeds after docs-only changes.

- [ ] **Step 3: Check final git scope**

Run:

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume'
git status --short
git diff --stat
```

Expected: changed files are limited to the FM2 native renderer integration, intentional generated output, and docs. Pre-existing dirty files may still appear; do not stage or revert unrelated files.

- [ ] **Step 4: Commit Task 4**

```powershell
Set-Location 'C:\Users\Tera\Documents\GitHub\ReXGlue080plume'
git add -- docs/FM2-native-renderer-generator-notes.md
git commit -m "docs: record FM2 Plume renderer bring-up"
```

---

## Self-Review

- Spec coverage: The plan covers scoped Plume build integration, FM2 native-renderer facade, default-safe runtime ownership, shadow packet capture, optional clear/present, diagnostics, and verification notes.
- Scope boundary: The plan does not attempt shader translation, resource lifetime replacement, UI rendering, pass replacement, or backend removal.
- Type consistency: `GuestArgs`, `Stats`, `Mode`, and every public function name are defined in Task 1 and reused unchanged in subsequent tasks.
- Generated code policy: Generated FM2 output is touched only in Task 3 through `fm2_codegen`.
- Dirty tree safety: Each commit command stages explicit paths and warns before staging generated output.
