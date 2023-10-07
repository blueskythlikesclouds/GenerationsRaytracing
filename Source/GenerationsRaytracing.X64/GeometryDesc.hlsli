#ifndef GEOMETRY_DESC_H
#define GEOMETRY_DESC_H
#include "GeometryFlags.h"

struct GeometryDesc
{
    uint Flags;
    uint IndexBufferId;
    uint VertexBufferId;
    uint VertexStride;
    uint NormalOffset;
    uint TangentOffset;
    uint BinormalOffset;
    uint TexCoordOffsets[4];
    uint ColorOffset;
    uint MaterialId;
};

struct Vertex
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float3 Binormal;
    float2 TexCoords[4];
    float4 Color;
};

float4 DecodeColor(uint color)
{
    return float4(
        ((color >> 0) & 0xFF) / 255.0,
        ((color >> 8) & 0xFF) / 255.0,
        ((color >> 16) & 0xFF) / 255.0,
        ((color >> 24) & 0xFF) / 255.0);
}

Vertex LoadVertex(GeometryDesc geometryDesc, BuiltInTriangleIntersectionAttributes attributes)
{
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.VertexBufferId)];
    Buffer<uint> indexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.IndexBufferId)];

    float3 uv = float3(
        1.0 - attributes.barycentrics.x - attributes.barycentrics.y,
        attributes.barycentrics.x,
        attributes.barycentrics.y);

    uint3 offsets;
    offsets.x = indexBuffer[PrimitiveIndex() * 3 + 0];
    offsets.y = indexBuffer[PrimitiveIndex() * 3 + 1];
    offsets.z = indexBuffer[PrimitiveIndex() * 3 + 2];
    offsets *= geometryDesc.VertexStride;

    uint3 normalOffsets = offsets + geometryDesc.NormalOffset;
    uint3 tangentOffsets = offsets + geometryDesc.TangentOffset;
    uint3 binormalOffsets = offsets + geometryDesc.BinormalOffset;
    uint3 colorOffsets = offsets + geometryDesc.ColorOffset;

    Vertex vertex;
    vertex.Position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    vertex.Normal =
        asfloat(vertexBuffer.Load3(normalOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(normalOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(normalOffsets.z)) * uv.z;

    vertex.Tangent =
        asfloat(vertexBuffer.Load3(tangentOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(tangentOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(tangentOffsets.z)) * uv.z;

    vertex.Binormal =
        asfloat(vertexBuffer.Load3(binormalOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(binormalOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(binormalOffsets.z)) * uv.z;

    [unroll]
    for (uint i = 0; i < 4; i++)
    {
        uint3 texCoordOffsets = offsets + geometryDesc.TexCoordOffsets[i];

        vertex.TexCoords[i] =
            asfloat(vertexBuffer.Load2(texCoordOffsets.x)) * uv.x +
            asfloat(vertexBuffer.Load2(texCoordOffsets.y)) * uv.y +
            asfloat(vertexBuffer.Load2(texCoordOffsets.z)) * uv.z;
    }

    if (geometryDesc.Flags & GEOMETRY_FLAG_D3DCOLOR)
    {
        vertex.Color =
            DecodeColor(vertexBuffer.Load(colorOffsets.x)) * uv.x +
            DecodeColor(vertexBuffer.Load(colorOffsets.y)) * uv.y +
            DecodeColor(vertexBuffer.Load(colorOffsets.z)) * uv.z;
    }
    else
    {
        vertex.Color =
            asfloat(vertexBuffer.Load4(colorOffsets.x)) * uv.x +
            asfloat(vertexBuffer.Load4(colorOffsets.y)) * uv.y +
            asfloat(vertexBuffer.Load4(colorOffsets.z)) * uv.z;
    }

    vertex.Normal = normalize(mul(ObjectToWorld3x4(), float4(vertex.Normal, 0.0)).xyz);
    vertex.Tangent = normalize(mul(ObjectToWorld3x4(), float4(vertex.Tangent, 0.0)).xyz);
    vertex.Binormal = normalize(mul(ObjectToWorld3x4(), float4(vertex.Binormal, 0.0)).xyz);

    return vertex;
}

#endif