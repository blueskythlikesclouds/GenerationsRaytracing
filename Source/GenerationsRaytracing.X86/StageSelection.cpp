#include "StageSelection.h"

struct Selection
{
    uint32_t stageIndex = 0;
    bool rememberSelection = true;
};

static xxHashMap<Selection> s_selections;

HOOK(void, __cdecl, MakeTerrainData, 0x7346F0,
    const Hedgehog::Base::CSharedString& name,
    const void* data,
    uint32_t dataSize,
    const boost::shared_ptr<Hedgehog::Database::CDatabase>& database,
    Hedgehog::Mirage::CRenderingInfrastructure* renderingInfrastructure)
{
    auto& selection = s_selections[XXH32(data, dataSize, 0)];
    StageSelection::s_stageIndex = &selection.stageIndex;
    StageSelection::s_rememberSelection = &selection.rememberSelection;

    originalMakeTerrainData(name, data, dataSize, database, renderingInfrastructure);
}

static std::string s_saveFilePath;

static void load()
{
    std::ifstream stream(s_saveFilePath);
    if (stream.is_open())
    {
        json json;
        stream >> json;

        for (const auto& obj : json)
        {
            auto& selection = s_selections[obj["hash"].get<XXH32_hash_t>()];
            obj["remember_selection"].get_to(selection.rememberSelection);

            if (selection.rememberSelection)
                obj["stage_index"].get_to(selection.stageIndex);
        }
    }
}

static void save()
{
    json json;

    for (auto& [hash, selection] : s_selections)
    {
        json.push_back({ {"hash", hash}, 
            {"remember_selection", selection.rememberSelection}, {"stage_index", selection.stageIndex} });
    }

    std::ofstream stream(s_saveFilePath);
    if (stream.is_open())
        stream << json;
}

void StageSelection::init(ModInfo_t* modInfo)
{
    INSTALL_HOOK(MakeTerrainData);

    s_saveFilePath = modInfo->CurrentMod->Path;
    s_saveFilePath.erase(s_saveFilePath.find_last_of("\\/") + 1);
    s_saveFilePath += "stage_selections.json";

    load();
    std::atexit(save);
}
