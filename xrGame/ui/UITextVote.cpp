#include "StdAfx.h"
/*
#include "UITextVote.h"
#include "UIVotingCategory.h"
#include "UIXmlInit.h"
#include "UI3tButton.h"
#include "UIEditboxEx.h"

//#include "UIMapList.h"
#include "../level.h"
//#include "../game_cl_base.h"
#include "../game_cl_teamdeathmatch.h"
#include "../../xrEngine/xr_ioconsole.h"



CUITextVote::CUITextVote(){
	bkgrnd = xr_new<CUIStatic>(); bkgrnd->SetAutoDelete(true);
	AttachChild(bkgrnd);

	header = xr_new<CUIStatic>(); header->SetAutoDelete(true);
	AttachChild(header);

	edit = xr_new<CUIEditBoxEx>(); edit->SetAutoDelete(true);
	AttachChild(edit);

	btn_ok = xr_new<CUI3tButtonEx>(); btn_ok->SetAutoDelete(true);
	AttachChild(btn_ok);

	btn_cancel = xr_new<CUI3tButtonEx>(); btn_cancel->SetAutoDelete(true);
	AttachChild(btn_cancel);
}

void CUITextVote::InitTextVote(CUIXml& xml_doc)
{
	CUIXmlInit::InitWindow(xml_doc,			"text_vote", 0, this);
	CUIXmlInit::InitStatic(xml_doc,			"text_vote:header", 0, header);
	CUIXmlInit::InitStatic(xml_doc,			"text_vote:background", 0, bkgrnd);
	CUIXmlInit::InitEditBoxEx(xml_doc,		"text_vote:edit_box", 0, edit);
	CUIXmlInit::Init3tButtonEx(xml_doc,		"text_vote:btn_ok", 0, btn_ok);
	CUIXmlInit::Init3tButtonEx(xml_doc,		"text_vote:btn_cancel", 0, btn_cancel);
}

void CUITextVote::SendMessage(CUIWindow* pWnd, s16 msg, void* pData){
	if (BUTTON_CLICKED == msg)
	{
		if (pWnd == btn_ok)
			OnBtnOk();
		else if (pWnd == btn_cancel)
			OnBtnCancel();
	}
}

void CUITextVote::OnBtnOk(){
	LPCSTR name = edit->GetText();
	if (name && name[0])
	{
		string512 command;
		xr_sprintf(command, "cl_votestart $%s", name);		
		Console->Execute(command);
		game_cl_mp* game = smart_cast<game_cl_mp*>(&Game());
		game->StartStopMenu(this, true);
	}
	else
		return;
}

void CUITextVote::OnBtnCancel(){
    game_cl_mp* game = smart_cast<game_cl_mp*>(&Game());
	game->StartStopMenu(this, false);
}
*/