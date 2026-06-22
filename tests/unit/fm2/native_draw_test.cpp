#include <catch2/catch_test_macros.hpp>

#include <string_view>

#include "native_renderer/fm2_native_draw.h"

namespace {

fm2::native_renderer::DirectDrawDebugReplayPlan BuildReadyNativeDrawPlan() {
  namespace fm2nr = fm2::native_renderer;

  fm2nr::DirectDrawDebugReplayPlan plan;
  plan.ready = true;
  plan.topology = fm2nr::DirectDrawReplayTopology::kTriangleStrip;
  plan.index_format = fm2nr::DirectDrawReplayIndexFormat::kUint16;
  plan.stream_count = 2u;
  plan.streams[0] = {.slot = 0u,
                     .stride = 32u,
                     .guest_base = 0xB0BBF697u,
                     .upload_guest_base = 0xB0BBF694u,
                     .upload_bytes = 0xFD60u,
                     .hash = 0x088CCFB61631658Cull};
  plan.streams[1] = {.slot = 1u,
                     .stride = 12u,
                     .guest_base = 0xB09BF463u,
                     .upload_guest_base = 0xB09BF460u,
                     .upload_bytes = 0x125748u,
                     .hash = 0x891376055F6CD53Aull};
  plan.index = {.guest_base = 0xB0BCF3F4u,
                .upload_guest_base = 0xB0BCF3F4u,
                .upload_bytes = 0x4704u,
                .hash = 0x64270C4F8840C8FBull,
                .upload_endian = fm2nr::DirectDrawReplayUploadEndian::kSwap16};
  plan.draw = {.index_count = 4062u,
               .instance_count = 1u,
               .start_index = 0u,
               .base_vertex = 0,
               .start_instance = 0u};
  plan.vertex_payload_hash = 0x924D29737CD56BDCull;
  plan.pixel_payload_hash = 0x225917CA19FF2FA7ull;
  plan.vertex_structural_ucode_hash = 0xAAAABBBBCCCCDDDDull;
  plan.pixel_structural_ucode_hash = 0x1111222233334444ull;
  plan.has_vertex_structural_ucode = true;
  plan.has_pixel_structural_ucode = true;

  fm2nr::DirectDrawReplayNativeStateSummary& state = plan.native_state;
  state.valid = true;
  state.sequence = 223189951u;
  state.render_context = 0x4004D100u;
  state.direct_render_context = 0x4028F960u;
  state.draw_iface = 0x2E0162C0u;
  state.vertex_shader = 0xBACBF1B8u;
  state.pixel_shader = 0xBACBF024u;
  state.streams[0] = {.valid = true,
                      .slot = 0u,
                      .resource = 0x2EF2A900u,
                      .byte_offset = 0u,
                      .stride_bytes = 28u,
                      .dirty_mask = 1u};
  state.streams[1] = {.valid = true,
                      .slot = 1u,
                      .resource = 0x2E660E40u,
                      .byte_offset = 0u,
                      .stride_bytes = 12u,
                      .dirty_mask = 1u};
  state.index_resource = 0x2E2867E0u;
  state.bound_surface = 0x2E049240u;
  state.bound_surface_arg = 2u;
  state.viewport = {.valid = true, .viewport_mode = 3u};
  state.texture_fetch = {.valid = true,
                         .fetch_bits_low = 0x12u,
                         .fetch_bits_mid = 0x34u};
  state.clear = {.valid = true,
                 .clear_color_byte = 0x56u,
                 .clear_flags = 0x78u};
  state.pass = {.valid = true,
                .submit_object = 0x41001000u,
                .tls_or_pass_context = 0x42002000u,
                .pass_flags = 0x33u,
                .drawable = 0x43003000u,
                .draw_callback = 0x44004000u,
                .wireframe = 1u,
                .draw_mode = 2u,
                .pass_marker = 3u};

  return plan;
}

fm2::native_renderer::DirectDrawDebugReplayPlan BuildReadyNativeLayoutDrawPlan() {
  fm2::native_renderer::DirectDrawDebugReplayPlan plan =
      BuildReadyNativeDrawPlan();
  plan.streams[0].stride = 28u;
  plan.streams[0].guest_base = 0xBA000010u;
  plan.streams[0].upload_guest_base = 0xBA000010u;
  plan.streams[0].upload_bytes = 0x7000u;
  plan.streams[1].stride = 12u;
  plan.streams[1].guest_base = 0xBB000000u;
  plan.streams[1].upload_guest_base = 0xBB000000u;
  plan.streams[1].upload_bytes = 0x3000u;
  plan.index.guest_base = 0xBC000000u;
  plan.index.upload_guest_base = 0xBC000000u;
  plan.index.upload_bytes = 0x2000u;
  return plan;
}

}  // namespace

