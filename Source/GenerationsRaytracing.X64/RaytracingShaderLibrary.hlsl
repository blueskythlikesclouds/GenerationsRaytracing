#ifndef FLT_MAX
#define FLT_MAX                         asfloat(0x7f7fffff)
#endif

#define Z_MAX                           10000.0f

struct Payload
{
    float3 color;
    uint random;
    uint depth;
    float t;
};

struct Attributes
{
    float2 uv;
};

struct Geometry
{
    uint vertexStride;
    uint normalOffset;
    uint tangentOffset;
    uint binormalOffset;
    uint texCoordOffset;
    uint colorOffset;
    uint material;
};

struct Vertex
{
    float3 position;
    float3 normal;
    float3 tangent;
    float3 binormal;
    float2 texCoord;
    float4 color;
};

struct Material
{
    uint textures[16];
};

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
    float4 mrgGIAtlasParam : packoffset(c186);
    float4 mrgTexcoordOffset[2] : packoffset(c191);
    float4 mrgMorphWeight : packoffset(c194);
    float4 mrgZOffsetRate : packoffset(c195);
    float4 g_IndexCount : packoffset(c196);
    float4 g_LightScattering_Ray_Mie_Ray2_Mie2 : packoffset(c197);
    float4 g_LightScattering_ConstG_FogDensity : packoffset(c198);
    float4 g_LightScatteringFarNearScale : packoffset(c199);
    row_major float4x4 g_MtxBillboardY : packoffset(c202);
    float4 mrgLocallightIndexArray : packoffset(c206);
    row_major float4x4 g_MtxVerticalLightViewProjection : packoffset(c207);
    float4 g_VerticalLightDirection : packoffset(c211);
    float4 g_SkyParam : packoffset(c219);
    float4 g_ViewZAlphaFade : packoffset(c220);
};

cbuffer cbGlobalsPS : register(b1)
{
    row_major float4x4 g_MtxInvProjection : packoffset(c4);
    float4 mrgGlobalLight_Direction : packoffset(c10);
    float4 mrgGlobalLight_Direction_View : packoffset(c11);
    float4 g_Diffuse : packoffset(c16);
    float4 g_Ambient : packoffset(c17);
    float4 g_Specular : packoffset(c18);
    float4 g_Emissive : packoffset(c19);
    float4 g_PowerGlossLevel : packoffset(c20);
    float4 g_OpacityReflectionRefractionSpectype : packoffset(c21);
    float4 g_ViewportSize : packoffset(c24);
    float4 g_CameraNearFarAspect : packoffset(c25);
    float4 mrgTexcoordIndex[4] : packoffset(c26);
    float4 mrgAmbientColor : packoffset(c30);
    float4 mrgGroundColor : packoffset(c31);
    float4 mrgSkyColor : packoffset(c32);
    float4 mrgPowerGlossLevel : packoffset(c33);
    float4 mrgEmissionPower : packoffset(c34);
    float4 mrgGlobalLight_Diffuse : packoffset(c36);
    float4 mrgGlobalLight_Specular : packoffset(c37);
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
    float4 mrgFresnelParam : packoffset(c62);
    float4 mrgLuminanceRange : packoffset(c63);
    float4 mrgInShadowScale : packoffset(c64);
    float4 g_ShadowMapParams : packoffset(c65);
    float4 mrgColourCompressFactor : packoffset(c66);
    float4 g_BackGroundScale : packoffset(c67);
    float4 g_TimeParam : packoffset(c68);
    float4 g_GIModeParam : packoffset(c69);
    float4 g_GI0Scale : packoffset(c70);
    float4 g_GI1Scale : packoffset(c71);
    float4 g_LightScatteringColor : packoffset(c75);
    float4 g_LightScatteringMode : packoffset(c76);
    float4 g_aLightField[6] : packoffset(c77);
    float4 g_MotionBlur_AlphaRef_VelocityLimit_VelocityCutoff_BlurMagnitude : packoffset(c85);
    float4 mrgPlayableParam : packoffset(c86);
    float4 mrgDebugDistortionParam : packoffset(c87);
    float4 mrgEdgeEmissionParam : packoffset(c88);
    float4 g_ForceAlphaColor : packoffset(c89);
    row_major float4x4 g_MtxInvView : packoffset(c94);
    float4 mrgVsmEpsilon : packoffset(c148);
    float4 g_DebugValue : packoffset(c149);
};

RaytracingAccelerationStructure g_BVH : register(t0);
StructuredBuffer<Geometry> g_GeometryBuffer : register(t1);
StructuredBuffer<Material> g_MaterialBuffer : register(t2);

