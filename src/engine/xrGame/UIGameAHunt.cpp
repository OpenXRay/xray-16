#include "stdafx.h"
#include "UIGameAHunt.h"

#include "team_base_zone.h"
#include "level.h"
#include "game_cl_ArtefactHunt.h"
#include "ui/UIStatic.h"
#include "ui/UIXmlInit.h"
#include "ui/UIMessageBoxEx.h"
#include "ui/UIMoneyIndicator.h"
#include "ui/UIRankIndicator.h"
#include "ui/UIHelper.h"
#include "UITeamPanels.h"
#include "object_broker.h"

#define TEAM_PANELS_AHUNT_XML_NAME "ui_team_panels_ahunt.xml"

CUIGameAHunt::CUIGameAHunt()
:m_game(NULL),m_pBuySpawnMsgBox(NULL)
{
}

void CUIGameAHunt::Init	(int stage)
{
	if(stage==0)
	{ // shared
		inherited::Init					(stage);
		m_buy_msg_caption				= UIHelper::CreateTextWnd(*m_msgs_xml, "mp_ah_buy", m_window);
	}
	if(stage==1)
	{ //unique
		m_pTeamPanels->Init				(TEAM_PANELS_AHUNT_XML_NAME, "team_panels_wnd");

		CUIXml							uiXml;
		uiXml.Load						(CONFIG_PATH, UI_PATH, "ui_game_ahunt.xml");

		CUIXmlInit::InitWindow			(uiXml, "global", 0,		m_window);
		CUIXmlInit::InitTextWnd			(uiXml, "fraglimit",0,		m_pFragLimitIndicator);

		m_pReinforcementInidcator		= xr_new<CUITextWnd>();
		m_pReinforcementInidcator->SetAutoDelete(true);
		CUIXmlInit::InitTextWnd			(uiXml, "reinforcement", 0, m_pReinforcementInidcator);		

		CUIXmlInit::InitStatic			(uiXml, "team1_icon", 0,	m_team1_icon);
		CUIXmlInit::InitStatic			(uiXml, "team2_icon", 0,	m_team2_icon);
		CUIXmlInit::InitTextWnd			(uiXml, "team1_score", 0,	m_team1_score);
		CUIXmlInit::InitTextWnd			(uiXml, "team2_score", 0,	m_team2_score);

		m_pMoneyIndicator->InitFromXML	(uiXml);
		m_pRankIndicator->InitFromXml	(uiXml);
	}
	if(stage==2)
	{ //after
		inherited::Init					(stage);
		m_window->AttachChild			(m_pReinforcementInidcator);
	}
};

void CUIGameAHunt::UnLoad()
{
	inherited::UnLoad	();
}

CUIGameAHunt::~CUIGameAHunt()
{
	delete_data			(m_pBuySpawnMsgBox);
}

void CUIGameAHunt::SetClGame (game_cl_GameState* g)
{
	inherited::SetClGame(g);
	m_game = smart_cast<game_cl_ArtefactHunt*>(g);
	R_ASSERT(m_game);
	//-----------------------------------------------------------------------
	delete_data							(m_pBuySpawnMsgBox);
	m_pBuySpawnMsgBox					= xr_new<CUIMessageBoxEx>();	
	m_pBuySpawnMsgBox->InitMessageBox	("message_box_buy_spawn");
	m_pBuySpawnMsgBox->SetText			("");

	game_cl_mp* clmp_game = smart_cast<game_cl_mp*>(g);
	//m_pBuySpawnMsgBox->AddCallback("msg_box", MESSAGE_BOX_YES_CLICKED, CUIWndCallback::void_function(clmp_game, &game_cl_mp::OnBuySpawn));
	m_pBuySpawnMsgBox->func_on_ok = CUIWndCallback::void_function(clmp_game, &game_cl_mp::OnBuySpawn);
}

void CUIGameAHunt::SetBuyMsgCaption(LPCSTR str)
{
	m_buy_msg_caption->SetTextST(str);
}