TEST_CASE("FM2 native draw packet separates pass pipeline resources and draw",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  const fm2nr::DirectDrawDebugReplayPlan plan = BuildReadyNativeDrawPlan();
  const fm2nr::NativeDrawPacket packet = fm2nr::BuildNativeDrawPacket(plan);

  REQUIRE(packet.ready);
  CHECK(packet.reject_reason == fm2nr::NativeDrawPacketRejectReason::kNone);
  CHECK(packet.source_sequence == 223189951u);

  REQUIRE(packet.pass.valid);
  CHECK(packet.pass.submit_object == 0x41001000u);
  CHECK(packet.pass.tls_or_pass_context == 0x42002000u);
  CHECK(packet.pass.pass_flags == 0x33u);
  CHECK(packet.pass.drawable == 0x43003000u);
  CHECK(packet.pass.draw_callback == 0x44004000u);
  CHECK(packet.pass.wireframe == 1u);
  CHECK(packet.pass.draw_mode == 2u);
  CHECK(packet.pass.pass_marker == 3u);

  REQUIRE(packet.pipeline.valid);
  CHECK(packet.pipeline.render_context == 0x4004D100u);
  CHECK(packet.pipeline.direct_render_context == 0x4028F960u);
  CHECK(packet.pipeline.draw_iface == 0x2E0162C0u);
  CHECK(packet.pipeline.vertex_shader == 0xBACBF1B8u);
  CHECK(packet.pipeline.pixel_shader == 0xBACBF024u);
  CHECK(packet.pipeline.bound_surface == 0x2E049240u);
  CHECK(packet.pipeline.bound_surface_arg == 2u);
  CHECK(packet.pipeline.viewport_mode == 3u);
  CHECK(packet.pipeline.texture_fetch_bits_low == 0x12u);
  CHECK(packet.pipeline.texture_fetch_bits_mid == 0x34u);
  CHECK(packet.pipeline.clear_color_byte == 0x56u);
  CHECK(packet.pipeline.clear_flags == 0x78u);
  CHECK(packet.pipeline.topology ==
        fm2nr::DirectDrawReplayTopology::kTriangleStrip);
  CHECK(packet.pipeline.index_format ==
        fm2nr::DirectDrawReplayIndexFormat::kUint16);
  CHECK(packet.pipeline.replay_layout ==
        fm2nr::DirectDrawReplayPipelineLayout::kDebugRaw32Side12);
  CHECK(packet.pipeline.native_layout ==
        fm2nr::DirectDrawReplayPipelineLayout::kNativePosition28Side12);
  CHECK(packet.pipeline.vertex_payload_hash == 0x924D29737CD56BDCull);
  CHECK(packet.pipeline.pixel_payload_hash == 0x225917CA19FF2FA7ull);

  REQUIRE(packet.resources.valid);
  CHECK(packet.resources.streams[0].native_resource == 0x2EF2A900u);
  CHECK(packet.resources.streams[0].native_stride_bytes == 28u);
  CHECK(packet.resources.streams[0].replay_guest_base == 0xB0BBF697u);
  CHECK(packet.resources.streams[0].replay_upload_bytes == 0xFD60u);
  CHECK(packet.resources.streams[1].native_resource == 0x2E660E40u);
  CHECK(packet.resources.index.native_resource == 0x2E2867E0u);
  CHECK(packet.resources.index.replay_upload_bytes == 0x4704u);
  CHECK(packet.resources.index.replay_upload_endian ==
        fm2nr::DirectDrawReplayUploadEndian::kSwap16);

  CHECK(packet.draw.index_count == 4062u);
  CHECK(packet.draw.instance_count == 1u);
  CHECK(packet.draw.start_index == 0u);
}

