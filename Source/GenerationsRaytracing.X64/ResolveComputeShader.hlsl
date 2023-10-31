#include "GeometryShading.hlsli"
#include "RayTracing.hlsli"
#include "RootSignature.hlsli"

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float3 color;

    GBufferData gBufferData = LoadGBufferData(dispatchThreadId.xy);
    float depth = g_DepthTexture[dispatchThreadId.xy];

    Reservoir<uint> diReservoir = (Reservoir<uint>)0;
    float3 diffuseAlbedo = 0.0;
    float3 specularAlbedo = 0.0;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IS_SKY))
    {
        uint random = InitRand(g_CurrentFrame, dispatchThreadId.y * g_InternalResolution.x + dispatchThreadId.x);

        diReservoir = LoadDIReservoir(g_DIReservoirTexture[dispatchThreadId.xy]);

        float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);

        for (uint i = 0; i < 5; i++)
        {
            float radius = 16.0 * NextRand(random);
            float angle = 2.0 * PI * NextRand(random);

            int2 neighborIndex = round((float2) dispatchThreadId.xy + float2(cos(angle), sin(angle)) * radius);

            if (all(and(neighborIndex >= 0, neighborIndex < g_InternalResolution)))
            {
                Reservoir<uint> spatialDIReservoir = LoadDIReservoir(g_DIReservoirTexture[neighborIndex]);

                float3 position = g_PositionFlagsTexture[neighborIndex].xyz;
                float3 normal = g_NormalTexture[neighborIndex].xyz;

                if (abs(depth - g_DepthTexture[neighborIndex]) <= 0.05 && dot(gBufferData.Normal, normal) >= 0.9063)
                {
                    uint newSampleCount = diReservoir.SampleCount + spatialDIReservoir.SampleCount;

                    UpdateReservoir(diReservoir, spatialDIReservoir.Sample, ComputeDIReservoirWeight(gBufferData, eyeDirection, 
                        g_LocalLights[spatialDIReservoir.Sample]) * spatialDIReservoir.Weight * spatialDIReservoir.SampleCount, NextRand(random));

                    diReservoir.SampleCount = newSampleCount;
                }
            }
        }
        ComputeReservoirWeight(diReservoir, ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[diReservoir.Sample]));

        ShadingParams shadingParams;

        shadingParams.EyePosition = g_EyePosition.xyz;
        shadingParams.EyeDirection = eyeDirection;
        shadingParams.Shadow = g_ShadowTexture[dispatchThreadId.xy];

        if (g_LocalLightCount > 0)
            shadingParams.DIReservoir = diReservoir;
        else
            shadingParams.DIReservoir = (Reservoir<uint>) 0;

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

        diffuseAlbedo = ComputeGI(gBufferData, 1.0);
        specularAlbedo = ComputeReflection(gBufferData, 1.0);
    }
    else
    {
        color = g_EmissionTexture[dispatchThreadId.xy];
    }

    g_ColorTexture[dispatchThreadId.xy] = float4(color, 1.0);
    g_PrevDIReservoirTexture[dispatchThreadId.xy] = StoreDIReservoir(diReservoir);
    g_DiffuseAlbedoTexture[dispatchThreadId.xy] = diffuseAlbedo;
    g_SpecularAlbedoTexture[dispatchThreadId.xy] = specularAlbedo;
}