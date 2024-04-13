#include "LightData.h"

static XXH32_hash_t s_currentHash = 0;
static xxHashMap<std::vector<Light>> s_serializedLights;

void LightData::update(XXH32_hash_t hash)
{
    s_currentHash = hash;
    s_lights = s_serializedLights[hash];
    s_dirty = true;
}

static std::string s_saveFilePath;

static void load()
{
    std::ifstream stream(s_saveFilePath);
    if (stream.is_open())
    {
        json json;
        stream >> json;

        for (const auto& hashObj : json)
        {
            const auto hash = hashObj["hash"].get<XXH32_hash_t>();
            auto& lights = s_serializedLights[hash];

            for (auto& lightObj : hashObj["lights"])
            {
                auto& light = lights.emplace_back();

                lightObj["name"].get_to(light.name);
                lightObj["position_x"].get_to(light.position[0]);
                lightObj["position_y"].get_to(light.position[1]);
                lightObj["position_z"].get_to(light.position[2]);
                lightObj["color_r"].get_to(light.color[0]);
                lightObj["color_g"].get_to(light.color[1]);
                lightObj["color_b"].get_to(light.color[2]);
                lightObj["color_intensity"].get_to(light.colorIntensity);
                lightObj["in_range"].get_to(light.inRange);
                lightObj["out_range"].get_to(light.outRange);
            }
        }
    }
}

static void save()
{
    json hashesObj;

    for (auto& [hash, lights] : s_serializedLights)
    {
        json lightsObj;

        for (auto& light : lights)
        {
            lightsObj.push_back(
                {
                    { "name", light.name },
                    { "position_x", light.position[0] },
                    { "position_y", light.position[1] },
                    { "position_z", light.position[2] },
                    { "color_r", light.color[0] },
                    { "color_g", light.color[1] },
                    { "color_b", light.color[2] },
                    { "color_intensity", light.colorIntensity },
                    { "in_range", light.inRange },
                    { "out_range", light.outRange }
                });
        }

        hashesObj.push_back({ { "hash", hash }, { "lights", lightsObj } });
    }

    std::ofstream stream(s_saveFilePath);
    if (stream.is_open())
        stream << std::setw(4) << hashesObj;
}

void LightData::init(ModInfo_t* modInfo)
{
    s_saveFilePath = modInfo->CurrentMod->Path;
    s_saveFilePath.erase(s_saveFilePath.find_last_of("\\/") + 1);
    s_saveFilePath += "lights.json";

    load();
}

static size_t s_selectedLightIndex = 0;
static bool s_showAllGismos = false;

static void makeName(const char* originalName, char* destName, size_t destNameSize)
{
    char prefix[0x100];
    strcpy_s(prefix, originalName);
    size_t length = strlen(prefix);
    while (length > 0 && std::isdigit(prefix[length - 1]))
    {
        --length;
        prefix[length] = '\0';
    }

    size_t index = LightData::s_lights.size();
    while (true)
    {
        sprintf_s(destName, destNameSize, "%s%03d", prefix, index);

        bool found = false;
        for (auto& light : LightData::s_lights)
        {
            if (light.name == destName)
            {
                found = true;
                break;
            }
        }

        if (!found)
            break;

        ++index;
    }
}

void LightData::renderImgui()
{
    if (ImGui::BeginChild("Child"))
    {
        if (ImGui::BeginListBox("##Light", { -FLT_MIN, 0.0f }))
        {
            for (size_t i = 0; i < s_lights.size(); i++)
            {
                if (ImGui::Selectable(s_lights[i].name.c_str(), s_selectedLightIndex == i))
                    s_selectedLightIndex = i;
            }

            ImGui::EndListBox();
        }

        if (ImGui::Button("Add"))
        {
            s_selectedLightIndex = s_lights.size();
            auto& light = s_lights.emplace_back();

            if (auto gameDocument = Sonic::CGameDocument::GetInstance())
            {
                char name[0x100];
                makeName("Omni", name, sizeof(name));
                light.name = name;

                const auto camera = gameDocument->GetWorld()->GetCamera();
                const auto position = camera->m_MyCamera.m_Position + camera->m_MyCamera.m_Direction;
                memcpy(light.position, position.data(), sizeof(light.position));

                light.color[0] = 1.0f;
                light.color[1] = 1.0f;
                light.color[2] = 1.0f;
                light.colorIntensity = 1.0f;
                light.inRange = 0.0f;
                light.outRange = 10.0f;
            }

            s_dirty = true;
        }
        ImGui::SameLine();

        if (ImGui::Button("Remove") && s_selectedLightIndex < s_lights.size())
        {
            s_lights.erase(s_lights.begin() + s_selectedLightIndex);

            if (s_selectedLightIndex >= s_lights.size())
                s_selectedLightIndex = s_lights.size() - 1;

            s_dirty = true;
        }
        ImGui::SameLine();

        if (ImGui::Button("Clone") && s_selectedLightIndex < s_lights.size())
        {
            auto& light = s_lights.emplace_back(s_lights[s_selectedLightIndex]);
            s_selectedLightIndex = s_lights.size() - 1;

            char name[0x100];
            makeName(light.name.c_str(), name, sizeof(name));
            light.name = name;

            s_dirty = true;
        }

        if (s_selectedLightIndex < s_lights.size())
        {
            auto& light = s_lights.at(s_selectedLightIndex);

            if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Name");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);

                char name[0x100];
                strcpy_s(name, light.name.c_str());
                if (ImGui::InputText("##Name", name, sizeof(name)))
                    light.name = name;

                static constexpr const char* s_format = "%g";

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Position");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                s_dirty |= ImGui::DragFloat3("##Position", light.position, 0.1f, 0.0f, 0.0f, s_format);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Color");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                s_dirty |= ImGui::ColorEdit3("##Color", light.color);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Color Intensity");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                s_dirty |= ImGui::DragFloat("##Color Intensity", &light.colorIntensity, 0.01f, 1.0f, FLT_MAX, s_format);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("In Range");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                s_dirty |= ImGui::DragFloat("##In Range", &light.inRange, 0.1f, 0.0f, light.outRange, s_format);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Out Range");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                s_dirty |= ImGui::DragFloat("##Out Range", &light.outRange, 0.1f, light.inRange, FLT_MAX, s_format);

                ImGui::EndTable();
            }
        }

        if (s_showAllGismos)
        {
            for (size_t i = 0; i < s_lights.size(); i++)
            {
                auto& light = s_lights[i];
                if (Im3d::GizmoTranslation(reinterpret_cast<Im3d::Id>(&light), light.position))
                {
                    s_selectedLightIndex = i;
                    s_dirty = true;
                }
            }
        }
        else if (s_selectedLightIndex < s_lights.size())
        {
            auto& light = s_lights[s_selectedLightIndex];
            if (Im3d::GizmoTranslation(reinterpret_cast<Im3d::Id>(&light), light.position))
                s_dirty = true;
        }

        if (s_selectedLightIndex < s_lights.size())
        {
            auto& light = s_lights[s_selectedLightIndex];
            Im3d::DrawSphere(reinterpret_cast<Im3d::Vec3&>(light.position), light.outRange);
        }
    }

    if (s_currentHash != 0 && ImGui::Button("Revert Changes"))
    {
        s_lights = s_serializedLights[s_currentHash];
        s_dirty = true;
    }

    ImGui::SameLine();

    if (s_currentHash != 0 && ImGui::Button("Save Changes"))
    {
        s_serializedLights[s_currentHash] = s_lights;
        save();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Show All Gismos", &s_showAllGismos);

    ImGui::EndChild();
    ImGui::EndTabItem();
}
