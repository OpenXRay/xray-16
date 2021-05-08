#include "StdAfx.h"
#include "ServerList.h"
#include "UIXmlInit.h"
#include "string_table.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrUICore/EditBox/UIEditBox.h"
#include "UIMessageBoxEx.h"
#include "xrUICore/MessageBox/UIMessageBox.h"
#include "TeamInfo.h"
#include "MainMenu.h"
#include "login_manager.h"
#include "xrGameSpy/GameSpy_Keys.h"
#include "xrGameSpy/GameSpy_Full.h"
#include "Spectator.h"
#include "VersionSwitcher.h"

LPCSTR GameTypeToStringEx(u32 gt, bool bShort);

CServerList::CServerList()
{
#ifdef XR_PLATFORM_WINDOWS
    CGameSpy_BrowsersWrapper::UpdateCallback updateCb;
    updateCb.bind(this, &CServerList::OnUpdate);
    m_subscriber_id = browser().SubscribeUpdates(updateCb);

    for (int i = 0; i < LST_COLUMN_COUNT; i++)
        AttachChild(&m_header_frames[i]);

    for (int i = 0; i < LST_COLUMN_COUNT; i++)
        AttachChild(&m_header[i]);

    for (int i = 0; i < 4; i++)
        AttachChild(&m_header2[i]);

    AttachChild(&m_edit_gs_filter);

    for (int i = 0; i < 3; i++)
    {
        m_list[i].Show(true);
        AttachChild(&m_frame[i]);
        AttachChild(&m_list[i]);
    }

    m_bShowServerInfo = false;
    m_bAnimation = false;

    m_sort_mode = SORT_PING;
    m_message_box = xr_new<CUIMessageBoxEx>();
    m_message_box->InitMessageBox("message_box_password");
    m_message_box->SetMessageTarget(this);

    if (CVersionSwitcher::GetVerCount() > 0)
    {
        m_version_switch_msgbox = xr_new<CUIMessageBoxEx>();
        m_version_switch_msgbox->SetMessageTarget(this);
        if (!m_version_switch_msgbox->InitMessageBox("message_box_version_switch"))
            xr_delete(m_version_switch_msgbox);
    }
    else
    {
        m_version_switch_msgbox = nullptr;
    }

    m_b_local = false;

    m_last_retreived_index = u32(-1);
    m_need_refresh_fr = u32(-1);
#endif
}

CServerList::~CServerList()
{
    xr_delete(m_version_switch_msgbox);
    xr_delete(m_message_box);

    auto bro = browser_LL();
    if (bro)
        bro->UnsubscribeUpdates(m_subscriber_id);

    DestroySrvItems();
};

CGameSpy_BrowsersWrapper* CServerList::browser_LL()
{
#ifdef XR_PLATFORM_WINDOWS
    auto mm = MainMenu();
    if (mm)
    {
        auto gs = mm->GetGS();
        if (gs)
            return gs->GetGameSpyBrowser();
    }
#endif
    return nullptr;
}

CGameSpy_BrowsersWrapper& CServerList::browser()
{
    // browser_LL must return not-null almost always; null is possible when the game exits (and main menu is already destroyed)
    auto bro = browser_LL();
    R_ASSERT(bro);
    return *bro;
}

void CServerList::Update()
{
    if (m_need_refresh_fr < Device.dwFrame + 10)
        RefreshList_internal();

    if (m_bAnimation)
    {
        //		m_pAnimation->Update();
        //		m_frame[LST_SRV_PROP].SetColor(subst_alpha(0xffffffff, color_get_A(m_pAnimation->GetColor())));
        //		m_frame[LST_PLAYERS].SetColor(subst_alpha(0xffffffff, color_get_A(m_pAnimation->GetColor())));

        if (true /*m_pAnimation->Done()*/)
        {
            m_bAnimation = false;
            if (m_bShowServerInfo)
                AfterAppear();
            else
                AfterDisappear();
        }
    }
    CUIWindow::Update();
}

bool CServerList::NeedToRefreshCurServer()
{
    CUIListItemServer* pItem = (CUIListItemServer*)m_list[LST_SERVER].GetSelectedItem();
    if (!pItem)
        return false;
    return browser().HasAllKeys(pItem->GetInfo()->info.Index) == false;
};

