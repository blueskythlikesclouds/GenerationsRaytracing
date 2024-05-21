#pragma once

#include "ShaderType.h"

struct RaytracingTexture
{
   const char* name;
   uint32_t index; 
   bool isOpacity;
   Hedgehog::Base::CStringSymbol nameSymbol;
};

struct RaytracingParameter
{
   const char* name;
   uint32_t index;
   uint32_t size;
   float x;
   float y;
   float z;
   float w;
   Hedgehog::Base::CStringSymbol nameSymbol;
};

struct RaytracingShader
{
   uint32_t type;
   RaytracingTexture** textures;
   uint32_t textureCount;
   RaytracingParameter** parameters;
   uint32_t parameterCount;
};

inline RaytracingTexture s_texture_DiffuseTexture = { "diffuse", 0, false };
inline RaytracingTexture s_texture_DiffuseTexture2 = { "diffuse", 1, false };
inline RaytracingTexture s_texture_SpecularTexture = { "specular", 0, false };
inline RaytracingTexture s_texture_SpecularTexture2 = { "specular", 1, false };
inline RaytracingTexture s_texture_GlossTexture = { "gloss", 0, false };
inline RaytracingTexture s_texture_GlossTexture2 = { "gloss", 1, false };
inline RaytracingTexture s_texture_NormalTexture = { "normal", 0, false };
inline RaytracingTexture s_texture_NormalTexture2 = { "normal", 1, false };
inline RaytracingTexture s_texture_ReflectionTexture = { "reflection", 0, false };
inline RaytracingTexture s_texture_OpacityTexture = { "opacity", 0, true };
inline RaytracingTexture s_texture_DisplacementTexture = { "displacement", 0, false };
inline RaytracingTexture s_texture_DisplacementTexture2 = { "displacement", 1, false };
inline RaytracingTexture s_texture_LevelTexture = { "level", 0, false };

inline RaytracingParameter s_parameter_Diffuse = { "diffuse", 0, 4, 1.0f, 1.0f, 1.0f, 1.0f };
inline RaytracingParameter s_parameter_Ambient = { "ambient", 0, 3, 1.0f, 1.0f, 1.0f, 0.0f };
inline RaytracingParameter s_parameter_Specular = { "specular", 0, 3, 0.9f, 0.9f, 0.9f, 0.0f };
inline RaytracingParameter s_parameter_Emissive = { "emissive", 0, 3, 0.0f, 0.0f, 0.0f, 0.0f };
inline RaytracingParameter s_parameter_GlossLevel = { "power_gloss_level", 1, 2, 50.0f, 0.1f, 0.01f, 0.0f };
inline RaytracingParameter s_parameter_Opacity = { "opacity_reflection_refraction_spectype", 0, 1, 1.0f, 0.0f, 0.0f, 0.0f };
inline RaytracingParameter s_parameter_LuminanceRange = { "mrgLuminancerange", 0, 1, 0.0f, 0.0f, 0.0f, 0.0f };
inline RaytracingParameter s_parameter_FresnelParam = { "mrgFresnelParam", 0, 2, 1.0f, 1.0f, 0.0f, 0.0f };
inline RaytracingParameter s_parameter_SonicEyeHighLightPosition = { "g_SonicEyeHighLightPosition", 0, 3, 0.0f, 0.0f, 0.0f, 0.0f };
inline RaytracingParameter s_parameter_SonicEyeHighLightColor = { "g_SonicEyeHighLightColor", 0, 3, 1.0f, 1.0f, 1.0f, 0.0f };
inline RaytracingParameter s_parameter_SonicSkinFalloffParam = { "g_SonicSkinFalloffParam", 0, 3, 0.15f, 2.0f, 3.0f, 0.0f };
inline RaytracingParameter s_parameter_ChrEmissionParam = { "mrgChrEmissionParam", 0, 4, 0.0f, 0.0f, 0.0f, 0.0f };
inline RaytracingParameter s_parameter_TransColorMask = { "g_TransColorMask", 0, 3, 0.0f, 0.0f, 0.0f, 0.0f };
inline RaytracingParameter s_parameter_EmissionParam = { "g_EmissionParam", 0, 4, 0.0f, 0.0f, 0.0f, 1.0f };
inline RaytracingParameter s_parameter_OffsetParam = { "g_OffsetParam", 0, 2, 0.1f, 0.1f, 0.0f, 0.0f };
inline RaytracingParameter s_parameter_WaterParam = { "g_WaterParam", 0, 4, 1.0f, 0.5f, 0.0f, 8.0f };
inline RaytracingParameter s_parameter_FurParam = { "FurParam", 0, 4, 0.1f, 8.0f, 8.0f, 1.0f };
inline RaytracingParameter s_parameter_FurParam2 = { "FurParam2", 0, 4, 0.0f, 0.6f, 0.5f, 1.0f };
inline RaytracingParameter s_parameter_ChrEyeFHL1 = { "ChrEyeFHL1", 0, 4, 0.03f, -0.05f, 0.01f, 0.01f };
inline RaytracingParameter s_parameter_ChrEyeFHL2 = { "ChrEyeFHL2", 0, 4, 0.02f, 0.09f, 0.12f, 0.07f };
inline RaytracingParameter s_parameter_ChrEyeFHL3 = { "ChrEyeFHL3", 0, 4, 0.0f, 0.0f, 0.1f, 0.1f };
inline RaytracingParameter s_parameter_DistortionParam = { "mrgDistortionParam", 0, 4, 30.0f, 1.0f, 1.0f, 0.03f };
inline RaytracingParameter s_parameter_IrisColor = { "IrisColor", 0, 3, 0.5f, 0.5f, 0.5f, 0.0f };
inline RaytracingParameter s_parameter_PupilParam = { "PupilParam", 0, 4, 1.03f, 0.47f, 0.0f, 1024.0f };
inline RaytracingParameter s_parameter_HighLightColor = { "HighLightColor", 0, 3, 0.5f, 0.5f, 0.5f, 0.0f };
inline RaytracingParameter s_parameter_ChaosWaveParamEx = { "g_ChaosWaveParamEx", 1, 1, 0.0f, 0.0f, 0.0f, 0.0f };

