"""Exercise production raw-surface translation and EDRAM ownership without a GPU.

Run with python scripts/tests/test_surface_header_reuse.py (requires clang++).
"""

import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

PROGRAM = r'''
#include <cassert>
#include <cstdio>
#include <map>
#include <tuple>
#include "render/guest_resources.h"
#include "render/render_commands.h"
using namespace plume;
using namespace pgr4::render;
namespace rr = pgr4::render;
#define REXGPU_WARN(...) ((void)0)
#define REXGPU_INFO(...) ((void)0)

std::vector<std::unique_ptr<GuestSurface>> allocations;
unsigned releases = 0;
template<class T> T* GuestNew(ResourceType type) {
  allocations.push_back(std::make_unique<T>(type));
  return allocations.back().get();
}
namespace ghp {
uint32_t ToGuest(const void* pointer) {
  ++releases;
  for (uint32_t i = 0; i < allocations.size(); ++i)
    if (allocations[i].get() == pointer) return i + 1;
  assert(false);
  return 0;
}
template<class T> T* ToHost(uint32_t address) {
  return static_cast<T*>(allocations.at(address - 1).get());
}
}
uint32_t g_origD3DResourceRelease(uint32_t) { assert(false); return 0; }
namespace pgr4::render {
bool IsDeviceLost() { return false; }
void ScheduleResourceDestruction(GuestResource*) { assert(false); }
FORMAT_SOURCE
namespace RenderQueue {
void Run(const RenderCommand& command) {
  assert(command.type == RenderCommandType::CreateSurfaceHost);
  const auto& c = command.createSurfaceHost;
  auto* surface = static_cast<GuestSurface*>(c.surface);
  surface->width = c.width;
  surface->height = c.height;
  surface->format = ConvertFormat(c.format);
  surface->guestFormat = c.format;
  surface->sampleCount = c.sampleCount;
}
}
CREATE_SOURCE
}
RELEASE_SOURCE

std::unordered_map<uint32_t, uint32_t> headerWords;
uint32_t ReadGuestU32At(uint32_t address) { return headerWords.at(address); }
void Header(uint32_t address, uint32_t width = 1280, uint32_t height = 720,
            uint32_t format = 0x18280186, uint32_t msaaLog2 = 0,
            uint32_t edram = 0) {
  headerWords[address] = 4;
  headerWords[address + 0x18] = msaaLog2 << 16;
  headerWords[address + 0x1C] = edram;
  headerWords[address + 0x24] = ((width - 1) << 18) | ((height - 1) << 3);
  headerWords[address + 0x28] = format;
}
TRANSLATE_SOURCE

int main() {
  constexpr uint32_t a = 0x1000, b = 0x2000, colour = 0x18280186, depth = 0x2D200196;
  Header(a);
  auto* colourSurface = TranslateRawSurface(a);
  assert(colourSurface && colourSurface->type == ResourceType::RenderTarget);
  assert(colourSurface->refCount == 2); // Registry plus raw-header cache.
  for (unsigned i = 0; i < 100; ++i) assert(TranslateRawSurface(a) == colourSurface);
  assert(allocations.size() == 1 && colourSurface->refCount == 2 && releases == 0);

  // The race allocators recycle a colour header as depth at the same address.
  Header(a, 1280, 720, depth);
  auto* depthSurface = TranslateRawSurface(a);
  assert(depthSurface != colourSurface && depthSurface->type == ResourceType::DepthStencil);
  assert(RenderFormatIsDepth(depthSurface->format));
  assert(depthSurface->refCount == 2 && colourSurface->refCount == 1);
  assert(colourSurface->type == ResourceType::RenderTarget); // Queued draw is unchanged.

  Header(a);
  assert(TranslateRawSurface(a) == colourSurface); // A -> B -> A reuses EDRAM.
  assert(depthSurface->refCount == 1 && colourSurface->refCount == 2);
  Header(b);
  assert(TranslateRawSurface(b) == colourSurface && colourSurface->refCount == 3);

  Header(a, 640);
  auto* narrow = TranslateRawSurface(a);
  assert(narrow != colourSurface && narrow->width == 640 && colourSurface->refCount == 2);
  Header(a, 640, 480);
  auto* shorter = TranslateRawSurface(a);
  assert(shorter != narrow && shorter->height == 480 && narrow->refCount == 1);
  Header(a, 640, 480, colour, 2);
  auto* msaa = static_cast<GuestSurface*>(TranslateRawSurface(a));
  assert(msaa != shorter && msaa->sampleCount == RenderSampleCount::COUNT_4);
  Header(a, 640, 480, colour, 2, 17);
  auto* relocated = TranslateRawSurface(a);
  assert(relocated != msaa && msaa->refCount == 1);

  // Full format changes are retranslated, while compatible EDRAM aliases keep
  // sharing the existing image (Bink depends on A8/X8 aliases doing this).
  const unsigned beforeFormatChange = releases;
  Header(a, 640, 480, 0x28280086, 2, 17);
  assert(TranslateRawSurface(a) == relocated && releases == beforeFormatChange + 1);
  assert(relocated->refCount == 2);
  headerWords[a + 0x1C] |= 0x80000000; // Non-layout header bits don't create images.
  assert(TranslateRawSurface(a) == relocated && releases == beforeFormatChange + 1);

  headerWords[a] |= 0x40000000; // Texture-level headers cannot use stale standalone images.
  assert(TranslateRawSurface(a) == nullptr);
  Header(a, 1280, 720, depth);
  assert(TranslateRawSurface(a) == depthSurface && relocated->refCount == 1);
  constexpr uint32_t c = 0x3000;
  headerWords[c] = 0x40000004;
  assert(TranslateRawSurface(c) == nullptr); // No standalone-layout reads.
  Header(c);
  assert(TranslateRawSurface(c) == colourSurface); // Unsupported result isn't permanent.
  assert(allocations.size() == 6 && depthSurface->refCount == 2 && colourSurface->refCount == 3);
  std::puts("Surface headers: colour/depth reuse, layout changes, EDRAM aliases, queued lifetime and balanced references passed");
}
'''