TEST_CASE("FM2 native draw packet reports missing semantic dependencies",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  fm2nr::DirectDrawDebugReplayPlan plan = BuildReadyNativeDrawPlan();
  plan.native_state.pass.valid = false;
  fm2nr::NativeDrawPacket packet = fm2nr::BuildNativeDrawPacket(plan);
  CHECK_FALSE(packet.ready);
  CHECK(packet.reject_reason ==
        fm2nr::NativeDrawPacketRejectReason::kMissingPass);
  CHECK(std::string_view(fm2nr::NativeDrawPacketRejectReasonName(
            packet.reject_reason)) == "missing_pass");

  plan = BuildReadyNativeDrawPlan();
  plan.native_state.vertex_shader = 0u;
  packet = fm2nr::BuildNativeDrawPacket(plan);
  CHECK_FALSE(packet.ready);
  CHECK(packet.reject_reason ==
        fm2nr::NativeDrawPacketRejectReason::kMissingPipelineState);

  plan = BuildReadyNativeDrawPlan();
  plan.native_state.streams[1].valid = false;
  plan.streams[1].upload_bytes = 0u;
  packet = fm2nr::BuildNativeDrawPacket(plan);
  CHECK_FALSE(packet.ready);
  CHECK(packet.reject_reason ==
        fm2nr::NativeDrawPacketRejectReason::kMissingNativeResources);

  plan = BuildReadyNativeDrawPlan();
  plan.ready = false;
  packet = fm2nr::BuildNativeDrawPacket(plan);
  CHECK_FALSE(packet.ready);
  CHECK(packet.reject_reason ==
        fm2nr::NativeDrawPacketRejectReason::kReplayPlanNotReady);
}

TEST_CASE("FM2 native pass command batches packets sharing pass context",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  fm2nr::DirectDrawDebugReplayPlan first_plan = BuildReadyNativeDrawPlan();
  fm2nr::DirectDrawDebugReplayPlan second_plan = BuildReadyNativeDrawPlan();
  second_plan.native_state.sequence = 223189952u;
  second_plan.draw.start_index = 64u;
  second_plan.draw.index_count = 384u;

  const fm2nr::NativeDrawPacket packets[] = {
      fm2nr::BuildNativeDrawPacket(first_plan),
      fm2nr::BuildNativeDrawPacket(second_plan),
  };
  const fm2nr::NativePassCommand command =
      fm2nr::BuildNativePassCommand(packets, 2u);

  REQUIRE(command.ready);
  CHECK(command.reject_reason == fm2nr::NativePassCommandRejectReason::kNone);
  CHECK(command.draw_count == 2u);
  CHECK(command.first_source_sequence == 223189951u);
  CHECK(command.last_source_sequence == 223189952u);

  REQUIRE(command.pass.valid);
  CHECK(command.pass.submit_object == 0x41001000u);
  CHECK(command.pass.pass_marker == 3u);

  CHECK(command.draws[0].pipeline.vertex_shader == 0xBACBF1B8u);
  CHECK(command.draws[0].draw.index_count == 4062u);
  CHECK(command.draws[1].draw.start_index == 64u);
  CHECK(command.draws[1].draw.index_count == 384u);
}

TEST_CASE("FM2 native pass command rejects non-native packet batches",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  fm2nr::DirectDrawDebugReplayPlan first_plan = BuildReadyNativeDrawPlan();
  fm2nr::DirectDrawDebugReplayPlan second_plan = BuildReadyNativeDrawPlan();
  second_plan.native_state.pass.pass_marker = 4u;

  fm2nr::NativeDrawPacket packets[] = {
      fm2nr::BuildNativeDrawPacket(first_plan),
      fm2nr::BuildNativeDrawPacket(second_plan),
  };
  fm2nr::NativePassCommand command =
      fm2nr::BuildNativePassCommand(packets, 2u);
  CHECK_FALSE(command.ready);
  CHECK(command.reject_reason ==
        fm2nr::NativePassCommandRejectReason::kPassMismatch);
  CHECK(std::string_view(fm2nr::NativePassCommandRejectReasonName(
            command.reject_reason)) == "pass_mismatch");

  packets[1] = fm2nr::BuildNativeDrawPacket(first_plan);
  packets[1].ready = false;
  packets[1].reject_reason =
      fm2nr::NativeDrawPacketRejectReason::kMissingPipelineState;
  command = fm2nr::BuildNativePassCommand(packets, 2u);
  CHECK_FALSE(command.ready);
  CHECK(command.reject_reason ==
        fm2nr::NativePassCommandRejectReason::kPacketRejected);
  CHECK(command.first_packet_reject_reason ==
        fm2nr::NativeDrawPacketRejectReason::kMissingPipelineState);
}

