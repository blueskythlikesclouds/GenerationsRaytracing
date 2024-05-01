#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"
#include "ThreadGroupTilingX.hlsl"

[numthreads(8, 8, 1)]
void main(uint2 groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    uint2 dispatchThreadId = ThreadGroupTilingX(
        (g_InternalResolution + 7) / 8,
        uint2(8, 8),
        8,
        groupThreadId,
        groupId);

    float3 color;
    Reservoir reservoir = (Reservoir) 0;
    float3 diffuseAlbedo = 0.0;
    float3 specularAlbedo = 0.0;
    float4 normalsRoughness = 0.0;
    float specularHitDistance = 0.0;

    GBufferData gBufferData = LoadGBufferData(uint3(dispatchThreadId, 0));
    if (!(gBufferData.Flags & GBUFFER_FLAG_IS_SKY))
    {
        float depth = g_Depth_SRV[dispatchThreadId];
        uint randSeed = InitRandom(dispatchThreadId.xy);
    
        ShadingParams shadingParams = (ShadingParams) 0;
    
        shadingParams.EyePosition = g_EyePosition.xyz;
        shadingParams.EyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);
    
        if (g_LocalLightCount > 0 && !(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT))
        {
            Reservoir prevReservoir = LoadReservoir(g_Reservoir_SRV[dispatchThreadId]);
    
            UpdateReservoir(reservoir, prevReservoir.Y, ComputeReservoirWeight(gBufferData, shadingParams.EyeDirection,
                g_LocalLights[prevReservoir.Y]) * prevReservoir.W * prevReservoir.M, prevReservoir.M, NextRandomFloat(randSeed));
    
            for (uint j = 0; j < 5; j++)
            {
                float radius = 30.0 * NextRandomFloat(randSeed);
                float angle = 2.0 * PI * NextRandomFloat(randSeed);
    
                int2 neighborIndex = round((float2) dispatchThreadId + float2(cos(angle), sin(angle)) * radius);
    
                if (all(and(neighborIndex >= 0, neighborIndex < g_InternalResolution)))
                {
                    GBufferData neighborGBufferData = LoadGBufferData(uint3(neighborIndex, 0));

                    if (!(neighborGBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT)) &&
                        abs(depth - g_Depth_SRV[neighborIndex]) <= 0.05 &&
                        dot(gBufferData.Normal, neighborGBufferData.Normal.xyz) >= 0.9063)
                    {
                        Reservoir neighborReservoir = LoadReservoir(g_Reservoir_SRV[neighborIndex]);

                        UpdateReservoir(reservoir, neighborReservoir.Y, ComputeReservoirWeight(gBufferData, shadingParams.EyeDirection,
                            g_LocalLights[neighborReservoir.Y]) * neighborReservoir.W * neighborReservoir.M, neighborReservoir.M, NextRandomFloat(randSeed));
                    }
                }
            }
    
            LocalLight localLight = g_LocalLights[reservoir.Y];
    
            ComputeReservoirWeight(reservoir,
                ComputeReservoirWeight(gBufferData, shadingParams.EyeDirection, localLight));
    
            if (reservoir.W > 0.0001)
            {
                float3 direction = localLight.Position - gBufferData.Position;
                float distance = length(direction);
                if (distance > 0.0)
                    direction /= distance;
    
                reservoir.W *= TraceLocalLightShadow(
                    gBufferData.Position,
                    direction,
                    float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)),
                    1.0 / localLight.OutRange,
                    distance);
            }
    
            shadingParams.Reservoir = reservoir;
        }
    
        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
        {
            shadingParams.GlobalIllumination = g_GlobalIllumination_SRV[uint3(dispatchThreadId, 0)].rgb;
            diffuseAlbedo = ComputeGI(gBufferData, 1.0);
        }
        
        diffuseAlbedo += gBufferData.Emission;
    
        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_REFLECTION))
        {
            float4 reflectionAndHitDistance = g_Reflection_SRV[uint3(dispatchThreadId, 0)];
            shadingParams.Reflection = reflectionAndHitDistance.rgb;
            specularAlbedo = ComputeReflection(gBufferData, 1.0);
            specularHitDistance = reflectionAndHitDistance.w;
        }
        
        normalsRoughness = float4(gBufferData.Normal, 0.0);
        if (!(gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION))
            normalsRoughness.w = ConvertSpecularGlossToRoughness(gBufferData.SpecularGloss);
    
        color = ComputeGeometryShading(gBufferData, shadingParams, randSeed);
        float3 viewPosition = mul(float4(gBufferData.Position, 1.0), g_MtxView).xyz;
        float2 lightScattering = ComputeLightScattering(gBufferData.Position, viewPosition);
    
        if (all(and(!isnan(lightScattering), !isinf(lightScattering))))
        {
            color = color * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
            diffuseAlbedo = diffuseAlbedo * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
            specularAlbedo = specularAlbedo * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
        }
    }
    else
    {
        color = gBufferData.Emission;
        diffuseAlbedo = gBufferData.Emission;
    }

    g_ColorBeforeTransparency[dispatchThreadId] = float4(color, 1.0);
    g_PrevReservoir[dispatchThreadId] = StoreReservoir(reservoir);
    g_DiffuseAlbedoBeforeTransparency[dispatchThreadId] = diffuseAlbedo;
    g_SpecularAlbedoBeforeTransparency[dispatchThreadId] = specularAlbedo;
    g_NormalsRoughness[dispatchThreadId] = normalsRoughness;
    g_SpecularHitDistance[dispatchThreadId] = specularHitDistance;

    if (dispatchThreadId.x == 0 && dispatchThreadId.y == 0)
        g_Exposure[dispatchThreadId] = GetExposure() * 2.0;
}