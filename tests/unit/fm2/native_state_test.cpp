#include <catch2/catch_test_macros.hpp>

#include "native_renderer/fm2_native_renderer.h"
#include "native_renderer/fm2_native_state.h"

TEST_CASE("FM2 native state recorder keeps render context state snapshots",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  fm2nr::NativeStateRecorder recorder;
  recorder.RecordVertexShaderState({.render_context = 0x1000u,
                                    .shader = 0x2000u});
  recorder.RecordPixelShaderState({.render_context = 0x1000u,
                                   .shader = 0x3000u});
  recorder.RecordVertexStreamBinding({.render_context = 0x1000u,
                                      .slot = 1u,
                                      .resource = 0x4000u,
                                      .byte_offset = 0x20u,
                                      .stride_bytes = 0x20u,
                                      .dirty_mask = 0x40u});
  recorder.RecordIndexBufferBinding({.render_context = 0x1000u,
                                     .resource = 0x5000u});
  recorder.RecordBoundSurface({.render_context = 0x1000u,
                               .surface = 0x6000u,
                               .surface_arg = 2u});
  recorder.RecordPassDrawBoundary({.submit_object = 0x7000u,
                                   .tls_or_pass_context = 0x8000u,
                                   .pass_flags = 0x22u,
                                   .drawable = 0x9000u,
                                   .draw_callback = 0xA000u,
                                   .wireframe = 1u,
                                   .draw_mode = 2u,
                                   .pass_marker = 3u});
  recorder.RecordDirectDrawEntry({.direct_render_context = 0x1000u,
                                  .draw_iface = 0xB000u});

  const fm2nr::NativeStateSnapshot snapshot = recorder.Snapshot(0x1000u);

  REQUIRE(snapshot.valid);
  CHECK(snapshot.render_context == 0x1000u);
  CHECK(snapshot.sequence == 7u);
  CHECK(snapshot.vertex_shader.valid);
  CHECK(snapshot.vertex_shader.shader == 0x2000u);
  CHECK(snapshot.pixel_shader.valid);
  CHECK(snapshot.pixel_shader.shader == 0x3000u);
  REQUIRE(snapshot.streams[1].valid);
  CHECK(snapshot.streams[1].resource == 0x4000u);
  CHECK(snapshot.streams[1].byte_offset == 0x20u);
  CHECK(snapshot.streams[1].stride_bytes == 0x20u);
  CHECK(snapshot.streams[1].dirty_mask == 0x40u);
  CHECK(snapshot.index_buffer.valid);
  CHECK(snapshot.index_buffer.resource == 0x5000u);
  CHECK(snapshot.bound_surface.valid);
  CHECK(snapshot.bound_surface.surface == 0x6000u);
  CHECK(snapshot.last_pass.valid);
  CHECK(snapshot.last_pass.draw_callback == 0xA000u);
  CHECK(snapshot.last_direct_draw.valid);
  CHECK(snapshot.last_direct_draw.draw_iface == 0xB000u);
}

TEST_CASE("FM2 native state adapters map hook arguments into snapshots",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  fm2nr::ResetNativeStateRecorder();

  fm2nr::RecordNativeVertexShaderStateArgs(0x1000u, 0x2000u);
  fm2nr::RecordNativePixelShaderStateArgs(0x1000u, 0x3000u);
  fm2nr::RecordNativeVertexStreamBindingArgs(0x1000u, 2u, 0x4000u, 0x24u,
                                             0x28u, 0x80u);
  fm2nr::RecordNativeIndexBufferBindingArgs(0x1000u, 0x5000u);
  fm2nr::RecordNativeBoundSurfaceArgs(0x1000u, 0x6000u, 3u);

  const fm2nr::NativeStateSnapshot snapshot =
      fm2nr::SnapshotNativeState(0x1000u);

  REQUIRE(snapshot.valid);
  CHECK(snapshot.sequence == 5u);
  CHECK(snapshot.vertex_shader.valid);
  CHECK(snapshot.vertex_shader.shader == 0x2000u);
  CHECK(snapshot.pixel_shader.valid);
  CHECK(snapshot.pixel_shader.shader == 0x3000u);
  REQUIRE(snapshot.streams[2].valid);
  CHECK(snapshot.streams[2].resource == 0x4000u);
  CHECK(snapshot.streams[2].byte_offset == 0x24u);
  CHECK(snapshot.streams[2].stride_bytes == 0x28u);
  CHECK(snapshot.streams[2].dirty_mask == 0x80u);
  CHECK(snapshot.index_buffer.valid);
  CHECK(snapshot.index_buffer.resource == 0x5000u);
  CHECK(snapshot.bound_surface.valid);
  CHECK(snapshot.bound_surface.surface == 0x6000u);
  CHECK(snapshot.bound_surface.surface_arg == 3u);
}

