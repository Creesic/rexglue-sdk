struct PushConstants
{
    uint ResourceDescriptorIndex;
};
[[vk::push_constant]] ConstantBuffer<PushConstants> g_PushConstants : register(b3, space4);
Texture2DMS<float4> g_TextureDescriptorHeap[] : register(t0, space0);

float4 ResolvePixel(Texture2DMS<float4> texture, int2 pixel, uint2 extent, uint samples)
{
    pixel = (pixel % int2(extent) + int2(extent)) % int2(extent);
    float4 color = 0;
    for (uint sample = 0; sample < samples; ++sample)
        color += texture.Load(pixel, sample);
    return color / samples;
}

float4 main(float4 position : SV_Position, float2 texCoord : TEXCOORD) : SV_Target
{
    Texture2DMS<float4> texture = g_TextureDescriptorHeap[g_PushConstants.ResourceDescriptorIndex];
    uint width, height, samples;
    texture.GetDimensions(width, height, samples);
    // Match the single-sample blit's default linear-wrap sampler, after resolving
    // each contributing pixel. No temporary texture or per-frame allocation.
    float2 pixel = texCoord * float2(width, height) - 0.5;
    int2 origin = int2(floor(pixel));
    float2 weight = frac(pixel);
    uint2 extent = uint2(width, height);
    return lerp(
        lerp(ResolvePixel(texture, origin, extent, samples),
             ResolvePixel(texture, origin + int2(1, 0), extent, samples), weight.x),
        lerp(ResolvePixel(texture, origin + int2(0, 1), extent, samples),
             ResolvePixel(texture, origin + int2(1, 1), extent, samples), weight.x), weight.y);
}
