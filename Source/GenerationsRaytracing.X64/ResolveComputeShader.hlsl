#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"

[numthreads(32, 32, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float4 color = 0.0;
    float depth = 1.0;

    GBufferData gBufferData = LoadGBufferData(dispatchThreadId.xy);

    if (!(gBufferData.Flags & GBUFFER_FLAG_MISS))
    {
        float3 eyeDirection = normalize(g_EyePosition.xyz - gBufferData.Position);

        color.rgb = ComputeDirectLighting(gBufferData, eyeDirection,
            -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb);

        color.rgb *= g_ShadowTexture[dispatchThreadId.xy];

        if (!(gBufferData.Flags & GBUFFER_FLAG_SKIP_EYE_LIGHT))
        {
            color.rgb += ComputeDirectLighting(gBufferData, eyeDirection,
               eyeDirection, mrgEyeLight_Diffuse.rgb, mrgEyeLight_Specular.rgb * mrgEyeLight_Specular.w);
        }

        color.rgb += g_GlobalIlluminationTexture[dispatchThreadId.xy] * (gBufferData.Diffuse + gBufferData.Falloff);

        color.rgb += g_ReflectionTexture[dispatchThreadId.xy] * gBufferData.Specular * gBufferData.SpecularLevel * 0.3;

        color.rgb += gBufferData.Emission;

        float3 viewPosition = mul(float4(gBufferData.Position, 1.0), g_MtxView).xyz;
        float2 lightScattering = ComputeLightScattering(gBufferData.Position, viewPosition);
        color.rgb = min(color.rgb, 4.0) * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
        color.a = 1.0;

        float4 projectedPosition = mul(float4(viewPosition, 1.0), g_MtxProjection);
        depth = projectedPosition.z / projectedPosition.w;
    }

    g_ColorTexture[dispatchThreadId.xy] = color;
    g_DepthTexture[dispatchThreadId.xy] = depth;
}