void CServerList::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (m_bShowServerInfo && LIST_ITEM_CLICKED == msg && &m_list[LST_SERVER] == pWnd)
    {
        if (NeedToRefreshCurServer())
        {
            RefreshQuick();
        }
        else
        {
            ClearDetailedServerInfo();
            FillUpDetailedServerInfo();
        }
    }
    else if (BUTTON_CLICKED == msg)
    {
        if (pWnd == &m_header[1])
        {
            SetSortFunc_internal(SORT_SERVERNAME, SORT_TYPE_AUTO, true);
        }
        else if (pWnd == &m_header[2])
        {
            SetSortFunc_internal(SORT_MAP, SORT_TYPE_AUTO, true);
        }
        else if (pWnd == &m_header[3])
        {
            SetSortFunc_internal(SORT_GAMETYPE, SORT_TYPE_AUTO, true);
        }
        else if (pWnd == &m_header[4])
        {
            SetSortFunc_internal(SORT_PLAYERSCOUNT, SORT_TYPE_AUTO, true);
        }
        else if (pWnd == &m_header[5])
        {
            SetSortFunc_internal(SORT_PING, SORT_TYPE_AUTO, true);
        }
        else if (pWnd == &m_header[6])
        {
            SetSortFunc_internal(SORT_GAMEVERSION, SORT_TYPE_AUTO, true);
        }
    }
    else if (EDIT_TEXT_COMMIT == msg && pWnd == &m_edit_gs_filter)
    {
        RefreshGameSpyList(m_b_local);
    }
    else if (MESSAGE_BOX_YES_CLICKED == msg)
    {
        CUIListItemServer* item = smart_cast<CUIListItemServer*>(m_list[LST_SERVER].GetSelectedItem());
        if (!item)
            return;

        if (m_version_switch_msgbox != nullptr && pWnd == m_version_switch_msgbox)
        {
            pcstr srvpsw = (item->GetInfo()->info.icons.pass) ? m_message_box->GetPassword() : "";
            pcstr upsw = (item->GetInfo()->info.icons.user_pass) ? m_message_box->m_pMessageBox->GetUserPassword() : "";
            CVersionSwitcher::SetupMPParams(m_playerName.c_str(), srvpsw, upsw, m_itemInfo.info.address.c_str());
            CVersionSwitcher::SwitchToGameVer(m_itemInfo.info.version.c_str(), CVersionSwitcher::SWITCH_TO_SERVER);
        }
        else if (pWnd == m_message_box)
        {
            if (m_version_switch_msgbox != nullptr &&
                xr_strcmp(item->GetInfo()->info.version, MainMenu()->GetGSVer()) != 0)
            {
                m_version_switch_msgbox->ShowDialog(true);
            }
            else
            {
                xr_string command;
                item->CreateConsoleCommand(command, m_playerName.c_str(),
                    m_message_box->m_pMessageBox->GetUserPassword(), m_message_box->GetPassword());
                Console->Execute(command.c_str());
            }
        }
    }
    else if (WINDOW_LBUTTON_DB_CLICK == msg && &m_list[LST_SERVER] == pWnd)
    {
        ConnectToSelected();
    }
}

void CServerList::BeforeAppear()
{
    UpdateSizes();
    UpdateVisibility();
}

void CServerList::AfterAppear()
{
    FillUpDetailedServerInfo();
    UpdateVisibility();
}

void CServerList::BeforeDisapear()
{
    ClearDetailedServerInfo();
    UpdateVisibility();
}

void CServerList::AfterDisappear()
{
    UpdateSizes();
    UpdateVisibility();
}

void CServerList::AddServerDetail(const GameInfo& info)
{
    CUIListBox& listBox = m_list[LST_SRV_PROP];
    float halfWidth = listBox.GetWidth() / 2;
    CUIListBoxItem* item = listBox.AddItem();
    item->SetText(info.InfoName.c_str());
    item->GetTextItem()->SetWidth(halfWidth);
    item->AddTextField(info.InfoData.c_str(), halfWidth);
}

void CServerList::AddBoolED(const char* keyName, bool value)
{
    AddServerDetail(GameInfo(*StringTable().translate(keyName),
        value ? *StringTable().translate("mp_si_enabled") : *StringTable().translate("mp_si_disabled")));
}

void CServerList::AddBoolYN(const char* keyName, bool value)
{
    AddServerDetail(GameInfo(*StringTable().translate(keyName),
        value ? *StringTable().translate("mp_si_yes") : *StringTable().translate("mp_si_no")));
}

void CServerList::AddBoolKeyED(void* s, const char* keyName, int k) { AddBoolED(keyName, browser().GetBool(s, k)); }
void CServerList::AddBoolKeyYN(void* s, const char* keyName, int k) { AddBoolYN(keyName, browser().GetBool(s, k)); }
void CServerList::AddIntKey(void* s, const char* keyName, int k)
{
    string256 tmp;
    xr_sprintf(tmp, "%d", browser().GetInt(s, k));
    AddServerDetail(GameInfo(*StringTable().translate(keyName), tmp));
}

void CServerList::AddIntKeyN(void* s, float m, const char* keyName, const char* suffix, int k)
{
    if (browser().GetInt(s, k))
    {
        string256 tmp;
        xr_sprintf(tmp, "%d%s", int(browser().GetInt(s, k) * m), suffix);
        AddServerDetail(GameInfo(*StringTable().translate(keyName), tmp));
    }
    else
        AddServerDetail(GameInfo(*StringTable().translate(keyName), *StringTable().translate("mp_si_no")));
}

void CServerList::AddTimeKey(void* s, const char* keyName, const char* format, const char* suffix, int k)
{
    if (browser().GetInt(s, k))
    {
        string256 tmp;
        xr_sprintf(tmp, format, browser().GetFloat(s, k), *StringTable().translate(suffix));
        AddServerDetail(GameInfo(*StringTable().translate(keyName), tmp));
    }
    else
        AddServerDetail(GameInfo(*StringTable().translate(keyName), *StringTable().translate("mp_si_no")));
}

void CServerList::AddString(const char* key, const char* value)
{
    AddServerDetail(GameInfo(*StringTable().translate(key), value));
}

void CServerList::AddStringSt(const char* key, const char* value)
{
    AddServerDetail(GameInfo(*StringTable().translate(key), *StringTable().translate(value)));
}