TEST_CASE("FM2 native pass submit plan accepts native-layout pass commands",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  fm2nr::DirectDrawDebugReplayPlan first_plan =
      BuildReadyNativeLayoutDrawPlan();
  fm2nr::DirectDrawDebugReplayPlan second_plan =
      BuildReadyNativeLayoutDrawPlan();
  second_plan.native_state.sequence = 223189952u;
  second_plan.draw.start_index = 128u;

  const fm2nr::NativeDrawPacket packets[] = {
      fm2nr::BuildNativeDrawPacket(first_plan),
      fm2nr::BuildNativeDrawPacket(second_plan),
  };
  const fm2nr::NativePassCommand command =
      fm2nr::BuildNativePassCommand(packets, 2u);
  const fm2nr::NativePassSubmitPlan submit_plan =
      fm2nr::BuildNativePassSubmitPlan(command);

  REQUIRE(submit_plan.ready);
  CHECK(submit_plan.reject_reason ==
        fm2nr::NativePassSubmitRejectReason::kNone);
  CHECK(submit_plan.draw_count == 2u);
  CHECK(submit_plan.topology ==
        fm2nr::DirectDrawReplayTopology::kTriangleStrip);
  CHECK(submit_plan.index_format ==
        fm2nr::DirectDrawReplayIndexFormat::kUint16);
  CHECK(submit_plan.layout ==
        fm2nr::DirectDrawReplayPipelineLayout::kNativePosition28Side12);
  CHECK(submit_plan.draws[1].draw.start_index == 128u);
  CHECK(submit_plan.draws[0].resources.streams[0].native_stride_bytes == 28u);
  CHECK(submit_plan.draws[0].resources.streams[0].replay_upload_guest_base ==
        0xBA000010u);
}

TEST_CASE("FM2 native pass submit plan rejects replay-layout packets",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  const fm2nr::NativeDrawPacket packet =
      fm2nr::BuildNativeDrawPacket(BuildReadyNativeDrawPlan());
  const fm2nr::NativePassCommand command =
      fm2nr::BuildNativePassCommand(&packet, 1u);
  const fm2nr::NativePassSubmitPlan submit_plan =
      fm2nr::BuildNativePassSubmitPlan(command);

  CHECK_FALSE(submit_plan.ready);
  CHECK(submit_plan.reject_reason ==
        fm2nr::NativePassSubmitRejectReason::kReplayLayoutNotNative);
  CHECK(std::string_view(fm2nr::NativePassSubmitRejectReasonName(
            submit_plan.reject_reason)) == "replay_layout_not_native");
}

TEST_CASE("FM2 native pass submit plan accepts direct-resource fallback",
          "[fm2][plume]") {
  namespace fm2nr = fm2::native_renderer;

  fm2nr::DirectDrawDebugReplayPlan plan = BuildReadyNativeDrawPlan();
  plan.native_state.streams[0].stride_bytes = 16u;
  plan.native_state.streams[1].resource = 0u;
  plan.native_state.streams[1].stride_bytes = 0u;

  const fm2nr::NativeDrawPacket packet = fm2nr::BuildNativeDrawPacket(plan);
  REQUIRE(packet.ready);
  REQUIRE(packet.resources.valid);
  CHECK(packet.resources.source ==
        fm2nr::NativeDrawResourceSource::kDirectReplay);
  CHECK(packet.resources.streams[0].native_stride_bytes == 32u);
  CHECK(packet.resources.streams[1].native_resource == 0xB09BF463u);

  const fm2nr::NativePassCommand command =
      fm2nr::BuildNativePassCommand(&packet, 1u);
  const fm2nr::NativePassSubmitPlan submit_plan =
      fm2nr::BuildNativePassSubmitPlan(command);

  REQUIRE(submit_plan.ready);
  CHECK(submit_plan.layout ==
        fm2nr::DirectDrawReplayPipelineLayout::kDebugRaw32Side12);
}