inline RaytracingParameter* s_parameters_SYS_ERROR[2] =
{
	&s_parameter_Diffuse,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_SYS_ERROR = { SHADER_TYPE_SYS_ERROR, nullptr, 0, s_parameters_SYS_ERROR, 2 };

inline RaytracingTexture* s_textures_BLEND[9] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
	&s_texture_OpacityTexture,
	&s_texture_DiffuseTexture2,
	&s_texture_SpecularTexture2,
	&s_texture_GlossTexture2,
	&s_texture_NormalTexture2,
};

inline RaytracingParameter* s_parameters_BLEND[4] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_BLEND = { SHADER_TYPE_BLEND, s_textures_BLEND, 9, s_parameters_BLEND, 4 };

inline RaytracingTexture* s_textures_COMMON[5] =
{
	&s_texture_DiffuseTexture,
	&s_texture_OpacityTexture,
	&s_texture_SpecularTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
};

inline RaytracingParameter* s_parameters_COMMON[4] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_COMMON = { SHADER_TYPE_COMMON, s_textures_COMMON, 5, s_parameters_COMMON, 4 };

inline RaytracingTexture* s_textures_INDIRECT[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
	&s_texture_DisplacementTexture,
};

inline RaytracingParameter* s_parameters_INDIRECT[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_OffsetParam,
};

inline RaytracingShader s_shader_INDIRECT = { SHADER_TYPE_INDIRECT, s_textures_INDIRECT, 4, s_parameters_INDIRECT, 5 };

inline RaytracingTexture* s_textures_LUMINESCENCE[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
	&s_texture_DisplacementTexture,
};

inline RaytracingParameter* s_parameters_LUMINESCENCE[6] =
{
	&s_parameter_Diffuse,
	&s_parameter_Emissive,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_Ambient,
};

inline RaytracingShader s_shader_LUMINESCENCE = { SHADER_TYPE_LUMINESCENCE, s_textures_LUMINESCENCE, 4, s_parameters_LUMINESCENCE, 6 };

inline RaytracingTexture* s_textures_CHAOS[5] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_OpacityTexture,
	&s_texture_NormalTexture,
	&s_texture_NormalTexture2,
};

