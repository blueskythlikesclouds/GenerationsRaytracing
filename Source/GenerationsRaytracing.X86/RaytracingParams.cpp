#include "RaytracingParams.h"

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
    { "ActN_SubAfrica_02", "light", "afr_sky_night_000" },
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
    { "ssh103", "ssh100_Direct01", "ssh_sky" },
    { "ssh200", "ssh200_Direct01", "ssh_sky" },
    { "ssh201", "ssh200_Direct01", "ssh_sky" },
    { "ssh205", "ssh200_Direct01", "ssh_sky" },
    { "ssz100", "ssz100_Direct01", "ssz_sky_000" },
    { "ssz103", "ssz103_Direct01", "ssz103_sky" },
    { "ssz200", "ssz200_Direct01", "ssz_sky_000" },
};

static size_t s_stageIndex = 0;

static boost::shared_ptr<Hedgehog::Database::CDatabase> s_database;
static boost::shared_ptr<Hedgehog::Database::CRawData> s_sceneEffect;

static void loadArchiveDatabase(const Stage& stage)
{
    s_database = Hedgehog::Database::CDatabase::CreateDatabase();
    auto& loader = Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spDatabaseLoader;

    const std::string base = "Stage/" + std::string(stage.name);
    const std::string ar = base + ".ar";
    const std::string arl = base + ".arl";

    loader->CreateArchiveList(ar.c_str(), arl.c_str(), { 200, 5 });
    loader->LoadArchiveList(s_database, arl.c_str());
    loader->LoadArchive(s_database, ar.c_str(), { -10, 5 }, false, false);

    Hedgehog::Mirage::CMirageDatabaseWrapper wrapper(s_database.get());

    if (stage.light)
        RaytracingParams::s_light = wrapper.GetLightData(stage.light);

    if (stage.sky)
        RaytracingParams::s_sky = wrapper.GetModelData(stage.sky);

    s_sceneEffect = s_database->GetRawData("SceneEffect.prm.xml");
}

static inline FUNCTION_PTR(void, __stdcall, createXmlDocument, 0x1101570,
    const Hedgehog::base::CSharedString& name, boost::anonymous_shared_ptr& xmlDocument, boost::shared_ptr<Hedgehog::Database::CDatabase> database);

static void parseXmlDocument(const Hedgehog::base::CSharedString* name, size_t parameterEditor, void* xmlElement, size_t priority)
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

    size_t parameterEditor = *(size_t*)((size_t)Sonic::CApplicationDocument::GetInstance()->m_pMember + 0x204);
    auto& parameterList = **(Hedgehog::list<Sonic::CAbstractParameter*>**)(parameterEditor + 0xAC);

    for (auto& parameter : parameterList)
    {
        if (parameter->m_Name == "SceneEffect.prm.xml")
        {
            const size_t virtualFunctionTable = *(size_t*)parameter;
            const size_t function = *(size_t*)(virtualFunctionTable + 0x14);

            ((Function*)function)((size_t)parameter); // resets parameter overrides

            break;
        }
    }

    Hedgehog::base::CSharedString str("SceneEffect.prm.xml");
    boost::anonymous_shared_ptr xmlDocument;

    createXmlDocument(str, xmlDocument, s_database);

    if (xmlDocument)
    {
        const size_t virtualFunctionTable = **(size_t**)xmlDocument.get();
        const size_t function = *(size_t*)(virtualFunctionTable + 0x30);

        void* xmlElement = ((Function*)function)(*(size_t*)xmlDocument.get()); // gets the xml element

        parseXmlDocument(
            &str,
            parameterEditor,
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
}

static size_t s_prevStageIndex;

bool RaytracingParams::update()
{
    createParameterFile();

    bool resetAccumulation = false;
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