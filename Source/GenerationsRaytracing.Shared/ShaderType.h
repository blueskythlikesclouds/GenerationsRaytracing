#ifndef SHADER_TYPE_H_INCLUDED
#define SHADER_TYPE_H_INCLUDED

#define SHADER_TYPE_BILLBOARD_PARTICLE            0
#define SHADER_TYPE_BILLBOARD_Y                   1
#define SHADER_TYPE_BLB_BLEND                     2
#define SHADER_TYPE_BLB_COMMON                    3
#define SHADER_TYPE_BLB_INDIRECT                  4
#define SHADER_TYPE_BLB_LUMINESCENCE              5
#define SHADER_TYPE_BLEND                         6
#define SHADER_TYPE_CHAOS                         7
#define SHADER_TYPE_CHAOS_V                       8
#define SHADER_TYPE_CHR_EYE                       9
#define SHADER_TYPE_CHR_SKIN                      10
#define SHADER_TYPE_CHR_SKIN_HALF                 11
#define SHADER_TYPE_CHR_SKIN_IGNORE               12
#define SHADER_TYPE_CLOAK                         13
#define SHADER_TYPE_CLOTH                         14
#define SHADER_TYPE_CLOUD                         15
#define SHADER_TYPE_COMMON                        16
#define SHADER_TYPE_DEFORMATION                   17
#define SHADER_TYPE_DEFORMATION_PARTICLE          18
#define SHADER_TYPE_DIM                           19
#define SHADER_TYPE_DIM_IGNORE                    20
#define SHADER_TYPE_DISTORTION                    21
#define SHADER_TYPE_DISTORTION_OVERLAY            22
#define SHADER_TYPE_DISTORTION_OVERLAY_CHAOS      23
#define SHADER_TYPE_ENM_CLOUD                     24
#define SHADER_TYPE_ENM_EMISSION                  25
#define SHADER_TYPE_ENM_GLASS                     26
#define SHADER_TYPE_ENM_IGNORE                    27
#define SHADER_TYPE_ENM_METAL                     28
#define SHADER_TYPE_FADE_OUT_NORMAL               29
#define SHADER_TYPE_FAKE_GLASS                    30
#define SHADER_TYPE_FALLOFF                       31
#define SHADER_TYPE_FALLOFF_V                     32
#define SHADER_TYPE_GLASS                         33
#define SHADER_TYPE_GLASS_REFRACTION              34
#define SHADER_TYPE_ICE                           35
#define SHADER_TYPE_IGNORE_LIGHT                  36
#define SHADER_TYPE_IGNORE_LIGHT_TWICE            37
#define SHADER_TYPE_IGNORE_LIGHT_V                38
#define SHADER_TYPE_INDIRECT                      39
#define SHADER_TYPE_INDIRECT_V                    40
#define SHADER_TYPE_INDIRECT_V_NO_GI_SHADOW       41
#define SHADER_TYPE_LAVA                          42
#define SHADER_TYPE_LUMINESCENCE                  43
#define SHADER_TYPE_LUMINESCENCE_V                44
#define SHADER_TYPE_MESH_PARTICLE                 45
#define SHADER_TYPE_MESH_PARTICLE_LIGHTING_SHADOW 46
#define SHADER_TYPE_MESH_PARTICLE_REF             47
#define SHADER_TYPE_MIRROR                        48
#define SHADER_TYPE_MYST                          49
#define SHADER_TYPE_PARALLAX                      50
#define SHADER_TYPE_RING                          51
#define SHADER_TYPE_TIME_EATER                    52
#define SHADER_TYPE_TIME_EATER_DISTORTION         53
#define SHADER_TYPE_TIME_EATER_EMISSION           54
#define SHADER_TYPE_TIME_EATER_GLASS              55
#define SHADER_TYPE_TIME_EATER_INDIRECT           56
#define SHADER_TYPE_TIME_EATER_METAL              57
#define SHADER_TYPE_TRANS_THIN                    58
#define SHADER_TYPE_WATER_ADD                     59
#define SHADER_TYPE_WATER_MUL                     60
#define SHADER_TYPE_WATER_OPACITY                 61


#ifdef __cplusplus
#include <string_view>

