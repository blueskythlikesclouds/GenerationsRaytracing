using System.Reflection.Metadata;
using Vortice.Dxc;

public class RaytracingTexture(string name, int index, string fieldName)
{
    public string Name { get; set; } = name;
    public int Index { get; set; } = index;
    public string FieldName { get; set; } = fieldName;

    public static readonly RaytracingTexture Diffuse = new("diffuse", 0, "DiffuseTexture");
    public static readonly RaytracingTexture Diffuse2 = new("diffuse", 1, "DiffuseTexture2");

    public static readonly RaytracingTexture Specular = new("specular", 0, "SpecularTexture");
    public static readonly RaytracingTexture Specular2 = new("specular", 1, "SpecularTexture2");

    public static readonly RaytracingTexture Gloss = new("gloss", 0, "GlossTexture");
    public static readonly RaytracingTexture Gloss2 = new("gloss", 1, "GlossTexture2");

    public static readonly RaytracingTexture Normal = new("normal", 0, "NormalTexture");
    public static readonly RaytracingTexture Normal2 = new("normal", 1, "NormalTexture2");

    public static readonly RaytracingTexture Reflection = new("reflection", 0, "ReflectionTexture");

    public static readonly RaytracingTexture Opacity = new("opacity", 0, "OpacityTexture");

    public static readonly RaytracingTexture Displacement = new("displacement", 0, "DisplacementTexture");
    public static readonly RaytracingTexture Displacement2 = new("displacement", 1, "DisplacementTexture2");

    public static readonly RaytracingTexture Level = new("level", 0, "LevelTexture");

    public static readonly RaytracingTexture[] AllTextures = [
        Diffuse,
        Diffuse2,
        Specular,
        Specular2,
        Gloss,
        Gloss2,
        Normal,
        Normal2,
        Reflection,
        Opacity,
        Displacement,
        Displacement2,
        Level];
}

public class RaytracingParameter(string name, int index, int size, string fieldName, float x, float y, float z, float w)
{
    public string Name { get; set; } = name;
    public int Index { get; set; } = index;
    public int Size { get; set; } = size;
    public string FieldName { get; set; } = fieldName;
    public float X { get; set; } = x;
    public float Y { get; set; } = y;
    public float Z { get; set; } = z;
    public float W { get; set; } = w;

