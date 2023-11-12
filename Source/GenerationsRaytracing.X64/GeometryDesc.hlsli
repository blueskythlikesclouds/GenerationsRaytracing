#ifndef GEOMETRY_DESC_HLSLI_INCLUDED
#define GEOMETRY_DESC_HLSLI_INCLUDED

#include "GeometryFlags.h"
#include "RayDifferentials.hlsli"
#include "SelfIntersectionAvoidance.hlsl"

struct GeometryDesc
{
    uint Flags;
    uint VertexCount;
    uint IndexBufferId;
    uint VertexBufferId;
    uint VertexStride;
    uint PositionOffset;
    uint NormalOffset;
    uint TangentOffset;
    uint BinormalOffset;
    uint TexCoordOffsets[4];
    uint ColorOffset;
    uint MaterialId;
    uint Padding0;
};

struct InstanceDesc
{
    row_major float3x4 PrevTransform;
};

#define VERTEX_FLAG_NONE     (0 << 0)
#define VERTEX_FLAG_MIP_MAP  (1 << 0)
#define VERTEX_FLAG_MULTI_UV (1 << 1)

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
    return lengthSquared > 0.0 ? value * rsqrt(lengthSquared) : 0.0;
}

float4 DecodeColor(uint color)
{
    return float4(
        ((color >> 0) & 0xFF) / 255.0,
        ((color >> 8) & 0xFF) / 255.0,
        ((color >> 16) & 0xFF) / 255.0,
        ((color >> 24) & 0xFF) / 255.0);
}

Vertex LoadVertex(
    GeometryDesc geometryDesc, 
    float4 texCoordOffsets[2],
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

    uint3 prevPositionOffsets = geometryDesc.PositionOffset +
        geometryDesc.VertexCount * geometryDesc.VertexStride + indices * 0xC;

    uint3 offsets = indices * geometryDesc.VertexStride;
    uint3 positionOffsets = offsets + geometryDesc.PositionOffset;
    uint3 normalOffsets = offsets + geometryDesc.NormalOffset;
    uint3 tangentOffsets = offsets + geometryDesc.TangentOffset;
    uint3 binormalOffsets = offsets + geometryDesc.BinormalOffset;
    uint3 colorOffsets = offsets + geometryDesc.ColorOffset;

    Vertex vertex;

    float3 p0 = asfloat(vertexBuffer.Load3(positionOffsets.x));
    float3 p1 = asfloat(vertexBuffer.Load3(positionOffsets.y));
    float3 p2 = asfloat(vertexBuffer.Load3(positionOffsets.z));

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
        asfloat(vertexBuffer.Load3(normalOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(normalOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(normalOffsets.z)) * uv.z;

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
        asfloat(vertexBuffer.Load3(tangentOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(tangentOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(tangentOffsets.z)) * uv.z;

    vertex.Binormal =
        asfloat(vertexBuffer.Load3(binormalOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(binormalOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(binormalOffsets.z)) * uv.z;

    float2 texCoords[4][3];

    [unroll]
    for (uint i = 0; i < 4; i++)
    {
        [unroll]
        for (uint j = 0; j < 3; j++)
            texCoords[i][j] = asfloat(vertexBuffer.Load2(offsets[j] + geometryDesc.TexCoordOffsets[i]));

        vertex.TexCoords[i] = texCoords[i][0] * uv.x + texCoords[i][1] * uv.y + texCoords[i][2] * uv.z;
    }

    vertex.TexCoords[0] += texCoordOffsets[0].xy;
    vertex.TexCoords[1] += texCoordOffsets[0].zw;
    vertex.TexCoords[2] += texCoordOffsets[1].xy;
    vertex.TexCoords[3] += texCoordOffsets[1].zw;

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

    vertex.Flags = flags;

    if (flags & VERTEX_FLAG_MIP_MAP)
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