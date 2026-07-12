// render/render_queue.h
//
// Dedicated render thread (Unleashed pattern, slim): guest hooks enqueue work;
// only the render thread records/submits Plume command lists.

#pragma once

#include <functional>

#include "render/render_commands.h"

namespace fm2::render {

struct RenderQueue {
  // Start/stop the render thread. Safe to call once around Video::Init/Shutdown.
  static void Start();
  static void Stop();

  // Run `fn` on the render thread and block until it finishes. Nested calls
  // from the render thread itself execute inline (no deadlock). Use for
  // Present / WaitForGPU / resource create / draws that must complete before
  // the guest continues.
  static void Run(std::function<void()> fn);

  // Fire-and-forget: enqueue `fn` and return immediately. Preserves FIFO order
  // with Run(). Nested calls from the render thread execute inline.
  // Prefer Enqueue(RenderCommand) for state setters.
  static void Enqueue(std::function<void()> fn);

  // Fire-and-forget POD command (Unleashed RenderCommand path).
  static void Enqueue(const RenderCommand& cmd);

  // True when called on the dedicated render thread.
  static bool IsOnRenderThread();
};

}  // namespace fm2::render
