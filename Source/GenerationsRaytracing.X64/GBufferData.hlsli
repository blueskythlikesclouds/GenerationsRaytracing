#ifndef GBUFFER_DATA_HLSLI_INCLUDED
#define GBUFFER_DATA_HLSLI_INCLUDED

#include "GeometryDesc.hlsli"
#include "MaterialData.hlsli"
#include "MaterialFlags.h"
#include "RootSignature.hlsli"
#include "ShaderType.h"
#include "SharedDefinitions.hlsli"

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
#define GBUFFER_FLAG_IS_GLASS_REFLECTION        (1 << 12)
#define GBUFFER_FLAG_IS_METALLIC                (1 << 13)
#define GBUFFER_FLAG_IS_WATER                   (1 << 14)
#define GBUFFER_FLAG_REFRACTION_ADD             (1 << 15)
#define GBUFFER_FLAG_REFRACTION_MUL             (1 << 16)
#define GBUFFER_FLAG_REFRACTION_OPACITY         (1 << 17)
#define GBUFFER_FLAG_IS_ADDITIVE                (1 << 18)

struct GBufferData
{
    float3 Position;
    uint Flags;

    float3 Diffuse;
    float Alpha;

    float Refraction;

    float3 Specular;
    float SpecularGloss;
    float SpecularLevel;
    float SpecularFresnel;

    float3 Normal;
    float3 Falloff;
    float3 Emission;
    float3 TransColor;
};

float ComputeFresnel(float3 normal)
{
    return pow(1.0 - saturate(dot(normal, -WorldRayDirection())), 5.0);
}

float ComputeFresnel(float3 normal, float2 fresnelParam)
{
    return lerp(fresnelParam.x, 1.0, ComputeFresnel(normal)) * fresnelParam.y;
}

float3 DecodeNormalMap(Vertex vertex, float2 value)
{
    float3 normalMap;
    normalMap.xy = value * 2.0 - 1.0;
    normalMap.z = sqrt(1.0 - saturate(dot(normalMap.xy, normalMap.xy)));

    return NormalizeSafe(
        vertex.Tangent * normalMap.x -
        vertex.Binormal * normalMap.y +
        vertex.Normal * normalMap.z);
}

float3 DecodeNormalMap(Vertex vertex, float4 value)
{
    value.x *= value.w;
    return DecodeNormalMap(vertex, value.xy);
}

float ComputeFalloff(float3 normal, float3 falloffParam)
{
    return pow(1.0 - saturate(dot(normal, -WorldRayDirection())), falloffParam.z) * falloffParam.y + falloffParam.x;
}

float2 ComputeEnvMapTexCoord(float3 eyeDirection, float3 normal)
{
    float4 C[4];
    C[0] = float4(0.5, 500, 5, 1);
    C[1] = float4(1024, 0, -2, 3);
    C[2] = float4(0.25, 4, 0, 0);
    C[3] = float4(-1, 1, 0, 0.5);
    float4 r3 = eyeDirection.xyzx;
    float4 r4 = normal.xyzx;
    float4 r1, r2, r6; 
    r1.w = dot(r3.yzw, r4.yzw);
    r1.w = r1.w + r1.w;
    r2 = r1.wwww * r4.xyzw + -r3.xyzw;
    r3 = r2.wyzw * C[3].xxyz + C[3].zzzw;
    r6 = r2.xyzw * C[3].yxxz;
    r2 = select(r2.zzzz >= 0, r3.xyzw, r6.xyzw);
    r1.w = r2.z + C[0].w;
    r1.w = 1.0 / r1.w;
    r2.xy = r2.yx * r1.ww + C[0].ww;
    r3.x = r2.y * C[2].x + r2.w;
    r3.y = r2.x * C[0].x;
    return r3.xy;
}

void CreateWaterGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IGNORE_LOCAL_LIGHT | 
        GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_IS_WATER;

    float2 offset = material.WaterParam.xy * g_TimeParam.y * 0.08;
    float4 decal = SampleMaterialTexture2D(material.DiffuseTexture, vertex, float2(0.0, offset.x));
    
    gBufferData.Diffuse = decal.rgb * vertex.Color.rgb;
    gBufferData.Alpha = decal.a * vertex.Color.a;
    
    float4 normal1 = SampleMaterialTexture2D(material.NormalTexture, vertex, float2(0.0, offset.x));
    float4 normal2 = SampleMaterialTexture2D(material.NormalTexture2, vertex, float2(0.0, offset.y));
    
    gBufferData.Normal = NormalizeSafe(DecodeNormalMap(vertex, normal1) + DecodeNormalMap(vertex, normal2));
}

float4 ApplyFurParamTransform(float4 value, float furParam)
{
    value = value * 2.0 - 1.0;
    value = exp(-value * furParam);
    value = (1.0 - value) / (1.0 + value);
    float num = exp(-furParam);
    value *= 1.0 + num;
    value /= 1.0 - num;
    return value * 0.5 + 0.5;
}

float ConvertSpecularGlossToRoughness(float specularGloss)
{
    return 1.0 - pow(specularGloss, 0.2) * 0.25;
}

