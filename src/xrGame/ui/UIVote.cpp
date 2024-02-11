#include "StdAfx.h"
#include "UIVote.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/ListBox/UIListBox.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "Level.h"
#include "game_cl_base.h"
#include "game_cl_teamdeathmatch.h"
#include "xrEngine/XR_IOConsole.h"

CUIVote::CUIVote() : CUIDialogWnd(CUIVote::GetDebugType())
{
    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "voting_category.xml");

    CUIXmlInit::InitWindow(xml_doc, "vote", 0, this);
    std::ignore = UIHelper::CreateStatic(xml_doc, "vote:background", this);
    std::ignore = UIHelper::CreateStatic(xml_doc, "vote:msg_back", this, false);
    msg         = UIHelper::CreateStatic(xml_doc, "vote:msg", this);

    string256 path;

    for (int i = 0; i < 3; i++)
    {
        xr_sprintf(path, "vote:list_cap_%d", i + 1);
        std::ignore = UIHelper::CreateStatic(xml_doc, path, this);

        xr_sprintf(path, "vote:list_back_%d", i + 1);
        std::ignore = UIHelper::CreateFrameWindow(xml_doc, path, this, false);

        xr_sprintf(path, "vote:list_%d", i + 1);
        list[i] = UIHelper::CreateListBox(xml_doc, path, this);
    }

    btn_yes    = UIHelper::Create3tButton(xml_doc, "vote:btn_yes", this);
    btn_no     = UIHelper::Create3tButton(xml_doc, "vote:btn_no", this);
    btn_cancel = UIHelper::Create3tButton(xml_doc, "vote:btn_cancel", this);
}

void CUIVote::SetVoting(LPCSTR txt) { msg->SetText(txt); }
void CUIVote::Update()
{
    CUIDialogWnd::Update();

    if (m_prev_upd_time > Device.dwTimeContinual - 1000)
        return;
    m_prev_upd_time = Device.dwTimeContinual;
    auto I = Game().players.begin();
    auto E = Game().players.end();

    using ItemVec = xr_vector<game_PlayerState*>;
    ItemVec items;
    for (; I != E; ++I)
        items.push_back(I->second);

    std::sort(items.begin(), items.end(), DM_Compare_Players);

    list[0]->Clear();
    list[1]->Clear();
    list[2]->Clear();

    for (u32 i = 0; i < items.size(); i++)
    {
        game_PlayerState* p = items[i];
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
    HideDialog();
}

void CUIVote::OnBtnNo()
{
    Console->Execute("cl_voteno");
    HideDialog();
}

void CUIVote::OnBtnCancel() { HideDialog(); }
