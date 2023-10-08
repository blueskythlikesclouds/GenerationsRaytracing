#include "GBufferData.hlsli"
#include "GeometryShading.hlsli"
#include "RayTracing.hlsli"

[shader("raygeneration")]
void PrimaryRayGeneration()
{
    float2 ndc = (DispatchRaysIndex().xy - g_PixelJitter + 0.5) / DispatchRaysDimensions().xy * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_EyePosition.xyz;
    ray.Direction = normalize(mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], -ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz);
    ray.TMin = 0.0;
    ray.TMax = g_CameraNearFarAspect.y;

    PrimaryRayPayload payload;

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
void PrimaryMiss(inout PrimaryRayPayload payload : SV_RayPayload)
{
}

[shader("closesthit")]
void PrimaryClosestHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, attributes);
    StoreGBufferData(DispatchRaysIndex().xy, CreateGBufferData(vertex, material));
}

[shader("anyhit")]
void PrimaryAnyHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, attributes);
    GBufferData gBufferData = CreateGBufferData(vertex, material);
    float alphaThreshold = geometryDesc.Flags & GEOMETRY_FLAG_PUNCH_THROUGH ? 0.5 : GetBlueNoise().x;

    if (gBufferData.Alpha < alphaThreshold)
        IgnoreHit();
}

[shader("raygeneration")]
void ShadowRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    float shadow = 0.0;
    [branch] if (!(gBufferData.Flags & GBUFFER_FLAG_SKIP_GLOBAL_LIGHT))
        shadow = TraceSoftShadow(gBufferData.Position, -mrgGlobalLight_Direction.xyz);

    g_ShadowTexture[DispatchRaysIndex().xy] = shadow;
}

[shader("raygeneration")]
void GlobalIlluminationRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    float3 globalIllumination = 0.0;
    [branch] if (!(gBufferData.Flags & GBUFFER_FLAG_SKIP_GLOBAL_ILLUMINATION))
        globalIllumination = TraceGlobalIllumination(0, gBufferData.Position, gBufferData.Normal);

    g_GlobalIlluminationTexture[DispatchRaysIndex().xy] = globalIllumination;
        
}

[shader("raygeneration")]
void ReflectionRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    float3 reflection = 0.0;
    [branch] if (!(gBufferData.Flags & GBUFFER_FLAG_SKIP_REFLECTION))
    {
        reflection = TraceReflection(0,
            gBufferData.Position,
            gBufferData.Normal,
            gBufferData.SpecularPower,
            normalize(g_EyePosition.xyz - gBufferData.Position));
    }
    g_ReflectionTexture[DispatchRaysIndex().xy] = reflection;
}

[shader("miss")]
void SecondaryMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    payload.Color = g_EnvironmentColor;
    payload.T = -1.0;
}

[shader("closesthit")]
void SecondaryClosestHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, attributes);
    GBufferData gBufferData = CreateGBufferData(vertex, material);

    payload.Color = ComputeDirectLighting(gBufferData, 
        -mrgGlobalLight_Direction.xyz, -WorldRayDirection(), mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb);

    payload.Color *= TraceHardShadow(vertex.Position, -mrgGlobalLight_Direction.xyz);

    payload.Color += ComputeDirectLighting(gBufferData, -WorldRayDirection(), 
        -WorldRayDirection(), mrgEyeLight_Diffuse.rgb, mrgEyeLight_Specular.rgb * mrgEyeLight_Specular.w);

    payload.Color += TraceGlobalIllumination(payload.Depth,
        gBufferData.Position, gBufferData.Normal) * (gBufferData.Diffuse + gBufferData.Falloff);

    payload.Color += gBufferData.Emission;

    payload.T = RayTCurrent();
}

[shader("anyhit")]
void SecondaryAnyHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];

    [branch] if (geometryDesc.Flags & GEOMETRY_FLAG_TRANSPARENT)
    {
        IgnoreHit();
    }
    else
    {
        Material material = g_Materials[geometryDesc.MaterialId];
        Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, attributes);

        if (SampleMaterialTexture2D(material.DiffuseTexture, geometryDesc.TexCoordOffsets[0]).a < 0.5)
            IgnoreHit();
    }
}

