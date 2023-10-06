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
    uint encodedBlendWeight = g_Source.Load(vertexOffset + g_BlendWeightOffset);
    uint encodedBlendIndices = g_Source.Load(vertexOffset + g_BlendIndicesOffset);

    float4 blendWeight = float4(
        ((encodedBlendWeight >> 0) & 0xFF) / 255.0,
        ((encodedBlendWeight >> 8) & 0xFF) / 255.0,
        ((encodedBlendWeight >> 16) & 0xFF) / 255.0,
        ((encodedBlendWeight >> 24) & 0xFF) / 255.0);

    blendWeight /= blendWeight.x + blendWeight.y + blendWeight.z + blendWeight.w;

    uint4 blendIndices = uint4(
        LoadPalette((encodedBlendIndices >> 0) & 0xFF),
        LoadPalette((encodedBlendIndices >> 8) & 0xFF),
        LoadPalette((encodedBlendIndices >> 16) & 0xFF),
        LoadPalette((encodedBlendIndices >> 24) & 0xFF));

    float4x4 nodeMatrix =
        g_NodeMatrixArray[blendIndices.x] * blendWeight.x +
        g_NodeMatrixArray[blendIndices.y] * blendWeight.y +
        g_NodeMatrixArray[blendIndices.z] * blendWeight.z +
        g_NodeMatrixArray[blendIndices.w] * blendWeight.w;

    float3 position = asfloat(g_Source.Load3(vertexOffset));
    float3 normal = asfloat(g_Source.Load3(vertexOffset + g_NormalOffset));
    float3 tangent = asfloat(g_Source.Load3(vertexOffset + g_TangentOffset));
    float3 binormal = asfloat(g_Source.Load3(vertexOffset + g_BinormalOffset));

    position = mul(nodeMatrix, float4(position, 1.0)).xyz;
    normal = mul(nodeMatrix, float4(normal, 0.0)).xyz;
    tangent = mul(nodeMatrix, float4(tangent, 0.0)).xyz;
    binormal = mul(nodeMatrix, float4(binormal, 0.0)).xyz;

    g_Destination.Store3(vertexOffset, asuint(position));
    g_Destination.Store3(vertexOffset + g_NormalOffset, asuint(normal));
    g_Destination.Store3(vertexOffset + g_TangentOffset, asuint(tangent));
    g_Destination.Store3(vertexOffset + g_BinormalOffset, asuint(binormal));
}