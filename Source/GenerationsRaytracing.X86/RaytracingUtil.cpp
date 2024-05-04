#include "RaytracingUtil.h"

#include "InstanceData.h"
#include "MaterialData.h"
#include "MessageSender.h"
#include "ModelData.h"

void RaytracingUtil::releaseResource(RaytracingResourceType resourceType, uint32_t& resourceId)
{
    if (resourceId != NULL)
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

        resourceId = NULL;
    }
}
