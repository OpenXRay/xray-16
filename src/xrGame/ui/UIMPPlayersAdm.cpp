#include "StdAfx.h"
#include "UIMPPlayersAdm.h"
#include "UIXmlInit.h"
#include "xrUICore/ListBox/UIListBox.h"
#include "xrUICore/ListBox/UIListBoxItem.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/TrackBar/UITrackBar.h"
#include "xrUICore/ComboBox/UIComboBox.h"
#include "Level.h"
#include "xrServer.h"
#include "game_cl_base.h"
#include "game_cl_mp.h"
#include "xrEngine/XR_IOConsole.h"
#include "string_table.h"
#include "xrCore/xr_token.h"

extern int g_sv_adm_menu_ping_limit;
extern const xr_token g_ban_times[] = {{"ui_mp_am_10_minutes", 600}, {"ui_mp_am_30_minutes", 1800}, {"ui_mp_am_1_hour", 3600},
    {"ui_mp_am_6_hours", 21600}, {"ui_mp_am_1_day", 86400}, {"ui_mp_am_1_week", 604800}, {"ui_mp_am_1_month", 2592000},
    {"ui_mp_am_3_monthes", 7776000}, {"ui_mp_am_forever", 999999999}, {0, 0}};

CUIMpPlayersAdm::CUIMpPlayersAdm()
{
    m_pPlayersList = new CUIListBox();
    m_pPlayersList->SetAutoDelete(true);
    AttachChild(m_pPlayersList);

    m_pRefreshBtn = new CUI3tButton();
    m_pRefreshBtn->SetAutoDelete(true);
    AttachChild(m_pRefreshBtn);

    m_pScreenAllBtn = new CUI3tButton();
    m_pScreenAllBtn->SetAutoDelete(true);
    AttachChild(m_pScreenAllBtn);

    m_pConfigAllBtn = new CUI3tButton();
    m_pConfigAllBtn->SetAutoDelete(true);
    AttachChild(m_pConfigAllBtn);

    m_pPingLimitBtn = new CUI3tButton();
    m_pPingLimitBtn->SetAutoDelete(true);
    AttachChild(m_pPingLimitBtn);

    m_pPingLimitTrack = new CUITrackBar();
    m_pPingLimitTrack->SetAutoDelete(true);
    AttachChild(m_pPingLimitTrack);

    m_pPingLimitText = new CUITextWnd();
    m_pPingLimitText->SetAutoDelete(true);
    AttachChild(m_pPingLimitText);

    m_pScreenPlayerBtn = new CUI3tButton();
    m_pScreenPlayerBtn->SetAutoDelete(true);
    AttachChild(m_pScreenPlayerBtn);

    m_pConfigPlayerBtn = new CUI3tButton();
    m_pConfigPlayerBtn->SetAutoDelete(true);
    AttachChild(m_pConfigPlayerBtn);

    m_pKickPlayerBtn = new CUI3tButton();
    m_pKickPlayerBtn->SetAutoDelete(true);
    AttachChild(m_pKickPlayerBtn);

    m_pBanPlayerBtn = new CUI3tButton();
    m_pBanPlayerBtn->SetAutoDelete(true);
    AttachChild(m_pBanPlayerBtn);

    m_pBanPlayerCombo = new CUIComboBox();
    m_pBanPlayerCombo->SetAutoDelete(true);
    AttachChild(m_pBanPlayerCombo);

    // m_pBanTimeTrack = new CUITrackBar();
    // m_pBanTimeTrack->SetAutoDelete(true);
    // AttachChild(m_pBanTimeTrack);

    // m_pBanTimeText = new CUITextWnd();
    // m_pBanTimeText->SetAutoDelete(true);
    // AttachChild(m_pBanTimeText);
}

CUIMpPlayersAdm::~CUIMpPlayersAdm() {}
void CUIMpPlayersAdm::Init(CUIXml& xml_doc)
{
    CUIXmlInit::InitWindow(xml_doc, "players_adm", 0, this);
    CUIXmlInit::InitListBox(xml_doc, "players_adm:players_list", 0, m_pPlayersList);
    CUIXmlInit::Init3tButton(xml_doc, "players_adm:refresh_button", 0, m_pRefreshBtn);
    CUIXmlInit::Init3tButton(xml_doc, "players_adm:screen_all_button", 0, m_pScreenAllBtn);
    CUIXmlInit::Init3tButton(xml_doc, "players_adm:config_all_button", 0, m_pConfigAllBtn);
    CUIXmlInit::Init3tButton(xml_doc, "players_adm:max_ping_limit_button", 0, m_pPingLimitBtn);
    CUIXmlInit::InitTrackBar(xml_doc, "players_adm:max_ping_limit_track", 0, m_pPingLimitTrack);
    CUIXmlInit::InitTextWnd(xml_doc, "players_adm:max_ping_limit_text", 0, m_pPingLimitText);
    CUIXmlInit::Init3tButton(xml_doc, "players_adm:screen_player_button", 0, m_pScreenPlayerBtn);
    CUIXmlInit::Init3tButton(xml_doc, "players_adm:config_player_button", 0, m_pConfigPlayerBtn);
    CUIXmlInit::Init3tButton(xml_doc, "players_adm:kick_player_button", 0, m_pKickPlayerBtn);
    CUIXmlInit::Init3tButton(xml_doc, "players_adm:ban_player_button", 0, m_pBanPlayerBtn);
    CUIXmlInit::InitComboBox(xml_doc, "players_adm:ban_player_combo", 0, m_pBanPlayerCombo);
    // CUIXmlInit::InitTrackBar(xml_doc, "players_adm:ban_time_track", 0, m_pBanTimeTrack);
    // CUIXmlInit::InitTextWnd(xml_doc, "players_adm:ban_time_text", 0, m_pBanTimeText);
    RefreshPlayersList();
    int min, max;
    g_sv_adm_menu_ping_limit = iCeil(Console->GetInteger("sv_max_ping_limit", min, max) / 10.0f);
    m_pPingLimitTrack->SetCurrentOptValue();
    SetMaxPingLimitText();
    m_pBanPlayerCombo->SetCurrentOptValue();
    m_pBanPlayerCombo->SetItemIDX(0);
    //	m_pBanTimeTrack->SetCurrentOptValue();
    //	SetBanSelPlayerText();
}

