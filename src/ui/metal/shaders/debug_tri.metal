using namespace metal;
struct VOut { float4 pos [[position]]; half4 col [[user(loc0)]]; };
vertex VOut debug_tri_vs(uint vid [[vertex_id]]) {
    float2 p[3] = { float2(-1,-1), float2(3,-1), float2(-1,3) };
    half3 c[3] = { half3(1,0,0), half3(0,1,0), half3(0,0,1) };
    VOut o;
    o.pos = float4(p[vid], 0, 1);
    o.col = half4(c[vid], 1);
    return o;
}
fragment half4 debug_tri_fs(VOut i [[stage_in]]) { return i.col; }
