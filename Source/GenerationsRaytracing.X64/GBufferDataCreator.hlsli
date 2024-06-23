#pragma once

#include "GBufferData.hlsli"

#if SHADER_TYPE == SHADER_TYPE_BLEND
#include "Blend.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_COMMON
#include "Common.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_INDIRECT
#include "Indirect.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_LUMINESCENCE
#include "Luminescence.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_CHR_EYE
#include "ChrEye.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_CHR_EYE_FHL || SHADER_TYPE == SHADER_TYPE_CHR_EYE_FHL_PROCEDURAL
#include "ChrEyeFHL.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_CHR_SKIN
#include "ChrSkin.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_CHR_SKIN_HALF
#include "ChrSkinHalf.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_CHR_SKIN_IGNORE
#include "ChrSkinIgnore.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_CLOUD
#include "Cloud.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_DIM
#include "Dim.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_DISTORTION
#include "Distortion.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_DISTORTION_OVERLAY
#include "DistortionOverlay.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_ENM_EMISSION
#include "EnmEmission.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_ENM_GLASS
#include "EnmGlass.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_ENM_IGNORE
#include "EnmIgnore.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_FADE_OUT_NORMAL
#include "FadeOutNormal.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_FALLOFF
#include "FallOff.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_FALLOFF_V
#include "FallOffV.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_FUR
#include "Fur.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_GLASS
#include "Glass.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_ICE
#include "Ice.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_IGNORE_LIGHT
#include "IgnoreLight.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_IGNORE_LIGHT_TWICE
#include "IgnoreLightTwice.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_INDIRECT_NO_LIGHT
#include "IndirectNoLight.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_INDIRECT_V
#include "IndirectV.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_LUMINESCENCE_V
#include "LuminescenceV.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_METAL
#include "Metal.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_MIRROR
#include "Mirror.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_RING
#include "Ring.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_SHOE
#include "Shoe.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_TIME_EATER
#include "TimeEater.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_TRANS_THIN
#include "TransThin.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_WATER_ADD || SHADER_TYPE == SHADER_TYPE_WATER_MUL || SHADER_TYPE == SHADER_TYPE_WATER_OPACITY
#include "Water.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_CHAOS
#include "Chaos.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_CLOAK
#include "Cloak.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_GLASS_REFRACTION
#include "GlassRefraction.hlsli"
#elif SHADER_TYPE == SHADER_TYPE_LAVA
#include "Lava.hlsli"
#endif

