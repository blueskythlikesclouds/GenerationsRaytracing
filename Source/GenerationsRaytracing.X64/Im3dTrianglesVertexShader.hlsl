#include "Im3dShared.hlsli"

void main(
    in uint vertexId : SV_VertexID,
    in uint instanceId : SV_InstanceID,
    out float4 position : SV_Position,
    out float4 color : COLOR)
{
    VertexData vertexData = g_VertexData[instanceId * 3 + vertexId];
    position = mul(mul(float4(vertexData.Position, 1.0), g_MtxView), g_MtxProjection);
    color = DecodeColor(vertexData.Color);
}