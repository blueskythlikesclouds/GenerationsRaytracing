#pragma once

struct InstanceDesc
{
    float prevTransform[3][4];

    float playableParam;
    uint32_t padding0;
    uint32_t padding1;
    uint32_t padding2;
};