#include "GeometryFlags.h"

cbuffer GlobalsVS : register(b0)
{
    row_major float4x4 g_MtxProjection : packoffset(c0);
    row_major float4x4 g_MtxView : packoffset(c4);
    row_major float4x4 g_MtxWorld : packoffset(c8);
    row_major float4x4 g_MtxWorldIT : packoffset(c12);
    row_major float4x4 g_MtxPrevView : packoffset(c16);
    row_major float4x4 g_MtxPrevWorld : packoffset(c20);
    row_major float4x4 g_MtxLightViewProjection : packoffset(c24);
    row_major float3x4 g_MtxPalette[25] : packoffset(c28);
    row_major float3x4 g_MtxPrevPalette[25] : packoffset(c103);
    float4 g_EyePosition : packoffset(c178);
    float4 g_EyeDirection : packoffset(c179);
    float4 g_ViewportSize : packoffset(c180);
    float4 g_CameraNearFarAspect : packoffset(c181);
    float4 mrgAmbientColor : packoffset(c182);
    float4 mrgGlobalLight_Direction : packoffset(c183);
    float4 mrgGlobalLight_Diffuse : packoffset(c184);
    float4 mrgGlobalLight_Specular : packoffset(c185);
    float4 mrgGIAtlasParam : packoffset(c186);
    float4 mrgTexcoordIndex[4] : packoffset(c187);
    float4 mrgTexcoordOffset[2] : packoffset(c191);
    float4 mrgFresnelParam : packoffset(c193);
    float4 mrgMorphWeight : packoffset(c194);
    float4 mrgZOffsetRate : packoffset(c195);
    float4 g_IndexCount : packoffset(c196);
    float4 g_LightScattering_Ray_Mie_Ray2_Mie2 : packoffset(c197);
    float4 g_LightScattering_ConstG_FogDensity : packoffset(c198);
    float4 g_LightScatteringFarNearScale : packoffset(c199);
    float4 g_LightScatteringColor : packoffset(c200);
    float4 g_LightScatteringMode : packoffset(c201);
    row_major float4x4 g_MtxBillboardY : packoffset(c202);
    float4 mrgLocallightIndexArray : packoffset(c206);
    row_major float4x4 g_MtxVerticalLightViewProjection : packoffset(c207);
    float4 g_VerticalLightDirection : packoffset(c211);
    float4 g_TimeParam : packoffset(c212);
    float4 g_aLightField[6] : packoffset(c213);
    float4 g_SkyParam : packoffset(c219);
    float4 g_ViewZAlphaFade : packoffset(c220);
    float4 g_PowerGlossLevel : packoffset(c245);
}

