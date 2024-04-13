#include "Im3dShared.hlsli"

float4 main(
    in float4 position : SV_Position,
    noperspective in float size : SIZE,
    in float4 color : COLOR,
    noperspective in float edgeDistance : EDGE_DISTANCE) : SV_Target
{
    float d = abs(edgeDistance) / size;
    d = smoothstep(1.0, 1.0 - (ANTI_ALIASING / size), d);
    return float4(color.rgb, d * color.a);
}