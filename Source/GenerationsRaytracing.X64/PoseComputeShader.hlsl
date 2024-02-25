#include "PackedPrimitives.hlsli"

cbuffer Pose : register(b0)
{
    float4x4 g_NodeMatrixArray[256];
}

cbuffer GeometryDesc : register(b1)
{
    uint g_VertexCount;
    uint g_VertexStride;
    uint g_NormalOffset;
    uint g_TangentOffset;
    uint g_BinormalOffset;
    uint g_BlendWeightOffset;
    uint g_BlendIndicesOffset;
    uint4 g_NodePalette[6];
    uint g_NodePaletteLast;
}

ByteAddressBuffer g_Source : register(t0);
RWByteAddressBuffer g_Destination : register(u0);

uint LoadPalette(uint index)
{
    return g_NodePalette[min(24, index) / 4][min(24, index) % 4];
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= g_VertexCount)
        return;

    uint vertexOffset = dispatchThreadId.x * g_VertexStride;
    uint prevVertexOffset = g_VertexCount * g_VertexStride + dispatchThreadId.x * 0xC;

    g_Destination.Store3(prevVertexOffset, g_Destination.Load3(vertexOffset));

    uint2 blendIndicesAndWeight = g_Source.Load2(vertexOffset + g_BlendIndicesOffset);

    uint4 blendIndices = uint4(
        LoadPalette((blendIndicesAndWeight.x >> 0) & 0xFF),
        LoadPalette((blendIndicesAndWeight.x >> 8) & 0xFF),
        LoadPalette((blendIndicesAndWeight.x >> 16) & 0xFF),
        LoadPalette((blendIndicesAndWeight.x >> 24) & 0xFF));

    float4 blendWeight = float4(
        ((blendIndicesAndWeight.y >> 0) & 0xFF) / 255.0,
        ((blendIndicesAndWeight.y >> 8) & 0xFF) / 255.0,
        ((blendIndicesAndWeight.y >> 16) & 0xFF) / 255.0,
        ((blendIndicesAndWeight.y >> 24) & 0xFF) / 255.0);

    float weightSum = blendWeight.x + blendWeight.y + blendWeight.z + blendWeight.w;
    if (weightSum > 0.0)
        blendWeight /= weightSum;

    float4x4 nodeMatrix =
        g_NodeMatrixArray[blendIndices.x] * blendWeight.x +
        g_NodeMatrixArray[blendIndices.y] * blendWeight.y +
        g_NodeMatrixArray[blendIndices.z] * blendWeight.z +
        g_NodeMatrixArray[blendIndices.w] * blendWeight.w;

    float3 position = g_Source.Load<float3>(vertexOffset);
    float3 normal = DecodeFloat3(g_Source.Load2(vertexOffset + g_NormalOffset));

    uint4 tangentAndBinormal = g_Source.Load4(vertexOffset + g_TangentOffset);
    float3 tangent = DecodeFloat3(tangentAndBinormal.xy);
    float3 binormal = DecodeFloat3(tangentAndBinormal.zw);

    position = mul(nodeMatrix, float4(position, 1.0)).xyz;
    normal = mul(nodeMatrix, float4(normal, 0.0)).xyz;
    tangent = mul(nodeMatrix, float4(tangent, 0.0)).xyz;
    binormal = mul(nodeMatrix, float4(binormal, 0.0)).xyz;

    g_Destination.Store<float3>(vertexOffset, position);
    g_Destination.Store2(vertexOffset + g_NormalOffset, EncodeFloat3(normal));
    g_Destination.Store4(vertexOffset + g_TangentOffset, uint4(EncodeFloat3(tangent), EncodeFloat3(binormal)));
}