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
    uint RayGenerationFlags : read(caller) : write(closesthit, miss);
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
    
    uint rayGenerationFlags = 0;
    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_SHADOW) || !(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT))
        rayGenerationFlags |= 1u << (GBUFFER_LAYER_NUM * (RAY_GENERATION_SHADOW - 1));

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
        rayGenerationFlags |= 1u << (GBUFFER_LAYER_NUM * (RAY_GENERATION_GI - 1));

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_REFLECTION))
        rayGenerationFlags |= 1u << (GBUFFER_LAYER_NUM * (RAY_GENERATION_REFLECTION - 1));

    payload.RayGenerationFlags = rayGenerationFlags;
}

struct [raypayload] PrimaryTransparentRayPayload
{
    float3 dDdx : read(anyhit) : write(caller);
    float3 dDdy : read(anyhit) : write(caller);
    float2 MotionVectors : read(caller, anyhit) : write(caller, anyhit);
    float T : read(caller, anyhit) : write(caller, anyhit);
    uint LayerIndex : read(caller, anyhit) : write(caller, anyhit);
    uint RayGenerationFlags : read(caller, anyhit) : write(caller, anyhit);
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

        if (!(gBufferData.Flags & (GBUFFER_FLAG_REFRACTION_ADD | GBUFFER_FLAG_IS_ADDITIVE)) && gBufferData.Alpha > 0.5 && RayTCurrent() < payload.T)
        {
            payload.MotionVectors =
                ComputePixelPosition(vertex.PrevPosition, g_MtxPrevView, g_MtxPrevProjection) -
                ComputePixelPosition(vertex.Position, g_MtxView, g_MtxProjection);
            
            payload.T = RayTCurrent();
        }

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_SHADOW))
            payload.RayGenerationFlags |= 1u << (GBUFFER_LAYER_NUM * (RAY_GENERATION_SHADOW - 1) + payload.LayerIndex);

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
            payload.RayGenerationFlags |= 1u << (GBUFFER_LAYER_NUM * (RAY_GENERATION_GI - 1) + payload.LayerIndex);

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_REFLECTION))
            payload.RayGenerationFlags |= 1u << (GBUFFER_LAYER_NUM * (RAY_GENERATION_REFLECTION - 1) + payload.LayerIndex);

        ++payload.LayerIndex;
        if (payload.LayerIndex == GBUFFER_LAYER_NUM)
            AcceptHitAndEndSearch();
    }

    IgnoreHit();
}

float TraceShadow(float3 position, float3 direction, float2 random, RAY_FLAG rayFlag, uint level)
{
    float radius = sqrt(random.x) * 0.01;
    float angle = random.y * 2.0 * PI;

    float3 sample;
    sample.x = cos(angle) * radius;
    sample.y = sin(angle) * radius;
    sample.z = sqrt(1.0 - saturate(dot(sample.xy, sample.xy)));

    RayDesc ray;

    ray.Origin = position;
    ray.Direction = TangentToWorld(direction, sample);
    ray.TMin = 0.0;
    ray.TMax = INF;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;

    query.TraceRayInline(
        g_BVH,
        rayFlag,
        INSTANCE_MASK_OPAQUE,
        ray);

    if (rayFlag & RAY_FLAG_CULL_NON_OPAQUE)
    {
        query.Proceed();
    }
    else
    {
        while (query.Proceed())
        {
            if (query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
            {
                GeometryDesc geometryDesc = g_GeometryDescs[query.CandidateInstanceID() + query.CandidateGeometryIndex()];
                MaterialData material = g_Materials[geometryDesc.MaterialId];

                if (material.Textures[0] != 0)
                {
                    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.VertexBufferId)];
                    Buffer<uint> indexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.IndexBufferId)];
                
                    uint3 indices;
                    indices.x = indexBuffer[geometryDesc.IndexOffset + query.CandidatePrimitiveIndex() * 3 + 0];
                    indices.y = indexBuffer[geometryDesc.IndexOffset + query.CandidatePrimitiveIndex() * 3 + 1];
                    indices.z = indexBuffer[geometryDesc.IndexOffset + query.CandidatePrimitiveIndex() * 3 + 2];
                
                    uint3 offsets = geometryDesc.VertexOffset + indices * geometryDesc.VertexStride + geometryDesc.TexCoordOffset0;
                
                    float2 texCoord0 = DecodeHalf2(vertexBuffer.Load(offsets.x));
                    float2 texCoord1 = DecodeHalf2(vertexBuffer.Load(offsets.y));
                    float2 texCoord2 = DecodeHalf2(vertexBuffer.Load(offsets.z));
                
                    float3 uv = float3(
                        1.0 - query.CandidateTriangleBarycentrics().x - query.CandidateTriangleBarycentrics().y,
                        query.CandidateTriangleBarycentrics().x,
                        query.CandidateTriangleBarycentrics().y);
                
                    float2 texCoord = texCoord0 * uv.x + texCoord1 * uv.y + texCoord2 * uv.z;
                    texCoord += material.TexCoordOffsets[0].xy;
                
                    if (SampleMaterialTexture2D(material.Textures[0], texCoord, level).a > 0.5)
                        query.CommitNonOpaqueTriangleHit();
                }
            }
        }
    }

    return query.CommittedStatus() == COMMITTED_NOTHING ? 1.0 : 0.0;
}

