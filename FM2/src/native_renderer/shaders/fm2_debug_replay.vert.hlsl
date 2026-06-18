struct VSInput {
  uint4 raw0 : RAW0;
  uint4 raw1 : RAW1;
  uint3 side0 : SIDE0;
  uint vertex_id : SV_VertexID;
};

struct VSOutput {
  float4 position : SV_POSITION;
  float4 color : COLOR0;
};

[[vk::push_constant]]
struct DebugReplayConstants {
  float4 transform_row0;
  float4 transform_row1;
  float4 transform_row2;
  float4 transform_row3;
  uint transform_mode;
  uint transform_valid;
  uint2 reserved;
} constants;

float4 TransformRowMajorClip(float4 local_position) {
  return float4(
      dot(constants.transform_row0, local_position),
      dot(constants.transform_row1, local_position),
      dot(constants.transform_row2, local_position),
      dot(constants.transform_row3, local_position));
}

float4 TransformColumnMajorClip(float4 local_position) {
  return constants.transform_row0 * local_position.x +
         constants.transform_row1 * local_position.y +
         constants.transform_row2 * local_position.z +
         constants.transform_row3 * local_position.w;
}

VSOutput VSMain(VSInput input) {
  const float3 stream0_position =
      float3(asfloat(input.raw0.x), asfloat(input.raw0.y), asfloat(input.raw0.z));
  const float4 local_position = float4(stream0_position, 1.0f);
  const float2 projected_position =
      clamp(float2(stream0_position.x, -stream0_position.y) * 0.85f,
            float2(-0.98f, -0.98f), float2(0.98f, 0.98f));
  const uint packed_color =
      input.raw1.x ^ (input.raw1.y >> 3u) ^ input.side0.z ^ input.vertex_id;

  VSOutput output;
  output.position = float4(projected_position, 0.5f, 1.0f);
  if (constants.transform_valid != 0u && constants.transform_mode == 1u) {
    output.position = TransformRowMajorClip(local_position);
  } else if (constants.transform_valid != 0u && constants.transform_mode == 2u) {
    output.position = TransformColumnMajorClip(local_position);
  } else if (constants.transform_valid != 0u && constants.transform_mode == 3u) {
    const float4 clip_position = TransformRowMajorClip(local_position);
    output.position = float4(
        clip_position.x, clip_position.y, clip_position.w * 0.5f,
        clip_position.w);
  }
  output.color = float4(
      saturate(stream0_position.z + 0.5f),
      float((packed_color >> 8u) & 0xFFu) / 255.0f,
      float((packed_color >> 16u) & 0xFFu) / 255.0f,
      1.0f);
  return output;
}