void CServerList::FillUpDetailedServerInfo()
{
    bool t1 = false;
    bool t2 = false;
    bool spect = false;

    CUIListItemServer* pItem = (CUIListItemServer*)m_list[LST_SERVER].GetSelectedItem();
    if (!pItem)
    {
        ClearDetailedServerInfo();
        return;
    }
    ServerInfo srvInfo;
    int serverIndex = pItem->GetInfo()->info.Index;
    browser().GetServerInfoByIndex(&srvInfo, serverIndex);
    u32 teams = srvInfo.m_aTeams.size();

    if (2 == teams)
    {
        pstr _buff = NULL;
        CUIListBoxItem* pItemAdv;
        // TEAM 1
        xr_vector<PlayerInfo>::iterator it;
        for (it = srvInfo.m_aPlayers.begin(); it != srvInfo.m_aPlayers.end(); ++it)
        {
            PlayerInfo pf = *it;
            if (1 != pf.Team)
                continue;
            if (pf.Spectator)
                continue;
            if (!t1) // add header
            {
                STRCONCAT(_buff, StringTable().translate("ui_st_team").c_str(), "\"",
                    CTeamInfo::GetTeam1_name().c_str(), "\"");
                pItemAdv = m_list[LST_PLAYERS].AddItem();
                pItemAdv->SetTextColor(m_list[LST_PLAYERS].GetTextColor());
                pItemAdv->SetFont(m_list[LST_PLAYERS].GetFont());
                pItemAdv->SetText(_buff);
                pItemAdv->GetTextItem()->SetWidth(m_list[LST_PLAYERS].GetDesiredChildWidth());
                t1 = true;
            }
            pItemAdv = m_list[LST_PLAYERS].AddItem();
            char buf[16];
            pItemAdv->SetTextColor(m_list[LST_PLAYERS].GetTextColor());
            pItemAdv->SetFont(m_list[LST_PLAYERS].GetFont());
            pItemAdv->SetText(pf.Name);
            pItemAdv->GetTextItem()->SetWidth(m_header2[1].GetWidth());
            xr_sprintf(buf, sizeof(buf), "%d", pf.Frags);
            pItemAdv->AddTextField(buf, m_header2[2].GetWidth());
            xr_sprintf(buf, sizeof(buf), "%d", pf.Deaths);
            pItemAdv->AddTextField(buf, m_header2[3].GetWidth());
        }

        // TEAM 2
        for (it = srvInfo.m_aPlayers.begin(); it != srvInfo.m_aPlayers.end(); ++it)
        {
            PlayerInfo pf = *it;
            if (2 != pf.Team)
                continue;
            if (pf.Spectator)
                continue;
            if (!t2)
            {
                STRCONCAT(_buff, StringTable().translate("ui_st_team").c_str(), "\"",
                    CTeamInfo::GetTeam2_name().c_str(), "\"");
                m_list[LST_PLAYERS].AddTextItem(_buff);
                t2 = true;
            }
            pItemAdv = m_list[LST_PLAYERS].AddItem();
            char buf[16];
            pItemAdv->SetTextColor(m_list[LST_PLAYERS].GetTextColor());
            pItemAdv->SetFont(m_list[LST_PLAYERS].GetFont());
            pItemAdv->SetText(pf.Name);
            pItemAdv->GetTextItem()->SetWidth(m_header2[1].GetWidth());
            xr_sprintf(buf, sizeof(buf), "%d", pf.Frags);
            pItemAdv->AddTextField(buf, m_header2[2].GetWidth());
            xr_sprintf(buf, sizeof(buf), "%d", pf.Deaths);
            pItemAdv->AddTextField(buf, m_header2[3].GetWidth());
        }
        // SPECTATORS
        for (it = srvInfo.m_aPlayers.begin(); it != srvInfo.m_aPlayers.end(); ++it)
        {
            PlayerInfo pf = *it;
            if (!pf.Spectator)
                continue;
            if (!spect)
            {
                pItemAdv = m_list[LST_PLAYERS].AddTextItem(StringTable().translate("mp_spectator").c_str());
                spect = true;
            }
            pItemAdv = m_list[LST_PLAYERS].AddItem();
            char buf[16];
            pItemAdv->SetFont(m_list[LST_PLAYERS].GetFont());
            pItemAdv->SetTextColor(m_list[LST_PLAYERS].GetTextColor());
            pItemAdv->SetText(pf.Name);
            pItemAdv->GetTextItem()->SetWidth(m_header2[1].GetWidth());
            xr_sprintf(buf, sizeof(buf), "%d", pf.Frags);
            pItemAdv->AddTextField(buf, m_header2[2].GetWidth());
            xr_sprintf(buf, sizeof(buf), "%d", pf.Deaths);
            pItemAdv->AddTextField(buf, m_header2[3].GetWidth());
        }
    }
    else
    {
        xr_vector<PlayerInfo>::iterator it;
        for (it = srvInfo.m_aPlayers.begin(); it != srvInfo.m_aPlayers.end(); ++it)
        {
            PlayerInfo pf = *it;
            CUIListBoxItem* pItemAdv = m_list[LST_PLAYERS].AddItem();
            char buf[16];
            pItemAdv->SetTextColor(m_list[LST_PLAYERS].GetTextColor());
            pItemAdv->SetFont(m_list[LST_PLAYERS].GetFont());
            pItemAdv->SetText(pf.Name);
            pItemAdv->GetTextItem()->SetWidth(m_header2[1].GetWidth());
            xr_sprintf(buf, sizeof(buf), "%d", pf.Frags);
            pItemAdv->AddTextField(buf, m_header2[2].GetWidth());
            xr_sprintf(buf, sizeof(buf), "%d", pf.Deaths);
            pItemAdv->AddTextField(buf, m_header2[3].GetWidth());
        }
    }

    void* sv = browser().GetServerByIndex(serverIndex);
    AddString("mp_si_servername", srvInfo.m_ServerName);
    AddString("mp_si_version", srvInfo.m_ServerVersion);
    AddIntKey(sv, "mp_si_max_ping", G_MAX_PING_KEY);
    AddBoolKeyYN(sv, "mp_si_maprotation", G_MAP_ROTATION_KEY);
    AddBoolKeyED(sv, "mp_si_voting", G_VOTING_ENABLED_KEY);
    AddBoolKeyYN(sv, "mp_si_voting", G_VOTING_ENABLED_KEY);
    AddString("mp_si_spectatormodes", "");
    AddBoolYN("mp_si_free_fly", srvInfo.SpectratorModes & (1 << CSpectator::eacFreeFly));
    AddBoolYN("mp_si_first_eye", srvInfo.SpectratorModes & (1 << CSpectator::eacFirstEye));
    AddBoolYN("mp_si_look_at", srvInfo.SpectratorModes & (1 << CSpectator::eacLookAt));
    AddBoolYN("mp_si_free_look", srvInfo.SpectratorModes & (1 << CSpectator::eacFreeLook));
    if (srvInfo.m_GameType != eGameIDDeathmatch)
        AddBoolYN("mp_si_team_only", srvInfo.SpectratorModes & (1 << CSpectator::eacMaxCam));
    AddIntKey(sv, "mp_si_fraglimit", G_FRAG_LIMIT_KEY);
    AddTimeKey(sv, "mp_si_time_limit", "%.0f %s", "mp_si_min", G_TIME_LIMIT_KEY);
    AddString("mp_si_invinsibility", "");
    AddBoolKeyYN(sv, "mp_si_invinsibility_indicators", G_DAMAGE_BLOCK_INDICATOR_KEY);
    AddTimeKey(sv, "mp_si_invinsibility_time", "%.f %s", "mp_si_sec", G_DAMAGE_BLOCK_TIME_KEY);
    AddBoolKeyYN(sv, "mp_si_anomalies", G_ANOMALIES_ENABLED_KEY);
    if (browser().GetInt(sv, G_ANOMALIES_TIME_KEY))
    {
        AddTimeKey(sv, "mp_si_anomalies_period", "%.1f %s", "mp_si_min", G_ANOMALIES_TIME_KEY);
    }
    else
        AddStringSt("mp_si_anomalies_period", "mp_si_infinite");
    AddTimeKey(sv, "mp_si_forcerespawn", "%.f %s", "mp_si_sec", G_FORCE_RESPAWN_KEY);
    AddTimeKey(sv, "mp_si_warmuptime", "%.0f %s", "mp_si_sec", G_WARM_UP_TIME_KEY);
    if (srvInfo.m_GameType == eGameIDTeamDeathmatch || srvInfo.m_GameType == eGameIDArtefactHunt ||
        srvInfo.m_GameType == eGameIDCaptureTheArtefact)
    {
        AddBoolKeyYN(sv, "mp_si_autoteam_balance", G_AUTO_TEAM_BALANCE_KEY);
        AddBoolKeyYN(sv, "mp_si_autoteam_swap", G_AUTO_TEAM_SWAP_KEY);
        AddBoolKeyYN(sv, "mp_si_friendly_indicators", G_FRIENDLY_INDICATORS_KEY);
        AddBoolKeyYN(sv, "mp_si_friendly_names", G_FRIENDLY_NAMES_KEY);
        AddIntKeyN(sv, 1 / 100.0f, "mp_si_friendly_fire", " %", G_FRIENDLY_FIRE_KEY);
    }
    if (srvInfo.m_GameType == eGameIDArtefactHunt || srvInfo.m_GameType == eGameIDCaptureTheArtefact)
    {
        AddString("mp_si_artefacts", "");
        AddIntKey(sv, "mp_si_afcount", G_ARTEFACTS_COUNT_KEY);
        AddTimeKey(sv, "mp_si_afstaytime", "%.2f %s", "mp_si_min", G_ARTEFACT_STAY_TIME_KEY);
        AddTimeKey(sv, "mp_si_afrespawntime", "%.0f %s", "mp_si_sec", G_ARTEFACT_RESPAWN_TIME_KEY);
        if (srvInfo.Reinforcement == -1)
            AddStringSt("mp_si_players_respawn", "mp_si_artefact_captured");
        else if (srvInfo.Reinforcement == 0)
            AddString("mp_si_players_respawn", "mp_si_any_time");
        else
            AddTimeKey(sv, "mp_si_players_respawn", "%.0f %s", "mp_si_sec", G_REINFORCEMENT_KEY);
        AddBoolKeyYN(sv, "mp_si_shielded_bases", G_SHIELDED_BASES_KEY);
        AddBoolKeyYN(sv, "mp_si_return_players", G_RETURN_PLAYERS_KEY);
        AddBoolKeyYN(sv, "mp_si_afbearer_cant_sprint", G_BEARER_CANT_SPRINT_KEY);
    }
    AddString("Uptime", srvInfo.m_ServerUpTime);
}

