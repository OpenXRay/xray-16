#include "StdAfx.h"
#include "UIVote.h"
#include "UIStatic.h"
#include "UI3tButton.h"
#include "UIListBox.h"
#include "UIFrameWindow.h"
#include "UIXmlInit.h"
#include "../level.h"
#include "../game_cl_base.h"
#include "../game_cl_teamdeathmatch.h"
#include "../../xrEngine/xr_ioconsole.h"

CUIVote::CUIVote()
{
	m_prev_upd_time		= 0;
	bkgrnd				= xr_new<CUIStatic>();	bkgrnd->SetAutoDelete(true);	AttachChild(bkgrnd);
	msg					= xr_new<CUITextWnd>();	msg->SetAutoDelete(true);		AttachChild(msg);

	for (int i = 0; i<3; i++)
	{
		cap[i]			= xr_new<CUITextWnd>();		cap[i]->SetAutoDelete(true);	AttachChild(cap[i]);
//		frame[i]		= xr_new<CUIFrameWindow>();	frame[i]->SetAutoDelete(true);	AttachChild(frame[i]);
		list[i]			= xr_new<CUIListBox>();		list[i]->SetAutoDelete(true);	AttachChild(list[i]);
	}	

	btn_yes		= xr_new<CUI3tButton>();	btn_yes->SetAutoDelete(true);		AttachChild(btn_yes);
	btn_no		= xr_new<CUI3tButton>();	btn_no->SetAutoDelete(true);		AttachChild(btn_no);
	btn_cancel	= xr_new<CUI3tButton>();	btn_cancel->SetAutoDelete(true);	AttachChild(btn_cancel);
	
	Init();
}

void CUIVote::Init()
{
	CUIXml xml_doc;
	xml_doc.Load			(CONFIG_PATH, UI_PATH, "voting_category.xml");
	CUIXmlInit::InitWindow	(xml_doc, "vote",				0, this);
	CUIXmlInit::InitStatic	(xml_doc, "vote:background",	0, bkgrnd);
	CUIXmlInit::InitTextWnd	(xml_doc, "vote:msg",			0, msg);

	string256 path;

	for (int i = 0; i<3; i++)
	{
		xr_sprintf						(path, "vote:list_cap_%d", i+1);
		CUIXmlInit::InitTextWnd			(xml_doc, path, 0, cap[i]);
//		xr_sprintf						(path, "vote:list_back_%d", i+1);
//		CUIXmlInit::InitFrameWindow		(xml_doc, path, 0, frame[i]);
		xr_sprintf						(path, "vote:list_%d", i+1);
		CUIXmlInit::InitListBox			(xml_doc, path, 0, list[i]);
	}	

	CUIXmlInit::Init3tButton(xml_doc, "vote:btn_yes", 0, btn_yes);
	CUIXmlInit::Init3tButton(xml_doc, "vote:btn_no", 0, btn_no);
	CUIXmlInit::Init3tButton(xml_doc, "vote:btn_cancel", 0, btn_cancel);
}

void CUIVote::SetVoting(LPCSTR txt)
{
	msg->SetText(txt);
}

void CUIVote::Update()
{
	CUIDialogWnd::Update();

	static string512 teaminfo;
	if (m_prev_upd_time > Device.dwTimeContinual - 1000)
		return;
	m_prev_upd_time = Device.dwTimeContinual;
	game_cl_GameState::PLAYERS_MAP_IT I=Game().players.begin();
	game_cl_GameState::PLAYERS_MAP_IT E=Game().players.end();

	DEFINE_VECTOR	(game_PlayerState*,ItemVec,ItemIt);
	ItemVec			items;
	for (;I!=E;++I)		
	{
		items.push_back(I->second);
	};

    std::sort(items.begin(), items.end(), DM_Compare_Players);

	list[0]->Clear();
	list[1]->Clear();
	list[2]->Clear();

	for (u32 i = 0; i<items.size(); i++){
		game_PlayerState* p					= items[i];
		if (p->m_bCurrentVoteAgreed == 1)
			list[0]->AddTextItem(p->getName());
		else if (p->m_bCurrentVoteAgreed == 0)
			list[1]->AddTextItem(p->getName());
		else
			list[2]->AddTextItem(p->getName());
	}
}

void CUIVote::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (BUTTON_CLICKED == msg)
	{
		if (btn_yes == pWnd)
			OnBtnYes();
		else if (btn_no == pWnd)
			OnBtnNo();
		else if (btn_cancel == pWnd)
			OnBtnCancel();
	}
}

void CUIVote::OnBtnYes()
{
    Console->Execute("cl_voteyes");
	HideDialog							();
}

void CUIVote::OnBtnNo()
{
    Console->Execute("cl_voteno");
	HideDialog							();
}

void CUIVote::OnBtnCancel()
{
	HideDialog							();
}
