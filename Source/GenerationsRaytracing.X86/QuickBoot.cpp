#include "QuickBoot.h"

#include "MessageSender.h"

static constexpr const char* s_stages[] =
{
    "ghz100",
    "ghz200",
    "cpz100",
    "cpz200",
    "ssz100",
    "ssz200",
    "sph100",
    "sph200",
    "cte100",
    "cte200",
    "ssh100",
    "ssh200",
    "csc100",
    "csc200",
    "euc100",
    "euc200",
    "pla100",
    "pla200",
    "cnz100",
    "bms",
    "bsd",
    "bsl",
    "bde",
    "bpc",
    "bne",
    "blb"
};

struct Mod
{
    std::string guid;
    std::string filePath;
    std::string name;
    std::bitset<_countof(s_stages)> stages;
};

static std::string s_modsDbFilePath;

static std::string s_manifestVersion;

static std::vector<Mod> s_mods;
static std::vector<uint32_t> s_activeMods;
static std::vector<uint32_t> s_favoriteMods;

static std::vector<std::string> s_codes;

static uint32_t s_quickBootIndex = ~0;

static bool s_success;

static struct FileSystemThreadHolder
{
    std::thread thread;

    ~FileSystemThreadHolder()
    {
        if (thread.joinable())
            thread.join();
    }
} s_fileSystemThreadHolder;

static void loadModsDb()
{
    IniFile cpkredirIni;
    if (!cpkredirIni.read("cpkredir.ini"))
        return;

    s_modsDbFilePath = cpkredirIni.getString("CPKREDIR", "ModsDbIni", "");

    IniFile modsDbIni;
    if (!modsDbIni.read(s_modsDbFilePath.c_str()))
        return;

    s_manifestVersion = modsDbIni.getString("Main", "ManifestVersion", "1.1");

    std::unordered_map<std::string_view, uint32_t> stageIndices;
    for (size_t i = 0; i < _countof(s_stages); i++)
        stageIndices.emplace(s_stages[i], i);

    std::unordered_map<std::string_view, uint32_t> modIndices;
    modsDbIni.enumerate("Mods", [&](const std::string& name, const std::string& value)
    {
        IniFile modIni;
        if (!modIni.read(value.c_str()))
            return;

        modIndices.emplace(name, s_mods.size());
        auto title = modIni.getString("Desc", "Title", "");

        if (title == "Quick Boot+")
            s_quickBootIndex = s_mods.size();

        s_mods.push_back({ name, value, std::move(title) });
    });

    s_fileSystemThreadHolder.thread = std::thread([stageIndices = std::move(stageIndices)]
    {
       for (auto& mod : s_mods)
       {
           const size_t sepIndex = mod.filePath.find_last_of("\\/");
           const std::string_view directoryPath = std::string_view(mod.filePath).substr(0, sepIndex);

           for (const auto& path : std::filesystem::recursive_directory_iterator(directoryPath,
               std::filesystem::directory_options::skip_permission_denied))
           {
               if (path.is_directory())
               {
                   const auto fileName = path.path().filename().u8string();
                   const auto stageIndex = stageIndices.find(fileName);
                   if (stageIndex != stageIndices.end())
                       mod.stages[stageIndex->second] = true;
               }
           }
       }
    });

    const uint32_t activeModCount = modsDbIni.get<uint32_t>("Main", "ActiveModCount", 0);
    s_activeMods.reserve(activeModCount);

    for (size_t i = 0; i < activeModCount; i++)
    {
        char propertyName[32];
        sprintf_s(propertyName, "ActiveMod%d", i);

        const auto modIndex = modIndices.find(modsDbIni.getString("Main", propertyName, ""));
        if (modIndex != modIndices.end())
            s_activeMods.push_back(modIndex->second);
    }

    const uint32_t favoriteModCount = modsDbIni.get<uint32_t>("Main", "FavoriteModCount", 0);
    s_favoriteMods.reserve(favoriteModCount);

    for (size_t i = 0; i < favoriteModCount; i++)
    {
        char propertyName[32];
        sprintf_s(propertyName, "FavoriteMod%d", i);

        const auto modIndex = modIndices.find(modsDbIni.getString("Main", propertyName, ""));
        if (modIndex != modIndices.end())
            s_favoriteMods.push_back(modIndex->second);
    }

    const uint32_t codeCount = modsDbIni.get<uint32_t>("Codes", "CodeCount", 0);
    s_codes.reserve(codeCount);

    for (size_t i = 0; i < codeCount; i++)
    {
        char propertyName[32];
        sprintf_s(propertyName, "Code%d", i);
        s_codes.push_back(modsDbIni.getString("Codes", propertyName, ""));
    }

    s_success = true;
}

static wchar_t s_bootstrapFilePath[0x400];

static void startBootstrapApp()
{
    TCHAR currentDirectory[0x400]{};
    GetCurrentDirectory(_countof(currentDirectory), currentDirectory);

    STARTUPINFO startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInformation{};

    CreateProcess(
        s_bootstrapFilePath,
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        currentDirectory,
        &startupInfo,
        &processInformation);
}

