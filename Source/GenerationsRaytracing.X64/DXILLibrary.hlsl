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

struct PathTraceParams
{
    bool TraceGlobalIllumination;
    bool TraceReflection;
    bool ShouldTraceReflection;
    float3 Position;
    float3 Direction;
    uint InstanceMask;
    uint MissShaderIndex;
    float3 Throughput;
    bool ApplyPower;
};

template<bool TCanBeWater>
PathTraceParams GetPathTraceParams(GBufferData gBufferData, float3 eyeDirection, inout uint randSeed)
{
    PathTraceParams params = (PathTraceParams) 0;
    
    params.TraceGlobalIllumination = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION));
    params.TraceReflection = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION));
        
    if (params.TraceGlobalIllumination || params.TraceReflection)
    {            
        float giProbability;
        if (params.TraceGlobalIllumination ^ params.TraceReflection)
        {
            giProbability = params.TraceGlobalIllumination ? 1.0 : 0.0;
        }
        else
        {
            float reflectionProbability;
                
            if (TCanBeWater && (gBufferData.Flags & GBUFFER_FLAG_IS_WATER))
                reflectionProbability = ComputeWaterFresnel(dot(gBufferData.Normal, eyeDirection));
            else
                reflectionProbability = dot(ComputeReflection(gBufferData, gBufferData.SpecularFresnel), float3(0.299, 0.587, 0.114));
                
            giProbability = saturate(1.0 - reflectionProbability);
        }
        
        params.ShouldTraceReflection = NextRandomFloat(randSeed) > giProbability;
        params.Position = gBufferData.Position;

        if (params.ShouldTraceReflection)
        {
            float3 specularFresnel = gBufferData.Flags & GBUFFER_FLAG_IS_METALLIC ?
                gBufferData.SpecularTint : gBufferData.SpecularFresnel;
                
            if (gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION)
            {
                params.Direction = reflect(-eyeDirection, gBufferData.Normal);
                params.MissShaderIndex = MISS_SECONDARY_GBUFFER;
                    
                float cosTheta = dot(gBufferData.Normal, eyeDirection);
                    
                if (TCanBeWater && (gBufferData.Flags & GBUFFER_FLAG_IS_WATER))
                    params.Throughput = ComputeWaterFresnel(cosTheta);
                else
                    params.Throughput = ComputeFresnel(specularFresnel, cosTheta);
                
                params.ApplyPower = false;
            }
            else
            {
                float4 sampleDirection = GetPowerCosWeightedSample(float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)), gBufferData.SpecularGloss);
                float3 halfwayDirection = TangentToWorld(gBufferData.Normal, sampleDirection.xyz);
                    
                params.Direction = reflect(-eyeDirection, halfwayDirection);
                params.MissShaderIndex = MISS_SECONDARY_GBUFFER_ENVIRONMENT_COLOR;
                params.Throughput = ComputeFresnel(specularFresnel, dot(halfwayDirection, eyeDirection)) *
                    pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularGloss) / (0.0001 + sampleDirection.w);
                
                params.ApplyPower = true;
            }
            
            params.InstanceMask = INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN | INSTANCE_MASK_INSTANCER;
            params.Throughput /= 1.0 - giProbability;
        }
        else
        {
            params.Direction = TangentToWorld(gBufferData.Normal, GetCosWeightedSample(float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed))));
            params.InstanceMask = INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN;
            params.MissShaderIndex = MISS_SECONDARY_GBUFFER_ENVIRONMENT_COLOR;
            params.Throughput = 1.0 / giProbability;
            params.ApplyPower = true;
        }
    }
    
    return params;
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
        
        PathTraceParams primaryParams = GetPathTraceParams<true>(gBufferData, normalize(-gBufferData.Position), randSeed);
        if (primaryParams.TraceGlobalIllumination || primaryParams.TraceReflection)
        {
            PathTraceParams secondaryParams = primaryParams;
            float3 radiance = 0.0;
            float hitDistance = 0.0;
        
            for (uint j = 0; j < 16; j++)
            {
                float3 environmentColorBalance = 1.0;
                if (secondaryParams.TraceGlobalIllumination && secondaryParams.TraceReflection)
                {
                    float3 gi = ComputeGI(gBufferData, 1.0);
                    float3 reflection = ComputeReflection(gBufferData, 1.0);
                    environmentColorBalance = gi / (gi + reflection + 0.0001);
                }
                
                RayDesc ray;
                ray.Origin = secondaryParams.Position;
                ray.Direction = secondaryParams.Direction;
                ray.TMin = 0.0;
                ray.TMax = INF;

                SecondaryGBufferRayPayload payload;

#ifdef NV_SHADER_EXTN_SLOT
                NvHitObject hitObject = NvTraceRayHitObject(
#else
                TraceRay(
#endif
                    g_BVH,
                    RAY_FLAG_NONE,
                    secondaryParams.InstanceMask,
                    HIT_GROUP_SECONDARY_GBUFFER,
                    HIT_GROUP_NUM,
                    secondaryParams.MissShaderIndex,
                    ray,
                    payload);

#ifdef NV_SHADER_EXTN_SLOT
                NvReorderThread(hitObject, 0, 0);
                NvInvokeHitObject(g_BVH, hitObject, payload);
#endif

                gBufferData = UnpackGBufferData(payload.PackedGBufferData);

                if (j == 0)
                    hitDistance = distance(secondaryParams.Position, gBufferData.Position);
            
                float3 color = 0.0;
                if (!(gBufferData.Flags & GBUFFER_FLAG_IS_SKY))
                {
                    float lightPower = 1.0;
                    if (secondaryParams.ApplyPower)
                    {
                        gBufferData.Diffuse *= g_DiffusePower;
                        gBufferData.Emission *= g_EmissivePower;
                        lightPower = g_LightPower;
                    }
                    
                    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT))
                    {
                        float3 directLighting = ComputeDirectLighting(gBufferData, -secondaryParams.Direction,
                            -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb);
        
                        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_SHADOW) && any(directLighting > 0.0001))
                        {
                            directLighting *= TraceShadow(gBufferData.Position,
                                -mrgGlobalLight_Direction.xyz, float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)), 0);
                        }
        
                        color += directLighting * lightPower;
                    }
                    
                    if (g_LocalLightCount > 0 && !(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT))
                    {
                        uint sample = min(uint(NextRandomFloat(randSeed) * g_LocalLightCount), g_LocalLightCount - 1);
                        LocalLight localLight = g_LocalLights[sample];
                        
                        float3 localLighting = ComputeLocalLighting(gBufferData, -secondaryParams.Direction, localLight);
                        
                        if ((localLight.Flags & LOCAL_LIGHT_FLAG_CAST_SHADOW) && any(localLighting > 0.0001))
                        {
                            float3 lightDirection = localLight.Position - gBufferData.Position;
                            float distance = length(lightDirection);

                            if (distance > 0.0)
                                lightDirection /= distance;
                            
                            localLighting *= TraceLocalLightShadow(
                                gBufferData.Position,
                                lightDirection,
                                float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)),
                                localLight.ShadowRange,
                                distance,
                                (localLight.Flags & LOCAL_LIGHT_FLAG_ENABLE_BACKFACE_CULLING) != 0);
                        }

                        color += localLighting * lightPower;
                    }
                }
                else
                {
                    if (secondaryParams.MissShaderIndex == MISS_SECONDARY_GBUFFER_ENVIRONMENT_COLOR && g_UseEnvironmentColor) 
                        gBufferData.Emission *= environmentColorBalance;
                }

                color += gBufferData.Emission;
                radiance += color * primaryParams.Throughput;
            
                if ((gBufferData.Flags & GBUFFER_FLAG_IS_SKY) != 0)
                    break;

                secondaryParams = GetPathTraceParams<false>(gBufferData, -secondaryParams.Direction, randSeed);
                if (!secondaryParams.TraceGlobalIllumination && !secondaryParams.TraceReflection)
                    break;
                
                float3 throughput;
                if (secondaryParams.ShouldTraceReflection)
                    throughput = ComputeReflection(gBufferData, secondaryParams.Throughput);
                else
                    throughput = ComputeGI(gBufferData, secondaryParams.Throughput);
                
                primaryParams.Throughput *= throughput;
                if (j >= 2)
                {
                    float russianRouletteProbability = max(primaryParams.Throughput.x, max(primaryParams.Throughput.y, primaryParams.Throughput.z));
                    if (NextRandomFloat(randSeed) > russianRouletteProbability) 
                        break;
                    
                    primaryParams.Throughput /= russianRouletteProbability;
                }
            }
            
            if (primaryParams.TraceGlobalIllumination)
                g_GlobalIllumination[dispatchRaysIndex] = primaryParams.ShouldTraceReflection ? 0.0 : float4(radiance, hitDistance);
        
            if (primaryParams.TraceReflection)
                g_Reflection[dispatchRaysIndex] = primaryParams.ShouldTraceReflection ? float4(radiance, hitDistance) : 0.0;
        }
    }
}

