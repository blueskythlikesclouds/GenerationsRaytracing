#include "GeometryShading.hlsli"

#define GBUFFER_DATA_PRIMARY                0
#define GBUFFER_DATA_SECONDARY_GI           1
#define GBUFFER_DATA_SECONDARY_REFLECTION   2
#define GBUFFER_DATA_TERTIARY_GI            3
#define GBUFFER_DATA_TERTIARY_REFLECTION_GI 4
#define GBUFFER_DATA_MAX                    5

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (any(dispatchThreadId.xy >= g_InternalResolution))
        return;

    GBufferData gBufferDatas[GBUFFER_DATA_MAX];
    ShadingParams shadingParams[GBUFFER_DATA_MAX];

    for (uint i = 0; i < GBUFFER_DATA_MAX; i++)
    {
        gBufferDatas[i] = LoadGBufferData(uint3(dispatchThreadId.xy, i));

        shadingParams[i] = (ShadingParams) 0;
        shadingParams[i].Shadow = g_ShadowTexture[uint3(dispatchThreadId.xy, i)];
    }

    shadingParams[GBUFFER_DATA_PRIMARY].EyePosition = g_EyePosition.xyz;

    shadingParams[GBUFFER_DATA_SECONDARY_GI].EyePosition = gBufferDatas[GBUFFER_DATA_PRIMARY].Position;
    shadingParams[GBUFFER_DATA_SECONDARY_REFLECTION].EyePosition = gBufferDatas[GBUFFER_DATA_PRIMARY].Position;

    shadingParams[GBUFFER_DATA_TERTIARY_GI].EyePosition = gBufferDatas[GBUFFER_DATA_SECONDARY_GI].Position;
    shadingParams[GBUFFER_DATA_TERTIARY_REFLECTION_GI].EyePosition = gBufferDatas[GBUFFER_DATA_SECONDARY_REFLECTION].Position;

    for (uint i = 0; i < GBUFFER_DATA_MAX; i++)
        shadingParams[i].EyeDirection = NormalizeSafe(shadingParams[i].EyePosition - gBufferDatas[i].Position);

    shadingParams[GBUFFER_DATA_SECONDARY_GI].GlobalIllumination =
        ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_TERTIARY_GI], shadingParams[GBUFFER_DATA_TERTIARY_GI]);

    shadingParams[GBUFFER_DATA_SECONDARY_REFLECTION].GlobalIllumination =
        ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_TERTIARY_REFLECTION_GI], shadingParams[GBUFFER_DATA_TERTIARY_REFLECTION_GI]);

    shadingParams[GBUFFER_DATA_PRIMARY].GlobalIllumination =
        ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_SECONDARY_GI], shadingParams[GBUFFER_DATA_SECONDARY_GI]);

    shadingParams[GBUFFER_DATA_PRIMARY].Reflection =
        ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_SECONDARY_REFLECTION], shadingParams[GBUFFER_DATA_SECONDARY_REFLECTION]);

    g_ColorTexture[dispatchThreadId.xy] = 
        float4(ComputeGeometryShadingAux(gBufferDatas[GBUFFER_DATA_PRIMARY], shadingParams[GBUFFER_DATA_PRIMARY]), 1.0);

    g_DiffuseAlbedoTexture[dispatchThreadId.xy] = gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION ? 0.0 :
        float4(ComputeGI(gBufferDatas[GBUFFER_DATA_PRIMARY], 1.0), 0.0);

    g_SpecularAlbedoTexture[dispatchThreadId.xy] = gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IGNORE_REFLECTION ? 0.0 :
        float4(ComputeReflection(gBufferDatas[GBUFFER_DATA_PRIMARY], 1.0), 0.0);

    g_NormalsRoughnessTexture[dispatchThreadId.xy] =
        float4(gBufferDatas[GBUFFER_DATA_PRIMARY].Normal, 1.0 - pow(gBufferDatas[GBUFFER_DATA_PRIMARY].SpecularGloss, 0.2) * 0.25);

    g_ReflectedAlbedoTexture[dispatchThreadId.xy] = gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IGNORE_REFLECTION ? 0.0 :
        float4(ComputeGI(gBufferDatas[GBUFFER_DATA_SECONDARY_REFLECTION], 1.0), 0.0);

    float3 diffuseRayDirection = gBufferDatas[GBUFFER_DATA_SECONDARY_GI].Position - gBufferDatas[GBUFFER_DATA_PRIMARY].Position;
    float diffuseHitDistance = length(diffuseRayDirection);
    if (diffuseHitDistance > 0.0)
        diffuseRayDirection /= diffuseHitDistance;

    g_DiffuseRayDirectionHitDistanceTexture[dispatchThreadId.xy] = gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION ? 0.0 :
        float4(diffuseRayDirection, diffuseHitDistance);

    float3 specularRayDirection = gBufferDatas[GBUFFER_DATA_SECONDARY_REFLECTION].Position - gBufferDatas[GBUFFER_DATA_PRIMARY].Position;
    float specularHitDistance = length(specularRayDirection);
    if (specularHitDistance > 0.0)
        specularRayDirection /= specularHitDistance;

    g_DiffuseRayDirectionHitDistanceTexture[dispatchThreadId.xy] = gBufferDatas[GBUFFER_DATA_PRIMARY].Flags & GBUFFER_FLAG_IGNORE_REFLECTION ? 0.0 :
        float4(specularRayDirection, specularHitDistance);
}
