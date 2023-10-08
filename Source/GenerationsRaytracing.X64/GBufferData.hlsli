#ifndef GBUFFER_DATA_H
#define GBUFFER_DATA_H

#include "GeometryDesc.hlsli"
#include "MaterialData.hlsli"
#include "RootSignature.hlsli"
#include "ShaderType.h"

struct GBufferData
{
    float3 Position;
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

GBufferData MakeGBufferData(Vertex vertex, Material material)
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

        default:
            {
                gBufferData = (GBufferData) 0;
                gBufferData.Alpha = 1.0;
                gBufferData.Emission = float3(1.0, 0.0, 0.0);
                break;
            }
    }

    gBufferData.SpecularPower = clamp(gBufferData.SpecularPower, 1.0, 1024.0);

    return gBufferData;
}

#endif
