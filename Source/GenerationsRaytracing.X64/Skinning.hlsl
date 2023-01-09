#define SHADER_DEFINITIONS_NO_BINDING_LAYOUT
#include "ShaderDefinitions.hlsli"

struct Node
{
    float4x4 Matrix;
};

ConstantBuffer<Mesh> g_Mesh : register(b0);

ByteAddressBuffer g_Source : register(t0);
Buffer<uint> g_NodeIndices : register(t1);
StructuredBuffer<Node> g_Nodes : register(t2);

RWByteAddressBuffer g_Destination : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= g_Mesh.VertexCount)
        return;

    uint vertexOffset = dispatchThreadId.x * g_Mesh.VertexStride;
    uint encodedBlendWeight = g_Source.Load(vertexOffset + g_Mesh.BlendWeightOffset);
    uint encodedBlendIndices = g_Source.Load(vertexOffset + g_Mesh.BlendIndicesOffset);

    float4 blendWeight = float4(
        ((encodedBlendWeight >> 0) & 0xFF) / 255.0,
        ((encodedBlendWeight >> 8) & 0xFF) / 255.0,
        ((encodedBlendWeight >> 16) & 0xFF) / 255.0,
        ((encodedBlendWeight >> 24) & 0xFF) / 255.0);

    blendWeight /= blendWeight.x + blendWeight.y + blendWeight.z + blendWeight.w;

    uint4 blendIndices = uint4(
        g_NodeIndices[(encodedBlendIndices >> 0) & 0xFF],
        g_NodeIndices[(encodedBlendIndices >> 8) & 0xFF],
        g_NodeIndices[(encodedBlendIndices >> 16) & 0xFF],
        g_NodeIndices[(encodedBlendIndices >> 24) & 0xFF]);

    float4x4 blendMatrix =
        g_Nodes[blendIndices.x].Matrix * blendWeight.x +
        g_Nodes[blendIndices.y].Matrix * blendWeight.y +
        g_Nodes[blendIndices.z].Matrix * blendWeight.z +
        g_Nodes[blendIndices.w].Matrix * blendWeight.w;

    float3 position = asfloat(g_Source.Load3(vertexOffset));
    float3 normal = asfloat(g_Source.Load3(vertexOffset + g_Mesh.NormalOffset));
    float3 tangent = asfloat(g_Source.Load3(vertexOffset + g_Mesh.TangentOffset));
    float3 binormal = asfloat(g_Source.Load3(vertexOffset + g_Mesh.BinormalOffset));

    position = mul(blendMatrix, float4(position, 1.0)).xyz;
    normal = mul(blendMatrix, float4(normal, 0.0)).xyz;
    tangent = mul(blendMatrix, float4(tangent, 0.0)).xyz;
    binormal = mul(blendMatrix, float4(binormal, 0.0)).xyz;

    g_Destination.Store3(vertexOffset, asuint(position));
    g_Destination.Store3(vertexOffset + g_Mesh.NormalOffset, asuint(normal));
    g_Destination.Store3(vertexOffset + g_Mesh.TangentOffset, asuint(tangent));
    g_Destination.Store3(vertexOffset + g_Mesh.BinormalOffset, asuint(binormal));
    g_Destination.Store2(vertexOffset + g_Mesh.TexCoordOffset, g_Source.Load2(vertexOffset + g_Mesh.TexCoordOffset));

    if (g_Mesh.ColorFormat != 0)
        g_Destination.Store(vertexOffset + g_Mesh.ColorOffset, g_Source.Load(vertexOffset + g_Mesh.ColorOffset));
    else
        g_Destination.Store4(vertexOffset + g_Mesh.ColorOffset, g_Source.Load4(vertexOffset + g_Mesh.ColorOffset));
}