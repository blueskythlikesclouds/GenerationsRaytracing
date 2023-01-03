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

    auto& blas = bottomLevelAccelStructs[msg->bottomLevelAS];
    auto& blasInstance = blas.instances[msg->instanceInfo];

    auto& geometryDesc =
        blasInstance.desc.bottomLevelGeometries.size() > blasInstance.curGeometryIndex ?
        blasInstance.desc.bottomLevelGeometries[blasInstance.curGeometryIndex] :
        blasInstance.desc.bottomLevelGeometries.emplace_back();

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

    auto& geometry =
        blasInstance.geometries.size() > blasInstance.curGeometryIndex ?
        blasInstance.geometries[blasInstance.curGeometryIndex] :
        blasInstance.geometries.emplace_back();

    geometry.gpu.vertexCount = msg->vertexCount;
    geometry.gpu.vertexStride = msg->vertexStride;
    geometry.gpu.normalOffset = msg->normalOffset;
    geometry.gpu.tangentOffset = msg->tangentOffset;
    geometry.gpu.binormalOffset = msg->binormalOffset;
    geometry.gpu.texCoordOffset = msg->texCoordOffset;
    geometry.gpu.colorOffset = msg->colorOffset;
    geometry.gpu.colorFormat = msg->colorFormat;
    geometry.gpu.blendWeightOffset = msg->blendWeightOffset;
    geometry.gpu.blendIndicesOffset = msg->blendIndicesOffset;
    geometry.gpu.material = msg->material;
    geometry.gpu.punchThrough = msg->punchThrough;

    if (geometry.vertexBuffer != triangles.vertexBuffer ||
        geometry.indexBuffer != triangles.indexBuffer)
    {
        geometry.skinningBuffer = nullptr;
        blasInstance.handle = nullptr;
    }

    geometry.vertexBuffer = triangles.vertexBuffer;
    geometry.indexBuffer = triangles.indexBuffer;

    if (msg->nodeNum > 0)
    {
        auto& nodeIndicesBuffer = blas.nodeIndicesBuffers[XXH64(data, msg->nodeNum, 0)];
        if (!nodeIndicesBuffer)
        {
            nodeIndicesBuffer = bridge.device.nvrhi->createBuffer(nvrhi::BufferDesc()
                .setByteSize(msg->nodeNum)
                .setFormat(nvrhi::Format::R8_UINT)
                .setCanHaveTypedViews(true)
                .setInitialState(nvrhi::ResourceStates::CopyDest)
                .setKeepInitialState(true));

            bridge.openCommandListForCopy();
            bridge.commandListForCopy->writeBuffer(nodeIndicesBuffer, data, msg->nodeNum);
            bridge.commandList->setPermanentBufferState(nodeIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
        }

        geometry.nodeIndicesBuffer = nodeIndicesBuffer;
    }
    else
    {
        geometry.nodeIndicesBuffer = nullptr;
    }

    blasInstance.curGeometryIndex += 1;
}

void RaytracingBridge::procMsgCreateBottomLevelAS(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateBottomLevelAS>();

    const size_t matrixByteSize = msg->matrixNum * 64;
    const void* data = bridge.msgReceiver.getDataAndMoveNext(matrixByteSize);

    const auto blasPair = bottomLevelAccelStructs.find(msg->bottomLevelAS);
    if (blasPair == bottomLevelAccelStructs.end())
        return;

    const auto instancePair = blasPair->second.instances.find(msg->instanceInfo);
    if (instancePair == blasPair->second.instances.end())
        return;

    if (msg->matrixNum > 0)
    {
        if (instancePair->second.matrixBuffer == nullptr ||
            instancePair->second.matrixBuffer->getDesc().byteSize != matrixByteSize)
        {
            instancePair->second.matrixBuffer = bridge.device.nvrhi->createBuffer(nvrhi::BufferDesc()
                .setByteSize(matrixByteSize)
                .setStructStride(64)
                .setCpuAccess(nvrhi::CpuAccessMode::Write));

            instancePair->second.matrixHash = 0;
        }

        const XXH64_hash_t matrixHash = XXH64(data, matrixByteSize, 0);
        if (matrixHash != instancePair->second.matrixHash)
        {
            void* memory = bridge.device.nvrhi->mapBuffer(instancePair->second.matrixBuffer, nvrhi::CpuAccessMode::Write);
            memcpy(memory, data, matrixByteSize);
            bridge.device.nvrhi->unmapBuffer(instancePair->second.matrixBuffer);

            instancePair->second.handle = nullptr;
            instancePair->second.matrixHash = matrixHash;
        }
    }

    instancePair->second.desc.bottomLevelGeometries.resize(instancePair->second.curGeometryIndex);
    instancePair->second.geometries.resize(instancePair->second.curGeometryIndex);
    instancePair->second.curGeometryIndex = 0;
}

