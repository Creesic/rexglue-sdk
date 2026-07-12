// render/render_queue.cpp

#include "render/render_queue.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

#include <rex/logging.h>

namespace fm2::render {
namespace {

struct Job {
  RenderCommand cmd{};
  std::atomic<bool>* done = nullptr;
};

std::mutex g_mutex;
std::condition_variable g_cv;
std::deque<Job> g_jobs;
std::thread g_thread;
std::atomic<bool> g_running{false};
std::thread::id g_renderThreadId{};

void ExecuteJob(Job& job) {
  DispatchRenderCommand(job.cmd);
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
        if (!g_running.load(std::memory_order_acquire)) break;
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
  if (!g_running.compare_exchange_strong(expected, true)) return;
  g_thread = std::thread(ThreadMain);
}

void RenderQueue::Stop() {
  {
    std::lock_guard lock(g_mutex);
    if (!g_running.load(std::memory_order_acquire)) return;
    g_running.store(false, std::memory_order_release);
  }
  g_cv.notify_all();
  if (g_thread.joinable()) g_thread.join();
  std::lock_guard lock(g_mutex);
  g_jobs.clear();
}

bool RenderQueue::IsOnRenderThread() {
  return std::this_thread::get_id() == g_renderThreadId;
}

void RenderQueue::Run(const RenderCommand& cmd) {
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

}  // namespace fm2::render
