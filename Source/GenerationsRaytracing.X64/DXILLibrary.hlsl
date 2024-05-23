#include "DXILLibrary.hlsli"
#include "LocalLight.h"

[shader("raygeneration")]
void PrimaryRayGeneration()
{
    float3 rayDirection = ComputePrimaryRayDirection(DispatchRaysIndex().xy, DispatchRaysDimensions().xy);

    RayDesc ray;
    ray.Origin = 0.0;
    ray.Direction = normalize(rayDirection);
    ray.TMin = 0.0;
    ray.TMax = INF;

    float3 dDdx;
    float3 dDdy;

    ComputeRayDifferentials(
        rayDirection,
        mul(float4(1.0, 0.0, 0.0, 0.0), g_MtxView).xyz / g_MtxProjection[0][0],
        mul(float4(0.0, 1.0, 0.0, 0.0), g_MtxView).xyz / g_MtxProjection[1][1],
        1.0 / g_ViewportSize.zw,
        dDdx,
        dDdy);

    PrimaryRayPayload payload;
    payload.dDdx = dDdx;
    payload.dDdy = dDdy;

    TraceRay(
        g_BVH,
        RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
        INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN | INSTANCE_MASK_INSTANCER,
        HIT_GROUP_PRIMARY,
        HIT_GROUP_NUM,
        MISS_PRIMARY,
        ray,
        payload);

    ray.TMax = payload.T;

    PrimaryTransparentRayPayload payload1;
    payload1.dDdx = dDdx;
    payload1.dDdy = dDdy;
    payload1.LayerCount = 1;

    TraceRay(
        g_BVHTransparent,
        RAY_FLAG_CULL_FRONT_FACING_TRIANGLES | RAY_FLAG_FORCE_NON_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN,
        HIT_GROUP_PRIMARY_TRANSPARENT,
        HIT_GROUP_NUM,
        MISS_PRIMARY_TRANSPARENT,
        ray,
        payload1);

    g_LayerNum[DispatchRaysIndex().xy] = payload1.LayerCount;
}

[shader("miss")]
void PrimaryMiss(inout PrimaryRayPayload payload : SV_RayPayload)
{
    TextureCube skyTexture = ResourceDescriptorHeap[g_SkyTextureId];

    GBufferData gBufferData = (GBufferData) 0;
    gBufferData.Position = WorldRayDirection() * g_CameraNearFarAspect.y;
    gBufferData.Flags = GBUFFER_FLAG_IS_SKY;
    gBufferData.SpecularGloss = 1.0;
    gBufferData.Normal = -WorldRayDirection();

    if (g_UseSkyTexture)
        gBufferData.Emission = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb;
    else
        gBufferData.Emission = g_BackgroundColor;

    StoreGBufferData(uint3(DispatchRaysIndex().xy, 0), gBufferData);

    g_Depth[DispatchRaysIndex().xy] = 1.0;

    g_MotionVectors[DispatchRaysIndex().xy] =
        ComputePixelPosition(gBufferData.Position, g_MtxPrevView, g_MtxPrevProjection) - (DispatchRaysIndex().xy - g_PixelJitter + 0.5);

    g_LinearDepth[DispatchRaysIndex().xy] = 65504.0;
    
    payload.T = INF;
}

[shader("miss")]
void PrimaryTransparentMiss(inout PrimaryTransparentRayPayload payload : SV_RayPayload)
{
    
}

struct TracePathArgs
{
    float3 Position;
    float3 Direction;
    uint InstanceMask;
    uint MissShaderIndex;
    float3 Throughput;
    bool ApplyPower;
};

float4 TracePath(TracePathArgs args, inout uint randSeed)
{
    float3 radiance = 0.0;
    float hitDistance = 0.0;

    [unroll]
    for (uint bounceIndex = 0; bounceIndex < 2; bounceIndex++)
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
            bounceIndex != 0 ? RAY_FLAG_CULL_NON_OPAQUE : RAY_FLAG_NONE,
            bounceIndex != 0 ? INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN : args.InstanceMask,
            HIT_GROUP_SECONDARY,
            HIT_GROUP_NUM,
            bounceIndex != 0 ? MISS_SECONDARY_ENVIRONMENT_COLOR : args.MissShaderIndex,
            ray,
            payload);

#ifdef NV_SHADER_EXTN_SLOT
        NvReorderThread(hitObject, 0, 0);
        NvInvokeHitObject(g_BVH, hitObject, payload);
