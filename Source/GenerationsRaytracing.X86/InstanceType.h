#pragma once

#include "GeometryFlags.h"
#include "ShaderTable.h"

inline constexpr struct
{
    uint32_t geometryMask;
    uint32_t instanceMask;
} s_instanceTypes[] =
{
    { GEOMETRY_FLAG_OPAQUE | GEOMETRY_FLAG_PUNCH_THROUGH, INSTANCE_TYPE_OPAQUE },
    { GEOMETRY_FLAG_TRANSPARENT, INSTANCE_TYPE_TRANSPARENT }
};
