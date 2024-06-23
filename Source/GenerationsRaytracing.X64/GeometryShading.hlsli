#pragma once

#include "GBufferData.hlsli"
#include "Reservoir.hlsli"
#include "RootSignature.hlsli"
#include "ShaderTable.h"
#include "SharedDefinitions.hlsli"

struct ShadingParams
{
    float3 EyeDirection;
    Reservoir Reservoir;
    float3 GlobalIllumination;
    float3 Reflection;
    float3 Refraction;
};

float3 ComputeDirectLighting(GBufferData gBufferData, float3 eyeDirection,
    float3 lightDirection, float3 diffuseColor, float3 specularColor)
{
    float3 transColor = diffuseColor;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT))
    {
        diffuseColor *= gBufferData.Diffuse;

        float cosTheta = dot(gBufferData.Normal, lightDirection);

        transColor *= gBufferData.TransColor;
        transColor *= saturate(-cosTheta);

        if (gBufferData.Flags & GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT)
        {
            cosTheta = (cosTheta - 0.05) / (1.0 - 0.05);
        }

        else if (gBufferData.Flags & GBUFFER_FLAG_HALF_LAMBERT)
        {
            cosTheta = cosTheta * 0.5 + 0.5;
            cosTheta *= cosTheta;
        }
        
        if (!(gBufferData.Flags & GBUFFER_FLAG_FULBRIGHT))
            diffuseColor *= saturate(cosTheta);
        
        if (gBufferData.Flags & GBUFFER_FLAG_MUL_BY_REFRACTION)
            diffuseColor *= gBufferData.Refraction;
    }
    else
    {
        diffuseColor = 0.0;
        transColor = 0.0;
    }

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_SPECULAR_LIGHT))
    {
        float3 halfwayDirection = NormalizeSafe(lightDirection + eyeDirection);
        
        if (gBufferData.Flags & GBUFFER_FLAG_IS_METALLIC)
        {
            specularColor *= gBufferData.Specular;
            
            specularColor *= ComputeFresnel(gBufferData.SpecularTint,
                dot(halfwayDirection, eyeDirection));
        }
        else
        {
            specularColor *= gBufferData.Specular * gBufferData.SpecularTint;

            if (gBufferData.Flags & GBUFFER_FLAG_MUL_BY_SPEC_GLOSS)
                specularColor *= gBufferData.SpecularGloss;

            specularColor *= gBufferData.SpecularLevel;
            
            specularColor *= ComputeFresnel(gBufferData.SpecularFresnel,
                dot(halfwayDirection, eyeDirection));
        }

        float cosTheta = dot(gBufferData.Normal, halfwayDirection);
        cosTheta = pow(saturate(cosTheta), gBufferData.SpecularGloss);
        specularColor *= cosTheta;
    }
    else
    {
        specularColor = 0.0;
    }

    return diffuseColor + specularColor + transColor;
}

