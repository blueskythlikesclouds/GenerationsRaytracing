#include "WallJumpBlock.h"
#include "OptimizedVertexData.h"
#include "RaytracingUtil.h"
#include "VertexBuffer.h"
#include "MeshOpt.h"
#include "MessageSender.h"
#include "ModelData.h"
#include "MaterialData.h"
#include "RaytracingRendering.h"
#include "Configuration.h"

class WallJumpBlockRenderEx : public Sonic::CObjWallJumpBlock::CRender
{
public:
    uint32_t m_bottomLevelAccelStructId;
    uint32_t m_arrowBottomLevelAccelStructId;
    float m_prevTransform[3][4];
    ComPtr<VertexBuffer> m_vertexBuffer;
    XXH32_hash_t m_vertexHash;
    uint32_t m_frame;
};

static constexpr size_t s_indices[] =
{
    0, 1, 2,
    1, 3, 2
};

static const uint32_t s_normals[] =
{
    quantizeSnorm10(Sonic::CNoAlignVector{ 0.0f, 1.0f, 0.0f }),
    quantizeSnorm10(Sonic::CNoAlignVector{ -1.0f, 0.0f, 0.0f }),
    quantizeSnorm10(Sonic::CNoAlignVector{ 0.0f, -1.0f, 0.0f }),
    quantizeSnorm10(Sonic::CNoAlignVector{ 1.0f, 0.0f, 0.0f }),
};

static void copyVertexData(OptimizedVertexData* dest, const Sonic::CObjWallJumpBlock::CRender::SVertexData* source, const Hedgehog::Math::CMatrix* inverseTransform, uint32_t index)
{
    for (size_t i = 0; i < 6; i++)
    {
        auto& destVertex = dest[i];
        const auto& sourceVertex = source[s_indices[i]];

        if (inverseTransform != nullptr)
        {
            destVertex.position = (*inverseTransform) * sourceVertex.Position;
            destVertex.normal = s_normals[index];
        }
        else
        {
            destVertex.position = sourceVertex.Position;
            destVertex.normal = quantizeSnorm10((-sourceVertex.Normal).normalized());
            destVertex.tangent = quantizeSnorm10(sourceVertex.Tangent.normalized());
            destVertex.binormal = quantizeSnorm10(sourceVertex.Binormal.normalized());
        }

        destVertex.color = 0xFFFFFFFF;
        destVertex.texCoord[0] = quantizeHalf(sourceVertex.TexCoord[0].x());
        destVertex.texCoord[1] = quantizeHalf(sourceVertex.TexCoord[0].y());
    }
}

static void fillGeometryDesc(MsgCreateBottomLevelAccelStruct::GeometryDesc* geometryDesc, uint32_t vertexBufferId, uint32_t& vertexOffset, uint32_t materialId)
{
    memset(geometryDesc, 0, sizeof(*geometryDesc));
    geometryDesc->flags = GEOMETRY_FLAG_OPAQUE;
    geometryDesc->vertexBufferId = vertexBufferId;
    geometryDesc->vertexStride = sizeof(OptimizedVertexData);
    geometryDesc->vertexCount = 6;
    geometryDesc->vertexOffset = vertexOffset;
    geometryDesc->normalOffset = offsetof(OptimizedVertexData, normal);
    geometryDesc->tangentOffset = offsetof(OptimizedVertexData, tangent);
    geometryDesc->binormalOffset = offsetof(OptimizedVertexData, binormal);
    geometryDesc->colorOffset = offsetof(OptimizedVertexData, color);
    geometryDesc->texCoordOffsets[0] = offsetof(OptimizedVertexData, texCoord);
    geometryDesc->texCoordOffsets[1] = offsetof(OptimizedVertexData, texCoord);
    geometryDesc->texCoordOffsets[2] = offsetof(OptimizedVertexData, texCoord);
    geometryDesc->texCoordOffsets[3] = offsetof(OptimizedVertexData, texCoord);
    geometryDesc->materialId = materialId;

    vertexOffset += sizeof(OptimizedVertexData) * 6;
}