void RaytracingBridge::procMsgCreateInstance(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateInstance>();

    auto& instance = instances.emplace_back();
    memcpy(instance.transform, msg->transform, sizeof(instance.transform));
    instance.bottomLevelAS = msg->bottomLevelAS;
    instance.instanceInfo = msg->instanceInfo;
    instance.instanceMask = msg->instanceMask;
}

void RaytracingBridge::procMsgCreateMaterial(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateMaterial>();

    auto& material = materials[msg->material];

    strcpy(material.shader, msg->shader);
    memcpy(material.gpu.textures, msg->textures, sizeof(msg->textures));
    memcpy(material.gpu.parameters, msg->parameters, sizeof(msg->parameters));
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
            .setByteSize(sizeof(BottomLevelAS::Geometry::GPU))
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

    static std::vector<MaterialForGpu> gpuMaterials;
    static std::unordered_map<unsigned int, uint32_t> materialMap;

    textures.clear();
    textureMap.clear();
    gpuMaterials.clear();
    materialMap.clear();

    textures.push_back(bridge.nullTexture);
    textureMap.insert(std::make_pair(0u, 0u));

    for (auto& [id, material] : materials)
    {
        materialMap[id] = (uint32_t)gpuMaterials.size();

        auto& gpuMaterial = gpuMaterials.emplace_back();
        for (size_t i = 0; i < 16; i++)
        {
            if (!material.gpu.textures[i])
                continue;

            const auto pair = textureMap.find(material.gpu.textures[i]);
            if (pair == textureMap.end())
            {
                gpuMaterial.textureIndices[i] = (uint32_t)textures.size();
                const auto texPair = bridge.resources.find(material.gpu.textures[i]);
                textures.push_back(texPair != bridge.resources.end() ? nvrhi::checked_cast<nvrhi::ITexture*>(texPair->second.Get()) : bridge.nullTexture.Get());
            }
            else
                gpuMaterial.textureIndices[i] = pair->second;
        }

        memcpy(gpuMaterial.parameters, material.gpu.parameters, sizeof(material.gpu.parameters));
    }

    textureDescriptorTable = bridge.device.nvrhi->createDescriptorTable(textureBindlessLayout);
    bridge.device.nvrhi->resizeDescriptorTable(textureDescriptorTable, (uint32_t)textures.size(), false);

    for (uint32_t i = 0; i < textures.size(); i++)
        bridge.device.nvrhi->writeDescriptorTable(textureDescriptorTable, nvrhi::BindingSetItem::Texture_SRV(i, textures[i]));

    static std::vector<BottomLevelAS::Geometry::GPU> gpuGeometries;
    gpuGeometries.clear();

    shaderTable = pipeline->createShaderTable();
    shaderTable->setRayGenerationShader("RayGeneration");

    shaderTable->addMissShader("MissSkyBox");
    shaderTable->addMissShader("MissEnvironmentColor");

    for (auto& [id, blas] : bottomLevelAccelStructs)
    {
        for (auto it = blas.instances.begin(); it != blas.instances.end();)
        {
            if (it->second.geometries.empty())
            {
                it = blas.instances.erase(it);
                continue;
            }

            it->second.indexInBuffer = (uint32_t)gpuGeometries.size();

            for (size_t i = 0; i < it->second.geometries.size(); i++)
            {
                auto& geometry = it->second.geometries[i];
                auto& gpuGeometry = gpuGeometries.emplace_back(geometry.gpu);

                const auto materialPair = materialMap.find(geometry.gpu.material);
                if (materialPair != materialMap.end())
                {
                    gpuGeometry.material = materialPair->second;
                    shaderTable->addHitGroup(materials[geometry.gpu.material].shader);
                }
                else
                {
                    gpuGeometry.material = 0;
                    shaderTable->addHitGroup("SysError");
                }

                if (it->second.handle == nullptr && geometry.nodeIndicesBuffer != nullptr)
                {
                    if (geometry.skinningBuffer == nullptr)
                    {
                        auto desc = geometry.vertexBuffer->getDesc();
                        geometry.skinningBuffer = bridge.device.nvrhi->createBuffer(desc.setCanHaveUAVs(true));
                    }

                    bridge.commandList->writeBuffer(skinningConstantBuffer, &geometry.gpu, sizeof(BottomLevelAS::Geometry::GPU));

                    auto skinningBindingSet = bridge.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
                        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, skinningConstantBuffer))
                        .addItem(nvrhi::BindingSetItem::RawBuffer_SRV(0, geometry.vertexBuffer))
                        .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(1, geometry.nodeIndicesBuffer))
                        .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(2, it->second.matrixBuffer))
                        .addItem(nvrhi::BindingSetItem::RawBuffer_UAV(0, geometry.skinningBuffer)), skinningBindingLayout);

                    bridge.commandList->setComputeState(nvrhi::ComputeState()
                        .setPipeline(skinningPipeline)
                        .addBindingSet(skinningBindingSet));

                    bridge.commandList->dispatch((geometry.gpu.vertexCount + 63) / 64);

                    it->second.desc.bottomLevelGeometries[i].geometryData.triangles.vertexBuffer = geometry.skinningBuffer;
                }
            }

            if (it->second.handle == nullptr)
            {
                it->second.handle = bridge.device.nvrhi->createAccelStruct(it->second.desc
                    .setBuildFlags(it->second.matrixBuffer != nullptr ? nvrhi::rt::AccelStructBuildFlags::PreferFastBuild : nvrhi::rt::AccelStructBuildFlags::PreferFastTrace));

                nvrhi::utils::BuildBottomLevelAccelStruct(bridge.commandList, it->second.handle, it->second.desc);
            }

            ++it;
        }
    }

    geometryDescriptorTable = bridge.device.nvrhi->createDescriptorTable(geometryBindlessLayout);
    bridge.device.nvrhi->resizeDescriptorTable(geometryDescriptorTable, (uint32_t)(gpuGeometries.size() * 2), false);

    uint32_t descriptorIndex = 0;

    for (auto& [blasId, blas] : bottomLevelAccelStructs)
    {
        for (auto& [instanceId, instance] : blas.instances)
        {
            for (auto& geometry : instance.geometries)
            {
                bridge.device.nvrhi->writeDescriptorTable(geometryDescriptorTable,
                    nvrhi::BindingSetItem::TypedBuffer_SRV(descriptorIndex++, geometry.indexBuffer));

                bridge.device.nvrhi->writeDescriptorTable(geometryDescriptorTable,
                    nvrhi::BindingSetItem::RawBuffer_SRV(descriptorIndex++, geometry.skinningBuffer != nullptr ? geometry.skinningBuffer : geometry.vertexBuffer));
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

        const auto instancePair = blasPair->second.instances.find(instance.instanceInfo);
        if (instancePair == blasPair->second.instances.end())
            continue;

        auto& desc = instanceDescs.emplace_back();
        memcpy(desc.transform, instance.transform, sizeof(desc.transform));
        desc.instanceID = instancePair->second.indexInBuffer;
        desc.instanceMask = instance.instanceMask;
        desc.instanceContributionToHitGroupIndex = instancePair->second.indexInBuffer;
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

    createUploadBuffer(bridge, gpuGeometries, geometryBuffer);
    createUploadBuffer(bridge, gpuMaterials, materialBuffer);

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