#ifndef GBUFFER_DATA_H
#define GBUFFER_DATA_H

#include "GeometryDesc.hlsli"
#include "MaterialData.hlsli"
#include "RootSignature.hlsli"
#include "ShaderType.h"

#define GBUFFER_FLAG_MISS                     (1 << 0)
#define GBUFFER_FLAG_SKIP_GLOBAL_LIGHT        (1 << 1)
#define GBUFFER_FLAG_SKIP_EYE_LIGHT           (1 << 2)
#define GBUFFER_FLAG_SKIP_GLOBAL_ILLUMINATION (1 << 3)
#define GBUFFER_FLAG_SKIP_REFLECTION          (1 << 4)

struct GBufferData
{
    float3 Position;
    uint Flags;
    float3 Normal;
    float3 Diffuse;
    float Alpha;
    float3 Specular;
    float SpecularPower;
    float SpecularLevel;
    float3 Emission;
    float3 Falloff;
};

float ComputeFresnel(float3 normal)
{
    return pow(1.0 - saturate(dot(normal, -WorldRayDirection())), 5.0);
}

float ComputeFresnel(float3 normal, float2 fresnelParam)
{
    return lerp(1.0, ComputeFresnel(normal), fresnelParam.x) * fresnelParam.y;
}

float3 DecodeNormalMap(Vertex vertex, float2 value)
{
    float3 normalMap;
    normalMap.xy = value * 2.0 - 1.0;
    normalMap.z = sqrt(1.0 - saturate(dot(normalMap.xy, normalMap.xy)));

    return normalize(
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

GBufferData CreateGBufferData(Vertex vertex, Material material)
{
    GBufferData gBufferData = (GBufferData) 0;

    gBufferData.Position = vertex.Position;
    gBufferData.Normal = vertex.Normal;
    gBufferData.Diffuse = material.Diffuse.rgb;
    gBufferData.Alpha = material.OpacityReflectionRefractionSpectype.x;
    gBufferData.Specular = material.Specular.rgb;
    gBufferData.SpecularPower = material.PowerGlossLevel.y * 500.0;
    gBufferData.SpecularLevel = material.PowerGlossLevel.z * 5.0;

    switch (material.ShaderType)
    {
        case SHADER_TYPE_BLEND:
            {
                float blendFactor = material.OpacityTexture != 0 ?
                    SampleMaterialTexture2D(material.OpacityTexture, vertex.TexCoords).x : vertex.Color.x;
                
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords);
                if (material.DiffuseTexture2 != 0)
                    diffuse = lerp(diffuse, SampleMaterialTexture2D(material.DiffuseTexture2, vertex.TexCoords), blendFactor);
                
                gBufferData.Diffuse *= diffuse.rgb;
                gBufferData.Alpha *= diffuse.a;

                if (material.SpecularTexture != 0)
                {
                    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex.TexCoords);
                    if (material.SpecularTexture2 != 0)
                        specular = lerp(specular, SampleMaterialTexture2D(material.SpecularTexture2, vertex.TexCoords), blendFactor);

                    gBufferData.Specular *= specular.rgb;
                }

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex.TexCoords).x;
                    if (material.GlossTexture2 != 0)
                        gloss = lerp(gloss, SampleMaterialTexture2D(material.GlossTexture2, vertex.TexCoords).x, blendFactor);

                    gBufferData.SpecularPower *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }
                else if (material.SpecularTexture == 0)
                {
                    gBufferData.Specular = 0.0;
                }

                if (material.NormalTexture != 0)
                {
                    float4 normal = SampleMaterialTexture2D(material.NormalTexture, vertex.TexCoords);
                    if (material.NormalTexture2 != 0)
                        normal = lerp(normal, SampleMaterialTexture2D(material.NormalTexture2, vertex.TexCoords), blendFactor);

                    gBufferData.Normal = DecodeNormalMap(vertex, normal);
                }
                gBufferData.Specular *= ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;
                
                break;
            }

        case SHADER_TYPE_CHR_EYE:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords);
                float2 gloss = SampleMaterialTexture2D(material.GlossTexture, vertex.TexCoords).xy;
                float4 normalMap = SampleMaterialTexture2D(material.NormalTexture, vertex.TexCoords);

                float3 highlightNormal = DecodeNormalMap(vertex, normalMap.zw);
                float3 lightDirection = normalize(vertex.Position - material.SonicEyeHighLightPosition.xyz);
                float3 halfwayDirection = normalize(-WorldRayDirection() + lightDirection);

                float highlightSpecular = saturate(dot(highlightNormal, halfwayDirection));
                highlightSpecular = pow(highlightSpecular, max(1.0, gBufferData.SpecularPower * gloss.x));
                highlightSpecular *= gBufferData.SpecularLevel * gloss.x;

                gBufferData.Diffuse.rgb *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a;
                gBufferData.SpecularPower *= gloss.y;    
                gBufferData.SpecularLevel *= gloss.y;
                gBufferData.Normal = DecodeNormalMap(vertex, normalMap.xy);
                gBufferData.Specular *= (ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3) * vertex.Color.a;
                gBufferData.Emission = highlightSpecular * material.SonicEyeHighLightColor.rgb;

                break;
            }

        case SHADER_TYPE_CHR_SKIN:
        case SHADER_TYPE_ENM_METAL:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords);
                float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex.TexCoords);

                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Specular *= specular.rgb * vertex.Color.rgb;

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex.TexCoords).x;
                    gBufferData.SpecularPower *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex.TexCoords));

                gBufferData.Specular *= ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz) * vertex.Color.rgb;
                if (material.DisplacementTexture != 0)
                    gBufferData.Falloff *= SampleMaterialTexture2D(material.DisplacementTexture, vertex.TexCoords).rgb;

                break;
            }

        case SHADER_TYPE_COMMON:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;
                
                if (material.SpecularTexture != 0)
                {
                    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex.TexCoords);
                    gBufferData.Specular *= specular.rgb * vertex.Color.rgb;
                }

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex.TexCoords).x;
                    gBufferData.SpecularPower *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }
                else if (material.SpecularTexture == 0)
                {
                    gBufferData.Specular = 0.0;
                }

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex.TexCoords));

                gBufferData.Specular *= ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;
            
                break;
            }

        case SHADER_TYPE_ENM_EMISSION:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;
                
                if (material.SpecularTexture != 0)
                {
                    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex.TexCoords);
                    gBufferData.Specular *= specular.rgb * vertex.Color.rgb;
                }
                else
                {
                    gBufferData.Specular = 0.0;
                }

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex.TexCoords));

                gBufferData.Specular *= ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                gBufferData.Emission = material.ChrEmissionParam.rgb;

                if (material.DisplacementTexture != 0)
                    gBufferData.Emission += SampleMaterialTexture2D(material.DisplacementTexture, vertex.TexCoords).rgb;

                gBufferData.Emission *= material.Ambient.rgb * material.ChrEmissionParam.w * vertex.Color.rgb;

                break;
            }

        case SHADER_TYPE_IGNORE_LIGHT:
            {
                gBufferData.Flags =
                    GBUFFER_FLAG_SKIP_GLOBAL_LIGHT | GBUFFER_FLAG_SKIP_EYE_LIGHT |
                    GBUFFER_FLAG_SKIP_GLOBAL_ILLUMINATION | GBUFFER_FLAG_SKIP_REFLECTION;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords);

                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                if (material.OpacityTexture != 0)
                    gBufferData.Alpha *= SampleMaterialTexture2D(material.OpacityTexture, vertex.TexCoords).x;

                if (material.DisplacementTexture != 0)
                {
                    gBufferData.Emission = SampleMaterialTexture2D(material.DisplacementTexture, vertex.TexCoords).rgb;
                    gBufferData.Emission += material.EmissionParam.rgb;
                    gBufferData.Emission *= material.Ambient.rgb * material.EmissionParam.w;
                }
                gBufferData.Emission += gBufferData.Diffuse;

                break;
            }
        
        case SHADER_TYPE_IGNORE_LIGHT_TWICE:
            {
                gBufferData.Flags =
                    GBUFFER_FLAG_SKIP_GLOBAL_LIGHT | GBUFFER_FLAG_SKIP_EYE_LIGHT |
                    GBUFFER_FLAG_SKIP_GLOBAL_ILLUMINATION | GBUFFER_FLAG_SKIP_REFLECTION;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords);

                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb * 2.0;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                gBufferData.Emission = gBufferData.Diffuse;

                break;
            }

        case SHADER_TYPE_LUMINESCENCE:
            {
                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords);
                gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
                gBufferData.Alpha *= diffuse.a * vertex.Color.a;

                if (material.GlossTexture != 0)
                {
                    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex.TexCoords).x;
                    gBufferData.SpecularPower *= gloss;
                    gBufferData.SpecularLevel *= gloss;
                }
                else
                {
                    gBufferData.Specular = 0.0;
                }

                if (material.NormalTexture != 0)
                    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex.TexCoords));

                gBufferData.Specular *= ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

                gBufferData.Emission = material.DisplacementTexture != 0 ? 
                    SampleMaterialTexture2D(material.DisplacementTexture, vertex.TexCoords).rgb : material.Emissive.rgb;

                gBufferData.Emission *= material.Ambient.rgb;
                break;
            }

        case SHADER_TYPE_RING:
            {
                gBufferData.Flags = GBUFFER_FLAG_SKIP_REFLECTION;

                float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords);
                gBufferData.Diffuse *= diffuse.rgb;
                gBufferData.Alpha *= diffuse.a;

                float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex.TexCoords);
                gBufferData.Specular *= specular.rgb * gBufferData.SpecularPower;

                float4 reflection = SampleMaterialTexture2D(material.ReflectionTexture,
                    ComputeEnvMapTexCoord(-WorldRayDirection(), gBufferData.Normal));

                gBufferData.Emission = reflection.rgb * material.LuminanceRange.x * reflection.a + reflection.rgb;
                gBufferData.Emission *= specular.rgb * (ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3);

                break;
            }

        default:
            {
                gBufferData.Flags = 
                    GBUFFER_FLAG_SKIP_GLOBAL_LIGHT | GBUFFER_FLAG_SKIP_EYE_LIGHT |
                    GBUFFER_FLAG_SKIP_GLOBAL_ILLUMINATION | GBUFFER_FLAG_SKIP_REFLECTION;

                gBufferData.Emission = float3(1.0, 0.0, 0.0);
                break;
            }
    }

    gBufferData.SpecularPower = clamp(gBufferData.SpecularPower, 1.0, 1024.0);

    if (dot(gBufferData.Diffuse, gBufferData.Diffuse) == 0.0)
        gBufferData.Flags |= GBUFFER_FLAG_SKIP_GLOBAL_ILLUMINATION;

    if (dot(gBufferData.Specular, gBufferData.Specular) == 0.0 || gBufferData.SpecularLevel == 0.0)
        gBufferData.Flags |= GBUFFER_FLAG_SKIP_REFLECTION;

    return gBufferData;
}

