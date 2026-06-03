#include <metal_stdlib>
using namespace metal;

struct Uniforms {
  float4x4 mvp;
};

struct VertexIn {
  float3 position [[attribute(0)]];
  float3 color [[attribute(1)]];
};

struct VertexOut {
  float4 position [[position]];
  float3 color;
};

vertex VertexOut test_cube_vs(VertexIn in [[stage_in]],
                              constant Uniforms& uniforms [[buffer(0)]]) {
  VertexOut out;
  out.position = uniforms.mvp * float4(in.position, 1.0);
  out.color = in.color;
  return out;
}

fragment float4 test_cube_fs(VertexOut in [[stage_in]]) {
  return float4(in.color, 1.0);
}
