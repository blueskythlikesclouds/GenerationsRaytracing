#include "ShaderToneMap.hlsli"

float4 main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD) : SV_Target0
{
    float lum0 = g_Texture0.Load(0).x;
    float lum1 = g_Texture1.Load(0).x;

    return min(lerp(lum1, lum0, exp2(g_Globals.deltaTime * -2.62317109)), 65504);
}