float TraceShadow(float3 position, float3 direction, float2 random, uint level)
{
    RayDesc ray;

    ray.Origin = position;
    ray.Direction = TangentToWorld(direction, GetShadowSample(random, 0.01));
    ray.TMin = 0.0;
    ray.TMax = INF;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;

    query.TraceRayInline(
        g_BVH,
        RAY_FLAG_NONE,
        INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN,
        ray);

    while (query.Proceed())
    {
        if (query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            GeometryDesc geometryDesc = g_GeometryDescs[query.CandidateInstanceID() + query.CandidateGeometryIndex()];
            MaterialData material = g_Materials[geometryDesc.MaterialId];
    
            if (material.Flags & MATERIAL_FLAG_HAS_DIFFUSE_TEXTURE)
            {
                ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.VertexBufferId)];

                uint3 indices = query.CandidatePrimitiveIndex() * 3 + uint3(0, 1, 2);
                if (geometryDesc.IndexBufferId != 0)
                {
                    Buffer<uint> indexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.IndexBufferId)];
                    indices.x = indexBuffer[geometryDesc.IndexOffset + indices.x];
                    indices.y = indexBuffer[geometryDesc.IndexOffset + indices.y];
                    indices.z = indexBuffer[geometryDesc.IndexOffset + indices.z];
                }
            
                uint3 offsets = geometryDesc.VertexOffset + indices * geometryDesc.VertexStride + geometryDesc.TexCoordOffset0;
            
                float2 texCoord0 = DecodeHalf2(vertexBuffer.Load(offsets.x));
                float2 texCoord1 = DecodeHalf2(vertexBuffer.Load(offsets.y));
                float2 texCoord2 = DecodeHalf2(vertexBuffer.Load(offsets.z));
            
                float3 uv = float3(
                    1.0 - query.CandidateTriangleBarycentrics().x - query.CandidateTriangleBarycentrics().y,
                    query.CandidateTriangleBarycentrics().x,
                    query.CandidateTriangleBarycentrics().y);
            
                float2 texCoord = texCoord0 * uv.x + texCoord1 * uv.y + texCoord2 * uv.z;
                texCoord += material.TexCoordOffsets[0].xy;
            
                if (SampleMaterialTexture2D(material.PackedData[0], texCoord, level).a > 0.5)
                    query.CommitNonOpaqueTriangleHit();
            }
        }
    }
    
    return query.CommittedStatus() == COMMITTED_NOTHING ? 1.0 : 0.0;
}

float TraceShadowCullNonOpaque(float3 position, float3 direction, float2 random)
{
    RayDesc ray;

    ray.Origin = position;
    ray.Direction = TangentToWorld(direction, GetShadowSample(random, 0.01));
    ray.TMin = 0.0;
    ray.TMax = INF;

    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;

    query.TraceRayInline(
        g_BVH,
        RAY_FLAG_NONE,
        INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN,
        ray);
    
    query.Proceed();
    
    return query.CommittedStatus() == COMMITTED_NOTHING ? 1.0 : 0.0;
}

float ComputeLocalLightFalloff(float distance, float inRange, float outRange)
{
    return 1.0 - saturate((distance - inRange) / (outRange - inRange));
}

float3 ComputeLocalLighting(GBufferData gBufferData, float3 eyeDirection, LocalLight localLight)
{
    float3 direction = localLight.Position - gBufferData.Position;
    float distance = length(direction);

    if (distance > 0.0)
        direction /= distance;

    float3 localLighting = ComputeDirectLighting(gBufferData, eyeDirection,
        direction, localLight.Color, localLight.Color);

    localLighting *= ComputeLocalLightFalloff(distance, localLight.InRange, localLight.OutRange);
    return localLighting;
}

float TraceLocalLightShadow(float3 position, float3 direction, float2 random, float radius, float distance, bool enableBackfaceCulling)
{
    RayDesc ray;

    ray.Origin = position;
    ray.Direction = TangentToWorld(direction, GetShadowSample(random, radius));
    ray.TMin = 0.0;
    ray.TMax = distance;

    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;

    query.TraceRayInline(
        g_BVH,
        enableBackfaceCulling ? RAY_FLAG_CULL_BACK_FACING_TRIANGLES : RAY_FLAG_NONE,
        INSTANCE_MASK_OBJECT | INSTANCE_MASK_TERRAIN,
        ray);

    query.Proceed();

    return query.CommittedStatus() == COMMITTED_NOTHING ? 1.0 : 0.0;
}

float ComputeReservoirWeight(GBufferData gBufferData, float3 eyeDirection, LocalLight localLight)
{
    float3 localLighting = ComputeLocalLighting(gBufferData, eyeDirection, localLight);
    return dot(localLighting, float3(0.299, 0.587, 0.114));
}