TEST_CASE("FM2 native state pairs direct draws with latest render context",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  fm2nr::ResetNativeStateRecorder();

  fm2nr::RecordNativeVertexShaderStateArgs(0x1000u, 0x2000u);
  fm2nr::RecordNativePixelShaderStateArgs(0x1000u, 0x3000u);
  fm2nr::RecordNativeVertexStreamBindingArgs(0x1000u, 0u, 0x4000u, 0x20u,
                                             0x24u, 0x40u);
  fm2nr::RecordNativeIndexBufferBindingArgs(0x1000u, 0x5000u);
  fm2nr::RecordNativeBoundSurfaceArgs(0x1000u, 0x6000u, 2u);
  fm2nr::RecordNativeDirectDrawEntry({.direct_render_context = 0x9000u,
                                      .draw_iface = 0xA000u});

  const fm2nr::NativeStateSnapshot snapshot =
      fm2nr::SnapshotNativeStateForDirectDraw(0x9000u);

  REQUIRE(snapshot.valid);
  CHECK(snapshot.render_context == 0x1000u);
  CHECK(snapshot.vertex_shader.valid);
  CHECK(snapshot.pixel_shader.valid);
  CHECK(snapshot.streams[0].valid);
  CHECK(snapshot.index_buffer.valid);
  CHECK(snapshot.bound_surface.valid);
  CHECK(snapshot.last_direct_draw.valid);
  CHECK(snapshot.last_direct_draw.direct_render_context == 0x9000u);
  CHECK(snapshot.last_direct_draw.draw_iface == 0xA000u);
}

TEST_CASE("FM2 native state recorder ignores invalid stream slots",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  fm2nr::NativeStateRecorder recorder;
  recorder.RecordVertexStreamBinding({.render_context = 0x1000u,
                                      .slot = 99u,
                                      .resource = 0x4000u,
                                      .byte_offset = 0u,
                                      .stride_bytes = 0x20u,
                                      .dirty_mask = 0u});

  const fm2nr::NativeStateSnapshot snapshot = recorder.Snapshot(0x1000u);

  REQUIRE(snapshot.valid);
  CHECK(snapshot.sequence == 0u);
  for (const fm2nr::NativeStateVertexStreamBinding& stream :
       snapshot.streams) {
    CHECK_FALSE(stream.valid);
  }
}

TEST_CASE("FM2 Plume companion window policy places it beside the host",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  const fm2nr::NativeWindowRect host = {
      .x = 100,
      .y = 80,
      .width = 1280,
      .height = 720,
  };
  const fm2nr::NativeWindowRect placed =
      fm2nr::PlaceCompanionWindowBesideHost(host, 960, 540, 24);

  CHECK(placed.x == 1404);
  CHECK(placed.y == 80);
  CHECK(placed.width == 960);
  CHECK(placed.height == 540);

  const fm2nr::NativeWindowRect invalid = {};
  const fm2nr::NativeWindowRect fallback =
      fm2nr::PlaceCompanionWindowBesideHost(invalid, 960, 540, 24);
  CHECK(fallback.x == 24);
  CHECK(fallback.y == 24);
  CHECK(fallback.width == 960);
  CHECK(fallback.height == 540);
}

TEST_CASE("FM2 Plume wireframe policy maps to render fill mode",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  CHECK(fm2nr::DebugReplayFillModeForWireframe(false) ==
        fm2nr::DebugReplayFillMode::kSolid);
  CHECK(fm2nr::DebugReplayFillModeForWireframe(true) ==
        fm2nr::DebugReplayFillMode::kWireframe);
}

TEST_CASE("FM2 Plume live direct draw hook uses per-frame hybrid path",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  CHECK(fm2nr::kFM2PlumeLiveDirectDrawHookAddress ==
        fm2nr::kFM2RenderInstanceHybridDrawPathAddress);
  CHECK(fm2nr::kFM2PlumeLiveDirectDrawHookAddress !=
        fm2nr::kFM2RenderBuildDirectIndexedDrawBuffersAddress);
  CHECK(fm2nr::IsFM2PlumeLiveDirectDrawHookAddress(0x82539650u));
  CHECK_FALSE(fm2nr::IsFM2PlumeLiveDirectDrawHookAddress(0x825380B8u));
}
