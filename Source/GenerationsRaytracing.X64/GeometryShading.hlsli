#ifndef GEOMETRY_SHADING_H
#define GEOMETRY_SHADING_H
#include "GBufferData.hlsli"
#include "RootSignature.hlsli"

float3 ComputeDirectLighting(GBufferData gBufferData, float3 eyeDirection,
    float3 lightDirection, float3 diffuseColor, float3 specularColor)
{
    // Diffuse Shading
    diffuseColor *= gBufferData.Diffuse;

    float nDotL = dot(gBufferData.Normal, lightDirection);
    if (gBufferData.Flags & GBUFFER_FLAG_LAMBERT_ADJUSTMENT)
        nDotL = (nDotL - 0.05) / (1.0 - 0.05);

    diffuseColor *= saturate(nDotL);

    // Specular Shading
    specularColor *= gBufferData.Specular * gBufferData.SpecularLevel;

    float3 halfwayDirection = normalize(lightDirection + eyeDirection);

    float nDotH = dot(gBufferData.Normal, halfwayDirection);
    nDotH = pow(saturate(nDotH), gBufferData.SpecularPower);
    specularColor *= nDotH;

    return diffuseColor + specularColor;
}

float3 ComputeEyeLighting(GBufferData gBufferData, float3 eyePosition, float3 eyeDirection)
{
    float3 eyeLighting = ComputeDirectLighting(
        gBufferData,
        eyeDirection,
        eyeDirection,
        mrgEyeLight_Diffuse.rgb * mrgEyeLight_Diffuse.w,
        mrgEyeLight_Specular.rgb * mrgEyeLight_Specular.w);

    if (mrgEyeLight_Attribute.x != 0) // If not directional
    {
        float distance = length(gBufferData.Position - eyePosition);
        eyeLighting *= 1.0 - saturate((distance - mrgEyeLight_Range.z) / (mrgEyeLight_Range.w - mrgEyeLight_Range.z));
    }

    return eyeLighting;
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

float2 ComputePixelPosition(float3 position, float4x4 view, float4x4 projection)
{
    float4 projectedPosition = mul(mul(float4(position, 1.0), view), projection);
    return (projectedPosition.xy / projectedPosition.w * float2(0.5, -0.5) + 0.5) * DispatchRaysDimensions().xy;
}

#endif 