void CServerList::ClearDetailedServerInfo()
{
    m_list[LST_SRV_PROP].Clear();
    m_list[LST_PLAYERS].Clear();
}

void CServerList::ShowServerInfo()
{
    if (!m_bShowServerInfo && NeedToRefreshCurServer())
    {
        RefreshQuick();
    }
    m_bShowServerInfo = !m_bShowServerInfo;
    m_bAnimation = true;
    if (m_bShowServerInfo)
        BeforeAppear();
    else
        BeforeDisapear();
}

void CServerList::UpdateSizes()
{
    float height = m_bShowServerInfo ? m_fListH[1] : m_fListH[0];
    m_list[LST_SERVER].SetHeight(height);
    int page_size = (m_list[LST_SERVER].GetSize() * m_list[LST_SERVER].GetItemHeight() < height) ? 0 : int(height);
    m_list[LST_SERVER].ScrollBar()->SetPageSize(page_size);
    m_list[LST_SERVER].ForceUpdate();

    m_frame[LST_SERVER].SetHeight(height + 2.0f);
    Fvector2 pos = m_edit_gs_filter.GetWndPos();
    pos.y = m_bShowServerInfo ? m_fEditPos[1] : m_fEditPos[0];
    m_edit_gs_filter.SetWndPos(pos);
}

