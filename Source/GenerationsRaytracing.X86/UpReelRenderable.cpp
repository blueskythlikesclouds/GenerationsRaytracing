#include "UpReelRenderable.h"

#include "IndexBuffer.h"
#include "InstanceData.h"
#include "MaterialData.h"
#include "MaterialFlags.h"
#include "MeshOpt.h"
#include "Message.h"
#include "MessageSender.h"
#include "ModelData.h"
#include "RaytracingRendering.h"
#include "RaytracingUtil.h"
#include "ShaderType.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "RaytracingShader.h"
#include "OptimizedVertexData.h"
#include "Configuration.h"

class ReelRendererEx : public Sonic::CObjUpReel::CReelRenderer
{
public:
    uint32_t m_materialId;
    float m_prevTransform[3][4];
    uint32_t m_bottomLevelAccelStructId;
    XXH32_hash_t m_vertexHash;
    ComPtr<VertexBuffer> m_vertexBuffer;
    uint32_t m_frame;
};

static uint16_t s_indices[] =
{
    0, 1, 2,
    1, 3, 2,
    2, 3, 4,
    3, 5, 4,
    4, 5, 6,
    5, 7, 6,
    6, 7, 8,
    7, 9, 8,
    8, 9, 10,
    9, 11, 10,
    10, 11, 12,
    11, 13, 12
};

