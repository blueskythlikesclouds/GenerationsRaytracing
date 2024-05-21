#include "ModelReplacer.h"

#include "ModelData.h"
#include "SampleChunkResource.h"
#include "InstanceData.h"
#include "MaterialData.h"
#include "Logger.h"

struct NoAoModel
{
    std::string name;
    std::vector<std::string> archives;
    std::vector<XXH32_hash_t> hashes;
};

static std::vector<NoAoModel> s_noAoModels;

static void parseJson(json& json)
{
    for (auto& obj : json)
    {
        auto& noAoModel = s_noAoModels.emplace_back();

        noAoModel.name = obj["name"];

        for (auto& archiveObj : obj["archives"])
            noAoModel.archives.push_back(archiveObj);

        for (auto& hashObj : obj["hashes"])
        {
            if (hashObj.is_number_unsigned())
            {
                noAoModel.hashes.push_back(hashObj);
            }
            else
            {
                std::string value = hashObj;

                if (value.size() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X'))
                    value = value.substr(2);

                noAoModel.hashes.push_back(std::stoul(value, nullptr, 16));
            }
        }
    }
}

static Mutex s_mutex;

struct PendingModel
{
    uint32_t noAoModelIndex;
    boost::shared_ptr<Hedgehog::Mirage::CModelData> modelData;
    boost::shared_ptr<Hedgehog::Database::CDatabase> database;
};

static std::vector<PendingModel> s_pendingModels;

HOOK(void, __cdecl, ModelDataMake, 0x7337A0,
    const Hedgehog::Base::CSharedString& name,
    void* data,
    uint32_t dataSize,
    const boost::shared_ptr<Hedgehog::Database::CDatabase>& database,
    Hedgehog::Mirage::CRenderingInfrastructure* renderingInfrastructure)
{
    if (data != nullptr)
    {
        Hedgehog::Mirage::CMirageDatabaseWrapper wrapper(database.get());

        const auto modelData = wrapper.GetModelData(name);
        if (!modelData->IsMadeOne())
        {
            auto& modelDataEx = *reinterpret_cast<ModelDataEx*>(modelData.get());
            const XXH32_hash_t hash = XXH32(data, dataSize, 0);
            modelDataEx.m_checkForEdgeEmission = (hash == 0xBAFA3FA1);

            for (size_t i = 0; i < s_noAoModels.size(); i++)
            {
                auto& noAoModel = s_noAoModels[i];

                if (std::find(noAoModel.hashes.begin(), noAoModel.hashes.end(), hash) != noAoModel.hashes.end())
                {
                    LockGuard lock(s_mutex);
                    s_pendingModels.push_back({ i, modelData, database });
                
                    break;
                }
            }
        }
    }

    originalModelDataMake(name, data, dataSize, database, renderingInfrastructure);
}

static std::unordered_set<std::string_view> s_archiveNames;
static std::vector<boost::shared_ptr<Hedgehog::Mirage::CModelData>> s_models;

template<typename T>
static void traverseModelData(const Hedgehog::Mirage::CModelData& modelData, const T& function)
{
    for (const auto& meshData : modelData.m_OpaqueMeshes) if (function(*meshData)) return;
    for (const auto& meshData : modelData.m_PunchThroughMeshes) if (function(*meshData)) return;
    for (const auto& meshData : modelData.m_TransparentMeshes) if (function(*meshData)) return;
    
    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        for (const auto& meshData : nodeGroupModelData->m_OpaqueMeshes) if (function(*meshData)) return;
        for (const auto& meshData : nodeGroupModelData->m_PunchThroughMeshes) if (function(*meshData)) return;
        for (const auto& meshData : nodeGroupModelData->m_TransparentMeshes) if (function(*meshData)) return;

        for (const auto& specialMeshGroup : nodeGroupModelData->m_SpecialMeshGroups)
        {
            for (const auto& meshData : specialMeshGroup)
            {
                if (function(*meshData)) 
                    return;
            }
        }
    }
}

