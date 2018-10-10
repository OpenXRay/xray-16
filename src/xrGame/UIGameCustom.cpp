#include "pch_script.h"
#include "UIGameCustom.h"
#include "Level.h"
#include "ui/UIXmlInit.h"
#include "xrUICore/Static/UIStatic.h"
#include "Common/object_broker.h"
#include "string_table.h"

#include "InventoryOwner.h"
#include "ui/UIActorMenu.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIMessagesWindow.h"
#include "ui/UIHudStatesWnd.h"
#include "Actor.h"
#include "Inventory.h"
#include "game_cl_base.h"

#include "xrEngine/x_ray.h"

#include "ui/UICellItem.h" //Alundaio
//#include "script_game_object.h" //Alundaio

EGameIDs ParseStringToGameType(const char* str);

struct predicate_find_stat
{
    const char* id;
    predicate_find_stat(const char* id) { this->id = id; }
    bool operator()(StaticDrawableWrapper* s) { return s->m_name == id; }
};

CUIGameCustom::CUIGameCustom()
{
    MsgConfig = nullptr;
    ActorMenu = nullptr;
    PdaMenu = nullptr;
    Window = nullptr;
    UIMainIngameWnd = nullptr;
    m_pMessagesWnd = nullptr;
    ShowGameIndicators(true);
    ShowCrosshair(true);
}

bool g_b_ClearGameCaptions = false;

CUIGameCustom::~CUIGameCustom()
{
    delete_data(CustomStatics);
    g_b_ClearGameCaptions = false;
}

void CUIGameCustom::OnFrame()
{
    CDialogHolder::OnFrame();
    for (auto item : CustomStatics)
        item->Update();
    auto comparer = [](
        const StaticDrawableWrapper* s1, const StaticDrawableWrapper* s2) { return s1->IsActual() > s2->IsActual(); };
    std::sort(CustomStatics.begin(), CustomStatics.end(), comparer);
    while (!CustomStatics.empty() && !CustomStatics.back()->IsActual())
    {
        delete_data(CustomStatics.back());
        CustomStatics.pop_back();
    }
    if (g_b_ClearGameCaptions)
    {
        delete_data(CustomStatics);
        g_b_ClearGameCaptions = false;
    }
    Window->Update();
    // update windows
    if (GameIndicatorsShown() && psHUD_Flags.is(HUD_DRAW | HUD_DRAW_RT))
        UIMainIngameWnd->Update();
    m_pMessagesWnd->Update();
}

void CUIGameCustom::Render()
{
    for (StaticDrawableWrapper* item : CustomStatics)
        item->Draw();
    Window->Draw();
    CEntity* pEntity = smart_cast<CEntity*>(Level().CurrentEntity());
    if (pEntity)
    {
        CActor* pActor = smart_cast<CActor*>(pEntity);
        if (pActor && pActor->HUDview() && pActor->g_Alive() &&
            psHUD_Flags.is(HUD_WEAPON | HUD_WEAPON_RT | HUD_WEAPON_RT2))
        {
            CInventory& inventory = pActor->inventory();
            u16 lastSlot = inventory.LastSlot();
            for (u16 slot = inventory.FirstSlot(); slot <= lastSlot; slot++)
            {
                CInventoryItem* item = inventory.ItemFromSlot(slot);
                if (item && item->render_item_ui_query())
                    item->render_item_ui();
            }
        }
        if (GameIndicatorsShown() && psHUD_Flags.is(HUD_DRAW | HUD_DRAW_RT))
            UIMainIngameWnd->Draw();
    }
    m_pMessagesWnd->Draw();
    DoRenderDialogs();
}

StaticDrawableWrapper* CUIGameCustom::AddCustomStatic(const char* id, bool singleInstance)
{
    if (singleInstance)
    {
        auto it = std::find_if(CustomStatics.begin(), CustomStatics.end(), predicate_find_stat(id));
        if (it != CustomStatics.end())
            return *it;
    }
    CUIXmlInit xmlInit;
    CustomStatics.push_back(new StaticDrawableWrapper());
    StaticDrawableWrapper* sss = CustomStatics.back();
    sss->m_static = new CUIStatic();
    sss->m_name = id;
    xmlInit.InitStatic(*MsgConfig, id, 0, sss->m_static);
    float ttl = MsgConfig->ReadAttribFlt(id, 0, "ttl", -1.0f);
    if (ttl > 0.0f)
        sss->m_endTime = Device.fTimeGlobal + ttl;
    return sss;
}

StaticDrawableWrapper* CUIGameCustom::GetCustomStatic(const char* id)
{
    auto it = std::find_if(CustomStatics.begin(), CustomStatics.end(), predicate_find_stat(id));
    if (it != CustomStatics.end())
        return *it;
    return nullptr;
}

void CUIGameCustom::RemoveCustomStatic(const char* id)
{
    auto it = std::find_if(CustomStatics.begin(), CustomStatics.end(), predicate_find_stat(id));
    if (it != CustomStatics.end())
    {
        delete_data(*it);
        CustomStatics.erase(it);
    }
}

