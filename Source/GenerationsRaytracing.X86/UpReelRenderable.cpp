#include "UpReelRenderable.h"

#include "IndexBuffer.h"
#include "InstanceData.h"
#include "MaterialData.h"
#include "MaterialFlags.h"
#include "MeshOpt.h"
#include "Message.h"
#include "MessageSender.h"
#include "ModelData.h"
#include "RaytracingUtil.h"
#include "ShaderType.h"
#include "Texture.h"
#include "VertexBuffer.h"

class ReelRendererEx : public Sonic::CObjUpReel::CReelRenderer
{
public:
    uint32_t m_materialId;
    uint32_t m_bottomLevelAccelStructId;
    uint32_t m_instanceId;
    XXH32_hash_t m_vertexHash;
    ComPtr<VertexBuffer> m_vertexBuffer;
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

static ComPtr<IndexBuffer> s_indexBuffer;

void UpReelRenderable::createInstanceAndBottomLevelAccelStruct(Sonic::CObjUpReel::CReelRenderer* reelRenderer)
{
    const auto reelRendererEx = reinterpret_cast<ReelRendererEx*>(reelRenderer);

    const XXH32_hash_t vertexHash = XXH32(reelRendererEx->m_aVertexData, 
        sizeof(reelRendererEx->m_aVertexData), 0);

    if (vertexHash != reelRendererEx->m_vertexHash)
    {
        if (s_indexBuffer == nullptr)
        {
            s_indexBuffer.Attach(new IndexBuffer(sizeof(s_indices)));

            auto& createMsg = s_messageSender.makeMessage<MsgCreateIndexBuffer>();
            createMsg.length = sizeof(s_indices);
            createMsg.format = D3DFMT_INDEX16;
            createMsg.indexBufferId = s_indexBuffer->getId();
            s_messageSender.endMessage();

            auto& writeMsg = s_messageSender.makeMessage<MsgWriteIndexBuffer>(sizeof(s_indices));
            writeMsg.indexBufferId = s_indexBuffer->getId();
            writeMsg.offset = 0;
            writeMsg.initialWrite = true;
            memcpy(writeMsg.data, s_indices, sizeof(s_indices));
            s_messageSender.endMessage();
        }

        reelRendererEx->m_vertexHash = vertexHash;

        if (reelRendererEx->m_materialId == NULL)
        {
            reelRendererEx->m_materialId = MaterialData::s_idAllocator.allocate();

            auto& createMsg = s_messageSender.makeMessage<MsgCreateMaterial>();
            createMsg.materialId = reelRendererEx->m_materialId;
            createMsg.shaderType = SHADER_TYPE_COMMON;
            createMsg.flags = MATERIAL_FLAG_CONST_TEX_COORD;

            createMsg.diffuseTexture.id = reinterpret_cast<Texture*>(reelRendererEx->m_pObjUpReel->m_spDiffusePicture->m_pD3DTexture)->getId();
            createMsg.diffuseTexture.addressModeU = D3DTADDRESS_WRAP;
            createMsg.diffuseTexture.addressModeV = D3DTADDRESS_WRAP;

            memset(&createMsg.diffuseTexture2, 0, sizeof(MsgCreateMaterial) - offsetof(MsgCreateMaterial, diffuseTexture2));

            createMsg.diffuse[0] = 1.0f;
            createMsg.diffuse[1] = 1.0f;
            createMsg.diffuse[2] = 1.0f;

            s_messageSender.endMessage();
        }

        struct Vertex
        {
            float position[3];
            uint16_t normal[4];
            uint16_t texCoord[2];
            uint32_t color;
        };

        constexpr size_t vertexByteSize = sizeof(Vertex) * _countof(reelRendererEx->m_aVertexData);
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

        Vertex* vertex = reinterpret_cast<Vertex*>(writeMsg.data);
        for (auto& vertexData : reelRendererEx->m_aVertexData)
        {
            vertex->position[0] = vertexData.Position.x();
            vertex->position[1] = vertexData.Position.y();
            vertex->position[2] = vertexData.Position.z();
            vertex->normal[0] = quantizeHalf(vertexData.Normal.x());
            vertex->normal[1] = quantizeHalf(vertexData.Normal.y());
            vertex->normal[2] = quantizeHalf(vertexData.Normal.z());
            vertex->texCoord[0] = quantizeHalf(vertexData.TexCoord.x());
            vertex->texCoord[1] = quantizeHalf(vertexData.TexCoord.y());
            vertex->color = 0xFFFFFFFF;

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
        createMsg.preferFastBuild = true;

        const auto geometryDesc = reinterpret_cast<MsgCreateBottomLevelAccelStruct::GeometryDesc*>(createMsg.data);

        memset(geometryDesc, 0, sizeof(*geometryDesc));
        geometryDesc->indexBufferId = s_indexBuffer->getId();
        geometryDesc->indexCount = _countof(s_indices);
        geometryDesc->vertexBufferId = reelRendererEx->m_vertexBuffer->getId();
        geometryDesc->vertexStride = sizeof(Vertex);
        geometryDesc->vertexCount = _countof(reelRendererEx->m_aVertexData);
        geometryDesc->normalOffset = offsetof(Vertex, normal);
        geometryDesc->texCoordOffsets[0] = offsetof(Vertex, texCoord);
        geometryDesc->colorOffset = offsetof(Vertex, color);
        geometryDesc->materialId = reelRendererEx->m_materialId;

        s_messageSender.endMessage();
    }

    const bool storePrevTransform = reelRendererEx->m_instanceId != NULL;

    if (reelRendererEx->m_instanceId == NULL)
        reelRendererEx->m_instanceId = InstanceData::s_idAllocator.allocate();

    auto& message = s_messageSender.makeMessage<MsgCreateInstance>(0);

    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < 4; j++)
            message.transform[i][j] = reelRendererEx->m_WorldMatrix(i, j);
    }

    message.instanceId = reelRendererEx->m_instanceId;
    message.bottomLevelAccelStructId = reelRendererEx->m_bottomLevelAccelStructId;
    message.storePrevTransform = storePrevTransform;
    message.isMirrored = false;
    message.instanceMask = INSTANCE_MASK_OPAQUE;

    s_messageSender.endMessage();
}

HOOK(void*, __stdcall, ReelRendererConstructor, 0x102EC00, ReelRendererEx* This, Sonic::CObjUpReel* objUpReel)
{
    void* result = originalReelRendererConstructor(This, objUpReel);

    This->m_materialId = NULL;
    This->m_bottomLevelAccelStructId = NULL;
    This->m_instanceId = NULL;
    This->m_vertexHash = 0;
    new (std::addressof(This->m_vertexBuffer)) ComPtr<VertexBuffer>();

    return result;
}

HOOK(void*, __fastcall, ReelRendererDestructor, 0x458890, ReelRendererEx* This, void* _, uint8_t flags)
{
    This->m_vertexBuffer.~ComPtr();
    RaytracingUtil::releaseResource(RaytracingResourceType::Instance, This->m_instanceId);
    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, This->m_bottomLevelAccelStructId);
    RaytracingUtil::releaseResource(RaytracingResourceType::Material, This->m_materialId);

    return originalReelRendererDestructor(This, _, flags);
}

void UpReelRenderable::init()
{
    WRITE_MEMORY(0x102FDB7, uint32_t, sizeof(ReelRendererEx));

    INSTALL_HOOK(ReelRendererConstructor);
    INSTALL_HOOK(ReelRendererDestructor);
}
