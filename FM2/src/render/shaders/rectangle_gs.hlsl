#include "rectangle_list.hlsli"

// Match XenosRecomp's fixed DXIL Interpolators ABI, including unused outputs.
struct Interpolators {
  precise float4 position : SV_Position;
  float4 texcoord[16] : TEXCOORD0;
  float4 color[3] : COLOR0;
  float4 normal[4] : NORMAL0;
  float clipDistance : SV_ClipDistance;
};

[maxvertexcount(4)]
void main(triangle Interpolators input[3], inout TriangleStream<Interpolators> stream) {
  float2 edge12 = input[2].position.xy - input[1].position.xy;
  float2 edge20 = input[0].position.xy - input[2].position.xy;
  float2 edge01 = input[1].position.xy - input[0].position.xy;
  uint first = RectangleFirstVertex(dot(edge12, edge12), dot(edge20, edge20),
                                    dot(edge01, edge01));
  Interpolators a = input[first];
  Interpolators b = input[(first + 1) % 3];
  Interpolators c = input[(first + 2) % 3];
  stream.Append(a);
  stream.Append(b);
  stream.Append(c);

  Interpolators fourth;
  [unroll] for (uint lane = 0; lane < 4; ++lane) {
    fourth.position[lane] = RectangleFourthComponent(a.position[lane], b.position[lane],
                                                      c.position[lane]);
    [unroll] for (uint i = 0; i < 16; ++i)
      fourth.texcoord[i][lane] = RectangleFourthComponent(a.texcoord[i][lane],
          b.texcoord[i][lane], c.texcoord[i][lane]);
    [unroll] for (uint i = 0; i < 3; ++i)
      fourth.color[i][lane] = RectangleFourthComponent(a.color[i][lane],
          b.color[i][lane], c.color[i][lane]);
    [unroll] for (uint i = 0; i < 4; ++i)
      fourth.normal[i][lane] = RectangleFourthComponent(a.normal[i][lane],
          b.normal[i][lane], c.normal[i][lane]);
  }
  fourth.clipDistance = RectangleFourthComponent(a.clipDistance, b.clipDistance, c.clipDistance);
  stream.Append(fourth);
  stream.RestartStrip();
}
