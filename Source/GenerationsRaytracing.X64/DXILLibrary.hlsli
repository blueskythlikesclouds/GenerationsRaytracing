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

    g_MotionVectors[DispatchRaysIndex().xy] =
        ComputePixelPosition(vertex.PrevPosition, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(vertex.Position, g_MtxView, g_MtxProjection);

    g_LinearDepth[DispatchRaysIndex().xy] = -mul(float4(vertex.Position, 0.0), g_MtxView).z;
    
    payload.T = RayTCurrent();
}

struct [raypayload] PrimaryTransparentRayPayload
{
    float3 dDdx : read(anyhit) : write(caller);
    float3 dDdy : read(anyhit) : write(caller);
    float T[GBUFFER_LAYER_NUM] : read(anyhit) : write(anyhit);
    uint LayerCount : read(caller, anyhit) : write(caller, anyhit);
};

void PrimaryTransparentAnyHit(uint vertexFlags, uint shaderType,
    inout PrimaryTransparentRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    bool isXluObject = !(geometryDesc.Flags & GEOMETRY_FLAG_TRANSPARENT);
    
    // Ignore duplicate any hit invocations or XLU objects that fail the depth test
    for (uint i = 1; i < payload.LayerCount; i++)
    {
        float layerDepth = abs(payload.T[i]);
        bool layerIsXluObject = payload.T[i] < 0.0;
        
        if (RayTCurrent() == layerDepth || (isXluObject && layerIsXluObject && RayTCurrent() > layerDepth))
            IgnoreHit();
    }
    
    MaterialData materialData = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescsTransparent[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, materialData.TexCoordOffsets, instanceDesc, attributes, payload.dDdx, payload.dDdy, vertexFlags);
    
    GBufferData gBufferData = CreateGBufferData(vertex, GetMaterial(shaderType, materialData), shaderType, instanceDesc);
    gBufferData.Alpha *= instanceDesc.ForceAlphaColor;
            
    if (gBufferData.Alpha > 0.0)
    {
        uint layerIndex = 1;
        
        while (layerIndex < payload.LayerCount)
        {
            float layerDepth = abs(payload.T[layerIndex]);
            bool layerIsXluObject = payload.T[layerIndex] < 0.0;
            
            if (isXluObject && layerIsXluObject)
                break;
            
            if (RayTCurrent() > layerDepth)
            {
                payload.LayerCount = min(GBUFFER_LAYER_NUM, payload.LayerCount + 1);
                
                for (uint i = payload.LayerCount - 1; i > layerIndex; i--)
                {
                    uint3 srcIndex = uint3(DispatchRaysIndex().xy, i - 1);
                    uint3 destIndex = uint3(DispatchRaysIndex().xy, i);
                
                    g_GBuffer0[destIndex] = g_GBuffer0[srcIndex];
                    g_GBuffer1[destIndex] = g_GBuffer1[srcIndex];
                    g_GBuffer2[destIndex] = g_GBuffer2[srcIndex];
                    g_GBuffer3[destIndex] = g_GBuffer3[srcIndex];
                    g_GBuffer4[destIndex] = g_GBuffer4[srcIndex];
                    g_GBuffer5[destIndex] = g_GBuffer5[srcIndex];
                    g_GBuffer6[destIndex] = g_GBuffer6[srcIndex];
                    g_GBuffer7[destIndex] = g_GBuffer7[srcIndex];
                    g_GBuffer8[destIndex] = g_GBuffer8[srcIndex];
                
                    payload.T[i] = payload.T[i - 1];
                }
                
                break;
            }
            
            ++layerIndex;
        }
        
        if (layerIndex < GBUFFER_LAYER_NUM)
        {
            StoreGBufferData(uint3(DispatchRaysIndex().xy, layerIndex), gBufferData);
            payload.T[layerIndex] = isXluObject ? -RayTCurrent() : RayTCurrent();
            payload.LayerCount = max(payload.LayerCount, layerIndex + 1);
            
            bool canWriteMotionVectorsAndDepth = !(gBufferData.Flags & (
                GBUFFER_FLAG_IS_ADDITIVE |
                GBUFFER_FLAG_REFRACTION_ADD |
                GBUFFER_FLAG_REFRACTION_MUL |
                GBUFFER_FLAG_REFRACTION_OVERLAY));
            
            if (canWriteMotionVectorsAndDepth && gBufferData.Alpha > 0.5)
            {
                float linearDepth = -mul(float4(vertex.Position, 0.0), g_MtxView).z;
                
                if (linearDepth < g_LinearDepth[DispatchRaysIndex().xy])
                {
                    g_MotionVectors[DispatchRaysIndex().xy] =
                        ComputePixelPosition(vertex.PrevPosition, g_MtxPrevView, g_MtxPrevProjection) -
                        ComputePixelPosition(vertex.Position, g_MtxView, g_MtxProjection);
                    
                    g_LinearDepth[DispatchRaysIndex().xy] = linearDepth;
                }
            }
        }                
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