GBufferData CreateGBufferData(Vertex vertex, Material material, uint shaderType)
{
    GBufferData gBufferData = (GBufferData) 0;

    gBufferData.Position = vertex.SafeSpawnPoint;
    gBufferData.Diffuse = material.Diffuse.rgb;
    gBufferData.Alpha = material.Diffuse.a * material.OpacityReflectionRefractionSpectype.x;
    gBufferData.Specular = material.Specular.rgb;
    gBufferData.SpecularGloss = material.GlossLevel.x * 500.0;
    gBufferData.SpecularLevel = material.GlossLevel.y * 5.0;
    gBufferData.Normal = vertex.Normal;

    switch (shaderType)
    {
        case SHADER_TYPE_BLEND:
            {
                float blendFactor = material.OpacityTexture != 0 ?
                    SampleMaterialTexture2D(material.OpacityTexture, vertex).x : vertex.Color.x;
                
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                if (material.DiffuseTexture2 != 0)
                    diffuse = lerp(diffuse, SampleMaterialTexture2D(material.DiffuseTexture2, vertex), blendFactor);
                
                gBufferData.Diffuse *= diffuse.rgb;
                gBufferData.Alpha *= diffuse.a;

                if (material.SpecularTexture != 0)
                {
                    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                    if (material.SpecularTexture2 != 0)
                        specular = lerp(specular, SampleMaterialTexture2D(material.SpecularTexture2, vertex), blendFactor);

                    gBufferData.Specular *= specular.rgb;
                }

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    if (material.GlossTexture2 != 0)
                        gloss = lerp(gloss, SampleMaterialTexture2D(material.GlossTexture2, vertex).x, blendFactor);

                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }
                else if (material.SpecularTexture == 0)
                {
                    gBufferData.Specular = 0.0;
                }

                if (material.NormalTexture != 0)
                {
                    float4 normal = SampleMaterialTexture2D(material.NormalTexture, vertex);
                    if (material.NormalTexture2 != 0)
                        normal = lerp(normal, SampleMaterialTexture2D(material.NormalTexture2, vertex), blendFactor);

                    gBufferData.Normal = DecodeNormalMap(vertex, normal);
                }
                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;
                
                break;
            }

        case SHADER_TYPE_CHR_EYE:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                float2 gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).xy;
                float4 normalMap = SampleMaterialTexture2D(material.NormalTexture, vertex);

                float3 highlightNormal = DecodeNormalMap(vertex, normalMap.xy);
                float3 lightDirection = NormalizeSafe(vertex.Position - material.SonicEyeHighLightPosition.xyz);
                float3 halfwayDirection = NormalizeSafe(-WorldRayDirection() + lightDirection);

                float highlightSpecular = saturate(dot(highlightNormal, halfwayDirection));
                highlightSpecular = pow(highlightSpecular, max(1.0, min(1024.0, gBufferData.SpecularGloss * gloss.x)));
                highlightSpecular *= gBufferData.SpecularLevel * gloss.x;
                highlightSpecular *= ComputeFresnel(vertex.Normal) * 0.7 + 0.3;

                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a;

                gBufferData.Specular *= vertex.Color.a;
                gBufferData.SpecularGloss *= gloss.y;    
                gBufferData.SpecularLevel *= gloss.y;
                gBufferData.SpecularFresnel = ComputeFresnel(DecodeNormalMap(vertex, normalMap.zw)) * 0.7 + 0.3;
                gBufferData.Emission = highlightSpecular * material.SonicEyeHighLightColor.rgb;

                break;
            }

        case SHADER_TYPE_CHR_EYE_FHL:
            {
                float3 direction = NormalizeSafe(mul(float4(material.SonicEyeHighLightPosition.xyz, 0.0), g_MtxView).xyz);
                float2 offset = direction.xy * float2(-1.0, 1.0);

                const float4 ChrEyeFHL1 = float4(0.03, -0.05, 0.01, 0.01);
                const float4 ChrEyeFHL2 = float4(0.02, 0.09, 0.12, 0.07);
                const float4 ChrEyeFHL3 = float4(0.0, 0.0, 0.1, 0.1);

                float2 pupilOffset = -ChrEyeFHL1.zw * offset;
                float2 highLightOffset = ChrEyeFHL3.xy + ChrEyeFHL3.zw * offset;
                float2 catchLightOffset = ChrEyeFHL1.xy - float2(offset.x < 0 ? ChrEyeFHL2.x : ChrEyeFHL2.y, offset.y < 0 ? ChrEyeFHL2.z : ChrEyeFHL2.w) * offset;

                float3 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex).rgb;
                float pupil = SampleMaterialTexture2D(material.DiffuseTexture, vertex, pupilOffset).w;
                float3 highLight = SampleMaterialTexture2D(material.LevelTexture, vertex, highLightOffset).rgb;
                float catchLight = SampleMaterialTexture2D(material.LevelTexture, vertex, catchLightOffset).w > 0.5 ? 1.0 : 0.0;
                float4 mask = SampleMaterialTexture2D(material.DisplacementTexture, vertex);

                gBufferData.Diffuse *= diffuse * pupil * (1.0 - catchLight);
                gBufferData.Specular *= pupil * mask.b * vertex.Color.w * (1.0 - catchLight);
                gBufferData.SpecularFresnel = ComputeFresnel(vertex.Normal) * 0.7 + 0.3;
                gBufferData.Emission = (highLight * pupil * mask.w * (1.0 - catchLight) + catchLight) / GetExposure();

                break;
            }

        case SHADER_TYPE_CHR_SKIN:
            {
                gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);

                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                gBufferData.Specular *= specular.rgb * vertex.Color.rgb;

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

                gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz) * vertex.Color.rgb;
                if (material.DisplacementTexture != 0)
                    gBufferData.Falloff *= SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

                break;
            }

        case SHADER_TYPE_CHR_SKIN_HALF:
            {
                gBufferData.Flags = GBUFFER_FLAG_HALF_LAMBERT;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                gBufferData.Specular *= specular.rgb * vertex.Color.rgb;
                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

                gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz) * vertex.Color.rgb;

                float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;
                float4 reflection = SampleMaterialTexture2D(material.ReflectionTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0);
                gBufferData.Diffuse *= reflection.rgb;

                break;
            }

        case SHADER_TYPE_CHR_SKIN_IGNORE:
            {
                gBufferData.Flags = GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                gBufferData.Specular *= specular.rgb * vertex.Color.rgb;
                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

                gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz) * vertex.Color.rgb;

                gBufferData.Emission = material.ChrEmissionParam.rgb;

                if (material.DisplacementTexture != 0)
                    gBufferData.Emission += SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

                gBufferData.Emission *= material.Ambient.rgb * material.ChrEmissionParam.w * vertex.Color.rgb;

                float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;
                float4 reflection = SampleMaterialTexture2D(material.ReflectionTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0);

                gBufferData.Diffuse *= reflection.rgb;
                gBufferData.Emission += gBufferData.Diffuse;

                break;
            }

        case SHADER_TYPE_CLOUD:
            {
                gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;

                gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

                gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz) * vertex.Color.rgb;
                gBufferData.Falloff *= SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

                float3 viewNormal = mul(float4(vertex.Normal, 0.0), g_MtxView).xyz;
                float4 reflection = SampleMaterialTexture2D(material.ReflectionTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0);
                gBufferData.Alpha *= reflection.a * vertex.Color.a;

                break;
            }

