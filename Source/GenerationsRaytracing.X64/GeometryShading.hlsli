#pragma once

#include "GBufferData.hlsli"
#include "Reservoir.hlsli"
#include "RootSignature.hlsli"
#include "ShaderTable.h"
#include "SharedDefinitions.hlsli"

struct ShadingParams
{
    float3 EyePosition;
    float3 EyeDirection;
    float3 Shadow;
    Reservoir Reservoir;
    float3 GlobalIllumination;
    float3 Reflection;
    float3 Refraction;
};

float3 ComputeDirectLighting(GBufferData gBufferData, float3 eyeDirection,
    float3 lightDirection, float3 diffuseColor, float3 specularColor)
{
    float3 transColor = diffuseColor;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT))
    {
        diffuseColor *= gBufferData.Diffuse;

        float cosTheta = dot(gBufferData.Normal, lightDirection);

        transColor *= gBufferData.TransColor;
        transColor *= saturate(-cosTheta);

        if (gBufferData.Flags & GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT)
        {
            cosTheta = (cosTheta - 0.05) / (1.0 - 0.05);
        }

        else if (gBufferData.Flags & GBUFFER_FLAG_HALF_LAMBERT)
        {
            cosTheta = cosTheta * 0.5 + 0.5;
            cosTheta *= cosTheta;
        }

        diffuseColor *= saturate(cosTheta);
    }
    else
    {
        diffuseColor = 0.0;
        transColor = 0.0;
    }

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_SPECULAR_LIGHT))
    {
        if (gBufferData.Flags & GBUFFER_FLAG_IS_METALLIC)
        {
            float3 fresnel = gBufferData.Specular;
            fresnel += (1.0 - fresnel) * gBufferData.SpecularFresnel;

            specularColor *= fresnel;
        }
        else
        {
            specularColor *= gBufferData.Specular;

            if (gBufferData.Flags & GBUFFER_FLAG_MUL_BY_SPEC_GLOSS)
                specularColor *= gBufferData.SpecularGloss;

            specularColor *= gBufferData.SpecularLevel;
            specularColor *= gBufferData.SpecularFresnel;
        }

        float3 halfwayDirection = NormalizeSafe(lightDirection + eyeDirection);

        float cosTheta = dot(gBufferData.Normal, halfwayDirection);
        cosTheta = pow(saturate(cosTheta), gBufferData.SpecularGloss);
        specularColor *= cosTheta;
    }
    else
    {
        specularColor = 0.0;
    }

    return diffuseColor + specularColor + transColor;
}

float ComputeLocalLightFalloff(float distance, float inRange, float outRange)
{
    return 1.0 - saturate((distance - inRange) / (outRange - inRange));
}

float3 ComputeLocalLighting(GBufferData gBufferData, float3 eyeDirection, LocalLight localLight)
{
    float3 direction = localLight.Position - gBufferData.Position;
    float distance = length(direction);

    if (distance > 0.0)
        direction /= distance;

    float3 localLighting = ComputeDirectLighting(gBufferData, eyeDirection,
        direction, localLight.Color, localLight.Color);

    localLighting *= ComputeLocalLightFalloff(distance, localLight.InRange, localLight.OutRange);
    return localLighting;
}

float TraceLocalLightShadow(float3 position, float3 direction, float2 random, float radius, float distance)
{
    radius *= sqrt(random.x);
    float angle = random.y * 2.0 * PI;

    float3 sample;
    sample.x = cos(angle) * radius;
    sample.y = sin(angle) * radius;
    sample.z = 1.0;

    RayDesc ray;

    ray.Origin = position;
    ray.Direction = TangentToWorld(direction, sample);
    ray.TMin = 0.0;
    ray.TMax = distance;

    RayQuery<RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;

    query.TraceRayInline(
        g_BVH,
        RAY_FLAG_NONE,
        INSTANCE_MASK_OPAQUE,
        ray);

    query.Proceed();

    return query.CommittedStatus() == COMMITTED_NOTHING ? 1.0 : 0.0;
}

float ComputeReservoirWeight(GBufferData gBufferData, float3 eyeDirection, LocalLight localLight)
{
    float3 localLighting = ComputeLocalLighting(gBufferData, eyeDirection, localLight);
    return length(localLighting);
}

float3 ComputeGI(GBufferData gBufferData, float3 globalIllumination)
{
    globalIllumination *= gBufferData.Diffuse + gBufferData.Falloff;
    if (gBufferData.Flags & GBUFFER_FLAG_IS_METALLIC)
        globalIllumination *= 1.0 - gBufferData.SpecularFresnel;

    return globalIllumination;
}

float3 ComputeReflection(GBufferData gBufferData, float3 reflection)
{
    if (gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION)
    {
        reflection *= gBufferData.SpecularFresnel;
    }

    else if (gBufferData.Flags & GBUFFER_FLAG_IS_GLASS_REFLECTION)
    {
        reflection *= gBufferData.Specular * gBufferData.SpecularFresnel;
    }

    else if (gBufferData.Flags & GBUFFER_FLAG_IS_METALLIC)
    {
        float3 fresnel = gBufferData.Specular;
        fresnel += (1.0 - fresnel) * gBufferData.SpecularFresnel;
        reflection *= fresnel;
    }

    else
    {
        float specularIntensity = 1.0 - exp2(-gBufferData.SpecularLevel * gBufferData.SpecularFresnel);
        reflection *= gBufferData.Specular * specularIntensity;
    }

    return reflection;
}

