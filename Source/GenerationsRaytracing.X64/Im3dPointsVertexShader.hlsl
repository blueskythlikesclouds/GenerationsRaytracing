#include "Im3dShared.hlsli"

void main(
    in uint vertexId : SV_VertexID,
    in uint instanceId : SV_InstanceID,
    out float4 position : SV_Position, 
    noperspective out float size : SIZE,
    out float4 color : COLOR, 
    noperspective out float2 texCoord : TEXCOORD)
{
    float2 vertex = ComputeVertex(vertexId);
    VertexData vertexData = g_VertexData[instanceId];
    size = max(vertexData.Size, ANTI_ALIASING);
    color = DecodeColor(vertexData.Color);
    color.a *= smoothstep(0.0, 1.0, size / ANTI_ALIASING);

    position = mul(mul(float4(vertexData.Position, 1.0), g_MtxView), g_MtxProjection);
    float2 scale = 1.0 / g_ViewportSize * size;
    position.xy += vertex * scale * position.w;
    texCoord = vertex * 0.5 + 0.5;
}