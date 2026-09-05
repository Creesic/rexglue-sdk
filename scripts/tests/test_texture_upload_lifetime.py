"""Exercise production texture-copy recording and frame retirement without a GPU.

Run with python scripts/tests/test_texture_upload_lifetime.py (requires clang++).
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
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include "render/render_commands.h"
using namespace pgr4::render;
unsigned destroyed = 0, frame = 0;
bool lost = false, noCommands = false, loseOnBegin = false;
struct RenderBuffer;
struct RenderBufferReference { RenderBuffer* ref = nullptr; uint64_t offset = 0; };
struct RenderBuffer {
  std::vector<unsigned> bytes{11, 22, 33, 44};
  RenderBufferReference at(uint64_t offset) { return {this, offset}; }
  ~RenderBuffer() { ++destroyed; }
};
struct RenderTexture { std::array<unsigned, 16> values{}; } texture;
enum class RenderFormat { Test };
enum class RenderBarrierStage { COPY };
enum class RenderTextureLayout { COPY_DEST };
struct RenderTextureBarrier { RenderTexture* texture; RenderTextureLayout layout; };
struct RenderTextureCopyLocation {
  RenderTexture* texture = nullptr;
  RenderBuffer* buffer = nullptr;
  uint32_t mip = 0, slice = 0, width = 0, height = 0, rowTexels = 0;
  uint64_t offset = 0;
  static auto Subresource(RenderTexture* t, uint32_t mip, uint32_t slice) {
    RenderTextureCopyLocation r; r.texture=t; r.mip=mip; r.slice=slice; return r;
  }
  static auto PlacedFootprint(RenderBuffer* b, RenderFormat, uint32_t w, uint32_t h,
                             uint32_t depth, uint32_t rowTexels, uint64_t offset) {
    assert(depth == 1);
    RenderTextureCopyLocation r; r.buffer=b; r.width=w; r.height=h;
    r.rowTexels=rowTexels; r.offset=offset; return r;
  }
};
std::vector<unsigned> drawn;
struct Commands {
  std::vector<std::function<void()>> work;
  std::vector<RenderTextureCopyLocation> footprints;
  RenderTexture* barrierTexture = nullptr;
  void barriers(RenderBarrierStage, RenderTextureBarrier b) { barrierTexture=b.texture; }
  void copyTextureRegion(RenderTextureCopyLocation dst, RenderTextureCopyLocation src) {
    assert(dst.texture == barrierTexture);
    footprints.push_back(src);
    work.push_back([dst,src] { dst.texture->values[dst.slice*4+dst.mip]=src.buffer->bytes[src.offset]; });
  }
  void Draw() { work.push_back([] { drawn.push_back(texture.values[0]); }); }
  void Execute() { for (auto& job : work) job(); work.clear(); }
} commands[2];
using RenderCommandList = Commands;
std::array<RenderBuffer, 2> poolBuffers;
bool poolFails = false;
RenderBufferReference UploadFrameData(const void* data, uint64_t size, bool swap, uint64_t alignment) {
  assert(!swap && alignment == 512);
  if (poolFails) return {};
  auto& buffer = poolBuffers[frame];
  buffer.bytes.resize(2048);
  std::memcpy(buffer.bytes.data() + 512, data, size);
  return buffer.at(512);
}
bool IsDeviceLost() { return lost; }
auto* CommandList() {
  if (loseOnBegin) lost=true;
  return noCommands ? nullptr : &commands[frame];
}
std::recursive_mutex mutex;
auto& RecordingMutex() { return mutex; }
uint32_t CurrentRecordingFrame() { return frame; }
constexpr uint32_t kNumFrames = 2;
std::array<std::vector<std::unique_ptr<RenderBuffer>>, kNumFrames> g_tempUploadBuffers;
struct Resettable { void Reset() {} } g_intermediaryUploadAllocator;
std::array<Resettable, kNumFrames> g_uploadAllocators;
std::array<bool, kNumFrames> retired{false, true};
void DestructTempResources(uint32_t slot) { assert(retired[slot]); }
namespace pgr4::render {
PRODUCTION_FUNCTIONS
}
int main() {
  // Owning upload commands execute once, never enter a replayable batch.
  assert(!IsRecordable(RenderCommandType::CopyTextureFromUpload));
  assert(!IsRecordable(RenderCommandType::CopyTextureSubresourcesFromUpload));
  assert(!IsRecordable(RenderCommandType::UploadTextureSubresources));
  commands[0].Draw();
  TextureUploadRegion regions[] = {{8,4,64,0,0,0}, {4,2,64,1,0,1}, {2,1,64,0,1,2}};
  ProcCopyTextureSubresourcesFromUpload(&texture, new RenderBuffer, 0, regions, 3);
  commands[0].Draw();
  ProcCopyTextureFromUpload(&texture, new RenderBuffer, 0, 8, 4, 64, 0, 0, 3);
  commands[0].Draw();
  assert(destroyed == 0 && g_tempUploadBuffers[0].size() == 2);
  assert(texture.values[0] == 0 && drawn.empty());  // No submit or wait during upload.
  assert(commands[0].footprints.size() == 4);
  assert(commands[0].footprints[1].width == 4 && commands[0].footprints[1].height == 2);
  assert(commands[0].footprints[1].rowTexels == 64 && commands[0].footprints[1].offset == 1);
  regions[1] = {};  // Footprints were consumed while recording, not borrowed until execution.
  frame = 1;
  OnRecordingFrameReady(1);
  assert(destroyed == 0);  // Retiring another slot cannot release these buffers.
  commands[0].Execute();
  assert((drawn == std::vector<unsigned>{0,11,44}));
  assert(texture.values[1] == 22 && texture.values[4] == 33);
  retired[0] = true;
  OnRecordingFrameReady(0);
  assert(destroyed == 2);
  // All early exits must also consume the transferred buffer exactly once.
  ProcCopyTextureSubresourcesFromUpload(nullptr, new RenderBuffer, 0, regions, 1);
  ProcCopyTextureSubresourcesFromUpload(&texture, new RenderBuffer, 0, nullptr, 1);
  ProcCopyTextureSubresourcesFromUpload(&texture, new RenderBuffer, 0, regions, 0);
  lost = true;
  ProcCopyTextureSubresourcesFromUpload(&texture, new RenderBuffer, 0, regions, 1);
  lost = false; noCommands = true;
  ProcCopyTextureSubresourcesFromUpload(&texture, new RenderBuffer, 0, regions, 1);
  noCommands = false; loseOnBegin = true;
  ProcCopyTextureSubresourcesFromUpload(&texture, new RenderBuffer, 0, regions, 1);
  assert(destroyed == 8 && g_tempUploadBuffers[1].empty());
  // CPU staging is copied into the frame pool before Run returns. Footprints
  // include both the arena's placement offset and each subresource's offset.
  lost = loseOnBegin = false;
  frame = 0;
  unsigned cpu[] = {91,92,93,94};
  TextureUploadRegion pooledRegions[] = {{8,4,64,0,0,0}, {4,2,64,1,0,1}};
  bool success = false;
  ProcUploadTextureSubresources(&texture, cpu, sizeof(cpu), 0, pooledRegions, 2, &success);
  assert(success && commands[0].footprints.back().offset == 513);
  cpu[0] = 999;
  commands[0].Execute();
  assert(texture.values[0] == 91 && texture.values[1] == 92);
  const auto copies = commands[0].footprints.size();
  poolFails = true;
  ProcUploadTextureSubresources(&texture, cpu, sizeof(cpu), 0, pooledRegions, 2, &success);
  assert(!success && commands[0].footprints.size() == copies);
  poolFails = false; noCommands = true;
  ProcUploadTextureSubresources(&texture, cpu, sizeof(cpu), 0, pooledRegions, 2, &success);
  assert(!success && commands[0].footprints.size() == copies);
  std::puts("Texture upload lifetime: ordered draws/copies, mip/array footprints, frame retirement, failures, and non-replay ownership passed");
}
'''


def test_texture_upload_lifetime():
    video = (ROOT / "pgr4-recomp/src/render/video.cpp").read_text(encoding="utf-8")
    state = (ROOT / "pgr4-recomp/src/render/render_state.cpp").read_text(encoding="utf-8")
    queue = (ROOT / "pgr4-recomp/src/render/render_queue.cpp").read_text(encoding="utf-8")
    functions = []
    for source, name in [(state, "RetainTempUploadBuffer"), (state, "OnRecordingFrameReady"),
                         (video, "CopyTextureSubresources"), (video, "ProcUploadTextureSubresources"),
                         (queue, "IsRecordable"), (video, "ProcCopyTextureSubresourcesFromUpload"),
                         (video, "ProcCopyTextureFromUpload")]:
        match = re.search(r"^(?:void|bool) " + name + r"\(.*?^}", source, re.M | re.S)
        assert match is not None, name
        functions.append(match.group())
    with tempfile.TemporaryDirectory(prefix="pgr4-texture-life-") as folder:
        cpp, exe = Path(folder) / "test.cpp", Path(folder) / "test.exe"
        cpp.write_text(PROGRAM.replace("PRODUCTION_FUNCTIONS", "\n".join(functions)), encoding="utf-8")
        subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++23", "-O1",
                        f"-I{ROOT / 'pgr4-recomp/src'}", str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    test_texture_upload_lifetime()
