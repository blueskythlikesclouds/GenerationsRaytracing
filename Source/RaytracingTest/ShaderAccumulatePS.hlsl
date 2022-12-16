#include "ShaderToneMap.hlsli"

float4 main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD) : SV_Target0
{
    float4 texture0 = g_Texture0.SampleLevel(g_LinearClampSampler, texCoord, 0);
    float4 texture1 = g_Texture1.SampleLevel(g_LinearClampSampler, texCoord, 0);

    return lerp(texture0, texture1, g_Globals.currentAccum);
}