#include "Stage.h"

static const std::unordered_map<std::string, std::pair<const char*, const char*>> STAGES =
{
    { "ActD_Africa", { "Direct01", "afr_sky_day" } },
    { "ActD_Beach", { "light", "sea_sky_day" } },
    { "ActD_China", { "Direct01", "chn_sky_day" } },
    { "ActD_EU", { "Direct01", "euc_sky_day" } },
    { "ActD_MykonosAct1", { "light", "myk_sky_morning" } },
    { "ActD_MykonosAct2", { "light", "myk_sky_day" } },
    { "ActD_NY", { "FDirect01", "nyc_sky_day" } },
    { "ActD_Petra", { "light", "ptr_sky_day" } },
    { "ActD_Snow", { "light", "snw_sky_day" } },
    { "ActD_SubAfrica_01", { "Direct01", "sky" } },
    { "ActD_SubAfrica_02", { "Direct01", "afr_sky_day" } },
    { "ActD_SubAfrica_03", { "Light01", "afr_sky_02" } },
    { "ActD_SubBeach_01", { "light", "sea_sky_day_sub01" } },
    { "ActD_SubBeach_02", { "light", "sea_sky_day" } },
    { "ActD_SubBeach_03", { "light", "sea_sky_day_sub04" } },
    { "ActD_SubBeach_04", { "light", "sea_sky_day_sub04" } },
    { "ActD_SubChina_01", { "Direct01", "chn_sky_day" } },
    { "ActD_SubChina_02", { "Direct01", "chn_sky_day" } },
    { "ActD_SubChina_03", { "Direct01", "chn_sky_day" } },
    { "ActD_SubChina_04", { "Direct01", "chn_sky_day" } },
    { "ActD_SubEU_01", { "Direct01", "euc_sky_eveningCloud" } },
    { "ActD_SubEU_02", { "Direct01", "euc_sky_day" } },
    { "ActD_SubEU_03", { "Direct01", "euc_sky_evening" } },
    { "ActD_SubEU_04", { "Direct01", "euc_sky_day" } },
    { "ActD_SubMykonos_01", { "light", "myk_sky_morning" } },
    { "ActD_SubMykonos_02", { "light", "myk_sky_morning" } },
    { "ActD_SubNY_01", { "FDirect01", "nyc_sky_day" } },
    { "ActD_SubNY_02", { "FDirect01", "nyc_sky_day" } },
    { "ActD_SubPetra_02", { "light", "ptr_sky_day" } },
    { "ActD_SubPetra_03", { "light", "ptr_sky_day" } },
    { "ActD_SubSnow_01", { "light", "snw_sky_day" } },
    { "ActD_SubSnow_02", { "light", "snw_sky_morning" } },
    { "ActD_SubSnow_03", { "light", "snw_sky_day" } },
    { "ActN_AfricaEvil", { "FDirect01", "afr_sky_night_000" } },
    { "ActN_BeachEvil", { "FDirect01", "sea_sky_night" } },
    { "ActN_ChinaEvil", { "FDirect01", "chn_sky_night" } },
    { "ActN_EUEvil", { "euc_night_Direct01", "euc_sky_night" } },
    { "ActN_MykonosEvil", { "Direct01", "evl_myk_sky_01" } },
    { "ActN_NYEvil", { "FDirect01", "nyc_sky_night2" } },
    { "ActN_PetraEvil", { "Direct01", "ptr_sky_night" } },
    { "ActN_SnowEvil", { "light", "snw_sky_night" } },
    { "ActN_SubAfrica_01", { "FDirect01", "afr_sky_night_000" } },
    { "ActN_SubAfrica_02", { "light", "afr_sky_night_000" } },
    { "ActN_SubAfrica_03", { "FDirect01", "afr_sky_night_000" } },
    { "ActN_SubBeach_01", { "euc_night_Direct01", "sea_sky_night" } },
    { "ActN_SubChina_01", { "FDirect01", "chn_sky_night" } },
    { "ActN_SubChina_02", { "FDirect01", "chn_sky_night" } },
    { "ActN_SubEU_01", { "euc_night_Direct01", "euc_evl_sky_000" } },
    { "ActN_SubMykonos_01", { "Direct01", "evl_myk_sky_01" } },
    { "ActN_SubNY_01", { "FDirect01", "nyc_sky_night2" } },
    { "ActN_SubPetra_02", { "Direct01", "ptr_sky_night" } },
    { "ActN_SubSnow_01", { "light", "snw_sky_sub01_night" } },
    { "ActN_SubSnow_02", { "light", "snw_sky_night" } },
    { "Act_EggmanLand", { "Direct02_001", "egb_sky_day" } },
    { "BossDarkGaia1_1Air", { "Direct01", nullptr } },
    { "BossDarkGaia1_1Run", { "Direct01", "fnl_sky_day" } },
    { "BossDarkGaia1_2Run", { "light", "fnl_sky_day" } },
    { "BossDarkGaia1_3Run", { "Direct01", "fnl_sky_day" } },
    { "BossDarkGaiaMoray", { "Direct01", "snw_sky_night" } },
    { "BossEggBeetle", { "Direct01", "afrboss_sky_day" } },
    { "BossEggDragoon", { "Direct01", nullptr } },
    { "BossEggLancer", { "FDirect01", "sea_sky_day" } },
    { "BossEggRayBird", { "FDirect01", "eucboss_sky_day" } },
    { "BossFinalDarkGaia", { "Direct01", nullptr } },
    { "BossPetra", { "Direct01", "ptrboss_sky_night" } },
    { "BossPhoenix", { "Direct01", "chn_sky_night" } },
    { "Event_M2_01_professor_room_new", { "Direct01", "euc_sky_day" } },
    { "Event_M4_01_egb_hideout", { "FDirect01", nullptr } },
    { "Event_M6_01_temple", { "FDirect01", "sea_sky_day" } },
    { "Event_M8_02_egbHall", { "event_m8-02_light", nullptr } },
    { "Event_M8_03", { "Direct01", nullptr } },
    { "Event_M8_16_myk", { "light", "myk_sky_morning02" } },
    { "Event_afr_hideout", { "FDirect01", nullptr } },
    { "Event_egb_hidout_exterior", { "Direct01", "egb_sky_evening" } },
    { "Event_temple", { "FDirect01", nullptr } },
    { "ExStageTails1", { nullptr, nullptr } },
    { "ExStageTails2", { nullptr, nullptr } },
    { "Town_Africa", { "twn_afr_day_light", "afr_sky_day" } },
    { "Town_AfricaETF", { "etf_afr_day_light", "afr_sky_day" } },
    { "Town_AfricaETF_Night", { "etf_afr_night_light", "afr_sky_night" } },
    { "Town_Africa_Night", { "twn_afr_night_light", "afr_sky_night" } },
    { "Town_China", { "twn_chn_day_light", "chn_sky_day" } },
    { "Town_ChinaETF", { "etf_chn_day_light", "chn_sky_day" } },
    { "Town_ChinaETF_Night", { "etf_chn_night_light", "chn_sky_night" } },
    { "Town_China_Night", { "twn_chn_night_light", "chn_sky_night" } },
    { "Town_EULabo", { "euc_professorroom_day_light", "euc_sky_day" } },
    { "Town_EULabo_Night", { "euc_professorroom_night_light", "euc_sky_night" } },
    { "Town_EggManBase", { "twn_egb_day_light", "egb_sky_day" } },
    { "Town_EuropeanCity", { "twn_euc_day_light", "euc_sky_village_day" } },
    { "Town_EuropeanCityETF", { "etf_euc_day_light.light", "euc_sky_01" } },
    { "Town_EuropeanCityETF_Night", { "etf_euc_night_light", "euc_sky_night" } },
    { "Town_EuropeanCity_Night", { "twn_euc_night_light", "euc_sky_night" } },
    { "Town_Mykonos", { "twn_myk_day_light", "myk_sky_day" } },
    { "Town_MykonosETF", { "light", "sky_day" } },
    { "Town_MykonosETF_Night", { "etf_myk_night_light", "evl_myk_sky_01" } },
    { "Town_Mykonos_Night", { "twn_myk_night_light", "myk_sky_night" } },
    { "Town_NYCity", { "twn_nyc_day_light", "nyc_sky_day" } },
    { "Town_NYCityETF", { "etf_nyc_day_light", "nyc_sky_day" } },
    { "Town_NYCityETF_Night", { "twn_nyc_night_light", "nyc_sky_night" } },
    { "Town_NYCity_Night", { "twn_nyc_night_light", "nyc_sky_night" } },
    { "Town_PetraCapital", { "twn_ptr_day_light", "ptr_sky_day" } },
    { "Town_PetraCapitalETF", { "etf_ptr_day_light", "ptr_sky_day" } },
    { "Town_PetraCapitalETF_Night", { "etf_ptr_night_light", "ptr_village_sky_night" } },
    { "Town_PetraCapital_Night", { "twn_ptr_night_light", "ptr_village_sky_night" } },
    { "Town_PetraLabo", { "labo_ptr_day_light", "ptr_sky_day" } },
    { "Town_PetraLabo_Night", { "labo_ptr_night_light", "ptr_sky_night" } },
    { "Town_Snow", { "twn_snw_day_light", "snw_sky_day" } },
    { "Town_SnowETF", { "etf_snw_day_light", "snw_sky_day" } },
    { "Town_SnowETF_Night", { "etf_snw_night_light", "snw_sky_night" } },
    { "Town_Snow_Night", { "twn_snw_night_light", "snw_sky_night" } },
    { "Town_SouthEastAsia", { "twn_sea_day_light", "sea_sky_day" } },
    { "Town_SouthEastAsiaETF", { "etf_sea_day_light", "sea_sky_day" } },
    { "Town_SouthEastAsiaETF_Night", { "etf_sea_night_light", "sea_sky_night" } },
    { "Town_SouthEastAsia_Night", { "twn_sea_night_light", "sea_sky_night" } },
    { "bde", { "Direct01", "bde_sky_0" } },
    { "blb", { "blb_sky", "blb_sky_000" } },
    { "bms", { "bms_Direct01", "bms_sky" } },
    { "bne", { "bne_Direct01", "bne_sky01" } },
    { "bpc", { "bpc_Direct00", "bpc300_sky" } },
    { "bsd", { "Direct01", "bsd_sky0" } },
    { "bsl", { "bsl_Direct00", "bsl_sky" } },
    { "cnz100", { "cnz_Direct00", "cnz100_sky" } },
    { "cpz100", { "cpz_Direct01", "cpz_sky0" } },
    { "cpz200", { "cpz_Direct01", "cpz_sky1" } },
    { "csc100", { "csc_Direct00", "csc100_sky" } },
    { "csc200", { "csc_Direct00", "csc200_sky" } },
    { "cte100", { "CTE_Direct01", "cte_sky_000" } },
    { "cte102", { nullptr, nullptr } },
    { "cte200", { "CTE_Direct01", "cte_sky_000" } },
    { "euc100", { "euc100_Direct01", "euc_sky_000" } },
    { "euc200", { "euc200_Direct01", "euc_sky_000" } },
    { "euc204", { "euc204_Direct01", "euc_sky_evening" } },
    { "evt041", { "ev041_Direct00", nullptr } },
    { "evt121", { "evt121_Direct01", "pam_sky_000" } },
    { "fig000", { "fig000_Direct01", "fig_sky_000" } },
    { "ghz100", { "Direct01", "ghz_sky_000" } },
    { "ghz103", { "ghz103_Direct01", "ghz103_sky_000" } },
    { "ghz104", { "ghz104_Direct01", "ghz_sea_sky_night" } },
    { "ghz200", { "ghz200_Direct01", "ghz_sky_000" } },
    { "pam000", { "pam000_Direct01", "pam_sky_000" } },
    { "pam001", { "pam001_Direct01", nullptr } },
    { "pla100", { "pla100_Direct01", "pla_sky" } },
    { "pla200", { "pla200_Direct01", "pla_sky" } },
    { "pla204", { "pla200_Direct01", "pla204_sky" } },
    { "pla205", { "pla200_Direct01", "pla_sky" } },
    { "sph100", { "sph100_Direct01", "sph100_sky00" } },
    { "sph101", { "sph100_Direct01", "sph_sky_sph101" } },
    { "sph200", { "sph03_Direct01", "sph200_sky01" } },
    { "ssh100", { "ssh100_Direct01", "ssh_sky" } },
    { "ssh103", { "ssh100_Direct01", "ssh_sky" } },
    { "ssh200", { "ssh200_Direct01", "ssh_sky" } },
    { "ssh201", { "ssh200_Direct01", "ssh_sky" } },
    { "ssh205", { "ssh200_Direct01", "ssh_sky" } },
    { "ssz100", { "ssz100_Direct01", "ssz_sky_000" } },
    { "ssz103", { "ssz103_Direct01", "ssz103_sky" } },
    { "ssz200", { "ssz200_Direct01", "ssz_sky_000" } },
};