void CServerList::UpdateVisibility()
{
    m_list[LST_SRV_PROP].Show(m_bShowServerInfo ? !m_bAnimation : false);
    m_list[LST_PLAYERS].Show(m_bShowServerInfo ? !m_bAnimation : false);
    m_frame[LST_SRV_PROP].Show(m_bShowServerInfo ? true : m_bAnimation);
    m_frame[LST_PLAYERS].Show(m_bShowServerInfo ? true : m_bAnimation);

    for (int i = 0; i < 4; i++)
        m_header2[i].Show(m_bShowServerInfo ? true : m_bAnimation);
}

void CServerList::SetFilters(SServerFilters& sf)
{
    m_sf = sf;
    RefreshList();
}

void CServerList::SetPlayerName(LPCSTR name) { m_playerName = name; }
bool CServerList::IsValidItem(ServerInfo& item)
{
    bool result = true;

    // Filter 'junk' servers (which are not replying for queries)
    result &= (item.m_Port > 0);

    result &= !m_sf.empty ? (m_sf.empty == (item.m_ServerNumPlayers == 0)) : true;
    result &= !m_sf.full ? (m_sf.full == (item.m_ServerNumPlayers == item.m_ServerMaxPlayers)) : true;
    result &= !m_sf.with_pass ? (m_sf.with_pass == item.m_bPassword) : true;
    result &= !m_sf.without_pass ? (m_sf.without_pass != item.m_bPassword) : true;
    result &= !m_sf.without_ff ? (m_sf.without_ff != item.m_bFFire) : true;
    result &= !m_sf.listen_servers ? (m_sf.listen_servers != item.m_bDedicated) : true;

    return result;
}

void CServerList::InitFromXml(CUIXml& xml_doc, LPCSTR path)
{
    CUIXmlInit::InitWindow(xml_doc, path, 0, this);
    string256 buf;
    CUIXmlInit::InitListBox(xml_doc, strconcat(sizeof(buf), buf, path, ":list"), 0, &m_list[LST_SERVER]);
    m_fListH[0] = m_list[LST_SERVER].GetHeight();
    m_fListH[1] = xml_doc.ReadAttribFlt(buf, 0, "height2");
    CUIXmlInit::InitListBox(
        xml_doc, strconcat(sizeof(buf), buf, path, ":list_server_properties"), 0, &m_list[LST_SRV_PROP]);
    CUIXmlInit::InitListBox(xml_doc, strconcat(sizeof(buf), buf, path, ":list_players_list"), 0, &m_list[LST_PLAYERS]);
    CUIXmlInit::InitFrameWindow(xml_doc, strconcat(sizeof(buf), buf, path, ":frame"), 0, &m_frame[LST_SERVER]);
    CUIXmlInit::InitFrameWindow(
        xml_doc, strconcat(sizeof(buf), buf, path, ":list_server_properties:frame"), 0, &m_frame[LST_SRV_PROP]);
    CUIXmlInit::InitFrameWindow(
        xml_doc, strconcat(sizeof(buf), buf, path, ":list_players_list:frame"), 0, &m_frame[LST_PLAYERS]);
    CUIXmlInit::InitFont(
        xml_doc, strconcat(sizeof(buf), buf, path, ":list_item:text"), 0, m_itemInfo.text_color, m_itemInfo.text_font);
    CUIXmlInit::InitEditBox(xml_doc, strconcat(sizeof(buf), buf, path, ":edit_gs_filter"), 0, &m_edit_gs_filter);
    m_fEditPos[0] = m_edit_gs_filter.GetWndPos().y;
    m_fEditPos[1] = xml_doc.ReadAttribFlt(buf, 0, "y2");
    CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":cap_server_properties"), 0, &m_header2[0]);
    CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":cap_players_list"), 0, &m_header2[1]);
    CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":cap_frags"), 0, &m_header2[2]);
    CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":cap_death"), 0, &m_header2[3]);

    m_itemInfo.size.icon = xml_doc.ReadAttribFlt(strconcat(sizeof(buf), buf, path, ":sizes"), 0, "icon");
    m_itemInfo.size.server = xml_doc.ReadAttribFlt(buf, 0, "server");
    m_itemInfo.size.map = xml_doc.ReadAttribFlt(buf, 0, "map");
    m_itemInfo.size.game = xml_doc.ReadAttribFlt(buf, 0, "game");
    m_itemInfo.size.players = xml_doc.ReadAttribFlt(buf, 0, "players");
    m_itemInfo.size.ping = xml_doc.ReadAttribFlt(buf, 0, "ping");
    m_itemInfo.size.version = xml_doc.ReadAttribFlt(buf, 0, "version");

    // init header elements
    for (int i = 0; i < LST_COLUMN_COUNT; i++)
    {
        CUIXmlInit::Init3tButton(xml_doc, strconcat(sizeof(buf), buf, path, ":header"), 0, &m_header[i]);
        CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":header_frames"), 0, &m_header_frames[i]);
    }
    m_header[0].Enable(false);
    InitHeader();
    UpdateSizes();
    UpdateVisibility();
}

