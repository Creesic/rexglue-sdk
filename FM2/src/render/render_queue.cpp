// render/render_queue.cpp

#include "render/render_queue.h"

#include <atomic>
#include <chrono>
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
  std::vector<uint8_t> replayShaderConstantPayload;
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
thread_local uint32_t t_recordingSource = 0;
thread_local uint32_t t_pendingRecordingSource = 0;

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
  using Clock = std::chrono::steady_clock;
  constexpr size_t replayIndex = size_t(RenderCommandType::ApplyPixelShaderConstantFixup) + 1;
  std::array<double, replayIndex + 1> milliseconds{};
  auto intervalStart = Clock::now();
  uint32_t jobs = 0, submits = 0;
  while (true) {
    Job job;
    size_t queued = 0;
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
      queued = g_jobs.size();
    }
    const auto start = Clock::now();
    ExecuteJob(job);
    const auto end = Clock::now();
    const size_t kind = job.replayBatch ? replayIndex : size_t(job.cmd.type);
    milliseconds[kind] += std::chrono::duration<double, std::milli>(end - start).count();
    ++jobs;
    submits += !job.replayBatch && job.cmd.type == RenderCommandType::ExecuteCommandList;
    const double wall = std::chrono::duration<double, std::milli>(end - intervalStart).count();
    if (wall >= 5000.0) {
      double busy = 0;
      size_t top = 0;
      for (size_t i = 0; i < milliseconds.size(); ++i) {
        busy += milliseconds[i];
        if (milliseconds[i] > milliseconds[top])
          top = i;
      }
      // Handler wall time includes waits inside it; this is not GPU timing.
      REXGPU_INFO("RenderTiming: wallMs={:.1f} handlerMs={:.1f} replayMs={:.1f} "
                  "topKind={} topMs={:.1f} jobs={} submits={} queued={} (replayKind={})",
                  wall, busy, milliseconds[replayIndex], top, milliseconds[top],
                  jobs, submits, queued, replayIndex);
      milliseconds.fill(0);
      jobs = submits = 0;
      intervalStart = end;
    }
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

void RenderQueue::BeginRecording(uint32_t sourceAddress) {
  t_pendingRecording.reset();
  t_recordingSource = sourceAddress;
  t_recording = std::make_shared<RecordedRenderBatch>();
  std::lock_guard lock(g_recordingsMutex);
  g_recordings.erase(sourceAddress);
}

void RenderQueue::EndRecording() {
  if (t_recording == nullptr)
    return;
  t_pendingRecording = std::move(t_recording);
  t_pendingRecordingSource = t_recordingSource;
  // Finalized buffers can be submitted directly, without CreateCommandBufferClone.
  // Keep the same pending object so subsequent source fixups remain visible.
  std::lock_guard lock(g_recordingsMutex);
  if (t_pendingRecordingSource != 0)
    g_recordings[t_pendingRecordingSource] = t_pendingRecording;
}

bool RenderQueue::IsRecording() {
  return t_recording != nullptr;
}

void RenderQueue::RecordPendingCommandBufferMarker(uint32_t marker) {
  if (t_recording != nullptr)
    t_recording->RecordMarker(marker);
}

uint32_t RenderQueue::RegisterRecordingTextureFixup(uint32_t sourceAddress, uint32_t handle,
                                                    uint32_t guestAddress, uint32_t startMarker,
                                                    uint32_t stopMarker) {
  std::lock_guard lock(g_recordingsMutex);
  const auto it = g_recordings.find(sourceAddress);
  return it == g_recordings.end()
             ? handle
             : it->second->RegisterTextureFixup(handle, guestAddress, startMarker, stopMarker);
}

size_t RenderQueue::AssociatePendingShaderConstantFixup(uint32_t handle, bool pixelShader,
                                                        uint32_t startRegister,
                                                        uint32_t registerCount,
                                                        uint32_t startMarker, uint32_t stopMarker) {
  if (t_pendingRecording == nullptr)
    return 0;
  std::lock_guard lock(g_recordingsMutex);
  return t_pendingRecording->AssociateShaderConstantFixup(handle, pixelShader, startRegister,
                                                          registerCount, startMarker, stopMarker);
}

void RenderQueue::BindPendingRecording(uint32_t cloneAddress, uint32_t sourceAddress,
                                        uint32_t caller) {
  std::shared_ptr<RecordedRenderBatch> recording;
  {
    std::lock_guard lock(g_recordingsMutex);
    if (sourceAddress != 0 && t_pendingRecording != nullptr &&
        sourceAddress == t_pendingRecordingSource) {
      g_recordings[sourceAddress] = std::move(t_pendingRecording);
    }
    const auto source = g_recordings.find(sourceAddress);
    if (cloneAddress != 0 && source != g_recordings.end()) {
      recording = std::make_shared<RecordedRenderBatch>(*source->second);
      g_recordings[cloneAddress] = recording;
    }
  }
  if (cloneAddress == 0 || recording == nullptr || recording->empty()) {
    static std::atomic<uint32_t> missingRecordings{0};
    const uint32_t n = ++missingRecordings;
    if (n <= 64 || n % 4096 == 0) {
      REXGPU_WARN("DeferredCloneUnrecorded: n={} lr=0x{:08X} source=0x{:08X} "
                  "clone=0x{:08X} pending={} commands={}",
                  n, caller, sourceAddress, cloneAddress, recording != nullptr,
                  recording != nullptr ? recording->size() : 0);
    }
    return;
  }
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
  static std::atomic<uint32_t> bindCount{0};
  const uint32_t count = bindCount.fetch_add(1, std::memory_order_relaxed) + 1;
  if (count <= 256) {
    REXGPU_INFO(
        "Deferred D3D command buffer recorded: n={} clone=0x{:08X} commands={} draws={} "
        "textures={}/{} booleans={} ps={} psBool0Or=0x{:08X} source=0x{:08X} lr=0x{:08X}",
        count, cloneAddress, commandCount, draws, textures, textureBases, booleans, pixelShaders,
        psBoolean0Or, sourceAddress, caller);
  }
}

bool RenderQueue::SetRecordingTextureFixup(uint32_t cloneAddress, uint32_t handle,
                                           const RenderCommand& replacement) {
  std::lock_guard lock(g_recordingsMutex);
  const auto it = g_recordings.find(cloneAddress);
  return it != g_recordings.end() && it->second->SetTextureFixup(handle, replacement);
}

bool RenderQueue::SetRecordingShaderConstantFixup(uint32_t cloneAddress, uint32_t handle,
                                                  const uint32_t* source) {
  std::lock_guard lock(g_recordingsMutex);
  const auto it = g_recordings.find(cloneAddress);
  return it != g_recordings.end() && it->second->SetShaderConstantFixup(handle, source);
}

bool RenderQueue::ReplayRecording(uint32_t cloneAddress,
                                  const DeferredExecutionSnapshot* executionSnapshot) {
  Job job{};
  std::shared_ptr<RecordedRenderBatch> recording;
  {
    std::lock_guard lock(g_recordingsMutex);
    const auto it = g_recordings.find(cloneAddress);
    if (it == g_recordings.end())
      return false;
    recording = it->second;
    job.replayCommands = recording->BuildReplayCommands(job.replayShaderConstantPayload);
  }

  job.replayBatch = std::move(recording);
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
