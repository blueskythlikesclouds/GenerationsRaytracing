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
    GBufferData gBufferData = (GBufferData) 0;
    gBufferData.Flags = GBUFFER_FLAG_IS_SKY;
    StoreGBufferData(DispatchRaysIndex().xy, gBufferData);

    g_MotionVectorsTexture[DispatchRaysIndex().xy] = 0.0;
}

[shader("closesthit")]
void PrimaryClosestHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes);
    StoreGBufferData(DispatchRaysIndex().xy, CreateGBufferData(vertex, material));

    g_MotionVectorsTexture[DispatchRaysIndex().xy] =
        ComputePixelPosition(vertex.PrevPosition, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(vertex.Position, g_MtxView, g_MtxProjection);
}

[shader("anyhit")]
void PrimaryAnyHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes);
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
    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT)))
        shadow = TraceSoftShadow(gBufferData.Position, -mrgGlobalLight_Direction.xyz);

    g_ShadowTexture[DispatchRaysIndex().xy] = shadow;
}

[shader("raygeneration")]
void GlobalIlluminationRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    float3 globalIllumination = 0.0;
    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION)))
    {
        globalIllumination = TraceGlobalIllumination(0, gBufferData.Position, gBufferData.Normal);

        float luminance = dot(globalIllumination, float3(0.299, 0.587, 0.114));
        globalIllumination = saturate((globalIllumination - luminance) * g_GI1Scale.w + luminance);
        globalIllumination *= g_GI0Scale.rgb;
    }

    g_GlobalIlluminationTexture[DispatchRaysIndex().xy] = globalIllumination;
}

[shader("raygeneration")]
void ReflectionRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    float3 reflection = 0.0;
    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION)))
    {
        float3 eyeDirection = normalize(g_EyePosition.xyz - gBufferData.Position);

        if (gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION)
        {
            reflection = TraceReflection(0, 
                gBufferData.Position, 
                gBufferData.Normal,
                eyeDirection);
        }
        else
        {
            reflection = TraceReflection(0,
                gBufferData.Position,
                gBufferData.Normal,
                gBufferData.SpecularPower,
                eyeDirection);
        }
    }
    g_ReflectionTexture[DispatchRaysIndex().xy] = reflection;
}

[shader("raygeneration")]
void RefractionRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    float3 refraction = 0.0;
    if (gBufferData.Flags & GBUFFER_FLAG_HAS_REFRACTION)
    {
        refraction = TraceRefraction(0, 
            gBufferData.Position, 
            gBufferData.Normal, 
            normalize(g_EyePosition.xyz - gBufferData.Position));
    }
    g_RefractionTexture[DispatchRaysIndex().xy] = refraction;
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
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes);
    GBufferData gBufferData = CreateGBufferData(vertex, material);

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT))
    {
        payload.Color = ComputeDirectLighting(gBufferData, -WorldRayDirection(), 
            -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb);

        payload.Color *= TraceSoftShadow(vertex.Position, -mrgGlobalLight_Direction.xyz);
    }
    else
    {
        payload.Color = 0.0;
    }

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
    {
        payload.Color += TraceGlobalIllumination(payload.Depth,
            gBufferData.Position, gBufferData.Normal) * (gBufferData.Diffuse + gBufferData.Falloff);
    }

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
        InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
        Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes);

        if (SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords[0]).a < 0.5)
            IgnoreHit();
    }
}