void CServerList::ConnectToSelected()
{
#ifdef XR_PLATFORM_WINDOWS
    gamespy_gp::login_manager const* lmngr = MainMenu()->GetLoginMngr();
    R_ASSERT(lmngr);
    gamespy_gp::profile const* tmp_profile = lmngr->get_current_profile();
    R_ASSERT2(tmp_profile, "need first to log in");
    if (tmp_profile->online())
    {
        if (!MainMenu()->ValidateCDKey())
            return;

        if (!xr_strcmp(tmp_profile->unique_nick(), "@unregistered"))
        {
            if (m_connect_cb)
                m_connect_cb(ece_unique_nick_not_registred, "mp_gp_unique_nick_not_registred");
            return;
        }
        if (!xr_strcmp(tmp_profile->unique_nick(), "@expired"))
        {
            if (m_connect_cb)
                m_connect_cb(ece_unique_nick_expired, "mp_gp_unique_nick_has_expired");
            return;
        }
    }

    CUIListItemServer* item = smart_cast<CUIListItemServer*>(m_list[LST_SERVER].GetSelectedItem());
    if (!item)
        return;
    if (!browser().CheckDirectConnection(item->GetInfo()->info.Index))
    {
        Msg("! Direct connection to this server is not available -> its behind firewall");
        return;
    }

    pcstr ver = item->GetInfo()->info.version.c_str();
    bool dif_ver = xr_strcmp(ver, MainMenu()->GetGSVer()) != 0;

    if (dif_ver &&
        (m_version_switch_msgbox == nullptr ||
            CVersionSwitcher::FindVersionIdByName(ver) == CVersionSwitcher::VERSION_NOT_FOUND))
    {
        MainMenu()->SetErrorDialog(CMainMenu::ErrDifferentVersion);
        return;
    }

    if (item->GetInfo()->info.icons.pass || item->GetInfo()->info.icons.user_pass)
    {
        m_message_box->m_pMessageBox->SetUserPasswordMode(item->GetInfo()->info.icons.user_pass);
        m_message_box->m_pMessageBox->SetPasswordMode(item->GetInfo()->info.icons.pass);
        m_message_box->ShowDialog(true);
    }
    else
    {
        if (dif_ver)
        {
            m_version_switch_msgbox->ShowDialog(true);
        }
        else
        {
            xr_string command;
            item->CreateConsoleCommand(command, m_playerName.c_str(), "", "");
            Console->Execute(command.c_str());
        }
    }
#endif
}

void CServerList::InitHeader()
{
    Fvector2 pos;
    pos.set(0, 0);

    m_header[0].SetWidth(m_itemInfo.size.icon);
    pos.x += m_itemInfo.size.icon;
    m_header[1].SetWidth(m_itemInfo.size.server);
    m_header[1].SetWndPos(pos);
    m_header[1].TextItemControl()->SetTextST("server name");
    pos.x += m_itemInfo.size.server;
    m_header[2].SetWidth(m_itemInfo.size.map);
    m_header[2].SetWndPos(pos);
    m_header[2].TextItemControl()->SetTextST("map");
    pos.x += m_itemInfo.size.map;
    m_header[3].SetWidth(m_itemInfo.size.game);
    m_header[3].SetWndPos(pos);
    m_header[3].TextItemControl()->SetTextST("game type");
    pos.x += m_itemInfo.size.game;
    m_header[4].SetWidth(m_itemInfo.size.players);
    m_header[4].SetWndPos(pos);
    m_header[4].TextItemControl()->SetTextST("players");
    pos.x += m_itemInfo.size.players;
    m_header[5].SetWidth(m_itemInfo.size.ping);
    m_header[5].SetWndPos(pos);
    m_header[5].TextItemControl()->SetTextST("ping");
    pos.x += m_itemInfo.size.ping;
    m_header[6].SetWidth(m_itemInfo.size.version);
    m_header[6].SetWndPos(pos);
    m_header[6].TextItemControl()->SetTextST("version");

    for (int i = 0; i < LST_COLUMN_COUNT; ++i)
    {
        m_header_frames[i].SetWndPos(m_header[i].GetWndPos());
        m_header_frames[i].SetWidth(m_header[i].GetWndSize().x);
        m_header_frames[i].Enable(true);
        m_header_frames[i].Show(true);
        m_header_frames[i].SetVisible(true);
    }
}