inline RaytracingParameter* s_parameters_CHAOS[6] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_SonicSkinFalloffParam,
	&s_parameter_ChaosWaveParamEx,
};

inline RaytracingShader s_shader_CHAOS = { SHADER_TYPE_CHAOS, s_textures_CHAOS, 5, s_parameters_CHAOS, 6 };

inline RaytracingTexture* s_textures_CHR_EYE[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
};

inline RaytracingParameter* s_parameters_CHR_EYE[6] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_SonicEyeHighLightPosition,
	&s_parameter_SonicEyeHighLightColor,
};

inline RaytracingShader s_shader_CHR_EYE = { SHADER_TYPE_CHR_EYE, s_textures_CHR_EYE, 3, s_parameters_CHR_EYE, 6 };

inline RaytracingParameter* s_parameters_CHR_EYE_FHL_PROCEDURAL[10] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_ChrEyeFHL1,
	&s_parameter_ChrEyeFHL2,
	&s_parameter_ChrEyeFHL3,
	&s_parameter_IrisColor,
	&s_parameter_PupilParam,
	&s_parameter_HighLightColor,
};

inline RaytracingShader s_shader_CHR_EYE_FHL_PROCEDURAL = { SHADER_TYPE_CHR_EYE_FHL_PROCEDURAL, nullptr, 0, s_parameters_CHR_EYE_FHL_PROCEDURAL, 10 };

inline RaytracingTexture* s_textures_CHR_EYE_FHL[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_LevelTexture,
	&s_texture_DisplacementTexture,
};

inline RaytracingParameter* s_parameters_CHR_EYE_FHL[7] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_ChrEyeFHL1,
	&s_parameter_ChrEyeFHL2,
	&s_parameter_ChrEyeFHL3,
};

inline RaytracingShader s_shader_CHR_EYE_FHL = { SHADER_TYPE_CHR_EYE_FHL, s_textures_CHR_EYE_FHL, 3, s_parameters_CHR_EYE_FHL, 7 };

inline RaytracingTexture* s_textures_CHR_SKIN[5] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
	&s_texture_DisplacementTexture,
};

inline RaytracingParameter* s_parameters_CHR_SKIN[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_SonicSkinFalloffParam,
};

inline RaytracingShader s_shader_CHR_SKIN = { SHADER_TYPE_CHR_SKIN, s_textures_CHR_SKIN, 5, s_parameters_CHR_SKIN, 5 };

inline RaytracingTexture* s_textures_CHR_SKIN_HALF[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_ReflectionTexture,
};

inline RaytracingParameter* s_parameters_CHR_SKIN_HALF[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_SonicSkinFalloffParam,
};

inline RaytracingShader s_shader_CHR_SKIN_HALF = { SHADER_TYPE_CHR_SKIN_HALF, s_textures_CHR_SKIN_HALF, 3, s_parameters_CHR_SKIN_HALF, 5 };

inline RaytracingTexture* s_textures_CHR_SKIN_IGNORE[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_DisplacementTexture,
	&s_texture_ReflectionTexture,
};

inline RaytracingParameter* s_parameters_CHR_SKIN_IGNORE[7] =
{
	&s_parameter_Diffuse,
	&s_parameter_Ambient,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_SonicSkinFalloffParam,
	&s_parameter_ChrEmissionParam,
};

inline RaytracingShader s_shader_CHR_SKIN_IGNORE = { SHADER_TYPE_CHR_SKIN_IGNORE, s_textures_CHR_SKIN_IGNORE, 4, s_parameters_CHR_SKIN_IGNORE, 7 };

inline RaytracingTexture* s_textures_CLOUD[3] =
{
	&s_texture_NormalTexture,
	&s_texture_DisplacementTexture,
	&s_texture_ReflectionTexture,
};

inline RaytracingParameter* s_parameters_CLOUD[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_SonicSkinFalloffParam,
};

inline RaytracingShader s_shader_CLOUD = { SHADER_TYPE_CLOUD, s_textures_CLOUD, 3, s_parameters_CLOUD, 5 };

inline RaytracingTexture* s_textures_DIM[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
	&s_texture_ReflectionTexture,
};