float3 SecondaryMiss()
{
    float3 color = 0.0;

    if (g_UseSkyTexture)
    {
        TextureCube skyTexture = ResourceDescriptorHeap[g_SkyTextureId];
        color = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb;
    }
    
    return color;
}

[shader("miss")]
void SecondaryMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    payload.Color = SecondaryMiss();
    payload.Diffuse = FLT_MAX;
}

[shader("miss")]
void SecondaryGBufferMiss(inout SecondaryGBufferRayPayload payload : SV_RayPayload)
{
    GBufferData gBufferData = (GBufferData) 0;
    gBufferData.Flags = GBUFFER_FLAG_IS_SKY;
    gBufferData.Emission = SecondaryMiss();
    payload.PackedGBufferData = PackGBufferData(gBufferData);
}

float3 SecondaryEnvironmentColorMiss()
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
    
    return color;
}

[shader("miss")]
void SecondaryEnvironmentColorMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    payload.Color = SecondaryEnvironmentColorMiss();
    payload.Diffuse = FLT_MAX;
}

[shader("miss")]
void SecondaryGBufferEnvironmentColorMiss(inout SecondaryGBufferRayPayload payload : SV_RayPayload)
{
    GBufferData gBufferData = (GBufferData) 0;
    gBufferData.Flags = GBUFFER_FLAG_IS_SKY;
    gBufferData.Emission = SecondaryEnvironmentColorMiss();
    payload.PackedGBufferData = PackGBufferData(gBufferData);
}

void SecondaryAnyHit(in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes, uint level)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    MaterialData materialData = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, materialData.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_NONE);
    
    if ((materialData.Flags & MATERIAL_FLAG_HAS_DIFFUSE_TEXTURE) &&
        SampleMaterialTexture2D(materialData.PackedData[0], vertex.TexCoords[0], level).a < 0.5)
    {
        IgnoreHit();
    }
}

[shader("anyhit")]
void SecondaryAnyHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    SecondaryAnyHit(attributes, 2);
}

[shader("anyhit")]
void SecondaryGBufferAnyHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    SecondaryAnyHit(attributes, 0);
}