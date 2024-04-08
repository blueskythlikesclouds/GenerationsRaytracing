#include "PackedPrimitives.hlsli"

StructuredBuffer<float4x4> g_Pose : register(t0);

cbuffer GeometryDesc : register(b0)
{
    uint g_VertexCount;
    uint g_VertexStride;
    uint g_NormalOffset;
    uint g_TangentOffset;
    uint g_BinormalOffset;
    uint g_BlendWeightOffset;
    uint g_BlendIndicesOffset;
    uint g_NodeCount;
}

StructuredBuffer<uint> g_Palette : register(t1);
ByteAddressBuffer g_Src : register(t2);
RWByteAddressBuffer g_Dest : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= g_VertexCount)
        return;

    uint vertexOffset = dispatchThreadId.x * g_VertexStride;
    uint prevVertexOffset = g_VertexCount * g_VertexStride + dispatchThreadId.x * 0xC;

    g_Dest.Store3(prevVertexOffset, g_Dest.Load3(vertexOffset));

    uint2 blendIndicesAndWeight = g_Src.Load2(vertexOffset + g_BlendIndicesOffset);

    uint4 blendIndices = uint4(
        (blendIndicesAndWeight.x >> 0) & 0xFF,
        (blendIndicesAndWeight.x >> 8) & 0xFF,
        (blendIndicesAndWeight.x >> 16) & 0xFF,
        (blendIndicesAndWeight.x >> 24) & 0xFF);

    blendIndices = min(g_NodeCount - 1, blendIndices);

    float4 blendWeight = float4(
        ((blendIndicesAndWeight.y >> 0) & 0xFF) / 255.0,
        ((blendIndicesAndWeight.y >> 8) & 0xFF) / 255.0,
        ((blendIndicesAndWeight.y >> 16) & 0xFF) / 255.0,
        ((blendIndicesAndWeight.y >> 24) & 0xFF) / 255.0);

    blendWeight.w = 1.0 - blendWeight.x - blendWeight.y - blendWeight.z;

    float4x4 nodeMatrix =
        g_Pose[g_Palette[blendIndices.x]] * blendWeight.x +
        g_Pose[g_Palette[blendIndices.y]] * blendWeight.y +
        g_Pose[g_Palette[blendIndices.z]] * blendWeight.z +
        g_Pose[g_Palette[blendIndices.w]] * blendWeight.w;

    float3 position = g_Src.Load<float3>(vertexOffset);
    uint3 nrmTgnBin = g_Src.Load3(vertexOffset + g_NormalOffset);
    float3 normal = DecodeSnorm10(nrmTgnBin.x);
    float3 tangent = DecodeSnorm10(nrmTgnBin.y);
    float3 binormal = DecodeSnorm10(nrmTgnBin.z);

    position = mul(nodeMatrix, float4(position, 1.0)).xyz;
    normal = normalize(mul(nodeMatrix, float4(normal, 0.0)).xyz);
    tangent = normalize(mul(nodeMatrix, float4(tangent, 0.0)).xyz);
    binormal = normalize(mul(nodeMatrix, float4(binormal, 0.0)).xyz);

    g_Dest.Store<float3>(vertexOffset, position);
    g_Dest.Store3(vertexOffset + g_NormalOffset, uint3(EncodeSnorm10(normal), EncodeSnorm10(tangent), EncodeSnorm10(binormal)));
}