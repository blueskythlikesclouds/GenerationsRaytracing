#ifndef SHADER_GLOBALS_H_INCLUDED
#define SHADER_GLOBALS_H_INCLUDED

cbuffer cbGlobalsVS : register(b0)
{
    row_major float4x4 g_MtxProjection : packoffset(c0);
    row_major float4x4 g_MtxView : packoffset(c4);
    row_major float4x4 g_MtxWorld : packoffset(c8);
    row_major float4x4 g_MtxWorldIT : packoffset(c12);
    row_major float4x4 g_MtxLightViewProjection : packoffset(c24);
    float4 g_EyePosition : packoffset(c178);
    float4 g_EyeDirection : packoffset(c179);
    float4 g_ViewportSize : packoffset(c180);
    float4 mrgGlobalLight_Direction : packoffset(c183);
    float4 mrgGlobalLight_Diffuse : packoffset(c184);
    float4 mrgGIAtlasParam : packoffset(c186);
    float4 mrgTexcoordIndex : packoffset(c187);
    float4 mrgTexcoordOffset : packoffset(c191);
    float4 mrgFresnelParam : packoffset(c193);
    float4 mrgZOffsetRate : packoffset(c195);
    float4 g_LightScattering_Ray_Mie_Ray2_Mie2 : packoffset(c197);
    float4 g_LightScattering_ConstG_FogDensity : packoffset(c198);
    float4 g_LightScatteringFarNearScale : packoffset(c199);
    row_major float4x4 g_MtxBillboardY : packoffset(c202);
    row_major float4x4 g_MtxVerticalLightViewProjection : packoffset(c207);
    float4 g_VerticalLightDirection : packoffset(c211);
    float4 g_TimeParam : packoffset(c212);
    float4 g_aLightField : packoffset(c213);
    float4 g_SkyParam : packoffset(c219);
    float4 g_ViewZAlphaFade : packoffset(c220);
};

cbuffer cbGlobalsPS : register(b1)
{
    row_major float4x4 g_MtxInvProjection : packoffset(c4);
    float4 g_Diffuse : packoffset(c16);
    float4 g_Ambient : packoffset(c17);
    float4 g_Specular : packoffset(c18);
    float4 g_Emission : packoffset(c19);
    float4 g_PowerGlossLevel : packoffset(c20);
    float4 g_OpacityReflectionRefractionSpectype : packoffset(c21);
    float4 g_CameraNearFarAspect : packoffset(c25);
    float4 mrgEmissionPower : packoffset(c34);
    float4 mrgGlobalLight_Specular : packoffset(c37);
    float4 mrgEyeLight_Diffuse : packoffset(c58);
    float4 mrgEyeLight_Specular : packoffset(c59);
    float4 mrgEyeLight_Range : packoffset(c60);
    float4 mrgEyeLight_Attribute : packoffset(c61);
    float4 mrgLuminanceRange : packoffset(c63);
    float4 mrgInShadowScale : packoffset(c64);
    float4 g_ShadowMapParams : packoffset(c65);
    float4 g_BackGroundScale : packoffset(c67);
    float4 g_GI0Scale : packoffset(c70);
    float4 g_LightScatteringColor : packoffset(c75);
    float4 mrgPlayableParam : packoffset(c86);
    float4 mrgEdgeEmissionParam : packoffset(c88);
    float4 g_ForceAlphaColor : packoffset(c89);
};

cbuffer cbGlobalsRT : register(b2)
{
    row_major float4x4 g_MtxPrevProjection;
    row_major float4x4 g_MtxPrevView;

    float2 g_PixelJitter;
    uint g_CurrentFrame;

    float3 g_EnvironmentColor;
};

#endif