inline RaytracingParameter* s_parameters_DIM[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Ambient,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_DIM = { SHADER_TYPE_DIM, s_textures_DIM, 4, s_parameters_DIM, 5 };

inline RaytracingTexture* s_textures_DISTORTION[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_NormalTexture,
	&s_texture_NormalTexture2,
};

inline RaytracingParameter* s_parameters_DISTORTION[4] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_DISTORTION = { SHADER_TYPE_DISTORTION, s_textures_DISTORTION, 4, s_parameters_DISTORTION, 4 };

inline RaytracingTexture* s_textures_DISTORTION_OVERLAY[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_NormalTexture,
	&s_texture_NormalTexture2,
};

inline RaytracingParameter* s_parameters_DISTORTION_OVERLAY[3] =
{
	&s_parameter_Diffuse,
	&s_parameter_Opacity,
	&s_parameter_DistortionParam,
};

inline RaytracingShader s_shader_DISTORTION_OVERLAY = { SHADER_TYPE_DISTORTION_OVERLAY, s_textures_DISTORTION_OVERLAY, 3, s_parameters_DISTORTION_OVERLAY, 3 };

inline RaytracingTexture* s_textures_ENM_EMISSION[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_NormalTexture,
	&s_texture_SpecularTexture,
	&s_texture_DisplacementTexture,
};

inline RaytracingParameter* s_parameters_ENM_EMISSION[6] =
{
	&s_parameter_Diffuse,
	&s_parameter_Ambient,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_ChrEmissionParam,
};

inline RaytracingShader s_shader_ENM_EMISSION = { SHADER_TYPE_ENM_EMISSION, s_textures_ENM_EMISSION, 4, s_parameters_ENM_EMISSION, 6 };

inline RaytracingTexture* s_textures_ENM_GLASS[5] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_NormalTexture,
	&s_texture_DisplacementTexture,
	&s_texture_ReflectionTexture,
};

inline RaytracingParameter* s_parameters_ENM_GLASS[6] =
{
	&s_parameter_Diffuse,
	&s_parameter_Ambient,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_ChrEmissionParam,
};

inline RaytracingShader s_shader_ENM_GLASS = { SHADER_TYPE_ENM_GLASS, s_textures_ENM_GLASS, 5, s_parameters_ENM_GLASS, 6 };

inline RaytracingTexture* s_textures_ENM_IGNORE[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_DisplacementTexture,
};

inline RaytracingParameter* s_parameters_ENM_IGNORE[6] =
{
	&s_parameter_Diffuse,
	&s_parameter_Ambient,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_ChrEmissionParam,
};

inline RaytracingShader s_shader_ENM_IGNORE = { SHADER_TYPE_ENM_IGNORE, s_textures_ENM_IGNORE, 3, s_parameters_ENM_IGNORE, 6 };

inline RaytracingTexture* s_textures_FADE_OUT_NORMAL[2] =
{
	&s_texture_DiffuseTexture,
	&s_texture_NormalTexture,
};

inline RaytracingParameter* s_parameters_FADE_OUT_NORMAL[4] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_FADE_OUT_NORMAL = { SHADER_TYPE_FADE_OUT_NORMAL, s_textures_FADE_OUT_NORMAL, 2, s_parameters_FADE_OUT_NORMAL, 4 };

inline RaytracingTexture* s_textures_FALLOFF[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_NormalTexture,
	&s_texture_GlossTexture,
	&s_texture_DisplacementTexture,
};

inline RaytracingParameter* s_parameters_FALLOFF[4] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_FALLOFF = { SHADER_TYPE_FALLOFF, s_textures_FALLOFF, 4, s_parameters_FALLOFF, 4 };

inline RaytracingTexture* s_textures_FALLOFF_V[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
	&s_texture_NormalTexture2,
};

inline RaytracingParameter* s_parameters_FALLOFF_V[4] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_FALLOFF_V = { SHADER_TYPE_FALLOFF_V, s_textures_FALLOFF_V, 4, s_parameters_FALLOFF_V, 4 };

inline RaytracingTexture* s_textures_FUR[6] =
{
	&s_texture_DiffuseTexture,
	&s_texture_DiffuseTexture2,
	&s_texture_SpecularTexture,
	&s_texture_NormalTexture,
	&s_texture_NormalTexture2,
	&s_texture_DisplacementTexture,
};