inline std::pair<std::string_view, size_t> s_shaderTypes[] =
{
    {"BillboardParticle_", SHADER_TYPE_BILLBOARD_PARTICLE},
    {"BillboardParticleY_", SHADER_TYPE_BILLBOARD_Y},
    {"BlbBlend_", SHADER_TYPE_BLB_BLEND},
    {"BlbCommon_", SHADER_TYPE_BLB_COMMON},
    {"BlbIndirect_", SHADER_TYPE_BLB_INDIRECT},
    {"BlbLuminescence_", SHADER_TYPE_BLB_LUMINESCENCE},
    {"Blend_", SHADER_TYPE_BLEND},
    {"Chaos_", SHADER_TYPE_CHAOS},
    {"ChaosV_", SHADER_TYPE_CHAOS_V},
    {"ChrEye_", SHADER_TYPE_CHR_EYE},
    {"ChrSkin_", SHADER_TYPE_CHR_SKIN},
    {"ChrSkinHalf_", SHADER_TYPE_CHR_SKIN_HALF},
    {"ChrSkinIgnore_", SHADER_TYPE_CHR_SKIN_IGNORE},
    {"Cloak_", SHADER_TYPE_CLOAK},
    {"Cloth_", SHADER_TYPE_CLOTH},
    {"Cloud_", SHADER_TYPE_CLOUD},
    {"Common_", SHADER_TYPE_COMMON},
    {"Deformation_", SHADER_TYPE_DEFORMATION},
    {"DeformationParticle_", SHADER_TYPE_DEFORMATION_PARTICLE},
    {"Dim_", SHADER_TYPE_DIM},
    {"DimIgnore_", SHADER_TYPE_DIM_IGNORE},
    {"Distortion_", SHADER_TYPE_DISTORTION},
    {"DistortionOverlay_", SHADER_TYPE_DISTORTION_OVERLAY},
    {"DistortionOverlayChaos_", SHADER_TYPE_DISTORTION_OVERLAY_CHAOS},
    {"EnmCloud_", SHADER_TYPE_ENM_CLOUD},
    {"EnmEmission_", SHADER_TYPE_ENM_EMISSION},
    {"EnmGlass_", SHADER_TYPE_ENM_GLASS},
    {"EnmIgnore_", SHADER_TYPE_ENM_IGNORE},
    {"EnmMetal_", SHADER_TYPE_ENM_METAL},
    {"FadeOutNormal_", SHADER_TYPE_FADE_OUT_NORMAL},
    {"FakeGlass_", SHADER_TYPE_FAKE_GLASS},
    {"FallOff_", SHADER_TYPE_FALLOFF},
    {"FallOffV_", SHADER_TYPE_FALLOFF_V},
    {"Glass_", SHADER_TYPE_GLASS},
    {"GlassRefraction_", SHADER_TYPE_GLASS_REFRACTION},
    {"Ice_", SHADER_TYPE_ICE},
    {"IgnoreLight_", SHADER_TYPE_IGNORE_LIGHT},
    {"IgnoreLightTwice_", SHADER_TYPE_IGNORE_LIGHT_TWICE},
    {"IgnoreLightV_", SHADER_TYPE_IGNORE_LIGHT_V},
    {"Indirect_", SHADER_TYPE_INDIRECT},
    {"IndirectV_", SHADER_TYPE_INDIRECT_V},
    {"IndirectVnoGIs_", SHADER_TYPE_INDIRECT_V_NO_GI_SHADOW},
    {"Lava_", SHADER_TYPE_LAVA},
    {"Luminescence_", SHADER_TYPE_LUMINESCENCE},
    {"LuminescenceV_", SHADER_TYPE_LUMINESCENCE_V},
    {"MeshParticle_", SHADER_TYPE_MESH_PARTICLE},
    {"MeshParticleLightingShadow_", SHADER_TYPE_MESH_PARTICLE_LIGHTING_SHADOW},
    {"MeshParticleRef_", SHADER_TYPE_MESH_PARTICLE_REF},
    {"Mirror_", SHADER_TYPE_MIRROR},
    {"Mirror2_", SHADER_TYPE_MIRROR},
    {"Myst_", SHADER_TYPE_MYST},
    {"Parallax_", SHADER_TYPE_PARALLAX},
    {"Ring_", SHADER_TYPE_RING},
    {"TimeEater_", SHADER_TYPE_TIME_EATER},
    {"TimeEaterDistortion_", SHADER_TYPE_TIME_EATER_DISTORTION},
    {"TimeEaterEmission_", SHADER_TYPE_TIME_EATER_EMISSION},
    {"TimeEaterGlass_", SHADER_TYPE_TIME_EATER_GLASS},
    {"TimeEaterIndirect_", SHADER_TYPE_TIME_EATER_INDIRECT},
    {"TimeEaterMetal_", SHADER_TYPE_TIME_EATER_METAL},
    {"TransThin_", SHADER_TYPE_TRANS_THIN},
    {"Water_Add", SHADER_TYPE_WATER_ADD},
    {"Water_Mul", SHADER_TYPE_WATER_MUL},
    {"Water_Opacity", SHADER_TYPE_WATER_OPACITY}
};

#endif

#endif