// render/render_queue.cpp

#include "render/render_queue.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <rex/logging.h>

namespace fm2::render {
namespace {

struct Job {
  RenderCommand cmd{};
  std::atomic<bool>* done = nullptr;
  std::shared_ptr<const RecordedRenderBatch> replayBatch;
  std::vector<RenderCommand> replayCommands;
  DeferredExecutionSnapshot executionSnapshot{};
  bool hasExecutionSnapshot = false;
};

std::mutex g_mutex;
std::condition_variable g_cv;
std::deque<Job> g_jobs;
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

void ExecuteJob(Job& job) {
  if (job.replayBatch != nullptr) {
    DispatchRecordedRenderCommands(job.replayCommands.data(), job.replayCommands.size(),
                                   job.hasExecutionSnapshot ? &job.executionSnapshot : nullptr);
  } else {
    DispatchRenderCommand(job.cmd);
  }
  if (job.done != nullptr) {
    job.done->store(true, std::memory_order_release);
    job.done->notify_one();
  }
}

void ThreadMain() {
  g_renderThreadId = std::this_thread::get_id();
  REXGPU_INFO("FM2 render queue: render thread started");
  while (true) {
    Job job;
    {
      std::unique_lock lock(g_mutex);
      g_cv.wait(lock, [] { return !g_jobs.empty() || !g_running.load(std::memory_order_acquire); });
      if (g_jobs.empty()) {
        if (!g_running.load(std::memory_order_acquire))
          break;
        continue;
      }
      job = std::move(g_jobs.front());
      g_jobs.pop_front();
    }
    ExecuteJob(job);
  }
  REXGPU_INFO("FM2 render queue: render thread stopped");
}

void PushJob(Job job) {
  {
    std::lock_guard lock(g_mutex);
    g_jobs.push_back(std::move(job));
  }
  g_cv.notify_one();
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
    g_jobs.clear();
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
  if (!g_running.load(std::memory_order_acquire)) {
    DispatchRenderCommand(cmd);
    return;
  }
  if (IsOnRenderThread()) {
    DispatchRenderCommand(cmd);
    return;
  }

  std::atomic<bool> done{false};
  PushJob(Job{cmd, &done});
  done.wait(false, std::memory_order_acquire);
}

void RenderQueue::Enqueue(const RenderCommand& cmd) {
  if (TryRecord(cmd))
    return;
  if (!g_running.load(std::memory_order_acquire)) {
    DispatchRenderCommand(cmd);
    return;
  }
  if (IsOnRenderThread()) {
    DispatchRenderCommand(cmd);
    return;
  }

  PushJob(Job{cmd, nullptr});
}

void RenderQueue::EnqueueBulk(const RenderCommand* commands, size_t count) {
  if (commands == nullptr || count == 0)
    return;
  if (t_recording != nullptr) {
    for (size_t i = 0; i < count; ++i)
      Enqueue(commands[i]);
    return;
  }
  if (!g_running.load(std::memory_order_acquire) || IsOnRenderThread()) {
    for (size_t i = 0; i < count; ++i)
      DispatchRenderCommand(commands[i]);
    return;
  }

  {
    std::lock_guard lock(g_mutex);
    for (size_t i = 0; i < count; ++i)
      g_jobs.push_back(Job{commands[i], nullptr});
  }
  g_cv.notify_one();
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
  std::shared_ptr<RecordedRenderBatch> recording;
  std::vector<RenderCommand> commands;
  {
    std::lock_guard lock(g_recordingsMutex);
    const auto it = g_recordings.find(cloneAddress);
    if (it == g_recordings.end())
      return false;
    recording = it->second;
    commands = recording->BuildReplayCommands();
  }

  Job job{};
  job.replayBatch = std::move(recording);
  job.replayCommands = std::move(commands);
  if (executionSnapshot != nullptr) {
    job.executionSnapshot = *executionSnapshot;
    job.hasExecutionSnapshot = true;
  }
  if (!g_running.load(std::memory_order_acquire) || IsOnRenderThread()) {
    ExecuteJob(job);
  } else {
    PushJob(std::move(job));
  }
  return true;
}

}  // namespace fm2::render
