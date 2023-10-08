#ifndef ROOT_SIGNATURE_H
#define ROOT_SIGNATURE_H

#include "GeometryDesc.hlsli"
#include "MaterialData.hlsli"

cbuffer GlobalsVS : register(b0)
{
    row_major float4x4 g_MtxProjection : packoffset(c0);
    row_major float4x4 g_MtxView : packoffset(c4);
    row_major float4x4 g_MtxWorld : packoffset(c8);
    row_major float4x4 g_MtxWorldIT : packoffset(c12);
    row_major float4x4 g_MtxPrevView : packoffset(c16);
    row_major float4x4 g_MtxPrevWorld : packoffset(c20);
    row_major float4x4 g_MtxLightViewProjection : packoffset(c24);
    row_major float3x4 g_MtxPalette[25] : packoffset(c28);
    row_major float3x4 g_MtxPrevPalette[25] : packoffset(c103);
    float4 g_EyePosition : packoffset(c178);
    float4 g_EyeDirection : packoffset(c179);
    float4 g_ViewportSize : packoffset(c180);
    float4 g_CameraNearFarAspect : packoffset(c181);
    float4 mrgAmbientColor : packoffset(c182);
    float4 mrgGlobalLight_Direction : packoffset(c183);
    float4 mrgGlobalLight_Diffuse : packoffset(c184);
    float4 mrgGlobalLight_Specular : packoffset(c185);
    float4 mrgGIAtlasParam : packoffset(c186);
    float4 mrgTexcoordIndex[4] : packoffset(c187);
    float4 mrgTexcoordOffset[2] : packoffset(c191);
    float4 mrgFresnelParam : packoffset(c193);
    float4 mrgMorphWeight : packoffset(c194);
    float4 mrgZOffsetRate : packoffset(c195);
    float4 g_IndexCount : packoffset(c196);
    float4 g_LightScattering_Ray_Mie_Ray2_Mie2 : packoffset(c197);
    float4 g_LightScattering_ConstG_FogDensity : packoffset(c198);
    float4 g_LightScatteringFarNearScale : packoffset(c199);
    float4 g_LightScatteringColor : packoffset(c200);
    float4 g_LightScatteringMode : packoffset(c201);
    row_major float4x4 g_MtxBillboardY : packoffset(c202);
    float4 mrgLocallightIndexArray : packoffset(c206);
    row_major float4x4 g_MtxVerticalLightViewProjection : packoffset(c207);
    float4 g_VerticalLightDirection : packoffset(c211);
    float4 g_TimeParam : packoffset(c212);
    float4 g_aLightField[6] : packoffset(c213);
    float4 g_SkyParam : packoffset(c219);
    float4 g_ViewZAlphaFade : packoffset(c220);
    float4 g_PowerGlossLevel : packoffset(c245);
}

cbuffer GlobalsPS : register(b1)
{
    row_major float4x4 g_MtxInvProjection : packoffset(c4);
    float4 mrgGlobalLight_Direction_View : packoffset(c11);
    float4 g_Diffuse : packoffset(c16);
    float4 g_Ambient : packoffset(c17);
    float4 g_Specular : packoffset(c18);
    float4 g_Emissive : packoffset(c19);
    float4 g_OpacityReflectionRefractionSpectype : packoffset(c21);
    float4 mrgGroundColor : packoffset(c31);
    float4 mrgSkyColor : packoffset(c32);
    float4 mrgPowerGlossLevel : packoffset(c33);
    float4 mrgEmissionPower : packoffset(c34);
    float4 mrgLocalLight0_Position : packoffset(c38);
    float4 mrgLocalLight0_Color : packoffset(c39);
    float4 mrgLocalLight0_Range : packoffset(c40);
    float4 mrgLocalLight0_Attribute : packoffset(c41);
    float4 mrgLocalLight1_Position : packoffset(c42);
    float4 mrgLocalLight1_Color : packoffset(c43);
    float4 mrgLocalLight1_Range : packoffset(c44);
    float4 mrgLocalLight1_Attribute : packoffset(c45);
    float4 mrgLocalLight2_Position : packoffset(c46);
    float4 mrgLocalLight2_Color : packoffset(c47);
    float4 mrgLocalLight2_Range : packoffset(c48);
    float4 mrgLocalLight2_Attribute : packoffset(c49);
    float4 mrgLocalLight3_Position : packoffset(c50);
    float4 mrgLocalLight3_Color : packoffset(c51);
    float4 mrgLocalLight3_Range : packoffset(c52);
    float4 mrgLocalLight3_Attribute : packoffset(c53);
    float4 mrgLocalLight4_Position : packoffset(c54);
    float4 mrgLocalLight4_Color : packoffset(c55);
    float4 mrgLocalLight4_Range : packoffset(c56);
    float4 mrgLocalLight4_Attribute : packoffset(c57);
    float4 mrgEyeLight_Diffuse : packoffset(c58);
    float4 mrgEyeLight_Specular : packoffset(c59);
    float4 mrgEyeLight_Range : packoffset(c60);
    float4 mrgEyeLight_Attribute : packoffset(c61);
    float4 mrgLuminanceRange : packoffset(c63);
    float4 mrgInShadowScale : packoffset(c64);
    float4 g_ShadowMapParams : packoffset(c65);
    float4 mrgColourCompressFactor : packoffset(c66);
    float4 g_BackGroundScale : packoffset(c67);
    float4 g_GIModeParam : packoffset(c69);
    float4 g_GI0Scale : packoffset(c70);
    float4 g_GI1Scale : packoffset(c71);
    float4 g_MotionBlur_AlphaRef_VelocityLimit_VelocityCutoff_BlurMagnitude : packoffset(c85);
    float4 mrgPlayableParam : packoffset(c86);
    float4 mrgDebugDistortionParam : packoffset(c87);
    float4 mrgEdgeEmissionParam : packoffset(c88);
    float4 g_ForceAlphaColor : packoffset(c89);
    row_major float4x4 g_MtxInvView : packoffset(c94);
    float4 mrgVsmEpsilon : packoffset(c148);
    float4 g_DebugValue : packoffset(c149);
}

cbuffer GlobalsRT : register(b2)
{
    float3 g_EnvironmentColor;
    uint g_CurrentFrame;
    float2 g_PixelJitter;
    uint2 g_BlueNoiseOffset;
    uint g_BlueNoiseTextureId;
}

RaytracingAccelerationStructure g_BVH : register(t0);
StructuredBuffer<GeometryDesc> g_GeometryDescs : register(t1);
StructuredBuffer<Material> g_Materials : register(t2);

RWTexture2D<float4> g_PositionAndFlagsTexture : register(u0);
RWTexture2D<float3> g_NormalTexture : register(u1);
RWTexture2D<float3> g_DiffuseTexture : register(u2);
RWTexture2D<float3> g_SpecularTexture : register(u3);
RWTexture2D<float> g_SpecularPowerTexture : register(u4);
RWTexture2D<float> g_SpecularLevelTexture : register(u5);
RWTexture2D<float3> g_EmissionTexture : register(u6);
RWTexture2D<float3> g_FalloffTexture : register(u7);

RWTexture2D<float> g_ShadowTexture : register(u8);
RWTexture2D<float3> g_GlobalIlluminationTexture : register(u9);
RWTexture2D<float3> g_ReflectionTexture : register(u10);

#endif