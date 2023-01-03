#include "RaytracingBridge.h"

#include "Bridge.h"
#include "DLSS.h"
#include "File.h"
#include "FSR.h"
#include "Message.h"
#include "ShaderMapping.h"
#include "Utilities.h"

#include "CopyPS.h"
#include "CopyVS.h"
#include "Skinning.h"

RaytracingBridge::RaytracingBridge(const Device& device, const std::string& directoryPath)
{
    const INIReader reader("GenerationsRaytracing.ini");

    if (reader.GetInteger("Mod", "Upscaler", 0))
        upscaler = std::make_unique<FSR>(device);
    else 
        upscaler = std::make_unique<DLSS>(device, directoryPath);
}

RaytracingBridge::~RaytracingBridge() = default;

void RaytracingBridge::procMsgCreateGeometry(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateGeometry>();
    const void* data = bridge.msgReceiver.getDataAndMoveNext(msg->nodeNum);

    auto& bottomLevelAS = bottomLevelAccelStructs[msg->bottomLevelAS][msg->instanceInfo];

    if (bottomLevelAS.handle)
        bottomLevelAS = {};

    auto& geometryDesc = bottomLevelAS.desc.bottomLevelGeometries.emplace_back();

    if (msg->opaque)
        geometryDesc.flags = nvrhi::rt::GeometryFlags::Opaque;

    auto& triangles = geometryDesc.geometryData.triangles;

    triangles.indexBuffer = nvrhi::checked_cast<nvrhi::IBuffer*>(bridge.resources[msg->indexBuffer].Get());
    triangles.vertexBuffer = nvrhi::checked_cast<nvrhi::IBuffer*>(bridge.resources[msg->vertexBuffer].Get());

    assert(triangles.indexBuffer && triangles.vertexBuffer);

    triangles.indexFormat = nvrhi::Format::R16_UINT;
    triangles.vertexFormat = nvrhi::Format::RGB32_FLOAT;
    triangles.indexOffset = msg->indexOffset;
    triangles.vertexOffset = msg->vertexOffset;
    triangles.indexCount = msg->indexCount;
    triangles.vertexCount = msg->vertexCount;
    triangles.vertexStride = msg->vertexStride;

    auto& geometry = bottomLevelAS.geometries.emplace_back();
    geometry.vertexCount = msg->vertexCount;
    geometry.vertexStride = msg->vertexStride;
    geometry.normalOffset = msg->normalOffset;
    geometry.tangentOffset = msg->tangentOffset;
    geometry.binormalOffset = msg->binormalOffset;
    geometry.texCoordOffset = msg->texCoordOffset;
    geometry.colorOffset = msg->colorOffset;
    geometry.colorFormat = msg->colorFormat;
    geometry.blendWeightOffset = msg->blendWeightOffset;
    geometry.blendIndicesOffset = msg->blendIndicesOffset;
    geometry.material = msg->material;
    geometry.punchThrough = msg->punchThrough;

    auto& skinnedGeometry = bottomLevelAS.skinnedGeometries.emplace_back();

    if (msg->nodeNum > 0)
    {
        skinnedGeometry.nodeIndices = bridge.device.nvrhi->createBuffer(nvrhi::BufferDesc()
            .setByteSize(msg->nodeNum)
            .setFormat(nvrhi::Format::R8_UINT)
            .setCanHaveTypedViews(true)
            .setCpuAccess(nvrhi::CpuAccessMode::Write));

        void* memory = bridge.device.nvrhi->mapBuffer(skinnedGeometry.nodeIndices, nvrhi::CpuAccessMode::Write);
        memcpy(memory, data, msg->nodeNum);
        bridge.device.nvrhi->unmapBuffer(skinnedGeometry.nodeIndices);
    }

    bottomLevelAS.buffers.push_back(triangles.indexBuffer);
    bottomLevelAS.buffers.push_back(triangles.vertexBuffer);
}

