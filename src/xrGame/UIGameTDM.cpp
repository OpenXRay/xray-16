#include "stdafx.h"
#include "UIGameTDM.h"

#include "game_cl_base.h"

#include "game_cl_TeamDeathmatch.h"

#include "ui/TeamInfo.h"

#include "Common/object_broker.h"

#include "UITeamPanels.h"
#include "ui/UIMoneyIndicator.h"
#include "ui/UIRankIndicator.h"

#define MSGS_OFFS 510
#define TEAM_PANELS_TDM_XML_NAME "ui_team_panels_tdm.xml"

//--------------------------------------------------------------------
CUIGameTDM::CUIGameTDM() : m_game(NULL) {}
void CUIGameTDM::SetClGame(game_cl_GameState* g)
{
    inherited::SetClGame(g);
    m_game = smart_cast<game_cl_TeamDeathmatch*>(g);
    R_ASSERT(m_game);
}

void CUIGameTDM::Init(int stage)
{
    if (stage == 0)
    { // shared
        m_pUITeamSelectWnd = new CUISpawnWnd();
        m_team1_icon = new CUIStatic();
        m_team2_icon = new CUIStatic();
        m_team1_score = new CUITextWnd();
        m_team1_score->SetAutoDelete(true);
        m_team2_score = new CUITextWnd();
        m_team2_score->SetAutoDelete(true);
        m_buy_msg_caption = new CUITextWnd();
        m_buy_msg_caption->SetAutoDelete(true);

        inherited::Init(stage);
        CUIXmlInit::InitTextWnd(*MsgConfig, "mp_tdm_buy", 0, m_buy_msg_caption);
    }
    if (stage == 1)
    { // unique
        m_pTeamPanels->Init(TEAM_PANELS_TDM_XML_NAME, "team_panels_wnd");

        CUIXml uiXml, xml2;
        uiXml.Load(CONFIG_PATH, UI_PATH, "ui_game_tdm.xml");

        CUIXmlInit::InitWindow(uiXml, "global", 0, Window);
        CUIXmlInit::InitStatic(uiXml, "team1_icon", 0, m_team1_icon);
        CUIXmlInit::InitStatic(uiXml, "team2_icon", 0, m_team2_icon);
        CUIXmlInit::InitTextWnd(uiXml, "team1_score", 0, m_team1_score);
        CUIXmlInit::InitTextWnd(uiXml, "team2_score", 0, m_team2_score);
        CUIXmlInit::InitTextWnd(uiXml, "fraglimit", 0, m_pFragLimitIndicator);

        m_pMoneyIndicator->InitFromXML(uiXml);
        m_pRankIndicator->InitFromXml(uiXml);
    }
    if (stage == 2)
    { // after
        inherited::Init(stage);
        Window->AttachChild(m_team1_score);
        Window->AttachChild(m_team2_score);
        Window->AttachChild(m_buy_msg_caption);
    }
}

void CUIGameTDM::UnLoad()
{
    inherited::UnLoad();
    xr_delete(m_team1_icon);
    xr_delete(m_team2_icon);
    delete_data(m_pUITeamSelectWnd);
}

CUIGameTDM::~CUIGameTDM() {}
bool CUIGameTDM::IR_UIOnKeyboardPress(int dik)
{
    switch (dik)
    {
    case SDL_SCANCODE_CAPSLOCK:
    {
        if (m_game)
        {
            if (m_game->Get_ShowPlayerNamesEnabled())
                m_game->Set_ShowPlayerNames(!m_game->Get_ShowPlayerNames());
            else
                m_game->Set_ShowPlayerNames(true);
            return true;
        };
    }
    break;
    }
    return inherited::IR_UIOnKeyboardPress(dik);
}

bool CUIGameTDM::IR_UIOnKeyboardRelease(int dik)
{
    switch (dik)
    {
    case SDL_SCANCODE_CAPSLOCK:
    {
        if (m_game)
        {
            if (!m_game->Get_ShowPlayerNamesEnabled())
                m_game->Set_ShowPlayerNames(false);
            return true;
        };
    }
    break;
    }

    return inherited::IR_UIOnKeyboardRelease(dik);
}

void CUIGameTDM::OnFrame()
{
    inherited::OnFrame();
    m_team1_icon->Update();
    m_team2_icon->Update();
}

void CUIGameTDM::Render()
{
    m_team1_icon->Draw();
    m_team2_icon->Draw();
    inherited::Render();
}

void CUIGameTDM::SetScoreCaption(int t1, int t2)
{
    string32 str;
    xr_sprintf(str, "%d", t1);
    m_team1_score->SetText(str);

    xr_sprintf(str, "%d", t2);
    m_team2_score->SetText(str);

    m_pTeamPanels->SetArtefactsCount(t1, t2);
}

void CUIGameTDM::SetFraglimit(int local_frags, int fraglimit)
{
    string64 str;
    if (fraglimit)
        xr_sprintf(str, "%d", fraglimit);
    else
        xr_sprintf(str, "%s", "--");

    m_pFragLimitIndicator->SetText(str);
}

void CUIGameTDM::SetBuyMsgCaption(LPCSTR str)
{
    if (!str)
        m_buy_msg_caption->SetText("");
    else
        m_buy_msg_caption->SetTextST(str);
}