float3 ComputeGI(GBufferData gBufferData, float3 globalIllumination)
{
    globalIllumination *= gBufferData.Diffuse + gBufferData.Falloff;
    
    if (gBufferData.Flags & GBUFFER_FLAG_MUL_BY_REFRACTION)
        globalIllumination *= gBufferData.Refraction;

    return globalIllumination;
}

float3 ComputeDiffuseAlbedo(GBufferData gBufferData)
{
    return ComputeGI(gBufferData, 1.0);
}

float3 ComputeReflection(GBufferData gBufferData, float3 eyeDirection, float3 reflection)
{
    if (gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION)
    {
        float3 specularFresnel = ComputeFresnel(gBufferData.SpecularFresnel, dot(gBufferData.Normal, eyeDirection));
        reflection *= gBufferData.SpecularTint * gBufferData.SpecularEnvironment * specularFresnel;
    }
    else if (gBufferData.Flags & GBUFFER_FLAG_IS_METALLIC)
    {
        reflection *= gBufferData.Specular;
    }
    else
    {
        float specularIntensity = 1.0 - exp2(-gBufferData.SpecularLevel);
        reflection *= gBufferData.Specular * gBufferData.SpecularTint * specularIntensity;
    }

    return reflection;
}

float3 ComputeSpecularAlbedo(GBufferData gBufferData, float3 eyeDirection)
{
    if (gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION)
    {
        return ComputeReflection(gBufferData, eyeDirection, 1.0);
    }
    else
    {
        Texture2D texture = ResourceDescriptorHeap[g_EnvBrdfTextureId];
    
        float2 envBRDF = texture.SampleLevel(g_SamplerState, float2(
            saturate(dot(gBufferData.Normal, eyeDirection)), (gBufferData.SpecularGloss - 1.0) / 1023.0), 0).xy;
    
        float3 specularFresnel = gBufferData.Flags & GBUFFER_FLAG_IS_METALLIC ? 
            gBufferData.SpecularTint : gBufferData.SpecularFresnel;
        
        float3 specularLighting = specularFresnel * envBRDF.x + envBRDF.y;
        
        return ComputeReflection(gBufferData, eyeDirection, specularLighting);
    }
}

float3 ComputeRefraction(GBufferData gBufferData, float3 refraction)
{
    if (gBufferData.Flags & GBUFFER_FLAG_REFRACTION_MUL)
    {
        return refraction * gBufferData.Diffuse;
    }

    if (gBufferData.Flags & GBUFFER_FLAG_REFRACTION_OPACITY)
    {
        return refraction * (1.0 - gBufferData.Refraction);
    }
    
    if (gBufferData.Flags & GBUFFER_FLAG_REFRACTION_OVERLAY)
    {
        refraction = select(refraction <= gBufferData.RefractionOverlay, 
            2.0 * gBufferData.Diffuse * refraction, 1.0 - 2.0 * (1.0 - gBufferData.Diffuse) * (1.0 - refraction));
        
        return lerp(refraction, gBufferData.Specular, gBufferData.Refraction);
    }

    return refraction + gBufferData.Diffuse * gBufferData.Refraction;
}

float3 ComputeGeometryShading(GBufferData gBufferData, ShadingParams shadingParams, inout uint randSeed)
{
    float3 resultShading = 0.0;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT))
    {
        float3 directLighting = ComputeDirectLighting(gBufferData, shadingParams.EyeDirection, 
            -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb);
        
        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_SHADOW) && any(directLighting > 0.0001))
        {
            directLighting *= TraceShadow(gBufferData.Position,
                -mrgGlobalLight_Direction.xyz, float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)), 0);
        }
        
        resultShading += directLighting;
    }

    if (g_LocalLightCount > 0 && !(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT))
        resultShading += ComputeLocalLighting(gBufferData, shadingParams.EyeDirection, g_LocalLights[shadingParams.Reservoir.Y]) * shadingParams.Reservoir.W;

    resultShading += ComputeGI(gBufferData, shadingParams.GlobalIllumination);
    resultShading += ComputeReflection(gBufferData, shadingParams.EyeDirection, shadingParams.Reflection);
    resultShading += ComputeRefraction(gBufferData, shadingParams.Refraction);
    resultShading += gBufferData.Emission;

    return resultShading;
}

