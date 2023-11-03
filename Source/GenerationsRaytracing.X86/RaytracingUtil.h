#pragma once

#include "Message.h"

struct RaytracingUtil
{
    static void releaseResource(RaytracingResourceType resourceType, uint32_t& resourceId);
};
