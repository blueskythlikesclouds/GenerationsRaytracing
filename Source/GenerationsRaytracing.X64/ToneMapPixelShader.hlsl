cbuffer Globals : register(b1)
{
    uint g_TextureIndex : packoffset(c224.x);
    uint g_SamplerIndex : packoffset(c228.x);
};

float4 main(float4 position : SV_Position, float4 texCoord : TEXCOORD) : SV_Target
{
    Texture2D texture = ResourceDescriptorHeap[g_TextureIndex];
    SamplerState samplerState = SamplerDescriptorHeap[g_SamplerIndex];
    
    float3 rgb = texture.SampleLevel(samplerState, texCoord.xy, 0).rgb;

    const float grayPoint = 0.5;
    const float whitePoint = 4.0;
    const float curve = 2.0;
    const float scale = (whitePoint - grayPoint) / pow(pow((1.0 - grayPoint) / (whitePoint - grayPoint), -curve) - 1.0, 1.0 / curve);
    
    float3 tonemappedRgb = (rgb - grayPoint) / scale;
    tonemappedRgb = saturate(grayPoint + scale * tonemappedRgb / pow(1.0 + pow(tonemappedRgb, curve), 1.0 / curve));
    
    rgb = select(rgb >= grayPoint, tonemappedRgb, rgb);
    
    return float4(rgb, 1.0);
}