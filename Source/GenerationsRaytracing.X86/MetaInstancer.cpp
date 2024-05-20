#include "MetaInstancer.h"
#include "VertexBuffer.h"
#include "RaytracingUtil.h"
#include "Logger.h"
#include "MessageSender.h"
#include "ModelData.h"
#include "MaterialData.h"
#include "ShaderType.h"
#include "MaterialFlags.h"
#include "RaytracingShader.h"
#include "Texture.h"
#include "RaytracingRendering.h"
#include "OptimizedVertexData.h"

class ObjGrassInstancerEx : public Sonic::CObjGrassInstancer
{
public:
    ComPtr<VertexBuffer> m_vertexBuffer;
    float m_prevTransform[3][4];
    uint32_t m_bottomLevelAccelStructId;
    uint32_t m_materialId;
    uint32_t m_frame;
};

void MetaInstancer::createInstanceAndBottomLevelAccelStruct(Sonic::CInstanceRenderObjDX9* instanceRenderObj)
{
    const auto objGrassInstancer = dynamic_cast<Sonic::CObjGrassInstancer*>(instanceRenderObj->m_pObjInstancer);

    if (objGrassInstancer == nullptr)
        return;

    uint32_t lodCount = 0;

    for (auto& instanceCount : instanceRenderObj->m_aInstanceNum)
    {
        if (instanceCount != 0)
            ++lodCount;
    }

    if (lodCount == 0)
        return;

    const auto objGrassInstancerEx = reinterpret_cast<ObjGrassInstancerEx*>(objGrassInstancer);

    if (objGrassInstancerEx->m_vertexBuffer == nullptr)
    {
        const size_t vertexByteSize = (reinterpret_cast<VertexBuffer*>(instanceRenderObj->m_pD3DVertexBuffers[0])->getByteSize() /
            instanceRenderObj->m_VertexSize) * sizeof(OptimizedVertexData) * 6;

        objGrassInstancerEx->m_vertexBuffer.Attach(new VertexBuffer(vertexByteSize));

        auto& message = s_messageSender.makeMessage<MsgCreateVertexBuffer>();
        message.vertexBufferId = objGrassInstancerEx->m_vertexBuffer->getId();
        message.length = vertexByteSize;
        message.allowUnorderedAccess = true;
        s_messageSender.endMessage();
    }

    auto& computeMsg = s_messageSender.makeMessage<MsgComputeGrassInstancer>(
        lodCount * sizeof(MsgComputeGrassInstancer::LodDesc));

    memcpy(computeMsg.instanceTypes, objGrassInstancer->m_scpInstanceTypes, sizeof(computeMsg.instanceTypes));
    computeMsg.misc = objGrassInstancer->m_Misc;
    computeMsg.instanceBufferId = reinterpret_cast<VertexBuffer*>(instanceRenderObj->m_pD3DVertexBuffers[0])->getId();
    computeMsg.vertexBufferId = objGrassInstancerEx->m_vertexBuffer->getId();

    auto lodDesc = reinterpret_cast<MsgComputeGrassInstancer::LodDesc*>(computeMsg.data);

    for (size_t i = 0; i < _countof(instanceRenderObj->m_aInstanceNum); i++)
    {
        auto& instanceCount = instanceRenderObj->m_aInstanceNum[i];
        if (instanceCount != 0)
        {
            auto& vertexOffset = instanceRenderObj->m_VertexOffsets[i];

            lodDesc->instanceOffset = vertexOffset;
            lodDesc->instanceCount = instanceCount;
            lodDesc->vertexOffset = (vertexOffset / instanceRenderObj->m_VertexSize) * sizeof(OptimizedVertexData) * 6;
            ++lodDesc;
        }
    }

    s_messageSender.endMessage();

    if (objGrassInstancerEx->m_materialId == NULL)
    {
        uint32_t textureId = NULL;

        if (instanceRenderObj->m_aInstanceModelData[0]->m_aTextureData[0] != nullptr &&
            instanceRenderObj->m_aInstanceModelData[0]->m_aTextureData[0]->m_spPictureData != nullptr &&
            instanceRenderObj->m_aInstanceModelData[0]->m_aTextureData[0]->m_spPictureData->m_pD3DTexture != nullptr)
        {
            textureId = reinterpret_cast<Texture*>(instanceRenderObj->m_aInstanceModelData[0]->m_aTextureData[0]->m_spPictureData->m_pD3DTexture)->getId();
        }

        RaytracingUtil::createSimpleMaterial(objGrassInstancerEx->m_materialId, MATERIAL_FLAG_DOUBLE_SIDED | MATERIAL_FLAG_FULBRIGHT, textureId);
    }

    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, 
        objGrassInstancerEx->m_bottomLevelAccelStructId);

    objGrassInstancerEx->m_bottomLevelAccelStructId = ModelData::s_idAllocator.allocate();

    auto& blasMsg = s_messageSender.makeMessage<MsgCreateBottomLevelAccelStruct>(
        lodCount * sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    blasMsg.bottomLevelAccelStructId = objGrassInstancerEx->m_bottomLevelAccelStructId;
    blasMsg.allowUpdate = false;
    blasMsg.allowCompaction = false;
    blasMsg.preferFastBuild = true;
    blasMsg.asyncBuild = false;

    auto geometryDesc = reinterpret_cast<MsgCreateBottomLevelAccelStruct::GeometryDesc*>(blasMsg.data);

    for (size_t i = 0; i < _countof(instanceRenderObj->m_aInstanceNum); i++)
    {
        auto& instanceCount = instanceRenderObj->m_aInstanceNum[i];

        if (instanceCount != 0)
        {
            memset(geometryDesc, 0, sizeof(*geometryDesc));

            geometryDesc->flags = GEOMETRY_FLAG_PUNCH_THROUGH;
            geometryDesc->vertexBufferId = objGrassInstancerEx->m_vertexBuffer->getId();
            geometryDesc->vertexStride = sizeof(OptimizedVertexData);
            geometryDesc->vertexCount = instanceCount * 6;
            geometryDesc->vertexOffset = (instanceRenderObj->m_VertexOffsets[i] / instanceRenderObj->m_VertexSize) * sizeof(OptimizedVertexData) * 6;
            geometryDesc->normalOffset = offsetof(OptimizedVertexData, normal);
            geometryDesc->tangentOffset = offsetof(OptimizedVertexData, tangent);
            geometryDesc->binormalOffset = offsetof(OptimizedVertexData, binormal);
            geometryDesc->colorOffset = offsetof(OptimizedVertexData, color);
            geometryDesc->texCoordOffsets[0] = offsetof(OptimizedVertexData, texCoord);
            geometryDesc->materialId = objGrassInstancerEx->m_materialId;

            ++geometryDesc;
        }
    }

    s_messageSender.endMessage();

    const bool shouldCopyPrevTransform = (objGrassInstancerEx->m_frame + 1) == RaytracingRendering::s_frame;
    objGrassInstancerEx->m_frame = RaytracingRendering::s_frame;

    auto& instanceMsg = s_messageSender.makeMessage<MsgCreateInstance>(0);

    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < 4; j++)
            instanceMsg.transform[i][j] = (i == j) ? 1.0f : j == 3 ? RaytracingRendering::s_worldShift[i] : 0.0f;
    }

    memcpy(instanceMsg.prevTransform, shouldCopyPrevTransform ? objGrassInstancerEx->m_prevTransform : instanceMsg.transform, sizeof(instanceMsg.prevTransform));

    instanceMsg.bottomLevelAccelStructId = objGrassInstancerEx->m_bottomLevelAccelStructId;
    instanceMsg.isMirrored = false;
    instanceMsg.instanceMask = INSTANCE_MASK_INSTANCER;
    instanceMsg.instanceType = INSTANCE_TYPE_OPAQUE;
    instanceMsg.playableParam = -10001.0f;
    instanceMsg.chrPlayableMenuParam = 10000.0f;
    instanceMsg.forceAlphaColor = 1.0f;

    memcpy(objGrassInstancerEx->m_prevTransform, instanceMsg.transform, sizeof(objGrassInstancerEx->m_prevTransform));

    s_messageSender.endMessage();
}

