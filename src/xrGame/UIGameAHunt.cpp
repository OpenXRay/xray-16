#include "StdAfx.h"
#include "UIGameAHunt.h"

#include "team_base_zone.h"
#include "Level.h"
#include "game_cl_artefacthunt.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/ProgressBar/UIProgressShape.h"
#include "ui/UIXmlInit.h"
#include "ui/UIMessageBoxEx.h"
#include "ui/UIMoneyIndicator.h"
#include "ui/UIRankIndicator.h"
#include "ui/UIHelper.h"
#include "UITeamPanels.h"
#include "Common/object_broker.h"

#define TEAM_PANELS_AHUNT_XML_NAME "ui_team_panels_ahunt.xml"

CUIGameAHunt::CUIGameAHunt() : m_game(NULL), m_pBuySpawnMsgBox(NULL) {}
void CUIGameAHunt::Init(int stage)
{
    if (stage == 0)
    { // shared
        inherited::Init(stage);
        m_buy_msg_caption = UIHelper::CreateStatic(*MsgConfig, "mp_ah_buy", Window);
    }
    if (stage == 1)
    { // unique
        m_pTeamPanels->Init(TEAM_PANELS_AHUNT_XML_NAME, "team_panels_wnd");

        CUIXml uiXml, uiXml2;
        uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "ui_game_ahunt.xml");
        uiXml2.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "ui_game_dm.xml"); // CS

        CUIXmlInit::InitWindow(uiXml, "global", 0, Window, false);

        m_pReinforcementInidcator = UIHelper::CreateProgressShape(uiXml, "reinforcement", nullptr, false); // CS
        if (!m_pReinforcementInidcator)
            m_pReinforcementInidcator = UIHelper::CreateStatic(uiXml, "reinforcement", nullptr);
        m_pReinforcementInidcator->SetAutoDelete(true);

        if (!CUIXmlInit::InitStatic(uiXml, "fraglimit", 0, m_pFragLimitIndicator, false))
            CUIXmlInit::InitStatic(uiXml2, "fraglimit", 0, m_pFragLimitIndicator, false); // CS

        CUIXmlInit::InitStatic(uiXml, "team1_icon", 0, m_team1_icon);
        CUIXmlInit::InitStatic(uiXml, "team2_icon", 0, m_team2_icon);
        CUIXmlInit::InitStatic(uiXml, "team1_score", 0, m_team1_score);
        CUIXmlInit::InitStatic(uiXml, "team2_score", 0, m_team2_score);

        if (!m_pMoneyIndicator->InitFromXML(uiXml))
            m_pMoneyIndicator->InitFromXML(uiXml2); // CS
        if (!m_pRankIndicator->InitFromXml(uiXml))
            m_pRankIndicator->InitFromXml(uiXml2); // CS
    }
    if (stage == 2)
    { // after
        inherited::Init(stage);
        Window->AttachChild(m_pReinforcementInidcator);
    }
};

void CUIGameAHunt::UnLoad() { inherited::UnLoad(); }
CUIGameAHunt::~CUIGameAHunt() { delete_data(m_pBuySpawnMsgBox); }
void CUIGameAHunt::SetClGame(game_cl_GameState* g)
{
    inherited::SetClGame(g);
    m_game = smart_cast<game_cl_ArtefactHunt*>(g);
    R_ASSERT(m_game);
    //-----------------------------------------------------------------------
    delete_data(m_pBuySpawnMsgBox);
    m_pBuySpawnMsgBox = xr_new<CUIMessageBoxEx>();
    m_pBuySpawnMsgBox->InitMessageBox("message_box_buy_spawn");
    m_pBuySpawnMsgBox->SetText("");

    game_cl_mp* clmp_game = smart_cast<game_cl_mp*>(g);
    // m_pBuySpawnMsgBox->AddCallback("msg_box", MESSAGE_BOX_YES_CLICKED, CUIWndCallback::void_function(clmp_game,
    // &game_cl_mp::OnBuySpawn));
    m_pBuySpawnMsgBox->func_on_ok = CUIWndCallback::void_function(clmp_game, &game_cl_mp::OnBuySpawn);
}

void CUIGameAHunt::SetBuyMsgCaption(LPCSTR str) { m_buy_msg_caption->SetTextST(str); }

void CUIGameAHunt::SetReinforcementTimes(int curTime, int maxTime)
{
    if (auto shape = smart_cast<CUIProgressShape*>(m_pReinforcementInidcator))
        shape->SetPos(curTime, maxTime);
    else
    {
        string128 _buff;
        m_pReinforcementInidcator->SetText(xr_itoa(curTime, _buff, 10));
    }
}