float3 ComputeWaterShading(GBufferData gBufferData, ShadingParams shadingParams, inout uint randSeed)
{
    float3 resultShading = 0.0;

    float3 halfwayDirection = NormalizeSafe(shadingParams.EyeDirection - mrgGlobalLight_Direction.xyz);
    float cosTheta = pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularGloss);
    float3 specularLight = mrgGlobalLight_Specular.rgb * cosTheta * gBufferData.Specular * gBufferData.SpecularGloss * gBufferData.SpecularLevel;
    float shadow = TraceShadow(gBufferData.Position, -mrgGlobalLight_Direction.xyz, float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)), 0);
    resultShading += specularLight * shadow;

    float diffuseFresnel = (1.0 - gBufferData.SpecularFresnel) * 
        pow(abs(dot(gBufferData.Normal, g_EyeDirection.xyz)), gBufferData.SpecularGloss);

    resultShading += shadingParams.GlobalIllumination * diffuseFresnel;

    float luminance = dot(resultShading, float3(0.2126, 0.7152, 0.0722));
    resultShading += shadingParams.Reflection * gBufferData.SpecularFresnel * (1.0 - saturate(luminance));

    float3 diffuseLight = 0.0;
    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT))
    {
        diffuseLight = mrgGlobalLight_Diffuse.rgb * saturate(dot(gBufferData.Normal, -mrgGlobalLight_Direction.xyz)) * shadow;
        diffuseLight += shadingParams.GlobalIllumination;
        diffuseLight *= gBufferData.Diffuse;
        diffuseLight *= gBufferData.Refraction;
    }

    resultShading += diffuseLight;
    resultShading += ComputeRefraction(gBufferData, shadingParams.Refraction);

    return resultShading;
}

float2 ComputeLightScattering(float3 position, float3 viewPosition)
{
    float4 r0, r3, r4;

    r0.x = -viewPosition.z + -g_LightScatteringFarNearScale.y;
    r0.x = saturate(r0.x * g_LightScatteringFarNearScale.x);
    r0.x = r0.x * g_LightScatteringFarNearScale.z;
    r0.y = g_LightScattering_Ray_Mie_Ray2_Mie2.y + g_LightScattering_Ray_Mie_Ray2_Mie2.x;
    r0.z = rcp(r0.y);
    r0.x = r0.x * -r0.y;
    r0.x = exp(r0.x);
    r0.y = -r0.x + 1;
    r3.xyz = -position;
    r4.xyz = NormalizeSafe(r3.xyz);
    r3.x = dot(-mrgGlobalLight_Direction.xyz, r4.xyz);
    r3.y = g_LightScattering_ConstG_FogDensity.z * r3.x + g_LightScattering_ConstG_FogDensity.y;
    r4.x = pow(abs(r3.y), 1.5);
    r3.y = rcp(r4.x);
    r3.y = r3.y * g_LightScattering_ConstG_FogDensity.x;
    r3.y = r3.y * g_LightScattering_Ray_Mie_Ray2_Mie2.w;
    r3.x = r3.x * r3.x + 1;
    r3.x = g_LightScattering_Ray_Mie_Ray2_Mie2.z * r3.x + r3.y;
    r0.z = r0.z * r3.x;
    r0.y = r0.y * r0.z;

    return float2(r0.x, r0.y * g_LightScatteringFarNearScale.w);
}

float3 ComputePrimaryRayDirection(uint2 index, uint2 resolution)
{
    float2 ndc = (index - g_PixelJitter + 0.5) / resolution * float2(2.0, -2.0) + float2(-1.0, 1.0);
    return mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz;
}