    public static readonly RaytracingParameter Diffuse = new("diffuse", 0, 4, "Diffuse", 1.0f, 1.0f, 1.0f, 1.0f);
    public static readonly RaytracingParameter Ambient = new("ambient", 0, 3, "Ambient", 1.0f, 1.0f, 1.0f, 0.0f);
    public static readonly RaytracingParameter Specular = new("specular", 0, 3, "Specular", 0.9f, 0.9f, 0.9f, 0.0f);
    public static readonly RaytracingParameter Emissive = new("emissive", 0, 3, "Emissive", 0.0f, 0.0f, 0.0f, 0.0f);
    public static readonly RaytracingParameter GlossLevel = new("power_gloss_level", 1, 2, "GlossLevel", 50.0f, 0.1f, 0.01f, 0.0f);
    public static readonly RaytracingParameter Opacity = new("opacity_reflection_refraction_spectype", 0, 1, "Opacity", 1.0f, 0.0f, 0.0f, 0.0f);
    public static readonly RaytracingParameter LuminanceRange = new("mrgLuminancerange", 0, 1, "LuminanceRange", 0.0f, 0.0f, 0.0f, 0.0f);
    public static readonly RaytracingParameter FresnelParam = new("mrgFresnelParam", 0, 2, "FresnelParam", 1.0f, 1.0f, 0.0f, 0.0f);
    public static readonly RaytracingParameter SonicEyeHighLightPosition = new("g_SonicEyeHighLightPosition", 0, 3, "SonicEyeHighLightPosition", 0.0f, 0.0f, 0.0f, 0.0f);
    public static readonly RaytracingParameter SonicEyeHighLightColor = new("g_SonicEyeHighLightColor", 0, 3, "SonicEyeHighLightColor", 1.0f, 1.0f, 1.0f, 0.0f);
    public static readonly RaytracingParameter SonicSkinFalloffParam = new("g_SonicSkinFalloffParam", 0, 3, "SonicSkinFalloffParam", 0.15f, 2.0f, 3.0f, 0.0f);
    public static readonly RaytracingParameter ChrEmissionParam = new("mrgChrEmissionParam", 0, 4, "ChrEmissionParam", 0.0f, 0.0f, 0.0f, 0.0f);
    public static readonly RaytracingParameter TransColorMask = new("g_TransColorMask", 0, 3, "TransColorMask", 0.0f, 0.0f, 0.0f, 0.0f);
    public static readonly RaytracingParameter EmissionParam = new("g_EmissionParam", 0, 4, "EmissionParam", 0.0f, 0.0f, 0.0f, 1.0f);
    public static readonly RaytracingParameter OffsetParam = new("g_OffsetParam", 0, 2, "OffsetParam", 0.1f, 0.1f, 0.0f, 0.0f);
    public static readonly RaytracingParameter WaterParam = new("g_WaterParam", 0, 4, "WaterParam", 1.0f, 0.5f, 0.0f, 8.0f);
    public static readonly RaytracingParameter FurParam = new("FurParam", 0, 4, "FurParam", 0.1f, 8.0f, 8.0f, 1.0f);
    public static readonly RaytracingParameter FurParam2 = new("FurParam2", 0, 4, "FurParam2", 0.0f, 0.6f, 0.5f, 1.0f);
    public static readonly RaytracingParameter ChrEyeFHL1 = new("ChrEyeFHL1", 0, 4, "ChrEyeFHL1", 0.03f, -0.05f, 0.01f, 0.01f);
    public static readonly RaytracingParameter ChrEyeFHL2 = new("ChrEyeFHL2", 0, 4, "ChrEyeFHL2", 0.02f, 0.09f, 0.12f, 0.07f);
    public static readonly RaytracingParameter ChrEyeFHL3 = new("ChrEyeFHL3", 0, 4, "ChrEyeFHL3", 0.0f, 0.0f, 0.1f, 0.1f);
    public static readonly RaytracingParameter DistortionParam = new("mrgDistortionParam", 0, 4, "DistortionParam", 30.0f, 1.0f, 1.0f, 0.03f);
    public static readonly RaytracingParameter IrisColor = new("IrisColor", 0, 3, "IrisColor", 0.5f, 0.5f, 0.5f, 0.0f);
    public static readonly RaytracingParameter PupilParam = new("PupilParam", 0, 4, "PupilParam", 1.03f, 0.47f, 0.0f, 1024.0f);
    public static readonly RaytracingParameter HighLightColor = new("HighLightColor", 0, 3, "HighLightColor", 0.5f, 0.5f, 0.5f, 0.0f);

    public static readonly RaytracingParameter[] AllParameters = [
        Diffuse,
        Ambient,
        Specular,
        Emissive,
        GlossLevel,
        Opacity,
        LuminanceRange,
        FresnelParam,
        SonicEyeHighLightPosition,
        SonicEyeHighLightColor,
        SonicSkinFalloffParam,
        ChrEmissionParam,
        TransColorMask,
        EmissionParam,
        OffsetParam,
        WaterParam,
        FurParam,
        FurParam2,
        ChrEyeFHL1,
        ChrEyeFHL2,
        ChrEyeFHL3,
        DistortionParam,
        IrisColor,
        PupilParam,
        HighLightColor];
}

public class RaytracingShader(string name, RaytracingTexture[] textures, RaytracingParameter[] parameters)
{
    public string Name { get; set; } = name;
    public RaytracingTexture[] Textures { get; set; } = textures;
    public RaytracingParameter[] Parameters { get; set; } = parameters;

    public static readonly RaytracingShader SysError = new("SYS_ERROR",
        [
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Opacity
        ]);

