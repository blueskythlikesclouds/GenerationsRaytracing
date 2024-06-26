#include "PackedPrimitives.hlsli"

cbuffer GeometryDesc : register(b0)
{
    uint g_IndexBufferId;
    uint g_IndexOffset;
    uint g_VertexStride;
    uint g_VertexCount;
    uint g_NormalOffset;
    bool g_IsMirrored;
}

RWByteAddressBuffer g_Vertices : register(u0);
ByteAddressBuffer g_Adjacency : register(t0);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= g_VertexCount)
        return;

    uint2 adjacency = g_Adjacency.Load2(dispatchThreadId.x * sizeof(uint) * 2);
    adjacency.x += g_VertexCount * 2;

    Buffer<uint> indices = ResourceDescriptorHeap[g_IndexBufferId];
    float3 normal = 0.0;

    for (uint i = 0; i < adjacency.y; i++)
    {
        uint index = g_Adjacency.Load((adjacency.x + i) * sizeof(uint));

        uint a = indices[g_IndexOffset + index * 3 + 0];
        uint b = indices[g_IndexOffset + index * 3 + 1];
        uint c = indices[g_IndexOffset + index * 3 + 2];

        float3 posA = g_Vertices.Load<float3>(a * g_VertexStride);
        float3 posB = g_Vertices.Load<float3>(b * g_VertexStride);
        float3 posC = g_Vertices.Load<float3>(c * g_VertexStride);

        if (!isnan(posA.x) && !isnan(posB.x) && !isnan(posC.x))
            normal += normalize(cross(posA - posC, posA - posB));
    }
    
    if (g_IsMirrored)
        normal *= -1.0;

    g_Vertices.Store(g_NormalOffset + dispatchThreadId.x * g_VertexStride, EncodeSnorm10(normalize(normal)));
}