inline RaytracingParameter* s_parameters_FUR[7] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_SonicSkinFalloffParam,
	&s_parameter_FurParam,
	&s_parameter_FurParam2,
};

inline RaytracingShader s_shader_FUR = { SHADER_TYPE_FUR, s_textures_FUR, 6, s_parameters_FUR, 7 };

inline RaytracingTexture* s_textures_GLASS[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
};

inline RaytracingParameter* s_parameters_GLASS[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_FresnelParam,
};

inline RaytracingShader s_shader_GLASS = { SHADER_TYPE_GLASS, s_textures_GLASS, 4, s_parameters_GLASS, 5 };

inline RaytracingTexture* s_textures_ICE[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
};

inline RaytracingParameter* s_parameters_ICE[4] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_ICE = { SHADER_TYPE_ICE, s_textures_ICE, 3, s_parameters_ICE, 4 };

inline RaytracingTexture* s_textures_IGNORE_LIGHT[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_OpacityTexture,
	&s_texture_DisplacementTexture,
};

inline RaytracingParameter* s_parameters_IGNORE_LIGHT[4] =
{
	&s_parameter_Diffuse,
	&s_parameter_Ambient,
	&s_parameter_Opacity,
	&s_parameter_EmissionParam,
};

inline RaytracingShader s_shader_IGNORE_LIGHT = { SHADER_TYPE_IGNORE_LIGHT, s_textures_IGNORE_LIGHT, 3, s_parameters_IGNORE_LIGHT, 4 };

inline RaytracingTexture* s_textures_IGNORE_LIGHT_TWICE[1] =
{
	&s_texture_DiffuseTexture,
};

inline RaytracingParameter* s_parameters_IGNORE_LIGHT_TWICE[2] =
{
	&s_parameter_Diffuse,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_IGNORE_LIGHT_TWICE = { SHADER_TYPE_IGNORE_LIGHT_TWICE, s_textures_IGNORE_LIGHT_TWICE, 1, s_parameters_IGNORE_LIGHT_TWICE, 2 };

inline RaytracingTexture* s_textures_INDIRECT_NO_LIGHT[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_DisplacementTexture,
	&s_texture_DisplacementTexture2,
};

inline RaytracingParameter* s_parameters_INDIRECT_NO_LIGHT[3] =
{
	&s_parameter_Diffuse,
	&s_parameter_Opacity,
	&s_parameter_OffsetParam,
};

inline RaytracingShader s_shader_INDIRECT_NO_LIGHT = { SHADER_TYPE_INDIRECT_NO_LIGHT, s_textures_INDIRECT_NO_LIGHT, 3, s_parameters_INDIRECT_NO_LIGHT, 3 };

inline RaytracingTexture* s_textures_INDIRECT_V[5] =
{
	&s_texture_DiffuseTexture,
	&s_texture_OpacityTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
	&s_texture_DisplacementTexture,
};

inline RaytracingParameter* s_parameters_INDIRECT_V[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_OffsetParam,
};

inline RaytracingShader s_shader_INDIRECT_V = { SHADER_TYPE_INDIRECT_V, s_textures_INDIRECT_V, 5, s_parameters_INDIRECT_V, 5 };

inline RaytracingTexture* s_textures_LUMINESCENCE_V[6] =
{
	&s_texture_DiffuseTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
	&s_texture_DisplacementTexture,
	&s_texture_DiffuseTexture2,
	&s_texture_GlossTexture2,
};

inline RaytracingParameter* s_parameters_LUMINESCENCE_V[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Ambient,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_LUMINESCENCE_V = { SHADER_TYPE_LUMINESCENCE_V, s_textures_LUMINESCENCE_V, 6, s_parameters_LUMINESCENCE_V, 5 };

inline RaytracingTexture* s_textures_METAL[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
};

inline RaytracingParameter* s_parameters_METAL[4] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_METAL = { SHADER_TYPE_METAL, s_textures_METAL, 4, s_parameters_METAL, 4 };

inline RaytracingTexture* s_textures_MIRROR[1] =
{
	&s_texture_DiffuseTexture,
};

inline RaytracingParameter* s_parameters_MIRROR[3] =
{
	&s_parameter_Diffuse,
	&s_parameter_Opacity,
	&s_parameter_FresnelParam,
};

inline RaytracingShader s_shader_MIRROR = { SHADER_TYPE_MIRROR, s_textures_MIRROR, 1, s_parameters_MIRROR, 3 };

inline RaytracingTexture* s_textures_RING[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_ReflectionTexture,
};

inline RaytracingParameter* s_parameters_RING[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_LuminanceRange,
};

inline RaytracingShader s_shader_RING = { SHADER_TYPE_RING, s_textures_RING, 3, s_parameters_RING, 5 };

inline RaytracingTexture* s_textures_SHOE[4] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
};

inline RaytracingParameter* s_parameters_SHOE[4] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
};

