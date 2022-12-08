#include "ShaderToneMap.hlsli"

float main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD) : SV_Target0
{
    float3 color = g_Texture0.Sample(g_LinearClampSampler, texCoord).rgb;
    return clamp(dot(color, float3(0.2126, 0.7152, 0.0722)), g_Globals.lumMin, g_Globals.lumMax);
}