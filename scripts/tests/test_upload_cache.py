"""Run with python scripts/tests/test_upload_cache.py (requires clang++).

Uses the production upload allocator and snapshot/batch types, with a memory
buffer replacing the GPU. Checks immutable versions, full palettes, written
ranges, fence-slot reset, allocation failures, and recorded-batch ownership.
"""

import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

PREAMBLE = r'''
#include <cassert>
#include <cstdio>
#include <algorithm>
#include <plume_render_interface.h>
#include "render/render_queue.h"
using namespace plume;
using namespace pgr4::render;
#define REXGPU_ERROR(...) ((void)0)
#define REXGPU_INFO(...) ((void)0)
uint64_t g_frameTraceIndex = 0;
bool lost = false, createFails = false, mapFails = false;
unsigned creates = 0, checks = 0, maps = 0, unmaps = 0;
uint64_t lastWritten = 0;
bool IsDeviceLost() { return lost; }
bool CheckDeviceLost(const char*) { ++checks; return lost; }
struct MemoryBuffer : RenderBuffer {
  std::vector<uint8_t> bytes;
  explicit MemoryBuffer(uint64_t size) : bytes(size) {}
  void* map(uint32_t, const RenderRange* read) override {
    assert(read && read->begin == 0 && read->end == 0);
    ++maps;
    return mapFails ? nullptr : bytes.data();
  }
  void unmap(uint32_t, const RenderRange* written) override {
    assert(written && written->begin == 0 && written->end <= bytes.size());
    ++unmaps;
    lastWritten = written->end;
  }
  std::unique_ptr<RenderBufferFormattedView> createBufferFormattedView(RenderFormat) override { return {}; }
  void setName(const std::string&) override {}
  uint64_t getDeviceAddress() const override { return 0; }
};
struct MemoryDevice {
  std::unique_ptr<RenderBuffer> createBuffer(const RenderBufferDesc& desc) {
    ++creates;
    return createFails ? nullptr : std::make_unique<MemoryBuffer>(desc.size);
  }
} device;
auto* Device() { return &device; }
struct Commands { void setGraphicsRootDescriptor(RenderBufferReference, uint32_t) {} } commands;
auto* CommandList() { return &commands; }
'''

MAIN = r'''
int main() {
  ByteSnapshotCache snapshots;
  uint8_t source[] = {0,1,2,3,4,5,6};
  auto* original = snapshots.Copy(source, sizeof(source));
  auto* swapped = snapshots.Copy(source, sizeof(source), true);
  const uint8_t expected[] = {3,2,1,0,4,5,6};
  assert(std::memcmp(swapped, expected, sizeof(source)) == 0);
  assert(snapshots.Copy(source, sizeof(source)) == original);
  source[0] = 9;
  assert(snapshots.Copy(source, sizeof(source)) != original);
  assert(original[0] == 0 && swapped[3] == 0);
  assert(snapshots.Copy(nullptr, 4) == nullptr && snapshots.Copy(source, 0) == nullptr);

  UploadAllocator uploads;
  std::vector<uint8_t> palette(221760, 7);
  auto first = uploads.UploadCached(palette.data(), uint32_t(palette.size()), false);
  assert(first.ref && first.offset == 0);
  for (unsigned i = 0; i < 2000; ++i) {
    auto repeated = uploads.UploadCached(palette.data(), uint32_t(palette.size()), false);
    assert(repeated.ref == first.ref && repeated.offset == first.offset);
  }
  assert(creates == 1 && maps == 1);
  // A changed last bone must produce a new version and preserve the old draw.
  palette.back() = 8;
  auto changed = uploads.UploadCached(palette.data(), uint32_t(palette.size()), false);
  assert(changed.ref == first.ref && changed.offset >= palette.size());
  const auto* memory = static_cast<const MemoryBuffer*>(first.ref);
  assert(memory->bytes[first.offset + palette.size() - 1] == 7);
  assert(memory->bytes[changed.offset + palette.size() - 1] == 8);
  uploads.FinishWrites();
  assert(unmaps == 1 && lastWritten == changed.offset + palette.size());
  assert(lastWritten < 1024 * 1024);  // Used bytes, not the full 64 MiB allocation.
  uploads.FinishWrites();
  assert(unmaps == 1);
  // Only after the slot's fence retires may its offsets and cached refs reset.
  uploads.Reset();
  auto nextFrame = uploads.UploadCached(palette.data(), uint32_t(palette.size()), false);
  assert(nextFrame.offset == 0 && creates == 1 && maps == 2);
  assert(static_cast<const MemoryBuffer*>(nextFrame.ref)->bytes[palette.size()-1] == 8);
  uploads.FinishWrites();

  UploadAllocator failing;
  createFails = true;
  const unsigned before = creates;
  for (unsigned i = 0; i < 100; ++i)
    assert(!failing.UploadCached(source, sizeof(source), false).ref);
  assert(creates == before + 1 && checks == 1 && failing.Failed());
  createFails = false;
  failing.Reset();
  assert(failing.UploadCached(source, sizeof(source), true).ref);
  failing.FinishWrites();
  failing.Reset();
  mapFails = true;
  assert(!failing.UploadCached(source, sizeof(source), false).ref);
  assert(failing.Failed() && checks == 2);
  mapFails = false;
  failing.Reset();
  assert(failing.UploadCached(source, sizeof(source), false).ref);
  failing.FinishWrites();
  lost = true;
  assert(!failing.UploadCached(source, sizeof(source), false).ref);

  RecordedRenderBatch batch;
  RenderCommand command{};
  command.type = RenderCommandType::SetDrawGeometrySnapshot;
  command.setDrawGeometrySnapshot.streams[1] = {nullptr, 0, 24, palette.data(), uint32_t(palette.size())};
  batch.Append(command);
  batch.Append(command);
  auto* recorded = batch.commands()[0].setDrawGeometrySnapshot.streams[1].rawData;
  assert(recorded == batch.commands()[1].setDrawGeometrySnapshot.streams[1].rawData);
  palette.back() = 9;
  batch.Append(command);
  assert(recorded[palette.size()-1] == 8);
  assert(batch.commands()[2].setDrawGeometrySnapshot.streams[1].rawData[palette.size()-1] == 9);
  std::puts("Upload cache: reuse, immutable full palettes, written ranges, reset, failures, and batch ownership passed");
}
'''


def test_upload_cache():
    source = (ROOT / "pgr4-recomp/src/render/render_state.cpp").read_text(encoding="utf-8")
    allocator = re.search(r"^class UploadAllocator \{.*?^};", source, re.M | re.S)
    assert allocator is not None
    with tempfile.TemporaryDirectory(prefix="pgr4-upload-") as folder:
        cpp, exe = Path(folder) / "test.cpp", Path(folder) / "test.exe"
        cpp.write_text(PREAMBLE + allocator.group() + MAIN, encoding="utf-8")
        subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++23", "-O1",
                        "-DNOMINMAX", "-DWIN32_LEAN_AND_MEAN",
                        *[f"-I{ROOT / path}" for path in
                          ["include", "thirdparty/plume", "thirdparty/xxHash", "pgr4-recomp/src"]],
                        str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    test_upload_cache()
