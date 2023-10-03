cbuffer cbGlobalsVS : register(b0)
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

cbuffer cbGlobalsPS : register(b1)
{
}

RaytracingAccelerationStructure g_BVH : register(t0);

struct GeometryDesc
{
    uint IndexBufferId;
    uint VertexBufferId;
    uint VertexStride;
    uint PositionOffset;
    uint NormalOffset;
    uint TangentOffset;
    uint BinormalOffset;
    uint TexCoordOffsets[4];
    uint ColorOffset;
    uint MaterialId;
};

StructuredBuffer<GeometryDesc> g_GeometryDescs : register(t1);

struct MaterialTexture
{
    uint Id;
    uint SamplerId;
};

struct Material
{
    MaterialTexture DiffuseTexture;
};

StructuredBuffer<Material> g_Materials : register(t2);

RWTexture2D<float4> g_ColorTexture : register(u0);

struct Payload
{
    uint dummy;
};

[shader("raygeneration")]
void RayGeneration()
{
    float2 ndc = (DispatchRaysIndex().xy + 0.5) / DispatchRaysDimensions().xy * 2.0 - 1.0;

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
}

[shader("miss")]
void Miss(inout Payload payload : SV_RayPayload)
{
    g_ColorTexture[DispatchRaysIndex().xy] = 0.0;
}

[shader("closesthit")]
void ClosestHit(inout Payload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[NonUniformResourceIndex(geometryDesc.MaterialId)];
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

    uint3 normalOffsets = geometryDesc.NormalOffset + offsets;
    uint3 texCoordOffsets = geometryDesc.TexCoordOffsets[0] + offsets;

    float3 normal =
        asfloat(vertexBuffer.Load3(normalOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(normalOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(normalOffsets.z)) * uv.z;

    float2 texCoord =
        asfloat(vertexBuffer.Load2(texCoordOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load2(texCoordOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load2(texCoordOffsets.z)) * uv.z;

    Texture2D diffuseTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.DiffuseTexture.Id)];
    SamplerState diffuseSampler = SamplerDescriptorHeap[NonUniformResourceIndex(material.DiffuseTexture.SamplerId)];

    g_ColorTexture[DispatchRaysIndex().xy] = diffuseTexture.SampleLevel(diffuseSampler, texCoord, 0);
}