#ifdef ENABLE_SYS_ERROR_FALLBACK
        default:
#endif
        case SHADER_TYPE_COMMON:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                if (material.OpacityTexture != 0)
                    gBufferData.Alpha *= SampleMaterialTexture2D(material.OpacityTexture, vertex).x;
                
                if (material.SpecularTexture != 0)
                {
                    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                    gBufferData.Specular *= specular.rgb * vertex.Color.rgb;
                }

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }
                else if (material.SpecularTexture == 0)
                {
                    gBufferData.Specular = 0.0;
                }

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;
            
                break;
            }

        case SHADER_TYPE_DIM:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;

                gBufferData.Emission = material.Ambient.rgb * vertex.Color.rgb * 
                    SampleMaterialTexture2D(material.ReflectionTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0).rgb;

                break;
            }

        case SHADER_TYPE_DISTORTION:
            {
                gBufferData.Flags = GBUFFER_FLAG_REFRACTION_ADD;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                gBufferData.Specular *= specular.rgb * vertex.Color.rgb;

                gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                if (material.NormalTexture2 != 0)
                    gBufferData.Normal = NormalizeSafe(gBufferData.Normal + DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture2, vertex)));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                break;
            }

        case SHADER_TYPE_DISTORTION_OVERLAY:
            {
                gBufferData.Flags =
                    GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT |
                    GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION | GBUFFER_FLAG_IGNORE_REFLECTION | GBUFFER_FLAG_REFRACTION_MUL;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                if (material.NormalTexture2 != 0)
                    gBufferData.Normal = NormalizeSafe(gBufferData.Normal + DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture2, vertex)));

                break;
            }

        case SHADER_TYPE_ENM_EMISSION:
            {
                gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                if (material.SpecularTexture != 0)
                {
                    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                    gBufferData.Specular *= specular.rgb * vertex.Color.rgb;
                    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;
                }
                else
                {
                    gBufferData.Specular = 0.0;
                }

                gBufferData.Emission = material.ChrEmissionParam.rgb;

                if (material.DisplacementTexture != 0)
                    gBufferData.Emission += SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

                gBufferData.Emission *= material.Ambient.rgb * material.ChrEmissionParam.w * vertex.Color.rgb;

                break;
            }

        case SHADER_TYPE_ENM_GLASS:
            {
                gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;

                float4 diffuse = 0.0;

                if (material.DiffuseTexture != 0)
                {
                    diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                }

                if (material.SpecularTexture != 0)
                {
                    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                    gBufferData.Specular *= specular.rgb * vertex.Color.rgb;
                }

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

                float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;
                float4 reflection = SampleMaterialTexture2D(material.ReflectionTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0);
                gBufferData.Alpha *= saturate(diffuse.a + reflection.a) * vertex.Color.a;

                gBufferData.Emission = material.ChrEmissionParam.rgb;

                if (material.DisplacementTexture != 0)
                    gBufferData.Emission += SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

                gBufferData.Emission *= material.Ambient.rgb * material.ChrEmissionParam.w * vertex.Color.rgb;

                break;
            }

        case SHADER_TYPE_ENM_IGNORE:
            {
                gBufferData.Flags = GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT;

                if (material.DiffuseTexture != 0)
                {
                    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                    gBufferData.Alpha *= diffuse.a * vertex.Color.a;
                }

                if (material.SpecularTexture != 0)
                {
                    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                    gBufferData.Specular *= specular.rgb * vertex.Color.rgb;
                }
                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

                gBufferData.Emission = material.ChrEmissionParam.rgb;

                if (material.DisplacementTexture != 0)
                    gBufferData.Emission += SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

                gBufferData.Emission *= material.Ambient.rgb * material.ChrEmissionParam.w * vertex.Color.rgb;

                break;
            }

        case SHADER_TYPE_FADE_OUT_NORMAL:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb;
                gBufferData.Alpha *= diffuse.a;

                float3 normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
                gBufferData.Normal = NormalizeSafe(lerp(normal, gBufferData.Normal, vertex.Color.x));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                break;
            }

        case SHADER_TYPE_FALLOFF:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;
                }
                else
                {
                    gBufferData.Specular = 0.0;
                }

                float3 viewNormal = mul(float4(vertex.Normal, 0.0), g_MtxView).xyz;
                gBufferData.Emission = SampleMaterialTexture2D(material.DisplacementTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0).rgb;

                break;
            }

        case SHADER_TYPE_FALLOFF_V:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb;
                gBufferData.Alpha *= diffuse.a;

                if (material.NormalTexture != 0)
                {
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                    if (material.NormalTexture2 != 0)
                    {
                        float3 normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture2, vertex));
                        gBufferData.Normal = NormalizeSafe(gBufferData.Normal + normal);
                    }
                }

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;
                }
                else
                {
                    gBufferData.Specular = 0.0;
                }

                float fresnel = 1.0 - saturate(dot(-WorldRayDirection(), vertex.Normal));
                fresnel *= fresnel;
                gBufferData.Emission = fresnel * vertex.Color.rgb;

                break;
            }

        case SHADER_TYPE_FUR:
            {
                gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                gBufferData.Specular *= specular.rgb * vertex.Color.rgb;

                gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture2, vertex));
                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

                gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz) * vertex.Color.rgb;
                gBufferData.Falloff *= SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb; 
                
                if (vertex.Flags & VERTEX_FLAG_MIPMAP)
                {
                    float2 flow = SampleMaterialTexture2D(material.NormalTexture, vertex).xy;
                    flow += 1.0 / 510.0;
                    flow = flow * 2.0 - 1.0;
                    flow *= 0.01 * material.FurParam.x;

                    float3 furColor = 1.0;
                    float furAlpha = 0.0;
                    float2 offset = 0.0;

                    for (uint i = 0; i < (uint) material.FurParam.z; i++)
                    {
                        float4 fur = SampleMaterialTexture2D(material.DiffuseTexture2, vertex, offset, material.FurParam.y);

                        float factor = (float) i / material.FurParam.z;
                        fur.rgb *= (1.0 - material.FurParam2.z) * factor + material.FurParam2.z;
                        fur.rgb *= fur.w * material.FurParam2.y;

                        furColor *= 1.0 - fur.w * material.FurParam2.y;
                        furColor += fur.rgb;
                        furAlpha += fur.w;

                        offset += flow;
                    }

                    furColor = ApplyFurParamTransform(float4(furColor, 0.0), material.FurParam.w).rgb;
                    furAlpha = ApplyFurParamTransform(float4(furAlpha / material.FurParam.z, 0.0, 0.0, 0.0), material.FurParam2.w).x;

                    // Convert to sRGB, the logic above is from Frontiers, meaning it was in linear space.
                    furColor = pow(saturate(furColor), 1.0 / 2.2);

                    gBufferData.Diffuse *= furColor;
                    gBufferData.Specular *= furColor;
                    gBufferData.SpecularGloss *= furAlpha;
                    gBufferData.Falloff *= furColor;

                    // Divide by last mip map to neutralize the color.
                    Texture2D furTexture = ResourceDescriptorHeap[
                        NonUniformResourceIndex(material.DiffuseTexture2 & 0xFFFFF)];
                
                    furColor = furTexture.Load(int3(0, 0, 8)).rgb;
                    furColor = 1.0 / pow(saturate(furColor), 1.0 / 2.2);

                    gBufferData.Diffuse = saturate(gBufferData.Diffuse * furColor.rgb);
                    gBufferData.Specular = saturate(gBufferData.Specular * furColor.rgb);
                    gBufferData.Falloff *= furColor.rgb;
                }

                break;
            }

        case SHADER_TYPE_GLASS:
            {
                gBufferData.Flags = GBUFFER_FLAG_IS_GLASS_REFLECTION;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb;
                gBufferData.Alpha *= diffuse.a;

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal, material.FresnelParam.xy);

                if (material.SpecularTexture != 0)
                {
                    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                    gBufferData.Specular *= specular.rgb;
                    gBufferData.SpecularFresnel *= specular.w;
                }

                if (material.GlossTexture != 0)
                {
                    float4 gloss = SampleMaterialTexture2D(material.GlossTexture, vertex);
                    if (material.SpecularTexture != 0)
                    {
                        gBufferData.SpecularGloss *= gloss.x;
                        gBufferData.SpecularLevel *= gloss.x;
                    }
                    else
                    {
                        gBufferData.Specular *= gloss.rgb;
                        gBufferData.SpecularFresnel *= gloss.w;
                    }
                }
                else if (material.SpecularTexture == 0)
                {
                    gBufferData.Specular = 0.0;
                }

                float3 visibilityFactor = gBufferData.Specular * gBufferData.SpecularFresnel * 0.5;
                gBufferData.Alpha = sqrt(max(gBufferData.Alpha * gBufferData.Alpha, dot(visibilityFactor, visibilityFactor)));

                break;
            }

        case SHADER_TYPE_ICE:
            {
                gBufferData.Flags = GBUFFER_FLAG_IS_MIRROR_REFLECTION;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularFresnel *= gloss;
                }

                break;
            }

        case SHADER_TYPE_IGNORE_LIGHT:
            {
                gBufferData.Flags =
                    GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT |
                    GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION | GBUFFER_FLAG_IGNORE_REFLECTION;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);

                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                if (material.OpacityTexture != 0)
                    gBufferData.Alpha *= SampleMaterialTexture2D(material.OpacityTexture, vertex).x;

                if (material.DisplacementTexture != 0)
                {
                    gBufferData.Emission = SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;
                    gBufferData.Emission += material.EmissionParam.rgb;
                    gBufferData.Emission *= material.Ambient.rgb * material.EmissionParam.w;
                }
                gBufferData.Emission += gBufferData.Diffuse;
                gBufferData.Diffuse = 0.0;

                break;
            }
        
        case SHADER_TYPE_IGNORE_LIGHT_TWICE:
            {
                gBufferData.Flags =
                    GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT |
                    GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION | GBUFFER_FLAG_IGNORE_REFLECTION;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);

                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb * 2.0;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                gBufferData.Emission = gBufferData.Diffuse;
                gBufferData.Diffuse = 0.0;

                break;
            }

        case SHADER_TYPE_INDIRECT:
            {
                float4 offset = SampleMaterialTexture2D(material.DisplacementTexture, vertex);
                offset.xy = (offset.wx * 2.0 - 1.0) * material.OffsetParam.xy * 3.0;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex, offset.xy);
                float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex, offset.xy).x;

                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                gBufferData.SpecularGloss *= gloss;
                gBufferData.SpecularLevel *= gloss;

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex, offset.xy));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                break;
            }

        case SHADER_TYPE_INDIRECT_V_NO_GI_SHADOW:
            {
                gBufferData.Flags = GBUFFER_FLAG_IGNORE_SHADOW;
                // fallthrough
            }

        case SHADER_TYPE_INDIRECT_V:
            {
                float4 offset = SampleMaterialTexture2D(material.DisplacementTexture, vertex);

                offset.xy = (offset.wx * 2.0 - 1.0) * material.OffsetParam.xy * 3.0 * vertex.Color.w;
                offset.xy *= SampleMaterialTexture2D(material.OpacityTexture, vertex).x;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex, offset.xy);
                float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex, offset.xy).x;

                gBufferData.Diffuse *= diffuse.rgb;
                gBufferData.Alpha *= diffuse.a;

                gBufferData.SpecularGloss *= gloss;
                gBufferData.SpecularLevel *= gloss;

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex, offset.xy));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                gBufferData.Emission = vertex.Color.rgb;

                break;
            }

        case SHADER_TYPE_LUMINESCENCE:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;
                }
                else
                {
                    gBufferData.Specular = 0.0;
                }

                gBufferData.Emission = material.DisplacementTexture != 0 ? 
                    SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb : material.Emissive.rgb;

                gBufferData.Emission *= material.Ambient.rgb;
                break;
            }

        case SHADER_TYPE_LUMINESCENCE_V:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                if (material.DiffuseTexture2 != 0)
                    diffuse = lerp(diffuse, SampleMaterialTexture2D(material.DiffuseTexture2, vertex), vertex.Color.w);
                
                gBufferData.Diffuse *= diffuse.rgb;
                gBufferData.Alpha *= diffuse.a;

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    if (material.GlossTexture2 != 0)
                        gloss = lerp(gloss, SampleMaterialTexture2D(material.GlossTexture2, vertex).x, vertex.Color.w);

                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                }
                else
                {
                    gBufferData.Specular = 0.0;
                }

                gBufferData.Emission = vertex.Color.rgb * material.Ambient.rgb;

                if (material.DisplacementTexture != 0)
                    gBufferData.Emission *= SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

                break;
            }

        case SHADER_TYPE_METAL:
            {
                gBufferData.Flags = GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT | GBUFFER_FLAG_IS_METALLIC;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);

                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                gBufferData.Specular *= specular.rgb * vertex.Color.rgb;

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal);

                break;
            }

        case SHADER_TYPE_MIRROR:
            {
                gBufferData.Flags = GBUFFER_FLAG_IS_MIRROR_REFLECTION;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                gBufferData.Specular = 0.0;
                gBufferData.SpecularFresnel = ComputeFresnel(vertex.Normal, material.FresnelParam.xy);

                break;
            }

        case SHADER_TYPE_RING:
            {
                gBufferData.Flags = GBUFFER_FLAG_MUL_BY_SPEC_GLOSS;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb;
                gBufferData.Alpha *= diffuse.a;

                float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
                gBufferData.Specular *= specular.rgb;
                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

                float4 reflection = SampleMaterialTexture2D(material.ReflectionTexture,
                    ComputeEnvMapTexCoord(-WorldRayDirection(), gBufferData.Normal), 0);

                gBufferData.Emission = reflection.rgb * material.LuminanceRange.x * reflection.a + reflection.rgb;
                gBufferData.Emission *= specular.rgb * gBufferData.SpecularFresnel;

                break;
            }

        case SHADER_TYPE_SHOE:
            {
                gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);

                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;
                gBufferData.Specular *= specular.rgb * vertex.Color.rgb;

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }

                gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

                float fresnel = saturate(dot(gBufferData.Normal, -WorldRayDirection()));
                gBufferData.Diffuse *= fresnel * 0.8 + 0.2;

                break;
            }

        case SHADER_TYPE_TRANS_THIN:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
                gBufferData.Diffuse *= diffuse.rgb;
                gBufferData.Alpha *= diffuse.a;

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
                    gBufferData.SpecularGloss *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

                gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                gBufferData.TransColor = gBufferData.Diffuse * material.TransColorMask.rgb;

                break;
            }

        case SHADER_TYPE_WATER_ADD:
            {
                CreateWaterGBufferData(vertex, material, gBufferData);
                gBufferData.Flags |= GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT | GBUFFER_FLAG_REFRACTION_ADD;
                break;
            }

        case SHADER_TYPE_WATER_MUL:
            {
                CreateWaterGBufferData(vertex, material, gBufferData);
                gBufferData.Flags |= GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT | GBUFFER_FLAG_REFRACTION_MUL;
                break;
            }

        case SHADER_TYPE_WATER_OPACITY:
            {
                CreateWaterGBufferData(vertex, material, gBufferData);
                gBufferData.Flags |= GBUFFER_FLAG_REFRACTION_OPACITY;
                gBufferData.Refraction = gBufferData.Alpha;
                gBufferData.Alpha = 1.0;
                break;
            }
