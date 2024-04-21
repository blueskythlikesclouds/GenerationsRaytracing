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
    float3 crgb = 1.0 - exp2(1.76112 - 5.0 * rgb);

    return float4(select(rgb >= 0.72974005284, crgb, rgb), 1.0);
}