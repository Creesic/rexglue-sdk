#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace fm2::native_renderer {

inline constexpr uint32_t kNativeStateMaxVertexStreams = 16u;

struct NativeStateShaderBinding {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t render_context = 0;
  uint32_t shader = 0;
};

struct NativeStateVertexStreamBinding {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t render_context = 0;
  uint32_t slot = 0;
  uint32_t resource = 0;
  uint32_t byte_offset = 0;
  uint32_t stride_bytes = 0;
  uint32_t dirty_mask = 0;
};

struct NativeStateIndexBufferBinding {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t render_context = 0;
  uint32_t resource = 0;
};

struct NativeStateSurfaceBinding {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t render_context = 0;
  uint32_t surface = 0;
  uint32_t surface_arg = 0;
};

struct NativeStateViewportBinding {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t render_context = 0;
  uint32_t viewport_mode = 0;
};

struct NativeStateTextureFetchBinding {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t render_context = 0;
  uint8_t fetch_bits_low = 0;
  uint8_t fetch_bits_mid = 0;
};

struct NativeStateClearBinding {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t render_context = 0;
  uint8_t clear_color_byte = 0;
  uint8_t clear_flags = 0;
};

struct NativeStatePassDrawBoundary {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t submit_object = 0;
  uint32_t tls_or_pass_context = 0;
  uint32_t pass_flags = 0;
  uint32_t drawable = 0;
  uint32_t draw_callback = 0;
  uint32_t wireframe = 0;
  uint32_t draw_mode = 0;
  uint32_t pass_marker = 0;
};

struct NativeStateDirectDrawEntry {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t direct_render_context = 0;
  uint32_t draw_iface = 0;
};

struct NativeStateSnapshot {
  bool valid = false;
  uint64_t sequence = 0;
  uint32_t render_context = 0;
  NativeStateShaderBinding vertex_shader;
  NativeStateShaderBinding pixel_shader;
  std::array<NativeStateVertexStreamBinding, kNativeStateMaxVertexStreams>
      streams = {};
  NativeStateIndexBufferBinding index_buffer;
  NativeStateSurfaceBinding bound_surface;
  NativeStateViewportBinding viewport;
  NativeStateTextureFetchBinding texture_fetch;
  NativeStateClearBinding clear;
  NativeStatePassDrawBoundary last_pass;
  NativeStateDirectDrawEntry last_direct_draw;
};

class NativeStateRecorder {
 public:
  NativeStateRecorder() = default;
  NativeStateRecorder(const NativeStateRecorder&) = delete;
  NativeStateRecorder& operator=(const NativeStateRecorder&) = delete;

  void Reset();
  void RecordVertexShaderState(const NativeStateShaderBinding& binding);
  void RecordPixelShaderState(const NativeStateShaderBinding& binding);
  void RecordVertexStreamBinding(
      const NativeStateVertexStreamBinding& binding);
  void RecordIndexBufferBinding(const NativeStateIndexBufferBinding& binding);
  void RecordBoundSurface(const NativeStateSurfaceBinding& binding);
  void RecordViewportMode(const NativeStateViewportBinding& binding);
  void RecordTextureFetchLow(uint32_t render_context, uint8_t fetch_bits_low);
  void RecordTextureFetchMid(uint32_t render_context, uint8_t fetch_bits_mid);
  void RecordClearColor(uint32_t render_context, uint8_t clear_color_byte);
  void RecordClearFlags(uint32_t render_context, uint8_t clear_flags);
  void RecordPassDrawBoundary(const NativeStatePassDrawBoundary& boundary);
  void RecordDirectDrawEntry(const NativeStateDirectDrawEntry& entry);

  NativeStateSnapshot Snapshot(uint32_t render_context) const;
  NativeStateSnapshot LastSnapshot() const;
  NativeStateSnapshot SnapshotForDirectDraw(uint32_t direct_render_context) const;

 private:
  NativeStateSnapshot& GetOrCreateContext(uint32_t render_context);
  uint64_t NextSequence();

  mutable std::mutex mutex_;
  uint64_t sequence_ = 0;
  uint32_t last_render_context_ = 0;
  uint32_t last_pipeline_render_context_ = 0;
  NativeStatePassDrawBoundary last_pass_;
  std::unordered_map<uint32_t, NativeStateSnapshot> contexts_;
};

void ResetNativeStateRecorder();
void RecordNativeVertexShaderState(const NativeStateShaderBinding& binding);
void RecordNativePixelShaderState(const NativeStateShaderBinding& binding);
void RecordNativeVertexStreamBinding(
    const NativeStateVertexStreamBinding& binding);
void RecordNativeIndexBufferBinding(
    const NativeStateIndexBufferBinding& binding);
void RecordNativeBoundSurface(const NativeStateSurfaceBinding& binding);
void RecordNativeViewportModeArgs(uint32_t render_context,
                                  uint32_t viewport_mode);
void RecordNativeTextureFetchLowArgs(uint32_t render_context,
                                     uint32_t fetch_bits_low);
void RecordNativeTextureFetchMidArgs(uint32_t render_context,
                                     uint32_t fetch_bits_mid);
void RecordNativeClearColorArgs(uint32_t render_context,
                                uint32_t clear_color_byte);
void RecordNativeClearFlagsArgs(uint32_t render_context, uint32_t clear_flags);
void RecordNativePassDrawBoundary(const NativeStatePassDrawBoundary& boundary);
void RecordNativePassDrawBoundaryArgs(uint32_t submit_object,
                                      uint32_t tls_or_pass_context,
                                      uint32_t pass_flags, uint32_t drawable,
                                      uint32_t draw_callback, uint32_t wireframe,
                                      uint32_t draw_mode, uint32_t pass_marker);
void RecordNativeDirectDrawEntry(const NativeStateDirectDrawEntry& entry);
void RecordNativeVertexShaderStateArgs(uint32_t render_context,
                                       uint32_t shader);
void RecordNativePixelShaderStateArgs(uint32_t render_context,
                                      uint32_t shader);
void RecordNativeVertexStreamBindingArgs(uint32_t render_context,
                                         uint32_t slot, uint32_t resource,
                                         uint32_t byte_offset,
                                         uint32_t stride_bytes,
                                         uint32_t dirty_mask);
void RecordNativeIndexBufferBindingArgs(uint32_t render_context,
                                        uint32_t resource);
void RecordNativeBoundSurfaceArgs(uint32_t render_context, uint32_t surface,
                                  uint32_t surface_arg);
NativeStateSnapshot SnapshotNativeState(uint32_t render_context);
NativeStateSnapshot SnapshotLastNativeState();
NativeStateSnapshot SnapshotNativeStateForDirectDraw(
    uint32_t direct_render_context);

}
