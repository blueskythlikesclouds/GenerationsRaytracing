#ifndef SHADER_DEFINITIONS_H_INCLUDED
#define SHADER_DEFINITIONS_H_INCLUDED

#ifndef FLT_MAX
#define FLT_MAX asfloat(0x7f7fffff)
#endif

#define Z_MAX 10000.0f

#define HAS_GLOBAL_ILLUMINATION (1 << 0)
#define HAS_REFLECTION          (1 << 1)
#define HAS_REFRACTION          (1 << 2)

struct Payload
{
    float3 Color;
    float T;

    uint Depth;
    uint Random;
};

struct Attributes
{
    float2 BarycentricCoords;
};

struct Vertex
{
    float3 Position;
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

struct Geometry
{
    uint VertexStride;
    uint NormalOffset;
    uint TangentOffset;
    uint BinormalOffset;
    uint TexCoordOffset;
    uint ColorOffset;
    uint ColorFormat;
    uint MaterialIndex;
    uint PunchThrough;
};

struct Instance
{
    float4x4 Delta;
};

RaytracingAccelerationStructure g_BVH : register(t0);

StructuredBuffer<Material> g_MaterialBuffer : register(t1);
StructuredBuffer<Geometry> g_GeometryBuffer : register(t2);
StructuredBuffer<Instance> g_InstanceBuffer : register(t3);

RWTexture2D<float4> g_Color : register(u0);
RWTexture2D<float> g_Depth : register(u1);
RWTexture2D<float2> g_MotionVector : register(u2);

RWTexture2D<float3> g_Normal : register(u3);
RWTexture2D<float4> g_GlobalIllumination : register(u4);

Texture2D<float> g_PrevDepth : register(t4);
Texture2D<float3> g_PrevNormal : register(t5);
Texture2D<float4> g_PrevGlobalIllumination : register(t6);

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

Vertex GetVertex(in Attributes attributes)
{
    float3 uv = float3(1.0 - attributes.BarycentricCoords.x - attributes.BarycentricCoords.y, attributes.BarycentricCoords.x, attributes.BarycentricCoords.y);
    uint index = InstanceID() + GeometryIndex();

    Geometry geometry = g_GeometryBuffer[index];
    Buffer<uint> indexBuffer = g_BindlessIndexBuffer[index * 2 + 0];
    ByteAddressBuffer vertexBuffer = g_BindlessVertexBuffer[index * 2 + 1];

    uint3 indices;
    indices.x = indexBuffer[PrimitiveIndex() * 3 + 0];
    indices.y = indexBuffer[PrimitiveIndex() * 3 + 1];
    indices.z = indexBuffer[PrimitiveIndex() * 3 + 2];

    uint3 offsets = indices * geometry.VertexStride;
    uint3 normalOffsets = offsets + geometry.NormalOffset;
    uint3 tangentOffsets = offsets + geometry.TangentOffset;
    uint3 binormalOffsets = offsets + geometry.BinormalOffset;
    uint3 texCoordOffsets = offsets + geometry.TexCoordOffset;
    uint3 colorOffsets = offsets + geometry.ColorOffset;

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

    vertex.TexCoord =
        asfloat(vertexBuffer.Load2(texCoordOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load2(texCoordOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load2(texCoordOffsets.z)) * uv.z;

    if (geometry.ColorFormat != 0)
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

    vertex.Normal = mul(ObjectToWorld3x4(), float4(vertex.Normal, 0)).xyz;
    vertex.Tangent = mul(ObjectToWorld3x4(), float4(vertex.Tangent, 0)).xyz;
    vertex.Binormal = mul(ObjectToWorld3x4(), float4(vertex.Binormal, 0)).xyz;

    return vertex;
}

Material GetMaterial()
{
    return g_MaterialBuffer[g_GeometryBuffer[InstanceID() + GeometryIndex()].MaterialIndex];
}

Geometry GetGeometry()
{
    return g_GeometryBuffer[InstanceID() + GeometryIndex()];
}

#endif