def function(source, signature):
    match = re.search(r"^" + re.escape(signature) + r".*?^}", source, re.M | re.S)
    assert match is not None, signature
    return match.group()


def test_surface_header_reuse():
    render = ROOT / "pgr4-recomp/src/render"
    hooks = (render / "d3d_hooks.cpp").read_text(encoding="utf-8")
    resources = (render / "d3d_resource_hooks.cpp").read_text(encoding="utf-8")
    create = resources[resources.index("static std::map<std::tuple<uint32_t,"):
                       resources.index("void ProcCreateSurfaceHost(")]
    program = (PROGRAM.replace("FORMAT_SOURCE", function(resources, "RenderFormat ConvertFormat("))
               .replace("CREATE_SOURCE", create)
               .replace("RELEASE_SOURCE", function(hooks, "uint32_t D3DResourceReleaseHook("))
               .replace("TRANSLATE_SOURCE", function(hooks, "rr::GuestBaseTexture* TranslateRawSurface(")))
    with tempfile.TemporaryDirectory(prefix="pgr4-surface-reuse-") as folder:
        cpp, exe = Path(folder) / "test.cpp", Path(folder) / "test.exe"
        cpp.write_text(program, encoding="utf-8")
        subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++23", "-O1",
                        "-DNOMINMAX", "-DWIN32_LEAN_AND_MEAN",
                        f"-I{ROOT / 'thirdparty/plume'}", f"-I{ROOT / 'pgr4-recomp/src'}",
                        f"-I{ROOT / 'pgr4-recomp'}",
                        str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=30)


if __name__ == "__main__":
    test_surface_header_reuse()
