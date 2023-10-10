#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"

[numthreads(32, 32, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float4 color = 0.0;
    float depth = 1.0;

    GBufferData gBufferData = LoadGBufferData(dispatchThreadId.xy);

    if (!(gBufferData.Flags & GBUFFER_FLAG_IS_SKY))
    {
        float3 eyeDirection = normalize(g_EyePosition.xyz - gBufferData.Position);

        color.rgb = ComputeDirectLighting(gBufferData, eyeDirection,
            -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb);

        color.rgb *= g_ShadowTexture[dispatchThreadId.xy];

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_EYE_LIGHT))
            color.rgb += ComputeEyeLighting(gBufferData, g_EyePosition.xyz, eyeDirection);

        float3 globalIllumination = g_GlobalIlluminationTexture[dispatchThreadId.xy];
        float luminance = dot(globalIllumination, float3(0.299, 0.587, 0.114));
        globalIllumination = saturate((globalIllumination - luminance) * g_GI1Scale.w + luminance);
        globalIllumination *= g_GI0Scale.rgb;

        color.rgb += globalIllumination * (gBufferData.Diffuse + gBufferData.Falloff);

        float3 reflection = g_ReflectionTexture[dispatchThreadId.xy];
        if (!(gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION))
            reflection *= min(1.0, gBufferData.Specular * gBufferData.SpecularLevel);

        color.rgb += reflection * gBufferData.SpecularFresnel;

        color.rgb += gBufferData.Emission;

        float3 viewPosition = mul(float4(gBufferData.Position, 1.0), g_MtxView).xyz;
        float2 lightScattering = ComputeLightScattering(gBufferData.Position, viewPosition);
        color.rgb = color.rgb * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
        color.a = 1.0;

        float4 projectedPosition = mul(float4(viewPosition, 1.0), g_MtxProjection);
        depth = projectedPosition.z / projectedPosition.w;
    }

    g_ColorTexture[dispatchThreadId.xy] = color;
    g_DepthTexture[dispatchThreadId.xy] = depth;
}