GBufferData CreateGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, bool storeSafeSpawnPoint)
{
    GBufferData gBufferData = (GBufferData) 0;

    gBufferData.Position = storeSafeSpawnPoint ? vertex.SafeSpawnPoint : vertex.Position;
    gBufferData.Diffuse = material.Diffuse.rgb;
    gBufferData.Alpha = material.Diffuse.a * material.Opacity.x;
    gBufferData.Specular = material.Specular.rgb;
    gBufferData.SpecularTint = 1.0;
    gBufferData.SpecularEnvironment = material.LuminanceRange + 1.0;
    gBufferData.SpecularGloss = material.GlossLevel.x * 500.0;
    gBufferData.SpecularLevel = material.GlossLevel.y * 5.0;
    gBufferData.Normal = vertex.Normal;

#if SHADER_TYPE == SHADER_TYPE_BLEND
    CreateBlendGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_COMMON
    CreateCommonGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_INDIRECT
    CreateIndirectGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_LUMINESCENCE
    CreateLuminescenceGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_CHR_EYE
    CreateChrEyeGBufferData(vertex, material, instanceDesc, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_CHR_EYE_FHL || SHADER_TYPE == SHADER_TYPE_CHR_EYE_FHL_PROCEDURAL
    CreateChrEyeFHLGBufferData(vertex, material, instanceDesc, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_CHR_SKIN
    CreateChrSkinGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_CHR_SKIN_HALF
    CreateChrSkinHalfGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_CHR_SKIN_IGNORE
    CreateChrSkinIgnoreGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_CLOUD
    CreateCloudGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_DIM
    CreateDimGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_DISTORTION
    CreateDistortionGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_DISTORTION_OVERLAY
    CreateDistortionOverlayGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_ENM_EMISSION
    CreateEnmEmissionGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_ENM_GLASS
    CreateEnmGlassGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_ENM_IGNORE
    CreateEnmIgnoreGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_FADE_OUT_NORMAL
    CreateFadeOutNormalGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_FALLOFF
    CreateFallOffGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_FALLOFF_V
    CreateFallOffVGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_FUR
    CreateFurGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_GLASS
    CreateGlassGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_ICE
    CreateIceGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_IGNORE_LIGHT
    CreateIgnoreLightGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_IGNORE_LIGHT_TWICE
    CreateIgnoreLightTwiceGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_INDIRECT_NO_LIGHT
    CreateIndirectNoLightGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_INDIRECT_V
    CreateIndirectVGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_LUMINESCENCE_V
    CreateLuminescenceVGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_METAL
    CreateMetalGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_MIRROR
    CreateMirrorGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_RING
    CreateRingGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_SHOE
    CreateShoeGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_TIME_EATER
    CreateTimeEaterGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_TRANS_THIN
    CreateTransThinGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_WATER_ADD || SHADER_TYPE == SHADER_TYPE_WATER_MUL || SHADER_TYPE == SHADER_TYPE_WATER_OPACITY
    CreateWaterGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_CHAOS
    CreateChaosGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_CLOAK
    CreateCloakGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_GLASS_REFRACTION
    CreateGlassRefractionGBufferData(vertex, material, gBufferData);
#elif SHADER_TYPE == SHADER_TYPE_LAVA
    CreateLavaGBufferData(vertex, material, gBufferData);
#else
    gBufferData.Flags =
        GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT |
        GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION | GBUFFER_FLAG_IGNORE_REFLECTION;

    gBufferData.Diffuse = 0.0;
    gBufferData.Emission = float3(1.0, 0.0, 0.0);
#endif

    if (material.Flags & MATERIAL_FLAG_ADDITIVE)
        gBufferData.Flags |= GBUFFER_FLAG_IS_ADDITIVE;

    if (material.Flags & MATERIAL_FLAG_REFLECTION)
        gBufferData.Flags |= GBUFFER_FLAG_IS_MIRROR_REFLECTION;
    
    if (material.Flags & MATERIAL_FLAG_FULBRIGHT)
        gBufferData.Flags |= GBUFFER_FLAG_FULBRIGHT;
    
    gBufferData.Emission += instanceDesc.EdgeEmissionParam * float3(0.0, 0.8, 0.5) *
        (1.0 - sqrt(saturate(dot(gBufferData.Normal, -WorldRayDirection()))));

    float playableParam = saturate(64.0 * (ComputeNdcPosition(vertex.Position, g_MtxView, g_MtxProjection).y - instanceDesc.PlayableParam));
    playableParam *= saturate((instanceDesc.ChrPlayableMenuParam - vertex.Position.y + 0.05) * 10);
    gBufferData.Diffuse = lerp(1.0, gBufferData.Diffuse, playableParam);
    gBufferData.Emission *= playableParam;

    if (material.Flags & MATERIAL_FLAG_VIEW_Z_ALPHA_FADE)
        gBufferData.Alpha *= 1.0 - saturate((RayTCurrent() - g_ViewZAlphaFade.y) * g_ViewZAlphaFade.x);

    gBufferData.SpecularGloss = clamp(gBufferData.SpecularGloss, 1.0, 1024.0);

    bool diffuseMask = all(gBufferData.Diffuse == 0.0);

    if (diffuseMask)
        gBufferData.Flags |= GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION;

    bool specularMask = (or(all(gBufferData.Specular == 0.0), all(gBufferData.SpecularTint == 0.0)) || 
        gBufferData.SpecularLevel == 0.0) && !(gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION);

    if (specularMask)
        gBufferData.Flags |= GBUFFER_FLAG_IGNORE_SPECULAR_LIGHT | GBUFFER_FLAG_IGNORE_REFLECTION;

    diffuseMask |= (gBufferData.Flags & GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT) != 0;
    specularMask |= (gBufferData.Flags & GBUFFER_FLAG_IGNORE_SPECULAR_LIGHT) != 0;

    if (diffuseMask && specularMask)
        gBufferData.Flags |= GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT;

    if ((material.Flags & MATERIAL_FLAG_NO_SHADOW) || (gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT))
        gBufferData.Flags |= GBUFFER_FLAG_IGNORE_SHADOW;

    return gBufferData;
}