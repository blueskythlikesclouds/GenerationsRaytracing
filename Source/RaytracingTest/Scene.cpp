#include "Scene.h"

#include "App.h"
#include "ArchiveDatabase.h"
#include "Math.h"
#include "Path.h"
#include "ShaderDefinitions.hlsli"
#include "Utilities.h"

struct SceneLoader
{
    Scene& scene;

    std::unordered_map<std::string, uint32_t> textureMap;
    std::unordered_map<std::string, uint32_t> materialMap;
    std::unordered_map<std::string, std::vector<uint32_t>> modelMap;

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
            newMaterial.parameters.emplace_back(param->name.get(), Eigen::Vector4f(
                param->values[0].x,
                param->values[0].y, 
                param->values[0].z, 
                strcmp(param->name.get(), "diffuse") == 0 ? 1.0f : param->values[0].w));
        }

        for (hl::u8 i = 0; i < material->textureEntryCount; i++)
        {
            const auto& texture = material->textureEntries[i];
            newMaterial.textures.emplace_back(texture->type.get(), textureMap[texture->texName.get()]);
        }
    }

    void loadMesh(const hl::hh::mirage::raw_mesh* mesh, uint32_t type)
    {
        Mesh newMesh;

        newMesh.type = type;
        newMesh.vertexOffset = (uint32_t)scene.cpu.vertices.size();
        newMesh.vertexCount = mesh->vertexCount;

        for (hl::u32 i = 0; i < mesh->vertexCount; i++)
        {
            Eigen::Vector3f position = { 0.0f, 0.0f, 0.0f };
            Eigen::Vector3f normal = { 0.0f, 1.0f, 0.0f };
            Eigen::Vector4f tangent = { 1.0f, 0.0f, 0.0f, 1.0f };
            Eigen::Vector2f texCoord = { 0.0f, 0.0f };
            Eigen::Vector4f color = { 1.0f, 1.0f, 1.0f, 1.0f };

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

                case hl::hh::mirage::raw_vertex_type::binormal:
                {
                    const Eigen::Vector3f binormal(value.x, value.y, value.z);
                    tangent.w() = normal.cross(tangent.head<3>()).dot(binormal) > 0.0f ? 1.0f : -1.0f;
                    break;
                }

                }
            }

            normal.normalize();
            tangent.head<3>().normalize();

            scene.cpu.vertices.push_back(position);

            scene.cpu.normals.push_back(
                (quantizeSint8(normal.x()) << 0) |
                (quantizeSint8(normal.y()) << 8) |
                (quantizeSint8(normal.z()) << 16)
            );

            scene.cpu.tangents.push_back(
                (quantizeSint8(tangent.x()) << 0) |
                (quantizeSint8(tangent.y()) << 8) |
                (quantizeSint8(tangent.z()) << 16) |
                (quantizeSint8(tangent.w()) << 24)
            );

            scene.cpu.texCoords.push_back(
                (quantizeHalf(texCoord.x()) << 0) |
                (quantizeHalf(texCoord.y()) << 16) 
            );

            scene.cpu.colors.push_back(
                (quantizeUint8(color.x()) << 0) |
                (quantizeUint8(color.y()) << 8) |
                (quantizeUint8(color.z()) << 16) |
                (quantizeUint8(color.w()) << 24) 
            );
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

    void loadMeshGroup(const hl::hh::mirage::raw_mesh_group* group, uint32_t types)
    {
        if (types & MESH_TYPE_OPAQUE)
        {
            for (const auto& mesh : group->opaq) 
                loadMesh(mesh.get(), MESH_TYPE_OPAQUE);
        }

        if (types & MESH_TYPE_TRANS)
        {
            for (const auto& mesh : group->trans) 
                loadMesh(mesh.get(), MESH_TYPE_TRANS);
        }

        if (types & MESH_TYPE_PUNCH)
        {
            for (const auto& mesh : group->punch)
                loadMesh(mesh.get(), MESH_TYPE_PUNCH);
        }

        if (types & MESH_TYPE_SPECIAL)
        {
            for (const auto& special : group->special)
            {
                for (const auto& mesh : special)
                    loadMesh(mesh.get(), MESH_TYPE_SPECIAL);
            }
        }
    }

    void loadTerrainModel(void* data)
    {
        hl::hh::mirage::fix(data);

        auto model = hl::hh::mirage::get_data<hl::hh::mirage::raw_terrain_model_v5>(data);
        model->fix();

        for (auto& types : { MESH_TYPE_OPAQUE | MESH_TYPE_PUNCH, MESH_TYPE_TRANS | MESH_TYPE_SPECIAL })
        {
            Model newModel;
            newModel.meshOffset = (uint32_t)scene.cpu.meshes.size();

            for (const auto& group : model->meshGroups)
                loadMeshGroup(group.get(), types);

            newModel.meshCount = (uint32_t)(scene.cpu.meshes.size() - newModel.meshOffset);

            if (newModel.meshCount > 0)
            {
                if (types & (MESH_TYPE_OPAQUE | MESH_TYPE_PUNCH))
                    newModel.instanceMask = INSTANCE_MASK_OPAQUE_OR_PUNCH;

                else if (types & (MESH_TYPE_TRANS | MESH_TYPE_SPECIAL))
                    newModel.instanceMask = INSTANCE_MASK_TRANS_OR_SPECIAL;

                modelMap[model->name.get()].push_back((uint32_t)scene.cpu.models.size());
                scene.cpu.models.push_back(std::move(newModel));
            }
        }
    }

    void loadModel(void* data)
    {
        hl::hh::mirage::fix(data);

        auto model = hl::hh::mirage::get_data<hl::hh::mirage::raw_skeletal_model_v5>(data);
        model->fix();

        Model newModel;
        newModel.meshOffset = (uint32_t)scene.cpu.meshes.size();

        for (const auto& group : model->meshGroups)
            loadMeshGroup(group.get(), ~0);

        newModel.meshCount = (uint32_t)(scene.cpu.meshes.size() - newModel.meshOffset);

        if (newModel.meshCount > 0)
        {
            for (size_t i = 0; i < newModel.meshCount; i++)
            {
                if (scene.cpu.materials[scene.cpu.meshes[newModel.meshOffset + i].materialIndex].shader.find("Sky") != std::string::npos)
                {
                    newModel.instanceMask = INSTANCE_MASK_SKY;
                    break;
                }
            }

            if (newModel.instanceMask == INSTANCE_MASK_SKY)
            {
                scene.cpu.instances.emplace_back().modelIndex = (uint32_t)scene.cpu.models.size();
                scene.cpu.models.push_back(std::move(newModel));
            }
        }
    }

    void loadInstance(void* data)
    {
        hl::hh::mirage::fix(data);

        auto instance = hl::hh::mirage::get_data<hl::hh::mirage::raw_terrain_instance_info_v0>(data);
        instance->fix();

        auto& modelIndices = modelMap[instance->modelName.get()];

        for (const auto modelIndex : modelIndices)
        {
            Instance& newInstance = scene.cpu.instances.emplace_back();

            assert(sizeof(Eigen::Matrix4f) == sizeof(hl::matrix4x4));
            memcpy(&newInstance.transform, instance->matrix.get(), sizeof(Eigen::Matrix4f));

            newInstance.modelIndex = modelIndex;
        }
    }

    void loadTerrainGroup(void* data, size_t dataSize)
    {
        hl::archive group = ArchiveDatabase::load(data, dataSize);

        for (auto& file : group)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT(".terrain-model")))
            {
                loadTerrainModel(file.file_data());
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
            scene.cpu.globalLight.position.x() = light->directionalData.dir.x;
            scene.cpu.globalLight.position.y() = light->directionalData.dir.y;
            scene.cpu.globalLight.position.z() = light->directionalData.dir.z;
            scene.cpu.globalLight.position.normalize();

            scene.cpu.globalLight.color.x() = light->directionalData.color.x;
            scene.cpu.globalLight.color.y() = light->directionalData.color.y;
            scene.cpu.globalLight.color.z() = light->directionalData.color.z;
        }
        else
        {
            auto& omniLight = scene.cpu.localLights.emplace_back();

            omniLight.position.x() = light->pointData.pos.x;
            omniLight.position.y() = light->pointData.pos.y;
            omniLight.position.z() = light->pointData.pos.z;
            omniLight.outerRadius = light->pointData.range.z;

            omniLight.color.x() = light->pointData.color.x;
            omniLight.color.y() = light->pointData.color.y;
            omniLight.color.z() = light->pointData.color.z;
            omniLight.innerRadius = light->pointData.range.w;
        }
    }

    void loadSceneEffect(const void* data, size_t dataSize)
    {
        tinyxml2::XMLDocument document;

        if (document.Parse((const char*)data, dataSize) == tinyxml2::XML_SUCCESS)
            scene.cpu.effect.load(document);
    }

    void load(const std::string& directoryPath)
    {
        const std::string name = getFileName(directoryPath);

        {
            hl::archive stage = ArchiveDatabase::load(directoryPath + "/../../#" + name + ".ar.00");

            for (const auto& file : stage)
            {
                if (hl::text::equal(file.name(), HL_NTEXT("SceneEffect.prm.xml")))
                {
                    loadSceneEffect(file.file_data(), file.size());
                    break;
                }
            }
        }

        hl::archive resources = ArchiveDatabase::load(directoryPath + "/" + name + ".ar.00");

        for (const auto& file : resources)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT(".dds")) || hl::text::strstr(file.name(), HL_NTEXT(".DDS")))
            {
                auto name = fromNchar(file.name());
                *strrchr(name.data(), '.') = '\0';

                textureMap.emplace(name.data(), (uint32_t)scene.cpu.textures.size());
                loadTexture(file.file_data(), file.size());
            }
        }

        for (auto& file : resources)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT(".material")))
            {
                auto name = fromNchar(file.name());
                *strrchr(name.data(), '.') = '\0';

                materialMap.emplace(name.data(), (uint32_t)scene.cpu.materials.size());
                loadMaterial(file.file_data());
            }
        }

        for (auto& file : resources)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT(".model")))
            {
                const size_t prevModels = scene.cpu.models.size();

                loadModel(file.file_data());

                if (prevModels != scene.cpu.models.size())
                    break;
            }
        }

        for (auto& file : resources)
        {
            if (hl::text::strstr(file.name(), HL_NTEXT(".light")) && !hl::text::strstr(file.name(), HL_NTEXT(".light-list")))
                loadLight(file.file_data());
        }

        if (scene.cpu.localLights.empty())
            scene.cpu.localLights.emplace_back();

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