float3 ComputeRefraction(GBufferData gBufferData, float3 refraction)
{
    if (gBufferData.Flags & GBUFFER_FLAG_REFRACTION_MUL)
        return refraction * gBufferData.Diffuse;

    if (gBufferData.Flags & GBUFFER_FLAG_REFRACTION_OPACITY)
        return refraction * (1.0 - gBufferData.Refraction);

    return refraction;
}

float3 ComputeGeometryShading(GBufferData gBufferData, ShadingParams shadingParams)
{
    float3 resultShading = 0.0;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT))
    {
        resultShading += ComputeDirectLighting(gBufferData, shadingParams.EyeDirection, 
            -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb) * shadingParams.Shadow;
    }

    if (g_LocalLightCount > 0 && !(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT))
        resultShading += ComputeLocalLighting(gBufferData, shadingParams.EyeDirection, g_LocalLights[shadingParams.Reservoir.Y]) * shadingParams.Reservoir.W;

    resultShading += ComputeGI(gBufferData, shadingParams.GlobalIllumination);
    resultShading += ComputeReflection(gBufferData, shadingParams.Reflection);
    resultShading += ComputeRefraction(gBufferData, shadingParams.Refraction);
    resultShading += gBufferData.Emission;

    return resultShading;
}

float3 ComputeWaterShading(GBufferData gBufferData, ShadingParams shadingParams)
{
    float3 resultShading = 0.0;

    float3 halfwayDirection = NormalizeSafe(shadingParams.EyeDirection - mrgGlobalLight_Direction.xyz);
    float cosTheta = pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularGloss);
    float3 specularLight = mrgGlobalLight_Specular.rgb * cosTheta * gBufferData.Specular * gBufferData.SpecularGloss * gBufferData.SpecularLevel;
    resultShading += specularLight * shadingParams.Shadow;

    float specularFresnel = 1.0 - abs(dot(gBufferData.Normal, shadingParams.EyeDirection));
    specularFresnel *= specularFresnel;
    specularFresnel *= specularFresnel;

    float diffuseFresnel = (1.0 - specularFresnel) * 
        pow(abs(dot(gBufferData.Normal, g_EyeDirection.xyz)), gBufferData.SpecularGloss);

    resultShading += shadingParams.GlobalIllumination * diffuseFresnel;

    float luminance = dot(resultShading, float3(0.2126, 0.7152, 0.0722));
    resultShading += shadingParams.Reflection * specularFresnel * (1.0 - saturate(luminance));

    float3 diffuseLight = 0.0;
    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT))
    {
        diffuseLight = mrgGlobalLight_Diffuse.rgb * saturate(dot(gBufferData.Normal, -mrgGlobalLight_Direction.xyz)) * shadingParams.Shadow;
        diffuseLight += shadingParams.GlobalIllumination;
        diffuseLight *= gBufferData.Diffuse;
        diffuseLight *= gBufferData.Refraction;
    }

    resultShading += diffuseLight;
    resultShading += ComputeRefraction(gBufferData, shadingParams.Refraction);

    return resultShading;
}

float2 ComputeLightScattering(float3 position, float3 viewPosition)
{
    float4 r0, r3, r4;

    r0.x = -viewPosition.z + -g_LightScatteringFarNearScale.y;
    r0.x = saturate(r0.x * g_LightScatteringFarNearScale.x);
    r0.x = r0.x * g_LightScatteringFarNearScale.z;
    r0.y = g_LightScattering_Ray_Mie_Ray2_Mie2.y + g_LightScattering_Ray_Mie_Ray2_Mie2.x;
    r0.z = rcp(r0.y);
    r0.x = r0.x * -r0.y;
    r0.x = exp(r0.x);
    r0.y = -r0.x + 1;
    r3.xyz = -position + g_EyePosition.xyz;
    r4.xyz = NormalizeSafe(r3.xyz);
    r3.x = dot(-mrgGlobalLight_Direction.xyz, r4.xyz);
    r3.y = g_LightScattering_ConstG_FogDensity.z * r3.x + g_LightScattering_ConstG_FogDensity.y;
    r4.x = pow(abs(r3.y), 1.5);
    r3.y = rcp(r4.x);
    r3.y = r3.y * g_LightScattering_ConstG_FogDensity.x;
    r3.y = r3.y * g_LightScattering_Ray_Mie_Ray2_Mie2.w;
    r3.x = r3.x * r3.x + 1;
    r3.x = g_LightScattering_Ray_Mie_Ray2_Mie2.z * r3.x + r3.y;
    r0.z = r0.z * r3.x;
    r0.y = r0.y * r0.z;

    return float2(r0.x, r0.y * g_LightScatteringFarNearScale.w);
}

float3 ComputePrimaryRayDirection(uint2 index, uint2 resolution)
{
    float2 ndc = (index - g_PixelJitter + 0.5) / resolution * float2(2.0, -2.0) + float2(-1.0, 1.0);
    return mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz;
}

float2 ComputePixelPosition(float3 position, float4x4 view, float4x4 projection)
{
    float4 projectedPosition = mul(mul(float4(position, 1.0), view), projection);
    return (projectedPosition.xy / projectedPosition.w * float2(0.5, -0.5) + 0.5) * DispatchRaysDimensions().xy;
}

float ComputeDepth(float3 position, float4x4 view, float4x4 projection)
{
    float4 projectedPosition = mul(mul(float4(position, 1.0), view), projection);
    return projectedPosition.z / projectedPosition.w;
}