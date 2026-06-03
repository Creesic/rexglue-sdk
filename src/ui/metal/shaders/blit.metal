#include <metal_stdlib>
using namespace metal;
struct VOut { float4 pos [[position]]; float2 uv [[user(loc0)]]; };
vertex VOut blit_vs(uint vid [[vertex_id]]) {
    float2 p[3] = { float2(-1,-1), float2(3,-1), float2(-1,3) };
    float2 u[3] = { float2(0,1), float2(2,1), float2(0,-1) };
    VOut o;
    o.pos = float4(p[vid], 0, 1);
    o.uv = u[vid];
    return o;
}
fragment float4 blit_fs(VOut i [[stage_in]],
                        texture2d<float, access::sample> tex [[texture(0)]],
                        sampler s [[sampler(0)]]) {
    return tex.sample(s, i.uv);
}
