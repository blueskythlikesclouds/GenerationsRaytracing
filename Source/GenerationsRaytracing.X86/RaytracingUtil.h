#pragma once

#include "Message.h"

struct RaytracingUtil
{
    static void createSimpleMaterial(uint32_t& materialId, uint32_t materialFlags, uint32_t textureId);
    static void createSimpleMaterial2(uint32_t& materialId, uint32_t materialFlags, uint32_t textureId);

    static void releaseResource(RaytracingResourceType resourceType, uint32_t& resourceId);
    static void releaseResources();
};