static boost::shared_ptr<hh::db::CDatabase> database;

static inline FUNCTION_PTR(void, __stdcall, createXmlDocument, 0x1101570,
    const hh::base::CSharedString& name, boost::anonymous_shared_ptr& xmlDocument, boost::shared_ptr<hh::db::CDatabase> database);

static void parseXmlDocument(const hh::base::CSharedString* name, size_t parameterEditor, void* xmlElement, size_t priority)
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

static boost::shared_ptr<hh::db::CRawData> sceneEffect;

bool Stage::setSceneEffect()
{
    if (!sceneEffect || !sceneEffect->IsMadeAll())
        return false;

    typedef void* __fastcall Function(size_t);

    size_t parameterEditor = *(size_t*)((size_t)Sonic::CApplicationDocument::GetInstance()->m_pMember + 0x204);
    auto& parameterList = **(hh::list<Sonic::CAbstractParameter*>**)(parameterEditor + 0xAC);

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

    hh::base::CSharedString str("SceneEffect.prm.xml");
    boost::anonymous_shared_ptr xmlDocument;

    createXmlDocument(str, xmlDocument, database);

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

    sceneEffect = nullptr;
    return true;
}

void Stage::setStage(size_t index)
{
    index %= STAGES.size();

    for (auto& [name, info] : STAGES)
    {
        if ((index--) == 0)
        {
            setStage(name);
            break;
        }
    }
}

void Stage::setStage(const std::string& name)
{
    database = nullptr;
    light = nullptr;
    sky = nullptr;

    const auto pair = STAGES.find(name);
    if (pair == STAGES.end())
        return;

    database = hh::db::CDatabase::CreateDatabase();
    auto& loader = Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spDatabaseLoader;

    const std::string base = "Stage/" + name;
    const std::string ar = base + ".ar";
    const std::string arl = base + ".arl";

    loader->CreateArchiveList(ar.c_str(), arl.c_str(), { 0, 0 });

    loader->LoadArchiveList(database, arl.c_str());
    loader->LoadArchive(database, ar.c_str(), { 0, 0 }, false, false);

    hh::mr::CMirageDatabaseWrapper wrapper(database.get());

    if (pair->second.first) 
        light = wrapper.GetLightData(pair->second.first);

    if (pair->second.second)
        sky = wrapper.GetModelData(pair->second.second);

    sceneEffect = database->GetRawData("SceneEffect.prm.xml");
}
