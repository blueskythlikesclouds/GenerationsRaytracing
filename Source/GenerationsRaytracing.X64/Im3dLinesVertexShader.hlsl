#include "Im3dShared.hlsli"

void main(
    in uint vertexId : SV_VertexID,
    in uint instanceId : SV_InstanceID,
    out float4 position : SV_Position,
    noperspective out float size : SIZE,
    out float4 color : COLOR,
    noperspective out float edgeDistance : EDGE_DISTANCE)
{
    float2 vertex = ComputeVertex(vertexId);

    int vertexId0 = instanceId * 2;
    int vertexId1 = vertexId0 + 1;
    int vertexId2 = (vertexId % 2 == 0) ? vertexId0 : vertexId1;
			
    color = DecodeColor(g_VertexData[vertexId2].Color);
    size = g_VertexData[vertexId2].Size;
    color.a *= smoothstep(0.0, 1.0, size / ANTI_ALIASING);
    size = max(size, ANTI_ALIASING);
    edgeDistance = size * vertex.y;
			
    float4 pos0 = mul(mul(float4(g_VertexData[vertexId0].Position, 1.0), g_MtxView), g_MtxProjection);
    float4 pos1 = mul(mul(float4(g_VertexData[vertexId1].Position, 1.0), g_MtxView), g_MtxProjection);
    float2 dir = (pos0.xy / pos0.w) - (pos1.xy / pos1.w);
    dir = normalize(float2(dir.x, dir.y * g_ViewportSize.y / g_ViewportSize.x));
    float2 tng = float2(-dir.y, dir.x) * size / g_ViewportSize;
			
    position = (vertexId % 2 == 0) ? pos0 : pos1;
    position.xy += tng * vertex.y * position.w;
}