﻿#include "RaytracingParams.h"

#include "Configuration.h"
#include "EnvironmentMode.h"
#include "QuickBoot.h"

struct Stage
{
    const char* name;
    const char* light;
    const char* sky;
    std::initializer_list<const char*> archiveNames;
};

static const Stage s_stages[] =
{
#include "RaytracingStages.h"
};

static size_t s_stageIndex = 0;

static boost::shared_ptr<Hedgehog::Database::CDatabase> s_database;
static boost::shared_ptr<Hedgehog::Database::CRawData> s_sceneEffect;

static void loadArchiveDatabase(const Stage& stage)
{
    s_database = Hedgehog::Database::CDatabase::CreateDatabase();
    auto& loader = Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spDatabaseLoader;

    static Hedgehog::Base::CSharedString s_ar(".ar");
    static Hedgehog::Base::CSharedString s_arl(".arl");

    for (const auto archiveName : stage.archiveNames)
    {
        loader->CreateArchiveList(
            archiveName + s_ar, 
            archiveName + s_arl, 
            { 200, 5 });
    }

    for (const auto archiveName : stage.archiveNames)
        loader->LoadArchiveList(s_database, archiveName + s_arl);

    for (const auto archiveName : stage.archiveNames)
        loader->LoadArchive(s_database, archiveName + s_ar, { -10, 5 }, false, false);

    Hedgehog::Mirage::CMirageDatabaseWrapper wrapper(s_database.get());

    if (stage.light)
        RaytracingParams::s_light = wrapper.GetLightData(stage.light);

    if (stage.sky)
        RaytracingParams::s_sky = wrapper.GetModelData(stage.sky);

    s_sceneEffect = s_database->GetRawData("SceneEffect.prm.xml");
}

static inline FUNCTION_PTR(void, __stdcall, createXmlDocument, 0x1101570,
    const Hedgehog::Base::CSharedString& name, boost::anonymous_shared_ptr& xmlDocument, boost::shared_ptr<Hedgehog::Database::CDatabase> database);

static void parseXmlDocument(const Hedgehog::Base::CSharedString* name, void* parameterEditor, void* xmlElement, size_t priority)
{
    static uint32_t parseXmlDocumentAddr = 0x11021F0;

    __asm
    {
        push priority
        push xmlElement
        push parameterEditor
        mov eax, name
        call[parseXmlDocumentAddr]
    }
}

static inline FUNCTION_PTR(void, __cdecl, refreshGlobalParameters, 0xD64710);

static bool loadSceneEffect()
{
    if (!s_sceneEffect || !s_sceneEffect->IsMadeAll())
        return false;

    typedef void* __fastcall Function(size_t);
    const auto& parameterEditor = Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spParameterEditor;

    for (const auto& parameter : parameterEditor->m_spGlobalParameterManager->m_GlobalParameterFileList)
    {
        if (parameter->m_Name == "SceneEffect.prm.xml")
        {
            const size_t virtualFunctionTable = *reinterpret_cast<size_t*>(parameter.get());
            const size_t function = *reinterpret_cast<size_t*>(virtualFunctionTable + 0x14);

            reinterpret_cast<Function*>(function)(reinterpret_cast<size_t>(parameter.get())); // resets parameter overrides

            break;
        }
    }

    Hedgehog::Base::CSharedString str("SceneEffect.prm.xml");
    boost::anonymous_shared_ptr xmlDocument;

    createXmlDocument(str, xmlDocument, s_database);

    if (xmlDocument)
    {
        const size_t virtualFunctionTable = **static_cast<size_t**>(xmlDocument.get());
        const size_t function = *reinterpret_cast<size_t*>(virtualFunctionTable + 0x30);

        void* xmlElement = reinterpret_cast<Function*>(function)(*static_cast<size_t*>(xmlDocument.get())); // gets the xml element

        parseXmlDocument(
            &str,
            parameterEditor.get(),
            &xmlElement,
            0);
    }

    s_sceneEffect = nullptr;
    return true;
}

static boost::shared_ptr<Sonic::CParameterFile> s_parameterFile;

