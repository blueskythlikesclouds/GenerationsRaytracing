using Vortice.Dxc;

public class RaytracingTexture(string name, int index, string fieldName)
{
    public string Name { get; set; } = name;
    public int Index { get; set; } = index;
    public string FieldName { get; set; } = fieldName;

    public static RaytracingTexture Diffuse = new("diffuse", 0, "DiffuseTexture");
    public static RaytracingTexture Diffuse2 = new("diffuse", 1, "DiffuseTexture2");

    public static RaytracingTexture Specular = new("specular", 0, "SpecularTexture");
    public static RaytracingTexture Specular2 = new("specular", 1, "SpecularTexture2");

    public static RaytracingTexture Gloss = new("gloss", 0, "GlossTexture");
    public static RaytracingTexture Gloss2 = new("gloss", 1, "GlossTexture2");

    public static RaytracingTexture Normal = new("normal", 0, "NormalTexture");
    public static RaytracingTexture Normal2 = new("normal", 1, "NormalTexture2");

    public static RaytracingTexture Reflection = new("reflection", 0, "ReflectionTexture");

    public static RaytracingTexture Opacity = new("opacity", 0, "OpacityTexture");

    public static RaytracingTexture Displacement = new("displacement", 0, "DisplacementTexture");
    public static RaytracingTexture Displacement2 = new("displacement", 1, "DisplacementTexture2");

    public static RaytracingTexture Level = new("level", 0, "LevelTexture");
}

public class RaytracingParameter(string name, int index, int size, string fieldName)
{
    public string Name { get; set; } = name;
    public int Index { get; set; } = index;
    public int Size { get; set; } = size;
    public string FieldName { get; set; } = fieldName;

    public static RaytracingParameter Diffuse = new("diffuse", 0, 4, "Diffuse");
    public static RaytracingParameter Ambient = new("ambient", 0, 3, "Ambient");
    public static RaytracingParameter Specular = new("specular", 0, 3, "Specular");
    public static RaytracingParameter Emissive = new("emissive", 0, 3, "Emissive");
    public static RaytracingParameter GlossLevel = new("power_gloss_level", 1, 2, "GlossLevel");
    public static RaytracingParameter Opacity = new("opacity_reflection_refraction_spectype", 0, 1, "Opacity");
    public static RaytracingParameter LuminanceRange = new("mrgLuminancerange", 0, 1, "LuminanceRange");
    public static RaytracingParameter FresnelParam = new("mrgFresnelParam", 0, 2, "FresnelParam");
    public static RaytracingParameter SonicEyeHighLightPosition = new("g_SonicEyeHighLightPositionRaytracing", 0, 3, "SonicEyeHighLightPosition");
    public static RaytracingParameter SonicEyeHighLightColor = new("g_SonicEyeHighLightColor", 0, 3, "SonicEyeHighLightColor");
    public static RaytracingParameter SonicSkinFalloffParam = new("g_SonicSkinFalloffParam", 0, 3, "SonicSkinFalloffParam");
    public static RaytracingParameter ChrEmissionParam = new("mrgChrEmissionParam", 0, 4, "ChrEmissionParam");
    public static RaytracingParameter TransColorMask = new("g_TransColorMask", 0, 3, "TransColorMask");
    public static RaytracingParameter EmissionParam = new("g_EmissionParam", 0, 4, "EmissionParam");
    public static RaytracingParameter OffsetParam = new("g_OffsetParam", 0, 2, "OffsetParam");
    public static RaytracingParameter WaterParam = new("g_WaterParam", 0, 4, "WaterParam");
    public static RaytracingParameter FurParam = new("FurParam", 0, 4, "FurParam");
    public static RaytracingParameter FurParam2 = new("FurParam2", 0, 4, "FurParam2");
    public static RaytracingParameter ChrEyeFHL1 = new("ChrEyeFHL1", 0, 4, "ChrEyeFHL1");
    public static RaytracingParameter ChrEyeFHL2 = new("ChrEyeFHL2", 0, 4, "ChrEyeFHL2");
    public static RaytracingParameter ChrEyeFHL3 = new("ChrEyeFHL3", 0, 4, "ChrEyeFHL3");
}

public class RaytracingShader(string name, RaytracingTexture[] textures, RaytracingParameter[] parameters)
{
    public string Name { get; set; } = name;
    public RaytracingTexture[] Textures { get; set; } = textures;
    public RaytracingParameter[] Parameters { get; set; } = parameters;

