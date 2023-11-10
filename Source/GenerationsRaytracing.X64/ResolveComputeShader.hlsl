#include "GBufferData.h"
#include "GeometryShading.hlsli"
#include "SharedDefinitions.hlsli"
#include "ThreadGroupTilingX.hlsli"

[numthreads(8, 8, 1)]
void main(uint2 groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    uint2 dispatchThreadId = ThreadGroupTilingX(
        (g_InternalResolution.xy + 7) / 8,
        uint2(8, 8),
        8,
        groupThreadId,
        groupId);

    uint random = InitRand(g_CurrentFrame, dispatchThreadId.y * g_InternalResolution.x + dispatchThreadId.x);

    GBufferData gBufferDatas[GBUFFER_DATA_NUM];
    ShadingParams shadingParams[GBUFFER_DATA_NUM];

    [unroll]
    for (uint i = 0; i < GBUFFER_DATA_NUM; i++)
        gBufferDatas[i] = LoadGBufferData(uint3(dispatchThreadId, i));

    [unroll]
    for (uint i = 0; i < GBUFFER_DATA_NUM; i++)
    {
        shadingParams[i] = (ShadingParams) 0;
        shadingParams[i].Shadow = g_Shadow[uint3(dispatchThreadId, i)];
    }

    shadingParams[GBUFFER_DATA_PRIMARY].EyePosition = g_EyePosition.xyz;

    shadingParams[GBUFFER_DATA_SECONDARY_GI].EyePosition = gBufferDatas[GBUFFER_DATA_PRIMARY].Position;
    shadingParams[GBUFFER_DATA_SECONDARY_REFLECTION].EyePosition = gBufferDatas[GBUFFER_DATA_PRIMARY].Position;
    shadingParams[GBUFFER_DATA_SECONDARY_REFRACTION].EyePosition = gBufferDatas[GBUFFER_DATA_PRIMARY].Position;

    shadingParams[GBUFFER_DATA_TERTIARY_GI].EyePosition = gBufferDatas[GBUFFER_DATA_SECONDARY_GI].Position;
    shadingParams[GBUFFER_DATA_TERTIARY_REFLECTION_GI].EyePosition = gBufferDatas[GBUFFER_DATA_SECONDARY_REFLECTION].Position;
    shadingParams[GBUFFER_DATA_TERTIARY_REFRACTION_GI].EyePosition = gBufferDatas[GBUFFER_DATA_SECONDARY_REFRACTION].Position;

    [unroll]
    for (uint i = 0; i < GBUFFER_DATA_NUM; i++)
        shadingParams[i].EyeDirection = NormalizeSafe(shadingParams[i].EyePosition - gBufferDatas[i].Position);

    shadingParams[GBUFFER_DATA_PRIMARY].Reservoir = LoadReservoir(g_Reservoir[dispatchThreadId]);

    [unroll]
    for (uint i = GBUFFER_DATA_PRIMARY + 1; i < GBUFFER_DATA_NUM; i++)
    {
        shadingParams[i].Reservoir.Y = min(floor(NextRand(random) * g_LocalLightCount), g_LocalLightCount - 1);
        shadingParams[i].Reservoir.W = g_LocalLightCount;
    }

    shadingParams[GBUFFER_DATA_SECONDARY_GI].GlobalIllumination =
        ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_TERTIARY_GI], shadingParams[GBUFFER_DATA_TERTIARY_GI]);

    shadingParams[GBUFFER_DATA_SECONDARY_REFLECTION].GlobalIllumination =
        ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_TERTIARY_REFLECTION_GI], shadingParams[GBUFFER_DATA_TERTIARY_REFLECTION_GI]);

    shadingParams[GBUFFER_DATA_SECONDARY_REFRACTION].GlobalIllumination =
        ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_TERTIARY_REFRACTION_GI], shadingParams[GBUFFER_DATA_TERTIARY_REFRACTION_GI]);

    shadingParams[GBUFFER_DATA_PRIMARY].GlobalIllumination =
        ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_SECONDARY_GI], shadingParams[GBUFFER_DATA_SECONDARY_GI]);

    shadingParams[GBUFFER_DATA_PRIMARY].Reflection = gBufferDatas[GBUFFER_DATA_PRIMARY].SpecularPDF *
        ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_SECONDARY_REFLECTION], shadingParams[GBUFFER_DATA_SECONDARY_REFLECTION]);

    shadingParams[GBUFFER_DATA_PRIMARY].Refraction = gBufferDatas[GBUFFER_DATA_PRIMARY].SpecularPDF *
        ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_SECONDARY_REFRACTION], shadingParams[GBUFFER_DATA_SECONDARY_REFRACTION]);

    float3 color = ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_PRIMARY], shadingParams[GBUFFER_DATA_PRIMARY]);

    if (!(gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IS_SKY))
    {
        float3 viewPosition = mul(float4(gBufferDatas[GBUFFER_DATA_PRIMARY].Position, 1.0), g_MtxView).xyz;
        float2 lightScattering = ComputeLightScattering(gBufferDatas[GBUFFER_DATA_PRIMARY].Position, viewPosition);

        if (all(and(!isnan(lightScattering), !isinf(lightScattering))))
            color = color * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
    }

    g_Color[dispatchThreadId] = float4(color, 1.0);

    g_DiffuseAlbedo[dispatchThreadId] = gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION ? 0.0 :
        float4(ComputeGI(gBufferDatas[GBUFFER_DATA_PRIMARY], 1.0), 0.0);

    g_SpecularAlbedo[dispatchThreadId] = gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IGNORE_REFLECTION ? 0.0 :
        float4(ComputeReflection(gBufferDatas[GBUFFER_DATA_PRIMARY], 1.0), 0.0);

    g_NormalsRoughness[dispatchThreadId] =
        float4(gBufferDatas[GBUFFER_DATA_PRIMARY].Normal, 1.0 - pow(gBufferDatas[GBUFFER_DATA_PRIMARY].SpecularGloss, 0.2) * 0.25);

    g_ReflectedAlbedo[dispatchThreadId] = gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IGNORE_REFLECTION ? 0.0 :
        float4(ComputeGI(gBufferDatas[GBUFFER_DATA_SECONDARY_REFLECTION], 1.0), 0.0);

    float3 diffuseRayDirection = gBufferDatas[GBUFFER_DATA_SECONDARY_GI].Position - gBufferDatas[GBUFFER_DATA_PRIMARY].Position;
    float diffuseHitDistance = length(diffuseRayDirection);
    if (diffuseHitDistance > 0.0)
        diffuseRayDirection /= diffuseHitDistance;

    g_DiffuseRayDirectionHitDistance[dispatchThreadId] = gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION ? 0.0 :
        float4(diffuseRayDirection, diffuseHitDistance);

    float3 specularRayDirection = gBufferDatas[GBUFFER_DATA_SECONDARY_REFLECTION].Position - gBufferDatas[GBUFFER_DATA_PRIMARY].Position;
    float specularHitDistance = length(specularRayDirection);
    if (specularHitDistance > 0.0)
        specularRayDirection /= specularHitDistance;

    g_SpecularRayDirectionHitDistance[dispatchThreadId] = gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IGNORE_REFLECTION ? 0.0 :
        float4(specularRayDirection, specularHitDistance);
}