void WallJumpBlock::createInstanceAndBottomLevelAccelStruct(Sonic::CObjWallJumpBlock::CRender* wallJumpBlockRender)
{
    static Hedgehog::Base::CStringSymbol s_ambientSymbol("ambient");

    for (const auto& float4Param : wallJumpBlockRender->m_pObjWallJumpBlock->m_spArrowMaterial->m_Float4Params)
    {
        if (float4Param->m_Name == s_ambientSymbol)
        {
            memcpy(float4Param->m_spValue.get(), wallJumpBlockRender->m_pObjWallJumpBlock->m_spArrowMaterialMotion->m_MaterialMotionData.Ambient, sizeof(float[4]));
            break;
        }
    }

    MaterialData::create(*wallJumpBlockRender->m_pObjWallJumpBlock->m_spArrowMaterial, true);

    const auto wallJumpBlockRenderEx = reinterpret_cast<WallJumpBlockRenderEx*>(wallJumpBlockRender);
    const auto& transform = wallJumpBlockRender->m_pObjWallJumpBlock->m_spMatrixNodeTransform->GetWorldMatrix();
    const auto inverseTransform = transform.inverse();

    constexpr size_t vertexByteSize = sizeof(OptimizedVertexData) * 6 * 6;
    const bool initialWrite = wallJumpBlockRenderEx->m_vertexBuffer == nullptr;

    if (initialWrite)
    {
        wallJumpBlockRenderEx->m_vertexBuffer.Attach(new VertexBuffer(vertexByteSize));

        auto& createMsg = s_messageSender.makeMessage<MsgCreateVertexBuffer>();
        createMsg.vertexBufferId = wallJumpBlockRenderEx->m_vertexBuffer->getId();
        createMsg.length = vertexByteSize;
        createMsg.allowUnorderedAccess = false;
        s_messageSender.endMessage();
    }

    const XXH32_hash_t vertexHash = XXH32(wallJumpBlockRender->m_aPanelVertexData,
        offsetof(Sonic::CObjWallJumpBlock::CRender, m_VertexDeclaration) -
        offsetof(Sonic::CObjWallJumpBlock::CRender, m_aPanelVertexData), 0);

    if (wallJumpBlockRenderEx->m_vertexHash != vertexHash)
    {
        wallJumpBlockRenderEx->m_vertexHash = vertexHash;

        auto& writeMsg = s_messageSender.makeMessage<MsgWriteVertexBuffer>(vertexByteSize);
        writeMsg.vertexBufferId = wallJumpBlockRenderEx->m_vertexBuffer->getId();
        writeMsg.offset = 0;
        writeMsg.initialWrite = initialWrite;

        auto dest = reinterpret_cast<OptimizedVertexData*>(writeMsg.data);

        copyVertexData(dest, wallJumpBlockRender->m_aPanelVertexData, nullptr, 0);
        dest += 6;

        copyVertexData(dest, wallJumpBlockRender->m_aBackVertexData, nullptr, 0);
        dest += 6;

        for (size_t i = 0; i < 4; i++)
        {
            copyVertexData(dest, wallJumpBlockRender->m_aArrowVertexData[i], &inverseTransform, i);
            dest += 6;
        }

        s_messageSender.endMessage();

        RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct,
            wallJumpBlockRenderEx->m_bottomLevelAccelStructId);

        wallJumpBlockRenderEx->m_bottomLevelAccelStructId = ModelData::s_idAllocator.allocate();

        auto& blasMessage = s_messageSender.makeMessage<MsgCreateBottomLevelAccelStruct>(
            sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc) * 6);

        blasMessage.bottomLevelAccelStructId = wallJumpBlockRenderEx->m_bottomLevelAccelStructId;
        blasMessage.allowUpdate = false;
        blasMessage.allowCompaction = false;
        blasMessage.preferFastBuild = false;
        blasMessage.asyncBuild = false;

        auto geometryDesc = reinterpret_cast<MsgCreateBottomLevelAccelStruct::GeometryDesc*>(blasMessage.data);

        const auto vertexBufferId = wallJumpBlockRenderEx->m_vertexBuffer->getId();
        uint32_t vertexOffset = 0;

        fillGeometryDesc(geometryDesc++,
            vertexBufferId,
            vertexOffset,
            reinterpret_cast<MaterialDataEx*>(wallJumpBlockRender->m_pObjWallJumpBlock->m_spPanelMaterial.get())->m_materialId);

        fillGeometryDesc(geometryDesc++,
            vertexBufferId,
            vertexOffset,
            reinterpret_cast<MaterialDataEx*>(wallJumpBlockRender->m_pObjWallJumpBlock->m_spBackMaterial.get())->m_materialId);

        for (size_t i = 0; i < 4; i++)
        {
            fillGeometryDesc(geometryDesc++,
                vertexBufferId,
                vertexOffset,
                reinterpret_cast<MaterialDataEx*>(wallJumpBlockRender->m_pObjWallJumpBlock->m_spArrowMaterial.get())->m_materialId);
        }

        s_messageSender.endMessage();
    }

    const bool shouldCopyPrevTransform = (wallJumpBlockRenderEx->m_frame + 1) == RaytracingRendering::s_frame;
    wallJumpBlockRenderEx->m_frame = RaytracingRendering::s_frame;

    auto& instanceMsg = s_messageSender.makeMessage<MsgCreateInstance>(0);

    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            instanceMsg.transform[i][j] = transform(i, j);

            if (j == 3)
                instanceMsg.transform[i][j] += RaytracingRendering::s_worldShift[i];
        }
    }

    memcpy(instanceMsg.prevTransform, shouldCopyPrevTransform ? wallJumpBlockRenderEx->m_prevTransform : instanceMsg.transform, sizeof(instanceMsg.prevTransform));

    instanceMsg.bottomLevelAccelStructId = wallJumpBlockRenderEx->m_bottomLevelAccelStructId;
    instanceMsg.isMirrored = false;
    instanceMsg.instanceMask = INSTANCE_MASK_OBJECT;
    instanceMsg.instanceType = INSTANCE_TYPE_OPAQUE;
    instanceMsg.playableParam = -10001.0f;
    instanceMsg.chrPlayableMenuParam = 10000.0f;
    instanceMsg.forceAlphaColor = 1.0f;
    instanceMsg.edgeEmissionParam = 0.0f;

    memcpy(wallJumpBlockRenderEx->m_prevTransform, instanceMsg.transform, sizeof(wallJumpBlockRenderEx->m_prevTransform));

    s_messageSender.endMessage();
}

static void __fastcall wallJumpBlockRenderConstructor(WallJumpBlockRenderEx* This)
{
    Hedgehog::Mirage::fpCRenderableCtor(This);

    This->m_bottomLevelAccelStructId = NULL;
    new (std::addressof(This->m_vertexBuffer)) ComPtr<VertexBuffer>();
    This->m_vertexHash = 0;
    This->m_frame = 0;
}

HOOK(void, __fastcall, WallJumpBlockRenderDestructor, 0x4673E0, WallJumpBlockRenderEx* This, void* _, uint8_t dtorFlags)
{
    This->m_vertexBuffer.~ComPtr();
    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, This->m_bottomLevelAccelStructId);

    originalWallJumpBlockRenderDestructor(This, _, dtorFlags);
}

void WallJumpBlock::init()
{
    if (!Configuration::s_enableRaytracing)
        return;

    WRITE_MEMORY(0x1001535, uint32_t, sizeof(WallJumpBlockRenderEx));

    WRITE_CALL(0x1001549, wallJumpBlockRenderConstructor);
    INSTALL_HOOK(WallJumpBlockRenderDestructor);
}