void CUIGameCustom::OnInventoryAction(PIItem item, u16 actionType)
{
    if (ActorMenu->IsShown())
        ActorMenu->OnInventoryAction(item, actionType);
}

#include "ui/UIGameTutorial.h"
// XXX nitrocaster: move to appropriate header
extern CUISequencer* g_tutorial;
extern CUISequencer* g_tutorial2;

bool CUIGameCustom::ShowActorMenu()
{
    if (ActorMenu->IsShown())
    {
        ActorMenu->HideDialog();
    }
    else
    {
        HidePdaMenu();
        auto actor = smart_cast<CInventoryOwner*>(Level().CurrentViewEntity());
        VERIFY(actor);
        ActorMenu->SetActor(actor);
        ActorMenu->SetMenuMode(mmInventory);
        ActorMenu->ShowDialog(true);
    }
    return true;
}

void CUIGameCustom::HideActorMenu()
{
    if (ActorMenu->IsShown())
        ActorMenu->HideDialog();
}

//Alundaio:
void CUIGameCustom::UpdateActorMenu()
{
    if (ActorMenu->IsShown())
    {
        ActorMenu->UpdateActor();
        ActorMenu->RefreshCurrentItemCell();
    }
}

CScriptGameObject* CUIGameCustom::CurrentItemAtCell()
{
    CUICellItem* itm = ActorMenu->CurrentItem();
    if (!itm->m_pData)
        return nullptr;

    PIItem IItm = static_cast<PIItem>(itm->m_pData);
    if (!IItm)
        return nullptr;

    CGameObject* GO = smart_cast<CGameObject*>(IItm);

    if (GO)
        return GO->lua_game_object();

    return nullptr;
}
//-Alundaio

void CUIGameCustom::HideMessagesWindow()
{
    if (m_pMessagesWnd->IsShown())
        m_pMessagesWnd->Show(false);
}

void CUIGameCustom::ShowMessagesWindow()
{
    if (!m_pMessagesWnd->IsShown())
        m_pMessagesWnd->Show(true);
}

bool CUIGameCustom::ShowPdaMenu()
{
    HideActorMenu();
    PdaMenu->ShowDialog(true);
    return true;
}

void CUIGameCustom::HidePdaMenu()
{
    if (PdaMenu->IsShown())
        PdaMenu->HideDialog();
}

void CUIGameCustom::SetClGame(game_cl_GameState* gameState) { gameState->SetGameUI(this); }
void CUIGameCustom::UnLoad()
{
    xr_delete(MsgConfig);
    xr_delete(ActorMenu);
    xr_delete(PdaMenu);
    xr_delete(Window);
    xr_delete(UIMainIngameWnd);
    xr_delete(m_pMessagesWnd);
}

void CUIGameCustom::Load()
{
    if (!g_pGameLevel)
        return;
    R_ASSERT(!MsgConfig);
    MsgConfig = new CUIXml();
    MsgConfig->Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "ui_custom_msgs.xml");
    R_ASSERT(!ActorMenu);
    ActorMenu = new CUIActorMenu();
    R_ASSERT(!PdaMenu);
    PdaMenu = new CUIPdaWnd();
    R_ASSERT(!Window);
    Window = new CUIWindow();
    R_ASSERT(!UIMainIngameWnd);
    UIMainIngameWnd = new CUIMainIngameWnd();
    UIMainIngameWnd->Init();
    R_ASSERT(!m_pMessagesWnd);
    m_pMessagesWnd = new CUIMessagesWindow();
    Init(0);
    Init(1);
    Init(2);
}

void CUIGameCustom::OnConnected()
{
    if (!g_pGameLevel)
        return;
    if (!UIMainIngameWnd)
        Load();
    UIMainIngameWnd->OnConnected();
}

void CUIGameCustom::CommonMessageOut(LPCSTR text) { m_pMessagesWnd->AddLogMessage(text); }
void CUIGameCustom::UpdatePda() { GetPdaMenu().UpdatePda(); }
void CUIGameCustom::update_fake_indicators(u8 type, float power)
{
    UIMainIngameWnd->get_hud_states()->FakeUpdateIndicatorType(type, power);
}

void CUIGameCustom::enable_fake_indicators(bool enable)
{
    UIMainIngameWnd->get_hud_states()->EnableFakeIndicators(enable);
}

StaticDrawableWrapper::StaticDrawableWrapper()
{
    m_static = nullptr;
    m_endTime = -1.0f;
}

void StaticDrawableWrapper::destroy() { delete_data(m_static); }
bool StaticDrawableWrapper::IsActual() const
{
    if (m_endTime < 0)
        return true;
    return Device.fTimeGlobal < m_endTime;
}

void StaticDrawableWrapper::SetText(const char* text)
{
    m_static->Show(text != nullptr);
    if (text)
    {
        m_static->TextItemControl()->SetTextST(text);
        m_static->ResetColorAnimation();
    }
}

void StaticDrawableWrapper::Draw()
{
    if (m_static->IsShown())
        m_static->Draw();
}

void StaticDrawableWrapper::Update()
{
    if (IsActual() && m_static->IsShown())
        m_static->Update();
}

CMapListHelper gMapListHelper;

