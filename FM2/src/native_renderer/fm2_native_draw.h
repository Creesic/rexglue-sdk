#pragma once

#include "native_renderer/fm2_direct_draw_decode.h"

#include <cstdint>

namespace fm2::native_renderer {

enum class NativeDrawPacketRejectReason : uint8_t {
  kNone,
  kReplayPlanNotReady,
  kMissingNativeState,
  kMissingPass,
  kMissingPipelineState,
  kMissingNativeResources,
  kUnsupportedTopology,
  kUnsupportedIndexFormat,
};

constexpr const char* NativeDrawPacketRejectReasonName(
    NativeDrawPacketRejectReason reason) {
  switch (reason) {
    case NativeDrawPacketRejectReason::kNone:
      return "none";
    case NativeDrawPacketRejectReason::kReplayPlanNotReady:
      return "replay_plan_not_ready";
    case NativeDrawPacketRejectReason::kMissingNativeState:
      return "missing_native_state";
    case NativeDrawPacketRejectReason::kMissingPass:
      return "missing_pass";
    case NativeDrawPacketRejectReason::kMissingPipelineState:
      return "missing_pipeline_state";
    case NativeDrawPacketRejectReason::kMissingNativeResources:
      return "missing_native_resources";
    case NativeDrawPacketRejectReason::kUnsupportedTopology:
      return "unsupported_topology";
    case NativeDrawPacketRejectReason::kUnsupportedIndexFormat:
      return "unsupported_index_format";
  }
  return "unknown";
}

struct NativeDrawPassKey {
  bool valid = false;
  uint32_t submit_object = 0;
  uint32_t tls_or_pass_context = 0;
  uint32_t pass_flags = 0;
  uint32_t drawable = 0;
  uint32_t draw_callback = 0;
  uint32_t wireframe = 0;
  uint32_t draw_mode = 0;
  uint32_t pass_marker = 0;
};

struct NativeDrawPipelineKey {
  bool valid = false;
  uint32_t render_context = 0;
  uint32_t direct_render_context = 0;
  uint32_t draw_iface = 0;
  uint32_t vertex_shader = 0;
  uint32_t pixel_shader = 0;
  uint32_t bound_surface = 0;
  uint32_t bound_surface_arg = 0;
  uint32_t viewport_mode = 0;
  uint8_t texture_fetch_bits_low = 0;
  uint8_t texture_fetch_bits_mid = 0;
  uint8_t clear_color_byte = 0;
  uint8_t clear_flags = 0;
  DirectDrawReplayTopology topology = DirectDrawReplayTopology::kUnknown;
  DirectDrawReplayIndexFormat index_format =
      DirectDrawReplayIndexFormat::kUnknown;
  DirectDrawReplayPipelineLayout replay_layout =
      DirectDrawReplayPipelineLayout::kUnsupported;
  DirectDrawReplayPipelineLayout native_layout =
      DirectDrawReplayPipelineLayout::kUnsupported;
  uint64_t vertex_payload_hash = 0;
  uint64_t pixel_payload_hash = 0;
  uint64_t vertex_structural_ucode_hash = 0;
  uint64_t pixel_structural_ucode_hash = 0;
  bool has_vertex_structural_ucode = false;
  bool has_pixel_structural_ucode = false;
};

struct NativeDrawStreamResource {
  bool valid = false;
  uint32_t slot = 0;
  uint32_t native_resource = 0;
  uint32_t native_byte_offset = 0;
  uint32_t native_stride_bytes = 0;
  uint32_t dirty_mask = 0;
  uint32_t replay_guest_base = 0;
  uint32_t replay_upload_guest_base = 0;
  uint32_t replay_upload_bytes = 0;
  uint64_t replay_hash = 0;
  DirectDrawReplayUploadEndian replay_upload_endian =
      DirectDrawReplayUploadEndian::kRaw;
};

struct NativeDrawIndexResource {
  bool valid = false;
  uint32_t native_resource = 0;
  uint32_t replay_guest_base = 0;
  uint32_t replay_upload_guest_base = 0;
  uint32_t replay_upload_bytes = 0;
  uint64_t replay_hash = 0;
  DirectDrawReplayUploadEndian replay_upload_endian =
      DirectDrawReplayUploadEndian::kRaw;
};

struct NativeDrawResourceKey {
  bool valid = false;
  NativeDrawStreamResource streams[2];
  NativeDrawIndexResource index;
};

struct NativeDrawPacket {
  bool ready = false;
  NativeDrawPacketRejectReason reject_reason =
      NativeDrawPacketRejectReason::kMissingNativeState;
  uint64_t source_sequence = 0;
  NativeDrawPassKey pass;
  NativeDrawPipelineKey pipeline;
  NativeDrawResourceKey resources;
  DirectDrawReplayIndexedDraw draw;
};

constexpr NativeDrawPassKey BuildNativeDrawPassKey(
    const DirectDrawReplayNativePassSummary& pass) {
  NativeDrawPassKey out;
  if (!pass.valid) {
    return out;
  }

  out.valid = true;
  out.submit_object = pass.submit_object;
  out.tls_or_pass_context = pass.tls_or_pass_context;
  out.pass_flags = pass.pass_flags;
  out.drawable = pass.drawable;
  out.draw_callback = pass.draw_callback;
  out.wireframe = pass.wireframe;
  out.draw_mode = pass.draw_mode;
  out.pass_marker = pass.pass_marker;
  return out;
}

constexpr bool HasNativeDrawPipelineState(
    const DirectDrawReplayNativeStateSummary& state) {
  return state.valid && state.render_context != 0 && state.vertex_shader != 0 &&
         state.pixel_shader != 0 && state.bound_surface != 0 &&
         state.viewport.valid && state.texture_fetch.valid &&
         state.clear.valid;
}

constexpr NativeDrawPipelineKey BuildNativeDrawPipelineKey(
    const DirectDrawDebugReplayPlan& plan) {
  NativeDrawPipelineKey out;
  const DirectDrawReplayNativeStateSummary& state = plan.native_state;
  if (!HasNativeDrawPipelineState(state)) {
    return out;
  }

  out.valid = true;
  out.render_context = state.render_context;
  out.direct_render_context = state.direct_render_context;
  out.draw_iface = state.draw_iface;
  out.vertex_shader = state.vertex_shader;
  out.pixel_shader = state.pixel_shader;
  out.bound_surface = state.bound_surface;
  out.bound_surface_arg = state.bound_surface_arg;
  out.viewport_mode = state.viewport.viewport_mode;
  out.texture_fetch_bits_low = state.texture_fetch.fetch_bits_low;
  out.texture_fetch_bits_mid = state.texture_fetch.fetch_bits_mid;
  out.clear_color_byte = state.clear.clear_color_byte;
  out.clear_flags = state.clear.clear_flags;
  out.topology = plan.topology;
  out.index_format = plan.index_format;
  out.replay_layout = DirectDrawReplayPipelineLayoutForPlan(plan);
  out.native_layout = DirectDrawReplayNativeLayoutFromState(state);
  out.vertex_payload_hash = plan.vertex_payload_hash;
  out.pixel_payload_hash = plan.pixel_payload_hash;
  out.vertex_structural_ucode_hash = plan.vertex_structural_ucode_hash;
  out.pixel_structural_ucode_hash = plan.pixel_structural_ucode_hash;
  out.has_vertex_structural_ucode = plan.has_vertex_structural_ucode;
  out.has_pixel_structural_ucode = plan.has_pixel_structural_ucode;
  return out;
}

constexpr NativeDrawStreamResource BuildNativeDrawStreamResource(
    const DirectDrawReplayNativeStreamBinding& native_stream,
    const DirectDrawReplayVertexStream& replay_stream) {
  NativeDrawStreamResource out;
  out.slot = native_stream.slot;
  out.native_resource = native_stream.resource;
  out.native_byte_offset = native_stream.byte_offset;
  out.native_stride_bytes = native_stream.stride_bytes;
  out.dirty_mask = native_stream.dirty_mask;
  out.replay_guest_base = replay_stream.guest_base;
  out.replay_upload_guest_base = replay_stream.upload_guest_base;
  out.replay_upload_bytes = replay_stream.upload_bytes;
  out.replay_hash = replay_stream.hash;
  out.replay_upload_endian = replay_stream.upload_endian;
  out.valid = native_stream.valid && native_stream.resource != 0 &&
              native_stream.stride_bytes != 0 &&
              replay_stream.upload_bytes != 0;
  return out;
}

constexpr NativeDrawIndexResource BuildNativeDrawIndexResource(
    const DirectDrawReplayNativeStateSummary& state,
    const DirectDrawReplayIndexBuffer& replay_index) {
  NativeDrawIndexResource out;
  out.native_resource = state.index_resource;
  out.replay_guest_base = replay_index.guest_base;
  out.replay_upload_guest_base = replay_index.upload_guest_base;
  out.replay_upload_bytes = replay_index.upload_bytes;
  out.replay_hash = replay_index.hash;
  out.replay_upload_endian = replay_index.upload_endian;
  out.valid = state.index_resource != 0 && replay_index.upload_bytes != 0;
  return out;
}

constexpr NativeDrawResourceKey BuildNativeDrawResourceKey(
    const DirectDrawDebugReplayPlan& plan) {
  NativeDrawResourceKey out;
  if (!plan.native_state.valid || plan.stream_count != 2u) {
    return out;
  }

  out.streams[0] = BuildNativeDrawStreamResource(plan.native_state.streams[0],
                                                 plan.streams[0]);
  out.streams[1] = BuildNativeDrawStreamResource(plan.native_state.streams[1],
                                                 plan.streams[1]);
  out.index = BuildNativeDrawIndexResource(plan.native_state, plan.index);
  out.valid = out.streams[0].valid && out.streams[1].valid && out.index.valid;
  return out;
}

constexpr NativeDrawPacket BuildNativeDrawPacket(
    const DirectDrawDebugReplayPlan& plan) {
  NativeDrawPacket out;
  out.source_sequence = plan.native_state.sequence;
  out.draw = plan.draw;
  out.pass = BuildNativeDrawPassKey(plan.native_state.pass);
  out.pipeline = BuildNativeDrawPipelineKey(plan);
  out.resources = BuildNativeDrawResourceKey(plan);

  if (!plan.ready) {
    out.reject_reason = NativeDrawPacketRejectReason::kReplayPlanNotReady;
    return out;
  }
  if (!plan.native_state.valid) {
    out.reject_reason = NativeDrawPacketRejectReason::kMissingNativeState;
    return out;
  }
  if (!out.pass.valid) {
    out.reject_reason = NativeDrawPacketRejectReason::kMissingPass;
    return out;
  }
  if (!out.pipeline.valid) {
    out.reject_reason = NativeDrawPacketRejectReason::kMissingPipelineState;
    return out;
  }
  if (plan.topology == DirectDrawReplayTopology::kUnknown) {
    out.reject_reason = NativeDrawPacketRejectReason::kUnsupportedTopology;
    return out;
  }
  if (plan.index_format == DirectDrawReplayIndexFormat::kUnknown) {
    out.reject_reason = NativeDrawPacketRejectReason::kUnsupportedIndexFormat;
    return out;
  }
  if (!out.resources.valid) {
    out.reject_reason = NativeDrawPacketRejectReason::kMissingNativeResources;
    return out;
  }

  out.ready = true;
  out.reject_reason = NativeDrawPacketRejectReason::kNone;
  return out;
}

}  // namespace fm2::native_renderer
