"""Real SDK/Windows write faults and snapshot benchmark; build rexruntime first.

python scripts/tests/test_physical_write_watch.py [--benchmark]
Uses the production snapshot function/allocator and SDK DLL, without launching PGR4.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SDK = ROOT / "out/win-amd64/RelWithDebInfo"

PREAMBLE = r'''
#include <cassert>
#include <chrono>
#include <cstdio>
#include <thread>
#include <windows.h>
#include <rex/logging.h>
#include "render/byte_snapshot_cache.h"
#include "render/physical_write_watch.h"
using namespace pgr4::render;
rex::memory::Memory memory;
namespace ghp {
auto* GuestMemory() { return &memory; }
uint32_t HeaderBaseToPhysical(uint32_t address) { return address & 0x1FFFFFFFu; }
}
'''

MAIN = r'''
int main(int argc, char**) {
  rex::InitLogging();
  assert(memory.Initialize());
  constexpr uint32_t size = 262144, payload = 221760;
  constexpr auto flags = rex::memory::kMemoryAllocationReserve | rex::memory::kMemoryAllocationCommit;
  constexpr auto rw = rex::memory::kMemoryProtectRead | rex::memory::kMemoryProtectWrite;
  auto& watch = g_physicalWriteWatch;
  // Each guest alias, including E's host offset, must fault and invalidate.
  for (uint32_t alias : {0xA0000000u, 0xC0000000u, 0xE0000000u}) {
    auto* heap = static_cast<rex::memory::PhysicalHeap*>(memory.LookupHeap(alias));
    uint32_t address = 0;
    assert(heap->Alloc(size, 4096, flags, rw, false, &address));
    const uint32_t physical = heap->GetPhysicalAddress(address);
    auto* guest = memory.TranslateVirtual<volatile uint8_t*>(address);
    guest[0] = 7;
    guest[payload-1] = 9;
    watch.Initialize(&memory);
    g_intermediaryUploadAllocator.Reset();
    uint64_t originalId = 0, id = 0;
    auto* original = SnapshotRawPhysicalBuffer(physical, payload, 0, false, &originalId);
    assert(original && original[0] == 7 && original[payload-1] == 9);
    assert(SnapshotRawPhysicalBuffer(physical, payload, 0, false, &id) == original && id == originalId);
    MEMORY_BASIC_INFORMATION info{};
    assert(VirtualQuery(const_cast<uint8_t*>(guest), &info, sizeof(info)));
    assert(info.Protect == PAGE_READONLY);
    // Real CPU store through a watched alias invokes the SDK exception handler.
    guest[payload-1] = 10;
    auto* changed = SnapshotRawPhysicalBuffer(physical, payload, 0, false, &id);
    assert(id != originalId && changed[payload-1] == 10 && original[payload-1] == 9);
    auto beforeWrite = watch.Revision(physical, payload);
    std::thread writer([&] { guest[0] = 11; });
    writer.join();
    assert(watch.Revision(physical, payload) > beforeWrite);
    assert(SnapshotRawPhysicalBuffer(physical, payload, 0, false)[0] == 11);
    // File/command-processor/audio stores use the direct physical mapping.
    memory.TranslatePhysical<uint8_t*>(physical)[0] = 12;
    memory.NotifyPhysicalMemoryWritten(physical, payload);
    assert(SnapshotRawPhysicalBuffer(physical, payload, 0, false)[0] == 12);
    // Registering while initially read-only must survive Protect(write).
    watch.Shutdown();
    assert(heap->Protect(address, size, rex::memory::kMemoryProtectRead));
    watch.Initialize(&memory);
    auto revision = watch.BeginSnapshot(&memory, physical, payload);
    assert(revision);
    assert(heap->Protect(address, size, rw));
    assert(watch.Revision(physical, payload) > revision);
    guest[0] = 13;
    assert(SnapshotRawPhysicalBuffer(physical, payload, 0, false)[0] == 13);
    // Recommitting an already committed allocation can restore host RW access.
    assert(heap->AllocFixed(address, size, 4096, rex::memory::kMemoryAllocationCommit, rw));
    guest[0] = 15;
    assert(SnapshotRawPhysicalBuffer(physical, payload, 0, false)[0] == 15);
    revision = watch.BeginSnapshot(&memory, physical, payload);
    assert(heap->Decommit(address, size));
    assert(watch.Revision(physical, payload) > revision);
    assert(heap->AllocFixed(address, size, 4096, rex::memory::kMemoryAllocationCommit, rw));
    guest[0] = 14;
    assert(SnapshotRawPhysicalBuffer(physical, payload, 0, false)[0] == 14);
    revision = watch.BeginSnapshot(&memory, physical, payload);
    assert(heap->Release(address));
    assert(watch.Revision(physical, payload) > revision);
    watch.Shutdown();
    std::printf("Alias %08X: CPU writes, thread writes, direct writes, read-only, decommit and release passed\n", alias);
  }
  {
    auto* a = static_cast<rex::memory::PhysicalHeap*>(memory.LookupHeap(0xA0000000));
    auto* e = static_cast<rex::memory::PhysicalHeap*>(memory.LookupHeap(0xE0000000));
    constexpr uint32_t physical = 0x05000000, addressA = 0xA5000000, addressE = 0xE4FFF000;
    assert(a->AllocFixed(addressA, size, 4096, flags, rw));
    memory.TranslateVirtual<uint8_t*>(addressA)[0] = 21;
    watch.Initialize(&memory);
    g_intermediaryUploadAllocator.Reset();
    auto* original = SnapshotRawPhysicalBuffer(physical, payload, 0, false);
    assert(original[0] == 21);
    assert(e->AllocFixed(addressE, size, 4096, flags, rw));
    memory.TranslateVirtual<uint8_t*>(addressE)[0] = 22;
    assert(SnapshotRawPhysicalBuffer(physical, payload, 0, false)[0] == 22 && original[0] == 21);
    watch.Shutdown();
    // Retire alias metadata once; both aliases share one parent allocation.
    assert(a->rex::memory::BaseHeap::Release(addressA));
    assert(e->Release(addressE));
    std::puts("Mapping another alias invalidates retained snapshots");
  }
  if (argc > 1) {
    auto* heap = static_cast<rex::memory::PhysicalHeap*>(memory.LookupHeap(0xA0000000));
    uint32_t address = 0;
    assert(heap->Alloc(size, 4096, flags, rw, false, &address));
    auto* guest = memory.TranslateVirtual<uint8_t*>(address);
    const uint32_t physical = heap->GetPhysicalAddress(address);
    uint64_t expectedChecksum = 0;
    for (unsigned repetition = 0; repetition < 3; ++repetition) {
      for (bool tracked : {false, true}) {
        watch.Shutdown();
        std::memset(guest, 7, size);
        if (tracked) watch.Initialize(&memory);
        uint64_t checksum = 0;
        const auto start = std::chrono::steady_clock::now();
        for (unsigned frame = 0; frame < 50; ++frame) {
          // Include write faults, first-copy/endian conversion and arena reuse.
          guest[payload-1] = uint8_t(frame);
          g_intermediaryUploadAllocator.Reset();
          uint64_t firstId = 0;
          auto* first = SnapshotRawPhysicalBuffer(physical, payload, 4, false, &firstId);
          for (unsigned draw = 0; draw < 2000; ++draw) {
            uint64_t id = 0;
            auto* bytes = SnapshotRawPhysicalBuffer(physical, payload, 4, false, &id);
            assert(id == firstId && bytes == first && bytes[payload-4] == uint8_t(frame));
            checksum += bytes[payload-4];
          }
        }
        const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now()-start).count();
        if (!expectedChecksum) expectedChecksum = checksum;
        assert(checksum == expectedChecksum);
        std::printf("benchmark tracked=%d run=%u ms=%.3f checksum=%llu\n", tracked, repetition, ms, checksum);
      }
    }
    watch.Shutdown();
    assert(heap->Release(address));
  }
}
'''


def test_physical_write_watch(benchmark=False):
    source = (ROOT / "pgr4-recomp/src/render/render_state.cpp").read_text(encoding="utf-8")
    allocator = re.search(r"^class IntermediaryUploadAllocator \{.*?^};", source, re.M | re.S)
    snapshot = re.search(r"^uint8_t\* SnapshotRawPhysicalBuffer\(.*?^}", source, re.M | re.S)
    assert allocator and snapshot
    with tempfile.TemporaryDirectory(prefix="pgr4-watch-") as folder:
        cpp, exe = Path(folder) / "test.cpp", Path(folder) / "test.exe"
        cpp.write_text(PREAMBLE + allocator.group() +
                       "\nIntermediaryUploadAllocator g_intermediaryUploadAllocator;\n" +
                       snapshot.group() + MAIN, encoding="utf-8")
        subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++23", "-O2", "-g",
                        "-DNOMINMAX", "-DWIN32_LEAN_AND_MEAN", "-DSPDLOG_FMT_EXTERNAL", "-D_DLL", "-D_MT",
                        "-Xclang", "--dependent-lib=msvcrt",
                        *[f"-I{ROOT / p}" for p in ["include", "out/install/win-amd64/include",
                                                    "thirdparty/xxHash", "pgr4-recomp/src"]],
                        str(cpp), str(SDK / "rexruntimerd.lib"), "-o", str(exe)], check=True)
        env = dict(os.environ, PATH=str(SDK) + os.pathsep + os.environ["PATH"])
        subprocess.run([str(exe), *(["--benchmark"] if benchmark else [])], env=env, check=True)


if __name__ == "__main__":
    test_physical_write_watch("--benchmark" in sys.argv)
