# Native GPU Path for FM4, Milestone 1: PM4 Sink and Present

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** FM4 runs its real D3D library against a host-owned PM4 sink with the swapchain, clears and presents driven by Plume, selectable with `gpu_plugin = "native"`, while a trace mode under the existing xenos path captures the shader bytecode and call statistics the next milestones need.

**Architecture:** Instead of replacing `Direct3D_CreateDevice` wholesale (Unleashed) or rebuilding the device from static templates (reblue), this milestone lets FM4's own D3D library initialise and maintain the guest `D3DDevice`, and neuters only the GPU side: the ring "make space" function is redirected to a host-allocated sink, every GPU wait becomes a no-op, and `D3DDevice_Swap`/`D3DDevice_Clear` are replaced by Plume calls. Unhooked D3D calls keep working (they write PM4 into the sink), so later milestones can migrate draws, resources and state one hook at a time without the game crashing on the parts not yet ported. Presentation follows reblue's inline model: no render thread, one mutex, a two-slot frame ring.

**Tech Stack:** ReXGlue 0.10 (this repo, clang-cl), Plume (zolaware fork, D3D12 backend), FM4 title project under `FM4/`, IDA (ida40) for address verification.

**Spec:** `FM4/docs/native-render-plume-deepdive.md`, sections 1, 3, 4 and 5 (5.1, 5.2, 5.7, 5.8).

## Global Constraints

