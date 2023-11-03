#include "GeometryShading.hlsli"
#include "RayTracing.hlsli"
#include "RootSignature.hlsli"

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float3 color;

    GBufferData gBufferData = LoadGBufferData(dispatchThreadId.xy);
    float depth = g_DepthTexture[dispatchThreadId.xy];

    Reservoir reservoir = (Reservoir)0;
    float3 diffuseAlbedo = 0.0;
    float3 specularAlbedo = 0.0;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IS_SKY))
    {
        uint random = InitRand(g_CurrentFrame, dispatchThreadId.y * g_InternalResolution.x + dispatchThreadId.x);

        reservoir = LoadReservoir(g_ReservoirTexture[dispatchThreadId.xy]);

        float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);

        for (uint i = 0; i < 5; i++)
        {
            float radius = 16.0 * NextRand(random);
            float angle = 2.0 * PI * NextRand(random);

            int2 neighborIndex = round((float2) dispatchThreadId.xy + float2(cos(angle), sin(angle)) * radius);

            if (all(and(neighborIndex >= 0, neighborIndex < g_InternalResolution)))
            {
                Reservoir spatialReservoir = LoadReservoir(g_ReservoirTexture[neighborIndex]);

                if (abs(depth - g_DepthTexture[neighborIndex]) <= 0.05 && dot(gBufferData.Normal, g_NormalTexture[neighborIndex].xyz) >= 0.9063)
                {
                    uint newSampleCount = reservoir.M + spatialReservoir.M;

                    UpdateReservoir(reservoir, spatialReservoir.Y, ComputeReservoirWeight(gBufferData, eyeDirection, 
                        g_LocalLights[spatialReservoir.Y]) * spatialReservoir.W * spatialReservoir.M, NextRand(random));

                    reservoir.M = newSampleCount;
                }
            }
        }

        ComputeReservoirWeight(reservoir, ComputeReservoirWeight(gBufferData, eyeDirection, g_LocalLights[reservoir.Y]));

        ShadingParams shadingParams;

        shadingParams.EyePosition = g_EyePosition.xyz;
        shadingParams.EyeDirection = eyeDirection;
        shadingParams.Shadow = g_ShadowTexture[dispatchThreadId.xy];

        if (g_LocalLightCount > 0)
            shadingParams.Reservoir = reservoir;
        else
            shadingParams.Reservoir = (Reservoir) 0;

        shadingParams.GlobalIllumination = g_GITexture[dispatchThreadId.xy];
        shadingParams.Reflection = g_ReflectionTexture[dispatchThreadId.xy];
        shadingParams.Refraction = g_RefractionTexture[dispatchThreadId.xy];

        if (gBufferData.Flags & GBUFFER_FLAG_IS_WATER)
            color = ComputeWaterShading(gBufferData, shadingParams);
        else
            color = ComputeGeometryShading(gBufferData, shadingParams);

        float3 viewPosition = mul(float4(gBufferData.Position, 1.0), g_MtxView).xyz;
        float2 lightScattering = ComputeLightScattering(gBufferData.Position, viewPosition);

        if (all(and(!isnan(lightScattering), !isinf(lightScattering))))
            color = color * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
            diffuseAlbedo = ComputeGI(gBufferData, 1.0);

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_REFLECTION))
            specularAlbedo = ComputeReflection(gBufferData, 1.0);
    }
    else
    {
        color = g_EmissionTexture[dispatchThreadId.xy];
    }

    g_ColorTexture[dispatchThreadId.xy] = float4(color, 1.0);
    g_PrevReservoirTexture[dispatchThreadId.xy] = StoreReservoir(reservoir);
    g_DiffuseAlbedoTexture[dispatchThreadId.xy] = diffuseAlbedo;
    g_SpecularAlbedoTexture[dispatchThreadId.xy] = specularAlbedo;
}