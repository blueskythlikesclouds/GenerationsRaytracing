#include "RaytracingParams.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Configuration.h"
#include "EnvironmentMode.h"
#include "LightData.h"
#include "MessageSender.h"
#include "QuickBoot.h"
#include "StageSelection.h"
#include "Logger.h"
#include "RaytracingRendering.h"

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

static struct
{
    boost::shared_ptr<Hedgehog::Database::CDatabase> database;
    boost::shared_ptr<Hedgehog::Mirage::CLightData> light;
    boost::shared_ptr<Hedgehog::Mirage::CModelData> sky;
    boost::shared_ptr<Hedgehog::Database::CRawData> sceneEffect;
} s_stage;

static void loadArchiveDatabase(const Stage& stage)
{
    s_stage.database = Hedgehog::Database::CDatabase::CreateDatabase();
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
        loader->LoadArchiveList(s_stage.database, archiveName + s_arl);

    for (const auto archiveName : stage.archiveNames)
        loader->LoadArchive(s_stage.database, archiveName + s_ar, { -10, 5 }, false, false);

    Hedgehog::Mirage::CMirageDatabaseWrapper wrapper(s_stage.database.get());

    s_stage.light = wrapper.GetLightData(stage.light);
    s_stage.sky = wrapper.GetModelData(stage.sky);
    s_stage.sceneEffect = s_stage.database->GetRawData("SceneEffect.prm.xml");
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

static void loadSceneEffect()
{
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

    createXmlDocument(str, xmlDocument, s_stage.database);

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
        envParam->CreateParamBool(&RaytracingParams::s_skyInRoughReflection, "SkyInRoughReflection");
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

    if (Configuration::s_gachaLighting && 
        StageSelection::s_stageIndex != nullptr && *StageSelection::s_stageIndex == NULL)
    {
        static std::default_random_engine engine(std::random_device{}());
        static std::uniform_int_distribution<size_t> distribution(0, _countof(s_stages));

        *StageSelection::s_stageIndex = distribution(engine);
    }

    const size_t curStageIndex = s_enable && StageSelection::s_stageIndex != nullptr ? 
        *StageSelection::s_stageIndex : NULL;

    if (s_prevStageIndex != curStageIndex)
    {
        if (curStageIndex != 0)
        {
            loadArchiveDatabase(s_stages[curStageIndex - 1]);
        }
        else
        {
            s_light = nullptr;
            s_sky = nullptr;
            refreshGlobalParameters();
            s_stage = {};
            resetAccumulation = true;
        }

        s_prevStageIndex = curStageIndex;
    }

    if (s_stage.database != nullptr &&
        s_stage.light->IsMadeAll() &&
        s_stage.sky->IsMadeAll() &&
        s_stage.sceneEffect->IsMadeAll())
    {
        s_light = s_stage.light;
        s_sky = s_stage.sky;
        loadSceneEffect();
        s_stage = {};
        resetAccumulation = true;
    }

    return resetAccumulation;
}

constexpr size_t s_durationNum = 120;
static double s_x86Durations[s_durationNum];
static double s_x64Durations[s_durationNum];
static double s_sceneTraverseDurations[s_durationNum];
static size_t s_durationIndex = 0;

void RaytracingParams::imguiWindow()
{
    s_prevComputeSmoothNormals = s_computeSmoothNormals;

    if (ImGui::Begin("Generations Raytracing", &Configuration::s_enableImgui, ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (Configuration::s_enableRaytracing)
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
                            fprintf(file, "          <SkyInRoughReflection>%s</SkyInRoughReflection>\n", s_skyInRoughReflection ? "true" : "false");
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
                }

                if (ImGui::MenuItem("Close"))
                    Configuration::s_enableImgui = false;

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        if (ImGui::BeginTabBar("TabBar"))
        {
            if (Configuration::s_enableRaytracing && ImGui::BeginTabItem("Upscaling"))
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
                        ImGui::RadioButton("FSR3", reinterpret_cast<int*>(&s_upscaler), 3);

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
            if (Configuration::s_enableRaytracing && ImGui::BeginTabItem("Stage"))
            {
                if (StageSelection::s_rememberSelection != nullptr)
                {
                    bool rememberSelection = *StageSelection::s_rememberSelection;
                    if (ImGui::Checkbox("Remember My Selection", &rememberSelection))
                        *StageSelection::s_rememberSelection = rememberSelection;
                }

                if (ImGui::BeginChild("Child"))
                {
                    if (StageSelection::s_stageIndex != nullptr)
                    {
                        int stageIndex = static_cast<int>(*StageSelection::s_stageIndex);
                        ImGui::RadioButton("None", &stageIndex, NULL);

                        for (int i = 0; i < _countof(s_stages); i++)
                            ImGui::RadioButton(s_stages[i].name, &stageIndex, i + 1);

                        *StageSelection::s_stageIndex = static_cast<uint32_t>(stageIndex);
                    }
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

                        if (Configuration::s_enableRaytracing)
                        {
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

                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted("Sky in Rough Reflection");
                            ImGui::TableNextColumn();
                            ImGui::Checkbox("##Sky in Rough Reflection", &RaytracingParams::s_skyInRoughReflection);
                        }

                        ImGui::EndTable();
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (Configuration::s_enableRaytracing && ImGui::BeginTabItem("Debug"))
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
                        ImGui::TextUnformatted("No AO Model");
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("##No AO Model", &RaytracingParams::s_enableNoAoModels);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Smooth Normal");
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("##Smooth Normal", &RaytracingParams::s_computeSmoothNormals);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("View");
                        ImGui::TableNextColumn();
                        ImGui::RadioButton("None", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_NONE);
                        ImGui::RadioButton("Diffuse", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_DIFFUSE);
                        ImGui::RadioButton("Specular", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_SPECULAR);
                        ImGui::RadioButton("Specular Tint", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_SPECULAR_TINT);
                        ImGui::RadioButton("Specular Environment", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_SPECULAR_ENVIRONMENT);
                        ImGui::RadioButton("Specular Gloss", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_SPECULAR_GLOSS);
                        ImGui::RadioButton("Specular Level", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_SPECULAR_LEVEL);
                        ImGui::RadioButton("Specular Fresnel", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_SPECULAR_FRESNEL);
                        ImGui::RadioButton("Normal", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_NORMAL);
                        ImGui::RadioButton("Falloff", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_FALLOFF);
                        ImGui::RadioButton("Emission", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_EMISSION);
                        ImGui::RadioButton("Trans Color", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_TRANS_COLOR);
                        ImGui::RadioButton("Refraction", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_REFRACTION);
                        ImGui::RadioButton("Refraction Overlay", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_REFRACTION_OVERLAY);
                        ImGui::RadioButton("Refraction Offset", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_REFRACTION_OFFSET);
                        ImGui::RadioButton("GI", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_GI);
                        ImGui::RadioButton("Reflection", reinterpret_cast<int*>(&s_debugView), DEBUG_VIEW_REFLECTION);

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

            if (Configuration::s_enableRaytracing && ImGui::BeginTabItem("Light Editor"))
                LightData::renderImgui();

            if (ImGui::BeginTabItem("Quick Boot"))
            {
                if (ImGui::BeginChild("Child"))
                {
                    QuickBoot::renderImgui();
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Profiler"))
            {
                if (ImGui::BeginChild("Child"))
                {
                    s_x86Durations[s_durationIndex] = s_messageSender.getX86Duration();
                    s_x64Durations[s_durationIndex] = s_messageSender.getX64Duration();
                    s_sceneTraverseDurations[s_durationIndex] = RaytracingRendering::s_duration;

                    if (ImPlot::BeginPlot("Frametimes"))
                    {
                        ImPlot::SetupAxisLimits(ImAxis_Y1, 8.0, 32.0);
                        ImPlot::SetupAxis(ImAxis_Y1, "ms", ImPlotAxisFlags_AutoFit);
                        ImPlot::PlotLine<double>("X86 Process", s_x86Durations, s_durationNum, 1.0, 0.0, ImPlotLineFlags_None, s_durationIndex);
                        ImPlot::PlotLine<double>("X64 Process", s_x64Durations, s_durationNum, 1.0, 0.0, ImPlotLineFlags_None, s_durationIndex);

                        if (Configuration::s_enableRaytracing && RaytracingParams::s_enable)
                            ImPlot::PlotLine<double>("Scene Traverse", s_sceneTraverseDurations, s_durationNum, 1.0, 0.0, ImPlotLineFlags_None, s_durationIndex);

                        ImPlot::EndPlot();
                    }

                    s_durationIndex = (s_durationIndex + 1) % s_durationNum;

                    const double x86DurationAvg = std::accumulate(s_x86Durations, s_x86Durations + s_durationNum, 0.0) / s_durationNum;
                    const double x64DurationAvg = std::accumulate(s_x64Durations, s_x64Durations + s_durationNum, 0.0) / s_durationNum;

                    ImGui::Text("Average X86 Process: %g ms (%g FPS)", x86DurationAvg, 1000.0 / x86DurationAvg);
                    ImGui::Text("Average X64 Process: %g ms (%g FPS)", x64DurationAvg, 1000.0 / x64DurationAvg);

                    if (Configuration::s_enableRaytracing && RaytracingParams::s_enable)
                    {
                        const double sceneTraverseDurationAvg = std::accumulate(s_sceneTraverseDurations, s_sceneTraverseDurations + s_durationNum, 0.0) / s_durationNum;
                        ImGui::Text("Average Scene Traverse: %g ms (%g FPS)", sceneTraverseDurationAvg, 1000.0 / sceneTraverseDurationAvg);
                    }

                    ImGui::Text("Vertex Buffer Wasted Memory: %g MB", static_cast<double>(VertexBuffer::s_wastedMemory) / (1024.0 * 1024.0));
                    ImGui::Text("Index Buffer Wasted Memory: %g MB", static_cast<double>(IndexBuffer::s_wastedMemory) / (1024.0 * 1024.0));
                    ImGui::Text("Memory Mapped File Committed Size: %g MB", static_cast<double>(s_messageSender.getLastCommittedSize()) / (1024.0 * 1024.0));
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Logs"))
            {
                if (ImGui::BeginChild("Child"))
                    Logger::renderImgui();

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