GBufferData LoadGBufferData(uint2 index)
{
    GBufferData gBufferData = (GBufferData) 0;

    float4 positionAndFlags = g_PositionAndFlagsTexture[index];
    gBufferData.Position = positionAndFlags.xyz;
    gBufferData.Flags = asuint(positionAndFlags.w);
    gBufferData.Normal = normalize(g_NormalTexture[index] * 2.0 - 1.0);
    gBufferData.Diffuse = g_DiffuseTexture[index];
    gBufferData.Specular = g_SpecularTexture[index];
    gBufferData.SpecularPower = g_SpecularPowerTexture[index];
    gBufferData.SpecularLevel = g_SpecularLevelTexture[index];
    gBufferData.Emission = g_EmissionTexture[index];
    gBufferData.Falloff = g_FalloffTexture[index];

    return gBufferData;
}

void StoreGBufferData(uint2 index, GBufferData gBufferData)
{
    g_PositionAndFlagsTexture[index] = float4(gBufferData.Position, asfloat(gBufferData.Flags));
    g_NormalTexture[index] = gBufferData.Normal * 0.5 + 0.5;
    g_DiffuseTexture[index] = gBufferData.Diffuse;
    g_SpecularTexture[index] = gBufferData.Specular;
    g_SpecularPowerTexture[index] = gBufferData.SpecularPower;
    g_SpecularLevelTexture[index] = gBufferData.SpecularLevel;
    g_EmissionTexture[index] = gBufferData.Emission;
    g_FalloffTexture[index] = gBufferData.Falloff;
}

#endif
