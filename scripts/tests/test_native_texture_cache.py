"""Run production raw-texture parsing, refresh and invalidation without a GPU.

Uses the real SDK mip/tiling layout and format table. Requires python and clang++.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROGRAM = r'''
#include <cassert>
#include <bit>
#include <cstdio>
#include <rex/hash.h>
#include <rex/memory/utils.h>
#include <rex/graphics/pipeline/texture/util.h>
#include "render/guest_resources.h"
#include "render/render_commands.h"
using namespace plume;
using namespace pgr4::render;
#define REXGPU_INFO(...) ((void)0)
#define REXGPU_WARN(...) ((void)0)
#define REXGPU_ERROR(...) ((void)0)
#define SCOPE_profile_cpu_f(name)
uint64_t frame = 0;
unsigned creates = 0, uploads = 0, attempts = 0;
bool lost = false, uploadFails = false;
std::vector<uint8_t> uploadedBytes;
namespace rex {
uint8_t lzcnt(uint32_t value) { return uint8_t(std::countl_zero(value)); }
}
namespace ghp {
struct Memory {
  std::vector<uint8_t> bytes = std::vector<uint8_t>(2 * 1024 * 1024, 0x42);
  std::vector<uint8_t> pages = std::vector<uint8_t>(512, 1);
  Memory* GetPhysicalHeap() { return this; }
  rex::memory::PageAccess QueryRangeAccess(uint32_t first, uint32_t last) {
    assert(first / 4096 == last / 4096);
    return pages.at(first / 4096) ? rex::memory::PageAccess::kReadWrite : rex::memory::PageAccess::kNoAccess;
  }
  template<class T> T TranslatePhysical(uint32_t address) {
    assert(address < bytes.size());
    return reinterpret_cast<T>(bytes.data() + address);
  }
} memory;
Memory* GuestMemory() { return &memory; }
uint32_t HeaderBaseToPhysicalForRead(uint32_t address, uint32_t) { return address & 0x1FFFFFFF; }
}
namespace pgr4::render {
bool IsDeviceLost() { return lost; }
uint64_t CurrentFrameIndex() { return frame; }
struct {
  uint64_t revision = 0;
  unsigned arms = 0;
  template <class M> uint64_t BeginSnapshot(M*, uint32_t, uint32_t) { ++arms; return revision; }
  uint64_t Revision(uint32_t, uint32_t) const { return revision; }
} g_physicalWriteWatch;
namespace RenderQueue {
void Run(const RenderCommand& cmd) {
  if (cmd.type == RenderCommandType::CreateTranslatedTextureHost) {
    ++creates;
    const auto& c = cmd.createTranslatedTextureHost;
    auto* texture = c.texture;
    texture->texture = reinterpret_cast<RenderTexture*>(uintptr_t(creates));
    texture->width = c.width;
    texture->height = c.height;
    texture->levels = c.levels;
    texture->format = static_cast<RenderFormat>(c.format);
    *c.createdOut = true;
  } else {
    assert(cmd.type == RenderCommandType::UploadTextureSubresources);
    ++attempts;
    const auto& c = cmd.uploadTextureSubresources;
    *c.success = !uploadFails;
    if (!uploadFails) {
      ++uploads;
      const auto* bytes = static_cast<const uint8_t*>(c.data);
      uploadedBytes.assign(bytes, bytes + c.size);
      for (uint32_t i = 0; i < c.regionCount; ++i)
        assert(c.regions[i].srcOffset % 512 == 0 && c.regions[i].srcOffset < c.size);
    }
  }
}
}
PRODUCTION_SOURCE
}
GuestTexture* Bind(const XenosTextureInfo& info, bool upload = true) {
  if (auto* cached = FindAndRefreshGuestTexture(info, upload)) return cached;
  return CreateAndRegisterGuestTexture(info, upload);
}
auto Header(const XenosTextureInfo& info) {
  std::array<rex::be<uint32_t>, 13> header{};
  header[0] = 3;
  header[7] = (info.pitchTexels / 32) << 22 | uint32_t(info.tiled) << 31;
  header[8] = info.baseAddress | info.gpuFormat | info.endian << 6;
  header[9] = (info.width - 1) | (info.height - 1) << 13;
  header[11] = info.mipMaxLevel << 6;
  header[12] = info.mipAddress | 1 << 9 | uint32_t(info.packedMips) << 11;
  return header;
}
int main() {
  XenosTextureInfo info;
  info.format = RenderFormat::B8G8R8A8_UNORM;
  info.gpuFormat = 6;
  info.width = info.height = info.pitchTexels = 128;
  info.baseAddress = 0x10000;
  info.mipAddress = 0x50000;
  info.mipMaxLevel = 1;
  info.mipLevels = 2;
  info.valid = true;
  auto header = Header(info);
  assert(GuestTextureLayoutKey(ParseTextureFetchConstant(header.data() + 7)) == GuestTextureLayoutKey(info));
  auto* texture = Bind(info);
  assert(texture && uploads == 1 && creates == 1);
  const auto original = uploadedBytes;
  for (frame = 1; frame < 12; ++frame) assert(Bind(info) == texture);
  assert(uploads == 1 && attempts == 1); // No conversion/Run on unchanged frames.
  ghp::memory.bytes[info.baseAddress] = 0x23;
  assert(Bind(info) == texture && uploads == 2 && uploadedBytes[0] == 0x23);
  assert(original[0] == 0x42); // Previous upload is immutable.
  ++frame;
  ghp::memory.bytes[info.mipAddress + 100] ^= 0xFF;
  assert(Bind(info) == texture && uploads == 3); // Mip-only changes count.

  // A missing page is zero-filled, never scanned for a content signature.
  ++frame;
  ghp::memory.pages[info.baseAddress / 4096 + 2] = 0;
  assert(Bind(info) == texture && uploads == 4 && uploadedBytes[8192] == 0);
  ++frame;
  ghp::memory.bytes[info.baseAddress + 8192] ^= 0xFF;
  assert(Bind(info) == texture && uploads == 4);
  ++frame;
  ghp::memory.pages[info.baseAddress / 4096 + 2] = 1;
  assert(Bind(info) == texture && uploads == 5);

  // Explicit unlock/reallocation invalidation takes effect within the frame,
  // including a CPU rewrite of a previously GPU-resolved texture.
  texture->gpuResolved = true;
  ++frame;
  assert(Bind(info) == texture && uploads == 5);
  InvalidateGuestTexture(header.data());
  assert(!texture->gpuResolved && Bind(info) == texture && uploads == 6);
  InvalidateGuestTexture(nullptr);
  header[0] = 1; // Buffer, not a texture: no fetch-header access or invalidation.
  InvalidateGuestTexture(header.data());
  assert(texture->guestUploadHashValid);
  header[0] = 3;

  // Failed uploads cannot publish the signature or suppress a same-frame retry.
  ++frame;
  ghp::memory.bytes[info.baseAddress + 1000] ^= 0xFF;
  const auto priorHash = texture->guestUploadHash;
  uploadFails = true;
  assert(Bind(info) == texture && uploads == 6 && texture->guestUploadHash == priorHash);
  const auto failedAttempts = attempts;
  uploadFails = false;
  assert(Bind(info) == texture && uploads == 7 && attempts == failedAttempts + 1);

  // Complete layout identity: new backing mips, pitch, endian and array shape
  // must not reuse an incompatible host image. Old queued objects stay alive.
  auto changed = info;
  changed.mipAddress += 0x10000;
  auto* movedMips = Bind(changed);
  assert(movedMips != texture && texture->texture);
  changed.endian = 2;
  assert(Bind(changed) != movedMips);
  auto* endianVersion = Bind(changed);
  changed.pitchTexels = 256;
  assert(Bind(changed) != endianVersion);
  auto* pitchVersion = Bind(changed);
  changed.arraySize = 2;
  auto* arrayVersion = Bind(changed);
  assert(arrayVersion != pitchVersion && arrayVersion->depth == 2);
  ++frame;
  const auto beforeArrayEdit = uploads;
  ghp::memory.bytes[changed.baseAddress + 256 * 128 * 4] ^= 0xFF;
  assert(Bind(changed) == arrayVersion && uploads == beforeArrayEdit + 1);
  changed = info;
  changed.mipAddress = 0;
  changed.mipLevels = 1;
  changed.mipMaxLevel = 0;
  auto* singleLevel = Bind(changed);
  changed.packedMips = true; // Unused for large base-only images.
  assert(Bind(changed) == singleLevel);
  changed.width = changed.pitchTexels = changed.height = 16;
  const auto packedSmall = GuestTextureLayoutKey(changed);
  changed.packedMips = false;
  assert(GuestTextureLayoutKey(changed) != packedSmall);

  // A transient all-unmapped source is not cached as initialized.
  changed = info;
  changed.baseAddress = 0xA0000;
  std::fill_n(ghp::memory.pages.begin() + changed.baseAddress / 4096, 16, 0);
  auto* pending = Bind(changed);
  assert(pending && !pending->guestUploadHashValid);
  std::fill_n(ghp::memory.pages.begin() + changed.baseAddress / 4096, 16, 1);
  assert(Bind(changed) == pending && pending->guestUploadHashValid);
  // Watched physical memory: an unchanged page revision skips the re-arm and
  // the content hash; a bumped or lost revision validates again.
  changed = info;
  changed.baseAddress = 0xC0000;
  changed.mipAddress = 0x100000;
  g_physicalWriteWatch.revision = 7;
  auto* watched = Bind(changed);
  const auto watchedUploads = uploads;
  const auto armsAfterCreate = g_physicalWriteWatch.arms;
  assert(watched && watched->guestWatchRevision == (std::array<uint64_t, 2>{7, 7}));
  ++frame;
  ghp::memory.bytes[changed.baseAddress] ^= 0xFF;
  assert(Bind(changed) == watched && uploads == watchedUploads);
  assert(g_physicalWriteWatch.arms == armsAfterCreate);
  g_physicalWriteWatch.revision = 8;
  ++frame;
  assert(Bind(changed) == watched && uploads == watchedUploads + 1);
  assert(watched->guestWatchRevision == (std::array<uint64_t, 2>{8, 8}));
  g_physicalWriteWatch.revision = 0; // Unwatched again: hash every frame.
  ++frame;
  ghp::memory.bytes[changed.baseAddress + 4] ^= 0xFF;
  assert(Bind(changed) == watched && uploads == watchedUploads + 2);
  assert(watched->guestWatchRevision[0] == 0);
  std::puts("Native texture cache: unchanged frames skip uploads; base/mips/arrays, sparse pages, layout reuse, invalidation and failure retries passed");
}
'''


def test_native_texture_cache():
    source = (ROOT / "pgr4-recomp/src/render/d3d_resource_hooks.cpp").read_text(encoding="utf-8")
    production = source[source.index("struct XenosTextureInfo {"):
                        source.index("// Translates a raw Xenos GPUTEXTURE_FETCH_CONSTANT")]
    # The source closes its anonymous namespace before the public invalidator.
    program = PROGRAM.replace("PRODUCTION_SOURCE", "namespace {\n" + production)
    with tempfile.TemporaryDirectory(prefix="pgr4-texture-cache-") as folder:
        cpp, exe = Path(folder) / "test.cpp", Path(folder) / "test.exe"
        cpp.write_text(program, encoding="utf-8")
        subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++23", "-O2", "-msse4.1",
                        "-DNOMINMAX", "-DWIN32_LEAN_AND_MEAN",
                        *[f"-I{ROOT / path}" for path in
                          ["include", "thirdparty/plume", "thirdparty/xxHash", "pgr4-recomp/src", "pgr4-recomp"]],
                        str(cpp), str(ROOT / "src/graphics/pipeline/texture/util.cpp"),
                        str(ROOT / "src/graphics/pipeline/texture/info_formats.cpp"),
                        "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=30)


if __name__ == "__main__":
    test_native_texture_cache()