static void createParameterFile()
{
    if (s_parameterFile)
        return;

    const auto appDocument = Sonic::CApplicationDocument::GetInstance();
    if (!appDocument)
        return;

    s_parameterFile = appDocument->m_pMember->m_spParameterEditor->m_spGlobalParameterManager->CreateParameterFile("Raytracing", "");

    auto paramGroup = s_parameterFile->CreateParameterGroup("Default", "");

    auto commonParam = paramGroup->CreateParameterCategory("Common", "");
    {
        commonParam->CreateParamBool(&RaytracingParams::s_enable, "Enable");
        commonParam->CreateParamTypeList(&RaytracingParams::s_debugView, "DebugView", "", {
            { "None", DEBUG_VIEW_NONE },
            { "Diffuse", DEBUG_VIEW_DIFFUSE },
            { "Specular", DEBUG_VIEW_SPECULAR },
            { "Normal", DEBUG_VIEW_NORMAL },
            { "Falloff", DEBUG_VIEW_FALLOFF },
            { "Emission", DEBUG_VIEW_EMISSION },
            { "Shadow", DEBUG_VIEW_SHADOW },
            { "GI", DEBUG_VIEW_GI },
            { "Reflection", DEBUG_VIEW_REFLECTION },
            { "Refraction", DEBUG_VIEW_REFRACTION } });

        paramGroup->Flush();
    }

    auto stageParam = paramGroup->CreateParameterCategory("Stage", "");
    {
        std::vector<Sonic::SParamEnumValue> enumValues = { { "None", 0 } };
        for (size_t i = 0; i < _countof(s_stages); i++)
            enumValues.push_back({ s_stages[i].name, i + 1 });

        stageParam->CreateParamTypeList(&s_stageIndex, "Stage", "", enumValues);
        paramGroup->Flush();
    }

    auto giParam = paramGroup->CreateParameterCategory("GI", "");
    {
        giParam->CreateParamFloat(&RaytracingParams::s_diffusePower, "DiffusePower");
        giParam->CreateParamFloat(&RaytracingParams::s_lightPower, "LightPower");
        giParam->CreateParamFloat(&RaytracingParams::s_emissivePower, "EmissivePower");
        giParam->CreateParamFloat(&RaytracingParams::s_skyPower, "SkyPower");
        paramGroup->Flush();
    }

    auto envParam = paramGroup->CreateParameterCategory("Environment", "");
    {
        envParam->CreateParamTypeList(&RaytracingParams::s_envMode, "Mode", "", {
            {"Auto", ENVIRONMENT_MODE_AUTO},
            {"Sky", ENVIRONMENT_MODE_SKY},
            {"Color", ENVIRONMENT_MODE_COLOR} });

        envParam->CreateParamFloat(&RaytracingParams::s_skyColor.x(), "SkyColorR");
        envParam->CreateParamFloat(&RaytracingParams::s_skyColor.y(), "SkyColorG");
        envParam->CreateParamFloat(&RaytracingParams::s_skyColor.z(), "SkyColorB");
        envParam->CreateParamFloat(&RaytracingParams::s_groundColor.x(), "GroundColorR");
        envParam->CreateParamFloat(&RaytracingParams::s_groundColor.y(), "GroundColorG");
        envParam->CreateParamFloat(&RaytracingParams::s_groundColor.z(), "GroundColorB");
        paramGroup->Flush();
    }

    auto upscalerParam = paramGroup->CreateParameterCategory("Upscaler", "");
    {
        upscalerParam->CreateParamTypeList(&RaytracingParams::s_upscaler, "Upscaler", "", {
            {"Unspecified", 0},
            {"DLSS", 1},
            {"DLSS Ray Reconstruction", 2},
            {"FSR2", 3} });

        upscalerParam->CreateParamTypeList(&RaytracingParams::s_qualityMode, "QualityMode", "", {
            {"Unspecified", 0},
            {"Native", 1},
            {"Quality", 2},
            {"Balanced", 3},
            {"Performance", 4},
            {"Ultra Performance", 5}});

        paramGroup->Flush();
    }

    auto toneMapParam = paramGroup->CreateParameterCategory("ToneMap", "");
    {
        toneMapParam->CreateParamTypeList(&RaytracingParams::s_toneMapMode, "Mode", "", {
            {"Unspecified", TONE_MAP_MODE_UNSPECIFIED},
            {"Enable", TONE_MAP_MODE_ENABLE},
            {"Disable", TONE_MAP_MODE_DISABLE}
        });

        paramGroup->Flush();
    }
}

static size_t s_prevStageIndex;

bool RaytracingParams::update()
{
    createParameterFile();

    bool resetAccumulation = false;

    if (Configuration::s_gachaLighting && s_stageIndex == NULL)
    {
        static std::default_random_engine engine(std::random_device{}());
        static std::uniform_int_distribution<size_t> distribution(0, _countof(s_stages));

        s_stageIndex = distribution(engine);
    }

    const size_t curStageIndex = s_enable ? s_stageIndex : NULL;

    if (s_prevStageIndex != curStageIndex)
    {
        s_database = nullptr;
        s_light = nullptr;
        s_sky = nullptr;
        s_sceneEffect = nullptr;

        if (curStageIndex != 0)
        {
            loadArchiveDatabase(s_stages[curStageIndex - 1]);
        }
        else
        {
            refreshGlobalParameters();
        }

        s_prevStageIndex = curStageIndex;
        resetAccumulation = true;
    }

    resetAccumulation |= loadSceneEffect();
    return resetAccumulation;
}