#ifndef ENABLE_SYS_ERROR_FALLBACK
        default:
            {
                gBufferData.Flags = 
                    GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT |
                    GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION | GBUFFER_FLAG_IGNORE_REFLECTION;

                gBufferData.Diffuse = 0.0;
                gBufferData.Emission = float3(1.0, 0.0, 0.0);
                break;
            }
#endif
    }

    gBufferData.SpecularGloss = clamp(gBufferData.SpecularGloss, 1.0, 1024.0);

    if (material.Flags & MATERIAL_FLAG_ADDITIVE)
        gBufferData.Flags |= GBUFFER_FLAG_IS_ADDITIVE;

    if (material.Flags & MATERIAL_FLAG_REFLECTION)
        gBufferData.Flags |= GBUFFER_FLAG_IS_MIRROR_REFLECTION;

    if (dot(gBufferData.Diffuse, gBufferData.Diffuse) == 0.0)
        gBufferData.Flags |= GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION;

    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_IS_GLASS_REFLECTION)) &&
        (dot(gBufferData.Specular, gBufferData.Specular) == 0.0 ||
        gBufferData.SpecularLevel == 0.0 || gBufferData.SpecularFresnel == 0.0))
    {
        gBufferData.Flags |= GBUFFER_FLAG_IGNORE_REFLECTION;
    }

    return gBufferData;
}

