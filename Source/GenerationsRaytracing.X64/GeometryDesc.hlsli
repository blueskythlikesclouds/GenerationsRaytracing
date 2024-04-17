#pragma once

#include "GeometryFlags.h"
#include "PackedPrimitives.hlsli"
#include "RayDifferentials.hlsli"
#include "SelfIntersectionAvoidance.hlsl"

struct GeometryDesc
{
    uint Flags : 12;
    uint IndexBufferId : 20;
    uint IndexOffset;

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
};

struct InstanceDesc
{
    row_major float3x4 PrevTransform;
    float PlayableParam;
    float ChrPlayableMenuParam;
    uint Padding1;
    uint Padding2;
};

#define VERTEX_FLAG_NONE                  (0 << 0)
#define VERTEX_FLAG_MIPMAP                (1 << 0)
#define VERTEX_FLAG_MULTI_UV              (1 << 1)
#define VERTEX_FLAG_SAFE_POSITION         (1 << 2)
#define VERTEX_FLAG_MIPMAP_LOD            (1 << 3)

struct Vertex
{
    uint Flags;
    float3 Position;
    float3 SafeSpawnPoint;
    float3 PrevPosition;
    float2 TexCoords[4];
    float2 TexCoordsDdx[4];
    float2 TexCoordsDdy[4];
    float3 Normal;
    float3 Tangent;
    float3 Binormal;
    float4 Color;
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
    indices.x = indexBuffer[geometryDesc.IndexOffset + PrimitiveIndex() * 3 + 0];
    indices.y = indexBuffer[geometryDesc.IndexOffset + PrimitiveIndex() * 3 + 1];
    indices.z = indexBuffer[geometryDesc.IndexOffset + PrimitiveIndex() * 3 + 2];

    uint3 offsets = geometryDesc.VertexOffset + indices * geometryDesc.VertexStride;
    uint3 normalOffsets = offsets + geometryDesc.NormalOffset;

    uint texCoordOffsets[4] = {
        geometryDesc.TexCoordOffset0, geometryDesc.TexCoordOffset1,
        geometryDesc.TexCoordOffset2, geometryDesc.TexCoordOffset3 };

    uint3 prevPositionOffsets = geometryDesc.VertexOffset +
        geometryDesc.VertexCount * geometryDesc.VertexStride + indices * 0xC;

    uint4 posColor0 = vertexBuffer.Load4(offsets.x);
    uint4 posColor1 = vertexBuffer.Load4(offsets.y);
    uint4 posColor2 = vertexBuffer.Load4(offsets.z);
    uint4 nrmTgnBinTex0 = vertexBuffer.Load4(normalOffsets.x);
    uint4 nrmTgnBinTex1 = vertexBuffer.Load4(normalOffsets.y);
    uint4 nrmTgnBinTex2 = vertexBuffer.Load4(normalOffsets.z);
    float3 position0 = asfloat(posColor0.xyz);
    float3 position1 = asfloat(posColor1.xyz);
    float3 position2 = asfloat(posColor2.xyz);
    float4 color0 = DecodeColor(posColor0.w);
    float4 color1 = DecodeColor(posColor1.w);
    float4 color2 = DecodeColor(posColor2.w);
    float3 normal0 = DecodeSnorm10(nrmTgnBinTex0.x);
    float3 normal1 = DecodeSnorm10(nrmTgnBinTex1.x);
    float3 normal2 = DecodeSnorm10(nrmTgnBinTex2.x);
    float3 tangent0 = DecodeSnorm10(nrmTgnBinTex0.y);
    float3 tangent1 = DecodeSnorm10(nrmTgnBinTex1.y);
    float3 tangent2 = DecodeSnorm10(nrmTgnBinTex2.y);
    float3 binormal0 = DecodeSnorm10(nrmTgnBinTex0.z);
    float3 binormal1 = DecodeSnorm10(nrmTgnBinTex1.z);
    float3 binormal2 = DecodeSnorm10(nrmTgnBinTex2.z);
    float2 texCoord0 = DecodeHalf2(nrmTgnBinTex0.w);
    float2 texCoord1 = DecodeHalf2(nrmTgnBinTex1.w);
    float2 texCoord2 = DecodeHalf2(nrmTgnBinTex2.w);

    Vertex vertex;

