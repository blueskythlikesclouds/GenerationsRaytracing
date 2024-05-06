#include "ModelReplacer.h"

#include "ModelData.h"
#include "SampleChunkResource.h"

struct NoAoModel
{
    std::string name;
    std::string noAoName;
    std::vector<XXH32_hash_t> hashes;
};

static std::vector<NoAoModel> s_noAoModels;

static void parseJson(json& json)
{
    for (auto& obj : json)
    {
        auto& noAoModel = s_noAoModels.emplace_back();

        noAoModel.name = obj["name"];
        noAoModel.noAoName = obj["noAoName"];

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

HOOK(void, __cdecl, ModelDataMake, 0x7337A0,
    const Hedgehog::Base::CSharedString& name,
    void* data,
    uint32_t dataSize,
    const boost::shared_ptr<Hedgehog::Database::CDatabase>& database,
    Hedgehog::Mirage::CRenderingInfrastructure* renderingInfrastructure)
{
    if (data != nullptr)
    {
        for (const auto& noAoModel : s_noAoModels)
        {
            if (noAoModel.name == name.c_str())
            {
                Hedgehog::Mirage::CMirageDatabaseWrapper wrapper(database.get());

                const auto modelData = wrapper.GetModelData(name);
                if (!modelData->IsMadeOne())
                {
                    auto& modelDataEx = *reinterpret_cast<ModelDataEx*>(modelData.get());
                    const XXH32_hash_t hash = XXH32(data, dataSize, 0);
                    if (std::find(noAoModel.hashes.begin(), noAoModel.hashes.end(), hash) != noAoModel.hashes.end())
                        modelDataEx.m_noAoModel = wrapper.GetModelData(noAoModel.noAoName.c_str());
                }

                break;
            }
        }
    }

    originalModelDataMake(name, data, dataSize, database, renderingInfrastructure);
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
