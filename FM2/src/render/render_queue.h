// render/render_queue.h
//
// Dedicated render thread (Unleashed pattern, slim): guest hooks enqueue work;
// only the render thread records/submits Plume command lists.

#pragma once

#include <cstddef>

#include "render/render_commands.h"

namespace fm2::render {

struct RenderQueue {
  // Start/stop the render thread. Safe to call once around Video::Init/Shutdown.
  static void Start();
  static void Stop();

  // Sync POD: enqueue `cmd`, block until Dispatch finishes (Unleashed
  // ExecuteCommandList wait pattern for Present/creates/WaitForGPU).
  static void Run(const RenderCommand& cmd);

  // Fire-and-forget POD command (Unleashed RenderCommand path).
  static void Enqueue(const RenderCommand& cmd);

  // Atomically append an ordered command batch. Draw-state snapshots and the
  // draw that consumes them must not be interleaved with another guest
  // thread's batch.
  static void EnqueueBulk(const RenderCommand* commands, size_t count);

  // True when called on the dedicated render thread.
  static bool IsOnRenderThread();
};

}  // namespace fm2::render
