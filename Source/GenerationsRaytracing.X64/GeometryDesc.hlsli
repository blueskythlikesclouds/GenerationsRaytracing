#ifndef GEOMETRY_DESC_HLSLI_INCLUDED
#define GEOMETRY_DESC_HLSLI_INCLUDED

#include "GeometryFlags.h"
#include "PackedPrimitives.hlsli"
#include "RayDifferentials.hlsli"
#include "SelfIntersectionAvoidance.hlsl"

struct GeometryDesc
{
    uint Flags : 12;
    uint IndexBufferId : 20;

    uint VertexBufferId : 20;
    uint VertexStride : 12;
    uint VertexCount;
    uint VertexOffset;

    uint NormalOffset : 8;
    uint TangentOffset : 8;
    uint BinormalOffset : 8;
    uint ColorOffset : 8;

    uint TexCoordOffset0 : 8;
    uint TexCoordOffset1 : 8;
    uint TexCoordOffset2 : 8;
    uint TexCoordOffset3 : 8;

    uint MaterialId;
    uint Padding0;
};

struct InstanceDesc
{
    row_major float3x4 PrevTransform;
};

#define VERTEX_FLAG_NONE     (0 << 0)
#define VERTEX_FLAG_MIPMAP   (1 << 0)
#define VERTEX_FLAG_MULTI_UV (1 << 0)

struct Vertex
{
    float3 Position;
    float3 PrevPosition;
    float3 SafeSpawnPoint;
    float3 Normal;
    float3 Tangent;
    float3 Binormal;
    float2 TexCoords[4];
    float2 TexCoordsDdx[4];
    float2 TexCoordsDdy[4];
    float4 Color;
    uint Flags;
};

float3 NormalizeSafe(float3 value)
{
    float lengthSquared = dot(value, value);
    return select(lengthSquared > 0.0, value * rsqrt(lengthSquared), 0.0);
}

