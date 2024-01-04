#include "RaytracingParams.h"

#include "Configuration.h"
#include "EnvironmentMode.h"

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