void CUIMpPlayersAdm::FillPlayersList(u32 const)
{
    m_pPlayersList->Clear();
    game_cl_GameState::PLAYERS_MAP_IT b = Game().players.begin();
    for (; b != Game().players.end(); b++)
    {
        //		if(b->first!=Game().local_svdpnid)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "%s, id:%u, ip:%s, ping:%u", b->second->getName(), b->first.value(),
                b->second->m_player_ip.c_str(), b->second->ping);

            CUIListBoxItem* itm = m_pPlayersList->AddTextItem(tmp_string);
            itm->SetTAG(b->first.value());
        }
    }
}

void CUIMpPlayersAdm::RefreshPlayersList()
{
    game_cl_mp* tmp_game = smart_cast<game_cl_mp*>(&Game());
    if (!tmp_game)
        return;

    tmp_game->RequestPlayersInfo(fastdelegate::FastDelegate<void(u32 const)>(this, &CUIMpPlayersAdm::FillPlayersList));
}

void CUIMpPlayersAdm::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    switch (msg)
    {
    case BUTTON_CLICKED:
    {
        if (pWnd == m_pRefreshBtn)
            RefreshPlayersList();
        else if (pWnd == m_pScreenAllBtn)
            Console->Execute("ra screenshot_all");
        else if (pWnd == m_pConfigAllBtn)
            Console->Execute("ra config_dump_all");
        else if (pWnd == m_pPingLimitBtn)
            SetMaxPingLimit();
        else if (pWnd == m_pPingLimitTrack)
            SetMaxPingLimitText();
        else if (pWnd == m_pScreenPlayerBtn)
            GetSelPlayerScreenshot();
        else if (pWnd == m_pConfigPlayerBtn)
            GetSelPlayerConfig();
        else if (pWnd == m_pKickPlayerBtn)
            KickSelPlayer();
        else if (pWnd == m_pBanPlayerBtn)
            BanSelPlayer();
        //			else if(pWnd==m_pBanTimeTrack)
        //				SetBanSelPlayerText();
        break;
    }
    };
}

void CUIMpPlayersAdm::SetMaxPingLimit()
{
    int ping_limit = m_pPingLimitTrack->GetIValue();
    string512 tmp_string;
    xr_sprintf(tmp_string, "ra sv_max_ping_limit %d", ping_limit * 10);
    Console->Execute(tmp_string);
}
void CUIMpPlayersAdm::SetMaxPingLimitText()
{
    int ping_limit = m_pPingLimitTrack->GetIValue();
    string512 tmp_string;
    xr_sprintf(tmp_string, "%s %d", StringTable().translate("ui_mp_am_ping_limit").c_str(), ping_limit * 10);
    m_pPingLimitText->SetText(tmp_string);
}
void CUIMpPlayersAdm::GetSelPlayerScreenshot()
{
    CUIListBoxItem* itm = m_pPlayersList->GetSelectedItem();
    if (!itm)
        return;

    u32 client_id = itm->GetTAG();
    string512 tmp_string;
    xr_sprintf(tmp_string, "ra make_screenshot %u", client_id);
    Console->Execute(tmp_string);
}
void CUIMpPlayersAdm::GetSelPlayerConfig()
{
    CUIListBoxItem* itm = m_pPlayersList->GetSelectedItem();
    if (!itm)
        return;

    u32 client_id = itm->GetTAG();
    string512 tmp_string;
    xr_sprintf(tmp_string, "ra make_config_dump %u", client_id);
    Console->Execute(tmp_string);
}
void CUIMpPlayersAdm::KickSelPlayer()
{
    CUIListBoxItem* itm = m_pPlayersList->GetSelectedItem();
    if (!itm)
        return;

    u32 client_id = itm->GetTAG();
    string512 tmp_string;
    xr_sprintf(tmp_string, "ra sv_kick_id %u", client_id);
    Console->Execute(tmp_string);
}
void CUIMpPlayersAdm::BanSelPlayer()
{
    CUIListBoxItem* itm = m_pPlayersList->GetSelectedItem();
    if (!itm)
        return;

    u32 client_id = itm->GetTAG();
    int ban_time = m_pBanPlayerCombo->CurrentID();
    //	int ban_time = m_pBanTimeTrack->GetIValue();
    string512 tmp_string;
    xr_sprintf(tmp_string, "ra sv_banplayer %u %d", client_id, ban_time);
    Console->Execute(tmp_string);
}

// void CUIMpPlayersAdm::SetBanSelPlayerText()
//{
//	int ban_time = m_pBanTimeTrack->GetIValue();
//	string512 tmp_string;
//	xr_sprintf(tmp_string, "Ban time (minutes): %d", ban_time);
//	m_pBanTimeText->SetText(tmp_string);
//}