RWTexture2D<float4> g_Texture : register(u0);

SamplerState g_LinearRepeatSampler : register(s0);

Buffer<uint> g_BindlessIndexBuffer[] : register(t0, space1);
ByteAddressBuffer g_BindlessVertexBuffer[] : register(t0, space2);
Texture2D<float4> g_BindlessTexture2D[] : register(t0, space3);

// http://intro-to-dxr.cwyman.org/

uint InitializeRandom(uint val0, uint val1, uint backoff = 16)
{
    uint v0 = val0, v1 = val1, s0 = 0;
    [unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

uint NextRandomUint(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return s;
}

float NextRandom(inout uint s)
{
    return float(NextRandomUint(s) & 0x00FFFFFF) / float(0x01000000);
}

float3 GetPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

float3 GetCosHemisphereSample(inout uint randSeed, float3 hitNormal)
{
    float2 randomValue = float2(NextRandom(randSeed), NextRandom(randSeed));

    float3 bitangent = GetPerpendicularVector(hitNormal);
    float3 tangent = cross(bitangent, hitNormal);
    float r = sqrt(randomValue.x);
    float phi = 2.0f * 3.14159265f * randomValue.y;

    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNormal.xyz * sqrt(max(0.0, 1.0f - randomValue.x));
}

float3 GetPosition(in RayDesc ray, in Payload payload)
{
    return ray.Origin + ray.Direction * min(payload.t, Z_MAX);
}

float3 GetPosition()
{
    return WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
}

float3 GetPixelPositionAndDepth(float3 position, float4x4 view, float4x4 projection)
{
    float4 screenCoords = mul(projection, mul(view, float4(position, 1.0)));
    screenCoords /= screenCoords.w;

    screenCoords.xy = (screenCoords.xy * float2(0.5, -0.5) + 0.5) * DispatchRaysDimensions().xy;
    return screenCoords.xyz;
}

float3 GetCurrentPixelPositionAndDepth(float3 position)
{
    return GetPixelPositionAndDepth(position, g_MtxView, g_MtxProjection);
}

float3 GetPreviousPixelPositionAndDepth(float3 position)
{
    return GetPixelPositionAndDepth(position, g_MtxPrevView, g_MtxProjection);
}

Vertex GetVertex(in Attributes attributes)
{
    float3 uv = float3(1.0 - attributes.uv.x - attributes.uv.y, attributes.uv.x, attributes.uv.y);
    uint index = InstanceID() + GeometryIndex();

    Geometry geometry = g_GeometryBuffer[index];
    Buffer<uint> indexBuffer = g_BindlessIndexBuffer[index * 2 + 0];
    ByteAddressBuffer vertexBuffer = g_BindlessVertexBuffer[index * 2 + 1];

    uint3 indices;
    indices.x = indexBuffer[PrimitiveIndex() * 3 + 0];
    indices.y = indexBuffer[PrimitiveIndex() * 3 + 1];
    indices.z = indexBuffer[PrimitiveIndex() * 3 + 2];

    uint3 offsets = indices * geometry.vertexStride;
    uint3 normalOffsets = offsets + geometry.normalOffset;
    uint3 tangentOffsets = offsets + geometry.tangentOffset;
    uint3 binormalOffsets = offsets + geometry.binormalOffset;
    uint3 texCoordOffsets = offsets + geometry.texCoordOffset;
    uint3 colorOffsets = offsets + geometry.colorOffset;

    Vertex vertex;
    vertex.position = GetPosition();

    vertex.normal =
        asfloat(vertexBuffer.Load3(normalOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(normalOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(normalOffsets.z)) * uv.z;

    vertex.tangent =
        asfloat(vertexBuffer.Load3(tangentOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(tangentOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(tangentOffsets.z)) * uv.z;

    vertex.binormal =
        asfloat(vertexBuffer.Load3(binormalOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load3(binormalOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load3(binormalOffsets.z)) * uv.z;

    vertex.texCoord =
        asfloat(vertexBuffer.Load2(texCoordOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load2(texCoordOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load2(texCoordOffsets.z)) * uv.z;

    vertex.color =
        asfloat(vertexBuffer.Load4(colorOffsets.x)) * uv.x +
        asfloat(vertexBuffer.Load4(colorOffsets.y)) * uv.y +
        asfloat(vertexBuffer.Load4(colorOffsets.z)) * uv.z;

    vertex.normal = normalize(mul(ObjectToWorld3x4(), float4(vertex.normal, 0)).xyz);
    vertex.tangent = normalize(mul(ObjectToWorld3x4(), float4(vertex.tangent, 0)).xyz);
    vertex.binormal = normalize(mul(ObjectToWorld3x4(), float4(vertex.binormal, 0)).xyz);

    return vertex;
}

Material GetMaterial()
{
    return g_MaterialBuffer[g_GeometryBuffer[InstanceID() + GeometryIndex()].material];
}

float3 TraceGlobalIllumination(inout Payload payload, float3 normal)
{
    if (payload.depth >= 4)
        return 0;

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = normalize(GetCosHemisphereSample(payload.random, normal));
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload1 = (Payload)0;
    payload1.random = payload.random;
    payload1.depth = payload.depth + 1;

    TraceRay(g_BVH, 0, 1, 0, 0, 0, ray, payload1);

    payload.random = payload1.random;
    return payload1.color;
}

float3 TraceReflection(inout Payload payload, float3 normal)
{
    if (payload.depth >= 2)
        return 0;

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = normalize(reflect(WorldRayDirection(), normal));
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload1 = (Payload)0;
    payload1.random = payload.random;
    payload1.depth = payload.depth + 1;

    TraceRay(g_BVH, 0, 1, 0, 0, 0, ray, payload1);

    payload.random = payload1.random;
    return payload1.color;
}

float3 TraceRefraction(inout Payload payload, float3 normal)
{
    if (payload.depth >= 2)
        return 0;

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = normalize(refract(WorldRayDirection(), normal, 1.0 / 1.333));
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload1 = (Payload)0;
    payload1.random = payload.random;
    payload1.depth = payload.depth + 1;

    TraceRay(g_BVH, 0, 1, 0, 0, 0, ray, payload1);

    payload.random = payload1.random;
    return payload1.color;
}

float TraceShadow(inout uint random)
{
    float3 normal = -mrgGlobalLight_Direction.xyz;
    float3 binormal = GetPerpendicularVector(normal);
    float3 tangent = cross(binormal, normal);

    float x = NextRandom(random);
    float y = NextRandom(random);

    float angle = x * 6.28318530718;
    float radius = sqrt(y) * 0.01;

    float3 direction;
    direction.x = cos(angle) * radius;
    direction.y = sin(angle) * radius;
    direction.z = sqrt(1.0 - saturate(dot(direction.xy, direction.xy)));

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = normalize(direction.x * tangent + direction.y * binormal + direction.z * normal);
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload = (Payload)0;
    payload.depth = 0xFF;
    TraceRay(g_BVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 1, 0, 0, 0, ray, payload);

    return payload.t == FLT_MAX ? 1.0 : 0.0;
}

[shader("raygeneration")]
void RayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dimensions = DispatchRaysDimensions().xy;

    float2 ndc = (index + 0.5) / dimensions * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_EyePosition.xyz;
    ray.Direction = normalize(mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], -ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz);
    ray.TMin = g_CameraNearFarAspect.x;
    ray.TMax = g_CameraNearFarAspect.y;

    Payload payload = (Payload)0;
    payload.random = InitializeRandom(index.x, index.y);

    TraceRay(g_BVH, 0, 1, 0, 0, 0, ray, payload);

    g_Texture[index] = float4(payload.color, 1.0);
}

[shader("closesthit")]
void ClosestHit(inout Payload payload : SV_RayPayload, Attributes attributes : SV_IntersectionAttributes)
{
    Vertex vertex = GetVertex(attributes);
    Material material = GetMaterial();

    float nDotL = saturate(dot(vertex.normal, -mrgGlobalLight_Direction.xyz));
    float nDotV = pow(saturate(1.0 + dot(vertex.normal, WorldRayDirection())), 5.0);

    if (nDotL > 0)
        payload.color += TraceShadow(payload.random) * mrgGlobalLight_Diffuse.rgb;

    payload.color += TraceGlobalIllumination(payload, vertex.normal);
    payload.color *= g_BindlessTexture2D[NonUniformResourceIndex(material.textures[0])].SampleLevel(g_LinearRepeatSampler, vertex.texCoord, 0).rgb;

    //if (nDotV > 0)
    //    payload.color += TraceReflection(payload, vertex.normal) * nDotV;

    payload.t = RayTCurrent();
}

[shader("miss")]
void Miss(inout Payload payload : SV_RayPayload)
{
    payload.color = float3(106.0f / 255.0f, 113.0f / 255.0f, 179.0f / 255.0f);
    payload.t = FLT_MAX;
}