    public static RaytracingShader SysError = new("SYS_ERROR",
        [
            RaytracingTexture.Diffuse
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Opacity
        ]);

    public static RaytracingShader Blend = new("BLEND",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular, 
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
            RaytracingTexture.Opacity,
            RaytracingTexture.Diffuse2,
            RaytracingTexture.Specular2,
            RaytracingTexture.Gloss2,
            RaytracingTexture.Normal2
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular, 
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader ChrEye = new("CHR_EYE",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal
        ],
        [
            RaytracingParameter.Diffuse, 
            RaytracingParameter.Specular, 
            RaytracingParameter.GlossLevel, 
            RaytracingParameter.Opacity, 
            RaytracingParameter.SonicEyeHighLightPosition, 
            RaytracingParameter.SonicEyeHighLightColor
        ]);

    public static RaytracingShader ChrEyeFHL = new("CHR_EYE_FHL",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Level,
            RaytracingTexture.Displacement
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.ChrEyeFHL1,
            RaytracingParameter.ChrEyeFHL2,
            RaytracingParameter.ChrEyeFHL3
        ]);

    public static RaytracingShader ChrSkin = new("CHR_SKIN",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
            RaytracingTexture.Displacement
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.SonicSkinFalloffParam
        ]);

    public static RaytracingShader ChrSkinHalf = new("CHR_SKIN_HALF",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Reflection,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.SonicSkinFalloffParam,
        ]);

    public static RaytracingShader ChrSkinIgnore = new("CHR_SKIN_IGNORE",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Displacement,
            RaytracingTexture.Reflection,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Ambient,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.SonicSkinFalloffParam,
            RaytracingParameter.ChrEmissionParam,
        ]);

    public static RaytracingShader Cloud = new("CLOUD",
        [
            RaytracingTexture.Normal,
            RaytracingTexture.Displacement,
            RaytracingTexture.Reflection,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.SonicSkinFalloffParam,
        ]);

    public static RaytracingShader Common = new("COMMON",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Opacity,
            RaytracingTexture.Specular,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader Dim = new("DIM",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
            RaytracingTexture.Reflection,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Ambient,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader Distortion = new("DISTORTION",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Normal,
            RaytracingTexture.Normal2,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader DistortionOverlay = new("DISTORTION_OVERLAY",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Normal,
            RaytracingTexture.Normal2,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader EnmEmission = new("ENM_EMISSION",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Normal,
            RaytracingTexture.Specular,
            RaytracingTexture.Displacement,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Ambient,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.ChrEmissionParam,
        ]);

    public static RaytracingShader EnmGlass = new("ENM_GLASS",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Normal,
            RaytracingTexture.Displacement,
            RaytracingTexture.Reflection,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Ambient,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.ChrEmissionParam,
        ]);

    public static RaytracingShader EnmIgnore = new("ENM_IGNORE",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Displacement,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Ambient,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.ChrEmissionParam,
        ]);

    public static RaytracingShader FadeOutNormal = new("FADE_OUT_NORMAL",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Normal,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader FallOff = new("FALLOFF",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Normal,
            RaytracingTexture.Gloss,
            RaytracingTexture.Displacement,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader FallOffV = new("FALLOFF_V",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
            RaytracingTexture.Normal2,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader Fur = new("FUR",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Diffuse2,
            RaytracingTexture.Specular,
            RaytracingTexture.Normal,
            RaytracingTexture.Normal2,
            RaytracingTexture.Displacement,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.SonicSkinFalloffParam,
            RaytracingParameter.FurParam,
            RaytracingParameter.FurParam2,
        ]);

    public static RaytracingShader Glass = new("GLASS",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.FresnelParam,
        ]);

    public static RaytracingShader Ice = new("ICE",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader IgnoreLight = new("IGNORE_LIGHT",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Opacity,
            RaytracingTexture.Displacement,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Ambient,
            RaytracingParameter.Opacity,
            RaytracingParameter.EmissionParam,
        ]);

    public static RaytracingShader IgnoreLightTwice = new("IGNORE_LIGHT_TWICE",
        [
            RaytracingTexture.Diffuse,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader Indirect = new("INDIRECT",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
            RaytracingTexture.Displacement,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.OffsetParam,
        ]);

    public static RaytracingShader IndirectNoLight = new("INDIRECT_NO_LIGHT",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Displacement,
            RaytracingTexture.Displacement2,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Opacity,
            RaytracingParameter.OffsetParam,
        ]);

    public static RaytracingShader IndirectV = new("INDIRECT_V",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Opacity,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
            RaytracingTexture.Displacement,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.OffsetParam,
        ]);

    public static RaytracingShader Luminescence = new("LUMINESCENCE",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
            RaytracingTexture.Displacement,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Emissive,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.Ambient,
        ]);

    public static RaytracingShader LuminescenceV = new("LUMINESCENCE_V",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
            RaytracingTexture.Displacement,
            RaytracingTexture.Diffuse2,
            RaytracingTexture.Gloss2,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Ambient,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader Metal = new("METAL",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader Mirror = new("MIRROR",
        [
            RaytracingTexture.Diffuse
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Opacity,
            RaytracingParameter.FresnelParam
        ]);

    public static RaytracingShader Ring = new("RING",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Reflection,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.LuminanceRange,
        ]);

    public static RaytracingShader Shoe = new("SHOE",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
        ]);

    public static RaytracingShader TimeEater = new("TIME_EATER",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Specular,
            RaytracingTexture.Opacity,
            RaytracingTexture.Normal,
            RaytracingTexture.Normal2,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.Opacity,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.SonicSkinFalloffParam,
        ]);

    public static RaytracingShader TransThin = new("TRANS_THIN",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Gloss,
            RaytracingTexture.Normal,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.TransColorMask,
        ]);

    public static RaytracingShader WaterAdd = new("WATER_ADD",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Normal,
            RaytracingTexture.Normal2,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.WaterParam,
        ]);

    public static RaytracingShader WaterMul = new("WATER_MUL",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Normal,
            RaytracingTexture.Normal2,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.WaterParam,
        ]);

    public static RaytracingShader WaterOpacity = new("WATER_OPACITY",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Normal,
            RaytracingTexture.Normal2,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Opacity,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.WaterParam,
        ]);
}