void ModelReplacer::createPendingModels()
{
    // Must make a local copy to avoid deadlock when calling LoadArchive
    s_mutex.lock();
    const auto pendingModels = s_pendingModels;
    s_pendingModels.clear();
    s_mutex.unlock();

    if (!pendingModels.empty())
    {
        auto database = Hedgehog::Database::CDatabase::CreateDatabase();

        for (auto& pendingModel : pendingModels)
        {
            for (auto& archiveName : s_noAoModels[pendingModel.noAoModelIndex].archives)
                s_archiveNames.emplace(archiveName);
        }

        auto& loader = Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spDatabaseLoader;

        static Hedgehog::Base::CSharedString s_ar(".ar");
        static Hedgehog::Base::CSharedString s_arl(".arl");

        for (const auto& archive : s_archiveNames)
        {
            loader->CreateArchiveList(
                archive.data() + s_ar,
                archive.data() + s_arl,
                { 200, 5 });
        }

        for (const auto& archive : s_archiveNames)
            loader->LoadArchiveList(database, archive.data() + s_arl);

        for (const auto& archive : s_archiveNames)
            loader->LoadArchive(database, archive.data() + s_ar, {-10, 5}, false, false);

        Hedgehog::Mirage::CMirageDatabaseWrapper wrapper(database.get());

        for (auto& pendingModel : pendingModels)
        {
            const auto modelDataEx = reinterpret_cast<ModelDataEx*>(pendingModel.modelData.get());
            modelDataEx->m_noAoModel = wrapper.GetModelData(s_noAoModels[pendingModel.noAoModelIndex].name.c_str());

            if (modelDataEx->m_noAoModel != nullptr)
                s_models.push_back(pendingModel.modelData);
        }

        s_archiveNames.clear();
    }

    for (auto it = s_models.begin(); it != s_models.end();)
    {
        bool shouldRemove = false;

        if ((*it)->IsMadeAll())
        {
            const auto modelDataEx = reinterpret_cast<ModelDataEx*>(it->get());
            if (modelDataEx->m_noAoModel->IsMadeAll())
            {
                traverseModelData(*modelDataEx->m_noAoModel, [&](const Hedgehog::Mirage::CMeshData& meshData)
                {
                    static constexpr const char* s_suffixes[] =
                    {
                        "_eye",
                        "hite",
                        "Teye"
                    };

                    const char* eyePart = nullptr;
                    for (const auto suffix : s_suffixes)
                    {
                        eyePart = strstr(meshData.m_MaterialName.c_str(), suffix);
                        if (eyePart != nullptr)
                        {
                            traverseModelData(**it, [&](const Hedgehog::Mirage::CMeshData& meshDataToCompare)
                            {
                                if (meshDataToCompare.m_spMaterial != nullptr)
                                {
                                    const char* eyePartToCompare = strstr(meshDataToCompare.m_MaterialName.c_str(), suffix);
                            
                                    if (eyePartToCompare != nullptr && strncmp(eyePart, eyePartToCompare, 5) == 0)
                                    {
                                        reinterpret_cast<MaterialDataEx*>(
                                            meshDataToCompare.m_spMaterial.get())->m_fhlMaterial = meshData.m_spMaterial;
                            
                                        return true;
                                    }
                                }
                                return false;
                            });

                            break;
                        }
                    }

                    return false;
                });

                shouldRemove = true;
            }
        }

        if (shouldRemove)
            it = s_models.erase(it);
        else
            ++it;
    }
}

static std::unordered_map<Hedgehog::Mirage::CMaterialData*, Hedgehog::Mirage::CMaterialData*> s_fhlMaterials;

static FUNCTION_PTR(void, __thiscall, cloneMaterial, 0x704CE0,
    Hedgehog::Mirage::CMaterialData* This, Hedgehog::Mirage::CMaterialData* rValue);

void ModelReplacer::processFhlMaterials(InstanceInfoEx& instanceInfoEx, const MaterialMap& materialMap)
{
    static Hedgehog::Base::CStringSymbol s_texcoordOffsetSymbol("mrgTexcoordOffset");

    for (auto& [key, value] : instanceInfoEx.m_effectMap)
    {
        auto& keyEx = *reinterpret_cast<MaterialDataEx*>(key);
        if (keyEx.m_fhlMaterial != nullptr)
            s_fhlMaterials.emplace(keyEx.m_fhlMaterial.get(), key);
    }

    for (auto& [key, value] : materialMap)
    {
        auto& keyEx = *reinterpret_cast<MaterialDataEx*>(key);
        if (keyEx.m_fhlMaterial != nullptr)
            s_fhlMaterials.emplace(keyEx.m_fhlMaterial.get(), key);
    }

    for (auto& [fhlMaterial, material] : s_fhlMaterials)
    {
        auto overrideFindResult = materialMap.find(material);
        if (overrideFindResult != materialMap.end())
            material = overrideFindResult->second.get();

        auto effectFindResult = instanceInfoEx.m_effectMap.find(material);
        if (effectFindResult != instanceInfoEx.m_effectMap.end())
            material = effectFindResult->second.get();

        auto& materialClone = instanceInfoEx.m_effectMap[fhlMaterial];
        if (materialClone == nullptr)
        {
            const auto alsoMaterialClone = static_cast<Hedgehog::Mirage::CMaterialData*>(__HH_ALLOC(sizeof(MaterialDataEx)));
            Hedgehog::Mirage::fpCMaterialDataCtor(alsoMaterialClone);
            cloneMaterial(alsoMaterialClone, fhlMaterial);
            materialClone = boost::shared_ptr<Hedgehog::Mirage::CMaterialData>(alsoMaterialClone);
        }

        for (auto& sourceParam : material->m_Float4Params)
        {
            if (sourceParam->m_Name == s_texcoordOffsetSymbol)
            {
                bool found = false;

                for (auto& destParam : materialClone->m_Float4Params)
                {
                    if (destParam->m_Name == s_texcoordOffsetSymbol)
                    {
                        destParam = sourceParam;
                        found = true;
                        break;
                    }
                }

                if (!found)
                    materialClone->m_Float4Params.push_back(sourceParam);

                break;
            }
        }
    }

    s_fhlMaterials.clear();
}

void ModelReplacer::init()
{
    INSTALL_HOOK(ModelDataMake);

    std::ifstream stream("no_ao_models.json");
    if (stream.is_open())
    {
        json json;
        stream >> json;
        parseJson(json);

        stream.close();
    }
    else
    {
        MessageBox(nullptr, 
            TEXT("Unable to open \"no_ao_models.json\" in mod directory."), 
            TEXT("Generations Raytracing"), 
            MB_ICONERROR);
    }
}
