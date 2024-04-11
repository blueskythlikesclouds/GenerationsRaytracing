#pragma once

#include "ShaderType.h"

struct RaytracingTexture
{
   const char* name;
   uint32_t index; 
   bool isOpacity;
   bool isReflection;
   Hedgehog::Base::CStringSymbol nameSymbol;
};

struct RaytracingParameter
{
   const char* name;
   uint32_t index;
   uint32_t size;
   Hedgehog::Base::CStringSymbol nameSymbol;
};

struct RaytracingShader
{
   uint32_t type;
   RaytracingTexture* textures;
   uint32_t textureNum;
   RaytracingParameter* parameters;
   uint32_t parameterNum;
};
inline RaytracingTexture s_textures_SYS_ERROR[1] =
{
	{ "diffuse", 0, false, false },
};
inline RaytracingParameter s_parameters_SYS_ERROR[2] =
{
	{ "diffuse", 0, 4 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_SYS_ERROR = { SHADER_TYPE_SYS_ERROR, s_textures_SYS_ERROR, 1, s_parameters_SYS_ERROR, 2 };
inline RaytracingTexture s_textures_BLEND[9] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
	{ "opacity", 0, true, false },
	{ "diffuse", 1, false, false },
	{ "specular", 1, false, false },
	{ "gloss", 1, false, false },
	{ "normal", 1, false, false },
};
inline RaytracingParameter s_parameters_BLEND[4] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_BLEND = { SHADER_TYPE_BLEND, s_textures_BLEND, 9, s_parameters_BLEND, 4 };
inline RaytracingTexture s_textures_COMMON[5] =
{
	{ "diffuse", 0, false, false },
	{ "opacity", 0, true, false },
	{ "specular", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
};
inline RaytracingParameter s_parameters_COMMON[4] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_COMMON = { SHADER_TYPE_COMMON, s_textures_COMMON, 5, s_parameters_COMMON, 4 };
inline RaytracingTexture s_textures_INDIRECT[4] =
{
	{ "diffuse", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
	{ "displacement", 0, false, false },
};
inline RaytracingParameter s_parameters_INDIRECT[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_OffsetParam", 0, 2 },
};
inline RaytracingShader s_shader_INDIRECT = { SHADER_TYPE_INDIRECT, s_textures_INDIRECT, 4, s_parameters_INDIRECT, 5 };
inline RaytracingTexture s_textures_LUMINESCENCE[4] =
{
	{ "diffuse", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
	{ "displacement", 0, false, false },
};
inline RaytracingParameter s_parameters_LUMINESCENCE[6] =
{
	{ "diffuse", 0, 4 },
	{ "emissive", 0, 3 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "ambient", 0, 3 },
};
inline RaytracingShader s_shader_LUMINESCENCE = { SHADER_TYPE_LUMINESCENCE, s_textures_LUMINESCENCE, 4, s_parameters_LUMINESCENCE, 6 };
inline RaytracingTexture s_textures_CHR_EYE[3] =
{
	{ "diffuse", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
};
inline RaytracingParameter s_parameters_CHR_EYE[6] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_SonicEyeHighLightPositionRaytracing", 0, 3 },
	{ "g_SonicEyeHighLightColor", 0, 3 },
};
inline RaytracingShader s_shader_CHR_EYE = { SHADER_TYPE_CHR_EYE, s_textures_CHR_EYE, 3, s_parameters_CHR_EYE, 6 };
inline RaytracingTexture s_textures_CHR_EYE_FHL[3] =
{
	{ "diffuse", 0, false, false },
	{ "level", 0, false, false },
	{ "displacement", 0, false, false },
};
inline RaytracingParameter s_parameters_CHR_EYE_FHL[7] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "ChrEyeFHL1", 0, 4 },
	{ "ChrEyeFHL2", 0, 4 },
	{ "ChrEyeFHL3", 0, 4 },
};
inline RaytracingShader s_shader_CHR_EYE_FHL = { SHADER_TYPE_CHR_EYE_FHL, s_textures_CHR_EYE_FHL, 3, s_parameters_CHR_EYE_FHL, 7 };
inline RaytracingTexture s_textures_CHR_SKIN[5] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
	{ "displacement", 0, false, false },
};
inline RaytracingParameter s_parameters_CHR_SKIN[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_SonicSkinFalloffParam", 0, 3 },
};
inline RaytracingShader s_shader_CHR_SKIN = { SHADER_TYPE_CHR_SKIN, s_textures_CHR_SKIN, 5, s_parameters_CHR_SKIN, 5 };
inline RaytracingTexture s_textures_CHR_SKIN_HALF[3] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "reflection", 0, false, true },
};
inline RaytracingParameter s_parameters_CHR_SKIN_HALF[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_SonicSkinFalloffParam", 0, 3 },
};
inline RaytracingShader s_shader_CHR_SKIN_HALF = { SHADER_TYPE_CHR_SKIN_HALF, s_textures_CHR_SKIN_HALF, 3, s_parameters_CHR_SKIN_HALF, 5 };
inline RaytracingTexture s_textures_CHR_SKIN_IGNORE[4] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "displacement", 0, false, false },
	{ "reflection", 0, false, true },
};
inline RaytracingParameter s_parameters_CHR_SKIN_IGNORE[7] =
{
	{ "diffuse", 0, 4 },
	{ "ambient", 0, 3 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_SonicSkinFalloffParam", 0, 3 },
	{ "mrgChrEmissionParam", 0, 4 },
};
inline RaytracingShader s_shader_CHR_SKIN_IGNORE = { SHADER_TYPE_CHR_SKIN_IGNORE, s_textures_CHR_SKIN_IGNORE, 4, s_parameters_CHR_SKIN_IGNORE, 7 };
inline RaytracingTexture s_textures_CLOUD[3] =
{
	{ "normal", 0, false, false },
	{ "displacement", 0, false, false },
	{ "reflection", 0, false, true },
};
inline RaytracingParameter s_parameters_CLOUD[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_SonicSkinFalloffParam", 0, 3 },
};
inline RaytracingShader s_shader_CLOUD = { SHADER_TYPE_CLOUD, s_textures_CLOUD, 3, s_parameters_CLOUD, 5 };
inline RaytracingTexture s_textures_DIM[4] =
{
	{ "diffuse", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
	{ "reflection", 0, false, true },
};
inline RaytracingParameter s_parameters_DIM[5] =
{
	{ "diffuse", 0, 4 },
	{ "ambient", 0, 3 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_DIM = { SHADER_TYPE_DIM, s_textures_DIM, 4, s_parameters_DIM, 5 };
inline RaytracingTexture s_textures_DISTORTION[4] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "normal", 0, false, false },
	{ "normal", 1, false, false },
};
inline RaytracingParameter s_parameters_DISTORTION[4] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_DISTORTION = { SHADER_TYPE_DISTORTION, s_textures_DISTORTION, 4, s_parameters_DISTORTION, 4 };
inline RaytracingTexture s_textures_DISTORTION_OVERLAY[3] =
{
	{ "diffuse", 0, false, false },
	{ "normal", 0, false, false },
	{ "normal", 1, false, false },
};
inline RaytracingParameter s_parameters_DISTORTION_OVERLAY[3] =
{
	{ "diffuse", 0, 4 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "mrgDistortionParam", 0, 4 },
};
inline RaytracingShader s_shader_DISTORTION_OVERLAY = { SHADER_TYPE_DISTORTION_OVERLAY, s_textures_DISTORTION_OVERLAY, 3, s_parameters_DISTORTION_OVERLAY, 3 };
inline RaytracingTexture s_textures_ENM_EMISSION[4] =
{
	{ "diffuse", 0, false, false },
	{ "normal", 0, false, false },
	{ "specular", 0, false, false },
	{ "displacement", 0, false, false },
};
inline RaytracingParameter s_parameters_ENM_EMISSION[6] =
{
	{ "diffuse", 0, 4 },
	{ "ambient", 0, 3 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "mrgChrEmissionParam", 0, 4 },
};
inline RaytracingShader s_shader_ENM_EMISSION = { SHADER_TYPE_ENM_EMISSION, s_textures_ENM_EMISSION, 4, s_parameters_ENM_EMISSION, 6 };
inline RaytracingTexture s_textures_ENM_GLASS[5] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "normal", 0, false, false },
	{ "displacement", 0, false, false },
	{ "reflection", 0, false, true },
};
inline RaytracingParameter s_parameters_ENM_GLASS[6] =
{
	{ "diffuse", 0, 4 },
	{ "ambient", 0, 3 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "mrgChrEmissionParam", 0, 4 },
};
inline RaytracingShader s_shader_ENM_GLASS = { SHADER_TYPE_ENM_GLASS, s_textures_ENM_GLASS, 5, s_parameters_ENM_GLASS, 6 };
inline RaytracingTexture s_textures_ENM_IGNORE[3] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "displacement", 0, false, false },
};
inline RaytracingParameter s_parameters_ENM_IGNORE[6] =
{
	{ "diffuse", 0, 4 },
	{ "ambient", 0, 3 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "mrgChrEmissionParam", 0, 4 },
};
inline RaytracingShader s_shader_ENM_IGNORE = { SHADER_TYPE_ENM_IGNORE, s_textures_ENM_IGNORE, 3, s_parameters_ENM_IGNORE, 6 };
inline RaytracingTexture s_textures_FADE_OUT_NORMAL[2] =
{
	{ "diffuse", 0, false, false },
	{ "normal", 0, false, false },
};
inline RaytracingParameter s_parameters_FADE_OUT_NORMAL[4] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_FADE_OUT_NORMAL = { SHADER_TYPE_FADE_OUT_NORMAL, s_textures_FADE_OUT_NORMAL, 2, s_parameters_FADE_OUT_NORMAL, 4 };
inline RaytracingTexture s_textures_FALLOFF[4] =
{
	{ "diffuse", 0, false, false },
	{ "normal", 0, false, false },
	{ "gloss", 0, false, false },
	{ "displacement", 0, false, false },
};
inline RaytracingParameter s_parameters_FALLOFF[4] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_FALLOFF = { SHADER_TYPE_FALLOFF, s_textures_FALLOFF, 4, s_parameters_FALLOFF, 4 };
inline RaytracingTexture s_textures_FALLOFF_V[4] =
{
	{ "diffuse", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
	{ "normal", 1, false, false },
};
inline RaytracingParameter s_parameters_FALLOFF_V[4] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_FALLOFF_V = { SHADER_TYPE_FALLOFF_V, s_textures_FALLOFF_V, 4, s_parameters_FALLOFF_V, 4 };
inline RaytracingTexture s_textures_FUR[6] =
{
	{ "diffuse", 0, false, false },
	{ "diffuse", 1, false, false },
	{ "specular", 0, false, false },
	{ "normal", 0, false, false },
	{ "normal", 1, false, false },
	{ "displacement", 0, false, false },
};
inline RaytracingParameter s_parameters_FUR[7] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_SonicSkinFalloffParam", 0, 3 },
	{ "FurParam", 0, 4 },
	{ "FurParam2", 0, 4 },
};
inline RaytracingShader s_shader_FUR = { SHADER_TYPE_FUR, s_textures_FUR, 6, s_parameters_FUR, 7 };
inline RaytracingTexture s_textures_GLASS[4] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
};
inline RaytracingParameter s_parameters_GLASS[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "mrgFresnelParam", 0, 2 },
};
inline RaytracingShader s_shader_GLASS = { SHADER_TYPE_GLASS, s_textures_GLASS, 4, s_parameters_GLASS, 5 };
inline RaytracingTexture s_textures_ICE[3] =
{
	{ "diffuse", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
};
inline RaytracingParameter s_parameters_ICE[4] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_ICE = { SHADER_TYPE_ICE, s_textures_ICE, 3, s_parameters_ICE, 4 };
inline RaytracingTexture s_textures_IGNORE_LIGHT[3] =
{
	{ "diffuse", 0, false, false },
	{ "opacity", 0, true, false },
	{ "displacement", 0, false, false },
};
inline RaytracingParameter s_parameters_IGNORE_LIGHT[4] =
{
	{ "diffuse", 0, 4 },
	{ "ambient", 0, 3 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_EmissionParam", 0, 4 },
};
inline RaytracingShader s_shader_IGNORE_LIGHT = { SHADER_TYPE_IGNORE_LIGHT, s_textures_IGNORE_LIGHT, 3, s_parameters_IGNORE_LIGHT, 4 };
inline RaytracingTexture s_textures_IGNORE_LIGHT_TWICE[1] =
{
	{ "diffuse", 0, false, false },
};
inline RaytracingParameter s_parameters_IGNORE_LIGHT_TWICE[2] =
{
	{ "diffuse", 0, 4 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_IGNORE_LIGHT_TWICE = { SHADER_TYPE_IGNORE_LIGHT_TWICE, s_textures_IGNORE_LIGHT_TWICE, 1, s_parameters_IGNORE_LIGHT_TWICE, 2 };
inline RaytracingTexture s_textures_INDIRECT_NO_LIGHT[3] =
{
	{ "diffuse", 0, false, false },
	{ "displacement", 0, false, false },
	{ "displacement", 1, false, false },
};
inline RaytracingParameter s_parameters_INDIRECT_NO_LIGHT[3] =
{
	{ "diffuse", 0, 4 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_OffsetParam", 0, 2 },
};
inline RaytracingShader s_shader_INDIRECT_NO_LIGHT = { SHADER_TYPE_INDIRECT_NO_LIGHT, s_textures_INDIRECT_NO_LIGHT, 3, s_parameters_INDIRECT_NO_LIGHT, 3 };
inline RaytracingTexture s_textures_INDIRECT_V[5] =
{
	{ "diffuse", 0, false, false },
	{ "opacity", 0, true, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
	{ "displacement", 0, false, false },
};
inline RaytracingParameter s_parameters_INDIRECT_V[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_OffsetParam", 0, 2 },
};
inline RaytracingShader s_shader_INDIRECT_V = { SHADER_TYPE_INDIRECT_V, s_textures_INDIRECT_V, 5, s_parameters_INDIRECT_V, 5 };
inline RaytracingTexture s_textures_LUMINESCENCE_V[6] =
{
	{ "diffuse", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
	{ "displacement", 0, false, false },
	{ "diffuse", 1, false, false },
	{ "gloss", 1, false, false },
};
inline RaytracingParameter s_parameters_LUMINESCENCE_V[5] =
{
	{ "diffuse", 0, 4 },
	{ "ambient", 0, 3 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_LUMINESCENCE_V = { SHADER_TYPE_LUMINESCENCE_V, s_textures_LUMINESCENCE_V, 6, s_parameters_LUMINESCENCE_V, 5 };
inline RaytracingTexture s_textures_METAL[4] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
};
inline RaytracingParameter s_parameters_METAL[4] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_METAL = { SHADER_TYPE_METAL, s_textures_METAL, 4, s_parameters_METAL, 4 };
inline RaytracingTexture s_textures_MIRROR[1] =
{
	{ "diffuse", 0, false, false },
};
inline RaytracingParameter s_parameters_MIRROR[3] =
{
	{ "diffuse", 0, 4 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "mrgFresnelParam", 0, 2 },
};
inline RaytracingShader s_shader_MIRROR = { SHADER_TYPE_MIRROR, s_textures_MIRROR, 1, s_parameters_MIRROR, 3 };
inline RaytracingTexture s_textures_RING[3] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "reflection", 0, false, true },
};
inline RaytracingParameter s_parameters_RING[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "mrgLuminancerange", 0, 1 },
};
inline RaytracingShader s_shader_RING = { SHADER_TYPE_RING, s_textures_RING, 3, s_parameters_RING, 5 };
inline RaytracingTexture s_textures_SHOE[4] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
};
inline RaytracingParameter s_parameters_SHOE[4] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
};
inline RaytracingShader s_shader_SHOE = { SHADER_TYPE_SHOE, s_textures_SHOE, 4, s_parameters_SHOE, 4 };
inline RaytracingTexture s_textures_TIME_EATER[5] =
{
	{ "diffuse", 0, false, false },
	{ "specular", 0, false, false },
	{ "opacity", 0, true, false },
	{ "normal", 0, false, false },
	{ "normal", 1, false, false },
};
inline RaytracingParameter s_parameters_TIME_EATER[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "power_gloss_level", 1, 2 },
	{ "g_SonicSkinFalloffParam", 0, 3 },
};
inline RaytracingShader s_shader_TIME_EATER = { SHADER_TYPE_TIME_EATER, s_textures_TIME_EATER, 5, s_parameters_TIME_EATER, 5 };
inline RaytracingTexture s_textures_TRANS_THIN[3] =
{
	{ "diffuse", 0, false, false },
	{ "gloss", 0, false, false },
	{ "normal", 0, false, false },
};
inline RaytracingParameter s_parameters_TRANS_THIN[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_TransColorMask", 0, 3 },
};
inline RaytracingShader s_shader_TRANS_THIN = { SHADER_TYPE_TRANS_THIN, s_textures_TRANS_THIN, 3, s_parameters_TRANS_THIN, 5 };
inline RaytracingTexture s_textures_WATER_ADD[3] =
{
	{ "diffuse", 0, false, false },
	{ "normal", 0, false, false },
	{ "normal", 1, false, false },
};
inline RaytracingParameter s_parameters_WATER_ADD[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_WaterParam", 0, 4 },
};
inline RaytracingShader s_shader_WATER_ADD = { SHADER_TYPE_WATER_ADD, s_textures_WATER_ADD, 3, s_parameters_WATER_ADD, 5 };
inline RaytracingTexture s_textures_WATER_MUL[3] =
{
	{ "diffuse", 0, false, false },
	{ "normal", 0, false, false },
	{ "normal", 1, false, false },
};
inline RaytracingParameter s_parameters_WATER_MUL[5] =
{
	{ "diffuse", 0, 4 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "g_WaterParam", 0, 4 },
};
inline RaytracingShader s_shader_WATER_MUL = { SHADER_TYPE_WATER_MUL, s_textures_WATER_MUL, 3, s_parameters_WATER_MUL, 5 };
inline RaytracingTexture s_textures_WATER_OPACITY[3] =
{
	{ "diffuse", 0, false, false },
	{ "normal", 0, false, false },
	{ "normal", 1, false, false },
};
inline RaytracingParameter s_parameters_WATER_OPACITY[5] =
{
	{ "diffuse", 0, 4 },
	{ "opacity_reflection_refraction_spectype", 0, 1 },
	{ "specular", 0, 3 },
	{ "power_gloss_level", 1, 2 },
	{ "g_WaterParam", 0, 4 },
};
inline RaytracingShader s_shader_WATER_OPACITY = { SHADER_TYPE_WATER_OPACITY, s_textures_WATER_OPACITY, 3, s_parameters_WATER_OPACITY, 5 };
inline std::pair<std::string_view, RaytracingShader*> s_shaders[] =
{
	{ "BillboardParticle_", &s_shader_SYS_ERROR },
	{ "BillboardParticleY_", &s_shader_SYS_ERROR },
	{ "BlbBlend_", &s_shader_BLEND },
	{ "BlbCommon_", &s_shader_COMMON },
	{ "BlbIndirect_", &s_shader_INDIRECT },
	{ "BlbLuminescence_", &s_shader_LUMINESCENCE },
	{ "Blend_", &s_shader_BLEND },
	{ "Chaos_", &s_shader_SYS_ERROR },
	{ "ChaosV_", &s_shader_SYS_ERROR },
	{ "ChrEye_", &s_shader_CHR_EYE },
	{ "ChrEyeFHL", &s_shader_CHR_EYE_FHL },
	{ "ChrSkin_", &s_shader_CHR_SKIN },
	{ "ChrSkinHalf_", &s_shader_CHR_SKIN_HALF },
	{ "ChrSkinIgnore_", &s_shader_CHR_SKIN_IGNORE },
	{ "Cloak_", &s_shader_SYS_ERROR },
	{ "Cloth_", &s_shader_SYS_ERROR },
	{ "Cloud_", &s_shader_CLOUD },
	{ "Common_", &s_shader_COMMON },
	{ "Deformation_", &s_shader_SYS_ERROR },
	{ "DeformationParticle_", &s_shader_SYS_ERROR },
	{ "Dim_", &s_shader_DIM },
	{ "DimIgnore_", &s_shader_SYS_ERROR },
	{ "Distortion_", &s_shader_DISTORTION },
	{ "DistortionOverlay_", &s_shader_DISTORTION_OVERLAY },
	{ "DistortionOverlayChaos_", &s_shader_SYS_ERROR },
	{ "EnmCloud_", &s_shader_CLOUD },
	{ "EnmEmission_", &s_shader_ENM_EMISSION },
	{ "EnmGlass_", &s_shader_ENM_GLASS },
	{ "EnmIgnore_", &s_shader_ENM_IGNORE },
	{ "EnmMetal_", &s_shader_CHR_SKIN },
	{ "FadeOutNormal_", &s_shader_FADE_OUT_NORMAL },
	{ "FakeGlass_", &s_shader_ENM_GLASS },
	{ "FallOff_", &s_shader_FALLOFF },
	{ "FallOffV_", &s_shader_FALLOFF_V },
	{ "Fur", &s_shader_FUR },
	{ "Glass_", &s_shader_GLASS },
	{ "GlassRefraction_", &s_shader_SYS_ERROR },
	{ "Ice_", &s_shader_ICE },
	{ "IgnoreLight_", &s_shader_IGNORE_LIGHT },
	{ "IgnoreLightTwice_", &s_shader_IGNORE_LIGHT_TWICE },
	{ "IgnoreLightV_", &s_shader_SYS_ERROR },
	{ "Indirect_", &s_shader_INDIRECT },
	{ "IndirectNoLight_", &s_shader_INDIRECT_NO_LIGHT },
	{ "IndirectV_", &s_shader_INDIRECT_V },
	{ "IndirectVnoGIs_", &s_shader_INDIRECT_V },
	{ "Lava_", &s_shader_SYS_ERROR },
	{ "Luminescence_", &s_shader_LUMINESCENCE },
	{ "LuminescenceV_", &s_shader_LUMINESCENCE_V },
	{ "MeshParticle_", &s_shader_SYS_ERROR },
	{ "MeshParticleLightingShadow_", &s_shader_SYS_ERROR },
	{ "MeshParticleRef_", &s_shader_SYS_ERROR },
	{ "Metal", &s_shader_METAL },
	{ "Mirror_", &s_shader_MIRROR },
	{ "Mirror2_", &s_shader_MIRROR },
	{ "Myst_", &s_shader_SYS_ERROR },
	{ "Parallax_", &s_shader_SYS_ERROR },
	{ "Ring_", &s_shader_RING },
	{ "Shoe", &s_shader_SHOE },
	{ "TimeEater_", &s_shader_TIME_EATER },
	{ "TimeEaterDistortion_", &s_shader_DISTORTION },
	{ "TimeEaterEmission_", &s_shader_ENM_EMISSION },
	{ "TimeEaterGlass_", &s_shader_ENM_GLASS },
	{ "TimeEaterIndirect_", &s_shader_INDIRECT_NO_LIGHT },
	{ "TimeEaterMetal_", &s_shader_CHR_SKIN },
	{ "TransThin_", &s_shader_TRANS_THIN },
	{ "Water_Add", &s_shader_WATER_ADD },
	{ "Water_Mul", &s_shader_WATER_MUL },
	{ "Water_Opacity", &s_shader_WATER_OPACITY },
};