void CMapListHelper::LoadMapInfo(const char* cfgName, const xr_string& levelName, const char* levelVer /*= "1.0"*/)
{
    CInifile levelCfg(cfgName);
    shared_str shLevelName = levelName.substr(0, levelName.find(_DELIMITER)).c_str();
    shared_str shLevelVer = levelVer;
    if (levelCfg.section_exist("map_usage"))
    {
        if (levelCfg.line_exist("map_usage", "ver") && !levelVer)
            shLevelVer = levelCfg.r_string("map_usage", "ver");
        for (CInifile::Item& kv : levelCfg.r_section("map_usage").Data)
        {
            const shared_str& gameType = kv.first;
            if (gameType == "ver")
                continue;
            SGameTypeMaps* suitableLevels = GetMapListInt(gameType);
            if (!suitableLevels)
            {
                Msg("--unknown game type-%s", gameType.c_str());
                m_storage.resize(m_storage.size() + 1);
                SGameTypeMaps& lastItem = m_storage.back();
                lastItem.m_game_type_name = gameType;
                lastItem.m_game_type_id = ParseStringToGameType(gameType.c_str());
                suitableLevels = &m_storage.back();
            }
            MPLevelDesc levelDesc;
            levelDesc.map_name = shLevelName;
            levelDesc.map_ver = shLevelVer;
            auto& levelNames = suitableLevels->m_map_names;
            if (std::find(levelNames.begin(), levelNames.end(), levelDesc) != levelNames.end())
            {
                Msg("! duplicate map found [%s] [%s]", shLevelName.c_str(), shLevelVer.c_str());
            }
            else
            {
#ifndef MASTER_GOLD
                Msg("added map [%s] [%s]", shLevelName.c_str(), shLevelVer.c_str());
#endif
                levelNames.push_back(levelDesc);
            }
        }
    }
}

void CMapListHelper::Load()
{
    string_path cfgFileName;
    FS.update_path(cfgFileName, "$game_config$", "mp" DELIMITER "map_list.ltx");
    CInifile maplistCfg(cfgFileName);
    // read weathers set
    CInifile::Sect weatherCfg = maplistCfg.r_section("weather");
    m_weathers.reserve(weatherCfg.Data.size());
    for (CInifile::Item& weatherDesc : weatherCfg.Data)
    {
        MPWeatherDesc gw;
        gw.Name = weatherDesc.first;
        gw.StartTime = weatherDesc.second;
        m_weathers.push_back(gw);
    }
    // scan for additional maps
    FS_FileSet levelCfgs;
    FS.file_list(levelCfgs, "$game_levels$", FS_ListFiles, "*level.ltx");
    for (const FS_File& cfg : levelCfgs)
    {
        FS.update_path(cfgFileName, "$game_levels$", cfg.name.c_str());
        LoadMapInfo(cfgFileName, cfg.name);
    }
    // scan all not loaded archieves
    LPCSTR tempRoot = "temporary_gamedata" DELIMITER;
    FS_Path* levelsPath = FS.get_path("$game_levels$");
    xr_string prevRoot = levelsPath->m_Root;
    levelsPath->_set_root(tempRoot);
    for (CLocatorAPI::archive& arch : FS.m_archives)
    {
        if (arch.hSrcFile)
            continue; // skip if loaded
        const char* levelName = arch.header->r_string("header", "level_name");
        const char* levelVersion = arch.header->r_string("header", "level_ver");
        FS.LoadArchive(arch, tempRoot);
        FS.update_path(cfgFileName, "$game_levels$", levelName);
        xr_strcat(cfgFileName, "" DELIMITER "level.ltx");
        LoadMapInfo(cfgFileName, levelName, levelVersion);
        FS.unload_archive(arch);
    }
    levelsPath->_set_root(prevRoot.c_str());
    // XXX nitrocaster: is that really fatal?
    R_ASSERT2(m_storage.size() > 0, "unable to fill map list");
    R_ASSERT2(m_weathers.size() > 0, "unable to fill weathers list");
}

const SGameTypeMaps& CMapListHelper::GetMapListFor(const shared_str& gameType)
{
    if (m_storage.size() == 0)
        Load();
    // XXX nitrocaster: always use enum for game type representation
    return *GetMapListInt(gameType);
}

SGameTypeMaps* CMapListHelper::GetMapListInt(const shared_str& gameType)
{
    for (SGameTypeMaps& maps : m_storage)
    {
        if (maps.m_game_type_name == gameType)
            return &maps;
    }
    return nullptr;
}

const SGameTypeMaps& CMapListHelper::GetMapListFor(const EGameIDs gameId)
{
    if (m_storage.size() == 0)
    {
        Load();
        // XXX nitrocaster: is that really fatal?
        R_ASSERT2(m_storage.size() > 0, "unable to fill map list");
    }
    for (SGameTypeMaps& maps : m_storage)
    {
        if (maps.m_game_type_id == gameId)
            return maps;
    }
    return m_storage[0];
}

const xr_vector<MPWeatherDesc>& CMapListHelper::GetGameWeathers()
{
    if (m_weathers.size() == 0)
        Load();
    return m_weathers;
}
