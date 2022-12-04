#include "Scene.h"

#include "ArchiveDatabase.h"
#include "Device.h"
#include "Path.h"
#include "ShaderMapping.h"
#include "Utilities.h"

struct SceneLoader
{
    Scene& scene;

    std::unordered_map<std::string, uint32_t> textureMap;
    std::unordered_map<std::string, uint32_t> materialMap;
    std::unordered_map<std::string, uint32_t> modelMap;

    SceneLoader(Scene& scene)
        : scene(scene)
    {
    }

    void loadTexture(const void* data, size_t dataSize)
    {
        Texture& texture = scene.cpu.textures.emplace_back();

        texture.data = std::make_unique<uint8_t[]>(dataSize);
        texture.dataSize = dataSize;

        memcpy(texture.data.get(), data, dataSize);
    }

    void loadMaterial(void* data)
    {
        hl::hh::mirage::fix(data);

        auto material = hl::hh::mirage::get_data<hl::hh::mirage::raw_material_v3>(data);
        material->fix();

        Material& newMaterial = scene.cpu.materials.emplace_back();
        newMaterial.shader = material->shaderName.get();

        for (hl::u8 i = 0; i < material->float4ParamCount; i++)
        {
            const auto& param = material->float4Params[i];
            newMaterial.parameters.emplace_back(param->name.get(), Eigen::Vector4f(param->values[0].x, param->values[0].y, param->values[0].z, param->values[0].w));
        }

        for (hl::u8 i = 0; i < material->textureEntryCount; i++)
        {
            const auto& texture = material->textureEntries[i];
            newMaterial.textures.emplace_back(texture->type.get(), textureMap[texture->texName.get()]);
        }
    }

    void loadMesh(const hl::hh::mirage::raw_mesh* mesh, Mesh::Type type)
    {
        Mesh newMesh;

        newMesh.type = type;
        newMesh.vertexOffset = (uint32_t)scene.cpu.vertices.size();
        newMesh.vertexCount = mesh->vertexCount;

        for (hl::u32 i = 0; i < mesh->vertexCount; i++)
        {
            auto& position = scene.cpu.vertices.emplace_back(0.0f, 0.0f, 0.0f);
            auto& normal = scene.cpu.normals.emplace_back(0.0f, 1.0f, 0.0f);
            auto& tangent = scene.cpu.tangents.emplace_back(1.0f, 0.0f, 0.0f, 1.0f);
            auto& texCoord = scene.cpu.texCoords.emplace_back(0.0f, 0.0f);
            auto& color = scene.cpu.colors.emplace_back(1.0f, 1.0f, 1.0f, 1.0f);

            for (hl::u32 j = 0; ; j++)
            {
                const hl::hh::mirage::raw_vertex_element& element = mesh->vertexElements[j];

                if (element.stream == 0xFF)
                    break;

                if (element.index > 0)
                    continue;

                hl::vec4 value;
                element.convert_to_vec4((char*)mesh->vertices.get() + i * mesh->vertexSize + element.offset, value);

                switch (element.type)
                {
                case hl::hh::mirage::raw_vertex_type::position:
                    position.x() = value.x;
                    position.y() = value.y;
                    position.z() = value.z;
                    break;

                case hl::hh::mirage::raw_vertex_type::normal:
                    normal.x() = value.x;
                    normal.y() = value.y;
                    normal.z() = value.z;
                    break;

                case hl::hh::mirage::raw_vertex_type::tangent:
                    tangent.x() = value.x;
                    tangent.y() = value.y;
                    tangent.z() = value.z;
                    break;

                case hl::hh::mirage::raw_vertex_type::binormal:
                {
                    const Eigen::Vector3f binormal(value.x, value.y, value.z);
                    tangent.w() = normal.cross(tangent.head<3>()).dot(binormal) > 0.0f ? 1.0f : -1.0f;
                    break;
                }

                case hl::hh::mirage::raw_vertex_type::texcoord:
                    texCoord.x() = value.x;
                    texCoord.y() = value.y;
                    break;

                case hl::hh::mirage::raw_vertex_type::color:
                    color.x() = value.x;
                    color.y() = value.y;
                    color.z() = value.z;
                    color.w() = value.w;
                    break;
                }
            }
        }

        newMesh.indexOffset = (uint32_t)scene.cpu.indices.size();

        hl::u32 index = 0;
        hl::u16 a = mesh->faces.data()[index++];
        hl::u16 b = mesh->faces.data()[index++];
        int direction = -1;

        while (index < mesh->faces.count)
        {
            hl::u16 c = mesh->faces.data()[index++];

            if (c == 0xFFFF)
            {
                a = mesh->faces.data()[index++];
                b = mesh->faces.data()[index++];
                direction = -1;
            }
            else
            {
                direction *= -1;

                if (a != b && b != c && c != a)
                {
                    scene.cpu.indices.push_back(a);

                    if (direction > 0)
                    {
                        scene.cpu.indices.push_back(b);
                        scene.cpu.indices.push_back(c);
                    }
                    else
                    {
                        scene.cpu.indices.push_back(c);
                        scene.cpu.indices.push_back(b);
                    }
                }

                a = b;
                b = c;
            }
        }

        newMesh.indexCount = (uint32_t)(scene.cpu.indices.size() - newMesh.indexOffset);
        newMesh.materialIndex = materialMap[mesh->materialName.get()];

        if (newMesh.vertexCount > 0 && newMesh.indexCount > 0)
            scene.cpu.meshes.push_back(std::move(newMesh));
    }

