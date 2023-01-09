#ifndef SHADER_DEFINITIONS_H_INCLUDED
#define SHADER_DEFINITIONS_H_INCLUDED

#ifndef FLT_MAX
#define FLT_MAX asfloat(0x7f7fffff)
#endif

#define Z_MAX 10000.0f

#define HAS_GLOBAL_ILLUMINATION    (1 << 0)
#define HAS_REFLECTION             (1 << 1)
#define HAS_REFRACTION             (1 << 2)

#define INSTANCE_MASK_NONE         0
#define INSTANCE_MASK_OPAQ_PUNCH   1
#define INSTANCE_MASK_TRANS_WATER  2
#define INSTANCE_MASK_DEFAULT      (INSTANCE_MASK_OPAQ_PUNCH | INSTANCE_MASK_TRANS_WATER)
#define INSTANCE_MASK_SKY          4

#define CLOSEST_HIT_PRIMARY        0
#define CLOSEST_HIT_SECONDARY      1
#define CLOSEST_HIT_NUM            2

#define MISS                       0
#define MISS_SKY                   1
#define MISS_GLOBAL_ILLUMINATION   2
#define MISS_SHADOW                3
#define MISS_REFLECTION_REFRACTION 4

struct Vertex
{
    float3 Position;
    float3 PrevPosition;
    float3 Normal;
    float3 Tangent;
    float3 Binormal;
    float2 TexCoord;
    float4 Color;
};

struct Material
{
    uint TextureIndices[16];
    float4 Parameters[16];
};

struct Mesh
{
    uint VertexBuffer;
    uint PrevVertexBuffer;

    uint VertexCount;
    uint VertexStride;
    uint NormalOffset;
    uint TangentOffset;
    uint BinormalOffset;
    uint TexCoordOffset;
    uint ColorOffset;
    uint ColorFormat;
    uint BlendWeightOffset;
    uint BlendIndicesOffset;

    uint IndexBuffer;

    uint Material;
    uint PunchThrough;
};

struct Instance
{
    row_major float3x4 PrevTransform;
};

struct ShaderParams
{
    Vertex Vertex;
    Material Material;

    float3 GlobalIllumination;
    float Shadow;
    float3 Reflection;
    float3 Refraction;

    float4x4 Projection;
    float4x4 InvProjection;
    float4x4 View;

    float3 EyePosition;
    float3 EyeDirection;
};

struct CallableParams
{
    uint MaterialIndex;
};

#ifndef SHADER_DEFINITIONS_NO_BINDING_LAYOUT

RaytracingAccelerationStructure g_BVH : register(t0);

StructuredBuffer<Material> g_MaterialBuffer : register(t1);
StructuredBuffer<Mesh> g_MeshBuffer : register(t2);
StructuredBuffer<Instance> g_InstanceBuffer : register(t3);

RWTexture2D<float4> g_Position : register(u0);
RWTexture2D<float> g_Depth : register(u1);
RWTexture2D<float2> g_MotionVector : register(u2);
RWTexture2D<float4> g_Normal : register(u3);
RWTexture2D<float2> g_TexCoord : register(u4);
RWTexture2D<float4> g_Color : register(u5);
RWTexture2D<uint4> g_Shader : register(u6);

RWTexture2D<float4> g_GlobalIllumination : register(u7);
RWTexture2D<float> g_Shadow : register(u8);
RWTexture2D<float4> g_Reflection : register(u9);
RWTexture2D<float4> g_Refraction : register(u10);

RWTexture2D<float4> g_Composite : register(u11);

Texture2D<float4> g_BlueNoise : register(t4);
Texture2D<float> g_PrevDepth : register(t5);
Texture2D<float4> g_PrevNormal : register(t6);
Texture2D<float4> g_PrevGlobalIllumination : register(t7);

SamplerState g_LinearRepeatSampler : register(s0);

Buffer<uint> g_BindlessIndexBuffer[] : register(t0, space1);
ByteAddressBuffer g_BindlessVertexBuffer[] : register(t0, space2);

