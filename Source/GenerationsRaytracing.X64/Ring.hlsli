#pragma once

#include "GBufferData.hlsli"

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

void CreateRingGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_MUL_BY_SPEC_GLOSS;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a;
    
    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
    gBufferData.SpecularTint *= specular.rgb;
    gBufferData.SpecularEnvironment *= specular.a;
    gBufferData.SpecularFresnel = 0.3;
    
    float4 reflection = SampleMaterialTexture2D(material.ReflectionTexture,
        ComputeEnvMapTexCoord(-WorldRayDirection(), gBufferData.Normal), 0);
    
    gBufferData.Emission = reflection.rgb * material.LuminanceRange.x * reflection.a + reflection.rgb;
    
    float3 specularFresnel = ComputeFresnel(gBufferData.SpecularFresnel, dot(gBufferData.Normal, -WorldRayDirection()));
    gBufferData.Emission *= specular.rgb * specularFresnel;
}