    void loadMeshGroup(const hl::hh::mirage::raw_mesh_group* group)
    {
        for (const auto& mesh : group->opaq) loadMesh(mesh.get(), Mesh::Type::Opaque);
        for (const auto& mesh : group->trans) loadMesh(mesh.get(), Mesh::Type::Trans);
        for (const auto& mesh : group->punch) loadMesh(mesh.get(), Mesh::Type::Punch);

        for (const auto& special : group->special)
        {
            for (const auto& mesh : special)
                loadMesh(mesh.get(), Mesh::Type::Special);
        }
    }

    void loadModel(void* data)
    {
        hl::hh::mirage::fix(data);

        auto model = hl::hh::mirage::get_data<hl::hh::mirage::raw_terrain_model_v5>(data);
        model->fix();

        Model newModel;
        newModel.meshOffset = (uint32_t)scene.cpu.meshes.size();

        for (const auto& group : model->meshGroups)
            loadMeshGroup(group.get());

        newModel.meshCount = (uint32_t)(scene.cpu.meshes.size() - newModel.meshOffset);

        if (newModel.meshCount > 0)
        {
            modelMap.emplace(model->name.get(), (uint32_t)scene.cpu.models.size());
            scene.cpu.models.push_back(std::move(newModel));
        }
    }

    void loadInstance(void* data)
    {
        hl::hh::mirage::fix(data);

        auto instance = hl::hh::mirage::get_data<hl::hh::mirage::raw_terrain_instance_info_v0>(data);
        instance->fix();

        Instance& newInstance = scene.cpu.instances.emplace_back();

        assert(sizeof(Eigen::Matrix4f) == sizeof(hl::matrix4x4));
        memcpy(&newInstance.transform, instance->matrix.get(), sizeof(Eigen::Matrix4f));

        newInstance.modelIndex = modelMap[instance->modelName.get()];
    }