void RaytracingBridge::procMsgCreateBottomLevelAS(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateBottomLevelAS>();
    const void* data = bridge.msgReceiver.getDataAndMoveNext(msg->matrixNum * 64);

    const auto blasPair = bottomLevelAccelStructs.find(msg->bottomLevelAS);
    if (blasPair == bottomLevelAccelStructs.end())
        return;

    const auto instancePair = blasPair->second.find(msg->instanceInfo);
    if (instancePair == blasPair->second.end())
        return;

    instancePair->second.handle = bridge.device.nvrhi->createAccelStruct(instancePair->second.desc
        .setBuildFlags(msg->matrixNum > 0 ? nvrhi::rt::AccelStructBuildFlags::PreferFastBuild : nvrhi::rt::AccelStructBuildFlags::PreferFastTrace));

    if (msg->matrixNum > 0)
    {
        instancePair->second.matrixBuffer = bridge.device.nvrhi->createBuffer(nvrhi::BufferDesc()
            .setByteSize(msg->matrixNum * 64)
            .setStructStride(64)
            .setCpuAccess(nvrhi::CpuAccessMode::Write));

        void* memory = bridge.device.nvrhi->mapBuffer(instancePair->second.matrixBuffer, nvrhi::CpuAccessMode::Write);
        memcpy(memory, data, msg->matrixNum * 64);
        bridge.device.nvrhi->unmapBuffer(instancePair->second.matrixBuffer);
    }
}

void RaytracingBridge::procMsgCreateInstance(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateInstance>();

    auto& instance = instances.emplace_back();
    memcpy(instance.transform, msg->transform, sizeof(instance.transform));
    memcpy(instance.delta, msg->delta, sizeof(instance.delta));
    instance.bottomLevelAS = msg->bottomLevelAS;
    instance.instanceInfo = msg->instanceInfo;
    instance.instanceMask = msg->instanceMask;
}

void RaytracingBridge::procMsgCreateMaterial(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateMaterial>();

    auto& material = materials[msg->material];

    strcpy(material.shader, msg->shader);
    memcpy(material.textures, msg->textures, sizeof(msg->textures));
    memcpy(material.parameters, msg->parameters, sizeof(msg->parameters));
}

void RaytracingBridge::procMsgReleaseInstanceInfo(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgReleaseInstanceInfo>();
    pendingReleases.push_back(std::make_pair(msg->bottomLevelAS, msg->instanceInfo));
}

template<typename T>
static void createUploadBuffer(const Bridge& bridge, const std::vector<T>& vector, nvrhi::BufferHandle& buffer)
{
    buffer = bridge.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(vector))
        .setStructStride((uint32_t)vectorByteStride(vector))
        .setCpuAccess(nvrhi::CpuAccessMode::Write));

    void* copyDest = bridge.device.nvrhi->mapBuffer(buffer, nvrhi::CpuAccessMode::Write);
    memcpy(copyDest, vector.data(), vectorByteSize(vector));
    bridge.device.nvrhi->unmapBuffer(buffer);
}