public static class RaytracingShaderCompiler
{
    private static readonly List<(string ShaderName, RaytracingShader Shader)> ShaderPairs = new()
    {
        ("BillboardParticle_", RaytracingShader.SysError),
        ("BillboardParticleY_", RaytracingShader.SysError),
        ("BlbBlend_", RaytracingShader.Blend),
        ("BlbCommon_", RaytracingShader.Common),
        ("BlbIndirect_", RaytracingShader.Indirect),
        ("BlbLuminescence_", RaytracingShader.Luminescence),
        ("Blend_", RaytracingShader.Blend),
        ("Chaos_", RaytracingShader.SysError),
        ("ChaosV_", RaytracingShader.SysError),
        ("ChrEye_", RaytracingShader.ChrEye),
        ("ChrEyeFHL", RaytracingShader.ChrEyeFHL),
        ("ChrSkin_", RaytracingShader.ChrSkin),
        ("ChrSkinHalf_", RaytracingShader.ChrSkinHalf),
        ("ChrSkinIgnore_", RaytracingShader.ChrSkinIgnore),
        ("Cloak_", RaytracingShader.SysError),
        ("Cloth_", RaytracingShader.SysError),
        ("Cloud_", RaytracingShader.Cloud),
        ("Common_", RaytracingShader.Common),
        ("Deformation_", RaytracingShader.SysError),
        ("DeformationParticle_", RaytracingShader.SysError),
        ("Dim_", RaytracingShader.Dim),
        ("DimIgnore_", RaytracingShader.SysError),
        ("Distortion_", RaytracingShader.Distortion),
        ("DistortionOverlay_", RaytracingShader.DistortionOverlay),
        ("DistortionOverlayChaos_", RaytracingShader.SysError),
        ("EnmCloud_", RaytracingShader.Cloud),
        ("EnmEmission_", RaytracingShader.EnmEmission),
        ("EnmGlass_", RaytracingShader.EnmGlass),
        ("EnmIgnore_", RaytracingShader.EnmIgnore),
        ("EnmMetal_", RaytracingShader.ChrSkin),
        ("FadeOutNormal_", RaytracingShader.FadeOutNormal),
        ("FakeGlass_", RaytracingShader.EnmGlass),
        ("FallOff_", RaytracingShader.FallOff),
        ("FallOffV_", RaytracingShader.FallOffV),
        ("Fur", RaytracingShader.Fur),
        ("Glass_", RaytracingShader.Glass),
        ("GlassRefraction_", RaytracingShader.SysError),
        ("Ice_", RaytracingShader.Ice),
        ("IgnoreLight_", RaytracingShader.IgnoreLight),
        ("IgnoreLightTwice_", RaytracingShader.IgnoreLightTwice),
        ("IgnoreLightV_", RaytracingShader.SysError),
        ("Indirect_", RaytracingShader.Indirect),
        ("IndirectNoLight_", RaytracingShader.IndirectNoLight),
        ("IndirectV_", RaytracingShader.IndirectV),
        ("IndirectVnoGIs_", RaytracingShader.IndirectV),
        ("Lava_", RaytracingShader.SysError),
        ("Luminescence_", RaytracingShader.Luminescence),
        ("LuminescenceV_", RaytracingShader.LuminescenceV),
        ("MeshParticle_", RaytracingShader.SysError),
        ("MeshParticleLightingShadow_", RaytracingShader.SysError),
        ("MeshParticleRef_", RaytracingShader.SysError),
        ("Metal", RaytracingShader.Metal),
        ("Mirror_", RaytracingShader.Mirror),
        ("Mirror2_", RaytracingShader.Mirror),
        ("Myst_", RaytracingShader.SysError),
        ("Parallax_", RaytracingShader.SysError),
        ("Ring_", RaytracingShader.Ring),
        ("Shoe", RaytracingShader.Shoe),
        ("TimeEater_", RaytracingShader.TimeEater),
        ("TimeEaterDistortion_", RaytracingShader.Distortion),
        ("TimeEaterEmission_", RaytracingShader.EnmEmission),
        ("TimeEaterGlass_", RaytracingShader.EnmGlass),
        ("TimeEaterIndirect_", RaytracingShader.IndirectNoLight),
        ("TimeEaterMetal_", RaytracingShader.ChrSkin),
        ("TransThin_", RaytracingShader.TransThin),
        ("Water_Add", RaytracingShader.WaterAdd),
        ("Water_Mul", RaytracingShader.WaterMul),
        ("Water_Opacity", RaytracingShader.WaterOpacity)
    };

