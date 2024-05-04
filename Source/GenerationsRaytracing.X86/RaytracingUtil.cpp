#include "RaytracingUtil.h"

#include "InstanceData.h"
#include "MaterialData.h"
#include "MessageSender.h"
#include "ModelData.h"

static std::vector<std::pair<RaytracingResourceType, uint32_t>> s_releasedResources;
static Mutex s_mutex;

void RaytracingUtil::releaseResource(RaytracingResourceType resourceType, uint32_t& resourceId)
{
    if (resourceId != NULL)
    {
        LockGuard lock(s_mutex);
        s_releasedResources.emplace_back(resourceType, resourceId);
        resourceId = NULL;
    }
}

void RaytracingUtil::releaseResources()
{
    LockGuard lock(s_mutex);

    for (auto& [resourceType, resourceId] : s_releasedResources)
    {
        auto& message = s_messageSender.makeMessage<MsgReleaseRaytracingResource>();
        message.resourceType = resourceType;
        message.resourceId = resourceId;
        s_messageSender.endMessage();

        switch (resourceType)
        {
        case RaytracingResourceType::BottomLevelAccelStruct:
            ModelData::s_idAllocator.free(resourceId);
            break;

        case RaytracingResourceType::Material:
            MaterialData::s_idAllocator.free(resourceId);
            break;
        }
    }

    s_releasedResources.clear();
}