void CServerList::NetRadioChanged(bool Local)
{
    m_edit_gs_filter.Enable(!Local);
    m_b_local = Local;
}

void CServerList::RefreshGameSpyList(bool Local)
{
    SetSortFunc_internal(SORT_PING, SORT_TYPE_ASCENDING, false);
    auto result = browser().RefreshList_Full(Local, m_edit_gs_filter.GetText());
    switch (result)
    {
    case GSUpdateStatus::ConnectingToMaster:
        if (MainMenu())
            MainMenu()->Show_CTMS_Dialog();
        break;
    case GSUpdateStatus::MasterUnreachable: MainMenu()->SetErrorDialog(CMainMenu::ErrMasterServerConnectFailed); break;
    }
    ResetCurItem();
    RefreshList();
}

void CServerList::AddServerToList(ServerInfo* pServerInfo)
{
    if (!IsValidItem(*pServerInfo))
        return;

    CUIListItemServer* item = GetFreeItem();

    SrvInfo2LstSrvInfo(pServerInfo);

    m_itemInfo.size.height = m_list[LST_SERVER].GetItemHeight();
    item->InitItemServer(m_itemInfo);

    m_list[LST_SERVER].AddExistingItem(item);
    item->SetAutoDelete(false);
};

void CServerList::UpdateServerInList(ServerInfo* pServerInfo, int index)
{
    int sz = m_list[LST_SERVER].GetSize();

    for (int i = 0; i < sz; i++)
    {
        CUIListItemServer* pItem = static_cast<CUIListItemServer*>(m_list[LST_SERVER].GetItemByIDX(i));
        if (pItem->Get_gs_index() == index)
        {
            UpdateServerInList(pServerInfo, pItem);
            return;
        }
    }

    R_ASSERT2(false, "CServerList::UpdateServerInList - invalid index");
};

void CServerList::UpdateServerInList(ServerInfo* pServerInfo, CUIListItemServer* pItem)
{
    SrvInfo2LstSrvInfo(pServerInfo);
    pItem->SetParams(m_itemInfo);
};

void CServerList::RefreshList() { m_need_refresh_fr = Device.dwFrame; }

class CServerComparer
{
private:
    CGameSpy_BrowsersWrapper& browser;
    CServerList::ESortingMode mode;
    bool ascending;

public:
    CServerComparer(CGameSpy_BrowsersWrapper& bro, CServerList::ESortingMode sortmode, bool asc)
        : browser(bro), mode(sortmode), ascending(asc)
    {
    }

    bool compare_ints(int val1, int val2) { return (ascending) ? (val1 < val2) : (val1 > val2); }

    bool compare_strs(const char* val1, const char* val2)
    {
        int res = xr_strcmp(val1, val2);
        return (ascending) ? (-1 == res) : (1 == res);
    }

    bool operator()(int p1, int p2)
    {
        ServerInfo info1, info2;

        browser.GetServerInfoByIndex(&info1, p1);
        browser.GetServerInfoByIndex(&info2, p2);

        switch (mode)
        {
        case CServerList::SORT_SERVERNAME: return compare_strs(info1.m_ServerName, info2.m_ServerName);
        case CServerList::SORT_MAP: return compare_strs(info1.m_SessionName, info2.m_SessionName);
        case CServerList::SORT_GAMETYPE: return compare_strs(info1.m_ServerGameType, info2.m_ServerGameType);
        case CServerList::SORT_PLAYERSCOUNT: return compare_ints(info1.m_ServerNumPlayers, info2.m_ServerNumPlayers);
        case CServerList::SORT_PING: return compare_ints(info1.m_Ping, info2.m_Ping);
        case CServerList::SORT_GAMEVERSION: return compare_strs(info1.m_ServerVersion, info2.m_ServerVersion);
        }

        return false;
    }
};


void CServerList::RefreshList_internal()
{
    m_need_refresh_fr = u32(-1);
    SaveCurItem();
    m_list[LST_SERVER].Clear();
    ClearSrvItems();

    auto& bro = browser();
    u32 NumServersFound = bro.GetServersCount();
    m_tmp_srv_lst.resize(NumServersFound);

    for (u32 i = 0; i < NumServersFound; i++)
        m_tmp_srv_lst[i] = i;

    CServerComparer cmpr(bro, m_sort_mode, m_sort_ascending);
    std::sort(m_tmp_srv_lst.begin(), m_tmp_srv_lst.end(), cmpr);

    for (u32 i = 0; i < NumServersFound; i++)
    {
        ServerInfo NewServerInfo;
        bro.GetServerInfoByIndex(&NewServerInfo, m_tmp_srv_lst[i]);

        AddServerToList(&NewServerInfo);
    }
    UpdateSizes();
    RestoreCurItem();
};

