#include "ColorSpace.hlsli"

cbuffer Globals : register(b1)
{
    uint g_TextureIndex : packoffset(c224.x);
    uint g_LutTextureIndex : packoffset(c225.x);
    
    uint g_SamplerIndex : packoffset(c228.x);
    uint g_LutSamplerIndex : packoffset(c229.x);
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
    
    rgb = LinearToSrgb(rgb);
    
    if (g_LutTextureIndex != 0)
    {
        Texture2D lutTexture = ResourceDescriptorHeap[g_LutTextureIndex];
        SamplerState lutSamplerState = SamplerDescriptorHeap[g_LutSamplerIndex];
        
        const float2 g_LUTParam = float2(256, 16);
        
        float cell = rgb.b * (g_LUTParam.y - 1);

        float cellL = floor(cell);
        float cellH = ceil(cell);

        float floatX = 0.5 / g_LUTParam.x;
        float floatY = 0.5 / g_LUTParam.y;
        float rOffset = floatX + rgb.r / g_LUTParam.y * ((g_LUTParam.y - 1) / g_LUTParam.y);
        float gOffset = floatY + rgb.g * ((g_LUTParam.y - 1) / g_LUTParam.y);

        float2 lutTexCoordL = float2(cellL / g_LUTParam.y + rOffset, gOffset);
        float2 lutTexCoordH = float2(cellH / g_LUTParam.y + rOffset, gOffset);

        float3 gradedColorL = lutTexture.SampleLevel(lutSamplerState, lutTexCoordL, 0).rgb;
        float3 gradedColorH = lutTexture.SampleLevel(lutSamplerState, lutTexCoordH, 0).rgb;

        rgb = lerp(gradedColorL, gradedColorH, frac(cell));
    }
    
    return float4(rgb, 1.0);
}