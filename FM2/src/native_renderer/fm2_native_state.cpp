#include "native_renderer/fm2_native_state.h"

namespace fm2::native_renderer {
namespace {

NativeStateRecorder g_native_state_recorder;

}

NativeStateSnapshot& NativeStateRecorder::GetOrCreateContext(
    uint32_t render_context) {
  auto [it, inserted] = contexts_.try_emplace(render_context);
  NativeStateSnapshot& snapshot = it->second;
  if (inserted) {
    snapshot.valid = true;
    snapshot.render_context = render_context;
  }
  return snapshot;
}

uint64_t NativeStateRecorder::NextSequence() { return ++sequence_; }

void NativeStateRecorder::Reset() {
  std::scoped_lock lock(mutex_);
  sequence_ = 0;
  last_render_context_ = 0;
  last_pipeline_render_context_ = 0;
  last_pass_ = {};
  contexts_.clear();
}

void NativeStateRecorder::RecordVertexShaderState(
    const NativeStateShaderBinding& binding) {
  if (binding.render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot = GetOrCreateContext(binding.render_context);
  snapshot.vertex_shader = binding;
  snapshot.vertex_shader.valid = true;
  snapshot.vertex_shader.sequence = NextSequence();
  snapshot.sequence = snapshot.vertex_shader.sequence;
  last_render_context_ = binding.render_context;
  last_pipeline_render_context_ = binding.render_context;
}

void NativeStateRecorder::RecordPixelShaderState(
    const NativeStateShaderBinding& binding) {
  if (binding.render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot = GetOrCreateContext(binding.render_context);
  snapshot.pixel_shader = binding;
  snapshot.pixel_shader.valid = true;
  snapshot.pixel_shader.sequence = NextSequence();
  snapshot.sequence = snapshot.pixel_shader.sequence;
  last_render_context_ = binding.render_context;
  last_pipeline_render_context_ = binding.render_context;
}

void NativeStateRecorder::RecordVertexStreamBinding(
    const NativeStateVertexStreamBinding& binding) {
  if (binding.render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot = GetOrCreateContext(binding.render_context);
  if (binding.slot >= kNativeStateMaxVertexStreams) {
    last_render_context_ = binding.render_context;
    last_pipeline_render_context_ = binding.render_context;
    return;
  }
  NativeStateVertexStreamBinding& stream = snapshot.streams[binding.slot];
  stream = binding;
  stream.valid = true;
  stream.sequence = NextSequence();
  snapshot.sequence = stream.sequence;
  last_render_context_ = binding.render_context;
  last_pipeline_render_context_ = binding.render_context;
}

void NativeStateRecorder::RecordIndexBufferBinding(
    const NativeStateIndexBufferBinding& binding) {
  if (binding.render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot = GetOrCreateContext(binding.render_context);
  snapshot.index_buffer = binding;
  snapshot.index_buffer.valid = true;
  snapshot.index_buffer.sequence = NextSequence();
  snapshot.sequence = snapshot.index_buffer.sequence;
  last_render_context_ = binding.render_context;
  last_pipeline_render_context_ = binding.render_context;
}

void NativeStateRecorder::RecordBoundSurface(
    const NativeStateSurfaceBinding& binding) {
  if (binding.render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot = GetOrCreateContext(binding.render_context);
  snapshot.bound_surface = binding;
  snapshot.bound_surface.valid = true;
  snapshot.bound_surface.sequence = NextSequence();
  snapshot.sequence = snapshot.bound_surface.sequence;
  last_render_context_ = binding.render_context;
  last_pipeline_render_context_ = binding.render_context;
}

void NativeStateRecorder::RecordViewportMode(
    const NativeStateViewportBinding& binding) {
  if (binding.render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot = GetOrCreateContext(binding.render_context);
  snapshot.viewport = binding;
  snapshot.viewport.valid = true;
  snapshot.viewport.sequence = NextSequence();
  snapshot.sequence = snapshot.viewport.sequence;
  last_render_context_ = binding.render_context;
  last_pipeline_render_context_ = binding.render_context;
}

void NativeStateRecorder::RecordTextureFetchLow(uint32_t render_context,
                                                uint8_t fetch_bits_low) {
  if (render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot = GetOrCreateContext(render_context);
  snapshot.texture_fetch.render_context = render_context;
  snapshot.texture_fetch.fetch_bits_low = fetch_bits_low;
  snapshot.texture_fetch.valid = true;
  snapshot.texture_fetch.sequence = NextSequence();
  snapshot.sequence = snapshot.texture_fetch.sequence;
  last_render_context_ = render_context;
  last_pipeline_render_context_ = render_context;
}

void NativeStateRecorder::RecordTextureFetchMid(uint32_t render_context,
                                                uint8_t fetch_bits_mid) {
  if (render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot = GetOrCreateContext(render_context);
  snapshot.texture_fetch.render_context = render_context;
  snapshot.texture_fetch.fetch_bits_mid = fetch_bits_mid;
  snapshot.texture_fetch.valid = true;
  snapshot.texture_fetch.sequence = NextSequence();
  snapshot.sequence = snapshot.texture_fetch.sequence;
  last_render_context_ = render_context;
  last_pipeline_render_context_ = render_context;
}

void NativeStateRecorder::RecordClearColor(uint32_t render_context,
                                           uint8_t clear_color_byte) {
  if (render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot = GetOrCreateContext(render_context);
  snapshot.clear.render_context = render_context;
  snapshot.clear.clear_color_byte = clear_color_byte;
  snapshot.clear.valid = true;
  snapshot.clear.sequence = NextSequence();
  snapshot.sequence = snapshot.clear.sequence;
  last_render_context_ = render_context;
  last_pipeline_render_context_ = render_context;
}

void NativeStateRecorder::RecordClearFlags(uint32_t render_context,
                                           uint8_t clear_flags) {
  if (render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot = GetOrCreateContext(render_context);
  snapshot.clear.render_context = render_context;
  snapshot.clear.clear_flags = clear_flags;
  snapshot.clear.valid = true;
  snapshot.clear.sequence = NextSequence();
  snapshot.sequence = snapshot.clear.sequence;
  last_render_context_ = render_context;
  last_pipeline_render_context_ = render_context;
}

void NativeStateRecorder::RecordPassDrawBoundary(
    const NativeStatePassDrawBoundary& boundary) {
  std::scoped_lock lock(mutex_);
  last_pass_ = boundary;
  last_pass_.valid = true;
  last_pass_.sequence = NextSequence();
  const uint32_t render_context =
      last_pipeline_render_context_ != 0 ? last_pipeline_render_context_
                                         : last_render_context_;
  if (render_context == 0) {
    return;
  }
  NativeStateSnapshot& snapshot = GetOrCreateContext(render_context);
  snapshot.last_pass = last_pass_;
  snapshot.sequence = last_pass_.sequence;
  last_render_context_ = render_context;
}

void NativeStateRecorder::RecordDirectDrawEntry(
    const NativeStateDirectDrawEntry& entry) {
  if (entry.direct_render_context == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  NativeStateSnapshot& snapshot =
      GetOrCreateContext(entry.direct_render_context);
  NativeStateDirectDrawEntry recorded_entry = entry;
  recorded_entry.valid = true;
  recorded_entry.sequence = NextSequence();
  snapshot.last_direct_draw = recorded_entry;
  snapshot.sequence = recorded_entry.sequence;
  if (last_pass_.valid && !snapshot.last_pass.valid) {
    snapshot.last_pass = last_pass_;
  }
  if (last_pipeline_render_context_ != 0 &&
      last_pipeline_render_context_ != entry.direct_render_context) {
    NativeStateSnapshot& pipeline_snapshot =
        GetOrCreateContext(last_pipeline_render_context_);
    pipeline_snapshot.last_direct_draw = recorded_entry;
    pipeline_snapshot.sequence = recorded_entry.sequence;
    if (last_pass_.valid && !pipeline_snapshot.last_pass.valid) {
      pipeline_snapshot.last_pass = last_pass_;
    }
    last_render_context_ = last_pipeline_render_context_;
    return;
  }
  last_render_context_ = entry.direct_render_context;
}

NativeStateSnapshot NativeStateRecorder::Snapshot(uint32_t render_context) const {
  std::scoped_lock lock(mutex_);
  const auto it = contexts_.find(render_context);
  if (it == contexts_.end()) {
    return {};
  }
  NativeStateSnapshot snapshot = it->second;
  if (last_pass_.valid && !snapshot.last_pass.valid) {
    snapshot.last_pass = last_pass_;
  }
  return snapshot;
}

NativeStateSnapshot NativeStateRecorder::LastSnapshot() const {
  std::scoped_lock lock(mutex_);
  if (last_render_context_ == 0) {
    return {};
  }
  const auto it = contexts_.find(last_render_context_);
  if (it == contexts_.end()) {
    return {};
  }
  NativeStateSnapshot snapshot = it->second;
  if (last_pass_.valid && !snapshot.last_pass.valid) {
    snapshot.last_pass = last_pass_;
  }
  return snapshot;
}

NativeStateSnapshot NativeStateRecorder::SnapshotForDirectDraw(
    uint32_t direct_render_context) const {
  std::scoped_lock lock(mutex_);
  if (last_pipeline_render_context_ != 0) {
    const auto pipeline_it = contexts_.find(last_pipeline_render_context_);
    if (pipeline_it != contexts_.end()) {
      NativeStateSnapshot snapshot = pipeline_it->second;
      if (snapshot.last_direct_draw.valid &&
          snapshot.last_direct_draw.direct_render_context ==
              direct_render_context) {
        if (last_pass_.valid && !snapshot.last_pass.valid) {
          snapshot.last_pass = last_pass_;
        }
        return snapshot;
      }
    }
  }
  const auto direct_it = contexts_.find(direct_render_context);
  if (direct_it == contexts_.end()) {
    return {};
  }
  NativeStateSnapshot snapshot = direct_it->second;
  if (last_pass_.valid && !snapshot.last_pass.valid) {
    snapshot.last_pass = last_pass_;
  }
  return snapshot;
}

void ResetNativeStateRecorder() { g_native_state_recorder.Reset(); }

void RecordNativeVertexShaderState(const NativeStateShaderBinding& binding) {
  g_native_state_recorder.RecordVertexShaderState(binding);
}

void RecordNativePixelShaderState(const NativeStateShaderBinding& binding) {
  g_native_state_recorder.RecordPixelShaderState(binding);
}

void RecordNativeVertexStreamBinding(
    const NativeStateVertexStreamBinding& binding) {
  g_native_state_recorder.RecordVertexStreamBinding(binding);
}

void RecordNativeIndexBufferBinding(
    const NativeStateIndexBufferBinding& binding) {
  g_native_state_recorder.RecordIndexBufferBinding(binding);
}

void RecordNativeBoundSurface(const NativeStateSurfaceBinding& binding) {
  g_native_state_recorder.RecordBoundSurface(binding);
}

void RecordNativePassDrawBoundary(const NativeStatePassDrawBoundary& boundary) {
  g_native_state_recorder.RecordPassDrawBoundary(boundary);
}

void RecordNativeDirectDrawEntry(const NativeStateDirectDrawEntry& entry) {
  g_native_state_recorder.RecordDirectDrawEntry(entry);
}

void RecordNativeVertexShaderStateArgs(uint32_t render_context,
                                       uint32_t shader) {
  RecordNativeVertexShaderState(
      {.render_context = render_context, .shader = shader});
}

void RecordNativePixelShaderStateArgs(uint32_t render_context,
                                      uint32_t shader) {
  RecordNativePixelShaderState(
      {.render_context = render_context, .shader = shader});
}

void RecordNativeVertexStreamBindingArgs(uint32_t render_context,
                                         uint32_t slot, uint32_t resource,
                                         uint32_t byte_offset,
                                         uint32_t stride_bytes,
                                         uint32_t dirty_mask) {
  RecordNativeVertexStreamBinding({.render_context = render_context,
                                   .slot = slot,
                                   .resource = resource,
                                   .byte_offset = byte_offset,
                                   .stride_bytes = stride_bytes,
                                   .dirty_mask = dirty_mask});
}

void RecordNativeIndexBufferBindingArgs(uint32_t render_context,
                                        uint32_t resource) {
  RecordNativeIndexBufferBinding(
      {.render_context = render_context, .resource = resource});
}

void RecordNativeBoundSurfaceArgs(uint32_t render_context, uint32_t surface,
                                  uint32_t surface_arg) {
  RecordNativeBoundSurface({.render_context = render_context,
                            .surface = surface,
                            .surface_arg = surface_arg});
}

void RecordNativeViewportModeArgs(uint32_t render_context,
                                  uint32_t viewport_mode) {
  g_native_state_recorder.RecordViewportMode({.render_context = render_context,
                                              .viewport_mode = viewport_mode});
}

void RecordNativeTextureFetchLowArgs(uint32_t render_context,
                                     uint32_t fetch_bits_low) {
  g_native_state_recorder.RecordTextureFetchLow(
      render_context, static_cast<uint8_t>(fetch_bits_low & 0xFFu));
}

void RecordNativeTextureFetchMidArgs(uint32_t render_context,
                                     uint32_t fetch_bits_mid) {
  g_native_state_recorder.RecordTextureFetchMid(
      render_context, static_cast<uint8_t>(fetch_bits_mid & 0xFFu));
}

void RecordNativeClearColorArgs(uint32_t render_context,
                              uint32_t clear_color_byte) {
  g_native_state_recorder.RecordClearColor(
      render_context, static_cast<uint8_t>(clear_color_byte & 0xFFu));
}

void RecordNativeClearFlagsArgs(uint32_t render_context, uint32_t clear_flags) {
  g_native_state_recorder.RecordClearFlags(
      render_context, static_cast<uint8_t>(clear_flags & 0xFFu));
}

void RecordNativePassDrawBoundaryArgs(uint32_t submit_object,
                                      uint32_t tls_or_pass_context,
                                      uint32_t pass_flags, uint32_t drawable,
                                      uint32_t draw_callback, uint32_t wireframe,
                                      uint32_t draw_mode, uint32_t pass_marker) {
  RecordNativePassDrawBoundary({
      .submit_object = submit_object,
      .tls_or_pass_context = tls_or_pass_context,
      .pass_flags = pass_flags,
      .drawable = drawable,
      .draw_callback = draw_callback,
      .wireframe = wireframe,
      .draw_mode = draw_mode,
      .pass_marker = pass_marker,
  });
}

NativeStateSnapshot SnapshotNativeState(uint32_t render_context) {
  return g_native_state_recorder.Snapshot(render_context);
}

NativeStateSnapshot SnapshotLastNativeState() {
  return g_native_state_recorder.LastSnapshot();
}

NativeStateSnapshot SnapshotNativeStateForDirectDraw(
    uint32_t direct_render_context) {
  return g_native_state_recorder.SnapshotForDirectDraw(direct_render_context);
}

}