GBufferData LoadGBufferData(uint2 index)
{
    float4 gBuffer0 = g_GBuffer0[index];
    float4 gBuffer1 = g_GBuffer1[index];
    float4 gBuffer2 = g_GBuffer2[index];
    float4 gBuffer3 = g_GBuffer3[index];
    float4 gBuffer4 = g_GBuffer4[index];
    float4 gBuffer5 = g_GBuffer5[index];
    float4 gBuffer6 = g_GBuffer6[index];

    GBufferData gBufferData = (GBufferData) 0;

    gBufferData.Position = gBuffer0.xyz;
    gBufferData.Flags = asuint(gBuffer0.w);

    gBufferData.Diffuse = gBuffer1.rgb;
    gBufferData.Refraction = gBuffer1.a;

    gBufferData.Specular = gBuffer2.rgb;

    gBufferData.SpecularGloss = gBuffer3.x;
    gBufferData.SpecularLevel = gBuffer3.y;
    gBufferData.SpecularFresnel = gBuffer3.z;

    gBufferData.Normal = gBuffer4.xyz;
    gBufferData.Falloff = gBuffer5.xyz;
    gBufferData.Emission = gBuffer6.xyz;
    gBufferData.TransColor = float3(gBuffer2.w, gBuffer3.w, gBuffer5.w);

    return gBufferData;
}

void StoreGBufferData(uint2 index, GBufferData gBufferData)
{
    float roughness = 0.0;
    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_IS_GLASS_REFLECTION)))
        roughness = ConvertSpecularGlossToRoughness(gBufferData.SpecularGloss);

    g_GBuffer0[index] = float4(gBufferData.Position, asfloat(gBufferData.Flags));
    g_GBuffer1[index] = float4(gBufferData.Diffuse, gBufferData.Refraction);
    g_GBuffer2[index] = float4(gBufferData.Specular, gBufferData.TransColor.r);
    g_GBuffer3[index] = float4(gBufferData.SpecularGloss, gBufferData.SpecularLevel, gBufferData.SpecularFresnel, gBufferData.TransColor.g);
    g_GBuffer4[index] = float4(gBufferData.Normal, roughness);
    g_GBuffer5[index] = float4(gBufferData.Falloff, gBufferData.TransColor.b);
    g_GBuffer6[index] = float4(gBufferData.Emission, 0.0);
}

#endif
