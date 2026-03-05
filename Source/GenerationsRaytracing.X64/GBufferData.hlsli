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

struct PackedGBufferData
{
    float4 GBuffer0;
    uint GBuffer1;
    uint GBuffer2;
    uint GBuffer3;
    float2 GBuffer4;
    float2 GBuffer5;
    uint GBuffer6;
    uint GBuffer7;
    float4 GBuffer8;
    float2 GBuffer9;
    float2 GBuffer10;
};

GBufferData UnpackGBufferData(PackedGBufferData packedGBufferData)
{
    GBufferData gBufferData = (GBufferData) 0;

    gBufferData.Position = packedGBufferData.GBuffer0.xyz;
    gBufferData.Flags = asuint(packedGBufferData.GBuffer0.w);

    gBufferData.Diffuse = DecodeUnorm11(packedGBufferData.GBuffer1) * 2.0;
    gBufferData.Alpha = packedGBufferData.GBuffer8.a;

    gBufferData.Specular = DecodeUnorm11(packedGBufferData.GBuffer2) * 2.0;
    gBufferData.SpecularTint = DecodeUnorm11(packedGBufferData.GBuffer3);
    gBufferData.SpecularEnvironment = packedGBufferData.GBuffer4.x;
    gBufferData.SpecularGloss = packedGBufferData.GBuffer5.x * 1024.0;
    gBufferData.SpecularLevel = packedGBufferData.GBuffer5.y * 1024.0;
    gBufferData.SpecularFresnel = packedGBufferData.GBuffer4.y;

    gBufferData.Normal = DecodeUnorm11(packedGBufferData.GBuffer6) * 2.0 - 1.0;
    gBufferData.Falloff = DecodeUnorm11(packedGBufferData.GBuffer7) * 2.0;
    gBufferData.Emission = packedGBufferData.GBuffer8.rgb;
    
    gBufferData.Refraction = packedGBufferData.GBuffer9.x;
    gBufferData.RefractionOverlay = packedGBufferData.GBuffer9.y;
    gBufferData.RefractionOffset = packedGBufferData.GBuffer10 * 2.0 - 1.0;
    
    float lengthSquared = dot(gBufferData.Normal, gBufferData.Normal);
    gBufferData.Normal = select(lengthSquared > 0.0, gBufferData.Normal * rsqrt(lengthSquared), 0.0);

    return gBufferData;
}

PackedGBufferData PackGBufferData(GBufferData gBufferData)
{
    PackedGBufferData packedGBufferData = (PackedGBufferData) 0;
    
    packedGBufferData.GBuffer0 = float4(gBufferData.Position, asfloat(gBufferData.Flags));
    packedGBufferData.GBuffer1 = EncodeUnorm11(gBufferData.Diffuse / 2.0);
    packedGBufferData.GBuffer2 = EncodeUnorm11(gBufferData.Specular / 2.0);
    packedGBufferData.GBuffer3 = EncodeUnorm11(gBufferData.SpecularTint);
    packedGBufferData.GBuffer4 = float2(gBufferData.SpecularEnvironment, gBufferData.SpecularFresnel);
    packedGBufferData.GBuffer5 = float2(gBufferData.SpecularGloss, gBufferData.SpecularLevel) / 1024.0;
    packedGBufferData.GBuffer6 = EncodeUnorm11(gBufferData.Normal * 0.5 + 0.5);
    packedGBufferData.GBuffer7 = EncodeUnorm11(gBufferData.Falloff / 2.0);
    packedGBufferData.GBuffer8 = float4(gBufferData.Emission, gBufferData.Alpha);
    packedGBufferData.GBuffer9 = float2(gBufferData.Refraction, gBufferData.RefractionOverlay);
    packedGBufferData.GBuffer10 = gBufferData.RefractionOffset * 0.5 + 0.5;
    
    return packedGBufferData;
}

GBufferData LoadGBufferData(uint3 index)
{
    PackedGBufferData packedGBufferData = (PackedGBufferData) 0;
    
    packedGBufferData.GBuffer0 = g_GBuffer0_SRV[index];
    packedGBufferData.GBuffer1 = g_GBuffer1_SRV[index];
    packedGBufferData.GBuffer2 = g_GBuffer2_SRV[index];
    packedGBufferData.GBuffer3 = g_GBuffer3_SRV[index];
    packedGBufferData.GBuffer4 = g_GBuffer4_SRV[index];
    packedGBufferData.GBuffer5 = g_GBuffer5_SRV[index];
    packedGBufferData.GBuffer6 = g_GBuffer6_SRV[index];
    packedGBufferData.GBuffer7 = g_GBuffer7_SRV[index];
    packedGBufferData.GBuffer8 = g_GBuffer8_SRV[index];
    packedGBufferData.GBuffer9 = g_GBuffer9_SRV[index];
    packedGBufferData.GBuffer10 = g_GBuffer10_SRV[index];
    
    return UnpackGBufferData(packedGBufferData);
}

void StoreGBufferData(uint3 index, GBufferData gBufferData)
{
    PackedGBufferData packedGBufferData = PackGBufferData(gBufferData);
    
    g_GBuffer0[index] = packedGBufferData.GBuffer0;
    g_GBuffer1[index] = packedGBufferData.GBuffer1;
    g_GBuffer2[index] = packedGBufferData.GBuffer2;
    g_GBuffer3[index] = packedGBufferData.GBuffer3;
    g_GBuffer4[index] = packedGBufferData.GBuffer4;
    g_GBuffer5[index] = packedGBufferData.GBuffer5;
    g_GBuffer6[index] = packedGBufferData.GBuffer6;
    g_GBuffer7[index] = packedGBufferData.GBuffer7;
    g_GBuffer8[index] = packedGBufferData.GBuffer8;
    g_GBuffer9[index] = packedGBufferData.GBuffer9;
    g_GBuffer10[index] =packedGBufferData.GBuffer10;
}