#include "RopeRenderable.h"

#include "IndexBuffer.h"
#include "InstanceData.h"
#include "MaterialData.h"
#include "MaterialFlags.h"
#include "MeshOpt.h"
#include "MessageSender.h"
#include "ModelData.h"
#include "RaytracingRendering.h"
#include "RaytracingUtil.h"
#include "ShaderType.h"
#include "Texture.h"
#include "VertexBuffer.h"

struct Vertex
{
    float position[3];
    uint32_t color;
    uint32_t normal;
    uint32_t tangent;
    uint32_t binormal;
    uint16_t texCoord[2];
};

class RopeRenderableEx : public Sonic::CRopeRenderable
{
public:
    uint32_t m_materialId;
    float m_prevTransform[3][4];
    uint32_t m_bottomLevelAccelStructId;
    uint32_t m_vertexBufferId;
    ComPtr<IndexBuffer> m_indexBuffer;
    uint64_t m_frame;
};

void RopeRenderable::createInstanceAndBottomLevelAccelStruct(Sonic::CRopeRenderable* ropeRenderable)
{
    const auto ropeRenderableEx = reinterpret_cast<RopeRenderableEx*>(ropeRenderable);

    if (!ropeRenderableEx->m_Enabled ||
        ropeRenderableEx->m_VertexNum == 0 ||
        ropeRenderableEx->m_pD3DVertexBuffer == nullptr ||
        ropeRenderableEx->m_frame == RaytracingRendering::s_frame)
    {
        return;
    }

    const auto vertexBuffer = reinterpret_cast<VertexBuffer*>(ropeRenderableEx->m_pD3DVertexBuffer);

    if (ropeRenderableEx->m_vertexBufferId != vertexBuffer->getId())
    {
        const size_t indexByteSize = ropeRenderableEx->m_VertexNum * sizeof(uint16_t);

        if (ropeRenderableEx->m_indexBuffer == nullptr || 
            ropeRenderableEx->m_indexBuffer->getByteSize() != indexByteSize)
        {
            ropeRenderableEx->m_indexBuffer = new IndexBuffer(indexByteSize);

            auto& createMsg = s_messageSender.makeMessage<MsgCreateIndexBuffer>();
            createMsg.length = indexByteSize;
            createMsg.format = D3DFMT_INDEX16;
            createMsg.indexBufferId = ropeRenderableEx->m_indexBuffer->getId();
            s_messageSender.endMessage();

            auto& writeMsg = s_messageSender.makeMessage<MsgWriteIndexBuffer>(indexByteSize);
            writeMsg.indexBufferId = ropeRenderableEx->m_indexBuffer->getId();
            writeMsg.offset = 0;
            writeMsg.initialWrite = true;

            for (size_t i = 0; i < ropeRenderableEx->m_VertexNum; i++)
                *(reinterpret_cast<uint16_t*>(writeMsg.data) + i) = static_cast<uint16_t>(i);

            s_messageSender.endMessage();
        }

        ropeRenderableEx->m_vertexBufferId = vertexBuffer->getId();

        if (ropeRenderableEx->m_materialId == NULL)
        {
            ropeRenderableEx->m_materialId = MaterialData::s_idAllocator.allocate();

            auto& createMsg = s_messageSender.makeMessage<MsgCreateMaterial>();
            createMsg.materialId = ropeRenderableEx->m_materialId;
            createMsg.shaderType = SHADER_TYPE_COMMON;
            createMsg.flags = MATERIAL_FLAG_CONST_TEX_COORD;
            memset(createMsg.texCoordOffsets, 0, sizeof(createMsg.texCoordOffsets));

            createMsg.textureNum = 1;
            if (ropeRenderableEx->m_spDiffusePicture != nullptr &&
                ropeRenderableEx->m_spDiffusePicture->m_pD3DTexture != nullptr)
            {
                createMsg.textures[0].id = reinterpret_cast<Texture*>(ropeRenderableEx->m_spDiffusePicture->m_pD3DTexture)->getId();
            }
            else
            {
                createMsg.textures[0].id = NULL;
            }

            createMsg.textures[0].addressModeU = D3DTADDRESS_WRAP;
            createMsg.textures[0].addressModeV = D3DTADDRESS_WRAP;

            createMsg.parameterNum = 3;
            createMsg.parameters[0] = 1.0f;
            createMsg.parameters[1] = 1.0f;
            createMsg.parameters[2] = 1.0f;

            s_messageSender.endMessage();
        }

        RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, ropeRenderableEx->m_bottomLevelAccelStructId);
        ropeRenderableEx->m_bottomLevelAccelStructId = ModelData::s_idAllocator.allocate();

        auto& createMsg = s_messageSender.makeMessage<MsgCreateBottomLevelAccelStruct>(
            sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

        createMsg.bottomLevelAccelStructId = ropeRenderableEx->m_bottomLevelAccelStructId;
        createMsg.preferFastBuild = false;
        createMsg.allowUpdate = false;
        createMsg.buildAsync = false;
        createMsg.allowCompaction = false;

        const auto geometryDesc = reinterpret_cast<MsgCreateBottomLevelAccelStruct::GeometryDesc*>(createMsg.data);

        memset(geometryDesc, 0, sizeof(*geometryDesc));
        geometryDesc->indexBufferId = ropeRenderableEx->m_indexBuffer->getId();
        geometryDesc->indexCount = ropeRenderableEx->m_VertexNum;
        geometryDesc->vertexBufferId = vertexBuffer->getId();
        geometryDesc->vertexStride = sizeof(Vertex);
        geometryDesc->vertexCount = ropeRenderableEx->m_VertexNum;
        geometryDesc->normalOffset = offsetof(Vertex, normal);
        geometryDesc->texCoordOffsets[0] = offsetof(Vertex, texCoord);
        geometryDesc->colorOffset = offsetof(Vertex, color);
        geometryDesc->materialId = ropeRenderableEx->m_materialId;

        s_messageSender.endMessage();
    }

    const bool shouldCopyPrevTransform = (ropeRenderableEx->m_frame + 1) == RaytracingRendering::s_frame;
    ropeRenderableEx->m_frame = RaytracingRendering::s_frame;

    auto& message = s_messageSender.makeMessage<MsgCreateInstance>(0);

    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < 4; j++)
            message.transform[i][j] = (i == j) ? 1.0f : j == 3 ? RaytracingRendering::s_worldShift[i] : 0.0f;
    }

    memcpy(message.prevTransform, shouldCopyPrevTransform ? ropeRenderableEx->m_prevTransform : message.transform, sizeof(message.prevTransform));

    message.bottomLevelAccelStructId = ropeRenderableEx->m_bottomLevelAccelStructId;
    message.isMirrored = false;
    message.instanceMask = INSTANCE_MASK_OPAQUE;
    message.playableParam = -10001.0f;
    message.chrPlayableMenuParam = 10000.0f;
    message.forceAlphaColor = 1.0f;

    memcpy(ropeRenderableEx->m_prevTransform, message.transform, sizeof(ropeRenderableEx->m_prevTransform));

    s_messageSender.endMessage();
}

