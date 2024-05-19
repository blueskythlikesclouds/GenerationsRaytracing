#include "SharedDefinitions.hlsli"
#include "PackedPrimitives.hlsli"

struct InstanceType
{
    float4 Scale;
    float4 TexTrimCoords;
};

StructuredBuffer<InstanceType> g_InstanceTypes : register(t0);

cbuffer Globals : register(b0)
{
    float g_Misc;
};

cbuffer LodDesc : register(b1)
{
    uint g_VertexCount;
};

struct Instance
{
    float3 Position;
    uint Type : 8;
    uint Sway : 8;
    uint PitchAfterSway : 8;
    uint YawAfterSway : 8;
    int PitchBeforeSway : 16;
    int YawBeforeSway : 16;
    uint Color;
};

StructuredBuffer<Instance> g_Instances : register(t1);

struct Vertex
{
    float3 Position;
    uint Color;
    uint Normal;
    uint Tangent;
    uint Binormal;
    uint TexCoord;
};

RWStructuredBuffer<Vertex> g_Vertices : register(u0);

static const float3 s_positions[] =
{
    float3(-0.5, 1.0, 0.0),
    float3(-0.5, 0.0, 0.0),
    float3(0.5, 1.0, 0.0),
    float3(0.5, 0.0, 0.0)
};

static const float2 s_texCoords[] =
{
    float2(0.0, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 0.0),
    float2(1.0, 1.0)
};

static const uint s_indices[] =
{
    0, 1, 2,
    1, 2, 3
};

float3 GrassInstanceDecompiled(
	float4 v0,
	float4 v1,
	float4 v2,
	uint4 v3,
	float4 v4,
    InstanceType instanceType)
{
    float4 C[12];
    
    C[8] = float4(0.5, 6.28318548, -3.14159274, 0.00392156886);
    C[9] = float4(0.25, 0.100000001, 0.5, -0.5);
    C[10] = float4(1.44269502, 1, 1.5, 0);
    C[11] = float4(0.300000012, 0.699999988, 0, 0);
    
    float4 r0 = float4(0, 0, 0, 0);
    float4 r1 = float4(0, 0, 0, 0);
    float4 r2 = float4(0, 0, 0, 0);
    float4 r3 = float4(0, 0, 0, 0);
    float4 r4 = float4(0, 0, 0, 0);
    float4 r5 = float4(0, 0, 0, 0);
    int4 a0 = int4(0, 0, 0, 0);
    
    r0.yz = C[9].yz;
    r0.x = g_Misc.x * r0.y + r0.z;
    r0.x = frac(r0.x);
    r0.x = r0.x * C[8].y + C[8].z;
    r1.y = float2(cos(r0.x), sin(r0.x)).y;
    r0.x = C[8].x + v3.x;
    r0.y = frac(r0.x);
    r0.x = -r0.y + r0.x;
    r0.x = r0.x + r0.x;
    a0.x = r0.x;
    r0.xyz = v0.xyz * instanceType.Scale.xyz;
    r1.xz = v4.xy * C[8].xx + C[8].xx;
    r1.xz = frac(r1.xz);
    r1.xz = r1.xz * C[8].yy + C[8].zz;
    r2.xy = float2(cos(r1.x), sin(r1.x)).xy;
    r3.xy = float2(cos(r1.z), sin(r1.z)).xy;
    r1.xzw = r0.yyz * r2.xyx;
    r0.y = r0.z * -r2.y + r1.x;
    r0.z = r1.w + r1.z;
    r0.zw = r3.yx * r0.zz;
    r1.xz = v3.zw * C[8].ww + C[8].xx;
    r1.xz = frac(r1.xz);
    r1.xz = r1.xz * C[8].yy + C[8].zz;
    r4.xy = float2(cos(r1.x), sin(r1.x)).xy;
    r5.xy = float2(cos(r1.z), sin(r1.z)).xy;
    r1.xz = r0.yy * r4.xy;
    r0.y = r0.x * -r3.y + r0.w;
    r0.x = r0.x * r3.x + r0.z;
    r0.z = r0.y * r4.x + r1.z;
    r3.y = r0.y * -r4.y + r1.x;
    r0.yz = r5.yx * r0.zz;
    r0.y = r0.x * r5.x + r0.y;
    r3.z = r0.x * -r5.y + r0.z;
    r0.x = r2.x * v0.y;
    r0.z = r2.x * C[11].x + C[11].y;
    r0.z = C[8].w * v3.y;
    r0.x = r0.z * r0.x;
    r0.x = r0.x * C[9].x;
    r3.x = r0.x * r1.y + r0.y;
    return r3.xyz + v2.xyz;
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= g_VertexCount)
        return;
    
    uint instanceId = dispatchThreadId.x / 6;
    Instance instance = g_Instances[instanceId];
    
    InstanceType instanceType = g_InstanceTypes[instance.Type];
    
    float sway = instance.Sway / 255.0;
    
    float pitchAfterSway = (2.0 * PI) * (instance.PitchAfterSway / 255.0);
    float yawAfterSway = (2.0 * PI) * (instance.YawAfterSway / 255.0);
    
    float pitchBeforeSway = PI * (instance.PitchBeforeSway / 32768.0);
    float yawBeforeSway = PI * (instance.YawBeforeSway / 32768.0);
    
    uint vertexId = dispatchThreadId.x % 6;
    Vertex vertex = (Vertex) 0;
    
    float2 texCoord = s_texCoords[s_indices[vertexId]];
    
    vertex.Position = GrassInstanceDecompiled(
        float4(s_positions[s_indices[vertexId]], 1.0),
        float4(texCoord, 0.0, 1.0),
        float4(instance.Position, 1.0),
        uint4(instance.Type, instance.Sway, instance.PitchAfterSway, instance.YawAfterSway),
        float4(instance.PitchBeforeSway / 32768.0, instance.YawBeforeSway / 32768.0, 0.0, 0.0),
        instanceType);
    
    vertex.Color = 0xFFFFFFFF;
    vertex.Normal = EncodeSnorm10(float3(0.0, 1.0, 0.0));
    vertex.TexCoord = EncodeHalf2((instanceType.TexTrimCoords.zw - instanceType.TexTrimCoords.xy) * texCoord + instanceType.TexTrimCoords.xy);
    
    g_Vertices[dispatchThreadId.x] = vertex;

}