#pragma once

#include "GBufferData.h"
#include "GBufferData.hlsli"
#include "GeometryShading.hlsli"
#include "Reservoir.hlsli"
#include "ShaderTable.h"

struct [raypayload] PrimaryRayPayload
{
    float3 dDdx : read(closesthit) : write(caller);
    float3 dDdy : read(closesthit) : write(caller);
    float2 MotionVectors : read(caller) : write(closesthit, miss);
    float T : read(caller) : write(closesthit, miss);
};

void PrimaryAnyHit(uint shaderType,
    inout PrimaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    MaterialData materialData = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, materialData.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_NONE);
    GBufferData gBufferData = CreateGBufferData(vertex, GetMaterial(shaderType, materialData), shaderType, instanceDesc);

    if (gBufferData.Alpha < 0.5)
        IgnoreHit();
}

void PrimaryClosestHit(uint vertexFlags, uint shaderType,
    inout PrimaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    MaterialData materialData = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, materialData.TexCoordOffsets, instanceDesc, attributes, payload.dDdx, payload.dDdy, vertexFlags);

    GBufferData gBufferData = CreateGBufferData(vertex, GetMaterial(shaderType, materialData), shaderType, instanceDesc);
    gBufferData.Alpha = 1.0;
    StoreGBufferData(uint3(DispatchRaysIndex().xy, 0), gBufferData);

    g_Depth[DispatchRaysIndex().xy] = ComputeDepth(vertex.Position, g_MtxView, g_MtxProjection);

    payload.MotionVectors =
        ComputePixelPosition(vertex.PrevPosition, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(vertex.Position, g_MtxView, g_MtxProjection);

    payload.T = RayTCurrent();
}

struct [raypayload] PrimaryTransparentRayPayload
{
    float3 dDdx : read(anyhit) : write(caller);
    float3 dDdy : read(anyhit) : write(caller);
    float2 MotionVectors : read(caller, anyhit) : write(caller, anyhit);
    float T : read(caller, anyhit) : write(caller, anyhit);
    uint LayerIndex : read(caller, anyhit) : write(caller, anyhit);
};

void PrimaryTransparentAnyHit(uint vertexFlags, uint shaderType,
    inout PrimaryTransparentRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    MaterialData materialData = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, materialData.TexCoordOffsets, instanceDesc, attributes, payload.dDdx, payload.dDdy, vertexFlags);
    GBufferData gBufferData = CreateGBufferData(vertex, GetMaterial(shaderType, materialData), shaderType, instanceDesc);

    if (gBufferData.Alpha > 0.0)
    {
        StoreGBufferData(uint3(DispatchRaysIndex().xy, payload.LayerIndex), gBufferData);

        if (!(gBufferData.Flags & GBUFFER_FLAG_IS_ADDITIVE) && gBufferData.Alpha > 0.5 && RayTCurrent() < payload.T)
        {
            payload.MotionVectors =
                ComputePixelPosition(vertex.PrevPosition, g_MtxPrevView, g_MtxPrevProjection) -
                ComputePixelPosition(vertex.Position, g_MtxView, g_MtxProjection);
            
            payload.T = RayTCurrent();
        }

        ++payload.LayerIndex;
        if (payload.LayerIndex == GBUFFER_LAYER_NUM)
            AcceptHitAndEndSearch();
    }

    IgnoreHit();
}

struct [raypayload] SecondaryRayPayload
{
    float3 Color : read(caller) : write(closesthit, miss);
    float3 Diffuse : read(caller) : write(closesthit, miss);
    float3 Position : read(caller) : write(closesthit);
    float3 Normal : read(caller) : write(closesthit);
};

void SecondaryClosestHit(uint shaderType,
    inout SecondaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    MaterialData materialData = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, materialData.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_MIPMAP_LOD);

    GBufferData gBufferData = CreateGBufferData(vertex, GetMaterial(shaderType, materialData), shaderType, instanceDesc);

    payload.Color = gBufferData.Emission;
    payload.Diffuse = gBufferData.Diffuse;
    payload.Position = gBufferData.Position;
    payload.Normal = gBufferData.Normal;
}