- Build preset: `win-amd64-relwithdebinfo` for both trees (`FM4/CMakePresets.json:161`); compiler is clang-cl (`FM4/CMakePresets.json:61`), which the weak-alias override in `FM4/generated/default/fm4_pch.h:75-78` requires.
- Never hand-edit `FM4/generated/`. Regenerate with `cd FM4 && ..\out\win-amd64\rexglue.exe codegen fm4_manifest.toml --force`.
- **The build command** (used by every "build" step; keeps 6 threads free per the user's machine rules):
  ```powershell
  cd C:\Users\Tera\Documents\GitHub\ReXGlue080FM4\FM4
  $p = Start-Process cmake -ArgumentList '--build','out/build/win-amd64-relwithdebinfo','--target','fm4' -PassThru -NoNewWindow
  $p.ProcessorAffinity = 0x03FFFFFF; $p.PriorityClass = 'BelowNormal'; $p.WaitForExit(); $p.ExitCode
  ```
  Expected: `0`. One build at a time.
- **The run command:** `FM4\out\build\win-amd64-relwithdebinfo\fm4.exe` from that directory; it reads `fm4.toml` beside it (copied by POST_BUILD) and writes `logs\fm4_NNN.log` (highest NNN is the newest).
- The existing native override of `sub_826E8358` (`D3D::CBlocker::Check`) in `FM4/src/fm4_midasm_hooks.cpp` must keep its `sub_826E8358` symbol: do not add a name for 0x826E8358 in any config.
- Hook sources are compiled directly into the `fm4` executable (`FM4/CMakeLists.txt:14-26`). Never move them into a STATIC library: an archive drops the `REX_HOOK` registration symbols.
- `FM4/` is untracked in git. Commit only the files each task names; never `FM4/assets`, `FM4/out`, `FM4/logs`, `FM4/.vs`, `FM4/generated`, `FM4/San`.
- Guest addresses below were verified in ida40 on 2026-09-02 (imagebase 0x82000000, NTSCU DVD1 `default.xex`).

---

## File structure

| file | responsibility |
|---|---|
| `FM4/fm4_d3d_functions.toml` (new) | names for the D3D entry points so hooks are symbolic |
| `FM4/fm4_manifest.toml` (modify) | include the new TOML |
| `FM4/src/gpu/d3d_guest.h` (new) | pure constexpr helpers: shader container size, FNV-1a, PM4 sink geometry, device offsets; self-checked with `static_assert` |
| `FM4/src/gpu/fm4_d3d_trace.cpp` (new) | opt-in counters and shader-bytecode capture, runs under the xenos path |
| `FM4/src/gpu/native_video.h/.cpp` (new) | Plume device, swapchain, clear, present |
| `FM4/src/gpu/fm4_d3d_hooks.cpp` (new) | PM4 sink, GPU-wait no-ops, `Swap`, `Clear` |
| `FM4/src/fm4_app.h` (modify) | `native` selection, init, shutdown |
| `FM4/src/fm4_midasm_hooks.cpp` (modify) | `CBlocker::Check` returns "done" under native |
| `FM4/CMakeLists.txt`, `FM4/CMakePresets.json`, `FM4/fm4.toml` (modify) | sources, Plume link, option, config docs |
| `CMakeLists.txt`, `thirdparty/CMakeLists.txt`, `.gitmodules` (modify) | `REXGLUE_ENABLE_PLUME` option and the Plume submodule |

---

### Task 1: Name the FM4 D3D entry points

**Files:**
- Create: `FM4/fm4_d3d_functions.toml`
- Modify: `FM4/fm4_manifest.toml:11-13` (the `includes` list)

**Interfaces:**
- Consumes: nothing.
- Produces: generated symbols `D3DDevice_Swap`, `D3DDevice_Clear`, `Direct3D_CreateDevice`, `D3D_RingMakeSpace`, `D3D_RingFlush`, `D3D_RingSubmit`, `D3DDevice_BlockUntilIdle`, `D3D_WaitUntilIdleOrFlushCaches`, `D3DDevice_InsertFence`, `D3DDevice_BlockOnFence`, `D3DDevice_IsFencePending`, `D3DResource_BlockUntilNotBusy`, `D3D_CDevice_BlockOnSecondaryPosition`, `D3DDevice_DrawIndexedVertices`, `D3DDevice_DrawIndexedVerticesUP`, `D3DDevice_BeginVertices`, `D3DDevice_RunCommandBuffer`, `D3DDevice_BeginTiling`, `D3DDevice_Resolve`, `D3DDevice_SetRenderTarget`, `D3DDevice_CreateTexture`, `D3DDevice_CreateVertexShader`, `D3DDevice_CreatePixelShader`, each with a `__imp__` twin holding the original body, all with signature `void name(PPCContext& ctx, uint8_t* base)`.

- [ ] **Step 1: Write the names file**

`FM4/fm4_d3d_functions.toml`:

```toml
# FM4 Xbox 360 D3D9 entry points. Addresses verified in ida40 on 2026-09-02.
# Naming a function makes the generated symbol `name` (weak) with `__imp__name`
# keeping the original body, so title code can `extern "C" REX_FUNC(name)`.
# Do NOT name 0x826E8358 (D3D::CBlocker::Check): fm4_midasm_hooks.cpp overrides
# it as sub_826E8358.
[functions]
0x826E8AA0 = { name = "Direct3D_CreateDevice" }
0x822D0C98 = { name = "D3DDevice_Release" }
0x8237FA28 = { name = "D3DDevice_Swap" }
0x8236D840 = { name = "D3DDevice_Clear" }
0x826E4D20 = { name = "D3DDevice_BlockUntilIdle" }
0x82353F18 = { name = "D3D_WaitUntilIdleOrFlushCaches" }
0x822C5260 = { name = "D3DDevice_InsertFence" }
0x822B0858 = { name = "D3DDevice_BlockOnFence" }
0x823102D0 = { name = "D3DDevice_IsFencePending" }
0x826E4768 = { name = "D3DResource_BlockUntilNotBusy" }
0x8234DA50 = { name = "D3D_CDevice_BlockOnSecondaryPosition" }
0x822FEF08 = { name = "D3D_RingMakeSpace" }
0x822FED80 = { name = "D3D_RingFlush" }
0x826E4848 = { name = "D3D_RingSubmit" }
0x82311080 = { name = "D3DDevice_DrawIndexedVertices" }
0x822D42A8 = { name = "D3DDevice_DrawIndexedVerticesUP" }
0x8234D278 = { name = "D3DDevice_BeginVertices" }
0x82310818 = { name = "D3DDevice_RunCommandBuffer" }
0x82374660 = { name = "D3DDevice_BeginTiling" }
0x822E2120 = { name = "D3DDevice_Resolve" }
0x823563B8 = { name = "D3DDevice_SetRenderTarget" }
0x822D9968 = { name = "D3DDevice_SetDepthStencilSurface" }
0x826DF128 = { name = "D3DDevice_CreateTexture" }
0x826DF248 = { name = "D3DDevice_CreateSurface" }
0x826E7088 = { name = "D3DDevice_CreateVertexShader" }
0x826E6EA0 = { name = "D3DDevice_CreatePixelShader" }
0x8233A9A8 = { name = "D3DDevice_SetTexture" }
0x823445A0 = { name = "D3DDevice_SetStreamSource" }
0x8236E3B8 = { name = "D3DDevice_SetIndices" }
0x82351A00 = { name = "D3DDevice_SetVertexShader" }
0x82351BD0 = { name = "D3DDevice_SetPixelShader" }
0x822FEAA8 = { name = "D3DDevice_SetViewport" }
0x822FF050 = { name = "D3DDevice_SetScissorRect" }
```

- [ ] **Step 2: Include it from the manifest**

In `FM4/fm4_manifest.toml`, change:

```toml
includes = [
  "fm4_config.toml",
]
```

to:

```toml
includes = [
  "fm4_config.toml",
  "fm4_d3d_functions.toml",
]
```

- [ ] **Step 3: Regenerate**

Run: `cd C:\Users\Tera\Documents\GitHub\ReXGlue080FM4\FM4; ..\out\win-amd64\rexglue.exe codegen fm4_manifest.toml --force`
Expected: exits 0, no `[config]` error lines mentioning `fm4_d3d_functions.toml`.

- [ ] **Step 4: Verify the symbols exist**

Run: `Select-String -Path FM4\generated\default\*.h -Pattern 'DECLARE_REX_FUNC\(D3DDevice_Swap\)' | Select-Object -First 1`
Expected: one hit. Also run `Select-String -Path FM4\generated\default\*.cpp -Pattern 'DEFINE_REX_FUNC\(D3D_RingMakeSpace\)'` and expect one hit.

- [ ] **Step 5: Build and confirm no regression under the xenos path**

Run the build command. Expected: `0`.
Run the run command, reach the title screen, close the game. Expected: `logs\fm4_NNN.log` contains no `LNK` or `unresolved` text and the game behaves as before.

- [ ] **Step 6: Commit**

```powershell
cd C:\Users\Tera\Documents\GitHub\ReXGlue080FM4
git add FM4/fm4_d3d_functions.toml FM4/fm4_manifest.toml
git commit -m "fm4: name the D3D9 entry points for symbolic hooks"
```

---

### Task 2: D3D trace and shader capture under the xenos path

**Files:**
- Create: `FM4/src/gpu/d3d_guest.h`
- Create: `FM4/src/gpu/fm4_d3d_trace.cpp`
- Modify: `FM4/CMakeLists.txt:14-20` (`FM4_SOURCES`), `FM4/fm4.toml`

**Interfaces:**
- Consumes: the generated `__imp__*` symbols from Task 1; `REXCVAR_DEFINE_BOOL(name, default, category, desc)` / `REXCVAR_GET(name)` from `include/rex/cvar.h:343,356`.
- Produces: `fm4::gpu::ShaderContainerBytes(uint32_t, uint32_t)`, `fm4::gpu::Fnv1a64(const uint8_t*, size_t)`, `fm4::gpu::kRingSinkBytes`, `fm4::gpu::RingSinkLimit(uint32_t)`, `fm4::gpu::RingSinkGuarantee(uint32_t)`, `fm4::gpu::kDevRing/kDevRingLimit/kDevRingGuarantee`; the cvar `fm4_d3d_trace`; shader dumps in `<cwd>\shaders\<hash>.vso|.pso`.

- [ ] **Step 1: Write the pure helpers with their compile-time checks**

`FM4/src/gpu/d3d_guest.h`:

```cpp
// Pure helpers shared by the trace and the native hooks. Everything here is
// constexpr and self-checked with static_assert; no runtime test target needed.
#pragma once

#include <cstddef>
#include <cstdint>

#include <rex/types.h>

namespace fm4::gpu {

// Xenos shader container header: dword[1] = virtualSize, dword[2] =
// physicalSize. XenosRecomp hashes exactly virtualSize + physicalSize bytes
// from the container start, so a dump of that many bytes is what it expects.
constexpr uint32_t ShaderContainerBytes(uint32_t virtual_size, uint32_t physical_size) {
  return virtual_size + physical_size;
}
inline uint32_t ShaderContainerBytes(const rex::be<uint32_t>* words) {
  return ShaderContainerBytes(static_cast<uint32_t>(words[1]), static_cast<uint32_t>(words[2]));
}

// File-name hash only; XenosRecomp computes its own content hash.
constexpr uint64_t Fnv1a64(const uint8_t* p, size_t n) {
  uint64_t h = 1469598103934665603ull;
  for (size_t i = 0; i < n; ++i) {
    h ^= p[i];
    h *= 1099511628211ull;
  }
  return h;
}

// Public D3DDevice ring fields (ida40 type_inspect D3DDevice, 2026-09-02).
constexpr uint32_t kDevRing = 0x30;           // m_pRing: next write
constexpr uint32_t kDevRingLimit = 0x34;      // m_pRingLimit
constexpr uint32_t kDevRingGuarantee = 0x38;  // m_pRingGuarantee: "make space" when m_pRing exceeds this

// PM4 sink: a host-allocated guest buffer the D3D library writes packets into
// and nobody reads. D3D_RingMakeSpace resets m_pRing to the base, so the
// guarantee window must cover the largest burst the library writes between two
// guarantee checks. 64 KiB is 16x the largest burst seen in the XDK library.
constexpr uint32_t kRingSinkBytes = 1u << 20;
constexpr uint32_t kRingSinkGuaranteeBytes = 64u << 10;
constexpr uint32_t RingSinkLimit(uint32_t base) { return base + kRingSinkBytes; }
constexpr uint32_t RingSinkGuarantee(uint32_t base) {
  return base + kRingSinkBytes - kRingSinkGuaranteeBytes;
}

// Self-checks.
static_assert(ShaderContainerBytes(0x100, 0x40) == 0x140);
static_assert(RingSinkGuarantee(0x1000) < RingSinkLimit(0x1000));
static_assert(RingSinkGuarantee(0x1000) > 0x1000);
constexpr uint8_t kFnvProbe[3] = {'a', 'b', 'c'};
static_assert(Fnv1a64(kFnvProbe, 3) == 0xE71FA2190541574Bull);  // FNV-1a("abc")

}  // namespace fm4::gpu
```

- [ ] **Step 2: Write the trace hooks**

`FM4/src/gpu/fm4_d3d_trace.cpp`:

```cpp
// Opt-in D3D call trace (fm4_d3d_trace = true in fm4.toml). Counts the entry
// points the native renderer must emulate and dumps every shader container the
// game creates, so the shader cache for the native path can be built offline.
// Runs under the xenos plugin; every hook forwards to the original body.
#include "generated/default/fm4_init.h"

#include <array>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include <rex/cvar.h>
#include <rex/logging.h>

#include "gpu/d3d_guest.h"

REXCVAR_DEFINE_BOOL(fm4_d3d_trace, false, "FM4",
                    "Count D3D entry points per 300 frames and dump shader bytecode to <cwd>/shaders");

namespace {

enum Counter : size_t {
  kSwap,
  kDrawIndexed,
  kDrawIndexedUP,
  kBeginVertices,
  kRunCommandBuffer,
  kBeginTiling,
  kResolve,
  kSetRenderTargetNonZero,
  kCreateTexture,
  kCreateVS,
  kCreatePS,
  kCount
};

constexpr std::array<const char*, kCount> kNames = {
    "swap",       "drawIdx",   "drawIdxUP", "beginVerts", "runCB", "beginTiling",
    "resolve",    "rtIdxNon0", "createTex", "createVS",   "createPS"};

std::array<std::atomic<uint32_t>, kCount> g_counters{};
std::atomic<uint32_t> g_resolve_flags{0};
std::atomic<uint32_t> g_frames{0};

bool Enabled() { return REXCVAR_GET(fm4_d3d_trace); }

void Bump(Counter c) { g_counters[c].fetch_add(1, std::memory_order_relaxed); }

void LogAndReset() {
  char line[512];
  int n = std::snprintf(line, sizeof(line), "[d3dtrace] frames=300");
  for (size_t i = 0; i < kCount; ++i) {
    n += std::snprintf(line + n, sizeof(line) - n, " %s=%u", kNames[i],
                       g_counters[i].exchange(0, std::memory_order_relaxed));
  }
  std::snprintf(line + n, sizeof(line) - n, " resolveFlags=0x%08X",
                g_resolve_flags.exchange(0, std::memory_order_relaxed));
  REXLOG_INFO("{}", line);
}

void DumpShader(uint8_t* base, uint32_t function_va, const char* ext) {
  const auto* words = reinterpret_cast<const rex::be<uint32_t>*>(base + function_va);
  const uint32_t bytes = fm4::gpu::ShaderContainerBytes(words);
  if (bytes < 12 || bytes > (1u << 20)) {
    REXLOG_WARN("[d3dtrace] shader at 0x{:08X} has implausible size {}", function_va, bytes);
    return;
  }
  const uint64_t hash = fm4::gpu::Fnv1a64(base + function_va, bytes);
  const auto dir = std::filesystem::current_path() / "shaders";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  char name[64];
  std::snprintf(name, sizeof(name), "%016llX.%s", static_cast<unsigned long long>(hash), ext);
  const auto path = dir / name;
  if (std::filesystem::exists(path, ec)) {
    return;
  }
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(base + function_va), bytes);
  REXLOG_INFO("[d3dtrace] dumped {} ({} bytes)", name, bytes);
}

}  // namespace

// D3DDevice_Swap(pDevice, pFrontBuffer, pParameters): one per presented frame.
extern "C" REX_FUNC(D3DDevice_Swap) {
  if (Enabled()) {
    Bump(kSwap);
    if ((g_frames.fetch_add(1, std::memory_order_relaxed) % 300) == 299) {
      LogAndReset();
    }
  }
  __imp__D3DDevice_Swap(ctx, base);
}

extern "C" REX_FUNC(D3DDevice_DrawIndexedVertices) {
  if (Enabled()) Bump(kDrawIndexed);
  __imp__D3DDevice_DrawIndexedVertices(ctx, base);
}

extern "C" REX_FUNC(D3DDevice_DrawIndexedVerticesUP) {
  if (Enabled()) Bump(kDrawIndexedUP);
  __imp__D3DDevice_DrawIndexedVerticesUP(ctx, base);
}

extern "C" REX_FUNC(D3DDevice_BeginVertices) {
  if (Enabled()) Bump(kBeginVertices);
  __imp__D3DDevice_BeginVertices(ctx, base);
}

extern "C" REX_FUNC(D3DDevice_RunCommandBuffer) {
  if (Enabled()) Bump(kRunCommandBuffer);
  __imp__D3DDevice_RunCommandBuffer(ctx, base);
}

extern "C" REX_FUNC(D3DDevice_BeginTiling) {
  if (Enabled()) Bump(kBeginTiling);
  __imp__D3DDevice_BeginTiling(ctx, base);
}

// D3DDevice_Resolve(pDevice, Flags, ...): r4 = Flags (D3DRESOLVE_*).
extern "C" REX_FUNC(D3DDevice_Resolve) {
  if (Enabled()) {
    Bump(kResolve);
    g_resolve_flags.fetch_or(ctx.r4.u32, std::memory_order_relaxed);
  }
  __imp__D3DDevice_Resolve(ctx, base);
}

// D3DDevice_SetRenderTarget(pDevice, RenderTargetIndex, pSurface): r4 = index.
extern "C" REX_FUNC(D3DDevice_SetRenderTarget) {
  if (Enabled() && ctx.r4.u32 != 0) Bump(kSetRenderTargetNonZero);
  __imp__D3DDevice_SetRenderTarget(ctx, base);
}

extern "C" REX_FUNC(D3DDevice_CreateTexture) {
  if (Enabled()) Bump(kCreateTexture);
  __imp__D3DDevice_CreateTexture(ctx, base);
}

// D3DDevice_CreateVertexShader(const DWORD* pFunction): r3 = container.
extern "C" REX_FUNC(D3DDevice_CreateVertexShader) {
  if (Enabled()) {
    Bump(kCreateVS);
    DumpShader(base, ctx.r3.u32, "vso");
  }
  __imp__D3DDevice_CreateVertexShader(ctx, base);
}

// D3DDevice_CreatePixelShader(const DWORD* pFunction): r3 = container.
extern "C" REX_FUNC(D3DDevice_CreatePixelShader) {
  if (Enabled()) {
    Bump(kCreatePS);
    DumpShader(base, ctx.r3.u32, "pso");
  }
  __imp__D3DDevice_CreatePixelShader(ctx, base);
}
```

- [ ] **Step 3: Add the sources and the config line**

In `FM4/CMakeLists.txt` change the `FM4_SOURCES` block to:

```cmake
set(FM4_SOURCES
    src/main.cpp
    src/fm4_guest_pc_fiber.cpp
    src/fm4_debug_media.cpp
    src/fm4_debug_audio_mix.cpp
    src/fm4_midasm_hooks.cpp
    src/gpu/fm4_d3d_trace.cpp
)
```

and add `${CMAKE_CURRENT_SOURCE_DIR}/src` to the existing `target_include_directories(fm4 PRIVATE ...)` list so `#include "gpu/d3d_guest.h"` resolves.

Append to `FM4/fm4.toml`:

```toml
# Count D3D entry points and dump shader bytecode to <cwd>/shaders (xenos path).
fm4_d3d_trace = true
```

- [ ] **Step 4: Build**

Run the build command. Expected: `0`. A failure on the `static_assert` line for `Fnv1a64` means the probe constant is wrong for this compiler's `constexpr` evaluation; fix the constant, not the function.

- [ ] **Step 5: Run and verify the trace**

Run the run command, reach the main menu, start one race, quit after the first lap.
Expected in the newest `logs\fm4_NNN.log`: lines matching `[d3dtrace] frames=300 swap=300 ...` every 300 presented frames, and `[d3dtrace] dumped ....vso`/`.pso` lines.
Run: `(Get-ChildItem FM4\out\build\win-amd64-relwithdebinfo\shaders).Count`
Expected: greater than 0.

Record from the log, for the deep dive's open questions: `runCB` versus `drawIdx` per 300 frames (command-buffer share), `beginTiling` (tiling in use), `rtIdxNon0` (MRT in use), and `resolveFlags`.

- [ ] **Step 6: Commit**

```powershell
git add FM4/src/gpu/d3d_guest.h FM4/src/gpu/fm4_d3d_trace.cpp FM4/CMakeLists.txt FM4/fm4.toml
git commit -m "fm4: opt-in D3D call trace and shader bytecode capture"
```

---

### Task 3: Vendor Plume behind an SDK option

**Files:**
- Modify: `.gitmodules`, `thirdparty/CMakeLists.txt:31-60` (the `REQUIRED_SUBMODULES` block), `CMakeLists.txt` (next to the existing `option(REXGLUE_ENABLE_TRACY ...)` line), `FM4/CMakePresets.json` (the `windows-amd64-base` cache variables)

**Interfaces:**
- Consumes: nothing.
- Produces: CMake option `REXGLUE_ENABLE_PLUME` (default `OFF`) and the `plume` library target with `plume_render_interface.h` on its public include path.

- [ ] **Step 1: Add the submodule at reblue's pin**

```powershell
cd C:\Users\Tera\Documents\GitHub\ReXGlue080FM4
git submodule add https://github.com/zolaware/plume.git thirdparty/plume
git -C thirdparty/plume checkout de0f70f
```

Expected: `thirdparty/plume/plume_render_interface.h` exists and `git -C thirdparty/plume log -1 --oneline` shows `de0f70f`.

- [ ] **Step 2: Read Plume's own options before wiring them**

Run: `Select-String -Path thirdparty\plume\CMakeLists.txt -Pattern 'option\(|find_package\('`
Expected: options named like `PLUME_D3D12_ENABLED`, `PLUME_VULKAN_ENABLED`, `PLUME_D3D12_AGILITY_SDK_ENABLED`, `PLUME_SDL_VULKAN_ENABLED`. If a `find_package(directx-headers ...)` or `find_package(directx12-agility ...)` appears outside an Agility-guarded block, stop and report: this SDK has no vcpkg, so that dependency has to be vendored first.

- [ ] **Step 3: Add the SDK option**

In the root `CMakeLists.txt`, directly after the `option(REXGLUE_ENABLE_TRACY ...)` line, add:

```cmake
option(REXGLUE_ENABLE_PLUME "Build the Plume render abstraction for titles that render natively" OFF)
```

- [ ] **Step 4: Wire the submodule**

In `thirdparty/CMakeLists.txt`, next to the block that does `if(REXGLUE_ENABLE_TRACY) list(APPEND REQUIRED_SUBMODULES tracy) endif()`, add:

```cmake
if(REXGLUE_ENABLE_PLUME)
    list(APPEND REQUIRED_SUBMODULES plume)
endif()
```

and at the end of the file add:

```cmake
#=============================================================================
# Plume (native-render titles only)
#=============================================================================
if(REXGLUE_ENABLE_PLUME)
    # System D3D12 runtime is enough for milestone 1; the Agility SDK needs a
    # vcpkg directx12-agility package this tree does not carry.
    set(PLUME_D3D12_AGILITY_SDK_ENABLED OFF CACHE BOOL "" FORCE)
    set(PLUME_D3D12_ENABLED ON CACHE BOOL "" FORCE)
    set(PLUME_VULKAN_ENABLED OFF CACHE BOOL "" FORCE)
    add_subdirectory(plume EXCLUDE_FROM_ALL)
endif()
```

If Step 2 showed different option names, use those names with the same values.

- [ ] **Step 5: Enable it for the FM4 tree**

In `FM4/CMakePresets.json`, in the `cacheVariables` of `windows-amd64-base` (the object that sets `CMAKE_CXX_COMPILER` at line 61), add:

```json
"REXGLUE_ENABLE_PLUME": "ON"
```

- [ ] **Step 6: Reconfigure and build the Plume target**

```powershell
cd C:\Users\Tera\Documents\GitHub\ReXGlue080FM4\FM4
cmake --preset win-amd64-relwithdebinfo
$p = Start-Process cmake -ArgumentList '--build','out/build/win-amd64-relwithdebinfo','--target','plume' -PassThru -NoNewWindow
$p.ProcessorAffinity = 0x03FFFFFF; $p.PriorityClass = 'BelowNormal'; $p.WaitForExit(); $p.ExitCode
```

Expected: configure prints no error mentioning `plume`, and the build exits `0` producing `plume.lib` under `FM4\out\build\win-amd64-relwithdebinfo`.

- [ ] **Step 7: Commit**

```powershell
cd C:\Users\Tera\Documents\GitHub\ReXGlue080FM4
git add .gitmodules thirdparty/plume CMakeLists.txt thirdparty/CMakeLists.txt FM4/CMakePresets.json
git commit -m "build: vendor Plume behind REXGLUE_ENABLE_PLUME"
```

---

### Task 4: Plume device, swapchain and present, selected by `gpu_plugin = "native"`

**Files:**
- Create: `FM4/src/gpu/native_video.h`, `FM4/src/gpu/native_video.cpp`
- Modify: `FM4/src/fm4_app.h:23-30`, `FM4/CMakeLists.txt` (`FM4_SOURCES` and a `target_link_libraries`), `FM4/fm4.toml:3-4`

**Interfaces:**
- Consumes: `plume::CreateD3D12Interface()`, `plume::RenderInterface::createDevice()`, `RenderDevice::createCommandQueue/createCommandFence/createCommandSemaphore/createFramebuffer`, `RenderCommandQueue::createCommandList/createSwapChain/executeCommandLists/waitForCommandFence`, `RenderSwapChain::getTextureCount/getTexture/isEmpty/needsResize/resize/acquireTexture/present/setVsyncEnabled`, `RenderCommandList::begin/end/barriers/setFramebuffer/clearColor`; `rex::ui::Window::GetNativeWindowHandle()` (`include/rex/ui/window.h:291`, implemented in `src/ui/window_sdl.cpp:231`); `rex::ReXApp::OnPreSetup(RuntimeConfig&)`, `OnPostSetup()`, `OnShutdown()`, `window()` (`include/rex/rex_app.h:99,105,111,250`).
- Produces:
  ```cpp
  namespace fm4::gpu {
  void SetNativeRequested(bool on);
  bool NativeRequested();
  class Video {
   public:
    static bool Init(rex::ui::Window* window);  // Plume device; swapchain when the HWND exists, else on first Present
    static void Shutdown();
    static void RequestClear(uint32_t argb);    // next Present clears the back buffer to this colour
    static void Present();                       // acquire, clear, present; no-op before Init
    static uint64_t PresentedFrames();
  };
  }
  ```

- [ ] **Step 1: Write the header**

`FM4/src/gpu/native_video.h`:

```cpp
// Milestone 1 native video: a Plume D3D12 device that clears and presents.
// Draw emulation arrives in later milestones; this file must stay small.
#pragma once

#include <cstdint>

namespace rex::ui {
class Window;
}

namespace fm4::gpu {

// Set from Fm4App::OnPreSetup when fm4.toml selects gpu_plugin = "native".
void SetNativeRequested(bool on);
bool NativeRequested();

class Video {
 public:
  // Creates the Plume interface, device, queue and per-frame objects. The
  // swapchain is created here if the window already has a native handle and
  // otherwise on the first Present. Returns false if the device cannot be made.
  static bool Init(rex::ui::Window* window);
  static void Shutdown();

  // The next Present clears the back buffer to this D3DCOLOR (ARGB8).
  static void RequestClear(uint32_t argb);

  // Acquire a swapchain image, clear it, present. Safe to call from any guest
  // thread; no-op until Init succeeded.
  static void Present();

  static uint64_t PresentedFrames();
};

}  // namespace fm4::gpu
```

- [ ] **Step 2: Write the implementation**

`FM4/src/gpu/native_video.cpp`:

```cpp
#include "gpu/native_video.h"

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include <plume_render_interface.h>
#include <rex/logging.h>
#include <rex/ui/window.h>

namespace plume {
extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
}

namespace fm4::gpu {
namespace {

constexpr uint32_t kNumFrames = 2;

std::atomic<bool> g_native_requested{false};

struct State {
  std::mutex mutex;
  rex::ui::Window* window = nullptr;
  std::unique_ptr<plume::RenderInterface> iface;
  std::unique_ptr<plume::RenderDevice> device;
  std::unique_ptr<plume::RenderCommandQueue> queue;
  std::array<std::unique_ptr<plume::RenderCommandList>, kNumFrames> lists;
  std::array<std::unique_ptr<plume::RenderCommandFence>, kNumFrames> fences;
  std::array<bool, kNumFrames> submitted{};
  std::array<std::unique_ptr<plume::RenderCommandSemaphore>, kNumFrames> acquire;
  std::vector<std::unique_ptr<plume::RenderCommandSemaphore>> render;  // one per swapchain image
  std::unique_ptr<plume::RenderSwapChain> swap_chain;
  std::vector<std::unique_ptr<plume::RenderFramebuffer>> framebuffers;
  uint32_t frame = 0;
  plume::RenderColor clear_color{};
  bool clear_logged_once = false;
  std::atomic<uint64_t> presented{0};
};

State& S() {
  static State s;
  return s;
}

plume::RenderColor MakeColor(float r, float g, float b, float a) {
  plume::RenderColor c{};
  c.rgba[0] = r;
  c.rgba[1] = g;
  c.rgba[2] = b;
  c.rgba[3] = a;
  return c;
}

void WaitAllSubmitted(State& s) {
  for (uint32_t i = 0; i < kNumFrames; ++i) {
    if (s.submitted[i]) {
      s.queue->waitForCommandFence(s.fences[i].get());
      s.submitted[i] = false;
    }
  }
}

bool BuildFramebuffers(State& s) {
  s.framebuffers.clear();
  s.render.clear();
  const uint32_t count = s.swap_chain->getTextureCount();
  for (uint32_t i = 0; i < count; ++i) {
    const plume::RenderTexture* color[1] = {s.swap_chain->getTexture(i)};
    plume::RenderFramebufferDesc desc(color, 1);
    auto fb = s.device->createFramebuffer(desc);
    if (!fb) {
      REXLOG_ERROR("native gpu: createFramebuffer failed for image {}", i);
      s.framebuffers.clear();
      return false;
    }
    s.framebuffers.push_back(std::move(fb));
    s.render.push_back(s.device->createCommandSemaphore());
  }
  return true;
}

// Needs the window's native handle, which does not exist when Init runs
// during SDK presentation setup; hence lazy.
bool BuildSwapChain(State& s) {
  if (!s.window) {
    return false;
  }
  auto handle = static_cast<plume::RenderWindow>(s.window->GetNativeWindowHandle());
  if (!handle) {
    return false;
  }
  // kNumFrames + 1: a flip-model swapchain needs one image beyond the frames
  // in flight so acquiring never waits on scanout.
  plume::RenderSwapChainDesc desc(handle, plume::RenderFormat::B8G8R8A8_UNORM, kNumFrames + 1);
  s.swap_chain = s.queue->createSwapChain(desc);
  if (!s.swap_chain || s.swap_chain->isEmpty()) {
    REXLOG_ERROR("native gpu: createSwapChain failed");
    s.swap_chain.reset();
    return false;
  }
  s.swap_chain->setVsyncEnabled(true);
  if (!BuildFramebuffers(s)) {
    s.swap_chain.reset();
    return false;
  }
  REXLOG_INFO("native gpu: swapchain {}x{} with {} images", s.swap_chain->getWidth(),
              s.swap_chain->getHeight(), s.swap_chain->getTextureCount());
  return true;
}

}  // namespace

void SetNativeRequested(bool on) { g_native_requested.store(on); }
bool NativeRequested() { return g_native_requested.load(); }

bool Video::Init(rex::ui::Window* window) {
  auto& s = S();
  std::lock_guard lock(s.mutex);
  s.window = window;
  if (s.device) {
    return true;
  }
  s.iface = plume::CreateD3D12Interface();
  if (!s.iface) {
    REXLOG_ERROR("native gpu: CreateD3D12Interface failed");
    return false;
  }
  s.device = s.iface->createDevice();
  if (!s.device) {
    REXLOG_ERROR("native gpu: createDevice failed");
    return false;
  }
  s.queue = s.device->createCommandQueue(plume::RenderCommandListType::DIRECT);
  for (uint32_t i = 0; i < kNumFrames; ++i) {
    s.lists[i] = s.queue->createCommandList();
    s.fences[i] = s.device->createCommandFence();
    s.acquire[i] = s.device->createCommandSemaphore();
  }
  s.clear_color = MakeColor(1.0f, 0.0f, 1.0f, 1.0f);  // magenta until the guest clears
  REXLOG_INFO("native gpu: D3D12 device '{}'", s.device->getDescription().name);
  BuildSwapChain(s);  // may legitimately fail here; retried on first Present
  return true;
}

void Video::Shutdown() {
  auto& s = S();
  std::lock_guard lock(s.mutex);
  if (!s.device) {
    return;
  }
  WaitAllSubmitted(s);
  s.framebuffers.clear();
  s.render.clear();
  s.swap_chain.reset();
  // Device, queue, lists and fences are leaked on purpose: Plume's device
  // release does not track children, and the process is exiting.
  s.window = nullptr;
}

void Video::RequestClear(uint32_t argb) {
  auto& s = S();
  std::lock_guard lock(s.mutex);
  s.clear_color = MakeColor(((argb >> 16) & 0xFF) / 255.0f, ((argb >> 8) & 0xFF) / 255.0f,
                            (argb & 0xFF) / 255.0f, ((argb >> 24) & 0xFF) / 255.0f);
  if (!s.clear_logged_once) {
    s.clear_logged_once = true;
    REXLOG_INFO("native gpu: first guest clear colour 0x{:08X}", argb);
  }
}

void Video::Present() {
  auto& s = S();
  std::lock_guard lock(s.mutex);
  if (!s.device) {
    return;
  }
  if (!s.swap_chain && !BuildSwapChain(s)) {
    return;
  }
  if (s.swap_chain->needsResize()) {
    WaitAllSubmitted(s);
    s.framebuffers.clear();
    s.swap_chain->resize();
    if (s.swap_chain->isEmpty() || !BuildFramebuffers(s)) {
      return;  // minimised: nothing to draw into
    }
  }

  uint32_t image = 0;
  if (!s.swap_chain->acquireTexture(s.acquire[s.frame].get(), &image)) {
    return;
  }
  plume::RenderTexture* back = s.swap_chain->getTexture(image);
  plume::RenderCommandList* list = s.lists[s.frame].get();

  list->begin();
  list->barriers(plume::RenderBarrierStage::GRAPHICS,
                 plume::RenderTextureBarrier(back, plume::RenderTextureLayout::COLOR_WRITE));
  list->setFramebuffer(s.framebuffers[image].get());
  list->clearColor(0, s.clear_color);
  list->setFramebuffer(nullptr);
  list->barriers(plume::RenderBarrierStage::GRAPHICS,
                 plume::RenderTextureBarrier(back, plume::RenderTextureLayout::PRESENT));
  list->end();

  const plume::RenderCommandList* lists[] = {list};
  plume::RenderCommandSemaphore* waits[] = {s.acquire[s.frame].get()};
  plume::RenderCommandSemaphore* signals[] = {s.render[image].get()};
  s.queue->executeCommandLists(lists, 1, waits, 1, signals, 1, s.fences[s.frame].get());
  s.submitted[s.frame] = true;
  s.swap_chain->present(image, signals, 1);

  // Advance, then wait for the slot about to be reused (one frame old). That
  // gap is the CPU/GPU overlap.
  s.frame = (s.frame + 1) % kNumFrames;
  if (s.submitted[s.frame]) {
    s.queue->waitForCommandFence(s.fences[s.frame].get());
    s.submitted[s.frame] = false;
  }
  s.presented.fetch_add(1, std::memory_order_relaxed);
}

uint64_t Video::PresentedFrames() { return S().presented.load(std::memory_order_relaxed); }

}  // namespace fm4::gpu
```

- [ ] **Step 3: Wire the app**

In `FM4/src/fm4_app.h`, add `#include "gpu/native_video.h"` and `#include <rex/logging.h>` after the existing includes, and replace the commented override list (lines 23-30) with:

```cpp
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
      REXLOG_ERROR("fm4: native GPU init failed; the guest will hang at Direct3D_CreateDevice");
    }
  }

  void OnShutdown() override { fm4::gpu::Video::Shutdown(); }
```

Keep `OnConfigurePaths` as it is.

- [ ] **Step 4: Sources, link and config**

In `FM4/CMakeLists.txt` add `src/gpu/native_video.cpp` to `FM4_SOURCES`, and after `rexglue_setup_target(fm4 GPU_PLUGINS xenos)` add:

```cmake
# Plume is only reachable when the SDK was configured with REXGLUE_ENABLE_PLUME
# (FM4/CMakePresets.json sets it). It is linked, not runtime-loaded.
target_link_libraries(fm4 PRIVATE plume)
```

Replace lines 3-4 of `FM4/fm4.toml` with:

```toml
# GPU path: "xenos" = SDK GPU emulation plugin (default), "native" = FM4's own
# Plume renderer (milestone 1: clear + present only; draws are not rendered).
gpu_plugin = "native"
```

- [ ] **Step 5: Temporary self-test of the present loop**

At the end of `OnPostSetup` (inside the `NativeRequested()` branch, after `Init`), temporarily add:

```cpp
    for (int i = 0; i < 3; ++i) fm4::gpu::Video::Present();
    REXLOG_INFO("fm4: native self-test presented {} frames", fm4::gpu::Video::PresentedFrames());
```

- [ ] **Step 6: Build and run**

Run the build command. Expected: `0`.
Run the run command. Expected in the newest log, in this order: `fm4: native GPU path selected`, `native gpu: D3D12 device '...'`, `fm4: window at OnPostSetup: present`, `native gpu: swapchain 1280x720 with 3 images`, `fm4: native self-test presented 3 frames`. The window flashes magenta. The guest then stalls inside `Direct3D_CreateDevice` (it waits for a GPU that does not exist); that is the expected end state of this task, close the window.

If the log says `window at OnPostSetup: null`, the SDK created the window after `OnPostSetup`; in that case move the `Video::Init(window())` call into `OnCreateDialogs(rex::ui::ImGuiDrawer*)` (declared in `include/rex/rex_app.h`, runs after the window exists) and rerun this step.

- [ ] **Step 7: Remove the self-test and rebuild**

Delete the two lines from Step 5. Run the build command. Expected: `0`.

- [ ] **Step 8: Commit**

```powershell
git add FM4/src/gpu/native_video.h FM4/src/gpu/native_video.cpp FM4/src/fm4_app.h FM4/CMakeLists.txt FM4/fm4.toml
git commit -m "fm4: Plume D3D12 device and present loop behind gpu_plugin = native"
```

---

### Task 5: Run the real D3D library against a PM4 sink

**Files:**
- Create: `FM4/src/gpu/fm4_d3d_hooks.cpp`
- Modify: `FM4/src/fm4_midasm_hooks.cpp:64-70` (the `REX_FUNC(sub_826E8358)` override), `FM4/src/gpu/fm4_d3d_trace.cpp`, `FM4/src/gpu/d3d_guest.h`, `FM4/CMakeLists.txt` (`FM4_SOURCES`)

**Interfaces:**
- Consumes: Task 1 symbols; `fm4::gpu::NativeRequested()`, `fm4::gpu::Video::Present()` from Task 4; `fm4::gpu::kDevRing/kDevRingLimit/kDevRingGuarantee/kRingSinkBytes/RingSinkLimit/RingSinkGuarantee` from Task 2; `rex::system::kernel_state()->memory()->SystemHeapAlloc(size, alignment)` (`include/rex/system/xmemory.h:517`).
- Produces: `fm4::gpu::TraceOnSwap()` (declared in `d3d_guest.h`, defined in `fm4_d3d_trace.cpp`); the guest device runs with `m_pRing` inside a host sink; every GPU wait returns immediately; `D3DDevice_Swap` presents through Plume.

Guest facts this task relies on (ida40):
- `Direct3D_CreateDevice(a1, DeviceType, a3, BehaviorFlags, pPresentParams, D3DDevice** ppDevice)`: `ppDevice` is r8; returns 0 on success.
- `D3D_RingMakeSpace(CDevice*)` (`sub_822FEF08`) flushes and returns the new `m_pRing`; callers do `if (m_pRing > m_pRingGuarantee) m_pRing = D3D_RingMakeSpace(dev)`.
- `D3D::CBlocker::Check()` is polled as `while (cond && Check()) ;`, so returning 0 ends every GPU wait.

- [ ] **Step 1: Move the Swap trace out of the trace file**

Two strong definitions of `D3DDevice_Swap` cannot link, and this task owns the hook. In `FM4/src/gpu/d3d_guest.h`, inside `namespace fm4::gpu`, add:

```cpp
// Implemented in fm4_d3d_trace.cpp: per-frame trace bookkeeping, no-op unless
// fm4_d3d_trace is set. Called by the D3DDevice_Swap hook in fm4_d3d_hooks.cpp.
void TraceOnSwap();
```

In `FM4/src/gpu/fm4_d3d_trace.cpp`, delete the whole `extern "C" REX_FUNC(D3DDevice_Swap) { ... }` block and add, after the anonymous namespace closes:

```cpp
void fm4::gpu::TraceOnSwap() {
  if (!Enabled()) {
    return;
  }
  Bump(kSwap);
  if ((g_frames.fetch_add(1, std::memory_order_relaxed) % 300) == 299) {
    LogAndReset();
  }
}
```

- [ ] **Step 2: Write the hooks**

`FM4/src/gpu/fm4_d3d_hooks.cpp`:

```cpp
// Native-path guest hooks, milestone 1. The real D3D library still builds and
// owns the guest D3DDevice; these hooks only remove the GPU from the picture:
// PM4 goes into a sink, GPU waits return at once, Swap presents through Plume.
// Every hook forwards to the original body when the native path is off, so the
// xenos plugin is unaffected.
#include "generated/default/fm4_init.h"

#include <atomic>

#include <rex/logging.h>
#include <rex/system/kernel_state.h>

#include "gpu/d3d_guest.h"
#include "gpu/native_video.h"

namespace {

bool Native() { return fm4::gpu::NativeRequested(); }

uint32_t GuestLoad32(uint8_t* base, uint32_t va) {
  return __builtin_bswap32(*reinterpret_cast<volatile uint32_t*>(base + va));
}

void GuestStore32(uint8_t* base, uint32_t va, uint32_t value) {
  *reinterpret_cast<volatile uint32_t*>(base + va) = __builtin_bswap32(value);
}

uint32_t g_sink_base = 0;  // guest VA of the PM4 sink, allocated once

uint32_t SinkBase() {
  if (g_sink_base == 0) {
    auto* memory = rex::system::kernel_state()->memory();
    g_sink_base = memory->SystemHeapAlloc(fm4::gpu::kRingSinkBytes, 0x100);
    REXLOG_INFO("native gpu: PM4 sink at 0x{:08X} ({} KiB)", g_sink_base,
                fm4::gpu::kRingSinkBytes >> 10);
  }
  return g_sink_base;
}

void PointRingAtSink(uint8_t* base, uint32_t device) {
  const uint32_t sink = SinkBase();
  GuestStore32(base, device + fm4::gpu::kDevRing, sink);
  GuestStore32(base, device + fm4::gpu::kDevRingLimit, fm4::gpu::RingSinkLimit(sink));
  GuestStore32(base, device + fm4::gpu::kDevRingGuarantee, fm4::gpu::RingSinkGuarantee(sink));
}

}  // namespace

// Let the library build the device, then repoint its ring at the sink.
extern "C" REX_FUNC(Direct3D_CreateDevice) {
  const uint32_t out_device_va = ctx.r8.u32;  // read before the call clobbers r8
  __imp__Direct3D_CreateDevice(ctx, base);
  if (!Native() || ctx.r3.u32 != 0) {
    return;
  }
  const uint32_t device = GuestLoad32(base, out_device_va);
  PointRingAtSink(base, device);
  REXLOG_INFO("native gpu: guest D3DDevice at 0x{:08X}, ring redirected to sink", device);
}

// Ring family: the sink never fills, so "make space" just rewinds it.
extern "C" REX_FUNC(D3D_RingMakeSpace) {
  if (!Native()) {
    __imp__D3D_RingMakeSpace(ctx, base);
    return;
  }
  const uint32_t device = ctx.r3.u32;
  const uint32_t sink = SinkBase();
  GuestStore32(base, device + fm4::gpu::kDevRing, sink);
  ctx.r3.u64 = sink;
}

extern "C" REX_FUNC(D3D_RingFlush) {
  if (!Native()) __imp__D3D_RingFlush(ctx, base);
}

extern "C" REX_FUNC(D3D_RingSubmit) {
  if (!Native()) __imp__D3D_RingSubmit(ctx, base);
}

// GPU waits: nothing to wait for.
extern "C" REX_FUNC(D3DDevice_BlockUntilIdle) {
  if (!Native()) __imp__D3DDevice_BlockUntilIdle(ctx, base);
}

extern "C" REX_FUNC(D3D_WaitUntilIdleOrFlushCaches) {
  if (!Native()) __imp__D3D_WaitUntilIdleOrFlushCaches(ctx, base);
}

extern "C" REX_FUNC(D3DDevice_BlockOnFence) {
  if (!Native()) __imp__D3DDevice_BlockOnFence(ctx, base);
}

extern "C" REX_FUNC(D3DResource_BlockUntilNotBusy) {
  if (!Native()) __imp__D3DResource_BlockUntilNotBusy(ctx, base);
}

extern "C" REX_FUNC(D3D_CDevice_BlockOnSecondaryPosition) {
  if (!Native()) __imp__D3D_CDevice_BlockOnSecondaryPosition(ctx, base);
}

// Fences: hand out increasing values and report every one as complete.
extern "C" REX_FUNC(D3DDevice_InsertFence) {
  if (!Native()) {
    __imp__D3DDevice_InsertFence(ctx, base);
    return;
  }
  static std::atomic<uint32_t> next_fence{1};
  ctx.r3.u64 = next_fence.fetch_add(1, std::memory_order_relaxed);
}

extern "C" REX_FUNC(D3DDevice_IsFencePending) {
  if (!Native()) {
    __imp__D3DDevice_IsFencePending(ctx, base);
    return;
  }
  ctx.r3.u64 = 0;
}

// D3DDevice_Swap(pDevice, pFrontBuffer, pParameters): present through Plume.
// The front buffer is ignored in milestone 1; the swapchain shows the clear.
extern "C" REX_FUNC(D3DDevice_Swap) {
  fm4::gpu::TraceOnSwap();
  if (!Native()) {
    __imp__D3DDevice_Swap(ctx, base);
    return;
  }
  fm4::gpu::Video::Present();
  ctx.r3.u64 = 0;
}
```

- [ ] **Step 3: Make the blocker return "done" under native**

In `FM4/src/fm4_midasm_hooks.cpp`, add `#include "gpu/native_video.h"` and change the start of the override to:

```cpp
extern "C" REX_FUNC(sub_826E8358) {
  if (fm4::gpu::NativeRequested()) {
    ctx.r3.u64 = 0;  // D3D::CBlocker::Check: 0 = stop waiting (callers loop while it returns nonzero)
    return;
  }
  const uint32_t wait = ctx.r3.u32;
```

- [ ] **Step 4: Add the source and build**

Add `src/gpu/fm4_d3d_hooks.cpp` to `FM4_SOURCES` in `FM4/CMakeLists.txt`. Run the build command. Expected: `0`. A duplicate-symbol link error names a hook both the trace and this file define; the trace file must only keep hooks this file does not define.

- [ ] **Step 5: Run and verify the guest reaches its frame loop**

Run the run command with `gpu_plugin = "native"` and `fm4_d3d_trace = true`.
Expected in the newest log: `native gpu: PM4 sink at ...`, `native gpu: guest D3DDevice at 0x..., ring redirected to sink`, then `[d3dtrace] frames=300 swap=300 ...` lines repeating. The window stays magenta (no clear hook yet); menu music starts and the log shows the same kernel activity as under xenos.

If no `[d3dtrace]` line ever appears: the guest is spinning in a GPU wait this task did not cover. Attach the debugger (`FM4/.vs/launch.vs.json`, profile `fm4 (Debug)`), break, read the guest PC from the main thread's `PPCContext`, look it up in ida40, add that function to `fm4_d3d_functions.toml` and a forwarding no-op hook here, regenerate, rebuild, rerun. Candidates to check first: `sub_82382A70` (frame token advance, called by Swap and BeginTiling), `sub_8236F838` (fence internals), `?VerticalBlankInterrupt@D3D@@` waiters.

- [ ] **Step 6: Commit**

```powershell
git add FM4/src/gpu/fm4_d3d_hooks.cpp FM4/src/gpu/fm4_d3d_trace.cpp FM4/src/gpu/d3d_guest.h FM4/src/fm4_midasm_hooks.cpp FM4/CMakeLists.txt
git commit -m "fm4: run the guest D3D library against a PM4 sink and present via Plume"
```

---

### Task 6: `D3DDevice_Clear` drives the presented colour

**Files:**
- Modify: `FM4/src/gpu/fm4_d3d_hooks.cpp` (append one hook)

**Interfaces:**
- Consumes: `fm4::gpu::Video::RequestClear(uint32_t argb)` from Task 4; `D3DDevice_Clear(pDevice, Count, pRects, Flags, D3DCOLOR Color, double Z, DWORD Stencil, BOOL EDRAMClear)` with `Flags` in r6 and `Color` in r7 (ida40 prototype). `Flags` bit `D3DCLEAR_TARGET = 0x1`.
- Produces: the swapchain shows the game's last requested clear colour instead of magenta.

- [ ] **Step 1: Append the hook**

```cpp
// D3DDevice_Clear(pDevice, Count, pRects, Flags, Color, Z, Stencil, EDRAMClear):
// r6 = Flags, r7 = Color (Z rides in f1 with a reserved GPR slot after it).
// The library's own body still runs so its pending state stays consistent.
extern "C" REX_FUNC(D3DDevice_Clear) {
  if (Native() && (ctx.r6.u32 & 0x1u) != 0) {
    fm4::gpu::Video::RequestClear(ctx.r7.u32);
  }
  __imp__D3DDevice_Clear(ctx, base);
}
```

- [ ] **Step 2: Build and run**

Run the build command. Expected: `0`.
Run the run command. Expected: log contains `native gpu: first guest clear colour 0x........` and the window is no longer magenta. Note the colour value; a `0xFF000000` black clear is a valid result at the title screen.

- [ ] **Step 3: Commit**

```powershell
git add FM4/src/gpu/fm4_d3d_hooks.cpp
git commit -m "fm4: forward D3DDevice_Clear to the native present colour"
```

---

## Milestone exit criteria

- `gpu_plugin = "xenos"` behaves exactly as before Task 1.
- `gpu_plugin = "native"` boots FM4 to the title screen loop: steady `[d3dtrace]` lines, menu audio, a window showing the guest's clear colour, clean exit through `OnShutdown`.
- `<cwd>\shaders\` holds the `.vso`/`.pso` set captured under xenos, ready for the shader-cache milestone.
- The trace numbers for `runCB`, `beginTiling`, `rtIdxNon0` and `resolveFlags` are recorded in `FM4/docs/native-render-plume-deepdive.md` section 6.

## Follow-up plans (separate documents)

1. **M2, resources and draws:** lift reblue's `host_resource_heap`, `surface_pool`, `native_texture_mirror`, `physical_buffers`, `sampler_cache`, `constant_buffers`, `pipeline_cache`, `draw*.cpp`, `resolve.cpp` into `src/graphics_native/` with a `TitleProfile`; hook `CreateTexture/CreateSurface/Create*Buffer/Lock/Unlock/SetTexture/SetStreamSource/SetIndices/SetRenderTarget/SetDepthStencilSurface/SetViewport/SetScissorRect/DrawIndexedVertices/DrawIndexedVerticesUP/BeginVertices/Resolve`; read pipeline state from the `GPU_*PACKET` shadows (deep dive 5.3).
2. **M3, shaders:** reblue's XenosRecomp fork with an `FM4_RECOMP` shared-constants block and vertex-location table; offline cache from the captured `shaders\` set; prelinker; cache-miss logging.
3. **M4, command buffers and tiling:** record/replay for `D3DCommandBuffer_*` and `RunCommandBuffer`; `BeginTiling`/`EndTiling` as clear-once-render-once-resolve (deep dive 5.5, 5.6).
4. **M5, quality:** MSAA/SSAA, occlusion queries, ImGui overlays in the present pass, PSO warm-up, Vulkan backend.
