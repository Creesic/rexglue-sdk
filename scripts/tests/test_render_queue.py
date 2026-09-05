"""Exercise the production worker queue without a GPU (python, clang++)."""

import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
QUEUE = ROOT / "pgr4-recomp/src/render/render_queue.cpp"

PROGRAM = r'''
#include <cassert>
#include <cstdio>
#include <semaphore>
#include "QUEUE_SOURCE"
using namespace pgr4::render;

// Queue payloads are already in host byte order; conversion is tested separately.
namespace rex::memory {
void copy_and_swap_16_unaligned(void*, const void*, size_t) { assert(false); }
void copy_and_swap_32_unaligned(void*, const void*, size_t) { assert(false); }
}

std::vector<uint32_t> seen;
std::binary_semaphore entered{0}, releaseGate{0};
std::vector<uint32_t> replayValues;
struct RecordingLock {
  std::recursive_mutex mutex;
  std::atomic<uint32_t> acquisitions{0};
  inline static thread_local uint32_t depth = 0;
  void lock() { mutex.lock(); ++acquisitions; ++depth; }
  void unlock() { --depth; mutex.unlock(); }
} recordingLock;
RenderCommand Command(uint32_t value, uint32_t state = 0) {
  RenderCommand c{};
  c.type = RenderCommandType::SetRenderState;
  c.setRenderState = {state, value};
  return c;
}
namespace pgr4::render {
auto& RecordingMutex() { return recordingLock; }
void DispatchRenderCommandUnlocked(const RenderCommand& c) {
  if (c.type == RenderCommandType::WaitForGpu ||
      c.type == RenderCommandType::CreateTranslatedTextureHost) {
    assert(RecordingLock::depth == 0);
    return;
  }
  assert(RecordingLock::depth != 0);
  if (c.setRenderState.state == 1) {
    entered.release();
    releaseGate.acquire();
  } else if (c.setRenderState.state == 2) {
    assert(RenderQueue::IsOnRenderThread());
    RenderQueue::Run(Command(c.setRenderState.value - 1));
  }
  seen.push_back(c.setRenderState.value);
}
DISPATCH_SOURCE
void DispatchRecordedRenderCommands(const RenderCommand* commands, size_t count,
                                    const DeferredExecutionSnapshot* snapshot) {
  assert(count == 1 && commands[0].type == RenderCommandType::SetPixelShaderConstants);
  assert(commands[0].setShaderConstants.size == 16);
  replayValues.push_back(commands[0].setShaderConstants.memory[15]);
  replayValues.push_back(snapshot ? snapshot->booleans[7] : 0);
  replayValues.push_back(snapshot ? snapshot->pixelConstants.back() : 0);
}
}
int main() {
  RenderQueue::Run(Command(1)); // Stopped queue dispatches inline.
  assert((seen == std::vector<uint32_t>{1}));
  RenderQueue::Start();
  RenderQueue::Run(Command(2));
  assert(seen.back() == 2); // Run acknowledges only after dispatch.
  RenderQueue::Enqueue(Command(3, 1));
  entered.acquire();
  RenderCommand bulk[] = {Command(4), Command(5)};
  RenderQueue::EnqueueBulk(bulk, 2);
  {
    std::lock_guard lock(g_mutex);
    assert(g_pending.size() == 2 && g_pendingCount == 2);
  }
  bulk[0] = Command(999); // Queued command storage is owned by the queue.
  std::thread sync([] {
    RenderQueue::Run(Command(6));
    assert(seen.back() == 6);
  });
  releaseGate.release();
  sync.join();
  assert((seen == std::vector<uint32_t>{1,2,3,4,5,6}));
  RenderQueue::Run(Command(8, 2));
  assert(seen[6] == 7 && seen[7] == 8); // Nested Run cannot deadlock.

  // The production dispatcher locks once per contiguous ordinary batch and
  // releases before both exempt synchronous command types.
  const uint32_t locks = recordingLock.acquisitions;
  RenderCommand mixed[] = {Command(20), Command(21), {}, Command(22), {}, Command(23)};
  mixed[2].type = RenderCommandType::WaitForGpu;
  mixed[4].type = RenderCommandType::CreateTranslatedTextureHost;
  RenderQueue::EnqueueBulk(mixed, std::size(mixed));
  RenderQueue::Run(Command(24));
  // Three spans, plus one for Run unless the worker coalesced it into the
  // batch holding the trailing span.
  assert(recordingLock.acquisitions >= locks + 3 && recordingLock.acquisitions <= locks + 4);
  assert((std::vector<uint32_t>(seen.end()-5, seen.end()) == std::vector<uint32_t>{20,21,22,23,24}));
  // In the real wait/creation paths the producer can hold RecordingMutex
  // while Run waits for the worker. Neither exception may reacquire it.
  {
    std::lock_guard lock(recordingLock);
    RenderQueue::Run(mixed[2]);
    RenderQueue::Run(mixed[4]);
  }

  // Each producer's bulk remains contiguous and ordered amid other producers.
  seen.clear();
  std::array<std::thread, 4> producers;
  for (uint32_t p = 0; p < producers.size(); ++p) {
    producers[p] = std::thread([p] {
      for (uint32_t i = 0; i < 200; ++i) {
        const uint32_t id = (p * 200 + i) * 3;
        RenderCommand batch[] = {Command(id), Command(id+1), Command(id+2)};
        RenderQueue::EnqueueBulk(batch, 3);
      }
    });
  }
  for (auto& producer : producers) producer.join();
  RenderQueue::Run(Command(9999));
  assert(seen.size() == 2401 && seen.back() == 9999);
  std::array<uint32_t, 4> next{};
  for (size_t i = 0; i < 2400; i += 3) {
    const uint32_t id = seen[i], p = id / 600;
    assert(p < 4 && id == (p * 200 + next[p]++) * 3);
    assert(seen[i+1] == id+1 && seen[i+2] == id+2);
  }
  for (auto count : next) assert(count == 200);

  // Replay owns both the recorded bytes and the execution-time snapshot,
  // even when the producer changes its inputs and replaces the recording.
  uint8_t constants[16]{};
  RenderCommand c{};
  c.type = RenderCommandType::SetPixelShaderConstants;
  c.setShaderConstants.memory = constants;
  c.setShaderConstants.size = sizeof(constants);
  auto record = [&] {
    RenderQueue::BeginRecording();
    RenderQueue::Enqueue(c);
    RenderQueue::EndRecording();
    RenderQueue::BindPendingRecording(42);
  };
  constants[15] = 11;
  record();
  RenderQueue::Enqueue(Command(10, 1));
  entered.acquire();
  DeferredExecutionSnapshot snapshot{};
  snapshot.booleans[7] = 123;
  snapshot.pixelConstants.back() = 79;
  assert(RenderQueue::ReplayRecording(42, &snapshot));
  snapshot = {};
  constants[15] = 22;
  record();
  assert(RenderQueue::ReplayRecording(42, nullptr));
  constants[15] = 99;
  releaseGate.release();
  RenderQueue::Run(Command(11));
  assert((replayValues == std::vector<uint32_t>{11,123,79,22,0,0}));
  assert(!RenderQueue::ReplayRecording(404, nullptr));

  // Stop drains work already taken by the worker plus newer pending jobs.
  seen.clear();
  RenderQueue::Enqueue(Command(1, 1));
  entered.acquire();
  RenderCommand tail[] = {Command(2, 1), Command(3)};
  RenderQueue::EnqueueBulk(tail, 2);
  releaseGate.release();
  entered.acquire();
  RenderQueue::Enqueue(Command(4));
  std::thread stopping([] { RenderQueue::Stop(); });
  while (g_running.load(std::memory_order_acquire)) std::this_thread::yield();
  releaseGate.release();
  stopping.join();
  assert((seen == std::vector<uint32_t>{1,2,3,4}));
  assert(!RenderQueue::ReplayRecording(42, nullptr));
  RenderQueue::Start();
  RenderQueue::Run(Command(5));
  RenderQueue::Stop();
  assert(seen.back() == 5);
  // Stopped batches still use the same dispatcher and one lock.
  const uint32_t stoppedLocks = recordingLock.acquisitions;
  RenderCommand stopped[] = {Command(6), Command(7)};
  RenderQueue::EnqueueBulk(stopped, 2);
  assert(recordingLock.acquisitions == stoppedLocks + 1 && seen.back() == 7);
  std::printf("Render queue: owned batches, lock scopes/exemptions, FIFO, concurrent bulk, sync/nested Run, replay ownership, stop/restart passed; RenderCommand=%zu bytes\n", sizeof(RenderCommand));
}
'''