static void* __cdecl allocRopeRenderable()
{
    return __HH_ALLOC(sizeof(RopeRenderableEx));
}

void __fastcall ropeRenderableConstructor(RopeRenderableEx* This)
{
    Hedgehog::Mirage::fpCRenderableCtor(This);

    This->m_materialId = NULL;
    This->m_bottomLevelAccelStructId = NULL;
    This->m_vertexBufferId = 0;
    new (std::addressof(This->m_indexBuffer)) ComPtr<IndexBuffer>();
    This->m_frame = 0;
}

HOOK(void*, __fastcall, RopeRenderableDestructor, 0x5007A0, RopeRenderableEx* This, void* _, uint8_t flags)
{
    This->m_indexBuffer.~ComPtr();
    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, This->m_bottomLevelAccelStructId);
    RaytracingUtil::releaseResource(RaytracingResourceType::Material, This->m_materialId);

    return originalRopeRenderableDestructor(This, _, flags);
}

static void __cdecl implOfMemCpy(Vertex* dest, const Sonic::CRopeRenderable::SVertexData* src, uint32_t numBytes)
{
    const size_t numVertices = numBytes / sizeof(Vertex);
    for (size_t i = 0; i < numVertices; i++)
    {
        dest->position[0] = src->Position.x();
        dest->position[1] = src->Position.y();
        dest->position[2] = src->Position.z();
        dest->color = src->Color;
        dest->normal = quantizeSnorm10(src->Normal.normalized());
        dest->texCoord[0] = quantizeHalf(src->TexCoord.x());
        dest->texCoord[1] = quantizeHalf(src->TexCoord.y());

        ++dest;
        ++src;
    }
}

void RopeRenderable::init()
{
    WRITE_CALL(0xB4F443, allocRopeRenderable);
    WRITE_CALL(0xF03B84, allocRopeRenderable);
    WRITE_CALL(0xF0CF2B, allocRopeRenderable);
    WRITE_CALL(0x101CFF3, allocRopeRenderable);

    WRITE_CALL(0xF03B96, ropeRenderableConstructor);
    WRITE_CALL(0xF0CF3B, ropeRenderableConstructor);
    WRITE_CALL(0x11204A2, ropeRenderableConstructor);

    INSTALL_HOOK(RopeRenderableDestructor);

    // Pulley
    WRITE_MEMORY(0x11212F3, uint8_t, 0x6B, 0xFE, sizeof(Vertex), 0x83, 0xFF, 0x00, 0x90);
    WRITE_CALL(0x1121341, implOfMemCpy);

    // Generic
    WRITE_MEMORY(0x1122AB0, uint8_t, 0x6B, 0xFE, sizeof(Vertex), 0x83, 0xFF, 0x00, 0x90);
    WRITE_CALL(0x1122AFE, implOfMemCpy);

    // Espio
    WRITE_MEMORY(0x1122C30, uint8_t, 0x6B, 0xFE, sizeof(Vertex), 0x83, 0xFF, 0x00, 0x90);
    WRITE_CALL(0x1122C7A, implOfMemCpy);

    // Rooftop Run
    WRITE_MEMORY(0x1122D38, uint8_t, 0x6B, 0xFE, sizeof(Vertex), 0x83, 0xFF, 0x00, 0x90);
    WRITE_CALL(0x1122D82, implOfMemCpy);

    WRITE_MEMORY(0x112054B, uint32_t, offsetof(Vertex, texCoord));
    WRITE_MEMORY(0x1120557, uint32_t, offsetof(Vertex, color));
    WRITE_MEMORY(0x112055D, uint32_t, offsetof(Vertex, normal));
    WRITE_MEMORY(0x1120598, uint8_t, D3DDECLTYPE_FLOAT16_4);
    WRITE_MEMORY(0x11205AA, uint8_t, D3DDECLTYPE_DEC3N);
    WRITE_MEMORY(0x11207D3, uint8_t, sizeof(Vertex));
}