static void writeModsDbAndRestart(size_t modIndex, const char* stage)
{
    startBootstrapApp();

    for (auto it = s_activeMods.begin(); it != s_activeMods.end();)
    {
        const auto& mod = s_mods[*it];

        if (s_mods[*it].stages.any() && mod.name != "Generations Raytracing")
            it = s_activeMods.erase(it);
        else
            ++it;
    }

    if (std::find(s_activeMods.begin(), s_activeMods.end(), s_quickBootIndex) == s_activeMods.end())
        s_activeMods.push_back(s_quickBootIndex);

    if (modIndex < s_mods.size())
    {
        if (s_mods[modIndex].name == "Unleashed Project")
        {
            for (size_t i = 0; i < s_mods.size(); i++)
            {
                if (s_mods[i].name == "Unleashed Project V2")
                {
                    s_activeMods.push_back(i);
                    break;
                }
            }
        }

        s_activeMods.push_back(modIndex);
    }

    const auto& quickBootMod = s_mods[s_quickBootIndex];
    const size_t index = quickBootMod.filePath.find_last_of("\\/");
    std::string filePath = quickBootMod.filePath.substr(0, index + 1);
    filePath += "GenerationsQuickBoot.ini";

    IniFile quickBootIni;
    quickBootIni.read(filePath.c_str());
    quickBootIni.setString("QuickBoot", "StageName", stage);
    quickBootIni.set("QuickBoot", "g_IsQuickBoot", 3);
    quickBootIni.write(filePath.c_str());

    IniFile modsDbIni;
    modsDbIni.setString("Main", "ManifestVersion", s_manifestVersion);
    modsDbIni.setString("Main", "ReverseLoadOrder", "0");

    modsDbIni.set("Main", "ActiveModCount", s_activeMods.size());
    for (size_t i = 0; i < s_activeMods.size(); i++)
    {
        char propertyName[32];
        sprintf_s(propertyName, "ActiveMod%d", i);
        modsDbIni.setString("Main", propertyName, s_mods[s_activeMods[i]].guid);
    }

    modsDbIni.set("Main", "FavoriteModCount", s_favoriteMods.size());
    for (size_t i = 0; i < s_favoriteMods.size(); i++)
    {
        char propertyName[32];
        sprintf_s(propertyName, "FavoriteMod%d", i);
        modsDbIni.setString("Main", propertyName, s_mods[s_favoriteMods[i]].guid);
    }

    for (const auto& mod : s_mods)
        modsDbIni.setString("Mods", mod.guid, mod.filePath);

    modsDbIni.set("Codes", "CodeCount", s_codes.size());
    for (size_t i = 0; i < s_codes.size(); i++)
    {
        char propertyName[32];
        sprintf_s(propertyName, "Code%d", i);
        modsDbIni.setString("Codes", propertyName, s_codes[i]);
    }

    modsDbIni.write(s_modsDbFilePath.c_str());

    ExitProcess(1);
}

void QuickBoot::init()
{
    GetCurrentDirectory(_countof(s_bootstrapFilePath), s_bootstrapFilePath);
    wcscat_s(s_bootstrapFilePath, L"\\GenerationsRaytracing.X86.Bootstrap.exe");
}

static bool s_loaded;

void QuickBoot::renderImgui()
{
    if (!s_loaded)
    {
        loadModsDb();
        s_loaded = true;
    }

    if (s_success)
    {
        if (s_quickBootIndex != ~0)
        {
            if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Sonic Generations");
                ImGui::TableNextColumn();

                constexpr float BUTTON_SIZE = 50.0f;
                const float windowVisible = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

                for (auto& stage : s_stages)
                {
                    if (ImGui::Button(stage, { BUTTON_SIZE, 0.0f }) && !*s_shouldExit)
                        writeModsDbAndRestart(~0, stage);

                    if (ImGui::GetItemRectMax().x + BUTTON_SIZE < windowVisible)
                        ImGui::SameLine();
                }
            
                for (size_t i = 0; i < s_mods.size(); i++)
                {
                    const auto& mod = s_mods[i];

                    if (mod.stages.any())
                    {
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(mod.name.c_str());
                        ImGui::TableNextColumn();

                        ImGui::PushID(&mod);
                        for (size_t j = 0; j < _countof(s_stages); j++)
                        {
                            if (mod.stages[j])
                            {
                                const char* stage = s_stages[j];

                                if (ImGui::Button(stage, { BUTTON_SIZE, 0.0f }) && !*s_shouldExit)
                                    writeModsDbAndRestart(i, stage);

                                if (ImGui::GetItemRectMax().x + BUTTON_SIZE < windowVisible)
                                    ImGui::SameLine();
                            }
                        }
                        ImGui::PopID();
                    }
                }
            
                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextUnformatted("You need to have Quick Boot+ installed to use this feature.");
        }
    }
    else
    {
        ImGui::TextUnformatted("An error occured while parsing mods.");
    }
}
