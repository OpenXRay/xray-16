#include "StdAfx.h"
#include "UIKickPlayer.h"
#include "UIVotingCategory.h"
#include "UIXmlInit.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/ListBox/UIListBox.h"
#include "xrUICore/ListBox/UIListBoxItem.h"
#include "xrUICore/SpinBox/UISpinNum.h"
#include "UIGameCustom.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "Level.h"
#include "game_cl_base.h"
#include "game_cl_teamdeathmatch.h"
#include "xrEngine/XR_IOConsole.h"

CUIKickPlayer::CUIKickPlayer()
{
    m_prev_upd_time = 0;
    bkgrnd = new CUIStatic();
    bkgrnd->SetAutoDelete(true);
    AttachChild(bkgrnd);

    header = new CUIStatic();
    header->SetAutoDelete(true);
    AttachChild(header);

    lst_back = new CUIFrameWindow();
    lst_back->SetAutoDelete(true);
    AttachChild(lst_back);

    m_ui_players_list = new CUIListBox();
    m_ui_players_list->SetAutoDelete(true);
    AttachChild(m_ui_players_list);

    btn_ok = new CUI3tButton();
    btn_ok->SetAutoDelete(true);
    AttachChild(btn_ok);

    btn_cancel = new CUI3tButton();
    btn_cancel->SetAutoDelete(true);
    AttachChild(btn_cancel);

    m_spin_ban_sec = new CUISpinNum();
    m_spin_ban_sec->SetAutoDelete(true);
    m_spin_ban_sec->SetMax(3000000);
    m_spin_ban_sec->SetMin(60);
    AttachChild(m_spin_ban_sec);

    m_ban_sec_label = new CUIStatic();
    m_ban_sec_label->SetAutoDelete(true);
    AttachChild(m_ban_sec_label);

    mode = MODE_KICK;
}

void CUIKickPlayer::Init_internal(CUIXml& xml_doc)
{
    CUIXmlInit::InitWindow(xml_doc, "kick_ban", 0, this);
    CUIXmlInit::InitStatic(xml_doc, "kick_ban:background", 0, bkgrnd);
    //	CUIXmlInit::InitFrameWindow	(xml_doc, "kick_ban:list_back",		0, lst_back);
    CUIXmlInit::InitListBox(xml_doc, "kick_ban:list", 0, m_ui_players_list);
    CUIXmlInit::Init3tButton(xml_doc, "kick_ban:btn_ok", 0, btn_ok);
    CUIXmlInit::Init3tButton(xml_doc, "kick_ban:btn_cancel", 0, btn_cancel);
    CUIXmlInit::InitSpin(xml_doc, "kick_ban:spin_ban_time", 0, m_spin_ban_sec);
    CUIXmlInit::InitStatic(xml_doc, "kick_ban:ban_time_lbl", 0, m_ban_sec_label);
}

void CUIKickPlayer::InitBan(CUIXml& xml_doc)
{
    CUIXmlInit::InitStatic(xml_doc, "kick_ban:header_ban", 0, header);
    Init_internal(xml_doc);
    mode = MODE_BAN;
    m_spin_ban_sec->Show(true);
    m_ban_sec_label->Show(true);
}

void CUIKickPlayer::InitKick(CUIXml& xml_doc)
{
    CUIXmlInit::InitStatic(xml_doc, "kick_ban:header_kick", 0, header);
    Init_internal(xml_doc);
    mode = MODE_KICK;
    m_spin_ban_sec->Show(false);
    m_ban_sec_label->Show(false);
}

bool CUIKickPlayer::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (dik == SDL_SCANCODE_ESCAPE)
    {
        OnBtnCancel();
        return true;
    }
    return CUIDialogWnd::OnKeyboardAction(dik, keyboard_action);
}

void CUIKickPlayer::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (LIST_ITEM_SELECT == msg && pWnd == m_ui_players_list)
    {
        CUIListBoxItem* itm = smart_cast<CUIListBoxItem*>(m_ui_players_list->GetSelected());
        m_selected_item_text = itm->GetText();
    }
    else if (BUTTON_CLICKED == msg)
    {
        if (pWnd == btn_ok)
            OnBtnOk();
        else if (pWnd == btn_cancel)
            OnBtnCancel();
    }
}

void CUIKickPlayer::OnBtnOk()
{
    CUIListBoxItem* item = smart_cast<CUIListBoxItem*>(m_ui_players_list->GetSelected());
    if (item)
    {
        string512 command;
        switch (mode)
        {
        case MODE_KICK: xr_sprintf(command, "cl_votestart kick %s", item->GetText()); break;
        case MODE_BAN: { xr_sprintf(command, "cl_votestart ban %s %d", item->GetText(), m_spin_ban_sec->Value());
        }
        break;
        }
        Console->Execute(command);
        HideDialog();
    }
    else
        return;
}

void CUIKickPlayer::OnBtnCancel() { HideDialog(); }
IC bool DM_Compare_Players(game_PlayerState* v1, game_PlayerState* v2);

using ItemVec = xr_vector<game_PlayerState*>;

void CUIKickPlayer::Update()
{
    CUIDialogWnd::Update();

    if (m_prev_upd_time > Device.dwTimeContinual - 1000)
        return;

    m_prev_upd_time = Device.dwTimeContinual;

    const game_cl_GameState::PLAYERS_MAP& items = Game().players;

    game_cl_GameState::PLAYERS_MAP_CIT I = items.begin();
    game_cl_GameState::PLAYERS_MAP_CIT E = items.end();

    bool bNeedRefresh = false;
    bool bHasSelected = false;

    xr_vector<game_PlayerState*>::iterator fit; //, fite;
    for (; I != E; ++I)
    {
        game_PlayerState* pI = I->second;
        if (m_selected_item_text.size() && !xr_stricmp(pI->getName(), m_selected_item_text.c_str()))
            bHasSelected = true;

        fit = std::find(m_current_set.begin(), m_current_set.end(), pI);
        if (fit == m_current_set.end())
            bNeedRefresh = true;
        else if (xr_stricmp((*fit)->getName(), pI->getName()))
            bNeedRefresh = true;
    }
    if (m_current_set.size() != items.size())
        bNeedRefresh = true;

    if (bNeedRefresh)
    {
        I = items.begin();
        m_ui_players_list->Clear();
        m_current_set.clear();

        for (; I != E; ++I)
        {
            game_PlayerState* p = I->second;
            m_current_set.push_back(p);
            m_ui_players_list->AddTextItem(p->getName());
        }
        if (bHasSelected)
            m_ui_players_list->SetSelectedText(m_selected_item_text.c_str());
    }
}