struct [raypayload] SecondaryRayPayload
{
    float3 Color : read(caller) : write(closesthit, miss);
    float NormalX : read(caller) : write(closesthit);
    float3 Diffuse : read(caller) : write(closesthit, miss);
    float NormalY : read(caller) : write(closesthit);
    float3 Position : read(caller) : write(closesthit);
    float NormalZ : read(caller) : write(closesthit);
};

struct TracePathArgs
{
    float3 Position;
    float3 Direction;
    float4 Random;
    uint MissShaderIndex;
    uint2 DispatchRaysIndex;
    bool StoreHitDistance;
    float3 Throughput;
    bool ApplyPower;
};

float3 TracePath(TracePathArgs args)
{
    min16float3 halfRadiance = 0.0;
    min16float3 halfThroughput = (min16float3) args.Throughput;

    [unroll]
    for (uint i = 0; i < 2; i++)
    {
        RayDesc ray;
        ray.Origin = args.Position;
        ray.Direction = args.Direction;
        ray.TMin = 0.0;
        ray.TMax = INF;

        SecondaryRayPayload payload;

#ifdef NV_SHADER_EXTN_SLOT
        NvHitObject hitObject = NvTraceRayHitObject(
#else
        TraceRay(
#endif
            g_BVH,
            i > 0 ? RAY_FLAG_CULL_NON_OPAQUE : RAY_FLAG_NONE,
            INSTANCE_MASK_OPAQUE,
            HIT_GROUP_SECONDARY,
            HIT_GROUP_NUM,
            i == 0 ? args.MissShaderIndex : MISS_GI,
            ray,
            payload);

#ifdef NV_SHADER_EXTN_SLOT
        NvReorderThread(hitObject, 0, 0);
        NvInvokeHitObject(g_BVH, hitObject, payload);
#endif

        float3 color = payload.Color;
        float3 diffuse = payload.Diffuse;
        float3 normal = float3(payload.NormalX, payload.NormalY, payload.NormalZ);

        bool terminatePath = false;

        if (any(diffuse != 0))
        {
            float lightPower = 1.0;

            if (i > 0 || args.ApplyPower)
            {
                color *= g_EmissivePower;
                diffuse *= g_DiffusePower;
                lightPower = g_LightPower;
            }

            color += diffuse * mrgGlobalLight_Diffuse.rgb * lightPower * saturate(dot(-mrgGlobalLight_Direction.xyz, normal)) *
               TraceShadow(payload.Position, -mrgGlobalLight_Direction.xyz, i == 0 ? args.Random.xy : args.Random.zw, i > 0 ? RAY_FLAG_CULL_NON_OPAQUE : RAY_FLAG_NONE, 2);

            if (g_LocalLightCount > 0)
            {
                uint sample = min(floor((i == 0 ? args.Random.x : args.Random.z) * g_LocalLightCount), g_LocalLightCount - 1);
                LocalLight localLight = g_LocalLights[sample];

                float3 lightDirection = localLight.Position - payload.Position;
                float distance = length(lightDirection);

                if (distance > 0.0)
                    lightDirection /= distance;

                float3 localLighting = diffuse * localLight.Color * lightPower * g_LocalLightCount * saturate(dot(normal, lightDirection)) *
                    ComputeLocalLightFalloff(distance, localLight.InRange, localLight.OutRange);

                if (any(localLighting != 0))
                    localLighting *= TraceLocalLightShadow(payload.Position, lightDirection, i == 0 ? args.Random.xy : args.Random.zw, 1.0 / localLight.OutRange, distance);

                color += localLighting;
            }
        }
        else
        {
            terminatePath = true;
        }

        if (args.StoreHitDistance && i == 0)
            g_SpecularHitDistance[args.DispatchRaysIndex] = terminatePath ? 0.0 : distance(args.Position, payload.Position);

        float3 radiance = (float3) halfRadiance;
        float3 throughput = (float3) halfThroughput;

        radiance += throughput * color;
        throughput *= diffuse;

        halfRadiance = (min16float3) radiance;
        halfThroughput = (min16float3) throughput;

        if (terminatePath)
            break;

        args.Position = payload.Position;
        args.Direction = TangentToWorld(normal, GetCosWeightedSample(args.Random.zw));
    }

    return (float3) halfRadiance;
}

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
    payload.NormalX = gBufferData.Normal.x;
    payload.NormalY = gBufferData.Normal.y;
    payload.NormalZ = gBufferData.Normal.z;
}