cbuffer GlobalsPS : register(b1)
{
    row_major float4x4 g_MtxInvProjection : packoffset(c4);
    float4 mrgGlobalLight_Direction_View : packoffset(c11);
    float4 g_Diffuse : packoffset(c16);
    float4 g_Ambient : packoffset(c17);
    float4 g_Specular : packoffset(c18);
    float4 g_Emissive : packoffset(c19);
    float4 g_OpacityReflectionRefractionSpectype : packoffset(c21);
    float4 mrgGroundColor : packoffset(c31);
    float4 mrgSkyColor : packoffset(c32);
    float4 mrgPowerGlossLevel : packoffset(c33);
    float4 mrgEmissionPower : packoffset(c34);
    float4 mrgLocalLight0_Position : packoffset(c38);
    float4 mrgLocalLight0_Color : packoffset(c39);
    float4 mrgLocalLight0_Range : packoffset(c40);
    float4 mrgLocalLight0_Attribute : packoffset(c41);
    float4 mrgLocalLight1_Position : packoffset(c42);
    float4 mrgLocalLight1_Color : packoffset(c43);
    float4 mrgLocalLight1_Range : packoffset(c44);
    float4 mrgLocalLight1_Attribute : packoffset(c45);
    float4 mrgLocalLight2_Position : packoffset(c46);
    float4 mrgLocalLight2_Color : packoffset(c47);
    float4 mrgLocalLight2_Range : packoffset(c48);
    float4 mrgLocalLight2_Attribute : packoffset(c49);
    float4 mrgLocalLight3_Position : packoffset(c50);
    float4 mrgLocalLight3_Color : packoffset(c51);
    float4 mrgLocalLight3_Range : packoffset(c52);
    float4 mrgLocalLight3_Attribute : packoffset(c53);
    float4 mrgLocalLight4_Position : packoffset(c54);
    float4 mrgLocalLight4_Color : packoffset(c55);
    float4 mrgLocalLight4_Range : packoffset(c56);
    float4 mrgLocalLight4_Attribute : packoffset(c57);
    float4 mrgEyeLight_Diffuse : packoffset(c58);
    float4 mrgEyeLight_Specular : packoffset(c59);
    float4 mrgEyeLight_Range : packoffset(c60);
    float4 mrgEyeLight_Attribute : packoffset(c61);
    float4 mrgLuminanceRange : packoffset(c63);
    float4 mrgInShadowScale : packoffset(c64);
    float4 g_ShadowMapParams : packoffset(c65);
    float4 mrgColourCompressFactor : packoffset(c66);
    float4 g_BackGroundScale : packoffset(c67);
    float4 g_GIModeParam : packoffset(c69);
    float4 g_GI0Scale : packoffset(c70);
    float4 g_GI1Scale : packoffset(c71);
    float4 g_MotionBlur_AlphaRef_VelocityLimit_VelocityCutoff_BlurMagnitude : packoffset(c85);
    float4 mrgPlayableParam : packoffset(c86);
    float4 mrgDebugDistortionParam : packoffset(c87);
    float4 mrgEdgeEmissionParam : packoffset(c88);
    float4 g_ForceAlphaColor : packoffset(c89);
    row_major float4x4 g_MtxInvView : packoffset(c94);
    float4 mrgVsmEpsilon : packoffset(c148);
    float4 g_DebugValue : packoffset(c149);
}

cbuffer GlobalsRT : register(b2)
{
    float3 g_EnvironmentColor;
    uint g_CurrentFrame;
    float2 g_PixelJitter;
    uint g_BlueNoiseTextureId;
}

struct Vertex
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float3 Binormal;
    float2 TexCoords[4];
    float4 Color;
};

struct GeometryDesc
{
    uint Flags;
    uint IndexBufferId;
    uint VertexBufferId;
    uint VertexStride;
    uint NormalOffset;
    uint TangentOffset;
    uint BinormalOffset;
    uint TexCoordOffsets[4];
    uint ColorOffset;
    uint MaterialId;
};

struct MaterialTexture
{
    uint Id;
    uint SamplerId;
    uint TexCoordIndex;
};

struct Material
{
    MaterialTexture DiffuseTexture;
    MaterialTexture SpecularTexture;
    MaterialTexture PowerTexture;
    MaterialTexture NormalTexture;
    MaterialTexture EmissionTexture;
    MaterialTexture DiffuseBlendTexture;
    MaterialTexture PowerBlendTexture;
    MaterialTexture NormalBlendTexture;
    MaterialTexture EnvironmentTexture;

