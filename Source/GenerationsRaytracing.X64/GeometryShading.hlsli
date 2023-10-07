#ifndef GEOMETRY_SHADING_H
#define GEOMETRY_SHADING_H
#include "GBufferData.hlsli"
#include "RootSignature.hlsli"

#define PI 3.14159265358979323846

float3 ComputeDirectLighting(GBufferData gBufferData, float3 lightDirection, float3 diffuseColor, float3 specularColor)
{
    // Diffuse Shading
    diffuseColor *= gBufferData.Diffuse;

    // Specular Shading
    specularColor *= gBufferData.Specular * gBufferData.SpecularLevel;

    float3 halfwayDirection = normalize(lightDirection + -WorldRayDirection());

    float specular = dot(gBufferData.Normal, halfwayDirection);
    specular = pow(saturate(specular), gBufferData.SpecularPower);
    specularColor *= specular;

    // Resolve
    float3 resolvedColor = diffuseColor + specularColor;
    resolvedColor *= saturate(dot(gBufferData.Normal, lightDirection));

    return resolvedColor;
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
    r4.xyz = normalize(r3.xyz);
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

#endif 