    private static readonly List<RaytracingShader> Shaders = ShaderPairs
        .Select(x => x.Shader)
        .Distinct()
        .ToList();

    private static void WriteShaderTypeHeaderFile(string solutionDirectoryPath)
    {
        using var writer = new StreamWriter(
            Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.Shared", "ShaderType.h"));

        writer.WriteLine("#pragma once");

        for (int i = 0; i < Shaders.Count; i++)
            writer.WriteLine("#define SHADER_TYPE_{0} {1}", Shaders[i].Name, i);

        writer.WriteLine("#define SHADER_TYPE_MAX {0}", Shaders.Count);
    }

    private static void WriteHitGroupsHeaderFile(string solutionDirectoryPath)
    {
        using var writer = new StreamWriter(
            Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.X64", "HitGroups.h"));

        writer.WriteLine("#pragma once");

        writer.WriteLine("inline const wchar_t* s_shaderHitGroups[] =");
        writer.WriteLine("{");

        foreach (var shader in Shaders)
        {
            writer.WriteLine("\tL\"{0}_PrimaryHitGroup\",", shader.Name);
            writer.WriteLine("\tL\"{0}_PrimaryHitGroup_ConstTexCoord\",", shader.Name);
            writer.WriteLine("\tL\"{0}_PrimaryTransparentHitGroup\",", shader.Name);
            writer.WriteLine("\tL\"{0}_PrimaryTransparentHitGroup_ConstTexCoord\",", shader.Name);
            writer.WriteLine("\tL\"{0}_SecondaryHitGroup\",", shader.Name);
        }

        writer.WriteLine("};");
    }

    private static void WriteRaytracingShaderHeaderFile(string solutionDirectoryPath)
    {
        using var writer = new StreamWriter(
            Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.X86", "RaytracingShader.h"));

        writer.WriteLine("""
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
                         """);

        foreach (var shader in Shaders)
        {
            writer.WriteLine("inline RaytracingTexture s_textures_{0}[{1}] =", shader.Name, shader.Textures.Length);
            writer.WriteLine("{");

            foreach (var texture in shader.Textures)
            {
                writer.WriteLine("\t{{ \"{0}\", {1}, {2}, {3} }},", texture.Name, texture.Index,
                    texture.Name == "opacity" ? "true" : "false", texture.Name == "reflection" ? "true" : "false");
            }

            writer.WriteLine("};");
            writer.WriteLine("inline RaytracingParameter s_parameters_{0}[{1}] =", shader.Name, shader.Parameters.Length);
            writer.WriteLine("{");

            foreach (var parameter in shader.Parameters)
                writer.WriteLine("\t{{ \"{0}\", {1}, {2} }},", parameter.Name, parameter.Index, parameter.Size);

            writer.WriteLine("};");
            writer.WriteLine("inline RaytracingShader s_shader_{0} = {{ SHADER_TYPE_{0}, s_textures_{0}, {1}, s_parameters_{0}, {2} }};",
                shader.Name, shader.Textures.Length, shader.Parameters.Length);
        }

        writer.WriteLine("inline std::pair<std::string_view, RaytracingShader*> s_shaders[] =");
        writer.WriteLine("{");

        foreach (var (shaderName, shader) in ShaderPairs)
            writer.WriteLine("\t{{ \"{0}\", &s_shader_{1} }},", shaderName, shader.Name);

        writer.WriteLine("};");
    }

    private static void WriteMaterialHeaderFile(string solutionDirectoryPath)
    {
        int textureNum = Shaders.Select(x => x.Textures.Length).Max();
        int parameterNum = Shaders.Select(x => x.Parameters.Sum(y => y.Size)).Max();

        using (var writer = new StreamWriter(
                   Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.X64", "Material.h")))
        {
            writer.WriteLine("""
                             #pragma once
                             
                             struct Material
                             {{
                                 uint32_t shaderType : 16;
                                 uint32_t flags : 16;
                                 float texCoordOffsets[8];
                                 uint32_t textures[{0}];
                                 float parameters[{1}];
                             }};
                             """,
                textureNum,
                parameterNum);
        }

        using (var writer = new StreamWriter(
                   Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.X64", "MaterialData.hlsli")))
        {
            writer.WriteLine("""
                             #pragma once

                             #include "ShaderType.h"
                             #include "Material.hlsli"
                             
                             struct MaterialData
                             {{
                                 uint ShaderType : 16;
                                 uint Flags : 16;
                                 float4 TexCoordOffsets[2];
                                 uint Textures[{0}];
                                 float Parameters[{1}];
                             }};
                             
                             Material GetMaterial(uint shaderType, MaterialData materialData)
                             {{
                                Material material = (Material) 0;
                                material.Flags = materialData.Flags;
                                
                                switch (shaderType)
                                {{
                             """,
                textureNum,
                parameterNum);

            foreach (var shader in Shaders)
            {
                writer.WriteLine("    case SHADER_TYPE_{0}:", shader.Name);

                for (var i = 0; i < shader.Textures.Length; i++)
                    writer.WriteLine("        material.{0} = materialData.Textures[{1}];", shader.Textures[i].FieldName, i);

                int index = 0;
                foreach (var parameter in shader.Parameters)
                {
                    if (parameter.Size > 1)
                    {
                        for (int j = 0; j < parameter.Size; j++)
                        {
                            writer.WriteLine("        material.{0}[{1}] = materialData.Parameters[{2}];",
                                parameter.FieldName, j, index);

                            ++index;
                        }
                    }
                    else
                    {
                        writer.WriteLine("        material.{0} = materialData.Parameters[{1}];",
                            parameter.FieldName, index);

                        ++index;
                    }
                }

                writer.WriteLine("        break;");
            }

            writer.WriteLine("""
                                }
                                return material;
                             }
                             """);
        }
    }

    public static void Compile(string solutionDirectoryPath, string destinationFilePath)
    {
        WriteShaderTypeHeaderFile(solutionDirectoryPath);
        WriteHitGroupsHeaderFile(solutionDirectoryPath);
        WriteRaytracingShaderHeaderFile(solutionDirectoryPath);
        WriteMaterialHeaderFile(solutionDirectoryPath);

        var shaderSources = new List<string>(Shaders.Count);

        foreach (var shader in Shaders)
        {
            shaderSources.Add(string.Format(
                """
                #include "DXILLibrary.hlsli"
                [shader("anyhit")]
                void {0}_PrimaryAnyHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
                {{
                	PrimaryAnyHit(SHADER_TYPE_{0}, payload, attributes);
                }}
                [shader("closesthit")]
                void {0}_PrimaryClosestHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
                {{
                	PrimaryClosestHit(VERTEX_FLAG_MIPMAP | VERTEX_FLAG_MULTI_UV | VERTEX_FLAG_SAFE_POSITION, SHADER_TYPE_{0}, payload, attributes);
                }}
                [shader("closesthit")]
                void {0}_PrimaryClosestHit_ConstTexCoord(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
                {{
                	PrimaryClosestHit(VERTEX_FLAG_MIPMAP | VERTEX_FLAG_SAFE_POSITION, SHADER_TYPE_{0}, payload, attributes);
                }}
                [shader("anyhit")]
                void {0}_PrimaryTransparentAnyHit(inout PrimaryTransparentRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
                {{
                	PrimaryTransparentAnyHit(VERTEX_FLAG_MIPMAP | VERTEX_FLAG_MULTI_UV | VERTEX_FLAG_SAFE_POSITION, SHADER_TYPE_{0}, payload, attributes);
                }}
                [shader("anyhit")]
                void {0}_PrimaryTransparentAnyHit_ConstTexCoord(inout PrimaryTransparentRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
                {{
                	PrimaryTransparentAnyHit(VERTEX_FLAG_MIPMAP | VERTEX_FLAG_SAFE_POSITION, SHADER_TYPE_{0}, payload, attributes);
                }}
                [shader("closesthit")]
                void {0}_SecondaryClosestHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
                {{
                	SecondaryClosestHit(SHADER_TYPE_{0}, payload, attributes);
                }}
                TriangleHitGroup {0}_PrimaryHitGroup =
                {{
                	"{0}_PrimaryAnyHit",
                	"{0}_PrimaryClosestHit"
                }};
                TriangleHitGroup {0}_PrimaryHitGroup_ConstTexCoord =
                {{
                	"{0}_PrimaryAnyHit",
                	"{0}_PrimaryClosestHit_ConstTexCoord"
                }};
                TriangleHitGroup {0}_PrimaryTransparentHitGroup =
                {{
                	"{0}_PrimaryTransparentAnyHit",
                	""
                }};
                TriangleHitGroup {0}_PrimaryTransparentHitGroup_ConstTexCoord =
                {{
                	"{0}_PrimaryTransparentAnyHit_ConstTexCoord",
                	""
                }};
                TriangleHitGroup {0}_SecondaryHitGroup =
                {{
                	"SecondaryAnyHit",
                	"{0}_SecondaryClosestHit"
                }};
                """, shader.Name));
        }

#if DEBUG
        const string configuration = "Debug";
#else
        const string configuration = "Release";
#endif
        var globalArgs = new[]
        {
            $"-I {Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.X64")}",
            $"-I {Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.Shared")}",
            $"-I {Path.Combine(solutionDirectoryPath, "..", "Dependencies", "self-intersection-avoidance")}",
            "-T lib_6_6",
            "-HV 2021",
            "-all-resources-bound",
            "-Zi",
            "-enable-payload-qualifiers",
            null,
        };

        var compiler = new ThreadLocal<IDxcCompiler3>(Dxc.CreateDxcCompiler<IDxcCompiler3>);
        var includeHandler = new ThreadLocal<IDxcIncludeHandler>(DxcCompiler.Utils.CreateDefaultIncludeHandler);
        var shaderBlobs = new byte[shaderSources.Count][];

        int progress = 0;

        Parallel.For(0, shaderSources.Count, i =>
        {
            string pdbFilePath = Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.X64", "obj", configuration, $"{Shaders[i].Name}.pdb");
            var localArgs = (string[])globalArgs.Clone();
            localArgs[^1] = $"-Fd {pdbFilePath}";

            var dxcResult = compiler.Value.Compile(shaderSources[i], localArgs, includeHandler.Value);

            var byteCode = dxcResult.GetObjectBytecode();
            string errors = dxcResult.GetErrors();

            if (byteCode.IsEmpty)
                throw new Exception(errors);

            using var pdb = File.Create(pdbFilePath);
            pdb.Write(dxcResult.GetOutput(DxcOutKind.Pdb).AsSpan());

            if (!string.IsNullOrEmpty(errors))
                Console.WriteLine(dxcResult.GetErrors());

            shaderBlobs[i] = byteCode.ToArray();

            Console.WriteLine("({0}/{1}): {2}", Interlocked.Increment(ref progress), Shaders.Count, Shaders[i].Name);
        });

        using var stream = File.Create(destinationFilePath);
        using var writer = new BinaryWriter(stream);

        foreach (var shaderBlob in shaderBlobs)
        {
            writer.Write(shaderBlob.Length);
            writer.Write(shaderBlob);
        }
    }
}