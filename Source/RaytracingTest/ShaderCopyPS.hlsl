#include "ShaderToneMap.hlsli"

float4 main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD) : SV_Target0
{
    return g_Texture0.Sample(g_LinearClampSampler, texCoord);
}