Vertex LoadVertex(
    GeometryDesc geometryDesc, 
    float4 texCoordOffset[2],
    InstanceDesc instanceDesc,
    BuiltInTriangleIntersectionAttributes attributes,
    float3 dDdx,
    float3 dDdy,
    uint flags)
{
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.VertexBufferId)];
    Buffer<uint> indexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.IndexBufferId)];

    float3 uv = float3(
        1.0 - attributes.barycentrics.x - attributes.barycentrics.y,
        attributes.barycentrics.x,
        attributes.barycentrics.y);

    uint3 indices;
    indices.x = indexBuffer[PrimitiveIndex() * 3 + 0];
    indices.y = indexBuffer[PrimitiveIndex() * 3 + 1];
    indices.z = indexBuffer[PrimitiveIndex() * 3 + 2];

    uint3 prevPositionOffsets = geometryDesc.VertexOffset +
        geometryDesc.VertexCount * geometryDesc.VertexStride + indices * 0xC;

    uint3 offsets = geometryDesc.VertexOffset + indices * geometryDesc.VertexStride;
    uint3 normalOffsets = offsets + geometryDesc.NormalOffset;
    uint3 tangentOffsets = offsets + geometryDesc.TangentOffset;
    uint3 binormalOffsets = offsets + geometryDesc.BinormalOffset;
    uint3 colorOffsets = offsets + geometryDesc.ColorOffset;

    Vertex vertex;

    float3 p0 = asfloat(vertexBuffer.Load3(offsets.x));
    float3 p1 = asfloat(vertexBuffer.Load3(offsets.y));
    float3 p2 = asfloat(vertexBuffer.Load3(offsets.z));

    vertex.Position = p0 * uv.x + p1 * uv.y + p2 * uv.z;

    if (geometryDesc.Flags & GEOMETRY_FLAG_POSE)
    {
        vertex.PrevPosition =
            asfloat(vertexBuffer.Load3(prevPositionOffsets.x)) * uv.x +
            asfloat(vertexBuffer.Load3(prevPositionOffsets.y)) * uv.y +
            asfloat(vertexBuffer.Load3(prevPositionOffsets.z)) * uv.z;
    }
    else
    {
        vertex.PrevPosition = vertex.Position;
    }

    vertex.Normal =
        DecodeNormal(vertexBuffer.Load2(normalOffsets.x)) * uv.x +
        DecodeNormal(vertexBuffer.Load2(normalOffsets.y)) * uv.y +
        DecodeNormal(vertexBuffer.Load2(normalOffsets.z)) * uv.z;

    // Some models have correct normals but flipped triangle order
    // Could check for view direction but that makes plants look bad
    // as they have spherical normals
    bool isBackFace = dot(cross(p0 - p2, p0 - p1), vertex.Normal) > 0.0;

    float3 outObjPosition;
    float3 outWldPosition;
    float3 outObjNormal;
    float3 outWldNormal;
    float outWldOffset;
    safeSpawnPoint(
        outObjPosition, outWldPosition,
        outObjNormal, outWldNormal,
        outWldOffset,
        p0, isBackFace ? p2 : p1, isBackFace ? p1 : p2,
        isBackFace ? attributes.barycentrics.yx : attributes.barycentrics.xy,
        ObjectToWorld3x4(),
        WorldToObject3x4());

    vertex.SafeSpawnPoint = safeSpawnPoint(outWldPosition, outWldNormal, outWldOffset);

    vertex.Tangent =
        DecodeNormal(vertexBuffer.Load2(tangentOffsets.x)) * uv.x +
        DecodeNormal(vertexBuffer.Load2(tangentOffsets.y)) * uv.y +
        DecodeNormal(vertexBuffer.Load2(tangentOffsets.z)) * uv.z;

    vertex.Binormal =
        DecodeNormal(vertexBuffer.Load2(binormalOffsets.x)) * uv.x +
        DecodeNormal(vertexBuffer.Load2(binormalOffsets.y)) * uv.y +
        DecodeNormal(vertexBuffer.Load2(binormalOffsets.z)) * uv.z;

    uint texCoordOffsets[4] = { geometryDesc.TexCoordOffset0, geometryDesc.TexCoordOffset1,
        geometryDesc.TexCoordOffset2, geometryDesc.TexCoordOffset3 };

    float2 texCoords[4][3];

    [unroll]
    for (uint i = 0; i < 4; i++)
    {
        [unroll]
        for (uint j = 0; j < 3; j++)
            texCoords[i][j] = DecodeTexCoord(vertexBuffer.Load(offsets[j] + texCoordOffsets[i]));

        vertex.TexCoords[i] = texCoords[i][0] * uv.x + texCoords[i][1] * uv.y + texCoords[i][2] * uv.z;
    }

    vertex.TexCoords[0] += texCoordOffset[0].xy;
    vertex.TexCoords[1] += texCoordOffset[0].zw;
    vertex.TexCoords[2] += texCoordOffset[1].xy;
    vertex.TexCoords[3] += texCoordOffset[1].zw;

    vertex.Color =
        DecodeColor(vertexBuffer.Load(colorOffsets.x)) * uv.x +
        DecodeColor(vertexBuffer.Load(colorOffsets.y)) * uv.y +
        DecodeColor(vertexBuffer.Load(colorOffsets.z)) * uv.z;

    vertex.Flags = flags;

    if (flags & VERTEX_FLAG_MIPMAP)
    {
        RayDiff rayDiff = (RayDiff) 0;
        rayDiff.dDdx = dDdx;
        rayDiff.dDdy = dDdy;

        float2 dBarydx;
        float2 dBarydy;
        ComputeBarycentricDifferentials(
            PropagateRayDifferentials(rayDiff, WorldRayOrigin(), WorldRayDirection(), RayTCurrent(), outWldNormal),
            WorldRayDirection(),
            mul(ObjectToWorld3x4(), float4(p1 - p0, 0.0)).xyz,
            mul(ObjectToWorld3x4(), float4(p2 - p0, 0.0)).xyz,
            outWldNormal, 
            dBarydx, 
            dBarydy);

        [unroll]
        for (uint i = 0; i < 4; i++)
            InterpolateDifferentials(dBarydx, dBarydy, texCoords[i][0], texCoords[i][1], texCoords[i][2], vertex.TexCoordsDdx[i], vertex.TexCoordsDdy[i]);
    }
    else
    {
        [unroll]
        for (uint i = 0; i < 4; i++)
        {
            vertex.TexCoordsDdx[i] = 0.0;
            vertex.TexCoordsDdy[i] = 0.0;
        }
    }

    vertex.Position = mul(ObjectToWorld3x4(), float4(vertex.Position, 1.0)).xyz;
    vertex.PrevPosition = mul(instanceDesc.PrevTransform, float4(vertex.PrevPosition, 1.0)).xyz;
    vertex.Normal = NormalizeSafe(mul(ObjectToWorld3x4(), float4(vertex.Normal, 0.0)).xyz);
    vertex.Tangent = NormalizeSafe(mul(ObjectToWorld3x4(), float4(vertex.Tangent, 0.0)).xyz);
    vertex.Binormal = NormalizeSafe(mul(ObjectToWorld3x4(), float4(vertex.Binormal, 0.0)).xyz);

    return vertex;
}

#endif