    void loadTerrainGroup(void* data, size_t dataSize)
    {
        hl::archive group = ArchiveDatabase::load(data, dataSize);

        for (auto& file : group)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT(".terrain-model")))
            {
                loadModel(file.file_data());
            }
        }

        for (auto& file : group)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT(".terrain-instanceinfo")))
            {
                loadInstance(file.file_data());
            }
        }
    }

    void loadLight(void* data)
    {
        hl::hh::mirage::fix(data);

        auto light = hl::hh::mirage::get_data<hl::hh::mirage::raw_light_v1>(data);
        light->fix();

        if (light->type == hl::hh::mirage::raw_light_type::directional)
        {
            memcpy(&scene.cpu.lightDirection, &light->directionalData.dir, sizeof(light->directionalData.dir));
            memcpy(&scene.cpu.lightColor, &light->directionalData.color, sizeof(light->directionalData.color));
        }
    }

    void load(const std::string& directoryPath)
    {
        const std::string name = getFileName(directoryPath);

        hl::archive resources = ArchiveDatabase::load(directoryPath + "/" + name + ".ar.00");

        for (const auto& file : resources)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT(".dds")) || hl::text::strstr(file.name(), HL_NTEXT(".DDS")))
            {
                auto name = fromNchar(file.name());
                *strchr(name.data(), '.') = '\0';

                textureMap.emplace(name.data(), (uint32_t)scene.cpu.textures.size());
                loadTexture(file.file_data(), file.size());
            }
        }

    	for (auto& file : resources)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT(".material")))
            {
                auto name = fromNchar(file.name());
                *strchr(name.data(), '.') = '\0';

                materialMap.emplace(name.data(), (uint32_t)scene.cpu.materials.size());
                loadMaterial(file.file_data());
            }
        }

    	for (auto& file : resources)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT(".light")) && !hl::text::strstr(file.name(), HL_NTEXT(".light-list")))
	            loadLight(file.file_data());
        }

        hl::archive stage = ArchiveDatabase::load(directoryPath + "/Stage.pfd");

        for (auto& file : stage)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT("tg-")))
                loadTerrainGroup(file.file_data(), file.size());
        }
    }
};

void Scene::loadCpuResources(const std::string& directoryPath)
{
    SceneLoader loader(*this);
    loader.load(directoryPath);
}