    float4 DiffuseColor;
    float4 SpecularColor;
    float SpecularPower;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct ShadingParams
{
    float3 Position;
    float4 DiffuseColor;
    float4 SpecularColor;
    float SpecularPower;
    float3 Normal;
};

RaytracingAccelerationStructure g_BVH : register(t0);
StructuredBuffer<GeometryDesc> g_GeometryDescs : register(t1);
StructuredBuffer<Material> g_Materials : register(t2);
RWTexture2D<float4> g_ColorTexture : register(u0);
RWTexture2D<float> g_DepthTexture : register(u1);

Vertex LoadVertex(in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.VertexBufferId)];
    Buffer<uint> indexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(geometryDesc.IndexBufferId)];

    float3 uv = float3(
        1.0 - attributes.barycentrics.x - attributes.barycentrics.y,
        attributes.barycentrics.x,
        attributes.barycentrics.y);

    uint3 offsets;
    offsets.x = indexBuffer[PrimitiveIndex() * 3 + 0];
    offsets.y = indexBuffer[PrimitiveIndex() * 3 + 1];
    offsets.z = indexBuffer[PrimitiveIndex() * 3 + 2];
    offsets *= geometryDesc.VertexStride;

    uint3 normalOffsets = offsets + geometryDesc.NormalOffset;
    uint3 tangentOffsets = offsets + geometryDesc.TangentOffset;
    uint3 binormalOffsets = offsets + geometryDesc.BinormalOffset;
    uint3 colorOffsets = offsets + geometryDesc.ColorOffset;

    Vertex vertex;
    vertex.Position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    vertex.Normal =
        asfloat(vertexBuffer.Load3(normalOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(normalOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(normalOffsets.z)) * uv.z;

    vertex.Tangent =
        asfloat(vertexBuffer.Load3(tangentOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(tangentOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(tangentOffsets.z)) * uv.z;

    vertex.Binormal =
        asfloat(vertexBuffer.Load3(binormalOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(binormalOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(binormalOffsets.z)) * uv.z;

    [unroll] for (uint i = 0; i < 4; i++)
    {
        uint3 texCoordOffsets = offsets + geometryDesc.TexCoordOffsets[i];

        vertex.TexCoords[i] =
            asfloat(vertexBuffer.Load2(texCoordOffsets.x)) * uv.x +
            asfloat(vertexBuffer.Load2(texCoordOffsets.y)) * uv.y +
            asfloat(vertexBuffer.Load2(texCoordOffsets.z)) * uv.z;
    }

    vertex.Color =
        asfloat(vertexBuffer.Load4(colorOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load4(colorOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load4(colorOffsets.z)) * uv.z;

    vertex.Normal = normalize(mul(ObjectToWorld3x4(), float4(vertex.Normal, 0.0)).xyz);
    vertex.Tangent = normalize(mul(ObjectToWorld3x4(), float4(vertex.Tangent, 0.0)).xyz);
    vertex.Binormal = normalize(mul(ObjectToWorld3x4(), float4(vertex.Binormal, 0.0)).xyz);

    return vertex;
}

Material LoadMaterial()
{
    return g_Materials[g_GeometryDescs[InstanceID() + GeometryIndex()].MaterialId];
}

float4 SampleMaterialTexture2D(Vertex vertex, MaterialTexture materialTexture)
{
    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(materialTexture.Id)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(materialTexture.SamplerId)];

    return texture.SampleLevel(samplerState, vertex.TexCoords[materialTexture.TexCoordIndex], 0);
}

ShadingParams LoadShadingParams(Vertex vertex)
{
    Material material = LoadMaterial();
    ShadingParams shadingParams = (ShadingParams) 0;

    shadingParams.Position = vertex.Position;
    shadingParams.DiffuseColor = material.DiffuseColor;
    shadingParams.SpecularColor = material.SpecularColor;
    shadingParams.SpecularPower = material.SpecularPower;
    shadingParams.Normal = vertex.Normal;

    if (material.DiffuseTexture.Id != 0)
    {
        float4 diffuseColor = SampleMaterialTexture2D(vertex, material.DiffuseTexture);

        if (material.DiffuseBlendTexture.Id != 0)
            diffuseColor = lerp(diffuseColor, SampleMaterialTexture2D(vertex, material.DiffuseBlendTexture), vertex.Color.x);
        else
            diffuseColor *= vertex.Color;

        shadingParams.DiffuseColor *= diffuseColor;
    }

    if (material.SpecularTexture.Id != 0)
        shadingParams.SpecularColor *= SampleMaterialTexture2D(vertex, material.SpecularTexture);

    if (material.PowerTexture.Id != 0)
    {
        float specularPower = SampleMaterialTexture2D(vertex, material.PowerTexture).x;

        if (material.PowerBlendTexture.Id != 0)
            specularPower = lerp(specularPower, SampleMaterialTexture2D(vertex, material.PowerBlendTexture).x, vertex.Color.x);

        shadingParams.SpecularColor.rgb *= specularPower;
        shadingParams.SpecularPower *= specularPower;
    }

    shadingParams.SpecularPower = clamp(shadingParams.SpecularPower, 1.0, 1024.0);

    if (material.NormalTexture.Id != 0)
    {
        float4 normal = SampleMaterialTexture2D(vertex, material.NormalTexture);
        normal.x *= normal.w;

        if (material.NormalBlendTexture.Id != 0)
        {
            float4 normalBlend = SampleMaterialTexture2D(vertex, material.NormalBlendTexture);
            normalBlend.x *= normalBlend.w;

            normal = lerp(normal, normalBlend, vertex.Color.x);
        }

        normal.xy = normal.xy * 2.0 - 1.0;
        normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));

        shadingParams.Normal = normalize(
            vertex.Tangent * normal.x +
            vertex.Binormal * normal.y +
            vertex.Normal * normal.z);
    }

    return shadingParams;
}

float3 ComputeDirectLighting(ShadingParams shadingParams, float3 lightDirection, float3 diffuseColor, float3 specularColor)
{
    diffuseColor *= shadingParams.DiffuseColor.rgb;

    float3 halfwayDirection = normalize(lightDirection - WorldRayDirection());
    float specular = pow(saturate(dot(shadingParams.Normal, halfwayDirection)), shadingParams.SpecularPower);
    float fresnel = pow(1.0 - saturate(dot(shadingParams.Normal, -WorldRayDirection())), 5.0) * 0.6 + 0.4;
    specularColor *= shadingParams.SpecularColor.rgb * specular * fresnel;

    return (diffuseColor + specularColor) * saturate(dot(shadingParams.Normal, lightDirection));
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
    r3.xyz = -position + g_EyePosition.xyz;
    r4.xyz = normalize(r3.xyz);
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

float4 GetBlueNoise()
{
    uint2 index = DispatchRaysIndex().xy + uint2(17, 31) * g_CurrentFrame;

    Texture2D texture = ResourceDescriptorHeap[g_BlueNoiseTextureId];
    return texture.Load(int3(index % 1024, 0));
}

float3 GetPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

float3 GetCosHemisphereSample(float3 normal, float2 random)
{
    float3 binormal = GetPerpendicularVector(normal);
    float3 tangent = cross(binormal, normal);

    float radius = sqrt(random.x);
    float angle = 2.0 * 3.14159265 * random.y;

    return tangent * (radius * cos(angle)) + binormal * (radius * sin(angle)) + normal * sqrt(max(0.0, 1.0 - random.x));
}

float3 GetCosHemisphereSample(float3 normal)
{
    return GetCosHemisphereSample(normal, GetBlueNoise().xy);
}

struct Payload
{
    float3 Color;
    float T;
    uint Depth;
};

[shader("raygeneration")]
void RayGeneration()
{
    float2 ndc = (DispatchRaysIndex().xy - g_PixelJitter + 0.5) / DispatchRaysDimensions().xy * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_EyePosition.xyz;
    ray.Direction = normalize(mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], -ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz);
    ray.TMin = 0.0;
    ray.TMax = g_CameraNearFarAspect.y;

    Payload payload = (Payload)0;

    TraceRay(
        g_BVH,
        0,
        1,
        0,
        0,
        0,
        ray,
        payload);

    if (payload.T >= 0.0)
    {
        float3 color = payload.Color;
        float3 position = ray.Origin + ray.Direction * payload.T;

        float2 lightScattering = ComputeLightScattering(position, mul(float4(position, 1.0), g_MtxView).xyz);
        color = color * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;

        if (g_CurrentFrame > 0)
        {
            float3 prevColor = g_ColorTexture[DispatchRaysIndex().xy].rgb;
            g_ColorTexture[DispatchRaysIndex().xy] = float4(lerp(prevColor, color, 1.0 / (g_CurrentFrame + 1.0)), 1.0);
        }
        else
        {
            g_ColorTexture[DispatchRaysIndex().xy] = float4(color, 1.0);
        }

        
        float3 viewPosition = mul(float4(position, 1.0), g_MtxView).xyz;
        float4 projectedPosition = mul(float4(viewPosition, 1.0), g_MtxProjection);

        g_DepthTexture[DispatchRaysIndex().xy] = projectedPosition.z / projectedPosition.w;
    }
    else
    {
        g_ColorTexture[DispatchRaysIndex().xy] = 0.0;
        g_DepthTexture[DispatchRaysIndex().xy] = 1.0;
    }
}

[shader("miss")]
void Miss(inout Payload payload : SV_RayPayload)
{
    payload.Color = g_EnvironmentColor;
    payload.T = -1.0;
}

[shader("closesthit")]
void ClosestHit(inout Payload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    Vertex vertex = LoadVertex(attributes);
    ShadingParams shadingParams = LoadShadingParams(vertex);

    payload.Color = ComputeDirectLighting(shadingParams, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb);

    {
        float3 normal = -mrgGlobalLight_Direction.xyz;
        float3 binormal = GetPerpendicularVector(normal);
        float3 tangent = cross(binormal, normal);

        float2 random = GetBlueNoise().xy;

        float angle = random.x * 6.28318530718;
        float radius = sqrt(random.y) * 0.01;

        float3 direction;
        direction.x = cos(angle) * radius;
        direction.y = sin(angle) * radius;
        direction.z = sqrt(1.0 - saturate(dot(direction.xy, direction.xy)));

        RayDesc ray;

        ray.Origin = vertex.Position + vertex.Normal * 0.001;
        ray.Direction = normalize(direction.x * tangent + direction.y * binormal + direction.z * normal);
        ray.TMin = 0.0;
        ray.TMax = 10000.0;

        Payload payload1 = (Payload)0;

        TraceRay(
            g_BVH,
            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
            1,
            0,
            0,
            0,
            ray,
            payload1);

        payload.Color *= payload1.T >= 0.0 ? 0.0 : 1.0;
    }

    payload.Color += ComputeDirectLighting(shadingParams, -WorldRayDirection(), mrgEyeLight_Diffuse.rgb, mrgEyeLight_Specular.rgb * mrgEyeLight_Specular.w);

    if (payload.Depth < 2)
    {
        RayDesc ray;

        ray.Origin = vertex.Position;
        ray.Direction = GetCosHemisphereSample(shadingParams.Normal);
        ray.TMin = 0.001;
        ray.TMax = 10000.0;

        Payload payload1 = (Payload)0;
        payload1.Depth = payload.Depth + 1;

        TraceRay(
            g_BVH,
            0,
            1,
            0,
            0,
            0,
            ray,
            payload1);

        payload.Color += shadingParams.DiffuseColor.rgb * payload1.Color;
    }
    
    payload.T = RayTCurrent();
}

[shader("anyhit")]
void AnyHit(inout Payload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    Material material = LoadMaterial();

    if (material.DiffuseTexture.Id != 0)
    {
        GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
        Vertex vertex = LoadVertex(attributes);

        Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(material.DiffuseTexture.Id)];
        SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(material.DiffuseTexture.SamplerId)];

        float alpha = texture.SampleLevel(samplerState, vertex.TexCoords[0], 0).a * vertex.Color.a;
        float alphaThreshold = geometryDesc.Flags & GEOMETRY_FLAG_PUNCH_THROUGH ? 0.5 : GetBlueNoise().x;

        if (alpha < alphaThreshold)
            IgnoreHit();
    }
}