inline RaytracingShader s_shader_SHOE = { SHADER_TYPE_SHOE, s_textures_SHOE, 4, s_parameters_SHOE, 4 };

inline RaytracingTexture* s_textures_TIME_EATER[5] =
{
	&s_texture_DiffuseTexture,
	&s_texture_SpecularTexture,
	&s_texture_OpacityTexture,
	&s_texture_NormalTexture,
	&s_texture_NormalTexture2,
};

inline RaytracingParameter* s_parameters_TIME_EATER[7] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_Opacity,
	&s_parameter_GlossLevel,
	&s_parameter_LuminanceRange,
	&s_parameter_SonicSkinFalloffParam,
	&s_parameter_ChaosWaveParamEx,
};

inline RaytracingShader s_shader_TIME_EATER = { SHADER_TYPE_TIME_EATER, s_textures_TIME_EATER, 5, s_parameters_TIME_EATER, 7 };

inline RaytracingTexture* s_textures_TRANS_THIN[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_GlossTexture,
	&s_texture_NormalTexture,
};

inline RaytracingParameter* s_parameters_TRANS_THIN[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_TransColorMask,
};

inline RaytracingShader s_shader_TRANS_THIN = { SHADER_TYPE_TRANS_THIN, s_textures_TRANS_THIN, 3, s_parameters_TRANS_THIN, 5 };

inline RaytracingTexture* s_textures_WATER_ADD[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_NormalTexture,
	&s_texture_NormalTexture2,
};

inline RaytracingParameter* s_parameters_WATER_ADD[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_WaterParam,
};

inline RaytracingShader s_shader_WATER_ADD = { SHADER_TYPE_WATER_ADD, s_textures_WATER_ADD, 3, s_parameters_WATER_ADD, 5 };

inline RaytracingTexture* s_textures_WATER_MUL[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_NormalTexture,
	&s_texture_NormalTexture2,
};

inline RaytracingParameter* s_parameters_WATER_MUL[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_Opacity,
	&s_parameter_WaterParam,
};

inline RaytracingShader s_shader_WATER_MUL = { SHADER_TYPE_WATER_MUL, s_textures_WATER_MUL, 3, s_parameters_WATER_MUL, 5 };

inline RaytracingTexture* s_textures_WATER_OPACITY[3] =
{
	&s_texture_DiffuseTexture,
	&s_texture_NormalTexture,
	&s_texture_NormalTexture2,
};

inline RaytracingParameter* s_parameters_WATER_OPACITY[5] =
{
	&s_parameter_Diffuse,
	&s_parameter_Opacity,
	&s_parameter_Specular,
	&s_parameter_GlossLevel,
	&s_parameter_WaterParam,
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
	{ "Chaos_da", &s_shader_COMMON },
	{ "Chaos_dsae1", &s_shader_COMMON },
	{ "Chaos_", &s_shader_CHAOS },
	{ "ChaosV_", &s_shader_SYS_ERROR },
	{ "ChrEye_", &s_shader_CHR_EYE },
	{ "ChrEyeFHLProcedural", &s_shader_CHR_EYE_FHL_PROCEDURAL },
	{ "ChrEyeFHL", &s_shader_CHR_EYE_FHL },
	{ "ChrSkin_", &s_shader_CHR_SKIN },
	{ "ChrSkinHalf_", &s_shader_CHR_SKIN_HALF },
	{ "ChrSkinIgnore_", &s_shader_CHR_SKIN_IGNORE },
	{ "Cloak_", &s_shader_SYS_ERROR },
	{ "Cloth_", &s_shader_COMMON },
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