void UpReelRenderable::createInstanceAndBottomLevelAccelStruct(Sonic::CObjUpReel::CReelRenderer* reelRenderer)
{
    const auto reelRendererEx = reinterpret_cast<ReelRendererEx*>(reelRenderer);
    if (reelRendererEx->m_frame == RaytracingRendering::s_frame)
        return;

    const XXH32_hash_t vertexHash = XXH32(reelRendererEx->m_aVertexData, 
        sizeof(reelRendererEx->m_aVertexData), 0);

    if (vertexHash != reelRendererEx->m_vertexHash)
    {
        reelRendererEx->m_vertexHash = vertexHash;

        if (reelRendererEx->m_materialId == NULL)
        {
            uint32_t textureId = NULL;

            if (reelRendererEx->m_pObjUpReel->m_spDiffusePicture != nullptr &&
                reelRendererEx->m_pObjUpReel->m_spDiffusePicture->m_pD3DTexture != nullptr)
            {
                textureId = reinterpret_cast<Texture*>(reelRendererEx->m_pObjUpReel->m_spDiffusePicture->m_pD3DTexture)->getId();
            }

            RaytracingUtil::createSimpleMaterial(reelRendererEx->m_materialId, MATERIAL_FLAG_NONE, textureId);
        }

        constexpr size_t vertexByteSize = sizeof(OptimizedVertexData) * _countof(s_indices);
        const bool initialWrite = reelRendererEx->m_vertexBuffer == nullptr;

        if (initialWrite)
        {
            reelRendererEx->m_vertexBuffer.Attach(new VertexBuffer(vertexByteSize));

            auto& createMsg = s_messageSender.makeMessage<MsgCreateVertexBuffer>();
            createMsg.vertexBufferId = reelRendererEx->m_vertexBuffer->getId();
            createMsg.length = vertexByteSize;
            createMsg.allowUnorderedAccess = false;
            s_messageSender.endMessage();
        }

        auto& writeMsg = s_messageSender.makeMessage<MsgWriteVertexBuffer>(vertexByteSize);
        writeMsg.vertexBufferId = reelRendererEx->m_vertexBuffer->getId();
        writeMsg.offset = 0;
        writeMsg.initialWrite = initialWrite;

        OptimizedVertexData* vertex = reinterpret_cast<OptimizedVertexData*>(writeMsg.data);
        for (const auto index : s_indices)
        {
            const auto& vertexData = reelRenderer->m_aVertexData[index];

            vertex->position = vertexData.Position;
            vertex->color = 0xFFFFFFFF;
            vertex->normal = quantizeSnorm10(vertexData.Normal.normalized());
            vertex->texCoord[0] = quantizeHalf(vertexData.TexCoord.x());
            vertex->texCoord[1] = quantizeHalf(vertexData.TexCoord.y());

            ++vertex;
        }

        s_messageSender.endMessage();

        // TODO: This is unnecessary, but we have to do it due to double buffering vertex buffers when
        // GPU upload heaps are used, need to come up with a solution that does not require recreation
        RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, reelRendererEx->m_bottomLevelAccelStructId);

        reelRendererEx->m_bottomLevelAccelStructId = ModelData::s_idAllocator.allocate();

        auto& createMsg = s_messageSender.makeMessage<MsgCreateBottomLevelAccelStruct>(
            sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

        createMsg.bottomLevelAccelStructId = reelRendererEx->m_bottomLevelAccelStructId;
        createMsg.preferFastBuild = false;
        createMsg.allowUpdate = false;
        createMsg.asyncBuild = false;
        createMsg.allowCompaction = false;

        const auto geometryDesc = reinterpret_cast<MsgCreateBottomLevelAccelStruct::GeometryDesc*>(createMsg.data);

        memset(geometryDesc, 0, sizeof(*geometryDesc));
        geometryDesc->vertexBufferId = reelRendererEx->m_vertexBuffer->getId();
        geometryDesc->vertexStride = sizeof(OptimizedVertexData);
        geometryDesc->vertexCount = _countof(s_indices);
        geometryDesc->normalOffset = offsetof(OptimizedVertexData, normal);
        geometryDesc->texCoordOffsets[0] = offsetof(OptimizedVertexData, texCoord);
        geometryDesc->colorOffset = offsetof(OptimizedVertexData, color);
        geometryDesc->materialId = reelRendererEx->m_materialId;

        s_messageSender.endMessage();
    }

    const bool shouldCopyPrevTransform = (reelRendererEx->m_frame + 1) == RaytracingRendering::s_frame;
    reelRendererEx->m_frame = RaytracingRendering::s_frame;

    auto& message = s_messageSender.makeMessage<MsgCreateInstance>(0);

    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            message.transform[i][j] = reelRendererEx->m_WorldMatrix(i, j);

            if (j == 3)
                message.transform[i][j] += RaytracingRendering::s_worldShift[i];
        }
    }

    memcpy(message.prevTransform, shouldCopyPrevTransform ? reelRendererEx->m_prevTransform : message.transform, sizeof(message.prevTransform));

    message.bottomLevelAccelStructId = reelRendererEx->m_bottomLevelAccelStructId;
    message.isMirrored = false;
    message.instanceMask = INSTANCE_MASK_OBJECT;
    message.instanceType = INSTANCE_TYPE_OPAQUE;
    message.playableParam = -10001.0f;
    message.chrPlayableMenuParam = 10000.0f;
    message.forceAlphaColor = 1.0f;
    message.edgeEmissionParam = 0.0f;

    memcpy(reelRendererEx->m_prevTransform, message.transform, sizeof(reelRendererEx->m_prevTransform));

    s_messageSender.endMessage();
}

HOOK(void*, __stdcall, ReelRendererConstructor, 0x102EC00, ReelRendererEx* This, Sonic::CObjUpReel* objUpReel)
{
    void* result = originalReelRendererConstructor(This, objUpReel);

    This->m_materialId = NULL;
    This->m_bottomLevelAccelStructId = NULL;
    This->m_vertexHash = 0;
    new (std::addressof(This->m_vertexBuffer)) ComPtr<VertexBuffer>();
    This->m_frame = 0;

    return result;
}

HOOK(void*, __fastcall, ReelRendererDestructor, 0x458890, ReelRendererEx* This, void* _, uint8_t flags)
{
    This->m_vertexBuffer.~ComPtr();
    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, This->m_bottomLevelAccelStructId);
    RaytracingUtil::releaseResource(RaytracingResourceType::Material, This->m_materialId);

    return originalReelRendererDestructor(This, _, flags);
}

void UpReelRenderable::init()
{
    if (!Configuration::s_enableRaytracing)
        return;

    WRITE_MEMORY(0x102FDB7, uint32_t, sizeof(ReelRendererEx));

    INSTALL_HOOK(ReelRendererConstructor);
    INSTALL_HOOK(ReelRendererDestructor);
}