void RaytracingBridge::procMsgNotifySceneTraversed(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgNotifySceneTraversed>();

    if (instances.empty())
        return;

    auto& colorAttachment = bridge.framebufferDesc.colorAttachments[0].texture;
    if (!colorAttachment || colorAttachment->getDesc().format != nvrhi::Format::RGBA16_FLOAT)
        return;

    if (!pipeline)
    {
        size_t shaderLibraryByteSize = 0;
        auto shaderLibraryBytes = readAllBytes(bridge.directoryPath + "/ShaderLibrary.cso", shaderLibraryByteSize);

        shaderLibrary = bridge.device.nvrhi->createShaderLibrary(shaderLibraryBytes.get(), shaderLibraryByteSize);

        bindingLayout = bridge.device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
            .setVisibility(nvrhi::ShaderType::AllRayTracing)
            .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
            .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1))
            .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(2))
            .addItem(nvrhi::BindingLayoutItem::RayTracingAccelStruct(0))

            .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1))
            .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2))

            .addItem(nvrhi::BindingLayoutItem::Texture_UAV(0))
            .addItem(nvrhi::BindingLayoutItem::Texture_UAV(1))
            .addItem(nvrhi::BindingLayoutItem::Texture_UAV(2))
            .addItem(nvrhi::BindingLayoutItem::Texture_UAV(3))
            .addItem(nvrhi::BindingLayoutItem::Texture_UAV(4))

            .addItem(nvrhi::BindingLayoutItem::Texture_SRV(4))
            .addItem(nvrhi::BindingLayoutItem::Texture_SRV(5))
            .addItem(nvrhi::BindingLayoutItem::Texture_SRV(6))

            .addItem(nvrhi::BindingLayoutItem::Sampler(0)));

        geometryBindlessLayout = bridge.device.nvrhi->createBindlessLayout(nvrhi::BindlessLayoutDesc()
            .setVisibility(nvrhi::ShaderType::AllRayTracing)
            .setMaxCapacity(65336)
            .addRegisterSpace(nvrhi::BindingLayoutItem::TypedBuffer_SRV(1))
            .addRegisterSpace(nvrhi::BindingLayoutItem::RawBuffer_SRV(2)));

        textureBindlessLayout = bridge.device.nvrhi->createBindlessLayout(nvrhi::BindlessLayoutDesc()
            .setVisibility(nvrhi::ShaderType::AllRayTracing)
            .setMaxCapacity(65336)
            .addRegisterSpace(nvrhi::BindingLayoutItem::Texture_SRV(3))
            .addRegisterSpace(nvrhi::BindingLayoutItem::Texture_SRV(4)));

        rtConstantBuffer = bridge.device.nvrhi->createBuffer(nvrhi::BufferDesc()
            .setByteSize(sizeof(RTConstants))
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setMaxVersions(1));

        linearRepeatSampler = bridge.device.nvrhi->createSampler(nvrhi::SamplerDesc()
            .setAllFilters(true)
            .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat));

        auto pipelineDesc = nvrhi::rt::PipelineDesc()
            .addBindingLayout(bindingLayout)
            .addBindingLayout(geometryBindlessLayout)
            .addBindingLayout(textureBindlessLayout)
            .addShader({ "", shaderLibrary->getShader("RayGeneration", nvrhi::ShaderType::RayGeneration) })
            .addShader({ "", shaderLibrary->getShader("MissSkyBox", nvrhi::ShaderType::Miss) })
            .addShader({ "", shaderLibrary->getShader("MissEnvironmentColor", nvrhi::ShaderType::Miss) })
            .setMaxRecursionDepth(8)
            .setMaxPayloadSize(32);

        ShaderMapping shaderMapping;
        shaderMapping.load(bridge.directoryPath + "/ShaderMapping.bin");

        for (auto& [name, shader] : shaderMapping.shaders)
        {
            pipelineDesc.addHitGroup({ name,
                shaderLibrary->getShader(shader.closestHit.c_str(), nvrhi::ShaderType::ClosestHit),
                shaderLibrary->getShader(shader.anyHit.c_str(), nvrhi::ShaderType::AnyHit) });
        }

        pipeline = bridge.device.nvrhi->createRayTracingPipeline(pipelineDesc);
    }

    if (!skinningPipeline)
    {
        skinningShader = bridge.device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Compute), SKINNING, sizeof(SKINNING));

        skinningBindingLayout = bridge.device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
            .setVisibility(nvrhi::ShaderType::Compute)
            .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
            .addItem(nvrhi::BindingLayoutItem::RawBuffer_SRV(0))
            .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(1))
            .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2))
            .addItem(nvrhi::BindingLayoutItem::RawBuffer_UAV(0)));

        skinningConstantBuffer = bridge.device.nvrhi->createBuffer(nvrhi::BufferDesc()
            .setByteSize(sizeof(Geometry))
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setMaxVersions(1));

        skinningPipeline = bridge.device.nvrhi->createComputePipeline(nvrhi::ComputePipelineDesc()
            .setComputeShader(skinningShader)
            .addBindingLayout(skinningBindingLayout));
    }

    geometryDescriptorTable = nullptr;
    textureDescriptorTable = nullptr;

    static std::vector<nvrhi::TextureHandle> textures;
    static std::unordered_map<unsigned int, uint32_t> textureMap;

    struct MaterialForGpu
    {
        uint32_t textureIndices[16]{};
        float parameters[16][4]{};
    };

    static std::vector<MaterialForGpu> materialsForGpu;
    static std::unordered_map<unsigned int, uint32_t> materialMap;

    textures.clear();
    textureMap.clear();
    materialsForGpu.clear();
    materialMap.clear();

    textures.push_back(bridge.nullTexture);
    textureMap.insert(std::make_pair(0u, 0u));

    for (auto& [id, material] : materials)
    {
        materialMap[id] = (uint32_t)materialsForGpu.size();

        auto& materialForGpu = materialsForGpu.emplace_back();
        for (size_t i = 0; i < 16; i++)
        {
            if (!material.textures[i])
                continue;

            const auto pair = textureMap.find(material.textures[i]);
            if (pair == textureMap.end())
            {
                materialForGpu.textureIndices[i] = (uint32_t)textures.size();
                const auto texPair = bridge.resources.find(material.textures[i]);
                textures.push_back(texPair != bridge.resources.end() ? nvrhi::checked_cast<nvrhi::ITexture*>(texPair->second.Get()) : bridge.nullTexture.Get());
            }
            else
                materialForGpu.textureIndices[i] = pair->second;
        }

        memcpy(materialForGpu.parameters, material.parameters, sizeof(material.parameters));
    }

    textureDescriptorTable = bridge.device.nvrhi->createDescriptorTable(textureBindlessLayout);
    bridge.device.nvrhi->resizeDescriptorTable(textureDescriptorTable, (uint32_t)textures.size(), false);

    for (uint32_t i = 0; i < textures.size(); i++)
        bridge.device.nvrhi->writeDescriptorTable(textureDescriptorTable, nvrhi::BindingSetItem::Texture_SRV(i, textures[i]));

    static std::vector<Geometry> geometriesForGpu;
    geometriesForGpu.clear();

    shaderTable = pipeline->createShaderTable();
    shaderTable->setRayGenerationShader("RayGeneration");

    shaderTable->addMissShader("MissSkyBox");
    shaderTable->addMissShader("MissEnvironmentColor");

    for (auto& [instanceId, instanceInfo] : bottomLevelAccelStructs)
    {
        for (auto it = instanceInfo.begin(); it != instanceInfo.end();)
        {
            if (it->second.geometries.empty())
            {
                it = instanceInfo.erase(it);
                continue;
            }

            if (!it->second.handle)
            {
                ++it;
                continue;
            }

            it->second.index = (uint32_t)geometriesForGpu.size();

            for (size_t i = 0; i < it->second.geometries.size(); i++)
            {
                auto& geometry = it->second.geometries[i];
                auto& geometryForGpu = geometriesForGpu.emplace_back(geometry);

                const auto materialPair = materialMap.find(geometry.material);
                if (materialPair != materialMap.end())
                {
                    geometryForGpu.material = materialPair->second;
                    shaderTable->addHitGroup(materials[geometry.material].shader);
                }
                else
                {
                    geometryForGpu.material = 0;
                    shaderTable->addHitGroup("SysError");
                }

                auto& skinnedGeometry = it->second.skinnedGeometries[i];

                if (!it->second.built && skinnedGeometry.nodeIndices)
                {
                    auto& vertexBuffer = it->second.desc.bottomLevelGeometries[i].geometryData.triangles.vertexBuffer;

                    if (!skinnedGeometry.buffer)
                    {
                        auto desc = vertexBuffer->getDesc();
                        skinnedGeometry.buffer = bridge.device.nvrhi->createBuffer(desc.setCanHaveUAVs(true));
                    }

                    bridge.commandList->writeBuffer(skinningConstantBuffer, &geometry, sizeof(Geometry));

                    auto skinningBindingSet = bridge.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
                        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, skinningConstantBuffer))
                        .addItem(nvrhi::BindingSetItem::RawBuffer_SRV(0, vertexBuffer))
                        .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(1, skinnedGeometry.nodeIndices))
                        .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(2, it->second.matrixBuffer))
                        .addItem(nvrhi::BindingSetItem::RawBuffer_UAV(0, skinnedGeometry.buffer)), skinningBindingLayout);

                    bridge.commandList->setComputeState(nvrhi::ComputeState()
                        .setPipeline(skinningPipeline)
                        .addBindingSet(skinningBindingSet));

                    bridge.commandList->dispatch((geometry.vertexCount + 63) / 64);

                    vertexBuffer = skinnedGeometry.buffer;
                }
            }

            if (!it->second.built)
            {
                nvrhi::utils::BuildBottomLevelAccelStruct(bridge.commandList, it->second.handle, it->second.desc);
                it->second.built = true;
            }

            ++it;
        }
    }

    geometryDescriptorTable = bridge.device.nvrhi->createDescriptorTable(geometryBindlessLayout);
    bridge.device.nvrhi->resizeDescriptorTable(geometryDescriptorTable, (uint32_t)(geometriesForGpu.size() * 2), false);

    uint32_t descriptorIndex = 0;

    for (auto& [instanceId, instanceInfo] : bottomLevelAccelStructs)
    {
        for (auto& [blasId, blas] : instanceInfo)
        {
            for (size_t i = 0; i < blas.geometries.size(); i++)
            {
                auto& geometry = blas.desc.bottomLevelGeometries[i];
                auto& skinnedGeometry = blas.skinnedGeometries[i];

                bridge.device.nvrhi->writeDescriptorTable(geometryDescriptorTable,
                    nvrhi::BindingSetItem::TypedBuffer_SRV(descriptorIndex++, geometry.geometryData.triangles.indexBuffer));

                bridge.device.nvrhi->writeDescriptorTable(geometryDescriptorTable,
                    nvrhi::BindingSetItem::RawBuffer_SRV(descriptorIndex++, skinnedGeometry.buffer ? skinnedGeometry.buffer.Get() : geometry.geometryData.triangles.vertexBuffer));
            }
        }
    }

    static std::vector<nvrhi::rt::InstanceDesc> instanceDescs;
    instanceDescs.clear();

    for (auto& instance : instances)
    {
        const auto blasPair = bottomLevelAccelStructs.find(instance.bottomLevelAS);
        if (blasPair == bottomLevelAccelStructs.end())
            continue;

        const auto instancePair = blasPair->second.find(instance.instanceInfo);
        if (instancePair == blasPair->second.end())
            continue;

        auto& desc = instanceDescs.emplace_back();
        memcpy(desc.transform, instance.transform, sizeof(desc.transform));
        desc.instanceID = instancePair->second.index;
        desc.instanceMask = instance.instanceMask;
        desc.instanceContributionToHitGroupIndex = instancePair->second.index;
        desc.bottomLevelAS = instancePair->second.handle;
    }

    if (instanceDescs.empty())
        return;

    auto topLevelAccelStruct = bridge.device.nvrhi->createAccelStruct(nvrhi::rt::AccelStructDesc()
        .setIsTopLevel(true)
        .setTrackLiveness(true)
        .setTopLevelMaxInstances(instanceDescs.size()));

    bridge.commandList->buildTopLevelAccelStruct(topLevelAccelStruct, instanceDescs.data(), instanceDescs.size());

    if (!output)
    {
        auto textureDesc = colorAttachment->getDesc();
        output = bridge.device.nvrhi->createTexture(textureDesc
            .setFormat(nvrhi::Format::RGBA16_FLOAT)
            .setIsRenderTarget(false)
            .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
            .setUseClearValue(false)
            .setIsUAV(true));

        upscaler->validate({ bridge, output });
    }

    bridge.vsConstants.writeBuffer(bridge.commandList, bridge.vsConstantBuffer);
    bridge.psConstants.writeBuffer(bridge.commandList, bridge.psConstantBuffer);

    upscaler->getJitterOffset(rtConstants.currentFrame, rtConstants.jitterX, rtConstants.jitterY);
    bridge.commandList->writeBuffer(rtConstantBuffer, &rtConstants, sizeof(rtConstants));

    memcpy(rtConstants.prevProj, bridge.vsConstants.c[0], sizeof(rtConstants.prevProj));
    memcpy(rtConstants.prevView, bridge.vsConstants.c[4], sizeof(rtConstants.prevView));

    createUploadBuffer(bridge, geometriesForGpu, geometryBuffer);
    createUploadBuffer(bridge, materialsForGpu, materialBuffer);

    auto bindingSet = bridge.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, bridge.vsConstantBuffer))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(1, bridge.psConstantBuffer))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(2, rtConstantBuffer))

        .addItem(nvrhi::BindingSetItem::RayTracingAccelStruct(0, topLevelAccelStruct))
        .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(1, materialBuffer))
        .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(2, geometryBuffer))

        .addItem(nvrhi::BindingSetItem::Texture_UAV(0, upscaler->color))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(1, upscaler->depth.getCurrent(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(2, upscaler->motionVector))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(3, upscaler->normal.getCurrent(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(4, upscaler->globalIllumination.getCurrent(rtConstants.currentFrame)))

        .addItem(nvrhi::BindingSetItem::Texture_SRV(4, upscaler->depth.getPrevious(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(5, upscaler->normal.getPrevious(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(6, upscaler->globalIllumination.getPrevious(rtConstants.currentFrame)))

        .addItem(nvrhi::BindingSetItem::Sampler(0, linearRepeatSampler)), bindingLayout);

    bridge.commandList->setRayTracingState(nvrhi::rt::State()
        .setShaderTable(shaderTable)
        .addBindingSet(bindingSet)
        .addBindingSet(geometryDescriptorTable)
        .addBindingSet(textureDescriptorTable));

    bridge.commandList->dispatchRays(nvrhi::rt::DispatchRaysArguments()
        .setWidth(upscaler->width)
        .setHeight(upscaler->height));

    if (!copyPipeline)
    {
        pointClampSampler = bridge.device.nvrhi->createSampler(nvrhi::SamplerDesc()
            .setAllFilters(false)
            .setAllAddressModes(nvrhi::SamplerAddressMode::Clamp));

        copyBindingLayout = bridge.device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
            .setVisibility(nvrhi::ShaderType::Pixel)
            .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
            .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1))
            .addItem(nvrhi::BindingLayoutItem::Sampler(0)));

        copyVertexShader = bridge.device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Vertex), COPY_VS, sizeof(COPY_VS));
        copyPixelShader = bridge.device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), COPY_PS, sizeof(COPY_PS));

        copyFramebuffer = bridge.device.nvrhi->createFramebuffer(bridge.framebufferDesc);

        copyPipeline = bridge.device.nvrhi->createGraphicsPipeline(nvrhi::GraphicsPipelineDesc()
            .setVertexShader(copyVertexShader)
            .setPixelShader(copyPixelShader)
            .setPrimType(nvrhi::PrimitiveType::TriangleList)
            .setRenderState(nvrhi::RenderState()
                .setDepthStencilState(nvrhi::DepthStencilState()
                    .enableDepthTest()
                    .enableDepthWrite()
                    .setDepthFunc(nvrhi::ComparisonFunc::Always)
                    .disableStencil())
                .setRasterState(nvrhi::RasterState()
                    .setCullNone()))
            .addBindingLayout(copyBindingLayout), copyFramebuffer);

        copyDrawArguments.setVertexCount(6);
    }

    upscaler->evaluate({ bridge, (size_t)rtConstants.currentFrame, rtConstants.jitterX, rtConstants.jitterY });

    auto copyBindingSet = bridge.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, output))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(1, upscaler->depth.getCurrent(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Sampler(0, pointClampSampler)), copyBindingLayout);;

    bridge.commandList->setGraphicsState(nvrhi::GraphicsState()
        .setPipeline(copyPipeline)
        .addBindingSet(copyBindingSet)
        .setFramebuffer(copyFramebuffer)
        .setViewport(nvrhi::ViewportState()
            .addViewportAndScissorRect(nvrhi::Viewport(
                (float)output->getDesc().width,
                (float)output->getDesc().height))));

    bridge.commandList->draw(copyDrawArguments);

    rtConstants.currentFrame += 1;
    instances.clear();
}