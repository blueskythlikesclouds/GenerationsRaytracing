#pragma once

struct Instance
{
    float transform[3][4]{};

    struct GPU
    {
        float prevTransform[3][4]{};
    } gpu{};

    unsigned int model = 0;
    unsigned int element = 0;
};
