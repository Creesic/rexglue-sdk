// render/render_queue.cpp

#include "render/render_queue.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <rex/logging.h>

#ifdef REXGLUE_ENABLE_PROFILING
#include <rex/dbg.h>
#else
#define SCOPE_profile_cpu_f(name)
#define PROFILE_THREAD_ENTER(name)
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace pgr4::render {
namespace {

// Replay of a recorded guest command buffer: owns the replay commands, the
// recording they were built from and the execution-time snapshot.
struct ReplayJob {
  std::shared_ptr<const RecordedRenderBatch> recording;
  std::vector<RenderCommand> commands;
  std::unique_ptr<DeferredExecutionSnapshot> executionSnapshot;
};

// Producers append under g_mutex; the worker swaps the whole vector out and
// dispatches it as one batch (the Unleashed bulk-dequeue shape), so a producer
// call costs one lock and a copy, with no per-command allocation.
// g_pendingCount mirrors g_pending.size() for the worker's lock-free idle
// spin, and producers only notify while the worker is parked on the condition.
std::mutex g_mutex;
std::condition_variable g_cv;
std::vector<RenderCommand> g_pending;
std::atomic<size_t> g_pendingCount{0};
bool g_parked = false;
std::thread g_thread;
std::atomic<bool> g_running{false};
std::thread::id g_renderThreadId{};

std::mutex g_recordingsMutex;
std::unordered_map<uint32_t, std::shared_ptr<RecordedRenderBatch>> g_recordings;
thread_local std::shared_ptr<RecordedRenderBatch> t_recording;
thread_local std::shared_ptr<RecordedRenderBatch> t_pendingRecording;

bool IsRecordable(RenderCommandType type) {
  const uint32_t value = uint32_t(type);
  return (value >= uint32_t(RenderCommandType::SetViewport) &&
          value <= uint32_t(RenderCommandType::SetDrawGeometrySnapshot)) ||
         (value >= uint32_t(RenderCommandType::DrawPrimitive) &&
          value <= uint32_t(RenderCommandType::DrawPrimitiveUP));
}

bool TryRecord(const RenderCommand& command) {
  if (t_recording == nullptr || !IsRecordable(command.type))
    return false;
  t_recording->Append(command);
  return true;
}

bool IsQueueControl(RenderCommandType type) {
  return type == RenderCommandType::Signal || type == RenderCommandType::ReplayRecording;
}

bool DispatchesInline() {
  return !g_running.load(std::memory_order_acquire) ||
         std::this_thread::get_id() == g_renderThreadId;
}

void Push(const RenderCommand* commands, size_t count) {
  bool parked;
  {
    std::lock_guard lock(g_mutex);
    g_pending.insert(g_pending.end(), commands, commands + count);
    g_pendingCount.store(g_pending.size(), std::memory_order_release);
    parked = g_parked;
  }
  if (parked)
    g_cv.notify_one();
}

void ExecuteReplay(ReplayJob& job) {
  DispatchRecordedRenderCommands(job.commands.data(), job.commands.size(),
                                 job.executionSnapshot.get());
}

void ExecuteControl(const RenderCommand& command) {
  if (command.type == RenderCommandType::Signal) {
    auto* done = static_cast<std::atomic<bool>*>(command.signal.done);
    done->store(true, std::memory_order_release);
    done->notify_one();
    return;
  }
  std::unique_ptr<ReplayJob> job(static_cast<ReplayJob*>(command.replayRecording.job));
  ExecuteReplay(*job);
}

// Ordinary commands reach the dispatcher in contiguous runs (one recording
// lock per run); queue-control commands are handled here, in order.
void ExecuteBatch(const RenderCommand* commands, size_t count) {
  size_t start = 0;
  for (size_t i = 0; i < count; ++i) {
    if (!IsQueueControl(commands[i].type))
      continue;
    if (i > start)
      DispatchRenderCommands(commands + start, i - start);
    ExecuteControl(commands[i]);
    start = i + 1;
  }
  if (count > start)
    DispatchRenderCommands(commands + start, count - start);
}

bool TakeBatch(std::vector<RenderCommand>& batch) {
  // Mid-frame the guest emits a command every few microseconds, so spin
  // briefly before parking: the worker then sleeps only between frames and
  // producers never pay a wake-up per command.
  const auto spinUntil = std::chrono::steady_clock::now() + std::chrono::microseconds(100);
  while (g_pendingCount.load(std::memory_order_acquire) == 0 &&
         g_running.load(std::memory_order_acquire) &&
         std::chrono::steady_clock::now() < spinUntil) {
    std::this_thread::yield();
  }
  std::unique_lock lock(g_mutex);
  if (g_pending.empty()) {
    g_parked = true;
    g_cv.wait(lock, [] {
      return !g_pending.empty() || !g_running.load(std::memory_order_acquire);
    });
    g_parked = false;
  }
  if (g_pending.empty())
    return false;  // Stopping; everything taken before this was dispatched.
  batch.swap(g_pending);
  g_pendingCount.store(0, std::memory_order_relaxed);
  return true;
}

void ThreadMain() {
  g_renderThreadId = std::this_thread::get_id();
#if defined(_WIN32)
  // Unleashed pattern: the worker must not lose its core to guest threads.
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
  PROFILE_THREAD_ENTER("PGR4 Render Thread");
  REXGPU_INFO("FM2 render queue: render thread started");
  std::vector<RenderCommand> batch;
  while (TakeBatch(batch)) {
    SCOPE_profile_cpu_f("RenderQueue::ExecuteBatch");
    ExecuteBatch(batch.data(), batch.size());
    batch.clear();
  }
  REXGPU_INFO("FM2 render queue: render thread stopped");
}

}  // namespace

void RenderQueue::Start() {
  bool expected = false;
  if (!g_running.compare_exchange_strong(expected, true))
    return;
  g_thread = std::thread(ThreadMain);
}

void RenderQueue::Stop() {
  {
    std::lock_guard lock(g_mutex);
    if (!g_running.load(std::memory_order_acquire))
      return;
    g_running.store(false, std::memory_order_release);
  }
  g_cv.notify_all();
  if (g_thread.joinable())
    g_thread.join();
  {
    std::lock_guard lock(g_mutex);
    g_pending.clear();
    g_pendingCount.store(0, std::memory_order_relaxed);
  }
  {
    std::lock_guard lock(g_recordingsMutex);
    g_recordings.clear();
  }
}

bool RenderQueue::IsOnRenderThread() {
  return std::this_thread::get_id() == g_renderThreadId;
}

void RenderQueue::Run(const RenderCommand& cmd) {
  if (TryRecord(cmd))
    return;
  if (DispatchesInline()) {
    DispatchRenderCommand(cmd);
    return;
  }

  std::atomic<bool> done{false};
  RenderCommand commands[2] = {cmd, RenderCommand{}};
  commands[1].type = RenderCommandType::Signal;
  commands[1].signal.done = &done;
  Push(commands, 2);
  done.wait(false, std::memory_order_acquire);
}

void RenderQueue::Enqueue(const RenderCommand& cmd) {
  if (TryRecord(cmd))
    return;
  if (DispatchesInline()) {
    DispatchRenderCommand(cmd);
    return;
  }
  Push(&cmd, 1);
}

void RenderQueue::EnqueueBulk(const RenderCommand* commands, size_t count) {
  if (commands == nullptr || count == 0)
    return;
  if (count == 1) {
    Enqueue(commands[0]);
    return;
  }
  if (t_recording != nullptr) {
    for (size_t i = 0; i < count; ++i)
      Enqueue(commands[i]);
    return;
  }
  if (DispatchesInline()) {
    DispatchRenderCommands(commands, count);
    return;
  }
  // One lock for the whole draw-state batch; it stays contiguous in the FIFO.
  Push(commands, count);
}

void RenderQueue::BeginRecording() {
  t_pendingRecording.reset();
  t_recording = std::make_shared<RecordedRenderBatch>();
}

void RenderQueue::EndRecording() {
  if (t_recording == nullptr)
    return;
  t_pendingRecording = std::move(t_recording);
}

bool RenderQueue::IsRecording() {
  return t_recording != nullptr;
}

size_t RenderQueue::AssociatePendingTextureFixup(uint32_t handle, uint32_t guestAddress) {
  if (t_pendingRecording == nullptr)
    return 0;
  return t_pendingRecording->AssociateTextureFixup(handle, guestAddress);
}

void RenderQueue::BindPendingRecording(uint32_t cloneAddress) {
  std::shared_ptr<RecordedRenderBatch> recording = std::move(t_pendingRecording);
  if (cloneAddress == 0 || recording == nullptr || recording->empty())
    return;
  const size_t commandCount = recording->size();
  size_t textures = 0;
  size_t textureBases = 0;
  size_t booleans = 0;
  size_t pixelShaders = 0;
  size_t draws = 0;
  uint32_t psBoolean0Or = 0;
  for (const RenderCommand& command : recording->commands()) {
    switch (command.type) {
      case RenderCommandType::SetTexture:
        ++textures;
        break;
      case RenderCommandType::SetTextureBase:
        ++textureBases;
        break;
      case RenderCommandType::SetBooleans:
        ++booleans;
        psBoolean0Or |= command.setBooleans.words[4];
        break;
      case RenderCommandType::SetPixelShader:
        ++pixelShaders;
        break;
      case RenderCommandType::DrawPrimitive:
      case RenderCommandType::DrawIndexedPrimitive:
      case RenderCommandType::DrawPrimitiveUP:
        ++draws;
        break;
      default:
        break;
    }
  }
  std::lock_guard lock(g_recordingsMutex);
  g_recordings[cloneAddress] = std::move(recording);
  static std::atomic<uint32_t> bindCount{0};
  const uint32_t count = bindCount.fetch_add(1, std::memory_order_relaxed) + 1;
  if (count <= 256) {
    REXGPU_INFO(
        "Deferred D3D command buffer recorded: n={} clone=0x{:08X} commands={} draws={} "
        "textures={}/{} booleans={} ps={} psBool0Or=0x{:08X}",
        count, cloneAddress, commandCount, draws, textures, textureBases, booleans, pixelShaders,
        psBoolean0Or);
  }
}

bool RenderQueue::SetRecordingTextureFixup(uint32_t cloneAddress, uint32_t handle,
                                           const RenderCommand& replacement) {
  std::lock_guard lock(g_recordingsMutex);
  const auto it = g_recordings.find(cloneAddress);
  return it != g_recordings.end() && it->second->SetTextureFixup(handle, replacement);
}

bool RenderQueue::ReplayRecording(uint32_t cloneAddress,
                                  const DeferredExecutionSnapshot* executionSnapshot) {
  auto job = std::make_unique<ReplayJob>();
  {
    std::lock_guard lock(g_recordingsMutex);
    const auto it = g_recordings.find(cloneAddress);
    if (it == g_recordings.end())
      return false;
    job->recording = it->second;
    job->commands = it->second->BuildReplayCommands();
  }
  if (executionSnapshot != nullptr) {
    job->executionSnapshot = std::make_unique<DeferredExecutionSnapshot>(*executionSnapshot);
  }
  if (DispatchesInline()) {
    ExecuteReplay(*job);
    return true;
  }
  RenderCommand cmd{};
  cmd.type = RenderCommandType::ReplayRecording;
  cmd.replayRecording.job = job.release();
  Push(&cmd, 1);
  return true;
}

}  // namespace pgr4::render
