#ifndef GEOMETRY_SHADING_HLSLI_INCLUDED
#define GEOMETRY_SHADING_HLSLI_INCLUDED

#include "GBufferData.hlsli"
#include "Reservoir.hlsli"

struct ShadingParams
{
    float3 EyePosition;
    min16float3 EyeDirection;
    min16float Shadow;
    Reservoir Reservoir;
    min16float3 GlobalIllumination;
    min16float3 Reflection;
    min16float3 Refraction;
};

min16float3 ComputeDirectLighting(GBufferData gBufferData, min16float3 eyeDirection,
    min16float3 lightDirection, min16float3 diffuseColor, min16float3 specularColor)
{
    min16float3 transColor = diffuseColor;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT))
    {
        diffuseColor *= gBufferData.Diffuse;

        min16float cosTheta = dot(gBufferData.Normal, lightDirection);

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
        specularColor *= gBufferData.Specular * gBufferData.SpecularLevel * gBufferData.SpecularFresnel;

        min16float3 halfwayDirection = NormalizeSafe(lightDirection + eyeDirection);

        min16float cosTheta = dot(gBufferData.Normal, halfwayDirection);
        cosTheta = pow(saturate(cosTheta), gBufferData.SpecularGloss);
        specularColor *= cosTheta;
    }
    else
    {
        specularColor = 0.0;
    }

    return diffuseColor + specularColor + transColor;
}

min16float3 ComputeLocalLighting(GBufferData gBufferData, min16float3 eyeDirection, LocalLight localLight)
{
    float3 direction = localLight.Position - gBufferData.Position;
    float distance = length(direction);

    if (distance > 0.0)
        direction /= distance;

    min16float3 localLighting = ComputeDirectLighting(gBufferData, eyeDirection,
        (min16float3) direction, (min16float3) localLight.Color, (min16float3) localLight.Color);

    localLighting *= 1.0 - (min16float) saturate((distance - localLight.InRange) / (localLight.OutRange - localLight.InRange));
    return localLighting;
}

float ComputeReservoirWeight(GBufferData gBufferData, min16float3 eyeDirection, LocalLight localLight)
{
    min16float3 localLighting = ComputeLocalLighting(gBufferData, eyeDirection, localLight);
    return dot(localLighting, min16float3(0.2126, 0.7152, 0.0722));
}

min16float3 ComputeGI(GBufferData gBufferData, min16float3 globalIllumination)
{
    return globalIllumination * (gBufferData.Diffuse + gBufferData.Falloff);
}

min16float3 ComputeReflection(GBufferData gBufferData, min16float3 reflection)
{
    if (!(gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION))
    {
        min16float3 specular = gBufferData.Specular;
        if (!(gBufferData.Flags & GBUFFER_FLAG_IS_GLASS_REFLECTION))
            specular = min(1.0, specular * gBufferData.SpecularLevel * 0.5);

        reflection *= specular;
    }

    return reflection * gBufferData.SpecularFresnel;
}

min16float3 ComputeRefraction(GBufferData gBufferData, min16float3 refraction)
{
    if (gBufferData.Flags & GBUFFER_FLAG_REFRACTION_MUL)
        return refraction * gBufferData.Diffuse;

    if (gBufferData.Flags & GBUFFER_FLAG_REFRACTION_OPACITY)
        return refraction * (1.0 - gBufferData.Refraction);

    return refraction;
}

min16float3 ComputeGeometryShading(GBufferData gBufferData, ShadingParams shadingParams)
{
    min16float3 resultShading = 0.0;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT))
    {
        resultShading += ComputeDirectLighting(gBufferData, 
            shadingParams.EyeDirection, 
            (min16float3) -mrgGlobalLight_Direction.xyz, 
            (min16float3) mrgGlobalLight_Diffuse.rgb, 
            (min16float3) mrgGlobalLight_Specular.rgb);

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_SHADOW))
            resultShading *= shadingParams.Shadow;
    }

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT))
        resultShading += ComputeLocalLighting(gBufferData, shadingParams.EyeDirection, g_LocalLights[shadingParams.Reservoir.Y]) * (min16float) shadingParams.Reservoir.W;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
        resultShading += ComputeGI(gBufferData, shadingParams.GlobalIllumination);

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_REFLECTION))
        resultShading += ComputeReflection(gBufferData, shadingParams.Reflection);

    if (gBufferData.Flags & (GBUFFER_FLAG_REFRACTION_ADD | GBUFFER_FLAG_REFRACTION_MUL | GBUFFER_FLAG_REFRACTION_OPACITY))
        resultShading += ComputeRefraction(gBufferData, shadingParams.Refraction);

    resultShading += gBufferData.Emission;

    return resultShading;
}

min16float3 ComputeWaterShading(GBufferData gBufferData, ShadingParams shadingParams)
{
    min16float3 resultShading = 0.0;

    min16float3 halfwayDirection = NormalizeSafe(shadingParams.EyeDirection - (min16float3) mrgGlobalLight_Direction.xyz);
    min16float cosTheta = pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularGloss);
    min16float3 specularLight = (min16float3) mrgGlobalLight_Specular.rgb * cosTheta * gBufferData.Specular * gBufferData.SpecularGloss * gBufferData.SpecularLevel;
    resultShading += specularLight * shadingParams.Shadow;

    min16float specularFresnel = 1.0 - abs(dot(gBufferData.Normal, shadingParams.EyeDirection));
    specularFresnel *= specularFresnel;
    specularFresnel *= specularFresnel;

    min16float diffuseFresnel = (1.0 - specularFresnel) *
        pow(abs(dot(gBufferData.Normal, (min16float3) g_EyeDirection.xyz)), gBufferData.SpecularGloss);

    resultShading += shadingParams.GlobalIllumination * diffuseFresnel;

    min16float luminance = dot(resultShading, min16float3(0.2126, 0.7152, 0.0722));
    resultShading += shadingParams.Reflection * specularFresnel * (1.0 - saturate(luminance));

    min16float3 diffuseLight = 0.0;
    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT))
    {
        diffuseLight = (min16float3) mrgGlobalLight_Diffuse.rgb * saturate(dot(gBufferData.Normal, (min16float3) -mrgGlobalLight_Direction.xyz)) * shadingParams.Shadow;
        diffuseLight += shadingParams.GlobalIllumination;
        diffuseLight *= gBufferData.Diffuse;
        diffuseLight *= gBufferData.Refraction;
    }

    resultShading += diffuseLight;
    resultShading += ComputeRefraction(gBufferData, shadingParams.Refraction);

    return resultShading;
}

min16float3 ComputeGeometryShadingAux(GBufferData gBufferData, ShadingParams shadingParams)
{
    if (gBufferData.Flags & GBUFFER_FLAG_IS_SKY)
        return gBufferData.Emission;

    else if (gBufferData.Flags & GBUFFER_FLAG_IS_WATER)
        return ComputeWaterShading(gBufferData, shadingParams);

    else
        return ComputeGeometryShading(gBufferData, shadingParams);
}

min16float2 ComputeLightScattering(float3 position, float3 viewPosition)
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

    return min16float2(r0.x, r0.y * g_LightScatteringFarNearScale.w);
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

#endif 