#endif

        float3 color = payload.Color;
        float3 diffuse = payload.Diffuse;

        bool terminatePath = false;
        bool applyPower = bounceIndex != 0 || args.ApplyPower;

        if (any(diffuse != 0))
        {
            float lightPower = 1.0;

            if (applyPower)
            {
                color *= g_EmissivePower;
                diffuse *= g_DiffusePower;
                lightPower = g_LightPower;
            }

            float3 directLighting = diffuse * mrgGlobalLight_Diffuse.rgb * lightPower * saturate(dot(-mrgGlobalLight_Direction.xyz, payload.Normal));
            
            if (any(directLighting > 0.0001))
            {
                float2 random = float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed));
                
                if (bounceIndex != 0)
                    directLighting *= TraceShadowCullNonOpaque(payload.Position, -mrgGlobalLight_Direction.xyz, random);
                else
                    directLighting *= TraceShadow(payload.Position, -mrgGlobalLight_Direction.xyz, random, 2);
            }
            
            color += directLighting;

            if (g_LocalLightCount > 0)
            {
                uint sample = min(uint(NextRandomFloat(randSeed) * g_LocalLightCount), g_LocalLightCount - 1);
                LocalLight localLight = g_LocalLights[sample];

                float3 lightDirection = localLight.Position - payload.Position;
                float distance = length(lightDirection);

                if (distance > 0.0)
                    lightDirection /= distance;

                float3 localLighting = diffuse * localLight.Color * lightPower * g_LocalLightCount * saturate(dot(payload.Normal, lightDirection)) *
                    ComputeLocalLightFalloff(distance, localLight.InRange, localLight.OutRange);

                if ((localLight.Flags & LOCAL_LIGHT_FLAG_CAST_SHADOW) && any(localLighting > 0.0001))
                {
                    localLighting *= TraceLocalLightShadow(
                        payload.Position, 
                        lightDirection, 
                        float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)), 
                        localLight.ShadowRange, 
                        distance,
                        (localLight.Flags & LOCAL_LIGHT_FLAG_ENABLE_BACKFACE_CULLING) != 0);
                }

                color += localLighting;
            }
        }
        else
        {
            terminatePath = true;
            
            if (applyPower && !g_UseEnvironmentColor)
                color *= g_SkyPower;
        }

        if (bounceIndex == 0 && !terminatePath)
            hitDistance = distance(args.Position, payload.Position);

        radiance += args.Throughput * color;

        if (terminatePath)
            break;

        args.Position = payload.Position;
        args.Direction = TangentToWorld(payload.Normal, GetCosWeightedSample(float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed))));
        args.Throughput *= diffuse;
    }

    return float4(radiance, hitDistance);
}

[shader("raygeneration")]
void SecondaryRayGeneration()
{
    uint randSeed = InitRandom(DispatchRaysIndex().xy);
    uint layerNum = g_LayerNum_SRV[DispatchRaysIndex().xy];
    
    for (uint i = 0; i < layerNum; i++)
    {
        uint3 dispatchRaysIndex = uint3(DispatchRaysIndex().xy, i);
        GBufferData gBufferData = LoadGBufferData(dispatchRaysIndex);
        
        bool traceGlobalIllumination = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION));
        bool traceReflection = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION));
        
        if (traceGlobalIllumination || traceReflection)
        {
            float giProbability;
            if (traceGlobalIllumination ^ traceReflection)
                giProbability = traceGlobalIllumination ? 1.0 : 0.0;
            else
                giProbability = 1.0 - saturate(dot(ComputeReflection(gBufferData, 1.0), float3(0.299, 0.587, 0.114)));
        
            bool shouldTraceReflection = NextRandomFloat(randSeed) > giProbability;
        
            TracePathArgs args;
            args.Position = gBufferData.Position;
        
            if (shouldTraceReflection)
            {
                float3 eyeDirection = NormalizeSafe(-gBufferData.Position);
    
                if (gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION)
                {
                    args.Direction = reflect(-eyeDirection, gBufferData.Normal);
                    args.MissShaderIndex = MISS_SECONDARY;
                    args.Throughput = 1.0;
                    args.ApplyPower = false;
                }
                else
                {
                    float4 sampleDirection = GetPowerCosWeightedSample(float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)), gBufferData.SpecularGloss);
                    float3 halfwayDirection = TangentToWorld(gBufferData.Normal, sampleDirection.xyz);
    
                    args.Direction = reflect(-eyeDirection, halfwayDirection);
                    args.MissShaderIndex = MISS_SECONDARY_ENVIRONMENT_COLOR;
                    args.Throughput = pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularGloss) / (0.0001 + sampleDirection.w);
                    args.ApplyPower = true;
                }
            
                args.InstanceMask = INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN | INSTANCE_MASK_INSTANCER;
                args.Throughput /= 1.0 - giProbability;
            }
            else
            {
                args.Direction = TangentToWorld(gBufferData.Normal, GetCosWeightedSample(float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed))));
                args.InstanceMask = INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN;
                args.MissShaderIndex = MISS_SECONDARY_ENVIRONMENT_COLOR;
                args.Throughput = 1.0 / giProbability;
                args.ApplyPower = true;
            }
        
            float4 radianceAndHitDistance = TracePath(args, randSeed);
        
            if (traceGlobalIllumination)
                g_GlobalIllumination[dispatchRaysIndex] = shouldTraceReflection ? 0.0 : radianceAndHitDistance;
        
            if (traceReflection)
                g_Reflection[dispatchRaysIndex] = shouldTraceReflection ? radianceAndHitDistance : 0.0;
        }
    }
}

[shader("miss")]
void SecondaryMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    float3 color = 0.0;

    if (g_UseSkyTexture)
    {
        TextureCube skyTexture = ResourceDescriptorHeap[g_SkyTextureId];
        color = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb;
    }

    payload.Color = color;
    payload.Diffuse = 0.0;
}

[shader("miss")]
void SecondaryEnvironmentColorMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    float3 color = 0.0;

    if (g_UseEnvironmentColor)
    {
        color = WorldRayDirection().y > 0.0 ? g_SkyColor : g_GroundColor;
    }
    else if (g_UseSkyTexture)
    {
        TextureCube skyTexture = ResourceDescriptorHeap[g_SkyTextureId];
        color = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb;
    }

    payload.Color = color;
    payload.Diffuse = 0.0;
}

[shader("anyhit")]
void SecondaryAnyHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    MaterialData materialData = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, materialData.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_NONE);
    
    if ((materialData.Flags & MATERIAL_FLAG_HAS_DIFFUSE_TEXTURE) &&
        SampleMaterialTexture2D(materialData.PackedData[0], vertex.TexCoords[0], 2).a < 0.5)
    {
        IgnoreHit();
    }
}