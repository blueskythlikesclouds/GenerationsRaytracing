#include "Im3dShared.hlsli"

float4 main(
    in float4 position : SV_Position,
    noperspective in float size : SIZE,
    in float4 color : COLOR,
    noperspective in float2 texCoord : TEXCOORD) : SV_Target
{
    float d = length(texCoord - 0.5);
    d = smoothstep(0.5, 0.5 - (ANTI_ALIASING / size), d);
    return float4(color.rgb, d * color.a);
}