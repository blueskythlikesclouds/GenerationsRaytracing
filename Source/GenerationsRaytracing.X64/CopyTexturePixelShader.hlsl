#include "GBufferData.hlsli"
#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"

void main(in float4 position : SV_Position, out float4 color : SV_Target0, out float depth : SV_Depth)
{
    GBufferData gBufferData = LoadGBufferData(position.xy);

    if (!(gBufferData.Flags & GBUFFER_FLAG_MISS))
    {
        float3 eyeDirection = normalize(g_EyePosition.xyz - gBufferData.Position);

        color.rgb = ComputeDirectLighting(gBufferData, eyeDirection,
            -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb);

        color.rgb *= g_ShadowTexture[position.xy];

        if (!(gBufferData.Flags & GBUFFER_FLAG_SKIP_EYE_LIGHT))
        {
            color.rgb += ComputeDirectLighting(gBufferData, eyeDirection,
               eyeDirection, mrgEyeLight_Diffuse.rgb, mrgEyeLight_Specular.rgb * mrgEyeLight_Specular.w);
        }

        color.rgb += g_GlobalIlluminationTexture[position.xy] * (gBufferData.Diffuse + gBufferData.Falloff);

        color.rgb += g_ReflectionTexture[position.xy] * gBufferData.Specular * gBufferData.SpecularLevel;

        color.rgb += gBufferData.Emission;

        float3 viewPosition = mul(float4(gBufferData.Position, 1.0), g_MtxView).xyz;
        float2 lightScattering = ComputeLightScattering(gBufferData.Position, viewPosition);
        color.rgb = min(color.rgb, 4.0) * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
        color.a = 1.0;

        float4 projectedPosition = mul(float4(viewPosition, 1.0), g_MtxProjection);
        depth = projectedPosition.z / projectedPosition.w;
    }
    else
    {
        color = 0.0;
        depth = 1.0;
    }
}