void Scene::createGpuResources(const Device& device, const ShaderMapping& shaderMapping)
{
    std::vector<std::vector<D3D12_SUBRESOURCE_DATA>> subResourcesPerTexture;

    for (const auto& texture : cpu.textures)
    {
        DirectX::LoadDDSTextureFromMemory(
            device.nvrhi,
            texture.data.get(),
            texture.dataSize,
            std::addressof(gpu.textures.emplace_back()),
            subResourcesPerTexture.emplace_back());
    }

    std::vector<Material::GPU> materialBuffer;

    for (auto& material : cpu.materials)
    {
        auto& gpuMaterial = materialBuffer.emplace_back();

        const auto shaderPair = shaderMapping.map.find(material.shader);

        if (shaderPair != shaderMapping.map.end())
        {
            for (auto& paramPair : material.parameters)
            {
                for (size_t i = 0; i < shaderPair->second.parameters.size(); i++)
                {
                    if (paramPair.first == shaderPair->second.parameters[i])
                    {
                        gpuMaterial.parameters[i] = paramPair.second;
                        break;
                    }
                }
            }

            std::unordered_map<std::string, size_t> counts;

            for (auto& texPair : material.textures)
            {
                const size_t target = counts[texPair.first];
                size_t current = 0;

                for (size_t i = 0; i < shaderPair->second.textures.size(); i++)
                {
                    if (texPair.first == shaderPair->second.textures[i] && (current++) == target)
                    {
                        gpuMaterial.textureIndices[i] = texPair.second;
                        ++counts[texPair.first];
                        break;
                    }
                }
            }
        }
        else
        {
            material.shader = "SysError";
        }
    }

    gpu.materialBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(materialBuffer))
        .setStructStride(vectorByteStride(materialBuffer))
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    std::vector<Mesh::GPU> meshBuffer;

    for (const auto& mesh : cpu.meshes)
        meshBuffer.push_back({ mesh.vertexOffset, mesh.indexOffset, mesh.materialIndex, mesh.type });

    gpu.meshBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(meshBuffer))
        .setFormat(nvrhi::Format::RGBA32_UINT)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    gpu.vertexBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.vertices))
        .setFormat(nvrhi::Format::RGB32_FLOAT)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true)
        .setIsAccelStructBuildInput(true));

    gpu.normalBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.normals))
        .setFormat(nvrhi::Format::RGB32_FLOAT)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    gpu.tangentBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.tangents))
        .setFormat(nvrhi::Format::RGBA32_FLOAT)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    gpu.texCoordBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.texCoords))
        .setFormat(nvrhi::Format::RG32_FLOAT)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    gpu.colorBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.colors))
        .setFormat(nvrhi::Format::RGBA32_FLOAT)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    gpu.indexBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.indices))
        .setFormat(nvrhi::Format::R16_UINT)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true)
        .setIsAccelStructBuildInput(true));

    std::vector<nvrhi::rt::AccelStructDesc> blasDescs;
    std::vector<nvrhi::rt::InstanceDesc> instanceDescs;

    for (const auto& model : cpu.models)
    {
        auto& blasDesc = blasDescs.emplace_back();

        for (size_t i = 0; i < model.meshCount; i++)
        {
            const Mesh& mesh = cpu.meshes[model.meshOffset + i];

            auto& meshDesc = blasDesc.bottomLevelGeometries.emplace_back();
            auto& triangles = meshDesc.geometryData.triangles;

            triangles.indexBuffer = gpu.indexBuffer;
            triangles.vertexBuffer = gpu.vertexBuffer;
            triangles.indexFormat = nvrhi::Format::R16_UINT;
            triangles.vertexFormat = nvrhi::Format::RGB32_FLOAT;
            triangles.indexOffset = vectorByteOffset(cpu.indices, mesh.indexOffset);
            triangles.vertexOffset = vectorByteOffset(cpu.vertices, mesh.vertexOffset);
            triangles.indexCount = mesh.indexCount;
            triangles.vertexCount = mesh.vertexCount;
            triangles.vertexStride = (uint32_t)vectorByteStride(cpu.vertices);
        }

        gpu.bottomLevelAccelStructs.push_back(device.nvrhi->createAccelStruct(blasDesc));
    }

    for (const auto& instance : cpu.instances)
    {
        auto& instanceDesc = instanceDescs.emplace_back();

        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < 4; j++)
                instanceDesc.transform[i * 4 + j] = instance.transform(j, i);
        }

        instanceDesc.instanceID = cpu.models[instance.modelIndex].meshOffset;
        instanceDesc.instanceMask = 1;
        instanceDesc.instanceContributionToHitGroupIndex = instanceDesc.instanceID;
        instanceDesc.bottomLevelAS = gpu.bottomLevelAccelStructs[instance.modelIndex];
    }

    gpu.topLevelAccelStruct = device.nvrhi->createAccelStruct(nvrhi::rt::AccelStructDesc()
        .setIsTopLevel(true)
        .setTopLevelMaxInstances(instanceDescs.size()));

    auto commandList = device.nvrhi->createCommandList();

    commandList->open();

    for (size_t i = 0; i < subResourcesPerTexture.size(); i++)
    {
        const auto& subResources = subResourcesPerTexture[i];
        const auto& texture = gpu.textures[i];

        for (size_t j = 0; j < subResources.size(); j++)
        {
            const auto& subResource = subResources[j];

            commandList->writeTexture(
                gpu.textures[i],
                j / texture->getDesc().mipLevels,
                j % texture->getDesc().mipLevels,
                subResource.pData,
                subResource.RowPitch,
                subResource.SlicePitch);
        }
    }

    commandList->writeBuffer(gpu.materialBuffer, materialBuffer.data(), vectorByteSize(materialBuffer));
    commandList->writeBuffer(gpu.meshBuffer, meshBuffer.data(), vectorByteSize(meshBuffer));
    commandList->writeBuffer(gpu.vertexBuffer, cpu.vertices.data(), vectorByteSize(cpu.vertices));
    commandList->writeBuffer(gpu.normalBuffer, cpu.normals.data(), vectorByteSize(cpu.normals));
    commandList->writeBuffer(gpu.tangentBuffer, cpu.tangents.data(), vectorByteSize(cpu.tangents));
    commandList->writeBuffer(gpu.texCoordBuffer, cpu.texCoords.data(), vectorByteSize(cpu.texCoords));
    commandList->writeBuffer(gpu.colorBuffer, cpu.colors.data(), vectorByteSize(cpu.colors));
    commandList->writeBuffer(gpu.indexBuffer, cpu.indices.data(), vectorByteSize(cpu.indices));

    for (size_t i = 0; i < blasDescs.size(); i++)
        nvrhi::utils::BuildBottomLevelAccelStruct(commandList, gpu.bottomLevelAccelStructs[i], blasDescs[i]);

    commandList->buildTopLevelAccelStruct(gpu.topLevelAccelStruct, instanceDescs.data(), instanceDescs.size());
    commandList->close();

    device.nvrhi->executeCommandList(commandList);
    device.nvrhi->waitForIdle();
}
