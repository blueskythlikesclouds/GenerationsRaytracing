#include "RayTracing.hlsli"

[shader("raygeneration")]
void RayGeneration()
{
    float2 ndc = (DispatchRaysIndex().xy - g_PixelJitter + 0.5) / DispatchRaysDimensions().xy * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_EyePosition.xyz;
    ray.Direction = normalize(mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], -ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz);
    ray.TMin = 0.0;
    ray.TMax = g_CameraNearFarAspect.y;

    Payload payload = (Payload) 0;

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
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Vertex vertex = LoadVertex(geometryDesc, attributes);
    GBufferData gBufferData = MakeGBufferData(vertex, g_Materials[geometryDesc.MaterialId]);

    float3 globalLight = ComputeDirectLighting(gBufferData, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb);
    float shadow = TraceShadow(vertex.Position);
    float3 eyeLight = ComputeDirectLighting(gBufferData, -WorldRayDirection(), mrgEyeLight_Diffuse.rgb, mrgEyeLight_Specular.rgb * mrgEyeLight_Specular.w);
    float3 globalIllumination = TraceGlobalIllumination(payload.Depth, gBufferData);
    float3 environmentColor = TraceEnvironmentColor(payload.Depth, gBufferData);

    payload.Color =
        (globalLight * shadow) +
        eyeLight +
        globalIllumination +
        environmentColor +
        gBufferData.Emission;

    payload.T = RayTCurrent();
}

[shader("anyhit")]
void AnyHit(inout Payload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Vertex vertex = LoadVertex(geometryDesc, attributes);
    GBufferData gBufferData = MakeGBufferData(vertex, g_Materials[geometryDesc.MaterialId]);
    float alphaThreshold = geometryDesc.Flags & GEOMETRY_FLAG_PUNCH_THROUGH ? 0.5 : GetBlueNoise().x;

    if (gBufferData.Alpha < alphaThreshold)
        IgnoreHit();
}