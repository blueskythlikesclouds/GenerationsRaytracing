#pragma once

struct InstanceDesc
{
    float prevTransform[3][4];
    float headTransform[3][4];
    float playableParam;
    float chrPlayableMenuParam;
    float forceAlphaColor;
    float edgeEmissionParam;
};