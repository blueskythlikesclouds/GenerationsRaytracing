#pragma once

#define LOCAL_LIGHT_FLAG_NONE                    (0 << 0)
#define LOCAL_LIGHT_FLAG_CAST_SHADOW             (1 << 0)
#define LOCAL_LIGHT_FLAG_ENABLE_BACKFACE_CULLING (1 << 1)

#ifdef __cplusplus

struct LocalLight
{
    float position[3];
    float inRange;
    float color[3];
    float outRange;

    uint32_t flags;
    float shadowRange;

    uint32_t padding0;
    uint32_t padding1;
};

#endif