    public static readonly RaytracingShader Blend = new("BLEND",
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

    public static readonly RaytracingShader ChrEye = new("CHR_EYE",
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

    public static readonly RaytracingShader ChrEyeFHLProcedural = new("CHR_EYE_FHL_PROCEDURAL",
        [
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Specular,
            RaytracingParameter.GlossLevel,
            RaytracingParameter.Opacity,
            RaytracingParameter.ChrEyeFHL1,
            RaytracingParameter.ChrEyeFHL2,
            RaytracingParameter.ChrEyeFHL3,
            RaytracingParameter.IrisColor,
            RaytracingParameter.PupilParam,
            RaytracingParameter.HighLightColor
        ]);

    public static readonly RaytracingShader ChrEyeFHL = new("CHR_EYE_FHL",
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

    public static readonly RaytracingShader ChrSkin = new("CHR_SKIN",
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

    public static readonly RaytracingShader ChrSkinHalf = new("CHR_SKIN_HALF",
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

    public static readonly RaytracingShader ChrSkinIgnore = new("CHR_SKIN_IGNORE",
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

    public static readonly RaytracingShader Cloud = new("CLOUD",
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

    public static readonly RaytracingShader Common = new("COMMON",
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

    public static readonly RaytracingShader Dim = new("DIM",
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

    public static readonly RaytracingShader Distortion = new("DISTORTION",
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

    public static readonly RaytracingShader DistortionOverlay = new("DISTORTION_OVERLAY",
        [
            RaytracingTexture.Diffuse,
            RaytracingTexture.Normal,
            RaytracingTexture.Normal2,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Opacity,
            RaytracingParameter.DistortionParam
        ]);

    public static readonly RaytracingShader EnmEmission = new("ENM_EMISSION",
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

    public static readonly RaytracingShader EnmGlass = new("ENM_GLASS",
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

    public static readonly RaytracingShader EnmIgnore = new("ENM_IGNORE",
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

    public static readonly RaytracingShader FadeOutNormal = new("FADE_OUT_NORMAL",
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

    public static readonly RaytracingShader FallOff = new("FALLOFF",
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

    public static readonly RaytracingShader FallOffV = new("FALLOFF_V",
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

    public static readonly RaytracingShader Fur = new("FUR",
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

    public static readonly RaytracingShader Glass = new("GLASS",
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

    public static readonly RaytracingShader Ice = new("ICE",
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

    public static readonly RaytracingShader IgnoreLight = new("IGNORE_LIGHT",
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

    public static readonly RaytracingShader IgnoreLightTwice = new("IGNORE_LIGHT_TWICE",
        [
            RaytracingTexture.Diffuse,
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Opacity,
        ]);

    public static readonly RaytracingShader Indirect = new("INDIRECT",
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

    public static readonly RaytracingShader IndirectNoLight = new("INDIRECT_NO_LIGHT",
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

    public static readonly RaytracingShader IndirectV = new("INDIRECT_V",
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

    public static readonly RaytracingShader Luminescence = new("LUMINESCENCE",
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

    public static readonly RaytracingShader LuminescenceV = new("LUMINESCENCE_V",
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

    public static readonly RaytracingShader Metal = new("METAL",
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

    public static readonly RaytracingShader Mirror = new("MIRROR",
        [
            RaytracingTexture.Diffuse
        ],
        [
            RaytracingParameter.Diffuse,
            RaytracingParameter.Opacity,
            RaytracingParameter.FresnelParam
        ]);

    public static readonly RaytracingShader Ring = new("RING",
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

    public static readonly RaytracingShader Shoe = new("SHOE",
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

    public static readonly RaytracingShader TimeEater = new("TIME_EATER",
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

    public static readonly RaytracingShader TransThin = new("TRANS_THIN",
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

    public static readonly RaytracingShader WaterAdd = new("WATER_ADD",
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

    public static readonly RaytracingShader WaterMul = new("WATER_MUL",
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

    public static readonly RaytracingShader WaterOpacity = new("WATER_OPACITY",
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
        ("ChrEyeFHLProcedural", RaytracingShader.ChrEyeFHLProcedural),
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

        writer.WriteLine("#pragma once\n");

        for (int i = 0; i < Shaders.Count; i++)
            writer.WriteLine("#define SHADER_TYPE_{0} {1}", Shaders[i].Name, i);

        writer.WriteLine("#define SHADER_TYPE_MAX {0}", Shaders.Count);
    }

    private static void WriteHitGroupsHeaderFile(string solutionDirectoryPath)
    {
        using var writer = new StreamWriter(
            Path.Combine(solutionDirectoryPath, "GenerationsRaytracing.X64", "HitGroups.h"));

        writer.WriteLine("#pragma once");

        writer.WriteLine("\ninline const wchar_t* s_shaderHitGroups[] =");
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

                         """);

        foreach (var texture in RaytracingTexture.AllTextures)
        {
            writer.WriteLine("inline RaytracingTexture s_texture_{0} = {{ \"{1}\", {2}, {3} }};", 
                texture.FieldName, texture.Name, texture.Index, texture.Name == "opacity" ? "true" : "false");
        }

        writer.WriteLine();

        foreach (var parameter in RaytracingParameter.AllParameters)
        { 
            writer.WriteLine("inline RaytracingParameter s_parameter_{0} = {{ \"{1}\", {2}, {3}, {4:0.0##}f, {5:0.0##}f, {6:0.0##}f, {7:0.0##}f }};", 
                parameter.FieldName, parameter.Name, parameter.Index, parameter.Size, parameter.X, parameter.Y, parameter.Z, parameter.W);
        }

        foreach (var shader in Shaders)
        {
            if (shader.Textures.Length > 0)
            { 
                writer.WriteLine("\ninline RaytracingTexture* s_textures_{0}[{1}] =", shader.Name, shader.Textures.Length);
                writer.WriteLine("{");

                foreach (var texture in shader.Textures)
                    writer.WriteLine("\t&s_texture_{0},", texture.FieldName);

                writer.WriteLine("};");
            }

            if (shader.Parameters.Length > 0)
            { 
                writer.WriteLine("\ninline RaytracingParameter* s_parameters_{0}[{1}] =", shader.Name, shader.Parameters.Length);
                writer.WriteLine("{");

                foreach (var parameter in shader.Parameters)
                    writer.WriteLine("\t&s_parameter_{0},", parameter.FieldName);

                writer.WriteLine("};");
            }

            writer.WriteLine("\ninline RaytracingShader s_shader_{0} = {{ SHADER_TYPE_{0}, {1}, {2}, {3}, {4} }};",
                shader.Name,
                shader.Textures.Length > 0 ? $"s_textures_{shader.Name}" : "nullptr",
                shader.Textures.Length,
                shader.Parameters.Length > 0 ? $"s_parameters_{shader.Name}" : "nullptr",
                shader.Parameters.Length);
        }

        writer.WriteLine("\ninline std::pair<std::string_view, RaytracingShader*> s_shaders[] =");
        writer.WriteLine("{");

        foreach (var (shaderName, shader) in ShaderPairs)
            writer.WriteLine("\t{{ \"{0}\", &s_shader_{1} }},", shaderName, shader.Name);

        writer.WriteLine("};");
    }

    private static void WriteMaterialHeaderFile(string solutionDirectoryPath)
    {
        int packedDataSize = AlignmentHelper.Align(1 + 
            Shaders.Select(x => x.Textures.Length + x.Parameters.Sum(y => y.Size)).Max(), 4) - 1;

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
                                 uint32_t packedData[{0}];
                             }};
                             """,
                packedDataSize);
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
                                 uint PackedData[{0}];
                             }};

                             """,
                packedDataSize);

            writer.WriteLine("""
                             Material GetMaterial(uint shaderType, MaterialData materialData)
                             {
                                Material material = (Material) 0;
                                material.Flags = materialData.Flags;
                                
                                switch (shaderType)
                                {
                             """);

            foreach (var shader in Shaders)
            {
                writer.WriteLine("    case SHADER_TYPE_{0}:", shader.Name);

                int index = 0;

                foreach (var texture in shader.Textures)
                { 
                    writer.WriteLine("        material.{0} = materialData.PackedData[{1}];", texture.FieldName, index);
                    ++index;
                }

                foreach (var parameter in shader.Parameters)
                {
                    if (parameter.Size > 1)
                    {
                        for (int i = 0; i < parameter.Size; i++)
                        {
                            writer.WriteLine("        material.{0}[{1}] = asfloat(materialData.PackedData[{2}]);",
                                parameter.FieldName, i, index);

                            ++index;
                        }
                    }
                    else
                    {
                        writer.WriteLine("        material.{0} = asfloat(materialData.PackedData[{1}]);",
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
            "-T lib_6_7",
            "-HV 2021",
            "-all-resources-bound",
            "-Zi",
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