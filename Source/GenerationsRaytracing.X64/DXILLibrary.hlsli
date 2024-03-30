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
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_NONE);
    GBufferData gBufferData = CreateGBufferData(vertex, material, shaderType);

    if (gBufferData.Alpha < 0.5)
        IgnoreHit();
}

void PrimaryClosestHit(uint vertexFlags, uint shaderType,
    inout PrimaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, payload.dDdx, payload.dDdy, vertexFlags);

    GBufferData gBufferData = CreateGBufferData(vertex, material, shaderType);
    gBufferData.Alpha = 1.0;
    StoreGBufferData(uint3(DispatchRaysIndex().xy, 0), gBufferData);

    g_Depth[DispatchRaysIndex().xy] = ComputeDepth(vertex.Position, g_MtxView, g_MtxProjection);

    g_MotionVectors[DispatchRaysIndex().xy] =
        ComputePixelPosition(vertex.PrevPosition, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(vertex.Position, g_MtxView, g_MtxProjection);

    g_LinearDepth[DispatchRaysIndex().xy] = -mul(float4(vertex.Position, 1.0), g_MtxView).z;

    payload.T = RayTCurrent();
}

struct [raypayload] PrimaryTransparentRayPayload
{
    float3 dDdx : read(anyhit) : write(caller);
    float3 dDdy : read(anyhit) : write(caller);
    uint LayerIndex : read(anyhit, caller) : write(caller, anyhit);
};

void PrimaryTransparentAnyHit(uint vertexFlags, uint shaderType,
    inout PrimaryTransparentRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, payload.dDdx, payload.dDdy, vertexFlags);
    GBufferData gBufferData = CreateGBufferData(vertex, material, shaderType);

    if (gBufferData.Alpha > 0.0)
    {
        StoreGBufferData(uint3(DispatchRaysIndex().xy, payload.LayerIndex), gBufferData);

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
                Material material = g_Materials[geometryDesc.MaterialId];

                if (material.DiffuseTexture != 0)
                {
                    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.VertexBufferId)];
                    Buffer<uint> indexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.IndexBufferId)];
                
                    uint3 indices;
                    indices.x = indexBuffer[query.CandidatePrimitiveIndex() * 3 + 0];
                    indices.y = indexBuffer[query.CandidatePrimitiveIndex() * 3 + 1];
                    indices.z = indexBuffer[query.CandidatePrimitiveIndex() * 3 + 2];
                
                    uint3 offsets = geometryDesc.VertexOffset + indices * geometryDesc.VertexStride + geometryDesc.TexCoordOffset0;
                
                    float2 texCoord0 = DecodeFloat2(vertexBuffer.Load(offsets.x));
                    float2 texCoord1 = DecodeFloat2(vertexBuffer.Load(offsets.y));
                    float2 texCoord2 = DecodeFloat2(vertexBuffer.Load(offsets.z));
                
                    float3 uv = float3(
                        1.0 - query.CandidateTriangleBarycentrics().x - query.CandidateTriangleBarycentrics().y,
                        query.CandidateTriangleBarycentrics().x,
                        query.CandidateTriangleBarycentrics().y);
                
                    float2 texCoord = texCoord0 * uv.x + texCoord1 * uv.y + texCoord2 * uv.z;
                    texCoord += material.TexCoordOffsets[0].xy;
                
                    if (SampleMaterialTexture2D(material.DiffuseTexture, texCoord, level).a > 0.5)
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

float3 TracePath(float3 position, float3 direction, uint missShaderIndex, bool storeHitDistance)
{
    float3 radiance = 0.0;
    float3 throughput = 1.0;
    float4 random = GetBlueNoise();

    [unroll]
    for (uint i = 0; i < 2; i++)
    {
        RayDesc ray;
        ray.Origin = position;
        ray.Direction = direction;
        ray.TMin = 0.0;
        ray.TMax = INF;

        SecondaryRayPayload payload;

        TraceRay(
            g_BVH,
            i > 0 ? RAY_FLAG_CULL_NON_OPAQUE : RAY_FLAG_NONE,
            INSTANCE_MASK_OPAQUE,
            HIT_GROUP_SECONDARY,
            HIT_GROUP_NUM,
            i == 0 ? missShaderIndex : MISS_GI,
            ray,
            payload);

        float3 color = payload.Color;
        float3 normal = float3(payload.NormalX, payload.NormalY, payload.NormalZ);

        bool terminatePath = false;

        if (any(payload.Diffuse != 0))
        {
            color += payload.Diffuse * mrgGlobalLight_Diffuse.rgb * g_LightPower * saturate(dot(-mrgGlobalLight_Direction.xyz, normal)) *
               TraceShadow(payload.Position, -mrgGlobalLight_Direction.xyz, i == 0 ? random.xy : random.zw, i > 0 ? RAY_FLAG_CULL_NON_OPAQUE : RAY_FLAG_NONE, 2);

            if (g_LocalLightCount > 0)
            {
                uint sample = min(floor((i == 0 ? random.x : random.z) * g_LocalLightCount), g_LocalLightCount - 1);
                LocalLight localLight = g_LocalLights[sample];

                float3 lightDirection = localLight.Position - payload.Position;
                float distance = length(lightDirection);

                if (distance > 0.0)
                    lightDirection /= distance;

                float3 localLighting = payload.Diffuse * localLight.Color * g_LightPower * g_LocalLightCount * saturate(dot(normal, lightDirection)) *
                    ComputeLocalLightFalloff(distance, localLight.InRange, localLight.OutRange);

                if (any(localLighting != 0))
                    localLighting *= TraceLocalLightShadow(payload.Position, lightDirection, i == 0 ? random.xy : random.zw, 1.0 / localLight.OutRange, distance);

                color += localLighting;
            }
        }
        else
        {
            terminatePath = true;
        }

        if (storeHitDistance && i == 0)
            g_SpecularHitDistance[DispatchRaysIndex().xy] = terminatePath ? 0.0 : distance(position, payload.Position);

        radiance += throughput * color;

        if (terminatePath)
            break;

        position = payload.Position;
        direction = TangentToWorld(normal, GetCosWeightedSample(random.zw));
        throughput *= payload.Diffuse;
    }

    return radiance;
}

void SecondaryClosestHit(uint shaderType,
    inout SecondaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_MIPMAP_LOD);

    GBufferData gBufferData = CreateGBufferData(vertex, material, shaderType);

    payload.Color = gBufferData.Emission * g_EmissivePower;
    payload.Diffuse = gBufferData.Diffuse * g_DiffusePower;
    payload.Position = gBufferData.Position;
    payload.NormalX = gBufferData.Normal.x;
    payload.NormalY = gBufferData.Normal.y;
    payload.NormalZ = gBufferData.Normal.z;
}

