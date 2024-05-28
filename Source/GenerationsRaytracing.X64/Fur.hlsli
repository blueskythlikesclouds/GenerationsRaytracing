#pragma once

#include "GBufferData.hlsli"

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

void CreateFurGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;
    
    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
    gBufferData.SpecularTint *= specular.rgb * vertex.Color.rgb;
    gBufferData.SpecularEnvironment *= specular.a;
    
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
        gBufferData.SpecularTint *= furColor;
        gBufferData.SpecularGloss *= furAlpha;
        gBufferData.Falloff *= furColor;
    
        // Divide by last mip map to neutralize the color.
        Texture2D furTexture = ResourceDescriptorHeap[
            NonUniformResourceIndex(material.DiffuseTexture2 & 0xFFFFF)];
        
        uint width, height, numberOfLevels;
        furTexture.GetDimensions(0, width, height, numberOfLevels);
    
        furColor = furTexture.Load(int3(0, 0, max(0, int(numberOfLevels) - 1))).rgb;
        furColor = 1.0 / pow(saturate(furColor), 1.0 / 2.2);
    
        gBufferData.Diffuse = saturate(gBufferData.Diffuse * furColor.rgb);
        gBufferData.SpecularTint = saturate(gBufferData.SpecularTint * furColor.rgb);
        gBufferData.Falloff *= furColor.rgb;
    }
}
