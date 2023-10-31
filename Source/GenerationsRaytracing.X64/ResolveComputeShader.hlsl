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
    Reservoir<GISample> giReservoir = (Reservoir<GISample>)0;
    float3 globalIllumination = 0.0;
    float3 diffuseAlbedo = 0.0;
    float3 specularAlbedo = 0.0;
    float3 rayDirection = 0.0;
    float hitDistance = 0.0;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IS_SKY))
    {
        uint random = InitRand(g_CurrentFrame, dispatchThreadId.y * g_InternalResolution.x + dispatchThreadId.x);

        diReservoir = LoadDIReservoir(g_DIReservoirTexture[dispatchThreadId.xy]);
        giReservoir = LoadGIReservoir(g_GIColorTexture, g_GIPositionTexture, g_GINormalTexture, dispatchThreadId.xy);

        float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);

        for (uint i = 0; i < 5; i++)
        {
            float radius = 16.0 * NextRand(random);
            float angle = 2.0 * PI * NextRand(random);

            int2 neighborIndex = round((float2) dispatchThreadId.xy + float2(cos(angle), sin(angle)) * radius);

            if (all(and(neighborIndex >= 0, neighborIndex < g_InternalResolution)))
            {
                Reservoir<uint> spatialDIReservoir = LoadDIReservoir(g_DIReservoirTexture[neighborIndex]);

                Reservoir<GISample> spatialGIReservoir = LoadGIReservoir(
                    g_GIColorTexture, g_GIPositionTexture, g_GINormalTexture, neighborIndex);

                float3 position = g_PositionFlagsTexture[neighborIndex].xyz;
                float3 normal = g_NormalTexture[neighborIndex].xyz;

                if (abs(depth - g_DepthTexture[neighborIndex]) <= 0.05 && dot(gBufferData.Normal, normal) >= 0.9063)
                {
                    uint newSampleCount = diReservoir.SampleCount + spatialDIReservoir.SampleCount;

                    UpdateReservoir(diReservoir, spatialDIReservoir.Sample, ComputeDIReservoirWeight(gBufferData, eyeDirection, 
                        g_LocalLights[spatialDIReservoir.Sample]) * spatialDIReservoir.Weight * spatialDIReservoir.SampleCount, NextRand(random));

                    diReservoir.SampleCount = newSampleCount;

                    float jacobian = ComputeJacobian(gBufferData.Position, position, spatialGIReservoir.Sample);
                    if (jacobian >= 1.0 / 10.0 && jacobian <= 10.0)
                    {
                        newSampleCount = giReservoir.SampleCount + spatialGIReservoir.SampleCount;
                        UpdateReservoir(giReservoir, spatialGIReservoir.Sample, ComputeGIReservoirWeight(gBufferData, spatialGIReservoir.Sample) *
                            spatialGIReservoir.Weight * spatialGIReservoir.SampleCount * clamp(jacobian, 1.0 / 3.0, 3.0), NextRand(random));

                        giReservoir.SampleCount = newSampleCount;
                    }
                }
            }
        }
        ComputeReservoirWeight(diReservoir, ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[diReservoir.Sample]));
        ComputeReservoirWeight(giReservoir, ComputeGIReservoirWeight(gBufferData, giReservoir.Sample));

        ShadingParams shadingParams;

        shadingParams.EyePosition = g_EyePosition.xyz;
        shadingParams.EyeDirection = eyeDirection;
        shadingParams.Shadow = g_ShadowTexture[dispatchThreadId.xy];

        if (g_LocalLightCount > 0)
            shadingParams.DIReservoir = diReservoir;
        else
            shadingParams.DIReservoir = (Reservoir<uint>) 0;

        globalIllumination = ComputeGI(gBufferData, giReservoir);
        diffuseAlbedo = ComputeGI(gBufferData, 1.0);
        specularAlbedo = ComputeReflection(gBufferData, 1.0);
        rayDirection = giReservoir.Sample.Position - gBufferData.Position;
        hitDistance = length(rayDirection);
        rayDirection /= hitDistance;

        shadingParams.GlobalIllumination = globalIllumination;
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
    }
    else
    {
        color = g_EmissionTexture[dispatchThreadId.xy];
    }

    g_ColorTexture[dispatchThreadId.xy] = float4(color, 1.0);
    g_PrevDIReservoirTexture[dispatchThreadId.xy] = StoreDIReservoir(diReservoir);
    StoreGIReservoir(g_PrevGIColorTexture, g_PrevGIPositionTexture, g_PrevGINormalTexture, dispatchThreadId.xy, giReservoir);
    g_GITexture[dispatchThreadId.xy] = globalIllumination;
    g_DiffuseAlbedoTexture[dispatchThreadId.xy] = diffuseAlbedo;
    g_SpecularAlbedoTexture[dispatchThreadId.xy] = specularAlbedo;
    g_DiffuseRayDirectionHitDistanceTexture[dispatchThreadId.xy] = float4(rayDirection, hitDistance);
}