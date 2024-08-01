#pragma once

#include "RootSignature.hlsli"
#include "PackedPrimitives.hlsli"

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
    
    float Refraction;
    float RefractionOverlay;
    float2 RefractionOffset;
};

GBufferData LoadGBufferData(uint3 index)
{
    float4 gBuffer0 = g_GBuffer0_SRV[index];
    uint gBuffer1 = g_GBuffer1_SRV[index];
    uint gBuffer2 = g_GBuffer2_SRV[index];
    uint gBuffer3 = g_GBuffer3_SRV[index];
    float2 gBuffer4 = g_GBuffer4_SRV[index];
    float2 gBuffer5 = g_GBuffer5_SRV[index];
    uint gBuffer6 = g_GBuffer6_SRV[index];
    uint gBuffer7 = g_GBuffer7_SRV[index];
    float4 gBuffer8 = g_GBuffer8_SRV[index];
    float2 gBuffer9 = g_GBuffer9_SRV[index];
    float2 gBuffer10 = g_GBuffer10_SRV[index];

    GBufferData gBufferData = (GBufferData) 0;

    gBufferData.Position = gBuffer0.xyz;
    gBufferData.Flags = asuint(gBuffer0.w);

    gBufferData.Diffuse = DecodeUnorm11(gBuffer1) * 2.0;
    gBufferData.Alpha = gBuffer8.a;

    gBufferData.Specular = DecodeUnorm11(gBuffer2) * 2.0;
    gBufferData.SpecularTint = DecodeUnorm11(gBuffer3);
    gBufferData.SpecularEnvironment = gBuffer4.x;
    gBufferData.SpecularGloss = gBuffer5.x * 1024.0;
    gBufferData.SpecularLevel = gBuffer5.y * 1024.0;
    gBufferData.SpecularFresnel = gBuffer4.y;

    gBufferData.Normal = DecodeUnorm11(gBuffer6) * 2.0 - 1.0;
    gBufferData.Falloff = DecodeUnorm11(gBuffer7) * 2.0;
    gBufferData.Emission = gBuffer8.rgb;
    
    gBufferData.Refraction = gBuffer9.x;
    gBufferData.RefractionOverlay = gBuffer9.y;
    gBufferData.RefractionOffset = gBuffer10 * 2.0 - 1.0;
    
    float lengthSquared = dot(gBufferData.Normal, gBufferData.Normal);
    gBufferData.Normal = select(lengthSquared > 0.0, gBufferData.Normal * rsqrt(lengthSquared), 0.0);

    return gBufferData;
}

void StoreGBufferData(uint3 index, GBufferData gBufferData)
{
    g_GBuffer0[index] = float4(gBufferData.Position, asfloat(gBufferData.Flags));
    g_GBuffer1[index] = EncodeUnorm11(gBufferData.Diffuse / 2.0);
    g_GBuffer2[index] = EncodeUnorm11(gBufferData.Specular / 2.0);
    g_GBuffer3[index] = EncodeUnorm11(gBufferData.SpecularTint);
    g_GBuffer4[index] = float2(gBufferData.SpecularEnvironment, gBufferData.SpecularFresnel);
    g_GBuffer5[index] = float2(gBufferData.SpecularGloss, gBufferData.SpecularLevel) / 1024.0;
    g_GBuffer6[index] = EncodeUnorm11(gBufferData.Normal * 0.5 + 0.5);
    g_GBuffer7[index] = EncodeUnorm11(gBufferData.Falloff / 2.0);
    g_GBuffer8[index] = float4(gBufferData.Emission, gBufferData.Alpha);
    g_GBuffer9[index] = float2(gBufferData.Refraction, gBufferData.RefractionOverlay);
    g_GBuffer10[index] = gBufferData.RefractionOffset * 0.5 + 0.5;
}