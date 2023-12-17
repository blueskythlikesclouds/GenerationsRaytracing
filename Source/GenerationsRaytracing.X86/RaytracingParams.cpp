#include "RaytracingParams.h"

#include "Configuration.h"
#include "EnvironmentMode.h"

struct Stage
{
    const char* name;
    const char* light;
    const char* sky;
};

static const Stage s_stages[] =
{
    { "ActD_Africa", "Direct01", "afr_sky_day" },
    { "ActD_Beach", "light", "sea_sky_day" },
    { "ActD_China", "Direct01", "chn_sky_day" },
    { "ActD_EU", "Direct01", "euc_sky_day" },
    { "ActD_MykonosAct1", "light", "myk_sky_morning" },
    { "ActD_MykonosAct2", "light", "myk_sky_day" },
    { "ActD_NY", "FDirect01", "nyc_sky_day" },
    { "ActD_Petra", "light", "ptr_sky_day" },
    { "ActD_Snow", "light", "snw_sky_day" },
    { "ActD_SubAfrica_01", "Direct01", "sky" },
    { "ActD_SubAfrica_02", "Direct01", "afr_sky_day" },
    { "ActD_SubAfrica_03", "Light01", "afr_sky_02" },
    { "ActD_SubBeach_01", "light", "sea_sky_day_sub01" },
    { "ActD_SubBeach_02", "light", "sea_sky_day" },
    { "ActD_SubBeach_03", "light", "sea_sky_day_sub04" },
    { "ActD_SubBeach_04", "light", "sea_sky_day_sub04" },
    { "ActD_SubChina_01", "Direct01", "chn_sky_day" },
    { "ActD_SubChina_02", "Direct01", "chn_sky_day" },
    { "ActD_SubChina_03", "Direct01", "chn_sky_day" },
    { "ActD_SubChina_04", "Direct01", "chn_sky_day" },
    { "ActD_SubEU_01", "Direct01", "euc_sky_eveningCloud" },
    { "ActD_SubEU_02", "Direct01", "euc_sky_day" },
    { "ActD_SubEU_03", "Direct01", "euc_sky_evening" },
    { "ActD_SubEU_04", "Direct01", "euc_sky_day" },
    { "ActD_SubMykonos_01", "light", "myk_sky_morning" },
    { "ActD_SubMykonos_02", "light", "myk_sky_morning" },
    { "ActD_SubNY_01", "FDirect01", "nyc_sky_day" },
    { "ActD_SubNY_02", "FDirect01", "nyc_sky_day" },
    { "ActD_SubPetra_02", "light", "ptr_sky_day" },
    { "ActD_SubPetra_03", "light", "ptr_sky_day" },
    { "ActD_SubSnow_01", "light", "snw_sky_day" },
    { "ActD_SubSnow_02", "light", "snw_sky_morning" },
    { "ActD_SubSnow_03", "light", "snw_sky_day" },
    { "ActN_AfricaEvil", "FDirect01", "afr_sky_night_000" },
    { "ActN_BeachEvil", "FDirect01", "sea_sky_night" },
    { "ActN_ChinaEvil", "FDirect01", "chn_sky_night" },
    { "ActN_EUEvil", "euc_night_Direct01", "euc_sky_night" },
    { "ActN_MykonosEvil", "Direct01", "evl_myk_sky_01" },
    { "ActN_NYEvil", "FDirect01", "nyc_sky_night2" },
    { "ActN_PetraEvil", "Direct01", "ptr_sky_night" },
    { "ActN_SnowEvil", "light", "snw_sky_night" },
    { "ActN_SubAfrica_01", "FDirect01", "afr_sky_night_000" },
    { "ActN_SubAfrica_03", "FDirect01", "afr_sky_night_000" },
    { "ActN_SubBeach_01", "euc_night_Direct01", "sea_sky_night" },
    { "ActN_SubChina_01", "FDirect01", "chn_sky_night" },
    { "ActN_SubChina_02", "FDirect01", "chn_sky_night" },
    { "ActN_SubEU_01", "euc_night_Direct01", "euc_evl_sky_000" },
    { "ActN_SubMykonos_01", "Direct01", "evl_myk_sky_01" },
    { "ActN_SubNY_01", "FDirect01", "nyc_sky_night2" },
    { "ActN_SubPetra_02", "Direct01", "ptr_sky_night" },
    { "ActN_SubSnow_01", "light", "snw_sky_sub01_night" },
    { "ActN_SubSnow_02", "light", "snw_sky_night" },
    { "Act_EggmanLand", "Direct02_001", "egb_sky_day" },
    { "BossDarkGaia1_1Run", "Direct01", "fnl_sky_day" },
    { "BossDarkGaia1_2Run", "light", "fnl_sky_day" },
    { "BossDarkGaia1_3Run", "Direct01", "fnl_sky_day" },
    { "BossDarkGaiaMoray", "Direct01", "snw_sky_night" },
    { "BossEggBeetle", "Direct01", "afrboss_sky_day" },
    { "BossEggLancer", "FDirect01", "sea_sky_day" },
    { "BossEggRayBird", "FDirect01", "eucboss_sky_day" },
    { "BossPetra", "Direct01", "ptrboss_sky_night" },
    { "BossPhoenix", "Direct01", "chn_sky_night" },
    { "Town_Africa", "twn_afr_day_light", "afr_sky_day" },
    { "Town_AfricaETF", "etf_afr_day_light", "afr_sky_day" },
    { "Town_AfricaETF_Night", "etf_afr_night_light", "afr_sky_night" },
    { "Town_Africa_Night", "twn_afr_night_light", "afr_sky_night" },
    { "Town_China", "twn_chn_day_light", "chn_sky_day" },
    { "Town_ChinaETF", "etf_chn_day_light", "chn_sky_day" },
    { "Town_ChinaETF_Night", "etf_chn_night_light", "chn_sky_night" },
    { "Town_China_Night", "twn_chn_night_light", "chn_sky_night" },
    { "Town_EULabo", "euc_professorroom_day_light", "euc_sky_day" },
    { "Town_EULabo_Night", "euc_professorroom_night_light", "euc_sky_night" },
    { "Town_EggManBase", "twn_egb_day_light", "egb_sky_day" },
    { "Town_EuropeanCity", "twn_euc_day_light", "euc_sky_village_day" },
    { "Town_EuropeanCityETF", "etf_euc_day_light.light", "euc_sky_01" },
    { "Town_EuropeanCityETF_Night", "etf_euc_night_light", "euc_sky_night" },
    { "Town_EuropeanCity_Night", "twn_euc_night_light", "euc_sky_night" },
    { "Town_Mykonos", "twn_myk_day_light", "myk_sky_day" },
    { "Town_MykonosETF", "light", "sky_day" },
    { "Town_MykonosETF_Night", "etf_myk_night_light", "evl_myk_sky_01" },
    { "Town_Mykonos_Night", "twn_myk_night_light", "myk_sky_night" },
    { "Town_NYCity", "twn_nyc_day_light", "nyc_sky_day" },
    { "Town_NYCityETF", "etf_nyc_day_light", "nyc_sky_day" },
    { "Town_NYCityETF_Night", "twn_nyc_night_light", "nyc_sky_night" },
    { "Town_NYCity_Night", "twn_nyc_night_light", "nyc_sky_night" },
    { "Town_PetraCapital", "twn_ptr_day_light", "ptr_sky_day" },
    { "Town_PetraCapitalETF", "etf_ptr_day_light", "ptr_sky_day" },
    { "Town_PetraCapitalETF_Night", "etf_ptr_night_light", "ptr_village_sky_night" },
    { "Town_PetraCapital_Night", "twn_ptr_night_light", "ptr_village_sky_night" },
    { "Town_PetraLabo", "labo_ptr_day_light", "ptr_sky_day" },
    { "Town_PetraLabo_Night", "labo_ptr_night_light", "ptr_sky_night" },
    { "Town_Snow", "twn_snw_day_light", "snw_sky_day" },
    { "Town_SnowETF", "etf_snw_day_light", "snw_sky_day" },
    { "Town_SnowETF_Night", "etf_snw_night_light", "snw_sky_night" },
    { "Town_Snow_Night", "twn_snw_night_light", "snw_sky_night" },
    { "Town_SouthEastAsia", "twn_sea_day_light", "sea_sky_day" },
    { "Town_SouthEastAsiaETF", "etf_sea_day_light", "sea_sky_day" },
    { "Town_SouthEastAsiaETF_Night", "etf_sea_night_light", "sea_sky_night" },
    { "Town_SouthEastAsia_Night", "twn_sea_night_light", "sea_sky_night" },
    { "bde", "Direct01", "bde_sky_0" },
    { "blb", "blb_sky", "blb_sky_000" },
    { "bms", "bms_Direct01", "bms_sky" },
    { "bne", "bne_Direct01", "bne_sky01" },
    { "bpc", "bpc_Direct00", "bpc300_sky" },
    { "bsd", "Direct01", "bsd_sky0" },
    { "bsl", "bsl_Direct00", "bsl_sky" },
    { "cnz100", "cnz_Direct00", "cnz100_sky" },
    { "cpz100", "cpz_Direct01", "cpz_sky0" },
    { "cpz200", "cpz_Direct01", "cpz_sky1" },
    { "csc100", "csc_Direct00", "csc100_sky" },
    { "csc200", "csc_Direct00", "csc200_sky" },
    { "cte100", "CTE_Direct01", "cte_sky_000" },
    { "cte102", "CTE_Direct01", "cte_sky_000" },
    { "cte200", "CTE_Direct01", "cte_sky_000" },
    { "euc100", "euc100_Direct01", "euc_sky_000" },
    { "euc200", "euc200_Direct01", "euc_sky_000" },
    { "euc204", "euc204_Direct01", "euc_sky_evening" },
    { "fig000", "fig000_Direct01", "fig_sky_000" },
    { "ghz100", "Direct01", "ghz_sky_000" },
    { "ghz103", "ghz103_Direct01", "ghz103_sky_000" },
    { "ghz104", "ghz104_Direct01", "ghz_sea_sky_night" },
    { "ghz200", "ghz200_Direct01", "ghz_sky_000" },
    { "pam000", "pam000_Direct01", "pam_sky_000" },
    { "pla100", "pla100_Direct01", "pla_sky" },
    { "pla200", "pla200_Direct01", "pla_sky" },
    { "pla204", "pla200_Direct01", "pla204_sky" },
    { "pla205", "pla200_Direct01", "pla_sky" },
    { "sph100", "sph100_Direct01", "sph100_sky00" },
    { "sph101", "sph100_Direct01", "sph_sky_sph101" },
    { "sph200", "sph03_Direct01", "sph200_sky01" },
    { "ssh100", "ssh100_Direct01", "ssh_sky" },
    { "ssh101", "ssh101_Direct01", "ssh_sky" },
    { "ssh200", "ssh200_Direct01", "ssh_sky" },
    { "ssh201", "ssh200_Direct01", "ssh_sky" },
    { "ssh205", "ssh200_Direct01", "ssh_sky" },
    { "ssz100", "ssz100_Direct01", "ssz_sky_000" },
    { "ssz103", "ssz103_Direct01", "ssz103_sky" },
    { "ssz200", "ssz200_Direct01", "ssz_sky_000" }
};

static size_t s_stageIndex = 0;

static boost::shared_ptr<Hedgehog::Database::CDatabase> s_database;
static boost::shared_ptr<Hedgehog::Database::CRawData> s_sceneEffect;

static void loadArchiveDatabase(const Stage& stage)
{
    s_database = Hedgehog::Database::CDatabase::CreateDatabase();
    auto& loader = Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spDatabaseLoader;

    const Hedgehog::Base::CSharedString base = "Stage/" + Hedgehog::Base::CSharedString(stage.name);
    const Hedgehog::Base::CSharedString ar = base + ".ar";
    const Hedgehog::Base::CSharedString arl = base + ".arl";

    loader->CreateArchiveList(ar, arl, { 200, 5 });
    loader->LoadArchiveList(s_database, arl);
    loader->LoadArchive(s_database, ar, { -10, 5 }, false, false);

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
            {"FSR2", 2} });

        upscalerParam->CreateParamTypeList(&RaytracingParams::s_qualityMode, "QualityMode", "", {
            {"Unspecified", 0},
            {"Native", 1},
            {"Quality", 2},
            {"Balanced", 3},
            {"Performance", 4},
            {"UltraPerformance", 5}});

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