void Scene::createGpuResources(const App& app)
{
    std::vector<std::vector<D3D12_SUBRESOURCE_DATA>> subResourcesPerTexture;

    for (const auto& texture : cpu.textures)
    {
        DirectX::LoadDDSTextureFromMemory(
            app.device.nvrhi,
            app.device.allocator.Get(),
            texture.data.get(),
            texture.dataSize,
            std::addressof(gpu.textures.emplace_back()),
            gpu.allocations.emplace_back().GetAddressOf(),
            subResourcesPerTexture.emplace_back());

        if (!gpu.textures.back())
            gpu.textures.back() = gpu.textures.front();
    }

    std::vector<Material::GPU> materialBuffer;

    for (auto& material : cpu.materials)
    {
        auto& gpuMaterial = materialBuffer.emplace_back();

        const auto shaderPair = app.shaderLibrary.shaderMapping.map.find(material.shader);

        if (shaderPair != app.shaderLibrary.shaderMapping.map.end())
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

    gpu.materialBuffer = app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(materialBuffer))
        .setStructStride(vectorByteStride(materialBuffer))
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    std::vector<Mesh::GPU> meshBuffer;

    for (const auto& mesh : cpu.meshes)
        meshBuffer.push_back({ mesh.vertexOffset, mesh.indexOffset, mesh.materialIndex, mesh.type });

    gpu.meshBuffer = app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(meshBuffer))
        .setFormat(nvrhi::Format::RGBA32_UINT)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    gpu.vertexBuffer = app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.vertices))
        .setFormat(nvrhi::Format::RGB32_FLOAT)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true)
        .setIsAccelStructBuildInput(true));

    gpu.normalBuffer = app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.normals))
        .setFormat(nvrhi::Format::RGBA8_SNORM)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    gpu.tangentBuffer = app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.tangents))
        .setFormat(nvrhi::Format::RGBA8_SNORM)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    gpu.texCoordBuffer = app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.texCoords))
        .setFormat(nvrhi::Format::RG16_FLOAT)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    gpu.colorBuffer = app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.colors))
        .setFormat(nvrhi::Format::RGBA8_UNORM)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    gpu.indexBuffer = app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.indices))
        .setFormat(nvrhi::Format::R16_UINT)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true)
        .setIsAccelStructBuildInput(true));

    gpu.lightBuffer = app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(vectorByteSize(cpu.localLights))
        .setFormat(nvrhi::Format::RGBA32_FLOAT)
        .setCanHaveTypedViews(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    std::vector<nvrhi::rt::AccelStructDesc> blasDescs;

    for (const auto& model : cpu.models)
    {
        auto& blasDesc = blasDescs.emplace_back();

        for (size_t i = 0; i < model.meshCount; i++)
        {
            const Mesh& mesh = cpu.meshes[model.meshOffset + i];

            auto& meshDesc = blasDesc.bottomLevelGeometries.emplace_back();

            if ((mesh.type & (MESH_TYPE_TRANS | MESH_TYPE_PUNCH)) == 0)
                meshDesc.flags = nvrhi::rt::GeometryFlags::Opaque;

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

        gpu.modelBVHs.push_back(app.device.nvrhi->createAccelStruct(blasDesc));
    }

    std::vector<nvrhi::rt::InstanceDesc> instanceDescs;
    std::vector<nvrhi::rt::InstanceDesc> shadowInstanceDescs;
    std::vector<nvrhi::rt::InstanceDesc> skyInstanceDescs;

    for (const auto& instance : cpu.instances)
    {
        nvrhi::rt::InstanceDesc instanceDesc;

        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < 4; j++)
                instanceDesc.transform[i * 4 + j] = instance.transform(j, i);
        }

        instanceDesc.instanceID = cpu.models[instance.modelIndex].meshOffset;
        instanceDesc.instanceMask = cpu.models[instance.modelIndex].instanceMask;
        instanceDesc.instanceContributionToHitGroupIndex = instanceDesc.instanceID;
        instanceDesc.bottomLevelAS = gpu.modelBVHs[instance.modelIndex];

        if (instanceDesc.instanceMask == INSTANCE_MASK_SKY)
            skyInstanceDescs.push_back(instanceDesc);

        else
        {
            instanceDescs.push_back(instanceDesc);

            if (instanceDesc.instanceMask == INSTANCE_MASK_OPAQUE_OR_PUNCH)
                shadowInstanceDescs.push_back(instanceDesc);
        }
    }

    auto bvhDesc = nvrhi::rt::AccelStructDesc()
        .setIsTopLevel(true)
        .setBuildFlags(nvrhi::rt::AccelStructBuildFlags::PreferFastTrace);

    gpu.bvh = app.device.nvrhi->createAccelStruct(bvhDesc
        .setTopLevelMaxInstances(instanceDescs.size()));

    gpu.shadowBVH = app.device.nvrhi->createAccelStruct(bvhDesc
        .setTopLevelMaxInstances(shadowInstanceDescs.size()));

    gpu.skyBVH = app.device.nvrhi->createAccelStruct(bvhDesc
        .setTopLevelMaxInstances(skyInstanceDescs.size()));

    for (size_t i = 0; i < subResourcesPerTexture.size(); i++)
    {
        const auto& subResources = subResourcesPerTexture[i];
        const auto& texture = gpu.textures[i];

        for (size_t j = 0; j < subResources.size(); j++)
        {
            const auto& subResource = subResources[j];

            app.renderer.commandList->writeTexture(
                gpu.textures[i],
                j / texture->getDesc().mipLevels,
                j % texture->getDesc().mipLevels,
                subResource.pData,
                subResource.RowPitch,
                subResource.SlicePitch);
        }
    }

    app.renderer.commandList->writeBuffer(gpu.materialBuffer, materialBuffer.data(), vectorByteSize(materialBuffer));
    app.renderer.commandList->writeBuffer(gpu.meshBuffer, meshBuffer.data(), vectorByteSize(meshBuffer));
    app.renderer.commandList->writeBuffer(gpu.vertexBuffer, cpu.vertices.data(), vectorByteSize(cpu.vertices));
    app.renderer.commandList->writeBuffer(gpu.normalBuffer, cpu.normals.data(), vectorByteSize(cpu.normals));
    app.renderer.commandList->writeBuffer(gpu.tangentBuffer, cpu.tangents.data(), vectorByteSize(cpu.tangents));
    app.renderer.commandList->writeBuffer(gpu.texCoordBuffer, cpu.texCoords.data(), vectorByteSize(cpu.texCoords));
    app.renderer.commandList->writeBuffer(gpu.colorBuffer, cpu.colors.data(), vectorByteSize(cpu.colors));
    app.renderer.commandList->writeBuffer(gpu.indexBuffer, cpu.indices.data(), vectorByteSize(cpu.indices));
    app.renderer.commandList->writeBuffer(gpu.lightBuffer, cpu.localLights.data(), vectorByteSize(cpu.localLights));

    for (size_t i = 0; i < blasDescs.size(); i++)
        nvrhi::utils::BuildBottomLevelAccelStruct(app.renderer.commandList, gpu.modelBVHs[i], blasDescs[i]);

    app.renderer.commandList->buildTopLevelAccelStruct(gpu.bvh, instanceDescs.data(), instanceDescs.size());
    app.renderer.commandList->buildTopLevelAccelStruct(gpu.shadowBVH, shadowInstanceDescs.data(), shadowInstanceDescs.size());
    app.renderer.commandList->buildTopLevelAccelStruct(gpu.skyBVH, skyInstanceDescs.data(), skyInstanceDescs.size());
}