static ObjGrassInstancerEx* __fastcall objGrassInstancerConstructorMidAsmHook(ObjGrassInstancerEx* This)
{
    new (std::addressof(This->m_vertexBuffer)) ComPtr<VertexBuffer>();
    This->m_bottomLevelAccelStructId = NULL;
    This->m_materialId = NULL;
    This->m_frame = 0;
    return This;
}

static void __declspec(naked) objGrassInstancerConstructorTrampoline()
{
    __asm
    {
        mov ecx, eax
        call objGrassInstancerConstructorMidAsmHook
        retn 4  
    }
}

HOOK(void, __fastcall, ObjGrassInstancerDestructor, 0x11B4EB0, ObjGrassInstancerEx* This)
{
    RaytracingUtil::releaseResource(RaytracingResourceType::Material, This->m_materialId);
    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, This->m_bottomLevelAccelStructId);
    This->m_vertexBuffer.~ComPtr();

    originalObjGrassInstancerDestructor(This);
}

HOOK(void, __stdcall, ParseInstancerAsset, 0xCDAB00, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4)
{
    originalParseInstancerAsset(a1, a2, a3, a4);

    static Hedgehog::Base::CStringSymbol s_grassSymbol("Grass");
    static Hedgehog::Base::CStringSymbol s_effectSymbol("Effect");

    auto desc = *reinterpret_cast<uintptr_t*>(a1 + 32);

    if (*reinterpret_cast<Hedgehog::Base::CStringSymbol*>(desc + 4) != s_grassSymbol)
        *reinterpret_cast<Hedgehog::Base::CStringSymbol*>(desc + 32) = s_effectSymbol;
}

void MetaInstancer::init()
{
    WRITE_MEMORY(0x11B55AC, uint32_t, sizeof(ObjGrassInstancerEx));

    WRITE_JUMP(0x11B5575, objGrassInstancerConstructorTrampoline);
    INSTALL_HOOK(ObjGrassInstancerDestructor);

    INSTALL_HOOK(ParseInstancerAsset);
}