void RaytracingParams::imguiWindow()
{
    if (ImGui::Begin("Generations Raytracing", &Configuration::s_enableImgui, ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save Raytracing.prm.xml"))
                {
                    FILE* file = fopen("work/Raytracing.prm.xml", "wb");
                    if (file)
                    {
                        fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n", file);
                        fputs("<Raytracing.prm.xml>\n", file);
                        fputs("  <Default>\n", file);
                        fputs("    <Category>\n", file);

                        fputs("      <GI>\n", file);
                        fputs("        <Param>\n", file);
                        fprintf(file, "          <DiffusePower>%g</DiffusePower>\n", s_diffusePower);
                        fprintf(file, "          <LightPower>%g</LightPower>\n", s_lightPower);
                        fprintf(file, "          <EmissivePower>%g</EmissivePower>\n", s_emissivePower);
                        fprintf(file, "          <SkyPower>%g</SkyPower>\n", s_skyPower);
                        fputs("        </Param>\n", file);
                        fputs("      </GI>\n", file);

                        fputs("      <Environment>\n", file);
                        fputs("        <Param>\n", file);
                        fprintf(file, "          <Mode>%d</Mode>\n", s_envMode);
                        fprintf(file, "          <SkyColorR>%g</SkyColorR>\n", s_skyColor.x());
                        fprintf(file, "          <SkyColorG>%g</SkyColorG>\n", s_skyColor.y());
                        fprintf(file, "          <SkyColorB>%g</SkyColorB>\n", s_skyColor.z());
                        fprintf(file, "          <GroundColorR>%g</GroundColorR>\n", s_groundColor.x());
                        fprintf(file, "          <GroundColorG>%g</GroundColorG>\n", s_groundColor.y());
                        fprintf(file, "          <GroundColorB>%g</GroundColorB>\n", s_groundColor.z());
                        fputs("        </Param>\n", file);
                        fputs("      </Environment>\n", file);

                        fputs("      <ToneMap>\n", file);
                        fputs("        <Param>\n", file);
                        fprintf(file, "          <Mode>%d</Mode>\n", s_toneMapMode);
                        fputs("        </Param>\n", file);
                        fputs("      </ToneMap>\n", file);

                        fputs("    </Category>\n", file);
                        fputs("  </Default>\n", file);
                        fputs("</Raytracing.prm.xml>\n", file);

                        fclose(file);
                    }
                }
                ImGui::Separator();

                if (ImGui::MenuItem("Close"))
                    Configuration::s_enableImgui = false;

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        if (ImGui::BeginTabBar("TabBar"))
        {
            if (ImGui::BeginTabItem("Upscaling"))
            {
                if (ImGui::BeginChild("Child"))
                {
                    if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed);
                        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Upscaler");
                        ImGui::TableNextColumn();
                        ImGui::RadioButton("Unspecified##Upscaler", reinterpret_cast<int*>(&s_upscaler), 0);
                        ImGui::RadioButton("DLSS", reinterpret_cast<int*>(&s_upscaler), 1);
                        ImGui::RadioButton("DLSS Ray Reconstruction", reinterpret_cast<int*>(&s_upscaler), 2);
                        ImGui::RadioButton("FSR2", reinterpret_cast<int*>(&s_upscaler), 3);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Quality Mode");
                        ImGui::TableNextColumn();
                        ImGui::RadioButton("Unspecified##Quality Mode", reinterpret_cast<int*>(&s_qualityMode), 0);
                        ImGui::RadioButton("Native", reinterpret_cast<int*>(&s_qualityMode), 1);
                        ImGui::RadioButton("Quality", reinterpret_cast<int*>(&s_qualityMode), 2);
                        ImGui::RadioButton("Balanced", reinterpret_cast<int*>(&s_qualityMode), 3);
                        ImGui::RadioButton("Performance", reinterpret_cast<int*>(&s_qualityMode), 4);
                        ImGui::RadioButton("Ultra Performance", reinterpret_cast<int*>(&s_qualityMode), 5);

                        ImGui::EndTable();
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Stage"))
            {
                if (ImGui::BeginChild("Child"))
                {
                    ImGui::RadioButton("None", reinterpret_cast<int*>(&s_stageIndex), NULL);

                    for (int i = 0; i < _countof(s_stages); i++)
                        ImGui::RadioButton(s_stages[i].name, reinterpret_cast<int*>(&s_stageIndex), i + 1);
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Parameter"))
            {
                if (ImGui::BeginChild("Child"))
                {
                    if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed);
                        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Tone Mapping");
                        ImGui::TableNextColumn();
                        ImGui::RadioButton("Unspecified", reinterpret_cast<int*>(&s_toneMapMode), TONE_MAP_MODE_UNSPECIFIED);
                        ImGui::RadioButton("Enable##Tone Mapping", reinterpret_cast<int*>(&s_toneMapMode), TONE_MAP_MODE_ENABLE);
                        ImGui::RadioButton("Disable", reinterpret_cast<int*>(&s_toneMapMode), TONE_MAP_MODE_DISABLE);

                        static constexpr float s_speed = 0.01f;
                        static constexpr float s_min = 1.0f;
                        static constexpr float s_max = FLT_MAX;
                        static constexpr const char* s_format = "%g";

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Diffuse Power");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##Diffuse Power", &s_diffusePower, s_speed, s_min, s_max, s_format);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Light Power");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##Light Power", &s_lightPower, s_speed, s_min, s_max, s_format);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Emissive Power");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##Emissive Power", &s_emissivePower, s_speed, s_min, s_max, s_format);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Sky Power");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::DragFloat("##Sky Power", &s_skyPower, s_speed, s_min, s_max, s_format);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Environment Mode");
                        ImGui::TableNextColumn();
                        ImGui::RadioButton("Auto", reinterpret_cast<int*>(&s_envMode), ENVIRONMENT_MODE_AUTO);
                        ImGui::RadioButton("Sky", reinterpret_cast<int*>(&s_envMode), ENVIRONMENT_MODE_SKY);
                        ImGui::RadioButton("Color", reinterpret_cast<int*>(&s_envMode), ENVIRONMENT_MODE_COLOR);
                        ImGui::SameLine();

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Sky Color");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::ColorEdit3("##Sky Color", s_skyColor.data());

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Ground Color");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::ColorEdit3("##Ground Color", s_groundColor.data());

                        ImGui::EndTable();
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Debug"))
            {
                if (ImGui::BeginChild("Child"))
                {
                    if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed);
                        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Enable");
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("##Enable", &s_enable);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("View");
                        ImGui::TableNextColumn();
                        ImGui::RadioButton("None", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_NONE);
                        ImGui::RadioButton("Diffuse", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_DIFFUSE);
                        ImGui::RadioButton("Specular", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_SPECULAR);
                        ImGui::RadioButton("Normal", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_NORMAL);
                        ImGui::RadioButton("Falloff", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_FALLOFF);
                        ImGui::RadioButton("Emission", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_EMISSION);
                        ImGui::RadioButton("Shadow", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_SHADOW);
                        ImGui::RadioButton("GI", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_GI);
                        ImGui::RadioButton("Reflection", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_REFLECTION);
                        ImGui::RadioButton("Refraction", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_REFRACTION);

                        ImGui::EndTable();
                    }

                    if (ImGui::Button("Switch To Environment Color Picking Mode (Requires Debug Shaders)"))
                    {
                        *reinterpret_cast<bool*>(0x1A421FB) = false;
                        *reinterpret_cast<bool*>(0x1A4358E) = false;
                        *reinterpret_cast<bool*>(0x1A4323D) = false;
                        *reinterpret_cast<bool*>(0x1A4358D) = false;
                        *reinterpret_cast<bool*>(0x1E5E333) = false;
                        *reinterpret_cast<bool*>(0x1B22E8C) = false;
                        *reinterpret_cast<uint32_t*>(0x1E5E3E0) = 1;
                        s_enable = false;
                        s_toneMapMode = TONE_MAP_MODE_DISABLE;
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Copy EnvironmentColor.cpp Array Item To Clipboard"))
                    {
                        const auto gameDocument = Sonic::CGameDocument::GetInstance();
                        if (gameDocument)
                        {
                            const auto lightManager = gameDocument->m_pMember->m_spLightManager;
                            if (lightManager)
                            {
                                XXH64_hash_t hash = 0;
                                hash = XXH64(lightManager->m_GlobalLightDirection.data(), 12, hash);
                                hash = XXH64(lightManager->m_GlobalLightDiffuse.data(), 12, hash);

                                char text[0x400];
                                sprintf(text, "{ 0x%llx, { 0, 0, 0 } }, // %s", hash, gameDocument->m_pMember->m_StageName.c_str());

                                if (OpenClipboard(nullptr))
                                {
                                    EmptyClipboard();
                                    const size_t strLen = strlen(text) + 1;
                                    const HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, strLen);
                                    memcpy(GlobalLock(hGlobal), text, strLen);
                                    SetClipboardData(CF_TEXT, hGlobal);
                                    GlobalUnlock(hGlobal);
                                    CloseClipboard();
                                }
                            }
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Quick Boot"))
            {
                if (ImGui::BeginChild("Child"))
                {
                    QuickBoot::renderImgui();
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
