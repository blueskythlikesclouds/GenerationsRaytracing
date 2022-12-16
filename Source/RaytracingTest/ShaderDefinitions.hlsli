#ifndef SHADER_DEFINITIONS_H
#define SHADER_DEFINITIONS_H

#define MESH_TYPE_OPAQUE                (1 << 0)
#define MESH_TYPE_TRANS                 (1 << 1)
#define MESH_TYPE_PUNCH                 (1 << 2)
#define MESH_TYPE_SPECIAL               (1 << 3)

#define INSTANCE_MASK_OPAQUE_OR_PUNCH   (1 << 0)
#define INSTANCE_MASK_TRANS_OR_SPECIAL  (1 << 1)
#define INSTANCE_MASK_SKY               (1 << 2)

#ifndef FLT_MAX
#define FLT_MAX                         asfloat(0x7f7fffff)
#endif

#define Z_MAX                           10000.0f

#ifndef __cplusplus

struct cbGlobals
{
    float3 position;
    float tanFovy;
    float4x4 rotation;

    float4x4 view;
    float4x4 projection;

    float4x4 previousView;
    float4x4 previousProjection;

    float aspectRatio;
    uint currentFrame;
    float skyIntensityScale;
    float deltaTime;

    float3 lightDirection;
    float3 lightColor;

    float4 lightScatteringColor;
    float4 rayMieRay2Mie2;
    float4 gAndFogDensity;
    float4 farNearScale;

    float4 eyeLightDiffuse;
    float4 eyeLightSpecular;
    float4 eyeLightRange;
    float4 eyeLightAttribute;

    float middleGray;
    float lumMin;
    float lumMax;
    uint lightCount;

    float2 pixelJitter;
    float currentAccum;
};

ConstantBuffer<cbGlobals> g_Globals : register(b0, space0);

#endif

#endif