void CServerList::RefreshQuick()
{
    CUIListItemServer* pItem = (CUIListItemServer*)m_list[LST_SERVER].GetSelectedItem();
    if (!pItem)
        return;
    browser().RefreshQuick(pItem->GetInfo()->info.Index);

    RefreshList();

    if (m_bShowServerInfo)
    {
        ClearDetailedServerInfo();
        FillUpDetailedServerInfo();
    }
}

void CServerList::SetSortFunc(const char* func_name, bool make_sort)
{
    ESortingMode mode;

    if (!xr_strcmp(func_name, "server_name"))
        mode = SORT_SERVERNAME;
    else if (!xr_strcmp(func_name, "map"))
        mode = SORT_MAP;
    else if (!xr_strcmp(func_name, "game_type"))
        mode = SORT_GAMETYPE;
    else if (!xr_strcmp(func_name, "player"))
        mode = SORT_PLAYERSCOUNT;
    else if (!xr_strcmp(func_name, "ping"))
        mode = SORT_PING;
    else if (!xr_strcmp(func_name, "version"))
        mode = SORT_GAMEVERSION;
    else
        R_ASSERT2(false, "Unsupported sorting function name");

    SetSortFunc_internal(mode, SORT_TYPE_AUTO, make_sort);
}

void CServerList::SetSortFunc_internal(ESortingMode sort_mode, ESortingType sorting_type, bool make_sort)
{
    switch (sorting_type)
    {
    case SORT_TYPE_ASCENDING:
    {
        m_sort_ascending = true;
        break;
    }
    case SORT_TYPE_DESCENDING:
    {
        m_sort_ascending = false;
        break;
    }
    default:
    {
        if (m_sort_mode == sort_mode)
            m_sort_ascending = !m_sort_ascending;
        else
            m_sort_ascending = true;
        break;
    }
    }

    m_sort_mode = sort_mode;
    if (make_sort)
        RefreshList();
}

void CServerList::SrvInfo2LstSrvInfo(const ServerInfo* pServerInfo)
{
    m_itemInfo.info.server = pServerInfo->m_ServerName;
    xr_string address = pServerInfo->m_HostName;
    char port[8];
    address += "/port=";
    address += xr_itoa(pServerInfo->m_Port, port, 10);
    m_itemInfo.info.address = address.c_str();
    m_itemInfo.info.map = pServerInfo->m_SessionName;
    m_itemInfo.info.game = GameTypeToStringEx(pServerInfo->m_GameType, true);
    m_itemInfo.info.players.printf("%d/%d", pServerInfo->m_ServerNumPlayers, pServerInfo->m_ServerMaxPlayers);
    m_itemInfo.info.ping.printf("%d", pServerInfo->m_Ping);
    m_itemInfo.info.version = pServerInfo->m_ServerVersion;
    m_itemInfo.info.icons.pass = pServerInfo->m_bPassword;
    m_itemInfo.info.icons.dedicated = pServerInfo->m_bDedicated;
    m_itemInfo.info.icons.punkbuster = false; //	= pServerInfo->m_bPunkBuster;
    m_itemInfo.info.icons.user_pass = pServerInfo->m_bUserPass;

    m_itemInfo.info.Index = pServerInfo->Index;
}

void CServerList::SaveCurItem()
{
    CUIListItemServer* pItem = (CUIListItemServer*)m_list[LST_SERVER].GetSelectedItem();
    if (!pItem)
    {
        m_cur_item = -1;
        return;
    }
    R_ASSERT(pItem);
    m_cur_item = pItem->GetTAG();
}

void CServerList::RestoreCurItem()
{
    if (-1 == m_cur_item)
        return;

    m_list[LST_SERVER].SetSelectedTAG(m_cur_item);
    m_list[LST_SERVER].SetScrollPos(m_list[LST_SERVER].GetSelectedIDX());
}

void CServerList::ResetCurItem()
{
    m_list[LST_SERVER].SetSelectedIDX(u32(-1));
    m_list[LST_SERVER].ScrollToBegin();
}

void CServerList::DestroySrvItems()
{
    m_last_retreived_index = u32(-1);

    m_list[LST_SERVER].Clear();
    SrvItems_It it = m_items_cache.begin();
    SrvItems_It it_e = m_items_cache.end();

    for (; it != it_e; ++it)
        xr_delete((*it).m_ui_item);
}

void CServerList::ClearSrvItems()
{
    SrvItems_It it = m_items_cache.begin();
    SrvItems_It it_e = m_items_cache.end();
    for (; it != it_e; ++it)
        (*it).m_busy = false;

    m_last_retreived_index = u32(-1);
}

CUIListItemServer* CServerList::GetFreeItem()
{
    SrvItems_It it = m_items_cache.begin();
    SrvItems_It it_e = m_items_cache.end();

    if (m_last_retreived_index != u32(-1))
        std::advance(it, m_last_retreived_index);

    for (; it != it_e; ++it)
    {
        if (it->m_busy == false)
        {
            it->m_busy = true;
            m_last_retreived_index = (u32)(it - m_items_cache.begin());
            return it->m_ui_item;
        }
    }
    m_items_cache.push_back(SrvItem(m_list[LST_SERVER].GetItemHeight()));
    m_last_retreived_index = m_items_cache.size() - 1;
    return m_items_cache.back().m_ui_item;
}
