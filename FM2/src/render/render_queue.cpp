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
  std::function<void()> fn;
  std::atomic<bool>* done = nullptr;
};

std::mutex g_mutex;
std::condition_variable g_cv;
std::deque<Job> g_jobs;
std::thread g_thread;
std::atomic<bool> g_running{false};
std::thread::id g_renderThreadId{};

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
    if (job.fn) job.fn();
    if (job.done != nullptr) {
      job.done->store(true, std::memory_order_release);
      job.done->notify_one();
    }
  }
  REXGPU_INFO("FM2 render queue: render thread stopped");
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

void RenderQueue::Run(std::function<void()> fn) {
  if (!fn) return;
  if (!g_running.load(std::memory_order_acquire)) {
    // Init/Shutdown edge: run inline if the queue is not up yet.
    fn();
    return;
  }
  if (IsOnRenderThread()) {
    fn();
    return;
  }

  std::atomic<bool> done{false};
  {
    std::lock_guard lock(g_mutex);
    g_jobs.push_back(Job{std::move(fn), &done});
  }
  g_cv.notify_one();
  done.wait(false, std::memory_order_acquire);
}

void RenderQueue::Enqueue(std::function<void()> fn) {
  if (!fn) return;
  if (!g_running.load(std::memory_order_acquire)) {
    fn();
    return;
  }
  if (IsOnRenderThread()) {
    fn();
    return;
  }

  {
    std::lock_guard lock(g_mutex);
    g_jobs.push_back(Job{std::move(fn), nullptr});
  }
  g_cv.notify_one();
}

}  // namespace fm2::render