def compile_program(program, queue, folder, name="test"):
    folder = Path(folder)
    stub = folder / "rex/logging.h"
    stub.parent.mkdir(exist_ok=True)
    stub.write_text("#pragma once\n#define REXGPU_INFO(...) ((void)0)\n", encoding="utf-8")
    cpp, exe = folder / f"{name}.cpp", folder / f"{name}.exe"
    cpp.write_text(program.replace("QUEUE_SOURCE", Path(queue).as_posix()), encoding="utf-8")
    subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++23", "-O2",
                    f"-I{folder}", f"-I{ROOT / 'include'}",
                    f"-I{ROOT / 'thirdparty/xxHash'}", f"-I{ROOT / 'pgr4-recomp/src'}",
                    str(cpp), "-o", str(exe)], check=True)
    return exe


def test_render_queue():
    source = (ROOT / "pgr4-recomp/src/render/render_state.cpp").read_text(encoding="utf-8")
    dispatch = []
    for signature in ("void DispatchRenderCommands(", "void DispatchRenderCommand("):
        match = re.search(r"^" + re.escape(signature) + r".*?^}", source, re.M | re.S)
        assert match is not None, signature
        dispatch.append(match.group())
    with tempfile.TemporaryDirectory(prefix="pgr4-queue-") as folder:
        exe = compile_program(PROGRAM.replace("DISPATCH_SOURCE", '\n'.join(dispatch)), QUEUE, folder)
        subprocess.run([str(exe)], check=True, timeout=30)


if __name__ == "__main__":
    test_render_queue()
