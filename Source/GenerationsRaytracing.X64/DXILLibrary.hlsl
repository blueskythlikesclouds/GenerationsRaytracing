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

void SecondaryRayGeneration()
{
    uint randSeed = InitRandom(DispatchRaysIndex().xy);
    
    NrcBuffers buffers;
    buffers.queryPathInfo = g_QueryPathInfo;
    buffers.trainingPathInfo = g_TrainingPathInfo;
    buffers.trainingPathVertices = g_TrainingPathVertices;
    buffers.queryRadianceParams = g_QueryRadianceParams;
    buffers.countersData = g_Counter;
    
    NrcContext context = NrcCreateContext(g_NrcConstants, buffers, DispatchRaysIndex().xy);
    NrcPathState pathState = NrcCreatePathState(g_NrcConstants, NextRandomFloat(randSeed));
    
    uint3 dispatchRaysIndex = uint3(DispatchRaysIndex().xy, 0);
    if (NrcIsUpdateMode())
        dispatchRaysIndex.xy = (dispatchRaysIndex.xy + 0.5) / DispatchRaysDimensions().xy * g_InternalResolution;
    
    GBufferData gBufferData = LoadGBufferData(dispatchRaysIndex);
    
    bool traceGlobalIllumination = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION));
    bool traceReflection = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION));

    if (traceGlobalIllumination || traceReflection)
    {
        float3 eyeDirection = NormalizeSafe(-gBufferData.Position);
        
        float giProbability;
        if (traceGlobalIllumination ^ traceReflection)
        {
            giProbability = traceGlobalIllumination ? 1.0 : 0.0;
        }
        else
        {
            float reflectionProbability;
                
            if (gBufferData.Flags & GBUFFER_FLAG_IS_WATER)
                reflectionProbability = ComputeWaterFresnel(dot(gBufferData.Normal, eyeDirection));
            else
                reflectionProbability = dot(ComputeReflection(gBufferData, gBufferData.SpecularFresnel), float3(0.299, 0.587, 0.114));
                
            giProbability = saturate(1.0 - reflectionProbability);
        }
        
        bool shouldTraceReflection = NextRandomFloat(randSeed) > giProbability;
        
        float3 direction;
        uint missShaderIndex;
        float3 throughput;
        bool applyPower;
        uint instanceMask;
        float pdf;
        
        if (shouldTraceReflection)
        {
            float3 specularFresnel = gBufferData.Flags & GBUFFER_FLAG_IS_METALLIC ?
                gBufferData.SpecularTint : gBufferData.SpecularFresnel;
                
            if (gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION)
            {
                direction = reflect(-eyeDirection, gBufferData.Normal);
                missShaderIndex = MISS_SECONDARY;
                    
                float cosTheta = dot(gBufferData.Normal, eyeDirection);
                    
                if (gBufferData.Flags & GBUFFER_FLAG_IS_WATER)
                    throughput = ComputeWaterFresnel(cosTheta);
                else
                    throughput = ComputeFresnel(specularFresnel, cosTheta);
                    
                applyPower = false;
                pdf = 1.0;
            }
            else
            {
                float4 sampleDirection = GetPowerCosWeightedSample(float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)), gBufferData.SpecularGloss);
                float3 halfwayDirection = TangentToWorld(gBufferData.Normal, sampleDirection.xyz);
                    
                direction = reflect(-eyeDirection, halfwayDirection);
                missShaderIndex = MISS_SECONDARY_ENVIRONMENT_COLOR;
                pdf = sampleDirection.w;
                throughput = ComputeFresnel(specularFresnel, dot(halfwayDirection, eyeDirection)) *
                        pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularGloss) / (0.0001 + sampleDirection.w);
                    
                applyPower = true;
            }
            
            instanceMask = INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN | INSTANCE_MASK_INSTANCER;
            throughput /= 1.0 - giProbability;
        }
        else
        {
            direction = TangentToWorld(gBufferData.Normal, GetCosWeightedSample(float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed))));
            instanceMask = INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN;
            missShaderIndex = MISS_SECONDARY_ENVIRONMENT_COLOR;
            throughput = 1.0 / giProbability;
            pdf = dot(gBufferData.Normal, direction) / PI;
            applyPower = true;
        }
        
        NrcSetBrdfPdf(pathState, pdf);
        
        float3 radiance = 0.0;
        float3 color = gBufferData.Emission;
        float3 diffuse = gBufferData.Diffuse;
        float3 position = gBufferData.Position;
        float3 normal = gBufferData.Normal;
        float hitDistance = 0.0;
    
        for (int bounceIndex = 0; bounceIndex < 8; ++bounceIndex)
        {
            RayDesc ray;
            ray.Origin = position;
            ray.Direction = direction;
            ray.TMin = 0.0;
            ray.TMax = INF;
        
            SecondaryRayPayload payload;
        
            TraceRay(
                g_BVH,
                bounceIndex != 0 ? RAY_FLAG_CULL_NON_OPAQUE : RAY_FLAG_NONE,
                bounceIndex != 0 ? INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN : instanceMask,
                HIT_GROUP_SECONDARY,
                HIT_GROUP_NUM,
                bounceIndex != 0 ? MISS_SECONDARY_ENVIRONMENT_COLOR : missShaderIndex,
                ray,
                payload);
        
            color = payload.Color;
            diffuse = payload.Diffuse;
            position = payload.Position;
            normal = payload.Normal;
        
            if (any(diffuse == FLT_MAX))
            {
                NrcUpdateOnMiss(pathState);
                radiance += payload.Color * throughput;
                break;
            }
            
            if (bounceIndex == 0)
                hitDistance = distance(ray.Origin, position);

            NrcSurfaceAttributes surfaceAttributes = (NrcSurfaceAttributes) 0;
            surfaceAttributes.encodedPosition = NrcEncodePosition(g_EyePosition.xyz + position, g_NrcConstants);
            surfaceAttributes.diffuseReflectance = diffuse;
            surfaceAttributes.shadingNormal = normal;
            surfaceAttributes.viewVector = -ray.Direction;

            NrcProgressState progressState = NrcUpdateOnHit(context, pathState, surfaceAttributes, distance(ray.Origin, position), bounceIndex, throughput, radiance);
            if (progressState == NrcProgressState::TerminateImmediately)
                break;
        
            float lightPower = 1.0;
            
            if (bounceIndex != 0 || applyPower)
            {
                color *= g_EmissivePower;
                diffuse *= g_DiffusePower;
                lightPower = g_LightPower;
            }
            
            float3 directLighting = diffuse * mrgGlobalLight_Diffuse.rgb * lightPower * saturate(dot(-mrgGlobalLight_Direction.xyz, normal));
            
            if (any(directLighting > 0.0001))
            {
                float2 random = float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed));
                
                if (bounceIndex != 0)
                    directLighting *= TraceShadowCullNonOpaque(position, -mrgGlobalLight_Direction.xyz, random);
                else
                    directLighting *= TraceShadow(position, -mrgGlobalLight_Direction.xyz, random, 2);
            }
            
            color += directLighting;

            if (g_LocalLightCount > 0)
            {
                uint sample = min(uint(NextRandomFloat(randSeed) * g_LocalLightCount), g_LocalLightCount - 1);
                LocalLight localLight = g_LocalLights[sample];

                float3 lightDirection = localLight.Position - position;
                float distance = length(lightDirection);

                if (distance > 0.0)
                    lightDirection /= distance;

                float3 localLighting = diffuse * localLight.Color * lightPower * g_LocalLightCount * saturate(dot(normal, lightDirection)) *
                ComputeLocalLightFalloff(distance, localLight.InRange, localLight.OutRange);

                if ((localLight.Flags & LOCAL_LIGHT_FLAG_CAST_SHADOW) && any(localLighting > 0.0001))
                {
                    localLighting *= TraceLocalLightShadow(
                    position,
                    lightDirection,
                    float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)),
                    localLight.ShadowRange,
                    distance,
                    (localLight.Flags & LOCAL_LIGHT_FLAG_ENABLE_BACKFACE_CULLING) != 0);
                }

                color += localLighting;
            }
        
            radiance += color * throughput;
        
            if (progressState == NrcProgressState::TerminateAfterDirectLighting)
                break;
        
            if (NrcCanUseRussianRoulette(pathState) && bounceIndex >= 2)
            {
                float probability = min(0.95f, dot(throughput, float3(0.299, 0.587, 0.114)));
                if (probability < NextRandomFloat(randSeed))
                    break;
            
                throughput /= probability;
            }
        
            direction = TangentToWorld(normal, GetCosWeightedSample(float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed))));
            throughput *= diffuse;
            
            NrcSetBrdfPdf(pathState, dot(normal, direction) / PI);
        }
    
        NrcWriteFinalPathInfo(context, pathState, throughput, radiance);
    
        if (traceGlobalIllumination)
            g_GlobalIllumination[dispatchRaysIndex] = shouldTraceReflection ? 0.0 : float4(radiance, hitDistance);
        
        if (traceReflection)
            g_Reflection[dispatchRaysIndex] = shouldTraceReflection ? float4(radiance, hitDistance) : 0.0;
    }
}

[shader("raygeneration")]
void QueryRayGeneration()
{
    g_nrcMode = NrcMode::Query;
    SecondaryRayGeneration();
}

[shader("raygeneration")]
void UpdateRayGeneration()
{
    g_nrcMode = NrcMode::Update;
    SecondaryRayGeneration();
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
    payload.Diffuse = FLT_MAX;
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
    payload.Diffuse = FLT_MAX;
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