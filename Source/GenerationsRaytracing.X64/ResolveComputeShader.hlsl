#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float3 color;

    GBufferData gBufferData = LoadGBufferData(dispatchThreadId.xy);
    float depth = g_Depth[dispatchThreadId.xy];

    float3 diffuseAlbedo = 0.0;
    float3 specularAlbedo = 0.0;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IS_SKY))
    {
        ShadingParams shadingParams = (ShadingParams) 0;

        shadingParams.EyePosition = g_EyePosition.xyz;
        shadingParams.EyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);

        if (!(gBufferData.Flags & (GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_SHADOW)))
            shadingParams.Shadow = g_Shadow[dispatchThreadId.xy];
        else
            shadingParams.Shadow = 1.0;

        if (g_LocalLightCount > 0 && !(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT))
        {
            shadingParams.Reservoir = LoadReservoir(g_Reservoir[dispatchThreadId.xy]);
            uint random = InitRand(g_CurrentFrame, dispatchThreadId.y * g_InternalResolution.x + dispatchThreadId.x);

            for (uint i = 0; i < 5; i++)
            {
                float radius = 16.0 * NextRand(random);
                float angle = 2.0 * PI * NextRand(random);

                int2 neighborIndex = round((float2) dispatchThreadId.xy + float2(cos(angle), sin(angle)) * radius);

                if (all(and(neighborIndex >= 0, neighborIndex < g_InternalResolution)))
                {
                    Reservoir spatialReservoir = LoadReservoir(g_Reservoir[neighborIndex]);

                    if (abs(depth - g_Depth[neighborIndex]) <= 0.05 && dot(gBufferData.Normal, LoadGBufferData(neighborIndex).Normal.xyz) >= 0.9063)
                    {
                        uint newSampleCount = shadingParams.Reservoir.M + spatialReservoir.M;

                        UpdateReservoir(shadingParams.Reservoir, spatialReservoir.Y, ComputeReservoirWeight(gBufferData, shadingParams.EyeDirection,
                            g_LocalLights[spatialReservoir.Y]) * spatialReservoir.W * spatialReservoir.M, NextRand(random));

                        shadingParams.Reservoir.M = newSampleCount;
                    }
                }
            }

            ComputeReservoirWeight(shadingParams.Reservoir, ComputeReservoirWeight(gBufferData, 
                shadingParams.EyeDirection, g_LocalLights[shadingParams.Reservoir.Y]));
        }

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
        {
            shadingParams.GlobalIllumination = g_GlobalIllumination[dispatchThreadId.xy];
            diffuseAlbedo = ComputeGI(gBufferData, 1.0);
        }

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_REFLECTION))
        {
            shadingParams.Reflection = g_Reflection[dispatchThreadId.xy];
            specularAlbedo = ComputeReflection(gBufferData, 1.0);
        }

        if (gBufferData.Flags & (GBUFFER_FLAG_REFRACTION_MUL | GBUFFER_FLAG_REFRACTION_ADD | GBUFFER_FLAG_REFRACTION_OPACITY | GBUFFER_FLAG_ADDITIVE))
            shadingParams.Refraction = g_Refraction[dispatchThreadId.xy];

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
        color = gBufferData.Emission;
    }

    g_Color[dispatchThreadId.xy] = float4(color, 1.0);
    g_DiffuseAlbedo[dispatchThreadId.xy] = diffuseAlbedo;
    g_SpecularAlbedo[dispatchThreadId.xy] = specularAlbedo;

    if (dispatchThreadId.x == 0 && dispatchThreadId.y == 0)
        g_Exposure[dispatchThreadId.xy] = GetExposure() * 2.0;
}