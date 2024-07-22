#pragma once

#include "RootSignature.hlsli"

#define GBUFFER_FLAG_IS_SKY                     (1 << 0)
#define GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT       (1 << 1)
#define GBUFFER_FLAG_IGNORE_SPECULAR_LIGHT      (1 << 2)
#define GBUFFER_FLAG_IGNORE_SHADOW              (1 << 3)
#define GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT        (1 << 4)
#define GBUFFER_FLAG_IGNORE_LOCAL_LIGHT         (1 << 5)
#define GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION (1 << 6)
#define GBUFFER_FLAG_IGNORE_REFLECTION          (1 << 7)
#define GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT     (1 << 8)
#define GBUFFER_FLAG_HALF_LAMBERT               (1 << 9)
#define GBUFFER_FLAG_MUL_BY_SPEC_GLOSS          (1 << 10)
#define GBUFFER_FLAG_IS_MIRROR_REFLECTION       (1 << 11)
#define GBUFFER_FLAG_IS_METALLIC                (1 << 12)
#define GBUFFER_FLAG_IS_WATER                   (1 << 13)
#define GBUFFER_FLAG_REFRACTION_ADD             (1 << 14)
#define GBUFFER_FLAG_REFRACTION_MUL             (1 << 15)
#define GBUFFER_FLAG_REFRACTION_OPACITY         (1 << 16)
#define GBUFFER_FLAG_REFRACTION_OVERLAY         (1 << 17)
#define GBUFFER_FLAG_REFRACTION_ALL             (GBUFFER_FLAG_REFRACTION_ADD | GBUFFER_FLAG_REFRACTION_MUL | GBUFFER_FLAG_REFRACTION_OPACITY | GBUFFER_FLAG_REFRACTION_OVERLAY)
#define GBUFFER_FLAG_IS_ADDITIVE                (1 << 18)
#define GBUFFER_FLAG_FULBRIGHT                  (1 << 19)
#define GBUFFER_FLAG_MUL_BY_REFRACTION          (1 << 20)
#define GBUFFER_FLAG_IS_PBR                     (1 << 21)

struct GBufferData
{
    float3 Position;
    uint Flags;

    float3 Diffuse;
    float Alpha;

    float3 Specular;
    float3 SpecularTint;
    float SpecularEnvironment;
    float SpecularGloss;
    float SpecularLevel;
    float SpecularFresnel;

    float3 Normal;
    float3 Falloff;
    float3 Emission;
    float3 TransColor;
    
    float Refraction;
    float RefractionOverlay;
    float2 RefractionOffset;
};

GBufferData LoadGBufferData(uint3 index)
{
    float4 gBuffer0 = g_GBuffer0_SRV[index];
    float4 gBuffer1 = g_GBuffer1_SRV[index];
    float4 gBuffer2 = g_GBuffer2_SRV[index];
    float4 gBuffer3 = g_GBuffer3_SRV[index];
    float4 gBuffer4 = g_GBuffer4_SRV[index];
    float4 gBuffer5 = g_GBuffer5_SRV[index];
    float4 gBuffer6 = g_GBuffer6_SRV[index];
    float4 gBuffer7 = g_GBuffer7_SRV[index];
    float4 gBuffer8 = g_GBuffer8_SRV[index];

    GBufferData gBufferData = (GBufferData) 0;

    gBufferData.Position = gBuffer0.xyz;
    gBufferData.Flags = asuint(gBuffer0.w);

    gBufferData.Diffuse = gBuffer1.rgb;
    gBufferData.Alpha = gBuffer1.a;

    gBufferData.Specular = gBuffer2.rgb;
    gBufferData.SpecularTint = gBuffer3.rgb;
    gBufferData.SpecularEnvironment = gBuffer2.w;
    gBufferData.SpecularGloss = gBuffer3.w;
    gBufferData.SpecularLevel = gBuffer4.x;
    gBufferData.SpecularFresnel = gBuffer4.y;

    gBufferData.Normal = gBuffer5.xyz;
    gBufferData.Falloff = gBuffer6.xyz;
    gBufferData.Emission = gBuffer7.xyz;
    
    gBufferData.TransColor = float3(gBuffer5.w, gBuffer6.w, gBuffer7.w);
    
    gBufferData.Refraction = gBuffer4.z;
    gBufferData.RefractionOverlay = gBuffer4.w;
    gBufferData.RefractionOffset = gBuffer8.xy;
    
    float lengthSquared = dot(gBufferData.Normal, gBufferData.Normal);
    gBufferData.Normal = select(lengthSquared > 0.0, gBufferData.Normal * rsqrt(lengthSquared), 0.0);

    return gBufferData;
}

void StoreGBufferData(uint3 index, GBufferData gBufferData)
{
    g_GBuffer0[index] = float4(gBufferData.Position, asfloat(gBufferData.Flags));
    g_GBuffer1[index] = float4(gBufferData.Diffuse, gBufferData.Alpha);
    g_GBuffer2[index] = float4(gBufferData.Specular, gBufferData.SpecularEnvironment);
    g_GBuffer3[index] = float4(gBufferData.SpecularTint, gBufferData.SpecularGloss);
    g_GBuffer4[index] = float4(gBufferData.SpecularLevel, gBufferData.SpecularFresnel, gBufferData.Refraction, gBufferData.RefractionOverlay);
    g_GBuffer5[index] = float4(gBufferData.Normal, gBufferData.TransColor.r);
    g_GBuffer6[index] = float4(gBufferData.Falloff, gBufferData.TransColor.g);
    g_GBuffer7[index] = float4(gBufferData.Emission, gBufferData.TransColor.b);
    g_GBuffer8[index] = float4(gBufferData.RefractionOffset, 0.0, 0.0);
}