Texture2D<float4> g_BindlessTexture2D[] : register(t0, space3);
TextureCube<float4> g_BindlessTextureCube[] : register(t0, space4);

float4 DecodeColor(uint color)
{
    return float4(
        ((color >> 0) & 0xFF) / 255.0,
        ((color >> 8) & 0xFF) / 255.0,
        ((color >> 16) & 0xFF) / 255.0,
        ((color >> 24) & 0xFF) / 255.0);
}

Vertex GetVertex(in BuiltInTriangleIntersectionAttributes attributes)
{
    float3 uv = float3(1.0 - attributes.barycentrics.x - attributes.barycentrics.y, attributes.barycentrics.x, attributes.barycentrics.y);
    uint index = InstanceID() + GeometryIndex();

    Mesh mesh = g_MeshBuffer[index];

    Buffer<uint> indexBuffer = g_BindlessIndexBuffer[NonUniformResourceIndex(mesh.IndexBuffer)];
    ByteAddressBuffer vertexBuffer = g_BindlessVertexBuffer[NonUniformResourceIndex(mesh.VertexBuffer)];
    ByteAddressBuffer prevVertexBuffer = g_BindlessVertexBuffer[NonUniformResourceIndex(mesh.PrevVertexBuffer)];

    uint3 indices;
    indices.x = indexBuffer[PrimitiveIndex() * 3 + 0];
    indices.y = indexBuffer[PrimitiveIndex() * 3 + 1];
    indices.z = indexBuffer[PrimitiveIndex() * 3 + 2];

    uint3 offsets = indices * mesh.VertexStride;
    uint3 normalOffsets = offsets + mesh.NormalOffset;
    uint3 tangentOffsets = offsets + mesh.TangentOffset;
    uint3 binormalOffsets = offsets + mesh.BinormalOffset;
    uint3 texCoordOffsets = offsets + mesh.TexCoordOffset;
    uint3 colorOffsets = offsets + mesh.ColorOffset;

    Vertex vertex;

    vertex.Position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    vertex.PrevPosition = 
        asfloat(prevVertexBuffer.Load3(offsets.x)) * uv.x +
        asfloat(prevVertexBuffer.Load3(offsets.y)) * uv.y +
        asfloat(prevVertexBuffer.Load3(offsets.z)) * uv.z;

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

    vertex.TexCoord =
        asfloat(vertexBuffer.Load2(texCoordOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load2(texCoordOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load2(texCoordOffsets.z)) * uv.z;

    if (mesh.ColorFormat != 0)
    {
        vertex.Color =
            DecodeColor(vertexBuffer.Load(colorOffsets.x)) * uv.x +
            DecodeColor(vertexBuffer.Load(colorOffsets.y)) * uv.y +
            DecodeColor(vertexBuffer.Load(colorOffsets.z)) * uv.z;
    }
    else
    {
        vertex.Color =
            asfloat(vertexBuffer.Load4(colorOffsets.x)) * uv.x +
            asfloat(vertexBuffer.Load4(colorOffsets.y)) * uv.y +
            asfloat(vertexBuffer.Load4(colorOffsets.z)) * uv.z;
    }

    vertex.PrevPosition = mul(g_InstanceBuffer[InstanceIndex()].PrevTransform, float4(vertex.PrevPosition, 1.0)).xyz;
    vertex.Normal = mul(ObjectToWorld3x4(), float4(vertex.Normal, 0.0)).xyz;
    vertex.Tangent = mul(ObjectToWorld3x4(), float4(vertex.Tangent, 0.0)).xyz;
    vertex.Binormal = mul(ObjectToWorld3x4(), float4(vertex.Binormal, 0.0)).xyz;

    return vertex;
}

Material GetMaterial()
{
    return g_MaterialBuffer[g_MeshBuffer[InstanceID() + GeometryIndex()].Material];
}

Mesh GetMesh()
{
    return g_MeshBuffer[InstanceID() + GeometryIndex()];
}

#endif

#endif