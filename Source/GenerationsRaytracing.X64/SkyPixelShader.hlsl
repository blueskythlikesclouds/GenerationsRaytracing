#include "SkySharedDefinitions.hlsli"

float4 SampleTexture2D(uint value, float2 texCoord)
{
    uint textureId = value & 0xFFFFF;
    uint samplerId = (value >> 20) & 0x3FF;

    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(textureId)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(samplerId)];

    return texture.Sample(samplerState, texCoord);
}

float4 main(in PixelShaderInput i) : SV_Target
{
    float4 color = SampleTexture2D(g_DiffuseTextureId, i.DiffuseTexCoord) * i.Color;

    if (g_AlphaTextureId != 0)
        color.a *= SampleTexture2D(g_AlphaTextureId, i.AlphaTexCoord).x;

    if (g_EmissionTextureId != 0)
        color.rgb += SampleTexture2D(g_EmissionTextureId, i.EmissionTexCoord).rgb * g_Ambient.rgb;

    return color;
}