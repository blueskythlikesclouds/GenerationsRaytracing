#include "RaytracingUtil.h"

#include "InstanceData.h"
#include "MaterialData.h"
#include "MessageSender.h"
#include "ModelData.h"
#include "ShaderType.h"
#include "MaterialFlags.h"
#include "RaytracingShader.h"

void RaytracingUtil::createSimpleMaterial(uint32_t& materialId, uint32_t materialFlags, uint32_t textureId)
{
    materialId = MaterialData::s_idAllocator.allocate();
    
    auto& message = s_messageSender.makeMessage<MsgCreateMaterial>();
    message.materialId = materialId;
    message.shaderType = SHADER_TYPE_COMMON;
    message.flags = MATERIAL_FLAG_CONST_TEX_COORD | materialFlags;
    memset(message.texCoordOffsets, 0, sizeof(message.texCoordOffsets));
    memset(message.textures, 0, sizeof(message.textures));
    
    message.textureCount = s_shader_COMMON.textureCount;
    message.textures[0].id = textureId;
    message.textures[0].addressModeU = D3DTADDRESS_WRAP;
    message.textures[0].addressModeV = D3DTADDRESS_WRAP;
    
    message.parameterCount = 0;
    for (size_t i = 0; i < s_shader_COMMON.parameterCount; i++)
    {
        const auto parameter = s_shader_COMMON.parameters[i];
        memcpy(&message.parameters[message.parameterCount], &(&parameter->x)[parameter->index], parameter->size * sizeof(float));
        message.parameterCount += parameter->size;
    }
    
    s_messageSender.endMessage();
}

void RaytracingUtil::createSimpleMaterial2(uint32_t& materialId, uint32_t materialFlags, uint32_t textureId)
{
    materialId = MaterialData::s_idAllocator.allocate();

    auto& message = s_messageSender.makeMessage<MsgCreateMaterial>();
    message.materialId = materialId;
    message.shaderType = SHADER_TYPE_COMMON_2;
    message.flags = MATERIAL_FLAG_CONST_TEX_COORD | materialFlags;
    memset(message.texCoordOffsets, 0, sizeof(message.texCoordOffsets));
    memset(message.textures, 0, sizeof(message.textures));

    message.textureCount = s_shader_COMMON_2.textureCount;
    message.textures[0].id = textureId;
    message.textures[0].addressModeU = D3DTADDRESS_WRAP;
    message.textures[0].addressModeV = D3DTADDRESS_WRAP;

    message.parameterCount = 0;
    for (size_t i = 0; i < s_shader_COMMON_2.parameterCount; i++)
    {
        const auto parameter = s_shader_COMMON_2.parameters[i];
        memcpy(&message.parameters[message.parameterCount], &(&parameter->x)[parameter->index], parameter->size * sizeof(float));
        message.parameterCount += parameter->size;
    }

    s_messageSender.endMessage();
}

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
