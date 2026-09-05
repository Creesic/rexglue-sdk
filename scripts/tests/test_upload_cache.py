"""Run with python scripts/tests/test_upload_cache.py (requires clang++).

Uses the production CPU/GPU allocators and snapshot/batch types, with memory
buffers replacing guest memory and the GPU. Checks immutable versions, full
palettes, draw byte ranges, endian modes, reset, and recorded-batch ownership.
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
#include <mutex>
#include <thread>
#include <cstdlib>
#include <new>
#include <rex/hash.h>
unsigned largeAllocations = 0;
void* operator new(std::size_t size) {
  if (void* memory = std::malloc(size == 0 ? 1 : size)) {
    if (size >= 65536) ++largeAllocations;
    return memory;
  }
  throw std::bad_alloc();
}
void operator delete(void* memory) noexcept { std::free(memory); }
unsigned hashCalls = 0;
uint64_t CountedHash(const void* source, size_t size) {
  ++hashCalls;
#ifdef FORCE_HASH_COLLISIONS
  return 1;
#else
  return XXH3_64bits(source, size);
#endif
}
#undef XXH3_64bits
#define XXH3_64bits(source, size) CountedHash(source, size)
#include <plume_render_interface.h>
#include "render/render_queue.h"
#include "render/guest_device.h"
#include "render/guest_resources.h"
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
struct Commands {
  std::array<RenderBufferReference, 3> roots{};
  unsigned binds = 0;
  void setGraphicsRootDescriptor(RenderBufferReference ref, uint32_t index) {
    assert(index < roots.size()); roots[index] = ref; ++binds;
  }
} commands;
auto* CommandList() { return &commands; }
bool recording = false;
RenderCommand lastEnqueued{};
bool RenderQueue::IsRecording() { return recording; }
void RenderQueue::Enqueue(const RenderCommand& command) { lastEnqueued = command; }
namespace ghp {
struct Memory {
  std::vector<uint8_t> bytes = std::vector<uint8_t>(4 * 1024 * 1024 + 16);
  bool readable = true, nullSource = false;
  using Callback = std::pair<uint32_t, uint32_t> (*)(void*, uint32_t, uint32_t, bool);
  Callback callback = nullptr;
  void* context = nullptr;
  bool armFails = false, invalidateOnArm = false;
  void* RegisterPhysicalMemoryInvalidationCallback(Callback cb, void* ctx) {
    callback = cb; context = ctx; return this;
  }
  void UnregisterPhysicalMemoryInvalidationCallback(void*) { callback = nullptr; }
  bool EnablePhysicalMemoryAccessCallbacks(uint32_t address, uint32_t size, bool, bool) {
    if (invalidateOnArm) {
      invalidateOnArm = false;
      bytes[address] ^= 0xFF;
      callback(context, address, size, true);
    }
    return !armFails;
  }
  unsigned translations = 0;
  uint32_t lastAddress = 0;
  Memory* GetPhysicalHeap() { return this; }
  rex::memory::PageAccess QueryRangeAccess(uint32_t, uint32_t) {
    return readable ? rex::memory::PageAccess::kReadWrite : rex::memory::PageAccess::kNoAccess;
  }
  template<class T> T TranslatePhysical(uint32_t address) {
    ++translations;
    lastAddress = address;
    assert(address < bytes.size());
    return nullSource ? nullptr : reinterpret_cast<T>(bytes.data() + address);
  }
} memory;
bool noMemory = false;
auto* GuestMemory() { return noMemory ? nullptr : &memory; }
uint32_t HeaderBaseToPhysical(uint32_t address) { return address & 0x1FFFFFFFu; }
template<class T> T* ToHost(uint32_t address) {
  assert(address < memory.bytes.size());
  return reinterpret_cast<T*>(memory.bytes.data() + address);
}
}
'''

DRAW_RANGES = r'''
void CheckDrawRanges() {
  constexpr uint32_t fullSize = 221760, base = 4096, paletteBase = 1048576;
  auto& bytes = ghp::memory.bytes;
  std::fill(bytes.begin(), bytes.end(), 7);
  GuestVertexDeclaration decl;
  decl.vertexElements = std::make_unique<GuestVertexElement[]>(3);
  decl.vertexElementCount = 2;
  decl.vertexElements[0] = {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0};
  decl.vertexElements[1] = {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 0};
  decl.vertexElements[1].stream = 15;
  assert(NonIndexedVertexSnapshotSize(&decl, 15, 28, fullSize, 0, 4) == fullSize);
  decl.vertexElements[1].stream = 0;
  GuestDevice producer{};
  producer.streamSources[0] = 64; // Raw header, not a native GuestBuffer.
  producer.streamStrideDwords[0] = 7;
  auto* fetch = reinterpret_cast<rex::be<uint32_t>*>(
      reinterpret_cast<uint8_t*>(&producer) + 0x778);
  fetch[0] = base;
  fetch[1] = fullSize | 3u; // Fetch flags must not enter the copied size.
  SetVertexDeclaration(&producer, &decl);
  assert(lastEnqueued.setVertexDeclaration.declaration == &decl);
  const auto snapshot = [&](uint32_t start = 5, uint32_t count = 4) {
    LocalRenderCommandQueue queue;
    QueueDrawGeometrySnapshot(&producer, queue, start, count);
    assert(queue.count == 1 && queue.commands[0].type == RenderCommandType::SetDrawGeometrySnapshot);
    return queue.commands[0];
  };
  auto first = snapshot();
  const auto& stream = first.setDrawGeometrySnapshot.streams[0];
  assert(stream.rawSize == 252 && stream.stride == 28 && stream.rawIdentity != 0);
  // Bytes beyond the consumed prefix cannot change a snapshot's identity.
  bytes[base + fullSize - 1] = 9;
  assert(snapshot().setDrawGeometrySnapshot.streams[0].rawIdentity == stream.rawIdentity);
  bytes[base + 251] = 8;
  auto changed = snapshot();
  const auto& changedStream = changed.setDrawGeometrySnapshot.streams[0];
  assert(changedStream.rawIdentity != stream.rawIdentity && changedStream.rawData[248] == 8);
  assert(stream.rawData[248] == 7); // Earlier queued draws stay immutable.
  UploadAllocator gpu;
  const auto upload = gpu.UploadSnapshot(changedStream.rawData, changedStream.rawSize,
                                        changedStream.rawIdentity);
  assert(upload.ref && std::memcmp(static_cast<const MemoryBuffer*>(upload.ref)->bytes.data() +
                                  upload.offset, changedStream.rawData, 252) == 0);

  // Indexed/UP draws pass no range; recordings may inherit another declaration.
  assert(snapshot(0, 0).setDrawGeometrySnapshot.streams[0].rawSize == fullSize);
  recording = true;
  SetVertexDeclaration(&producer, nullptr);
  assert(lastEnqueued.setVertexDeclaration.declaration == nullptr);
  assert(snapshot().setDrawGeometrySnapshot.streams[0].rawSize == fullSize);
  recording = false;
  assert(snapshot().setDrawGeometrySnapshot.streams[0].rawSize == 252);
  SetVertexDeclaration(&producer, nullptr);
  assert(snapshot().setDrawGeometrySnapshot.streams[0].rawSize == fullSize);
  SetVertexDeclaration(&producer, &decl);
  decl.vertexElements[1].offset = 16; // Renderer would reject this declaration.
  assert(snapshot().setDrawGeometrySnapshot.streams[0].rawSize == fullSize);
  decl.vertexElements[1].offset = 12;
  decl.vertexElements[1].method = 1;
  assert(snapshot().setDrawGeometrySnapshot.streams[0].rawSize == fullSize);
  decl.vertexElements[1].method = 0;
  decl.vertexElements[1].type = 0xFFFFFFFFu;
  assert(snapshot().setDrawGeometrySnapshot.streams[0].rawSize == fullSize);
  decl.vertexElements[1].type = D3DDECLTYPE_FLOAT4;

  // Arbitrarily indexed POSITION palettes stay whole on any stream, including
  // stream 0 and a stream mixing ordinary attributes with palette elements.
  decl.vertexElementCount = 3;
  for (uint16_t slot : {0u, 1u, 7u, 15u}) {
    producer.streamSources[slot] = 64;
    producer.streamStrideDwords[slot] = 7;
    fetch[-int(slot) * 2] = slot == 0 ? base : paletteBase;
    fetch[-int(slot) * 2 + 1] = fullSize;
    for (uint8_t semantic : {1u, 2u, 3u}) {
      decl.vertexElements[2] = {slot, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITION, semantic};
      auto palette = snapshot().setDrawGeometrySnapshot.streams[slot];
      assert(palette.rawSize == fullSize);
      bytes[(slot == 0 ? base : paletteBase) + fullSize - 1] ^= 1;
      assert(snapshot().setDrawGeometrySnapshot.streams[slot].rawIdentity != palette.rawIdentity);
      if (slot != 0) assert(snapshot().setDrawGeometrySnapshot.streams[0].rawSize == 252);
    }
    if (slot != 0) producer.streamSources[slot] = 0;
  }
  decl.vertexElementCount = 2;
  // Partial attribute footprint, endian dword rounding and oversized inputs.
  decl.vertexElements[1] = {0, 13, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0};
  assert(NonIndexedVertexSnapshotSize(&decl, 0, 28, fullSize, 5, 4) == 244);
  assert(NonIndexedVertexSnapshotSize(&decl, 0, 28, 240, 5, 4) == 240);
  assert(NonIndexedVertexSnapshotSize(&decl, 0, 0, fullSize, 0, 4) == fullSize);
  assert(NonIndexedVertexSnapshotSize(&decl, 0, UINT32_MAX, fullSize,
                                     UINT32_MAX, UINT32_MAX) == fullSize);
  assert(snapshot(UINT32_MAX, UINT32_MAX).setDrawGeometrySnapshot.streams[0].rawSize == fullSize);
  assert(NonIndexedVertexSnapshotSize(&decl, 3, 28, fullSize, 0, 4) == fullSize);
  RecordedRenderBatch retained;
  retained.Append(changed);
  g_intermediaryUploadAllocator.Reset();
  std::fill(bytes.begin(), bytes.end(), 0);
  const auto& saved = retained.commands()[0].setDrawGeometrySnapshot.streams[0];
  assert(saved.rawSize == 252 && saved.rawData[248] == 8);
  g_producerGeometry.clear();
  std::puts("Draw ranges: actual producer snapshots, GPU bytes, palette preservation, recording isolation and range guards passed");
}
'''

MAIN = r'''
int main() {
  CheckDrawRanges();
  CheckShaderConstants();
  creates = checks = maps = unmaps = 0;
  lastWritten = 0;
  ByteSnapshotCache snapshots;
  uint8_t source[] = {0,1,2,3,4,5,6};
  auto* original = snapshots.Copy(source, sizeof(source));
  const unsigned firstHash = hashCalls;
  auto* swapped = snapshots.Copy(source, sizeof(source), 4);
  auto* words = snapshots.Copy(source, sizeof(source), 2);
  const uint8_t expected[] = {3,2,1,0,4,5,6};
  const uint8_t expectedWords[] = {1,0,3,2,5,4,6};
  assert(std::memcmp(swapped, expected, sizeof(source)) == 0);
  assert(std::memcmp(words, expectedWords, sizeof(source)) == 0);
  assert(snapshots.Copy(source, sizeof(source), 2) == words);
  assert(snapshots.Copy(source, sizeof(source), 4) == swapped);
  assert(snapshots.Copy(source, sizeof(source)) == original);
  assert(hashCalls == firstHash);  // Repeated content needs no second hash pass.
  // SIMD chunk boundaries, residual elements, odd lengths and unaligned sources.
  std::array<uint8_t, 80> boundaryBytes;
  for (uint32_t i = 0; i < boundaryBytes.size(); ++i) boundaryBytes[i] = uint8_t(i);
  for (uint32_t length = 1; length <= 65; ++length) {
    for (uint32_t offset = 0; offset < 4; ++offset) {
      for (uint32_t swap : {2u, 4u}) {
        const auto* converted = snapshots.Copy(boundaryBytes.data() + offset, length, swap);
        for (uint32_t i = 0; i < length; ++i) {
          const uint32_t index = i < length / swap * swap
              ? i / swap * swap + swap - 1 - i % swap : i;
          assert(converted[i] == boundaryBytes[offset + index]);
        }
      }
    }
  }
  assert(snapshots.Copy(source, sizeof(source) - 1) != original);
  assert(snapshots.Copy(source, sizeof(source)) == original);
  uint8_t otherAddress[] = {0,1,2,3,4,5,6};
  assert(snapshots.Copy(otherAddress, sizeof(otherAddress)) == original);
  source[0] = 9;
  assert(snapshots.Copy(source, sizeof(source)) != original);
  assert(original[0] == 0 && swapped[3] == 0);
  assert(snapshots.Copy(nullptr, 4) == nullptr && snapshots.Copy(source, 0) == nullptr);
  // Address hints must remain valid through a deep copy and be cleared with
  // their content entries; recycled addresses must see the new bytes.
  ByteSnapshotCache copied = snapshots;
  snapshots.Clear();
  assert(copied.Copy(source, sizeof(source))[0] == 9);
  source[0] = 10;
  assert(snapshots.Copy(source, sizeof(source))[0] == 10);
  assert(copied.Copy(source, sizeof(source))[0] == 10);
  for (uint8_t value = 11; value < 25; ++value) {
    source[sizeof(source) - 1] = value;
    assert(copied.Copy(source, sizeof(source))[sizeof(source) - 1] == value);
  }
  // Once a changing buffer stabilizes, the no-hash path becomes usable again.
  auto* stable = copied.Copy(source, sizeof(source));
  const unsigned stableHashes = hashCalls;
  for (unsigned i = 0; i < 20; ++i) assert(copied.Copy(source, sizeof(source)) == stable);
  assert(hashCalls == stableHashes);

  // Retired frame payload storage is reused, with fresh contents, converted
  // variants and identities. Only Clear permits overwriting an old generation.
  ByteSnapshotCache recycled;
  std::vector<uint8_t> recycleSource(131075, 0);
  uint64_t recycledIdentity = 0;
  unsigned warmAllocations = 0;
  for (unsigned generation = 0; generation < 5; ++generation) {
    const uint32_t length = uint32_t(recycleSource.size()) - generation;
    std::fill(recycleSource.begin(), recycleSource.end(), uint8_t(generation));
    for (uint32_t swap : {0u, 2u, 4u}) {
      uint64_t id = 0;
      const auto* bytes = recycled.Copy(recycleSource.data(), length, swap, &id);
      assert(bytes[0] == generation && bytes[length - 1] == generation);
      assert(id != 0 && id != recycledIdentity);
      recycledIdentity = id;
    }
    recycled.Clear();
    if (generation == 0) warmAllocations = largeAllocations;
    else assert(largeAllocations == warmAllocations);
  }

  // The real guest snapshot path must reuse unchanged full palettes, observe
  // mutations at the end, and retain old converted bytes for queued draws.
  constexpr uint32_t paletteBytes = 221760;
  auto& guest = ghp::memory;
  std::fill(guest.bytes.begin(), guest.bytes.end(), 7);
  const auto capture = [](uint32_t swap = 4, uint64_t* identity = nullptr) {
    return SnapshotRawPhysicalBuffer(0, paletteBytes, swap, false, identity);
  };
  uint64_t stagedIdentity = 0, changedIdentity = 0;
  uint8_t* staged = capture(4, &stagedIdentity);
  assert(staged && stagedIdentity != 0 && staged[paletteBytes - 1] == 7);
  for (unsigned i = 0; i < 2000; ++i) assert(capture() == staged);
  // Cache mutation is serialized across producers; consumers only read bytes.
  std::vector<std::thread> producers;
  for (unsigned i = 0; i < 4; ++i) {
    producers.emplace_back([&] {
      for (unsigned j = 0; j < 30; ++j)
        assert(g_intermediaryUploadAllocator.CopyCached(guest.bytes.data(), paletteBytes, 4) == staged);
    });
  }
  for (auto& producer : producers) producer.join();
  guest.bytes[paletteBytes - 1] = 8;
  auto* changedStaged = capture(4, &changedIdentity);
  assert(changedIdentity != 0 && changedIdentity != stagedIdentity);
  assert(changedStaged != staged && changedStaged[paletteBytes - 4] == 8);
  assert(staged[paletteBytes - 4] == 7);
  guest.bytes[paletteBytes - 1] = 7;
  assert(capture(4, &changedIdentity) == staged && changedIdentity == stagedIdentity);
  // A recorded batch owns its bytes even after producer staging retires.
  RecordedRenderBatch retained;
  RenderCommand stagedCommand{};
  stagedCommand.type = RenderCommandType::SetDrawGeometrySnapshot;
  stagedCommand.setDrawGeometrySnapshot.streams[0] = {nullptr, 0, 24, staged, paletteBytes, stagedIdentity};
  retained.Append(stagedCommand);
  const auto& recordedSnapshot = retained.commands()[0].setDrawGeometrySnapshot.streams[0];
  assert(recordedSnapshot.rawIdentity != 0 && recordedSnapshot.rawIdentity != stagedIdentity);
  g_intermediaryUploadAllocator.Reset();
  guest.bytes[paletteBytes - 1] = 9;
  assert(capture()[paletteBytes - 4] == 9);
  assert(retained.commands()[0].setDrawGeometrySnapshot.streams[0].rawData[paletteBytes - 4] == 7);
  guest.readable = false;
  assert(capture(4, &changedIdentity) == nullptr && changedIdentity == 0);
  guest.readable = true;
  guest.nullSource = true;
  assert(capture() == nullptr);
  guest.nullSource = false;
  ghp::noMemory = true;
  assert(capture() == nullptr);
  ghp::noMemory = false;
  const unsigned translations = guest.translations;
  assert(SnapshotRawPhysicalBuffer(0, 0, 4, false) == nullptr);
  assert(SnapshotRawPhysicalBuffer(0, 4 * 1024 * 1024 + 4, 4, false) == nullptr);
  assert(SnapshotRawPhysicalBuffer(0x1FFFFFFC, 8, 4, false) == nullptr);
  assert(guest.translations == translations);
  for (unsigned i = 0; i < 16; ++i) guest.bytes[i] = uint8_t(i);
  std::vector<uint64_t> identities;
  for (uint32_t swap : {0u, 2u, 4u}) {
    for (bool preserve : {false, true}) {
      uint64_t identity = 0;
      auto* data = SnapshotRawPhysicalBuffer(0xA0000001, 8, swap, preserve, &identity);
      assert(identity != 0 && std::find(identities.begin(), identities.end(), identity) == identities.end());
      identities.push_back(identity);
      const uint32_t base = preserve ? 1 : 0;
      assert(guest.lastAddress == base);
      for (uint32_t i = 0; i < 8; ++i)
        assert(data[i] == base + (swap ? (i / swap) * swap + swap - 1 - i % swap : i));
    }
  }
  assert(SnapshotRawPhysicalBuffer(0, 4 * 1024 * 1024, 4, false));

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
  const uint64_t recordedIdentity = batch.commands()[0].setDrawGeometrySnapshot.streams[1].rawIdentity;
  assert(recordedIdentity != 0 && recordedIdentity == batch.commands()[1].setDrawGeometrySnapshot.streams[1].rawIdentity);
  palette.back() = 9;
  batch.Append(command);
  assert(recorded[palette.size()-1] == 8);
  assert(batch.commands()[2].setDrawGeometrySnapshot.streams[1].rawData[palette.size()-1] == 9);
  assert(batch.commands()[2].setDrawGeometrySnapshot.streams[1].rawIdentity != recordedIdentity);

  lost = false;
  UploadAllocator immutableUploads;
  uint64_t identity = 0;
  auto* bytes = capture(4, &identity);
  const unsigned hashesBeforeUpload = hashCalls;
  const auto immutableFirst = immutableUploads.UploadSnapshot(bytes, paletteBytes, identity);
  assert(immutableFirst.ref);
  for (unsigned i = 0; i < 2000; ++i) {
    const auto repeated = immutableUploads.UploadSnapshot(bytes, paletteBytes, identity);
    assert(repeated.ref == immutableFirst.ref && repeated.offset == immutableFirst.offset);
  }
  assert(hashCalls == hashesBeforeUpload);
  const auto* immutableBuffer = static_cast<const MemoryBuffer*>(immutableFirst.ref);
  assert(std::memcmp(immutableBuffer->bytes.data() + immutableFirst.offset, bytes, paletteBytes) == 0);
  guest.bytes[paletteBytes - 1] ^= 0xFF;
  const uint64_t previousIdentity = identity;
  bytes = capture(4, &identity);
  assert(identity != previousIdentity);
  const auto immutableChanged = immutableUploads.UploadSnapshot(bytes, paletteBytes, identity);
  assert(immutableChanged.ref == immutableFirst.ref && immutableChanged.offset >= paletteBytes);
  assert(immutableBuffer->bytes[immutableFirst.offset + paletteBytes - 4] != bytes[paletteBytes - 4]);
  // CPU address reuse across cache generations cannot alias a GPU upload.
  g_intermediaryUploadAllocator.Reset();
  const uint64_t retiredIdentity = identity;
  bytes = capture(4, &identity);
  assert(identity != retiredIdentity);
  const auto regenerated = immutableUploads.UploadSnapshot(bytes, paletteBytes, identity);
  assert(regenerated.offset > immutableChanged.offset);
  immutableUploads.Reset();
  assert(immutableUploads.UploadSnapshot(bytes, paletteBytes, identity).offset == 0);
  // Callers without immutable provenance still receive full content validation.
  auto fallback = immutableUploads.UploadSnapshot(palette.data(), uint32_t(palette.size()), 0);
  palette.back() ^= 0xFF;
  assert(immutableUploads.UploadSnapshot(palette.data(), uint32_t(palette.size()), 0).offset != fallback.offset);
  lost = true;
  assert(!immutableUploads.UploadSnapshot(bytes, paletteBytes, identity).ref);
  lost = false;
  // Texture placements share the existing frame arena but need 512-byte
  // alignment even after a smaller constant/geometry allocation.
  UploadAllocator mixed;
  assert(mixed.Upload(source, sizeof(source), false).offset == 0);
  const auto texturePlacement = mixed.Upload(source, sizeof(source), false, 512);
  assert(texturePlacement.ref && texturePlacement.offset == 512);
  assert(std::memcmp(static_cast<const MemoryBuffer*>(texturePlacement.ref)->bytes.data() + 512,
                     source, sizeof(source)) == 0);
  mixed.Reset();
  assert(mixed.Upload(source, sizeof(source), false, 512).offset == 0);
  UploadAllocator edge;
  std::vector<uint8_t> almostFull(64 * 1024 * 1024 - 256, 9);
  const auto preceding = edge.Upload(almostFull.data(), almostFull.size(), false);
  const auto nextChunk = edge.Upload(source, sizeof(source), false, 512);
  assert(nextChunk.ref && nextChunk.ref != preceding.ref && nextChunk.offset == 0);
  g_intermediaryUploadAllocator.Reset();
  auto& watched = ghp::memory;
  std::fill(watched.bytes.begin(), watched.bytes.end(), 0x12);
  g_physicalWriteWatch.Initialize(&watched);
  uint64_t trackedId = 0;
  auto* tracked = SnapshotRawPhysicalBuffer(4096, 65536, 4, false, &trackedId);
  assert(tracked && trackedId);
  const unsigned initialHashes = hashCalls;
  uint64_t reusedId = 0;
  assert(SnapshotRawPhysicalBuffer(4096, 65536, 4, false, &reusedId) == tracked);
  assert(trackedId == reusedId && hashCalls == initialHashes);
  // Change DURING arming must update this draw, not just the following one.
  watched.invalidateOnArm = true;
  auto* armChanged = SnapshotRawPhysicalBuffer(4096, 65536, 4, false, &reusedId);
  assert(armChanged[3] == uint8_t(0x12 ^ 0xFF) && tracked[3] == 0x12);
  assert(reusedId != trackedId);
  // Failed protection cannot authorize reuse of a retained revision.
  SnapshotRawPhysicalBuffer(4096, 65536, 4, false, &reusedId);
  watched.armFails = true;
  watched.bytes[4096 + 65535] = 0x34;
  auto* unprotected = SnapshotRawPhysicalBuffer(4096, 65536, 4, false);
  assert(unprotected[65532] == 0x34 && armChanged[65532] == 0x12);
  watched.armFails = false;
  const auto retiredRevision = g_physicalWriteWatch.BeginSnapshot(&watched, 4096, 65536);
  g_physicalWriteWatch.Shutdown();
  assert(!watched.callback && g_physicalWriteWatch.BeginSnapshot(&watched, 4096, 65536) == 0);
  g_physicalWriteWatch.Initialize(&watched);
  assert(g_physicalWriteWatch.BeginSnapshot(&watched, 4096, 65536) > retiredRevision);
  assert(g_physicalWriteWatch.BeginSnapshot(nullptr, 4096, 65536) == 0);
  assert(g_physicalWriteWatch.BeginSnapshot(&watched, 0x1FFFFFF0, 4096) == 0);
  assert(g_physicalWriteWatch.BeginSnapshot(&watched, 0, 8) == 0);
  // Multiple sizes and endian views at one address keep distinct entries.
  ByteSnapshotCache revisions;
  uint64_t idA = 0, idB = 0;
  auto* a = revisions.Copy(watched.bytes.data(), 4096, 0, &idA, 7);
  auto* b = revisions.Copy(watched.bytes.data(), 8192, 4, &idB, 7);
  assert(idA != idB && a != b);
  assert(revisions.Copy(watched.bytes.data(), 4096, 0, nullptr, 7) == a);
  ByteSnapshotCache cloned = revisions;
  auto* clone = cloned.Copy(watched.bytes.data(), 4096, 0, nullptr, 7);
  assert(clone != a && std::memcmp(clone, a, 4096) == 0);
  watched.bytes[1] ^= 0xFF;
  assert(revisions.Copy(watched.bytes.data(), 4096, 0, nullptr, 8)[1] == watched.bytes[1]);
  watched.bytes[2] ^= 0xFF;
  assert(revisions.Copy(watched.bytes.data(), 4096)[2] == watched.bytes[2]);
  revisions.Clear();
  revisions.Copy(watched.bytes.data(), 4096, 0, &idB, 8);
  assert(idB != idA);
  g_physicalWriteWatch.Shutdown();
  std::puts("Upload cache: immutable identities, producer validation, endian modes, full palettes, range guards, written ranges, reset, failures, and batch ownership passed");
}
'''


CONSTANTS = r'''
void CheckShaderConstants() {
  UploadAllocator slots[2];
  auto bind = [&](unsigned slot, bool vertex) {
    return slots[slot].UploadAndBindRootDescriptor(
        vertex ? g_vertexShaderConstants : g_pixelShaderConstants, 4096, vertex ? 0 : 1, true,
        vertex ? g_dirtyStates.vertexShaderConstants : g_dirtyStates.pixelShaderConstants);
  };
  auto equalRef = [](auto a, auto b) { return a.ref == b.ref && a.offset == b.offset; };
  auto data = [](auto ref) { return static_cast<const MemoryBuffer*>(ref.ref)->bytes.data() + ref.offset; };
  const unsigned hashes = hashCalls;
  assert(bind(0, true) && bind(0, false));
  auto vsInitial = commands.roots[0], psInitial = commands.roots[1];
  uint32_t values[] = {0x01234567u, 0x89ABCDEFu, 0x76543210u, 0xFEDCBA98u};
  const auto* bytes = reinterpret_cast<const uint8_t*>(values);
  // A high-register update changes only VS, and earlier draws retain zeros.
  ProcSetShaderConstants(true, bytes, 1020, sizeof(values));
  assert(g_dirtyStates.vertexShaderConstants && !g_dirtyStates.pixelShaderConstants);
  assert(bind(0, true) && bind(0, false));
  const auto vsChanged = commands.roots[0];
  assert(!equalRef(vsInitial, vsChanged) && equalRef(psInitial, commands.roots[1]));
  assert(data(vsInitial)[4080] == 0 && data(vsChanged)[4080] == 0x01 && data(vsChanged)[4095] == 0x98);
  const unsigned bindsBefore = commands.binds;
  for (unsigned i = 0; i < 2000; ++i) {
    ProcSetShaderConstants(true, bytes, 1020, sizeof(values));
    assert(!g_dirtyStates.vertexShaderConstants);
    assert(bind(0, true) && bind(0, false));
    assert(equalRef(vsChanged, commands.roots[0]) && equalRef(psInitial, commands.roots[1]));
  }
  assert(commands.binds == bindsBefore + 4000 && hashCalls == hashes);
  ProcSetShaderConstants(false, bytes, 0, sizeof(values));
  assert(!g_dirtyStates.vertexShaderConstants && g_dirtyStates.pixelShaderConstants);
  assert(bind(0, false) && data(commands.roots[1])[0] == 0x01 && data(psInitial)[0] == 0);
  // Invalid ranges cannot dirty either stage or overwrite its register file.
  for (const auto [index, size] : {std::pair{1024u, 4u}, {1023u, 8u}, {0u, 3u}, {UINT32_MAX, 4u}})
    ProcSetShaderConstants(true, bytes, index, size);
  ProcSetShaderConstants(true, nullptr, 0, 4);
  assert(!g_dirtyStates.vertexShaderConstants && g_vertexShaderConstants[1020] == values[0]);
  // Recorded draws replace constants, then replay restores through this same
  // setter. The following live draw must see the saved bytes again.
  std::array<uint32_t, 1024> saved;
  std::copy_n(g_vertexShaderConstants, 1024, saved.begin());
  ProcSetShaderConstants(true, bytes, 0, sizeof(values));
  assert(bind(0, true));
  const auto replay = commands.roots[0];
  assert(data(replay)[0] == 0x01);
  ProcSetShaderConstants(true, reinterpret_cast<const uint8_t*>(saved.data()), 0, sizeof(saved));
  assert(bind(0, true) && data(commands.roots[0])[0] == 0 && data(replay)[0] == 0x01);
  // An unused frame slot needs its own allocation even if registers are clean.
  assert(bind(1, true) && commands.roots[0].ref != vsChanged.ref);
  slots[0].Reset();
  assert(bind(0, true) && commands.roots[0].offset == 0 && data(commands.roots[0])[4080] == 0x01);
  // Command-list begin/replay marks all states dirty; clean draws still rebind.
  g_dirtyStates = DirtyStates(true);
  assert(g_dirtyStates.vertexShaderConstants && g_dirtyStates.pixelShaderConstants);
  assert(bind(0, true) && bind(0, false));
  lost = true;
  assert(!bind(0, true));
  lost = false;
  slots[0].Reset();
  mapFails = true;
  g_dirtyStates.vertexShaderConstants = true;
  assert(!bind(0, true) && g_dirtyStates.vertexShaderConstants);
  mapFails = false;
  slots[0].Reset();
  assert(bind(0, true) && !g_dirtyStates.vertexShaderConstants);
  assert(!slots[0].UploadAndBindRootDescriptor(bytes, 16, 2, true, g_dirtyStates.vertexShaderConstants));
  slots[0].FinishWrites(); slots[1].FinishWrites();
  assert(hashCalls == hashes);
  std::puts("Shader constants: independent stage updates, unchanged binds, high registers, immutable draws, restore, slot reset and failure retry passed");
}
'''


def test_upload_cache():
    source = (ROOT / "pgr4-recomp/src/render/render_state.cpp").read_text(encoding="utf-8")
    allocator = re.search(r"^class UploadAllocator \{.*?^};", source, re.M | re.S)
    intermediary = re.search(r"^class IntermediaryUploadAllocator \{.*?^};", source, re.M | re.S)
    snapshot = re.search(r"^uint32_t DecodeRawBufferSize\(.*?^}\n\nuint8_t\* SnapshotRawPhysicalBuffer\(.*?^}", source, re.M | re.S)
    assert allocator is not None and intermediary is not None and snapshot is not None
    geometry = []
    for signature in ("struct ProducerGeometryState", "RenderFormat ConvertDeclType(",
                      "uint32_t DeclTypeByteSize(", "bool DeclarationFitsStreamStride(",
                      "uint32_t NonIndexedVertexSnapshotSize(", "void SetVertexDeclaration(",
                      "struct LocalRenderCommandQueue", "void QueueDrawGeometrySnapshot("):
        if signature == "struct ProducerGeometryState":
            geometry.append(source[source.index(signature):source.index("}  // namespace", source.index(signature))])
        else:
            match = re.search(r"^" + re.escape(signature) + r".*?^};?", source, re.M | re.S)
            assert match is not None, signature
            geometry.append(match.group())
    assert "QueueDrawStateSnapshots(device, queue, startVertex, vertexCount);" in source
    # Compile the actual x64 runtime conversions without unrelated cvar setup.
    memory = (ROOT / "src/core/memory.cpp").read_text(encoding="utf-8")
    conversions = []
    for bits in (16, 32):
        match = re.search(rf"^void copy_and_swap_{bits}_unaligned\(.*?^}}", memory, re.M | re.S)
        assert match is not None
        conversions.append(match.group())
    conversion_source = ("\n#define REX_WORKAROUND_CONSTANT_RETURN_IF(x)\nnamespace rex::memory {\n"
                         + "\n".join(conversions) + "\n}\n")
    dirty = re.search(r"^struct DirtyStates \{.*?^};", source, re.M | re.S).group()
    files = "\n".join(re.findall(r"^alignas\(16\) uint32_t g_(?:vertex|pixel)ShaderConstants.*?$", source, re.M))
    setter = re.search(r"^void ProcSetShaderConstants\(.*?^}", source, re.M | re.S).group()
    constants = dirty + "\nDirtyStates g_dirtyStates(true);\n" + files + "\n" + setter + CONSTANTS
    # Both normal frame begin and replay restore must invalidate root uploads.
    for signature in ("void ProcBeginRenderStateFrame(", "void DispatchRecordedRenderCommands("):
        body = re.search(r"^" + re.escape(signature) + r".*?^}", source, re.M | re.S).group()
        assert "g_dirtyStates = DirtyStates(true);" in body
    assert "reinterpret_cast<const uint8_t*>(saved.vertexConstants.data())" in source
    assert "reinterpret_cast<const uint8_t*>(saved.pixelConstants.data())" in source
    watch = (ROOT / "pgr4-recomp/src/render/physical_write_watch.h").read_text(encoding="utf-8")
    watch = watch.replace("#pragma once", "").replace("#include <rex/system/xmemory.h>", "")
    watch = watch.replace("rex::memory::Memory", "ghp::Memory")
    with tempfile.TemporaryDirectory(prefix="pgr4-upload-") as folder:
        cpp, exe = Path(folder) / "test.cpp", Path(folder) / "test.exe"
        cpp.write_text(PREAMBLE + watch + conversion_source + allocator.group() + intermediary.group() +
                       '\nIntermediaryUploadAllocator g_intermediaryUploadAllocator;\n' +
                       snapshot.group() + '\n'.join(geometry) + DRAW_RANGES + constants + MAIN, encoding="utf-8")
        for defines in ([], ["-DFORCE_HASH_COLLISIONS"]):
            subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++23", "-O1", "-msse4.1", *defines,
                        "-DNOMINMAX", "-DWIN32_LEAN_AND_MEAN",
                        *[f"-I{ROOT / path}" for path in
                          ["include", "thirdparty/plume", "thirdparty/xxHash", "pgr4-recomp/src", "pgr4-recomp"]],
                            str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    test_upload_cache()