    vertex.Flags = flags;
    vertex.Position = position0 * uv.x + position1 * uv.y + position2 * uv.z;
    vertex.Color = color0 * uv.x + color1 * uv.y + color2 * uv.z;
    vertex.Normal = normal0 * uv.x + normal1 * uv.y + normal2 * uv.z;
    vertex.Tangent = tangent0 * uv.x + tangent1 * uv.y + tangent2 * uv.z;
    vertex.Binormal = binormal0 * uv.x + binormal1 * uv.y + binormal2 * uv.z;

    float2 texCoords[4][3];
    for (uint i = 0; i < 4; i++)
    {
        if (i == 0)
        {
            texCoords[i][0] = texCoord0;
            texCoords[i][1] = texCoord1;
            texCoords[i][2] = texCoord2;
        }
        else
        {
            for (uint j = 0; j < 3; j++)
                texCoords[i][j] = DecodeHalf2(vertexBuffer.Load(offsets[j] + texCoordOffsets[i]));
        }

        vertex.TexCoords[i] = texCoords[i][0] * uv.x + texCoords[i][1] * uv.y + texCoords[i][2] * uv.z;
    }

    if (geometryDesc.Flags & GEOMETRY_FLAG_POSE)
    {
        vertex.PrevPosition =
            vertexBuffer.Load<float3>(prevPositionOffsets.x) * uv.x +
            vertexBuffer.Load<float3>(prevPositionOffsets.y) * uv.y +
            vertexBuffer.Load<float3>(prevPositionOffsets.z) * uv.z;
    }
    else
    {
        vertex.PrevPosition = vertex.Position;
    }

    float3 outWldNormal = 0.0;
    if (flags & VERTEX_FLAG_SAFE_POSITION)
    {
        // Some models have correct normals but flipped triangle order
        // Could check for view direction but that makes plants look bad
        // as they have spherical normals
        bool isBackFace = dot(cross(position0 - position2, position0 - position1), vertex.Normal) > 0.0;

        float3 outObjPosition;
        float3 outWldPosition;
        float3 outObjNormal;
        float outWldOffset;
        safeSpawnPoint(
            outObjPosition, outWldPosition,
            outObjNormal, outWldNormal,
            outWldOffset,
            position0, isBackFace ? position2 : position1, isBackFace ? position1 : position2,
            isBackFace ? attributes.barycentrics.yx : attributes.barycentrics.xy,
            ObjectToWorld3x4(),
            WorldToObject3x4());

        vertex.SafeSpawnPoint = safeSpawnPoint(outWldPosition, outWldNormal, outWldOffset);
    }

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
            mul(ObjectToWorld3x4(), float4(position1 - position0, 0.0)).xyz,
            mul(ObjectToWorld3x4(), float4(position2 - position0, 0.0)).xyz,
            outWldNormal,
            dBarydx,
            dBarydy);

        [unroll]
        for (uint i = 0; i < 4; i++)
            InterpolateDifferentials(dBarydx, dBarydy, texCoords[i][0], texCoords[i][1], texCoords[i][2], vertex.TexCoordsDdx[i], vertex.TexCoordsDdy[i]);
    }

    vertex.Position = mul(ObjectToWorld3x4(), float4(vertex.Position, 1.0)).xyz;
    vertex.PrevPosition = mul(instanceDesc.PrevTransform, float4(vertex.PrevPosition, 1.0)).xyz;
    vertex.Normal = NormalizeSafe(mul(ObjectToWorld3x4(), float4(vertex.Normal, 0.0)).xyz);
    vertex.Tangent = NormalizeSafe(mul(ObjectToWorld3x4(), float4(vertex.Tangent, 0.0)).xyz);
    vertex.Binormal = NormalizeSafe(mul(ObjectToWorld3x4(), float4(vertex.Binormal, 0.0)).xyz);
    vertex.TexCoords[0] += texCoordOffset[0].xy;
    vertex.TexCoords[1] += texCoordOffset[0].zw;
    vertex.TexCoords[2] += texCoordOffset[1].xy;
    vertex.TexCoords[3] += texCoordOffset[1].zw;

    if (!(flags & VERTEX_FLAG_SAFE_POSITION))
        vertex.SafeSpawnPoint = vertex.Position + vertex.Normal * 0.001;

    return vertex;
}