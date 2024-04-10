#include "StageSelection.h"

struct Selection
{
    bool rememberSelection = false;
    uint32_t stageIndex = 0;
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
    if (!selection.rememberSelection)
        selection.stageIndex = NULL;

    StageSelection::s_rememberSelection = &selection.rememberSelection;
    StageSelection::s_stageIndex = &selection.stageIndex;

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
            selection.rememberSelection = true;
            obj["stage_index"].get_to(selection.stageIndex);
        }
    }
}

static void save()
{
    json json;

    for (auto& [hash, selection] : s_selections)
    {
        if (selection.rememberSelection)
            json.push_back({ {"hash", hash}, {"stage_index", selection.stageIndex} });
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
