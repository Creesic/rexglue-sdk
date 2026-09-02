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
  if (ec && !std::filesystem::exists(dir)) {
    REXLOG_WARN("[d3dtrace] cannot create {}", dir.string());
    return;
  }
  char name[64];
  std::snprintf(name, sizeof(name), "%016llX.%s", static_cast<unsigned long long>(hash), ext);
  const auto path = dir / name;
  if (std::filesystem::exists(path, ec)) {
    return;
  }
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(base + function_va), bytes);
  if (!out) {
    REXLOG_WARN("[d3dtrace] failed to write {}", path.string());
    return;
  }
  REXLOG_INFO("[d3dtrace] dumped {} ({} bytes)", name, bytes);
}

}  // namespace

void fm4::gpu::TraceOnSwap() {
  if (!Enabled()) {
    return;
  }
  Bump(kSwap);
  if ((g_frames.fetch_add(1, std::memory_order_relaxed) % 300) == 299) {
    LogAndReset();
  }
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
